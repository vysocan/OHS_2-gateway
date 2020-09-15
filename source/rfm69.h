/*
 * rfm69.h
 *
 *  Created on: 26. 3. 2020
 *      Author: vysocan76
 *
 *  ChibiOS specific driver for RFM69
 *
 */

#ifndef RFM69_H
#define RFM69_H

#include "hal.h"

#ifndef RFM69_STATISTICS
#define RFM69_STATISTICS 0
#endif

#define RF69_MAX_DATA_LEN       61 // to take advantage of the built in AES/CRC we want to limit the frame size to the internal FIFO size (66 bytes - 3 bytes overhead - 2 bytes crc)
#define CSMA_LIMIT              -90 // upper RX signal sensitivity threshold in dBm for carrier sense access

// available frequency bands
#define RF69_315MHZ             31 // non trivial values to avoid misconfiguration
#define RF69_433MHZ             43
#define RF69_868MHZ             86
#define RF69_915MHZ             91

#define COURSE_TEMP_COEF        -90 // puts the temperature reading in the ballpark, user can fine tune the returned value
#define RF69_BROADCAST_ADDR     0
#define RF69_CSMA_LIMIT_MS      200
#define RF69_FSTEP              61.03515625 // == FXOSC / 2^19 = 32MHz / 2^19 (p13 in datasheet)

// TWS: define CTLbyte bits
#define RF69_CTL_SENDACK        0x80
#define RF69_CTL_REQACK         0x40
#define RF69_CTL_RESERVE1       0x20

#define RFM69_ACK_TIMEOUT_MS    30  // 30ms roundtrip req for 61byte packets

#define RF69_RSLT_NOK           -1
#define RF69_RSLT_BUSY          0
#define RF69_RSLT_OK            1


typedef struct {
  SPIDriver       *spidp;
  const SPIConfig *spiConfig;
  ioline_t         irqLine;
  uint8_t          freqBand;
  uint16_t         nodeID;
  uint8_t          networkID;
} rfm69Config_t;

typedef enum {
  RF69_MODE_SLEEP       = 0,// XTAL OFF
  RF69_MODE_STANDBY     = 1,// XTAL ON
  RF69_MODE_SYNTH       = 2,// PLL ON
  RF69_MODE_RX          = 3,// RX MODE
  RF69_MODE_TX          = 4,// TX MODE
  RF69_MODE_TX_ACK      = 5,// TX MODE to send ACK only
  RF69_MODE_TX_WAIT_ACK = 6,// TX MODE with subsequent wait for ACK
  RF69_MODE_RX_WAIT_ACK = 7 // RX MODE waiting for ACK
} rfm69Transceiver_t;

typedef struct {
  uint8_t data[RF69_MAX_DATA_LEN+1];
  uint8_t length;
  uint16_t senderId;
  uint16_t targetId;
  uint8_t packetLength;
  uint8_t ackRequested;
  uint8_t ackReceived;
  uint8_t ackRssiRequested;
  // TODO OHS move byte flags to bit wise flags
  //uint8_t flags;
  int8_t rssi;
} rfm69data_t;

extern rfm69data_t rfm69Data;
extern binary_semaphore_t rfm69DataReceived;
#if RFM69_STATISTICS
extern uint32_t rfm69PacketSent;
extern uint32_t rfm69PacketReceived;
extern uint32_t rfm69PacketAckFailed;
#endif

void rfm69SetPowerLevel(uint8_t powerLevel);
void rfm69AutoPower(int8_t targetRssi);
void rfm69SetSensBoost(bool onOff);
int8_t rfm69GetData(void);
int8_t rfm69Send(uint16_t toAddress, const void* buffer, uint8_t bufferSize, bool requestAck);
int8_t rfm69SendWithRetry(uint16_t toAddress, const void* buffer, uint8_t bufferSize, uint8_t retries);
void rfm69Sleep(void);
int8_t rfm69ReadTemperature(uint8_t calFactor);
void rfm69SetHighPower(bool onOff);
void rfm69Encrypt(const char* key);
void rfm69Start(rfm69Config_t *rfm69cfg);
void rfm69Stop(rfm69Config_t *rfm69cfg);
void rfm69ReadAllRegs(void);

#endif /* RFM69_H */
