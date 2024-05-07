/*
 * pubsubSerial.h
 *
 *  Created on: Mar 15, 2024
 *      Author: vysocan
 */

#ifndef SOURCE_PUBSUBSERIAL_H_
#define SOURCE_PUBSUBSERIAL_H_

#include "ch.h"
#include "chsys.h"
#include "hal.h"
#include "chprintf.h"
#include <string.h>

//
#define PUBSUB_BUFFER_SIZE 128

// State machine
typedef enum {
  PUBSUB_UNINIT      = 0,
  PUBSUB_ERROR          ,
  PUBSUB_READY          ,
  PUBSUB_HEADER_2       ,
  PUBSUB_HEADER_3       ,
  PUBSUB_MSG_LEN        ,
  PUBSUB_TOPIC_LEN      ,
  PUBSUB_TOPIC          ,
  PUBSUB_PAYLOAD        ,
  PUBSUB_PAYLOAD_LEN    ,
  PUBSUB_CRC_1          ,
  PUBSUB_CRC_2          ,
  PUBSUB_CRC_3          ,
  PUBSUB_CRC_4          ,
  PUBSUB_DONE
} pubsubRxState_t;

/*
 * Data ring buffer
 */
typedef struct {
  uint8_t rxData[PUBSUB_BUFFER_SIZE];
  uint8_t rxHead;
  uint8_t rxLength;
  uint32_t rxCrc;

  uint8_t txData[PUBSUB_BUFFER_SIZE];
  uint8_t txHead;
  uint32_t txCrc;

  binary_semaphore_t *semTx;
  binary_semaphore_t *semRx;
} pubsubData_t;

void pubsubInit(UARTDriver *uartp, binary_semaphore_t *semRxp, binary_semaphore_t *semTxp);
uint8_t pubsubSend(uint8_t *topic, uint8_t topicLength, uint8_t *payload, uint8_t payloadLength);
uint8_t pubsubReceive(uint8_t *topic, uint8_t topicLength, uint8_t *payload, uint8_t payloadLength);

#endif /* SOURCE_PUBSUBSERIAL_H_ */
