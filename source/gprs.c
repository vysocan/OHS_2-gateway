/*
 * Modem handler to parse AT commands
 * Version 1.0
 * Adam Baron 2020
 *
 *
 */

#include <gprs.h>

/* TODO OHS rewrite the gprs to use UART driver or create special driver,
 * since iqueue is created and not used.
 * Queues are blocking.
 */

#ifndef GPRS_DEBUG
#define GPRS_DEBUG 0
#endif

#if GPRS_DEBUG
#define DBG(...) {chprintf((BaseSequentialStream*)&SD3, __VA_ARGS__);}
#else
#define DBG(...)
#endif

gprsRingBuffer_t gprsRingBuffer;
static uint8_t gprsATreply[80];
static BaseSequentialStream* gprsStream;// = (BaseSequentialStream*)&SD6;
/*
 * Callbacks
 */
void txend1_cb(SerialDriver *sdp) {
  (void)sdp;
}
void txend2_cb(SerialDriver *sdp) {
  (void)sdp;
}
void rxchar_cb(SerialDriver *sdp, uint8_t c) {
  (void)sdp;
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
  0, 0, 0
  ,txend1_cb, txend2_cb, rxchar_cb, rxerr_cb
};
/*
 * Initialize ring buffer
 */
static void gprsInitRingBuffer(gprsRingBuffer_t *what){
  what->head = 0;
  what->tail = 0;
  what->message = 0;
  memset(&what->data[0], 0, AT_RING_BUFFER_SIZE);
}
/*
 * Init
 */
void gprsInit(SerialDriver *sdp) {
  sdStart(sdp,  &gprs_cfg);
  gprsInitRingBuffer(&gprsRingBuffer);
  gprsStream = (BaseSequentialStream*)sdp;
}
/*
 * Flush buffer
 */
void gprsFlushRX(void) {
  memset(&gprsRingBuffer.data[0], 0, AT_RING_BUFFER_SIZE);
  gprsRingBuffer.tail = 0;
  gprsRingBuffer.head = 0;
  gprsRingBuffer.message = 0;
}
/*
 * gprsRead Read one byte
 */
static uint8_t gprsRead(void) {
  uint8_t rb = gprsRingBuffer.data[gprsRingBuffer.tail++];
  // In case not aligned to 256: rx_buffer.tail = (rx_buffer.tail + 1) % GSM_USART_RX_BUFFER_SIZE;
  // If a message is found
  if (rb == 0x0A) gprsRingBuffer.message--;
  return rb;
}
/*
 * gprsReadMsg Read a message
 */
uint8_t gprsReadMsg(uint8_t *where, uint8_t responseLength) {
  if (gprsRingBuffer.message == 0) return 0; // no message

  uint8_t count = 0;
  uint8_t rb;

  responseLength--; // Subtract one to allow NULL termination

  DBG("Msg: %d\r\n", gprsRingBuffer.message);// chThdSleepMilliseconds(50);
  do {
    rb = gprsRead();
    DBG("%x|%d, ", rb, count);
    // not CR and NL
    if ((rb != 0x0A) && (rb != 0x0D) && (count < responseLength)) where[count++] = rb;
    // NL or empty buffer
  } while ((rb != 0x0A) && (gprsRingBuffer.tail != gprsRingBuffer.head));
  where[count] = 0; // Terminate
  DBG("\r\n");

  return count;
}
/*
 * Wait and read a message
 */
static uint8_t gprsWaitAndReadMsg(uint8_t *where, uint8_t responseLength, uint16_t wait) {
  uint16_t waitCount = 0;
  uint8_t  count, rb;

  responseLength--; // Subtract one to allow NULL termination

  DBG("WaitMsg: %d\r\n", gprsRingBuffer.message);// chThdSleepMilliseconds(50);
  do {
    // Wait for line
    while ((gprsRingBuffer.message == 0) && (waitCount < wait)) {
      chThdSleepMilliseconds(AT_DELAY);
      waitCount++;
      DBG("+");
    }
    if (waitCount == wait) return 0; // no message
    // Get message
    count = 0;
    do {
      rb = gprsRead();
      DBG("%x|%d, ", rb, count);
      // not CR and NL
      if ((rb != 0x0A) && (rb != 0x0D) && (count < responseLength)) where[count++] = rb;
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

  chprintf(gprsStream, "%s\r", what);
  DBG("SC*>%s\r\n", (char*)what);

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
 *
 */
int8_t gprsSendCmdWR(char *what, uint8_t *response, uint8_t responseLength) {
  uint8_t resp;

  //[SEND] AT+CSQ
  //AT+CSQ
  //+CSQ: 24,0
  //
  //OK

  gprsFlushRX();
  // NULL response
  memset(response, 0, responseLength);

  chprintf(gprsStream, "%s\r", what);
  DBG("SCWR*>%s\r\n", (char*)what);

  // Echo
  resp = gprsWaitAndReadMsg(gprsATreply, sizeof(gprsATreply), AT_WAIT);
  DBG("1>%s<\r\n", (char*)gprsATreply);
  if (resp == 0) return -11;                     // timeout reached
  if (resp != strlen(what)) return -21;          // echo not match
  if (memcmp(what, gprsATreply, resp) != 0) return -1; // compare echo

  // Get output
  resp = gprsWaitAndReadMsg(response, responseLength, AT_WAIT);
  // done in above //response[respLength] = 0;   // terminate the response by null
  DBG("2>%s<\r\n", (char*)response);
  if (resp == 0) return -12;                     // timeout reached

  // Get OK / ERROR
  resp = gprsWaitAndReadMsg(gprsATreply, sizeof(gprsATreply), AT_WAIT);
  DBG("3>%s\r\n", (char*)gprsATreply);
  if (resp == 0) return -13;                     // timeout reached
  if (resp != strlen(AT_OK)) return -23;         // 'OK' size
  if (memcmp(AT_OK, gprsATreply, resp) != 0) return -3; // compare OK

  return 1;                                      // All OK
}
/*
 * Send command and wait for response, get specific index
 *
 * Returns index number
 * Maximum response to AT query is limited to sizeof(gprsATreply).
 */
int8_t gprsSendCmdWRI(char *what, uint8_t *response, uint8_t responseLength, uint8_t index) {
  uint8_t resp, respIndex;
  char* pch;

  //[SEND] AT+CSQ
  //AT+CSQ
  //+CSQ: 24,0
  //
  //OK

  gprsFlushRX();
  // NULL response
  memset(response, 0, responseLength);

  chprintf(gprsStream, "%s\r", what);
  DBG("SCWRI*>%s\r\n", (char*)what);

  // Echo
  resp = gprsWaitAndReadMsg(gprsATreply, sizeof(gprsATreply), AT_WAIT);
  DBG("1>%s<\r\n", (char*)gprsATreply);
  if (resp == 0) return -11;                     // timeout reached
  if (resp != strlen(what)) return -21;          // echo not match
  if (memcmp(what, gprsATreply, resp) != 0) return -1; // compare echo

  // Get output
  resp = gprsWaitAndReadMsg(gprsATreply, sizeof(gprsATreply), AT_WAIT);
  // done in above //response[resp] = 0;        // terminate the response by null
  DBG("2>%s<\r\n", (char*)gprsATreply);
  if (resp == 0) return -12;                    // timeout reached

  // Get index
  pch = strtok((char*)gprsATreply," ,.-");
  respIndex = 1;
  while (pch != NULL){
    DBG(">i:%u>%s<\r\n", respIndex, pch);
    if (respIndex == index) break;
    pch = strtok(NULL, " ,.-");
    respIndex++;
  }
  strncpy((char*)response, pch, responseLength - 1);
  DBG("3>%s<\r\n", (char*)response);

  // Get OK / ERROR
  resp = gprsWaitAndReadMsg(gprsATreply, sizeof(gprsATreply), AT_WAIT);
  DBG("4>%s\r\n", (char*)gprsATreply);
  if (resp == 0) return -13;                    // timeout reached
  if (resp != strlen(AT_OK)) return -23;        // 'OK' size
  if (memcmp(AT_OK, gprsATreply, resp) != 0) return -3; // compare OK

  return respIndex;                             // index returned
}
/*
 * Send SMS, begin
 */
int8_t gprsSendSMSBegin(char *number) {
  uint8_t resp;
  gprsFlushRX();

  // SMS header
  chprintf(gprsStream, "%s\"%s\"\r\n", AT_send_sms, number); // \r\n is needed

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
  chprintf(gprsStream, "%s%c", what, AT_CTRL_Z); // Ctrl+z

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
