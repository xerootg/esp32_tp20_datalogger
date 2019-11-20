#ifndef __CAN_FUNCTIONS_H__
#define __CAN_FUNCTIONS_H__

typedef struct
{
  uint16_t id;
  uint8_t  len;
  uint8_t  payload[8];
} CanMessage_t;

void CAN_Open();
void CAN_Close();
uint8_t CAN_ReceiveMsg(CanMessage_t * msg);
uint8_t CAN_SendMsg(CanMessage_t * msg);
uint8_t CAN_MessagePending();
void CAN_SetFilter0(uint32_t id);
void CAN_ResetFilter0();
void CAN_FlushReceiveFifo();
void CAN_Debug(uint8_t on, FILE *debug_file);
void CAN_UseStdId();
void CAN_UseExtId();

#endif