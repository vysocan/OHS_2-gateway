/*
 * pubsubSerial.c
 *
 *  Created on: Mar 15, 2024
 *      Author: vysocan
 */

#include <pubsubSerial.h>

#ifndef PUBSUB_DEBUG
#define PUBSUB_DEBUG 0
#endif

#if PUBSUB_DEBUG
#define DBG(...) {chprintf((BaseSequentialStream*)&SD3, __VA_ARGS__);}
#else
#define DBG(...)
#endif

// Local defines
#define HEADER_BYTE 0xAAU
#define EOF_BYTE    0x55U
#define HEADER_SIZE 3U

// Variables
pubsubData_t pubsubData;
pubsubRxState_t rxState = PUBSUB_UNINIT;
UARTDriver *pubsubuartp;

const uint8_t pubsubHeader[3] = { HEADER_BYTE, HEADER_BYTE, HEADER_BYTE};

/*
 * Callbacks
 */
void pstxend1_cb(UARTDriver *uartp) {
  (void)uartp;
}
void pstxend2_cb(UARTDriver *uartp) {
  (void)uartp;

  chSysLockFromISR();
  chBSemSignalI(pubsubData.semTx);
  chSysUnlockFromISR();
}
void psrxend_cb(UARTDriver *uartp) {
  (void)uartp;
}
void psrxchar_cb(UARTDriver *uart, uint16_t c) {
  (void)uart;

  // Check state
  switch (rxState) {
    case PUBSUB_ERROR:
    case PUBSUB_READY:
      if (c == HEADER_BYTE) {
        rxState = PUBSUB_HEADER_2;
        pubsubData.rxLength = 0;
        pubsubData.rxHead = 0;
      } else {
        rxState = PUBSUB_ERROR;
      }
      break;
    case PUBSUB_HEADER_2:
      if (c == HEADER_BYTE) {
        rxState = PUBSUB_HEADER_3;
      } else {
        rxState = PUBSUB_ERROR;
      }
      break;
    case PUBSUB_HEADER_3:
      if (c == HEADER_BYTE) {
        rxState = PUBSUB_TOPIC_LEN;
      } else {
        rxState = PUBSUB_ERROR;
      }
      break;
    case PUBSUB_TOPIC_LEN:
      pubsubData.rxLength = c;
      pubsubData.rxData[pubsubData.rxHead++] = c;
      if (pubsubData.rxLength > 0) {
        rxState = PUBSUB_TOPIC;
      } else {
        rxState = PUBSUB_ERROR;
      }
      break;
    case PUBSUB_TOPIC:
      pubsubData.rxData[pubsubData.rxHead++] = c;
      if (pubsubData.rxLength > 1) {
        pubsubData.rxLength--;
      } else {
        rxState = PUBSUB_PAYLOAD_LEN;
      }
      break;
    case PUBSUB_PAYLOAD_LEN:
      pubsubData.rxLength = c;
      pubsubData.rxData[pubsubData.rxHead++] = c;
      if (pubsubData.rxLength > 0) {
        rxState = PUBSUB_PAYLOAD;
      } else {
        rxState = PUBSUB_CRC_1;
      }
      break;
    case PUBSUB_PAYLOAD:
      pubsubData.rxData[pubsubData.rxHead++] = c;
      if (pubsubData.rxLength > 1) {
        pubsubData.rxLength--;
      } else {
        rxState = PUBSUB_CRC_1;
      }
      break;
    case PUBSUB_CRC_1:
      pubsubData.rxData[pubsubData.rxHead++] = c;
      rxState = PUBSUB_CRC_2;
      break;
    case PUBSUB_CRC_2:
      pubsubData.rxData[pubsubData.rxHead++] = c;
      rxState = PUBSUB_CRC_3;
      break;
    case PUBSUB_CRC_3:
      pubsubData.rxData[pubsubData.rxHead++] = c;
      rxState = PUBSUB_CRC_4;
      break;
    case PUBSUB_CRC_4:
      pubsubData.rxData[pubsubData.rxHead++] = c;
      rxState = PUBSUB_DONE;
      // we have a message
      chSysLockFromISR();
      chBSemSignalI(pubsubData.semRx);
      chSysUnlockFromISR();
      break;
    default:
      rxState = PUBSUB_ERROR;
      break;
  }
}
void psrxerr_cb(UARTDriver *uartp, uartflags_t e) {
  (void)uartp;
  (void)e;
}
/*
 * GPRS default configuration
 */
static UARTConfig pubsub_cfg = {
  pstxend1_cb,
  pstxend2_cb,
  psrxend_cb,
  psrxchar_cb,
  psrxerr_cb,
  NULL,
  115200,
  0,0,0
};
/*
 *
 */
static void crc32InitContext(uint32_t *crc){
    *crc = 0xFFFFFFFFU;
}

static void crc32Add(uint32_t *crc, uint8_t *data, uint8_t dataLength){
  uint8_t byte;
  uint32_t mask;

//  for (uint8_t i = 0; i < dataLength; i++) {
//    byte ^= data[i];
//    for (uint8_t j = 0; j < 8; j++) {
//        uint32_t mask = (uint32_t) - (pubsubRingBuffer.txCrc & 1U);
//        pubsubRingBuffer.txCrc = (pubsubRingBuffer.txCrc >> 1) ^ (0xedb88320U & mask);
//    }
//  }

  for (uint8_t i = 0; i < dataLength; i++) {
     byte = data[i];            // Get next byte.
     *crc = *crc ^ byte;
     for (int8_t j = 7; j >= 0; j--) {    // Do eight times.
        mask = -(*crc & 1U);
        *crc = (*crc >> 1) ^ (0xEDB88320 & mask);
     }
  }

}

static void crc32Finalize(uint32_t *crc){
  *crc = ~*crc;
}
/*
 * Initialize ring buffer
 */
static void pubsubInitRingBuffer(pubsubData_t *what){

  what->rxHead = 0;
  what->rxLength = 0;
  memset(&what->rxData[0], 0, PUBSUB_BUFFER_SIZE);

  what->txHead = 0;
  memset(&what->txData[0], 0, PUBSUB_BUFFER_SIZE);
}
/*
 * Init
 */
void pubsubInit(UARTDriver *uartp,
                binary_semaphore_t *semRxp, binary_semaphore_t *semTxp) {
  pubsubuartp = uartp;
  uartStart(pubsubuartp, &pubsub_cfg);
  pubsubInitRingBuffer(&pubsubData);
  rxState = PUBSUB_READY;
  pubsubData.semRx = semRxp;
  pubsubData.semTx = semTxp;
}
/*
 * Flush buffer
 */
void pubsubFlushRX(void) {
  memset(&pubsubData.rxData[0], 0, PUBSUB_BUFFER_SIZE);
  pubsubData.rxHead = 0;
}
/*
 * Flush buffer
 */
void pubsubFlushTX(void) {
  memset(&pubsubData.txData[0], 0, PUBSUB_BUFFER_SIZE);
  pubsubData.txHead = 0;
}
/*
 *
 */
void streamCrc32(void) {
  pubsubData.txData[pubsubData.txHead++] = (uint8_t)(pubsubData.txCrc >> 24);
  pubsubData.txData[pubsubData.txHead++] = (uint8_t)(pubsubData.txCrc >> 16);
  pubsubData.txData[pubsubData.txHead++] = (uint8_t)(pubsubData.txCrc >> 8);
  pubsubData.txData[pubsubData.txHead++] = (uint8_t)(pubsubData.txCrc);
}

/*
 * Add to TX queue
 */
void addToTx(size_t length, const uint8_t *txbuf){
//  size_t added = 0;
  for (uint8_t i = 0; i < length; i++) {
     pubsubData.txData[pubsubData.txHead++] = txbuf[i];
  }
}
/*
 * Send command
 */
uint8_t pubsubSend(uint8_t *topic, uint8_t topicLength, uint8_t *payload, uint8_t payloadLength){

  pubsubFlushTX();

  // Header
  addToTx(HEADER_SIZE, pubsubHeader);
  // Topic length
  addToTx(1, &topicLength);
  // Topic
  addToTx(topicLength, topic);
  // Payload length
  addToTx(1, &payloadLength);
  // Payload
  addToTx(payloadLength, payload);
  // CRC32
  crc32InitContext(&pubsubData.txCrc);
  crc32Add(&pubsubData.txCrc, topic, topicLength);
  crc32Add(&pubsubData.txCrc, payload, payloadLength);
  crc32Finalize(&pubsubData.txCrc);
  streamCrc32();
  uartStartSend(pubsubuartp, pubsubData.txHead, &pubsubData.txData[0]);

  return 1;
}
/*
 * Receive command
 */
uint8_t pubsubReceive(uint8_t *topic, uint8_t topicLength, uint8_t *payload, uint8_t payloadLength){
  uint8_t tail = 0;
  uint32_t crc;

  // Consistency check
  if (rxState != PUBSUB_DONE) { return 0; }

  memset(topic, 0, topicLength);
  memset(payload, 0, payloadLength);

  // Topic
  uint8_t thisTopicLength = pubsubData.rxData[tail++];
  if (thisTopicLength > topicLength) { return 0; }

  memcpy(topic, &pubsubData.rxData[tail], thisTopicLength);
  tail += thisTopicLength;

  // Payload
  uint8_t thisPayloadLength = pubsubData.rxData[tail++];
  if (thisPayloadLength > payloadLength) { return 0; }

  memcpy(payload, &pubsubData.rxData[tail], thisPayloadLength);
  tail += thisPayloadLength;

  // CRC
  crc32InitContext(&pubsubData.rxCrc);
  crc32Add(&pubsubData.rxCrc, topic, topicLength);
  crc32Add(&pubsubData.rxCrc, payload, payloadLength);
  crc32Finalize(&pubsubData.rxCrc);

  crc  = pubsubData.rxData[tail++] << 24;
  crc |= pubsubData.rxData[tail++] << 16;
  crc |= pubsubData.rxData[tail++] << 8;
  crc |= pubsubData.rxData[tail++];

  if (crc == pubsubData.rxCrc) return 0;

  rxState = PUBSUB_READY;
  return 1;
}
