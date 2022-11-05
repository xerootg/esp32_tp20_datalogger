#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "diag.h"
#include "file_operations.h"
#include "vwtp.h"
#include "can.h"
#include "config.h"

extern volatile uint16_t timeSec;
extern volatile uint8_t time10MSec;

uint8_t Byte2HexStr(char * s, uint8_t val)
{
  const char * hexArray = "0123456789abcdef";
  s[0] = hexArray[val/16];
  s[1] = hexArray[val%16];
  return 2;
}


uint8_t DecodeUnits(char *p, uint8_t *frameData)
{
  char * p_saved = p;
  int i;
  uint8_t len;
  
  for (i=0; i<12; i+=3)
  {
    switch (frameData[i])
    {
      case 1: 
        p += sprintf(p, "rpm");
        break;

      case 2: 
      case 20: 
      case 23:
      case 33:
        p += sprintf(p, "%%");
        break;

      case 3: 
      case 9: 
      case 27: 
      case 34: 
      case 67:
        p += sprintf(p, "deg");
        break;

      case 5: 
      case 31:
        p += sprintf(p, "�C");
        break;
        
      case 6: 
      case 21:
      case 43:
      case 66:
        p += sprintf(p, "V");
        break;
        
      case 7: 
        p += sprintf(p, "km/h");
        break;

      case 12: 
      case 64:
        p += sprintf(p, "ohm");
        break;

      case 13: 
      case 65:
        p += sprintf(p, "mm");
        break;

      case 14: 
      case 69:
        p += sprintf(p, "bar");
        break;

      case 15:
      case 22:
      case 47:
        p += sprintf(p, "ms");
        break;
        
      case 16: 
        p += sprintf(p, "bitval");
        break;
        
      case 18: 
      case 50: 
        p += sprintf(p, "mbar");
        break;
                
      case 19: 
        p += sprintf(p, "l");
        break;
                
      case 24:
      case 40:
        p += sprintf(p, "A");
        break;
             
      case 25: 
      case 53: 
        p += sprintf(p, "g/s");
        break;
                
      case 26: 
        p += sprintf(p, "C");
        break;
                        
      case 35: 
        p += sprintf(p, "l/h");
        break;

      case 36: 
        p += sprintf(p, "km");
        break;

      case 37: 
        p += sprintf(p, "state");
        break;
      
      case 30:
      case 38:
      case 46: 
        p += sprintf(p, "deg k/w");
        break;

      case 41: 
        p += sprintf(p, "Ah");
        break;

      case 42: 
        p += sprintf(p, "Kw");
        break;

      case 44: 
        p += sprintf(p, "h:m");
        break;

      case 39:
      case 49: 
      case 51:
        p += sprintf(p, "mg/h");
        break;
        
      case 52: 
        p += sprintf(p, "Nm");
        break;
       
      case 54: 
        p += sprintf(p, "count");
        break;
       
      case 55: 
        p += sprintf(p, "s");
        break;
      
      case 56: 
      case 57:
        p += sprintf(p, "WSC");
        break;
              
      case 60: 
        p += sprintf(p, "sec");
        break;

      case 62: 
        p += sprintf(p, "S");
        break;
      
      case 68: 
        p += sprintf(p, "deg/s");
        break;

      case 70: 
        p += sprintf(p, "m/s^2");
        break;

      default:
        break;
    }
    *p++ = ';';
  }

  len = p - p_saved;
  return len;
}


uint8_t DecodeFrame(char *p, uint8_t *frameData)
{
  char * p_saved = p;

  uint16_t val_u16;
  int16_t val_s16;
  float f;
  int i, j, len;
  
  for (i=0; i<12; i+=3)
  {
    switch (frameData[i])
    {
    
      case 1: //0.2*a*b rpm
        val_u16 = frameData[i+1] * frameData[i+2];
        val_u16 /= 5;
        p += sprintf(p, "%d", val_u16);
        break;

      case 2: //a*0.002*b  	%
      case 3: //0.002*a*b  	Deg
        f = frameData[i+1] * frameData[i+2];
        f *= 0.002;
        p += sprintf(p, "%.1f", f);
        break;

      case 4: // abs(b-127)*0.01*a  	"ATDC" if Value >127, else"BTDC"
        val_u16 = abs(frameData[i+2] - 127);
        val_u16 *= frameData[i+1];
        val_u16 /= 100;
        p += sprintf(p, "%d", val_u16);
        break;
                
      case 5: // a*(b-100)*0.1 �C
        val_s16 = frameData[i+1] * (frameData[i+2] - 100);
        val_s16 /= 10;
        p += sprintf(p, "%d", val_s16);
        break;
      
      case 6:  //0.001*a*b  	V
      case 12: //0.001*a*b  	Ohm
      case 21: //0.001*a*b  	V
      case 22: //0.001*a*b  	ms
      case 24: //0.001*a*b  	A
        f = frameData[i+1] * frameData[i+2];
        f *= 0.001;
        p += sprintf(p, "%.2f", f);
        break;
      
      case 7: // 0.01*a*b km/h
        val_u16 = frameData[i+1] * frameData[i+2];
        val_u16 /= 100;
        p += sprintf(p, "%d", val_u16);
        break;
        
      case 8: //0.1*a*b  	(no units)
        f = frameData[i+1] * frameData[i+2];
        f *= 0.1;
        p += sprintf(p, "%.1f", f);
        break;

      case 9: //(b-127)*0.02*a  	Deg
        f = (frameData[i+2] - 127.0) * frameData[i+1];
        f *= 0.02;
        p += sprintf(p, "%.1f", f);
        break;
        
      case 10:
        if (0 == frameData[i+2])
        {
          p += sprintf(p, "COLD");
        }
        else
        {
          p += sprintf(p, "WARM");
        }
        break;
        
      case 11: //0.0001*a*(b-128)+1  -
        f = frameData[i+1] * (frameData[i+2] - 128.0);
        f *= 0.0001;
        f += 1;
        p += sprintf(p, "%.1f", f);
        break;

      case 13: //(b-127)*0.001*a  	mm
        f = (frameData[i+2] - 127.0) * frameData[i+1];
        f *= 0.001;
        p += sprintf(p, "%.1f", f);
        break;

      case 14: //0.005*a*b  	bar
        f = frameData[i+2] * frameData[i+1];
        f *= 0.005;
        p += sprintf(p, "%.1f", f);
        break;
        
      case 15: //0.01*a*b ms
      case 19: //a*b*0.01  	l
        f = frameData[i+1] * frameData[i+2];
        f *= 0.01;
        p += sprintf(p, "%.2f", f);
        break;
        
      case 16: //bitvalue
        for (j = 128; j > 0; j = j>>1)
        {
          if (frameData[i+1] & j)
          {
            *p++ = (frameData[i+2] & j)?'1':'0';
          }
          else
          {
            *p++ = 'X';
          }
        }
        break;
        
      case 17: //chr(a) + chr(b) 
        p += sprintf(p, "%c%c", frameData[i+1], frameData[i+2]);
        break;
        
      case 18:  //0.04*a*b  mbar
        val_u16 = frameData[i+1] * frameData[i+2];
        val_u16 /= 25;
        p += sprintf(p, "%d", val_u16);
        break;
        
      case 20:  //	a*(b-128)/128  	%
        f = frameData[i+1] * (frameData[i+2] - 128);
        f /= 128.0;
        p += sprintf(p, "%.2f", f);
        break;
        
      case 23: //23  	EGR Valve, Duty Cycle / Inj. Timing ???  	   	b/256*a  	%
        f = frameData[i+2] / 256.0;
        f *= frameData[i+1];
        p += sprintf(p, "%.1f", f);
        break;
        
      case 25: //  	(b*1.421)+(a/182)  	g/s
        f = (frameData[i+1] / 182.0) + (1.421 * frameData[i+2]);
        p += sprintf(p, "%.2f", f);
        break;

      case 26: //b-a  	C
      case 28:
        val_s16 = frameData[i+2] - frameData[i+1];
        p += sprintf(p, "%d", val_s16);
        break;
       
      case 27: //abs(b-128)*0.01*a 
        f = (frameData[i+2] - 128.0);
        if (f < 0)
        {
          f = f * (-1);
        }
        f *= frameData[i+1];
        f *= 0.01;
        p += sprintf(p, "%.1f", f);
        break;

      case 30: //b/12*a  	Deg k/w
        f = frameData[i+2] / 12.0;
        f *= frameData[i+1];
        p += sprintf(p, "%.1f", f);
        break;

      case 31: //b/2560*a  	�C
        f = frameData[i+2] / 2560.0;
        f *= frameData[i+1];
        p += sprintf(p, "%.1f", f);
        break;
        
      case 33: // 100*b/a  (if a==0 then 100*b)  	%
        if (frameData[i+1] == 0)
        {
          val_u16 = 100 * frameData[i+2];
        }
        else
        {
          val_u16 = (100 * frameData[i+2])/frameData[i+1];
        }
        p += sprintf(p, "%d", val_u16);
        break;
      
      case 34: // (b-128)*0.01*a  	kW
        f = (frameData[i+2] - 128.0) * frameData[i+1];
        f *= 0.01;
        p += sprintf(p, "%.1f", f); 
        break;
        
      case 35: // 0.01*a*b  	l/h
        f = frameData[i+2] * frameData[i+1];
        f *= 0.01;
        p += sprintf(p, "%.1f", f); 
        break;        

      case 37:
        switch (frameData[i+2])
        {
          case 0x00:
            p += sprintf(p, "-");
            break;

          case 0x02:
            p += sprintf(p, "ADP OK"); 
            break;
                      
          case 0x05:
            p += sprintf(p, "Idle"); 
            break;
            
          case 0x06:
            p += sprintf(p, "Partial thr"); 
            break;
            
          case 0x07:
            p += sprintf(p, "WOT"); 
            break;

          case 0x08:
            p += sprintf(p, "Enrichment"); 
            break;

          case 0x09:
            p += sprintf(p, "Deceleration"); 
            break;

          case 0x0E:
            p += sprintf(p, "A/C low"); 
            break;

          case 0x10:
            p += sprintf(p, "Compr. OFF"); 
            break;

          case 0xD6:
            p += sprintf(p, "Htg. S1 0xD6"); 
            break;

          case 0xD7:
            p += sprintf(p, "Htg. S1 0xD7"); 
            break;

          case 0xD9:
            p += sprintf(p, "Htg. S2 0xD9"); 
            break;

          case 0xEB:
            p += sprintf(p, "Test OFF"); 
            break;

          default:
            p += sprintf(p, "0x%02x", frameData[i+2]); 
        }
        break;

      case 38: //(b-128)*0.001*a  	Deg k/w
        f = (frameData[i+2] - 128.0);
        f *= frameData[i+1];
        f *= 0.001;
        p += sprintf(p, "%.1f", f); 
        break;
              
      case 39: //b/256*a  mg/h
        f = frameData[i+2] / 256.0;
        f *= frameData[i+1];
        p += sprintf(p, "%.1f", f); 
        break;

      case 43: // 	b*0.1+(25.5*a)  	V
        f = frameData[i+2] * 0.1;
        f += 25.5 * frameData[i+1];
        p += sprintf(p, "%.2f", f); 
        break;
              
      case 44: //a : b  	h:m
        p += sprintf(p, "%d:%d", frameData[i+1], frameData[i+2]);
        break;        

      case 45: //0.1*a*b/100  	 
        f = frameData[i+2] * frameData[i+1];
        f /= 1000.0;
        p += sprintf(p, "%.3f", f); 
        break;

      case 46: //(a*b-3200)*0.0027  	Deg k/w
        f = frameData[i+2] * frameData[i+1];
        f -= 3200.0;
        f *= 0.0027;
        p += sprintf(p, "%.1f", f); 
        break;

      case 47: //(b-128)*a  	ms
        val_s16 = (frameData[i+2] - 128) * frameData[i+1];
        p += sprintf(p, "%d", val_s16); 
        break;

      case 49: //(b/4)*a*0,1  mg/h
        f = (frameData[i+2] / 4.0);
        f *= frameData[i+1];
        f *= 0.1;
        p += sprintf(p, "%.1f", f); 
        break;
      
      case 50: // (b-128)/(0.01*a), if a==0 (b-128)/0.01  	mbar
        f = (frameData[i+2] - 128.0);
        f /= 0.01;
        if (frameData[i+1] != 0)
        {
          f /= frameData[i+1];
        }
        p += sprintf(p, "%.1f", f); 
        break;

      case 51: //((b-128)/255)*a  	mg/h
        f = frameData[i+2] - 128.0;
        f /= 255.0;
        f *= frameData[i+1];
        p += sprintf(p, "%.1f", f); 
        break;

      case 52: // b*0.02*a-a  	Nm
        val_u16 = frameData[i+1] * frameData[i+2];
        val_u16 /= 50;
        val_u16 -= frameData[i+1];
        p += sprintf(p, "%d", val_u16);
        break;

      case 53: // 53  	Luftdurchflu� Luftmassenmesser (???)  	   	(b-128)*1.4222+0.006*a  	g/s
        f = (frameData[i+2] - 128.0) * 1.4222;
        f += frameData[i+1] * 0.006;
        p += sprintf(p, "%.1f", f); 
        break;

      case 48: //b+a*255  	-      
      case 54: // 	a*256+b  	Count
        val_u16 = frameData[i+1] * 256;
        val_u16 += frameData[i+2];
        p += sprintf(p, "%d", val_u16);
        break;

      case 55: //a*b/200  	s
        f = frameData[i+1] * frameData[i+2];
        f /= 200.0;
        p += sprintf(p, "%.1f", f);
        break;

      case 56: //a*256+b  	WSC
        val_u16 = 256 * frameData[i+1] + frameData[i+2];
        p += sprintf(p, "%d", val_u16);
        break;

      case 59: //(a*256+b)/32768  	-
        f = 256.0 * frameData[i+1] + frameData[i+2];
        f = 32768.0;
        p += sprintf(p, "%.2f", f);
        break;

      case 60: //(a*256+b)*0.01  	sec
        f = 256.0 * frameData[i+1] + frameData[i+2];
        f *= 0.01;
        p += sprintf(p, "%.1f", f);
        break;

      case 61: // (b-128)/a, if a==0 (b-128)  -
        f = frameData[i+2] - 128.0;
        if (frameData[i+1] != 0)
        {
          f /= frameData[i+1];
        }
        p += sprintf(p, "%.2f", f); 
        break;

      case 62: //0.256*a*b  	S
        f = frameData[i+1] * frameData[i+2];
        f *= 0.256;
        p += sprintf(p, "%.1f", f);
        break;
        
      case 63: //chr(a) + chr(b) + "?"  -
        p += sprintf(p, "%c%c?", frameData[i+1], frameData[i+2]);
        break;

      case 64: //a+b  	Ohm
        val_u16 = frameData[i+1] + frameData[i+2];
        p += sprintf(p, "%d", val_u16);
        break;

      case 65: //0.01*a*(b-127)  	mm
        f = 0.01 * frameData[i+1];
        f *= frameData[i+2] - 127.0;
        p += sprintf(p, "%.2f", f);
        break;

      case 66: //(a*b)/511.12  	V
        f = frameData[i+1] * frameData[i+2];
        f /= 511.12;
        p += sprintf(p, "%.2f", f);
        break;

      case 67: //(640*a)+b*2.5  	Deg
        f = 640.0 * frameData[i+1];
        f += 2.5 * frameData[i+2];
        p += sprintf(p, "%.1f", f);
        break;

      case 68: //(256*a+b)/7.365  	deg/s
        f = 256.0 * frameData[i+1] + frameData[i+2];
        f /= 7.365;
        p += sprintf(p, "%.2f", f);
        break;

      case 69: //(256*a +b)*0.3254  	Bar
        f = 256.0 * frameData[i+1] + frameData[i+2];
        f *= 0.3254;
        p += sprintf(p, "%.2f", f);
        break;

      case 70: //(256*a +b)*0.192  	m/s^2
        f = 256.0 * frameData[i+1] + frameData[i+2];
        f *= 0.192;
        p += sprintf(p, "%.2f", f);
        break;

      default:
        *p++ = '0';
        *p++ = 'x';
        p += Byte2HexStr(p, frameData[i]);
        p += Byte2HexStr(p, frameData[i+1]);
        p += Byte2HexStr(p, frameData[i+2]);
    }
    *p++ = ';';
  }

  len = p - p_saved;
  return len;
}

// return 0 - terminated by user
// return 1 - vwtp error
// return 2 - filesystem error

uint8_t vwtp(char *config, FILE *log_file, uint8_t vwtpDebug) 
{
  uint8_t groupNumber = config[1];
  uint8_t groupPos = 1;
  char currentLogLine[150];
  char *currentLogLine_p = currentLogLine;
  uint8_t errorCode = 1; //default to vwtp error
  uint8_t kwp[128];
  VWTP_Result_t result;

  
  char units_buf[150];
  char *units_buf_p = units_buf;
  
  char file_header[120];
  char *file_header_p = file_header;
  uint8_t i, j;
  
  CAN_FlushReceiveFifo();

  // 1. Connect with ECU
  result = VWTP_Connect();
  if (result != VWTP_OK)
  {
    return 11;
  }

  // 2. Start Diagnostic Session
  kwp[0] = 0;
  result =  VWTP_KWP2000Message(0x10, 0x89, kwp);
  if (result != VWTP_OK)
  {
    return 12;
  }

  if ( (kwp[0] != 4) || (kwp[2] != 2) || (kwp[3] != 0x50) || (kwp[4] != 0x89) )
  {
    return 13;
  }

  // 3. ACK
  result = VWTP_ACK();
  if (result != VWTP_OK)
  {
    return 14;
  }
  
  // 4. ReadEcuId
  kwp[0] = 0;
  result =  VWTP_KWP2000Message(0x1A, 0x9B, kwp);
  if (result != VWTP_OK)
  {
    printf("%s:%d ERROR: %d\n", __FILE__, __LINE__,result);
    return 15;
  }

  if ((kwp[3] == 0x5A) && (kwp[4] == 0x9B) )
  {
    memmove(currentLogLine, kwp+5, 11);
    currentLogLine[11] = '\n';
    currentLogLine[12] = 0;
    if (f_puts(currentLogLine, log_file) < 0)
    {
      return 2; //filesystem error
    }
  }

  // 5. ACK
  result = VWTP_ACK();
  if (result != VWTP_OK)
  {
    return 16;
  }
  
  // 6. Start Routine By Local Identifier
  kwp[0] = 2;
  kwp[1] = 0;
  kwp[2] = 0;
  result =  VWTP_KWP2000Message(0x31, 0xB8, kwp);
  if (result != VWTP_OK)
  {
    return 17;
  }

  if ( (kwp[3] != 0x71) || (kwp[4] != 0xB8) )
  {
    return 18;
  }

  // 7. ACK
  result = VWTP_ACK();
  if (result != VWTP_OK)
  {
    return 19;
  }
  
  // create file header
  file_header_p = file_header;
  file_header_p += sprintf(file_header_p, "time;");
  for (i=1; i <= config[0]; i++)
  {
    for (j=1; j <= 4; j++)
    {
      file_header_p += sprintf(file_header_p,"%d %d;", config[i], j);
    }
  }
  file_header_p += sprintf(file_header_p, "\n");
  
  currentLogLine_p += sprintf(currentLogLine_p, "%d.%02d;", timeSec, time10MSec);

  units_buf_p += sprintf(units_buf_p, "s;");

  while (1)
  {
    kwp[0] = 0;
    result =  VWTP_KWP2000Message(0x21, groupNumber, kwp);
    if (result != VWTP_OK)
    {
      errorCode = 20;
      break;
    }
    
    if ( (kwp[3] == 0x61) && (kwp[4] == groupNumber) ) //group response
    {
      currentLogLine_p += DecodeFrame(currentLogLine_p, &kwp[5]);
      
      if (units_buf_p)
      {
        units_buf_p += DecodeUnits(units_buf_p, &kwp[5]);
      }
    }
    else if ( (kwp[3] == 0x7f) && (kwp[4] == 0x21) ) //group response error
    {
      currentLogLine_p += sprintf(currentLogLine_p, ";;;;;");
      
      if (units_buf_p)
      {
        units_buf_p += sprintf(units_buf_p, ";;;;;");
      }
    }
    else
    {
      errorCode = 21;
      break;
    }
  
    //last group in current line received, save it to log file
    if (groupPos == config[0])
    {
      //save units header to file
      if (units_buf_p)
      {
        //write header to log file
        if ((f_puts("-------------\n\n", log_file) < 0) ||
            (f_puts(file_header, log_file) < 0))
        {
          errorCode = 2;
          break;
        }
        
        *units_buf_p++ = '\n';
        *units_buf_p = 0;
        if (f_puts(units_buf, log_file) < 0)
        {
          errorCode = 2;
          break;
        }
        units_buf_p = 0;
      }
      
      *currentLogLine_p++ = '\n';
      *currentLogLine_p = 0;
      if (f_puts(currentLogLine, log_file) < 0)
      {
        errorCode = 2;
        break;
      }
      currentLogLine_p = currentLogLine;
      currentLogLine_p += sprintf(currentLogLine_p, "%d.%02d;", timeSec, time10MSec);
      
      groupPos = 1;
      groupNumber = config[groupPos];
    }
    else
    {
      //next group
      groupPos = (groupPos < config[0])?groupPos+1:1;
      groupNumber = config[groupPos];
    }

    result = VWTP_ACK();
    if (result != VWTP_OK)
    {
      errorCode = 22;
      break;
    }
  }
  
  if (0 == errorCode)
  {
    result = VWTP_Disconnect();
  }

  return errorCode;
}
