/*
 * Modem handler to parse AT commands
 * Version 1.1
 * Adam Baron 2020-2026
 *
 *
 */

#include <gprs.h>

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
static UartStream UartS;
/*
 * Callbacks
 */
void txend1_cb(UARTDriver *uartp) {
	(void) uartp;
}
void txend2_cb(UARTDriver *uartp) {
	(void) uartp;
}
void rxend_cb(UARTDriver *uartp) {
	(void) uartp;
}
void rxchar_cb(UARTDriver *uartp, uint16_t c) {
	(void) uartp;
	gprsRingBuffer.data[gprsRingBuffer.head++] = (uint8_t)c;
	if ((uint8_t)c == 0x0A)
		gprsRingBuffer.message++; // we have a one line message
}
void rxerr_cb(UARTDriver *uartp, uartflags_t e) {
	(void) uartp;
	(void) e;
}
/*
 * GPRS default configuration
 */
static const UARTConfig gprs_cfg = {
  txend1_cb,
  txend2_cb,
  rxend_cb,
  rxchar_cb,
  rxerr_cb,
  NULL,
  115200,
  0,
  USART_CR1_TE | USART_CR1_RE,
  0
};
/*
 * Send string to UART
 */
static size_t uartstream_write(void *ip, const uint8_t *bp, size_t n) {
  UartStream *usp = (UartStream *)ip;

  /* Blocking send using high‑level UART API */
  uartSendTimeout(usp->uartp, &n, bp, TIME_INFINITE);
  return n;
}

static size_t uartstream_read(void *ip, uint8_t *bp, size_t n) {
  UartStream *usp = (UartStream *)ip;

  msg_t msg = uartReceiveTimeout(usp->uartp, &n, bp, TIME_INFINITE);
  if (msg == MSG_OK) {
    return n;
  }
  return 0;
}

static msg_t uartstream_put(void *ip, uint8_t b) {
  return (msg_t)uartstream_write(ip, &b, 1) == 1 ? MSG_OK : MSG_RESET;
}

static msg_t uartstream_get(void *ip) {
  uint8_t b;
  if (uartstream_read(ip, &b, 1) == 1) {
    return (msg_t)b;
  }
  return MSG_TIMEOUT;
}

static const struct BaseSequentialStreamVMT uartstream_vmt = {
  .write = uartstream_write,
  .read  = uartstream_read,
  .put   = uartstream_put,
  .get   = uartstream_get
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
void gprsInit(UARTDriver *usdp) {
  uartStart(usdp, &gprs_cfg);
  gprsInitRingBuffer(&gprsRingBuffer);
  UartS.vmt   = &uartstream_vmt;
  UartS.uartp = usdp;
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
#if AT_RING_BUFFER_SIZE != 256
  gprsRingBuffer.tail = (gprsRingBuffer.tail + 1) % GSM_USART_RX_BUFFER_SIZE;
  uint8_t rb = gprsRingBuffer.data[gprsRingBuffer.tail];
#else
  uint8_t rb = gprsRingBuffer.data[gprsRingBuffer.tail++];
#endif
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
 * @brief Send command
 *
 * @param what Command to send
 *
 * @return int8_t 1 if OK, or negative error code
 *
 */
int8_t gprsSendCmd(char *what){
  uint8_t resp;

  //[SEND] AT
  //AT
  //OK
  gprsFlushRX();

  chprintf((BaseSequentialStream *)&UartS, "%s\r", what);
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
 * @brief Send command and wait for response
 *
 * @param what Command to send
 * @param response Buffer to store response
 * @param responseLength Length of response buffer
 *
 * @return int8_t 1 if OK, or negative error code
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

  chprintf((BaseSequentialStream *)&UartS, "%s\r", what);
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
  if (memcmp(AT_ERROR_reply, response, strlen(AT_ERROR_reply)) == 0) return -22; // Error reply

  // Get OK / ERROR
  resp = gprsWaitAndReadMsg(gprsATreply, sizeof(gprsATreply), AT_WAIT);
  DBG("3>%s\r\n", (char*)gprsATreply);
  if (resp == 0) return -13;                     // timeout reached
  if (resp != strlen(AT_OK)) return -23;         // 'OK' size
  if (memcmp(AT_OK, gprsATreply, resp) != 0) return -3; // compare OK

  return 1;                                      // All OK
}
/*
 * @brief Send command and wait for response, get specific index
 *
 * @param what Command to send
 * @param response Buffer to store response
 * @param responseLength Length of response buffer
 * @param index Index number to retrieve from response
 *
 * @return int8_t Index number retrieved, or negative error code
 *
 * @note
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

  chprintf((BaseSequentialStream *)&UartS, "%s\r", what);
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
  if (memcmp(AT_ERROR_reply, response, strlen(AT_ERROR_reply)) == 0) return -22; // Error reply

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
 * @brief Send SMS, begin
 *
 * @param number Phone number to send SMS to
 *
 * @return int8_t Result code
 *
 */
int8_t gprsSendSMSBegin(const char *number) {
  uint8_t resp;
  gprsFlushRX();

  // SMS header
  chprintf((BaseSequentialStream *)&UartS, "%s\"%s\"\r\n", AT_send_sms, number); // \r\n is needed

  // Echo
  resp = gprsWaitAndReadMsg(gprsATreply, sizeof(gprsATreply), AT_WAIT);
  DBG("sms-b>%s\r\n", (char*)gprsATreply);
  if (resp == 0) return -11; // timeout reached
  if (memcmp(AT_send_sms, gprsATreply, strlen(AT_send_sms)) != 0) return -1; // compare only command part, echo not match

  return 1;
}
/*
 * @brief Send SMS, end
 *
 * @param what SMS text to send
 *
 * @return int8_t Result code
 *
 */
int8_t gprsSendSMSEnd(const char *what) {
  uint8_t resp;
  gprsFlushRX();

  // End of SMS
  chprintf((BaseSequentialStream *)&UartS, "%s%c", what, AT_CTRL_Z); // Ctrl+z

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
/*
 * @brief Read SMS at index and wait for response
 *
 * @param index SMS index to read
 * @param response Buffer to store response header
 * @param responseLength Length of response buffer
 * @param text Buffer to store SMS text
 * @param textLength Length of SMS text buffer
 *
 * @return int8_t Result code
 *
 */
#define SMS_TEL_NUMBER_POSITION 4
int8_t gprsGetSMS(uint8_t index, uint8_t *telNumber, uint8_t telNumberLength, uint8_t *text, uint8_t textLength) {
  uint8_t resp, respIndex;
  char* pch;

  //[SEND] AT+CMGR=2
  //AT+CMGR=2
  //+CMGR: "REC UNREAD","+420777666555","","25/12/19,15:33:25+4"
  //SMS text here
  //
  //OK

  gprsFlushRX();
  // NULL response
  memset(telNumber, 0, telNumberLength);

  chprintf((BaseSequentialStream *)&UartS, "%s%d\r", AT_read_sms, index);
  DBG("GSMS*>%s%d\r\n", (char*)AT_read_sms, index);

  // Echo
  resp = gprsWaitAndReadMsg(gprsATreply, sizeof(gprsATreply), AT_WAIT);
  DBG("1>%s<\r\n", (char*)gprsATreply);
  if (resp == 0) return -11;                     // timeout reached
  if (memcmp(AT_read_sms, gprsATreply, strlen(AT_read_sms)) != 0) return -1; // compare echo

  // Get output
  resp = gprsWaitAndReadMsg(gprsATreply, sizeof(gprsATreply), AT_WAIT);
  // done in above //response[respLength] = 0;   // terminate the response by null
  DBG("2>%s<\r\n", (char*)gprsATreply);
  if (resp == 0) return -12;                     // timeout reached
  if (memcmp(AT_ERROR_reply, gprsATreply, strlen(AT_ERROR_reply)) == 0) return -22; // Error reply

  // Get index
  pch = strtok((char*)gprsATreply,"\"");
  respIndex = 1;
  while (pch != NULL){
    DBG(">i:%u>%s<\r\n", respIndex, pch);
    if (respIndex == SMS_TEL_NUMBER_POSITION) break;
    pch = strtok(NULL, "\"");
    respIndex++;
  }
  if (respIndex != SMS_TEL_NUMBER_POSITION) return -2; // tel number not found
  strncpy((char*)telNumber, pch, telNumberLength - 1);
  DBG("2>%s<\r\n", (char*)telNumber);

  // Get SMS text
  resp = gprsWaitAndReadMsg(text, textLength, AT_WAIT);
  // done in above //text[textLength] = 0;       // terminate the response by null
  DBG("3>%s<\r\n", (char*)text);
  if (resp == 0) return -13;                     // timeout reached
  if (memcmp(AT_ERROR_reply, telNumber, strlen(AT_ERROR_reply)) == 0) return -23; // Error reply

  // Get OK / ERROR
  resp = gprsWaitAndReadMsg(gprsATreply, sizeof(gprsATreply), AT_WAIT);
  DBG("4>%s\r\n", (char*)gprsATreply);
  if (resp == 0) return -14;                     // timeout reached
  if (resp != strlen(AT_OK)) return -24;         // 'OK' size
  if (memcmp(AT_OK, gprsATreply, resp) != 0) return -4; // compare OK

  return 1;                                      // All OK
}
/*
 * @brief Delete SMS at index
 *
 * @param index SMS index to delete
 *
 * @return int8_t Result code
 *
 */
int8_t gprsDeleteSMS(uint8_t index) {
	uint8_t resp;

	//[SEND] AT+CMGD=2
	//AT+CMGD=2
	//
	//OK

	gprsFlushRX();

	chprintf((BaseSequentialStream*) &UartS, "%s%d\r", AT_delete_sms, index);
	DBG("DSMS*>%s%d\r\n", (char*)AT_delete_sms, index);

	// Echo
	resp = gprsWaitAndReadMsg(gprsATreply, sizeof(gprsATreply), AT_WAIT);
	DBG("1>%s<\r\n", (char* )gprsATreply);
	if (resp == 0) return -11;                     // timeout reached
	if (memcmp(AT_delete_sms, gprsATreply, strlen(AT_delete_sms)) != 0) return -1; // compare echo

	// Get OK / ERROR
	resp = gprsWaitAndReadMsg(gprsATreply, sizeof(gprsATreply), AT_WAIT);
	DBG("2>%s\r\n", (char* )gprsATreply);
	if (resp == 0) return -12;                     // timeout reached
	if (resp != strlen(AT_OK)) return -22;         // 'OK' size
	if (memcmp(AT_OK, gprsATreply, resp) != 0) return -2; // compare OK

	return 1;                                      // All OK
}
