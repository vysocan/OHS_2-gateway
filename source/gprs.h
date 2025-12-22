#ifndef GPRS_H
#define GPRS_H

#include "hal.h"
#include "chprintf.h"
#include <string.h>

#define AT_CTRL_Z          26          // Escape Ctrl+Z
#define AT_WAIT            50          // how many delay loops wait for modem response
#define AT_DELAY           100         // delay in milliseconds for modem response
// Commands
#define AT_is_alive        "AT"        // --'CR'OK'CR''CR'
#define AT_registered      "AT+CREG?"  // --'CR'+CREG: 0,1'CR''CR'OK'CR'
#define AT_signal_strength "AT+CSQ"    // --'CR'+CSQ: 21,99'CR''CR'OK'CR'
#define AT_set_sms_to_text "AT+CMGF=1" // --'CR'OK'CR'
#define AT_set_sms_receive "AT+CNMI=1,2,0,0,0" // Set modem to dump SMS to serial
#define AT_set_sms_store   "AT+CNMI=2,1,0,0,0" // Set modem to store SMS in SIM and notify
#define AT_get_sms_storage "AT+CPMS?"  //
#define AT_list_unread_sms "AT+CMGL=\"REC UNREAD\"" // List all unread SMS
#define AT_list_all_sms    "AT+CMGL=\"ALL\""        // List all SMS
#define AT_read_sms        "AT+CMGR="  // --index
#define AT_delete_sms      "AT+CMGD="  // --index
#define AT_send_sms        "AT+CMGS="  // --"+420123456789" followed by message then Ctrl+Z then enter
#define AT_CLIP_ON         "AT+CLIP=1" // Set CLI On
#define AT_CLIP_OFF        "AT+CLIP=0" // Set CLI Off
#define AT_D               "ATD"       // Dial number
#define AT_H               "ATH"       // Hang up
#define AT_modem_info      "ATI"       // Full modem info
#define AT_model_info      "AT+CGMM"   // Modem model identification
#define AT_system_info     "AT+CPSI?"  // Modem UE system information
#define AT_set_ATD         "AT+CVHU=0" // Set ATD to end up voice call
#define AT_GET_DATA        "AT+HTTPREAD" //
// Replies
#define AT_OK              "OK"        //
#define AT_CMT_reply       "+CMT"      // Incoming SMS
#define AT_send_sms_reply  "+CMGS:"    // SMS sent and number
#define AT_SMS_received    "+CMTI:"    // SMS received notification
#define AT_ERROR_reply     "+CMS ERROR:"  // Error reply
#define AT_DOWNLOAD        "DOWNLOAD"  //
#define AT_HTTPDATA        "AT+HTTPDATA="
#define AT_DATA_SIZE       "+HTTPREAD:"
//
#define AT_RING_BUFFER_SIZE 256

typedef struct {
  const struct BaseSequentialStreamVMT *vmt;
  UARTDriver *uartp;
} UartStream;
/*
 * GSM data ring buffer
 */
typedef struct {
  uint8_t data[AT_RING_BUFFER_SIZE];
  uint8_t head;
  uint8_t tail;
  uint8_t message;
} gprsRingBuffer_t;

void gprsInit(UARTDriver *usdp);
void gprsFlushRX(void);
uint8_t gprsReadMsg(uint8_t *where, uint8_t responseLength);
int8_t gprsSendCmd(char *what);
int8_t gprsSendCmdWR(char *what, uint8_t *response, uint8_t responseLength);
int8_t gprsSendCmdWRI(char *what, uint8_t *response, uint8_t responseLength, uint8_t index);
int8_t gprsSendSMSBegin(char *number);
int8_t gprsSendSMSEnd(char *what);
int8_t gprsGetSMS(uint8_t index, uint8_t *telNumber, uint8_t telNumberLength, uint8_t *text, uint8_t textLength);
int8_t gprsDeleteSMS(uint8_t index);

#endif /* GPRS_H */
