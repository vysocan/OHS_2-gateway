/*
 * ohs_multipart.h
 *
 *  Created on: 20. 2. 2026
 *      Author: adam
 *
 *  Multipart message abstraction layer over RS485Msg_t and rfm69data_t.
 *  Allows sending/receiving messages up to MP_MAX_MSG_SIZE by chunking
 *  them into transport-level packets.
 *
 *  Chunk wire format (in data[] of transport message):
 *    data[0] = 'M'          // multipart marker
 *    data[1] = chunk_index  // 0-based position of this chunk
 *    data[2] = chunk_total  // total number of chunks (1-based, max 32)
 *    data[3..N] = payload   // up to MP_CHUNK_PAYLOAD bytes
 */

#ifndef OHS_MULTIPART_H_
#define OHS_MULTIPART_H_

#ifndef MULTIPART_DEBUG
#define MULTIPART_DEBUG 0
#endif

#if MULTIPART_DEBUG
#define DBG_MP(...) {chprintf(console, __VA_ARGS__);}
#else
#define DBG_MP(...)
#endif

/*===========================================================================*/
/* Constants                                                                 */
/*===========================================================================*/

#define MP_MARKER          'M'
#define MP_HEADER_SIZE     3       // 'M' + chunk_index + chunk_total
#define MP_CHUNK_PAYLOAD   58      // min(RS485_MSG_SIZE, RF69_MAX_DATA_LEN) - MP_HEADER_SIZE
#define MP_MAX_MSG_SIZE    1536    // 1.5 kB max reassembled message
#define MP_MAX_CHUNKS      32      // limited by uint32_t bitmask
#define MP_TIMEOUT_MS      5000    // timeout for incomplete reassembly

/*===========================================================================*/
/* Data structures                                                           */
/*===========================================================================*/

typedef struct {
  uint8_t   data[MP_MAX_MSG_SIZE]; // reassembly buffer
  uint16_t  receivedLength;        // total received length so far
  uint8_t   chunksTotal;           // expected number of chunks
  uint32_t  chunksReceived;        // bitmask of received chunks
  uint8_t   senderAddress;         // address of sender (reject interleaved)
  systime_t startTime;             // timestamp of first chunk (for timeout)
  bool      active;                // reassembly in progress
} multipartRx_t;

/*===========================================================================*/
/* Shared reassembly buffer                                                  */
/*===========================================================================*/

static multipartRx_t mpRx;

/*===========================================================================*/
/* Receive-side functions                                                    */
/*===========================================================================*/

/*
 * @brief Reset the reassembly buffer
 *
 * @param rx  Pointer to the reassembly buffer
 */
static inline void multipartRxReset(multipartRx_t *rx) {
  memset(rx, 0, sizeof(*rx));
}

/*
 * @brief Check for timeout on active reassembly and reset if expired.
 *        Call periodically from a housekeeping thread (e.g. ServiceThread).
 *
 * @param rx  Pointer to the reassembly buffer
 */
static inline void multipartRxCheckTimeout(multipartRx_t *rx) {
  if (rx->active &&
      chTimeI2MS(chVTTimeElapsedSinceX(rx->startTime)) > MP_TIMEOUT_MS) {
    DBG_MP("MP: timeout, resetting\r\n");
    multipartRxReset(rx);
  }
}

/*
 * @brief Process a received multipart chunk.
 *
 * @param rx       Pointer to the reassembly buffer
 * @param sender   Address of the sender
 * @param data     Raw data from transport (data[0]='M', data[1]=index, data[2]=total, data[3..]=payload)
 * @param length   Total length of data from transport
 *
 * @retval  1  Message complete, rx->data contains full message, rx->totalLength is the length
 * @retval  0  Chunk accepted, more chunks expected
 * @retval -1  Error (bad params, timeout, sender mismatch, etc.)
 */
static inline int8_t multipartRxProcess(multipartRx_t *rx, uint8_t sender,
                                        const uint8_t *data, uint8_t length) {
  uint8_t  chunkIndex, chunkTotal, payloadLen;
  uint16_t offset;
  uint32_t expectedMask;

  // Validate minimum length
  if (length <= MP_HEADER_SIZE) {
    DBG_MP("MP: chunk too short %u\r\n", length);
    return -1;
  }

  chunkIndex = data[1];
  chunkTotal = data[2];
  payloadLen = length - MP_HEADER_SIZE;

  // Validate chunk parameters
  if (chunkTotal == 0 || chunkTotal > MP_MAX_CHUNKS || chunkIndex >= chunkTotal) {
    DBG_MP("MP: bad chunk params idx=%u total=%u\r\n", chunkIndex, chunkTotal);
    multipartRxReset(rx);
    return -1;
  }

  // Check if resulting message would exceed buffer
  if ((uint16_t)chunkTotal * MP_CHUNK_PAYLOAD > MP_MAX_MSG_SIZE + MP_CHUNK_PAYLOAD) {
    DBG_MP("MP: message too large %u chunks\r\n", chunkTotal);
    multipartRxReset(rx);
    return -1;
  }

  // Handle sender mismatch - new sender restarts reassembly
  if (rx->active && rx->senderAddress != sender) {
    DBG_MP("MP: sender mismatch %u != %u, resetting\r\n", sender, rx->senderAddress);
    multipartRxReset(rx);
  }

  // Handle chunk total mismatch - restart
  if (rx->active && rx->chunksTotal != chunkTotal) {
    DBG_MP("MP: chunk total mismatch %u != %u, resetting\r\n", chunkTotal, rx->chunksTotal);
    multipartRxReset(rx);
  }

  // Start new reassembly if not active
  if (!rx->active) {
    rx->active         = true;
    rx->chunksTotal    = chunkTotal;
    rx->chunksReceived = 0;
    rx->senderAddress  = sender;
    rx->receivedLength = 0;
    rx->startTime      = chVTGetSystemTimeX();
  }

  // Check for duplicate chunk
  if (rx->chunksReceived & (1U << chunkIndex)) {
    DBG_MP("MP: duplicate chunk %u\r\n", chunkIndex);
    return 0; // Silently accept duplicate
  }

  // Copy payload into correct position in reassembly buffer
  offset = (uint16_t)chunkIndex * MP_CHUNK_PAYLOAD;
  if (offset + payloadLen > MP_MAX_MSG_SIZE) {
    DBG_MP("MP: chunk would overflow buffer\r\n");
    multipartRxReset(rx);
    return -1;
  }

  memcpy(&rx->data[offset], &data[MP_HEADER_SIZE], payloadLen);

  // Mark chunk as received
  rx->chunksReceived |= (1U << chunkIndex);

  // Update received length
  rx->receivedLength += payloadLen;

  DBG_MP("MP: chunk %u/%u received, bitmask=0x%08lx\r\n",
         chunkIndex + 1, chunkTotal, rx->chunksReceived);

  // Check if all chunks received
  expectedMask = (chunkTotal == 32) ? 0xFFFFFFFFU : ((1U << chunkTotal) - 1);
  if (rx->chunksReceived == expectedMask) {
    DBG_MP("MP: complete, length=%u\r\n", rx->receivedLength);
    // Don't reset here - caller reads data first, then resets
    return 1;
  }

  return 0;
}

/*===========================================================================*/
/* Send-side function                                                        */
/*===========================================================================*/

/*
 * Send a message that may be larger than a single transport packet.
 * The message is chunked into multipart packets and sent via sendData().
 *
 * For messages that fit in a single transport packet (<=MP_CHUNK_PAYLOAD),
 * this still wraps them in multipart format for consistency.
 *
 * @param address  Destination node address
 * @param data     Pointer to the message data
 * @param length   Total message length (1..MP_MAX_MSG_SIZE)
 *
 * @retval  1  All chunks sent successfully
 * @retval -1  Error (length exceeds max, or sendData() failed)
 */
static int8_t sendDataMultipart(uint8_t address, const uint8_t *data, uint16_t length) {
  uint8_t  chunksTotal, payloadLen, i;
  uint8_t  chunkBuf[MP_HEADER_SIZE + MP_CHUNK_PAYLOAD];
  uint16_t offset;
  int8_t   resp;

  if (length == 0 || length > MP_MAX_MSG_SIZE) {
    return -1;
  }

  chunksTotal = (length + MP_CHUNK_PAYLOAD - 1) / MP_CHUNK_PAYLOAD;
  if (chunksTotal > MP_MAX_CHUNKS) {
    return -1;
  }

  chunkBuf[0] = MP_MARKER;
  chunkBuf[2] = chunksTotal;

  offset = 0;
  for (i = 0; i < chunksTotal; i++) {
    payloadLen = (length - offset > MP_CHUNK_PAYLOAD)
                 ? MP_CHUNK_PAYLOAD
                 : (uint8_t)(length - offset);

    chunkBuf[1] = i; // chunk index
    memcpy(&chunkBuf[MP_HEADER_SIZE], &data[offset], payloadLen);

    resp = sendData(address, chunkBuf, MP_HEADER_SIZE + payloadLen);
    if (resp != 1) {
      DBG_MP("MP: sendData failed at chunk %u/%u, resp=%d\r\n", i + 1, chunksTotal, resp);
      return -1;
    }

    offset += payloadLen;

    // Inter-chunk delay to avoid overwhelming receiver
    if (i < chunksTotal - 1) {
      chThdSleepMilliseconds(10);
    }
  }

  DBG_MP("MP: sent %u chunks, %u bytes to addr %u\r\n", chunksTotal, length, address);
  return 1;
}

#endif /* OHS_MULTIPART_H_ */
