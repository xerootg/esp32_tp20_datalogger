#include <stdint.h>

#ifndef VWTP_H
#define VWTP_H

typedef enum
{
  VWTP_CAN_TX_ERROR,
  VWTP_CAN_RX_TIMEOUT,
  VWTP_ERROR,
  VWTP_FRAME_ERROR,
  VWTP_MSG_TOO_LONG,
  VWTP_TX_MSG_TOO_LONG,
  VWTP_OK = 255
} VWTP_Result_t;

VWTP_Result_t VWTP_Connect();
VWTP_Result_t VWTP_Disconnect();
VWTP_Result_t VWTP_ACK();
VWTP_Result_t VWTP_KWP2000Message(uint8_t SID, uint8_t parameter, uint8_t * kwpMessage);


#endif //VWTP_H
