#include <gprs.h>

#ifndef GPRS_DEBUG
#define GPRS_DEBUG 0
#endif

#if GPRS_DEBUG
#define DBG(...) {(BaseSequentialStream*)&SD3, __VA_ARGS__);}
#else
#define DBG(...)
#endif

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
 * gprsRead one byte
 */
uint8_t gprsRead(void) {
  uint8_t rb = gprsRingBuffer.data[gprsRingBuffer.tail++];
  // In case not aligned to 256: rx_buffer.tail = (rx_buffer.tail + 1) % GSM_USART_RX_BUFFER_SIZE;
  // We have a line message
  if (rb == 0x0A) gprsRingBuffer.message--;
  return rb;
}

/*
 * gprsReadMsg message
 */
uint8_t gprsReadMsg(uint8_t *where, uint8_t response_len) {
  if (gprsRingBuffer.message == 0) return 0; // no message
  DBG("Msg: %d\r\n", gprsRingBuffer.message);// chThdSleepMilliseconds(50);
  uint8_t count = 0;
  uint8_t rb;
  do {
    rb = gprsRead();
    DBG("%c-%d\r\n", rb, count); //chThdSleepMilliseconds(50);
    if (rb != 0x0A && rb != 0x0D) { // not CR and NL
      if (count < response_len) where[count++] = rb;
    }
    // NL or empty buffer
  } while ((rb != 0x0A) && (gprsRingBuffer.tail != gprsRingBuffer.head));
  where[count] = 0; // Terminate
  return count;
}

/*
 * Wait and read a message
 */
uint8_t gprsWaitAndReadMsg(uint8_t *where, uint8_t response_len, uint16_t wait) {
  uint16_t atWait;
  uint8_t  count, rb;

  DBG("gprsWaitAndReadMsg: ");
  do {
    // Wait for line
    while ((gprsRingBuffer.message == 0) && (atWait < wait)) {
      chThdSleepMilliseconds(AT_DELAY);
      atWait++;
      DBG("+");
    }
    if (atWait == wait) return 0; // no message
    // Get message
    count = 0;
    do {
      rb = gprsRead();
      DBG(" %x-%d,", rb, count);
      if (rb != 0x0A && rb != 0x0D) { // not CR and NL
        if (count < response_len) where[count++] = rb;
      }
      // NL or empty buffer
    } while ((rb != 0x0A) && (gprsRingBuffer.tail != gprsRingBuffer.head));
    where[count] = 0; // Terminate
    DBG("\r\n");
  } while (count == 0);

  return count;
}

/*
 * Send command
 */
int8_t gprsSendCmd(char *what){
  uint8_t resp;

  //[SEND] AT
  //AT
  //OK
  gprsFlushRX();

  chprintf(gprs, "%s\r", what);
  DBG("*>%s\r\n", (char*)what);

  // Echo
  resp = gprsWaitAndReadMsg(gprsATreply, sizeof(gprsATreply), AT_WAIT);
  DBG("1>%s<\r\n", (char*)gprsATreply);
  if (resp == 0) return -11;                     // timeout reached
  if (resp != strlen(what)) return -21;          // echo not match
  if (memcmp(what, gprsATreply, resp) != 0) return -1; // compare echo

  // Get OK / ERROR
  resp = gprsWaitAndReadMsg(gprsATreply, sizeof(gprsATreply), AT_WAIT);
  DBG("2>%s\r\n", (char*)gprsATreply);
  if (resp == 0) return -12;                    // timeout reached
  if (resp != strlen(AT_OK)) return -22;        // 'OK' size
  if (memcmp(AT_OK, gprsATreply, resp) != 0) return -2; // compare OK

  return 1;
}

/*
 * Send command and wait for response
 */
int8_t gprsSendCmdWR(char *what, uint8_t *response, uint8_t response_len) {
  uint8_t resp, ret;

  //[SEND] AT+CSQ
  //AT+CSQ
  //+CSQ: 24,0
  //
  //OK
  gprsFlushRX();

  chprintf(gprs, "%s\r", what);
  DBG("*>%s\r\n", (char*)what);

  // Echo
  resp = gprsWaitAndReadMsg(gprsATreply, sizeof(gprsATreply), AT_WAIT);
  DBG("1>%s<\r\n", (char*)gprsATreply);
  if (resp == 0) return -11;                     // timeout reached
  if (resp != strlen(what)) return -21;          // echo not match
  if (memcmp(what, gprsATreply, resp) != 0) return -1; // compare echo

  // Get output
  ret = gprsWaitAndReadMsg(response, response_len, AT_WAIT);
  response[ret] = 0;                            // terminate the response by null
  DBG("2>%s<\r\n", (char*)response);
  if (resp == 0) return -12;                    // timeout reached

  // Get OK / ERROR
  resp = gprsWaitAndReadMsg(gprsATreply, sizeof(gprsATreply), AT_WAIT);
  DBG("3>%s\r\n", (char*)gprsATreply);
  if (resp == 0) return -13;                    // timeout reached
  if (resp != strlen(AT_OK)) return -23;        // 'OK' size
  if (memcmp(AT_OK, gprsATreply, resp) != 0) return -3; // compare OK

  return ret;                                   // size of reply
}

/*
 * Send command and wait for response, get specific index
 */
int8_t gprsSendCmdWRI(char *what, uint8_t *response, uint8_t response_len, uint8_t index) {
  uint8_t resp, ret;
  char* pch;

  //[SEND] AT+CSQ
  //AT+CSQ
  //+CSQ: 24,0
  //
  //OK
  gprsFlushRX();

  chprintf(gprs, "%s\r", what);
  DBG("*>%s\r\n", (char*)what);

  // Echo
  resp = gprsWaitAndReadMsg(gprsATreply, sizeof(gprsATreply), AT_WAIT);
  DBG("1>%s<\r\n", (char*)gprsATreply);
  if (resp == 0) return -11;                     // timeout reached
  if (resp != strlen(what)) return -21;          // echo not match
  if (memcmp(what, gprsATreply, resp) != 0) return -1; // compare echo

  // Get output
  ret = gprsWaitAndReadMsg(response, response_len, AT_WAIT);
  response[ret] = 0;                            // terminate the response by null
  DBG("2>%s<\r\n", (char*)response);
  if (resp == 0) return -12;                    // timeout reached

  // Get index
  pch = strtok((char*)response," ,.-");
  ret = 1;
  while (pch != NULL){
    DBG(">%u>%s<\r\n", r, pch);
    if (ret == index) break;
    pch = strtok(NULL, " ,.-");
    ret++;
  }
  strncpy((char*)response, pch, response_len);
  response[response_len] = 0;                    // NULL terminate
  DBG("3>%s<\r\n", (char*)response);

  // Get OK / ERROR
  resp = gprsWaitAndReadMsg(gprsATreply, sizeof(gprsATreply), AT_WAIT);
  DBG("4>%s\r\n", (char*)gprsATreply);
  if (resp == 0) return -13;                    // timeout reached
  if (resp != strlen(AT_OK)) return -23;        // 'OK' size
  if (memcmp(AT_OK, gprsATreply, resp) != 0) return -3; // compare OK

  return ret;                                   // index returned
}

/*
 * Send SMS, begin
 */
int8_t gprsSendSMSBegin(char *number) {
  uint8_t resp;
  gprsFlushRX();

  // SMS header
  chprintf(gprs, "%s\"%s\"\r\n", AT_send_sms, number); // \r\n is needed

  // Echo
  resp = gprsWaitAndReadMsg(gprsATreply, sizeof(gprsATreply), AT_WAIT);
  DBG("sms-b>%s\r\n", (char*)gprsATreply);
  if (resp == 0) return -11; // timeout reached
  if (memcmp(AT_send_sms, gprsATreply, strlen(AT_send_sms)) != 0) return -1; // compare only command part, echo not match

  return 1;
}

/*
 * Send SMS, end
 */
int8_t gprsSendSMSEnd(char *what) {
  uint8_t resp;
  gprsFlushRX();

  // End of SMS
  chprintf(gprs, "%s%c", what, AT_CTRL_Z); // Ctrl+z

  // Wait for SMS reply
  resp = gprsWaitAndReadMsg(gprsATreply, sizeof(gprsATreply), AT_WAIT*10);
  DBG("sms-e1>%s\r\n", (char*)gprsATreply);
  if (resp == 0) return -11; // timeout reached, waiting for network ACK
  if (memcmp(AT_send_sms_reply, gprsATreply, strlen(AT_send_sms_reply)) != 0) return -1; // compare strlen

  // Get OK / ERROR
  resp = gprsWaitAndReadMsg(gprsATreply, sizeof(gprsATreply), AT_WAIT);
  DBG("sms-e2>%s\r\n", (char*)gprsATreply);
  if (resp == 0) return -12;                            // timeout reached
  if (resp != strlen(AT_OK)) return -22;                // 'OK' size
  if (memcmp(AT_OK, gprsATreply, resp) != 0) return -2; // compare OK

  return 1;
}
