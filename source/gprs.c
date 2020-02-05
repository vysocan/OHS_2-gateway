#include <gprs.h>

gprsRingBuffer_t gprsRingBuffer;
static uint8_t gprsATreply[128];
BaseSequentialStream* gprs;

void txend1_cb(SerialDriver *sdp) {
  (void)sdp;
}

void txend2_cb(SerialDriver *sdp) {
  (void)sdp;

}
void rxchar_cb(SerialDriver *sdp, uint8_t c) {
  (void)sdp;
  //(void)c;
  gprsRingBuffer.data[gprsRingBuffer.head++] = c;
  if (c == 0x0A) gprsRingBuffer.message++; // we have a one line message

}
void rxerr_cb(SerialDriver *sdp, eventflags_t e) {
  (void)sdp;
  (void)e;
}

/*
 * GPRS default configuration
 */
static SerialConfig gprs_cfg = {
    115200,
    0, 0, 0,
    txend1_cb,txend2_cb,rxchar_cb,rxerr_cb
};

void gprsInitRingBuffer(gprsRingBuffer_t *what){
  what->head = 0;
  what->tail = 0;
  what->message = 0;
}

void gprsInit(SerialDriver *sdp) {
  sdStart(sdp,  &gprs_cfg);
  gprsInitRingBuffer(&gprsRingBuffer);
  gprs = (BaseSequentialStream*)sdp;
}


/*
 * Flush buffer
 */
void gprsFlushRX(void) {
  gprsRingBuffer.tail = gprsRingBuffer.head;
  gprsRingBuffer.message = 0;
}

/*
 * gprsReadMsg procedure
 */
uint8_t gprsRead(void) {
  uint8_t datal = gprsRingBuffer.data[gprsRingBuffer.tail++];
  // In case not aligned t 256 - rx_buffer.tail = (rx_buffer.tail + 1) % GSM_USART_RX_BUFFER_SIZE;
  if (datal == 0x0A) { // NL
    gprsRingBuffer.message--;
  }
  return datal;
}

/*
 * gprsReadMsg message
 */
uint8_t gprsReadMsg(uint8_t *where, uint8_t response_len) {
  if (gprsRingBuffer.message == 0) return 0; // no message
  //chprintf((BaseSequentialStream*)&SD3, "Msg: %d\r\n", gprsRingBuffer.message); chThdSleepMilliseconds(50);
  uint8_t count = 0;
  uint8_t rb;
  do {
    rb = gprsRead();
    //chprintf((BaseSequentialStream*)&SD3, "%c-%d\r\n", rb, count); chThdSleepMilliseconds(50);
    if (rb != 0x0A && rb != 0x0D) { // not CR and NL
      where[count++] = rb;
    }
  } while ((rb != 0x0A) && (gprsRingBuffer.tail != gprsRingBuffer.head) &&
      (count < response_len)); // NL or empty buffer
  where[count] = 0; // Terminate
  return count;
}

/*
 * Wait for incoming message
 */
uint8_t gprsWaitMsgSpec(uint16_t wait){
  uint8_t at_wait = 0;

  while ((!gprsRingBuffer.message) && (at_wait < wait)) {
    chThdSleepMilliseconds(AT_DELAY);
    at_wait++;
  }
  if (at_wait == wait) return 0;
  else return 1;
}

uint8_t gprsWaitMsg(void){
  return gprsWaitMsgSpec(AT_WAIT);
}

/*
 *
 */
uint8_t gprsIsMsg(void){
  return gprsRingBuffer.message;
}

/*
 * Send command
 */
int8_t gprsSendCmd(char *what){
  uint8_t t_size;
  int8_t  at_tmp;

  //[SEND] AT
  //AT
  //OK
  gprsFlushRX();

  chprintf(gprs, "%s\r", what);
  //WS.print(F("*>")); WS.println(what);

  if (!gprsWaitMsg()) return -20;               // timeout reached
  t_size = gprsReadMsg(gprsATreply, sizeof(gprsATreply)); // gprsReadMsg serial
  //WS.print(F("1>")); WS.println((char*)gprsATreply);
  if (t_size != strlen(what)) return -11;     // echo not match
  at_tmp = memcmp(gprsATreply, what, t_size);
  if (at_tmp != 0) return -1;                 // echo not match

  if (!gprsWaitMsg()) return -21;               // timeout reached
  t_size = gprsReadMsg(gprsATreply, sizeof(gprsATreply)); // gprsReadMsg serial
  //WS.print(F("2>")); WS.println((char*)gprsATreply);
  if (t_size != 2) return -12;                // 'OK' size
  at_tmp = memcmp(AT_OK, gprsATreply, t_size);// compare
  if (at_tmp != 0) return -2;                 // OK not received
  else return 1;
}

/*
 * Send command and wait for response
 */
int8_t gprsSendCmdWR(char *what, uint8_t *response, uint8_t response_len) {
  uint8_t t_size, r;
  int8_t  at_tmp;

  //[SEND] AT+CSQ
  //AT+CSQ
  //+CSQ: 24,0
  //
  //OK
  gprsFlushRX();

  chprintf(gprs, "%s\r", what);
  //WS.print(F("*>")); WS.println(what);

  // get echo
  if (!gprsWaitMsg()) return -20;               // timeout reached
  t_size = gprsReadMsg(gprsATreply, sizeof(gprsATreply)); // gprsReadMsg serial
  //WS.print(F("1>")); WS.println((char*)gprsATreply);
  if (t_size != strlen(what)) return -11;       // echo not match
  at_tmp = memcmp(gprsATreply, what, t_size);
  if (at_tmp != 0) return -1;                   // echo not match

  // get output
  if (!gprsWaitMsg()) return -21;               // timeout reached
  r = gprsReadMsg(response, response_len);      // gprsReadMsg serial
  response[r] = 0;                              // terminate the response by null
  //WS.print(F("2>")); WS.println((char*)response);

  // empty line
  if (!gprsWaitMsg()) return -22;               // timeout reached
  t_size = gprsReadMsg(gprsATreply, sizeof(gprsATreply)); // gprsReadMsg serial
  if (t_size != 0) return -2;                   // not empty line

  // get OK / ERROR
  if (!gprsWaitMsg()) return -23;               // timeout reached
  t_size = gprsReadMsg(gprsATreply, sizeof(gprsATreply)); // gprsReadMsg serial
  //WS.print(F("3>")); WS.println((char*)gprsATreply);
  if (t_size != 2) return -13;                  // 'OK' size
  at_tmp = memcmp(AT_OK, gprsATreply, t_size);  // compare
  if (at_tmp != 0) return -3;                   // OK not received
  else return r;                                // size of reply
}

/*
 * Send command and wait for response, get specific index
 */
int8_t gprsSendCmdWRI(char *what, uint8_t *response, uint8_t response_len, uint8_t index) {
  uint8_t t_size, r;
  int8_t  at_tmp;
  char* pch;

  //[SEND] AT+CSQ
  //AT+CSQ
  //+CSQ: 24,0
  //
  //OK
  gprsFlushRX();

  chprintf(gprs, "%s\r", what);
  //WS.print(F("*>")); WS.println(what);

  // get echo
  if (!gprsWaitMsg()) return -20;               // timeout reached
  t_size = gprsReadMsg(gprsATreply, sizeof(gprsATreply)); // gprsReadMsg serial
  //WS.print(F("1>")); WS.println((char*)gprsATreply);
  if (t_size != strlen(what)) return -11;       // echo not match
  at_tmp = memcmp(gprsATreply, what, t_size);
  if (at_tmp != 0) return -1;                   // echo not match

  // get output
  if (!gprsWaitMsg()) return -21;               // timeout reached
  r = gprsReadMsg(response, response_len);      // gprsReadMsg serial
  response[r] = 0;                              // terminate the response by null
  //chprintf((BaseSequentialStream*)&SD3, "2>%s\r\n", (char*)response);

  // get index
  pch = strtok((char*)response," ,.-");
  r = 0;
  while (pch != NULL){
    ++r;
    if (r == index) break;
    pch = strtok(NULL, " ,.-");
  }
  strncpy((char*)response, pch, response_len);   // ** strlen
  response[strlen(pch)] = 0;  // ** strlen
  //chprintf((BaseSequentialStream*)&SD3, "r>%s\r\n", (char*)response);

  // empty line
  if (!gprsWaitMsg()) return -22;               // timeout reached
  t_size = gprsReadMsg(gprsATreply, sizeof(gprsATreply)); // gprsReadMsg serial
  if (t_size != 0) return -2;                   // not empty line

  // get OK / ERROR
  if (!gprsWaitMsg()) return -23;               // timeout reached
  t_size = gprsReadMsg(gprsATreply, sizeof(gprsATreply)); // gprsReadMsg serial
  //WS.print(F("3>")); WS.println((char*)gprsATreply);
  if (t_size != 2) return -13;                  // 'OK' size
  at_tmp = memcmp(AT_OK, gprsATreply, t_size);  // compare
  if (at_tmp != 0) return -3;                   // OK not received
  else return r;                                // size of reply
}

/*
 * Send SMS, begin
 */
int8_t gprsSendSMSBegin(char *number) {
  int8_t at_tmp;
  gprsFlushRX();

  // SMS header
  //print(AT_send_sms); print('"'); print(number); println('"'); // println is needed
  chprintf(gprs, "%s\"%s\"\r\n", AT_send_sms, number);
  if (!gprsWaitMsg()) return -20;               // timeout reached
  at_tmp = gprsReadMsg(gprsATreply, sizeof(gprsATreply)); // gprsReadMsg serial
  chprintf((BaseSequentialStream*)&SD3, "sms-b>%s\r\n", (char*)gprsATreply);
  at_tmp = memcmp(AT_send_sms, gprsATreply, strlen(AT_send_sms)); // compare only command part
  if (at_tmp != 0) return -1;                   // echo not match
  else return 1;
}

/*
 * Send SMS, end
 */
int8_t gprsSendSMSEnd(char *what) {
  uint8_t t_size;
  int8_t  at_tmp;
  gprsFlushRX();

  // End of SMS
  //print(what); write(26);
  chprintf(gprs, "%s%c", what, AT_CTRL_Z); // Ctrl+z

  /*
  // empty line
  if (!gprsWaitMsg()) return -20;               // timeout reached
  t_size = gprsReadMsg(gprsATreply, sizeof(gprsATreply)); // gprsReadMsg serial
  if (t_size != 0) return -1;                   // not empty line
  else WS.println(F("-sms el-"));
  */

  // gprsReadMsg reply
  if (!gprsWaitMsgSpec(AT_WAIT*2)) return -21;  // timeout reached
  t_size = gprsReadMsg(gprsATreply, sizeof(gprsATreply)); // gprsReadMsg serial
  chprintf((BaseSequentialStream*)&SD3, "sms-e1>%s\r\n", (char*)gprsATreply);
  at_tmp = memcmp(what, gprsATreply, strlen(what));       // compare
  if (at_tmp != 0) return -2;                   // not received

  if (!gprsWaitMsgSpec(AT_WAIT*10)) return -22; // timeout reached, waiting for network ACK
  t_size = gprsReadMsg(gprsATreply, sizeof(gprsATreply)); // gprsReadMsg serial
  chprintf((BaseSequentialStream*)&SD3, "sms-e2>%s\r\n", (char*)gprsATreply);
  at_tmp = memcmp(AT_send_sms_reply, gprsATreply, strlen(AT_send_sms_reply));  // compare strlen
  if (at_tmp != 0) return -3;                   // not received

  // empty line
  if (!gprsWaitMsg()) return -23;               // timeout reached
  t_size = gprsReadMsg(gprsATreply, sizeof(gprsATreply)); // gprsReadMsg serial
  if (t_size != 0) return -4;                   // not empty line
  //else WS.println(F("-sms el-"));

  // get OK / ERROR
  if (!gprsWaitMsg()) return -24;               // timeout reached
  t_size = gprsReadMsg(gprsATreply, sizeof(gprsATreply)); // gprsReadMsg serial
  chprintf((BaseSequentialStream*)&SD3, "sms-e3>%s\r\n", (char*)gprsATreply);
  if (t_size != strlen(AT_OK)) return -15;      // 'OK' size
  at_tmp = memcmp(AT_OK, gprsATreply, t_size);  // compare
  if (at_tmp != 0) return -5;                   // OK not received
  else return 1;                                // size of reply
}
