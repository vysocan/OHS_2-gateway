/*
 * rfm69.h
 *
 *  Created on: 26. 3. 2020
 *      Author: adam
 */

#ifndef RFM69_H
#define RFM69_H

#include "hal.h"

#define RF69_MAX_DATA_LEN       61 // to take advantage of the built in AES/CRC we want to limit the frame size to the internal FIFO size (66 bytes - 3 bytes overhead - 2 bytes crc)
#define CSMA_LIMIT              -90 // upper RX signal sensitivity threshold in dBm for carrier sense access

// available frequency bands
#define RF69_315MHZ             31 // non trivial values to avoid misconfiguration
#define RF69_433MHZ             43
#define RF69_868MHZ             86
#define RF69_915MHZ             91

#define null                    0
#define COURSE_TEMP_COEF        -90 // puts the temperature reading in the ballpark, user can fine tune the returned value
#define RF69_BROADCAST_ADDR     0
#define RF69_CSMA_LIMIT_MS      1000
#define RF69_TX_LIMIT_MS        1000
#define RF69_FSTEP              61.03515625 // == FXOSC / 2^19 = 32MHz / 2^19 (p13 in datasheet)

// TWS: define CTLbyte bits
#define RFM69_CTL_SENDACK       0x80
#define RFM69_CTL_REQACK        0x40

#define RFM69_ACK_TIMEOUT_MS    30  // 30ms roundtrip req for 61byte packets

#define RF69_RSLT_NOK           -1
#define RF69_RSLT_BUSY          0
#define RF69_RSLT_OK            1

typedef struct {
  SPIDriver       *spidp;
  const SPIConfig *spiConfig;
  ioline_t         irqLine;
  bool             isRfm69HW;
  uint8_t          freqBand;
  uint16_t         nodeID;
  uint8_t          networkID;
  uint8_t          powerLevel;  // 0 - 31
} rfm69Config_t;

typedef enum {
  RF69_MODE_SLEEP       = 0,// XTAL OFF
  RF69_MODE_STANDBY     = 1,// XTAL ON
  RF69_MODE_SYNTH       = 2,// PLL ON
  RF69_MODE_RX          = 3,// RX MODE
  RF69_MODE_TX          = 4,// TX MODE
  RF69_MODE_TX_ACK      = 5,
  RF69_MODE_TX_WAIT_ACK = 6,
  RF69_MODE_RX_WAIT_ACK = 7
} rfm69Transceiver_t;

typedef struct {
  uint8_t data[RF69_MAX_DATA_LEN+1];
  uint8_t length;
  uint16_t senderId;
  uint16_t targetId;     // should match _address
  uint8_t payloadLength;
  uint8_t ackRequested;
  uint8_t ackReceived;   // should be polled immediately after sending a packet with ACK request
  int16_t rssi;          // most accurate RSSI during reception (closest to the reception)
} rfm69data_t;

extern rfm69data_t rfm69Data;
extern binary_semaphore_t rfm69DataReceived;

//uint8_t rfm69ReadRegister(uint8_t reg);
//void rfm69WriteRegister(uint8_t reg, uint8_t value);
//void rfm69SetHighPowerRegs(bool onOff);
//void rfm69SetMode(uint8_t newMode);

int8_t rfm69GetData(void);
void rfm69SetHighPower(bool onOff);
void rfm69Sleep(void);
void SetPowerLevel(uint8_t powerLevel);
void rfm69Encrypt(const char* key);
int8_t rfm69ReadTemperature(uint8_t calFactor);
bool rfm69SendACK(void);
bool rfm69ReceiveDone(void);
bool rfm69ACKRequested(void);
int8_t rfm69Send(uint16_t toAddress, const void* buffer, uint8_t bufferSize, bool requestAck);
bool frm69SendWithRetry(uint16_t toAddress, const void* buffer, uint8_t bufferSize, uint8_t retries);
void rfm69Start(rfm69Config_t *rfm69cfg);
void rfm69Stop(rfm69Config_t *rfm69cfg);
void rfm69ReadAllRegs(void);

#endif /* RFM69_H */
