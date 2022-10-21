#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include "file_operations.h"

#include "can.h"
#include "vwtp.h"
#include "config.h"

uint16_t testerId = 0x300;
uint16_t ecuId = 0;
uint8_t nextSN = 0;
uint16_t delayT3 = 12;  //intermessage delay

uint8_t ecuT1, ecuT3;

volatile uint16_t timerVWTP = 0;

#define CAN_RX_TIMEOUT 500 //maximum time (ms) for receiving message from ecu
#define CAN_RX_GATEWAY_TIMEOUT 1000 //maximum time (ms) for receiving message from gateway

const TickType_t oneMs = 1 / portTICK_PERIOD_MS;

void timerVWTPTick(){
  timerVWTP--;
  vTaskDelay(oneMs);
}

// VWTP_OK
VWTP_Result_t VWTP_Connect()
{
  CanMessage_t msg;
  uint8_t i = 0;
  CanMessage_t VWTPInitMsg = {0x200, 7, {0x01, 0xC0, 0x00, 0x10, 0x00, 0x03, 0x01}};
  CanMessage_t VWTPTimingMsg = {0, 6, {0xA0, 0x0F, 0x8A, 0xFF, 0x32, 0xFF}};
  
  nextSN = 0;
  testerId = 0x300;
  ecuId = 0x000;
  
  printf("%s:%d resetting filter\n", __FILE__, __LINE__);
  CAN_ResetFilter0();

  if (!CAN_SendMsg(&VWTPInitMsg))
  {
    printf("%s:%d failed to send message\n", __FILE__, __LINE__);
    return VWTP_CAN_TX_ERROR; // cannot send msg
  }
  printf("%s:%d message sent\n", __FILE__, __LINE__);

  timerVWTP = CAN_RX_GATEWAY_TIMEOUT;
  printf("%s:%d waiting for can response\n", __FILE__, __LINE__);
  //Tick the clock until theres a message
  if (!CAN_ReceiveMsg(&msg, timerVWTP)){
    return VWTP_CAN_RX_TIMEOUT;
  };

  printf("%s:%d Got a message \n", __FILE__, __LINE__);

  if ( (msg.id == 0x201) && 
       (msg.len == 7) &&
       (((msg.payload[3]<<8) | msg.payload[2]) == testerId) &&
       (msg.payload[1] == 0xD0) ) //positive response
  {
    ecuId = (msg.payload[5]<<8) | msg.payload[4]; //address where you need to send messages to ecu
  }
  else
  {
    printf("%s:%d Bad VWTP packet - ID:%03x\n",  __FILE__, __LINE__, msg.id);
    return VWTP_FRAME_ERROR;
  }
  printf("%s:%d destination address: %x - \n", __FILE__, __LINE__, ecuId);
  
  memmove(&msg, &VWTPTimingMsg, sizeof(VWTPTimingMsg));
  msg.id = ecuId;
  
  /*timerVWTP = delayT3;
  while (timerVWTP){
    timerVWTPTick();
  };*/
  vTaskDelay(delayT3 / portTICK_PERIOD_MS);

  if (!CAN_SendMsg(&msg))
  {
    return VWTP_CAN_TX_ERROR; // cannot send msg
  }
  
  CAN_SetFilter0(testerId);
  i = 3;
  do //protection if there were still frames with a different id than testerId in the fifo
  {
    if (!CAN_ReceiveMsg(&msg, CAN_RX_GATEWAY_TIMEOUT))
    {
      return VWTP_CAN_RX_TIMEOUT;
    }

    
  } while ((msg.id != testerId) && --i);
  
  if (0 == i)
  {
    return VWTP_FRAME_ERROR;
  }

  if ((msg.len != 6) || (msg.payload[0] != 0xA1))
  {
    return VWTP_FRAME_ERROR;
  }
  
  ecuT1 = msg.payload[2];
  ecuT3 = msg.payload[4];

  return VWTP_OK;
}


// VWTP_OK
VWTP_Result_t VWTP_Disconnect()
{
  CanMessage_t msg;
  
  msg.id = ecuId;
  msg.len = 1;
  msg.payload[0] = 0xA8; //disconnect

  timerVWTP = delayT3;
  while (timerVWTP){
    timerVWTPTick();
  };

  if (!CAN_SendMsg(&msg))
  {
    return VWTP_CAN_TX_ERROR; // cannot send msg
  }
  
  return VWTP_OK;
}


// VWTP_OK
VWTP_Result_t VWTP_ACK()
{
  CanMessage_t msg;
  
  msg.id = ecuId;
  msg.len = 1;
  msg.payload[0] = 0xA3; //connection test

  timerVWTP = delayT3;
  while (timerVWTP){
    timerVWTPTick();
  };

  if (!CAN_SendMsg(&msg))
  {
    return VWTP_CAN_TX_ERROR; // cannot send msg
  }
  
  if (!CAN_ReceiveMsg(&msg,CAN_RX_TIMEOUT))
  {
    return VWTP_CAN_RX_TIMEOUT;
  }

  
  if ((msg.len != 6) || (msg.payload[0] != 0xA1))
  {
    return VWTP_FRAME_ERROR;
  }

  return VWTP_OK;
}


typedef enum
{
  SEND_REQUEST, // next state: RECEIVE_ACK
  RECEIVE_ACK, //next state: RECEIVE_FIRST_MSG
  REQUEST_PENDING_RECEIVED_SEND_ACK, //next state: RECEIVE_FIRST_MSG
  RECEIVE_FIRST_MSG,
  RECEIVE_NEXT_MSG, //next state: LAST_MSG_RECEIVED_SEND_ACK
  LAST_MSG_RECEIVED_SEND_ACK,
  FINISHED
} VWTP_State_t;


#define MAXIMUM_MESSAGE_LENGTH 120

// VWTP_OK
VWTP_Result_t VWTP_KWP2000Message(uint8_t SID, uint8_t parameter, uint8_t * kwpMessage)
{
  VWTP_Result_t errorCode = VWTP_OK;
  VWTP_State_t state = SEND_REQUEST;
  uint8_t frameNumber = -1;
  CanMessage_t msg;
  uint8_t * buf_p = kwpMessage + 1;
  uint8_t i;

  if (kwpMessage[0] > 3)
  {
    printf("%s:%d frame too long error\n", __FILE__, __LINE__);
    return VWTP_TX_MSG_TOO_LONG;
  }

  while ((state != FINISHED) && (errorCode == VWTP_OK))
  {
    if ((buf_p-kwpMessage) > MAXIMUM_MESSAGE_LENGTH)
    {
      printf("%s:%d frame too long error\n", __FILE__, __LINE__);
      errorCode = VWTP_MSG_TOO_LONG;
      break;
    }

    switch (state)
    {
      case SEND_REQUEST:
        msg.id = ecuId;
        msg.len = 5 + kwpMessage[0];
        msg.payload[0] = 0x10 | nextSN;
        msg.payload[1] = 0x00;
        msg.payload[2] = 0x02 + kwpMessage[0]; //KWP2000, block length
        msg.payload[3] = SID; 
        msg.payload[4] = parameter;
        for (i=0; i<kwpMessage[0]; i++)
        {
          msg.payload[5+i] = kwpMessage[1+i];
        }
        kwpMessage[0] = 0;

        timerVWTP = delayT3;
        while (timerVWTP){
          timerVWTPTick();
        };
        
        if (!CAN_SendMsg(&msg))
        {
          errorCode = VWTP_CAN_TX_ERROR; // cannot send msg
          break;
        }
        nextSN = (nextSN + 1) & 0x0F;
        state = RECEIVE_ACK;
        break;
    
      case RECEIVE_ACK:
        if (!CAN_ReceiveMsg(&msg,CAN_RX_TIMEOUT))
        {
          errorCode = VWTP_CAN_RX_TIMEOUT;
          break;
        }

        if (msg.payload[0] == (0xB0 | nextSN))
        {
          state = RECEIVE_FIRST_MSG;
        }
        else
        {
          errorCode = VWTP_FRAME_ERROR;
          printf("%s:%d frame error\n", __FILE__, __LINE__);
          break;
        }
        break;
        
      case RECEIVE_FIRST_MSG:
        if (!CAN_ReceiveMsg(&msg, CAN_RX_TIMEOUT))
        {
          errorCode = VWTP_CAN_RX_TIMEOUT;
          break;
        }

        frameNumber = (msg.payload[0]+1) & 0x0F; //save next frame number 

        if ((msg.payload[0] & 0xF0) == 0x10)
        {
          if ( (msg.len == 6) &&
               (msg.payload[3] == 0x7F) &&
               (msg.payload[4] == SID) &&
               (msg.payload[5] == 0x78) )
          {
            state = REQUEST_PENDING_RECEIVED_SEND_ACK;
            //printf("%s:%d response pending\n", __FILE__, __LINE__);
          }
          else 
          {
            memmove(buf_p, msg.payload + 1, msg.len - 1);
            kwpMessage[0] += msg.len - 1;
            buf_p += msg.len-1;
            state = LAST_MSG_RECEIVED_SEND_ACK;
          }
        }
        else if ( ((msg.payload[0] & 0xF0) == 0x20) &&
                  (msg.payload[3] == (0x40 + SID)) &&
                  (msg.payload[4] == parameter) )
        {
          memmove(buf_p, msg.payload + 1, msg.len - 1);
          kwpMessage[0] += msg.len - 1;
          buf_p += msg.len-1;
          state = RECEIVE_NEXT_MSG;
        }
        else
        {
          printf("%s:%d frame error\n", __FILE__, __LINE__);
          errorCode = VWTP_FRAME_ERROR;
          break;
        }
        break;
        
      case RECEIVE_NEXT_MSG:
        if (!CAN_ReceiveMsg(&msg, CAN_RX_TIMEOUT))
        {
          errorCode = VWTP_CAN_RX_TIMEOUT;
          break;
        }
        
        if ( ((msg.payload[0] & 0xF0) == 0x10) &&
             ((msg.payload[0] & 0x0F) == frameNumber) )
        {
          memmove(buf_p, msg.payload + 1, msg.len - 1);
          kwpMessage[0] += msg.len - 1;
          buf_p += msg.len-1;
          frameNumber = (frameNumber + 1) & 0x0F;
          state = LAST_MSG_RECEIVED_SEND_ACK;
          //printf("%s:%d got last frame #%d\n", __FILE__, __LINE__,frameNumber);
        }
        else if ( ((msg.payload[0] & 0xF0) == 0x20) &&
                  ((msg.payload[0] & 0x0F) == frameNumber) )
        {
          frameNumber = (frameNumber + 1) & 0x0F;
          memmove(buf_p, msg.payload + 1, msg.len - 1);
          kwpMessage[0] += msg.len - 1;
          buf_p += msg.len-1;
          state = RECEIVE_NEXT_MSG;
          //printf("%s:%d got frame #%d\n", __FILE__, __LINE__,frameNumber);
        }
        else
        {
          errorCode = VWTP_FRAME_ERROR;
          printf("%s:%d frame error at #%d\n", __FILE__, __LINE__,frameNumber);
          break;
        }
        break;
        
      case LAST_MSG_RECEIVED_SEND_ACK:
        msg.id = ecuId;
        msg.len = 1;
        msg.payload[0] = 0xB0 | frameNumber;

        timerVWTP = delayT3;
        while (timerVWTP){
          timerVWTPTick();
        };
        
        if (!CAN_SendMsg(&msg))
        {
          errorCode = VWTP_CAN_TX_ERROR; // cannot send msg
          break;
        }
        state = FINISHED;
        break;
        
      case REQUEST_PENDING_RECEIVED_SEND_ACK:
        msg.id = ecuId;
        msg.len = 1;
        msg.payload[0] = 0xB0 | frameNumber;

        timerVWTP = delayT3;
        while (timerVWTP){
          timerVWTPTick();
        };
        
        if (!CAN_SendMsg(&msg))
        {
          errorCode = VWTP_CAN_TX_ERROR; // cannot send msg
          break;
        }
      
        state = RECEIVE_FIRST_MSG;
        break;
      case FINISHED: //making the compiler happy. This should never happen, since we WHILE NOT FINISHED above this switch :(
        break;
    }
  }
  
  return errorCode;
}
