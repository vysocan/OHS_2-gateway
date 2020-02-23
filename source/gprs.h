#ifndef GPRS_H
#define GPRS_H

#include "hal.h"
#include "chprintf.h"
#include <string.h>

#define AT_CTRL_Z          26
#define AT_WAIT            50          // how many delay loops wait for modem response
#define AT_DELAY           100         // delay in millis for modem response
// Commands
#define AT_is_alive        "AT"        // --'CR'OK'CR''CR'
#define AT_model           "AT+CGMM"   // --'CR'TC35'CR''CR'OK'CR'
#define AT_registered      "AT+CREG?"  // --'CR'+CREG: 0,1'CR''CR'OK'CR'
#define AT_signal_strength "AT+CSQ"    // --'CR'+CSQ: 21,99'CR''CR'OK'CR'
#define AT_set_sms_to_text "AT+CMGF=1" // --'CR'OK'CR'
#define AT_set_sms_receive "AT+CNMI=1,2,0,0,0" // Set modem to dump SMS to serial
#define AT_send_sms        "AT+CMGS="  // --"+420123456789" followed by message then CTRL-Z then enter
#define AT_CLIP_ON         "AT+CLIP=1" // Set CLI On
#define AT_CLIP_OFF        "AT+CLIP=0" // Set CLI Off
#define AT_D               "ATD"       // Dial number
#define AT_H               "ATH"       // Hang up
#define AT_modem_info      "ATI"       // Full modem info
#define AT_model_info      "AT+CGMM"   // Modem model identification
#define AT_set_ATD         "AT+CVHU=0" // Set ATD to end up voice call
#define AT_GET_DATA        "AT+HTTPREAD" //
// Replies
#define AT_OK              "OK"        //
#define AT_CMT_reply       "+CMT"      // Incoming SMS
#define AT_send_sms_reply  "+CMGS:"    // SMS sent and number
#define AT_DOWNLOAD        "DOWNLOAD"  //
#define AT_HTTPDATA        "AT+HTTPDATA="
#define AT_DATA_SIZE       "+HTTPREAD:"

/*
 * GSM data ring buffer
 */
typedef struct {
  uint8_t data[256];
  uint8_t head;
  uint8_t tail;
  uint8_t message;
} gprsRingBuffer_t;

//extern gprsRingBuffer_t gprsRingBuffer;

void gprsInitRingBuffer(gprsRingBuffer_t *what);
void gprsInit(SerialDriver *sdp);
void gprsFlushRX(void);
uint8_t gprsRead(void);
uint8_t gprsReadMsg(uint8_t *where, uint8_t response_len);
uint8_t gprsWaitAndReadMsg(uint8_t *where, uint8_t response_len, uint16_t wait);
int8_t gprsSendCmd(char *what);
int8_t gprsSendCmdWR(char *what, uint8_t *response, uint8_t response_len);
int8_t gprsSendCmdWRI(char *what, uint8_t *response, uint8_t response_len, uint8_t index);
int8_t gprsSendSMSBegin(char *number);
int8_t gprsSendSMSEnd(char *what);

#endif /* GPRS_H */
