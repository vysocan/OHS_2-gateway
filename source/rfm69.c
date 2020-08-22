/*
 * rfm69.c
 *
 *  Created on: 26. 3. 2020
 *      Author: vysocan76
 *
 *  ChibiOS specific driver for RFM69
 *
 * Created to be compatible with:
 ************************************************************************************
 * Driver definition for HopeRF RFM69W/RFM69HW/RFM69CW/RFM69HCW, Semtech SX1231/1231H
 * Copyright LowPowerLab LLC 2018, https://www.LowPowerLab.com/contact
 ************************************************************************************
 */

#include <rfm69.h>
#include <rfm69Registers.h>
#include <string.h>
#include "hal.h"
#include "chprintf.h"

#ifndef RFM69_DEBUG
#define RFM69_DEBUG 0
#endif

#if RFM69_DEBUG
#define DBG(...) {chprintf((BaseSequentialStream*) &SD3, __VA_ARGS__);}
#else
#define DBG(...)
#endif

// Global variables
rfm69Config_t      rfm69Config;
rfm69data_t        rfm69Data;
rfm69Transceiver_t transceiverMode;     // Current transceiver mode
uint8_t            rfm69PowerLevel;     // 0 - 31, Auto-adjusted when ATC is enabled
bool               rfm69IsHW = 0;       // HighPower flag
bool               rfm69SensBoost = 0;  // SensitivityBoost flag
int8_t             rfm69TargetRssi = 0; // 0 = ATC disabled
#if RFM69_STATISTICS
// Statistics
uint32_t           rfm69PacketSent;
uint32_t           rfm69PacketReceived;
uint32_t           rfm69PacketAckFailed;
#endif
// Semaphores
binary_semaphore_t rfm69DataReceived;
binary_semaphore_t rfm69AckReceived;
binary_semaphore_t rfm69Lock;

/*
 * Read a RFM69 register value.
 *
 * @param address The register address to be read
 * @return The value of the register
 */
static uint8_t rfm69ReadRegister(uint8_t address) {
  uint8_t resp;

  address &= 0x7F;

  spiAcquireBus(rfm69Config.spidp);
  spiSelect(rfm69Config.spidp);
  spiSend(rfm69Config.spidp, 1, (void *)&address);
  spiReceive(rfm69Config.spidp, 1, (void *)&resp);
  spiUnselect(rfm69Config.spidp);
  spiReleaseBus(rfm69Config.spidp);

  return resp;
}

/*
 * Write a RFM69 register value.
 *
 * @param address The register address to be written
 * @param value The value of the register to be set
 */
static void rfm69WriteRegister(uint8_t address, uint8_t value) {
  uint8_t txBuffer[2] = { address | 0x80, value};

  spiAcquireBus(rfm69Config.spidp);
  spiSelect(rfm69Config.spidp);
  spiSend(rfm69Config.spidp, 2, (void *)&txBuffer[0]);
  spiUnselect(rfm69Config.spidp);
  spiReleaseBus(rfm69Config.spidp);
}

/*
 * Set *transmit/TX* output power: 0=min, 31=max.
 * The power configurations are explained in the SX1231H datasheet (Table 10 on p21; RegPaLevel p66): http://www.semtech.com/images/datasheet/sx1231h.pdf
 * This results in a "weaker" transmitted signal, and directly results in a lower RSSI at the receiver.
 * Valid powerLevel parameter values are 0-31 and result in a directly proportional effect on the output/transmission power.
 * This function implements 2 modes as follows:
 *       - for RFM69W the range is from 0-31 [-18dBm to 13dBm] (PA0 only on RFIO pin)
 *       - for RFM69HW the range is from 0-31 [5dBm to 20dBm]  (PA1 & PA2 on PA_BOOST pin & high Power PA settings - see section 3.3.7 in datasheet, p22)
 */
void rfm69SetPowerLevel(uint8_t powerLevel) {
  rfm69PowerLevel = (powerLevel > 31 ? 31 : powerLevel);
  if (rfm69IsHW) powerLevel /= 2;
  rfm69WriteRegister(REG_PALEVEL, (rfm69ReadRegister(REG_PALEVEL) & 0xE0) | powerLevel);
}

/*
 * Set High power registers
 *
 * @parm onOff HW or W
 */
void rfm69SetHighPowerRegs(bool onOff) {
  rfm69WriteRegister(REG_TESTPA1, onOff ? 0x5D : 0x55);
  rfm69WriteRegister(REG_TESTPA2, onOff ? 0x7C : 0x70);
}

/*
 * Set RFM69 mode
 */
static void rfm69SetMode(uint8_t newMode) {

  if (newMode == transceiverMode) return;
  // Switch modes
  switch (newMode) {
    case RF69_MODE_TX:
    case RF69_MODE_TX_ACK:
    case RF69_MODE_TX_WAIT_ACK:
      rfm69WriteRegister(REG_OPMODE, (rfm69ReadRegister(REG_OPMODE) & 0xE3) | RF_OPMODE_TRANSMITTER);
      if (rfm69IsHW) rfm69SetHighPowerRegs(true);
      if (rfm69TargetRssi) rfm69SetPowerLevel(rfm69PowerLevel); // Do ATC
      break;
    case RF69_MODE_RX:
    case RF69_MODE_RX_WAIT_ACK:
      rfm69WriteRegister(REG_OPMODE, (rfm69ReadRegister(REG_OPMODE) & 0xE3) | RF_OPMODE_RECEIVER);
      if (rfm69IsHW) rfm69SetHighPowerRegs(false);
      // Avoid RX deadlocks
      rfm69WriteRegister(REG_PACKETCONFIG2, (rfm69ReadRegister(REG_PACKETCONFIG2) & 0xFB) | RF_PACKET2_RXRESTART);
      break;
    case RF69_MODE_SYNTH:
      rfm69WriteRegister(REG_OPMODE, (rfm69ReadRegister(REG_OPMODE) & 0xE3) | RF_OPMODE_SYNTHESIZER);
      break;
    case RF69_MODE_STANDBY:
      rfm69WriteRegister(REG_OPMODE, (rfm69ReadRegister(REG_OPMODE) & 0xE3) | RF_OPMODE_STANDBY);
      break;
    case RF69_MODE_SLEEP:
      rfm69WriteRegister(REG_OPMODE, (rfm69ReadRegister(REG_OPMODE) & 0xE3) | RF_OPMODE_SLEEP);
      break;
    default:
      return;
  }

  /* We are using packet mode, so this check is not really needed.
   * But waiting for mode ready is necessary when going from sleep,
   * because the FIFO may not be immediately available from previous mode
   * wait for ModeReady.
   */
  while ((transceiverMode == RF69_MODE_SLEEP) &&
         ((rfm69ReadRegister(REG_IRQFLAGS1) & RF_IRQFLAGS1_MODEREADY) == 0x00));

  transceiverMode = newMode;
  DBG("RFM M: %u\r\n", transceiverMode);
}

/*
 * Get the received signal strength indicator (RSSI)
 *
 * @return RSSI
 */
static int8_t rfm69ReadRSSI(void) {
  int16_t rssi = 0;

  // *** missing force trigger part
  rssi = -rfm69ReadRegister(REG_RSSIVALUE);
  rssi >>= 1;
  return (int8_t)rssi;
}

/*
 * Send data or ACK frame.
 *
 * @@return RF69_RSLT_BUSY - Line is busy.
 *          RF69_RSLT_OK - Sent.
 */
#define RFM69_SEND_HEADER_SIZE 5
static int8_t rfm69SendFrame(uint16_t toAddress, const void* buffer, uint8_t bufferSize,
                      bool requestACK, bool sendACK, bool sendRssi) {
  uint8_t CTLbyte;
  uint16_t temp;
  uint8_t txBuffer[RFM69_SEND_HEADER_SIZE + 1]; // +1 for ATC, to send RSSI
  uint8_t txBufferSize = RFM69_SEND_HEADER_SIZE;

  DBG("RFM SendFrame start\r\n");

  // Check address is not us
  if (toAddress == rfm69Config.nodeID) return RF69_RSLT_NOK;
  // Lock
  chBSemWait(&rfm69Lock);

  rfm69SetMode(RF69_MODE_RX);
  // Wait for clear line
  while ((rfm69ReadRSSI() > CSMA_LIMIT) && ((temp * 1) < RF69_CSMA_LIMIT_MS)) {
    chThdSleepMilliseconds(1);
    temp++;
    DBG("RFM SendWA RSSI: %d\r\n", rfm69ReadRSSI());
  }
  // Line is busy
  if (temp == RF69_CSMA_LIMIT_MS) {
    chBSemSignal(&rfm69Lock);
    return RF69_RSLT_BUSY;
  }

  DBG("RFM SendFrame CSMA: %u\r\n", temp);

  // turn off receiver to prevent reception while filling fifo
  rfm69SetMode(RF69_MODE_STANDBY);
  // wait for ModeReady
  while ((rfm69ReadRegister(REG_IRQFLAGS1) & RF_IRQFLAGS1_MODEREADY) == 0x00) {}

  DBG("RFM SendFrame MR: %x\r\n", rfm69ReadRegister(REG_IRQFLAGS1));
  //writeReg(REG_DIOMAPPING1, RF_DIOMAPPING1_DIO0_00); // DIO0 is "Packet Sent"

  // Force maximum size
  if (bufferSize > RF69_MAX_DATA_LEN) bufferSize = RF69_MAX_DATA_LEN;
  // Force flags
  if (toAddress == RF69_BROADCAST_ADDR) {
    requestACK = false;
    sendACK = false;
    sendRssi = false;
  }

  // control byte
  CTLbyte = 0x00;
  if (sendACK)         CTLbyte = RF69_CTL_SENDACK | (sendRssi ? RF69_CTL_RESERVE1:0);
  else if (requestACK) CTLbyte = RF69_CTL_REQACK | (sendRssi ? RF69_CTL_RESERVE1:0);

  DBG("RFM SendFrame CTLbyte: %d\r\n", CTLbyte);

  if (toAddress > 0xFF) CTLbyte |= (toAddress & 0x300) >> 6; //assign last 2 bits of address if > 255
  if (rfm69Config.nodeID > 0xFF)  CTLbyte |= (rfm69Config.nodeID & 0x300) >> 8; //assign last 2 bits of address if > 255

  txBuffer[0] = REG_FIFO | 0x80;
  txBuffer[1] = bufferSize + 3;
  txBuffer[2] = (uint8_t)toAddress;
  txBuffer[3] = (uint8_t)rfm69Config.nodeID;
  txBuffer[4] = CTLbyte;
  if ((sendACK) && (sendRssi)) {
    txBuffer[txBufferSize++] = ~rfm69Data.rssi;
    DBG("RFM SendFrame ATC RSSI: %d\r\n", txBuffer[txBufferSize - 1]);
  }

  spiAcquireBus(rfm69Config.spidp);
  spiSelect(rfm69Config.spidp);
  spiSend(rfm69Config.spidp, txBufferSize, txBuffer);

  // Transfer data
  if (bufferSize > 0) spiSend(rfm69Config.spidp, bufferSize, buffer);
  spiUnselect(rfm69Config.spidp);
  spiReleaseBus(rfm69Config.spidp);

  // No need to wait for transmit mode to be ready since its handled by the radio
  if (sendACK)         rfm69SetMode(RF69_MODE_TX_ACK);
  else if (requestACK) rfm69SetMode(RF69_MODE_TX_WAIT_ACK);
  else                 rfm69SetMode(RF69_MODE_TX);
  // wait for PacketSent
  CTLbyte = 0; // Used here as counter
  while ((rfm69ReadRegister(REG_IRQFLAGS2) & RF_IRQFLAGS2_PACKETSENT) == 0x00) {
    CTLbyte++;
    chThdSleepMilliseconds(1);
  }
#if RFM69_STATISTICS
  rfm69PacketSent++;
#endif
  DBG("RFM SendFrame PS: wait %u, flag %x\r\n", CTLbyte, rfm69ReadRegister(REG_IRQFLAGS2));

  if (transceiverMode == RF69_MODE_TX_WAIT_ACK) rfm69SetMode(RF69_MODE_RX_WAIT_ACK);
  else rfm69SetMode(RF69_MODE_RX);

  //*** ---
  // Enable receiving
  if (rfm69ReadRegister(REG_IRQFLAGS2) & RF_IRQFLAGS2_PAYLOADREADY) {
    // avoid RX deadlocks
    rfm69WriteRegister(REG_PACKETCONFIG2, (rfm69ReadRegister(REG_PACKETCONFIG2) & 0xFB) | RF_PACKET2_RXRESTART);
  }
  //*** ---
  // set DIO0 to "PAYLOADREADY" in receive mode
  rfm69WriteRegister(REG_DIOMAPPING1, RF_DIOMAPPING1_DIO0_01);

  DBG("RFM SendFrame end\r\n");
  chBSemSignal(&rfm69Lock);
  return RF69_RSLT_OK;
}

/*
 * ISR
 */
static void rfm69IsrCb(void *arg){
  (void)arg;

  chSysLockFromISR();
  if (transceiverMode == RF69_MODE_RX) chBSemSignalI(&rfm69DataReceived);
  if (transceiverMode == RF69_MODE_RX_WAIT_ACK) chBSemSignalI(&rfm69AckReceived);
  chSysUnlockFromISR();
}


/*
 * Enable Automatic Transmission Control
 *
 * @parm targetRssi Target RSSI,
 *                  0 = disabled
 */
void rfm69AutoPower(int8_t targetRssi) {
  rfm69TargetRssi = targetRssi;
}

/*
 * High sensitivity or normal sensitivity mode.
 *
 * Good to enable when RSSI is less than -70dbm.
 * It adds 2db to the bottom end, but takes off 10db at the top
 */
void rfm69SetSensBoost(bool onOff) {
  rfm69SensBoost = onOff;
  rfm69WriteRegister(REG_TESTLNA, rfm69SensBoost ? RF_TESTLNA_HIGH_SENSITIVITY : RF_TESTLNA_NORMAL);
  DBG("RFM SensBoost: %u, %x\r\n", rfm69SensBoost, rfm69ReadRegister(REG_TESTLNA));
}

 /*
  * Get data
  *
  * @@return RF69_RSLT_NOK - Data not for us, or not ACKed.
  *          RF69_RSLT_OK - Data received.
  */
#define RFM69_INTERRUPT_DATA_SIZE 4
int8_t rfm69GetData(void) {
  uint8_t rxBuffer[RFM69_INTERRUPT_DATA_SIZE];
  uint8_t txBuffer[1] = {REG_FIFO & 0x7F};

  DBG("RFM GD: Mode %u, flag %x\r\n", transceiverMode, rfm69ReadRegister(REG_IRQFLAGS2));

  if (rfm69ReadRegister(REG_IRQFLAGS2) & RF_IRQFLAGS2_PAYLOADREADY) {
    // Lock
    chBSemWait(&rfm69Lock);
    //
    rfm69SetMode(RF69_MODE_STANDBY);

    spiAcquireBus(rfm69Config.spidp);
    spiSelect(rfm69Config.spidp);
    spiSend(rfm69Config.spidp, 1, (void *)&txBuffer[0]);
    spiReceive(rfm69Config.spidp, RFM69_INTERRUPT_DATA_SIZE, rxBuffer);

    //for(uint8_t q = 0; q < RFM69_INTERRUPT_DATA_SIZE; q++) { DBG("%u,", rxBuffer[q]); }
    //DBG("\r\n");

    rfm69Data.payloadLength = rxBuffer[0] > 66 ? 66 : rxBuffer[0]; // precaution
    rfm69Data.targetId = rxBuffer[1] | (((uint16_t)rxBuffer[3] & 0x0C) << 6); //10 bit address (most significant 2 bits stored in bits(2,3) of CTL byte
    rfm69Data.senderId = rxBuffer[2] | (((uint16_t)rxBuffer[3] & 0x03) << 8); //10 bit address (most significant 2 bits stored in bits(0,1) of CTL byte

    DBG("RFM GD: F:%u, T:%u, PL:%u\r\n", rfm69Data.senderId, rfm69Data.targetId, rfm69Data.payloadLength);

    // Match this node's address, or broadcast address
    if (!(rfm69Data.targetId == rfm69Config.nodeID || rfm69Data.targetId == RF69_BROADCAST_ADDR) ||
        (rfm69Data.payloadLength < 3)) {
      spiUnselect(rfm69Config.spidp);
      spiReleaseBus(rfm69Config.spidp);
      // Clear data
      rfm69Data.length = 0;
      rfm69Data.senderId = 0;
      rfm69Data.targetId = 0;
      rfm69Data.payloadLength = 0;
      rfm69Data.ackRequested = 0;
      rfm69Data.ackReceived = 0;
      rfm69Data.rssi = 0;
      // Set receiving
      rfm69SetMode(RF69_MODE_RX);
      chBSemSignal(&rfm69Lock);
      return RF69_RSLT_NOK;
    }

    rfm69Data.length = rfm69Data.payloadLength - 3;
    rfm69Data.ackReceived = rxBuffer[3] & RF69_CTL_SENDACK; // extract ACK-received flag
    rfm69Data.ackRequested = rxBuffer[3] & RF69_CTL_REQACK; // extract ACK-requested flag
    rfm69Data.ackRssiRequested = rxBuffer[3] & RF69_CTL_RESERVE1; // extract the ACK RSSI request flag
    DBG("RFM GD: dl:%u, are:%u, arq:%u, AckRssi: %u\r\n",
             rfm69Data.length, rfm69Data.ackReceived, rfm69Data.ackRequested, rfm69Data.ackRssiRequested);

    // Get data, if they are present
    if (rfm69Data.length > 0) {
      spiReceive(rfm69Config.spidp, rfm69Data.length, rfm69Data.data);
      rfm69Data.data[rfm69Data.length] = 0; // Add null at end of string
    }

    spiUnselect(rfm69Config.spidp);
    spiReleaseBus(rfm69Config.spidp);

    rfm69Data.rssi = rfm69ReadRSSI();
#if RFM69_STATISTICS
    rfm69PacketReceived++;
#endif

    if ((rfm69Data.ackRequested) && (rfm69Data.targetId == rfm69Config.nodeID))  {
      /*
      // Sensitivity boost
      if ((rfm69Data.rssi < -70) && !rfm69SensBoost)  {
        rfm69SetSensBoost(true);
      } else if ((rfm69Data.rssi > -50) && rfm69SensBoost) {
        rfm69SetSensBoost(false);
      }
      */

      //chThdSleepMilliseconds(5);
      DBG("RFM GD sending ACK\r\n");
      // Send the frame and return
      chBSemSignal(&rfm69Lock);
      return rfm69SendFrame(rfm69Data.senderId, "", 0, false, true, rfm69Data.ackRssiRequested);
    }

    // ATC
    if ((rfm69Data.ackReceived) && (rfm69Data.ackRssiRequested)) {
      int8_t rfm69AckRssi; // this contains the RSSI our destination Ack'd back to us (if we enabledAutoPower)
      // the next two bytes contain the ACK_RSSI (assuming the datalength is valid)
      if (rfm69Data.length >= 1) {
        rfm69AckRssi = -rfm69Data.data[rfm69Data.length - 1]; // RSSI was sent as single byte positive value
        rfm69Data.length--; // Compensate data length accordingly
        if (rfm69TargetRssi != 0) {
          if ((rfm69AckRssi < rfm69TargetRssi) && (rfm69PowerLevel < 31)) {
            rfm69PowerLevel++;
          } else if ((rfm69AckRssi > rfm69TargetRssi) && (rfm69PowerLevel > 0)) {
            rfm69PowerLevel--;
          }
        }
      }
      DBG("RFM ATC start, rfm69AckedRssi %d, rfm69PowerLevel %d \r\n",
                   rfm69AckRssi, rfm69PowerLevel);
    }

    rfm69SetMode(RF69_MODE_RX);
    chBSemSignal(&rfm69Lock);
    return RF69_RSLT_OK;
  }

  return RF69_RSLT_NOK;
}

/*
 * Send packet
 *
 * @return RF69_RSLT_BUSY - Line is busy.
 *         RF69_RSLT_OK - OK
 *         RF69_RSLT_NOK - Data not acked.
 */
int8_t rfm69Send(uint16_t toAddress, const void* buffer, uint8_t bufferSize, bool requestAck) {
  msg_t resp;

  DBG("RFM Send start\r\n");
  // Send the frame
  resp = rfm69SendFrame(toAddress, buffer, bufferSize, requestAck, false, rfm69TargetRssi);
  // If BUSY or NOK then return
  if (resp != RF69_RSLT_OK) return resp;

  // If ACK not requested we are done.
  if (!requestAck) return RF69_RSLT_OK;

  DBG("RFM SendWA wait ACK, time %u\r\n", chVTGetSystemTimeX());

  // Wait ACK
  chBSemReset(&rfm69AckReceived, true);
  resp = chBSemWaitTimeout(&rfm69AckReceived, TIME_MS2I(RFM69_ACK_TIMEOUT_MS));
  DBG("RFM time %u\r\n", chVTGetSystemTimeX());
  if (resp != MSG_OK) {
    DBG("RFM SendWA ACK timeout\r\n");
    rfm69SetMode(RF69_MODE_RX);
    return RF69_RSLT_NOK;
  }
  // Read ACK data
  rfm69GetData();

  if ((rfm69Data.senderId == toAddress || toAddress == RF69_BROADCAST_ADDR) && rfm69Data.ackReceived) {
    DBG("RFM SendWA received ACK\r\n");
    return RF69_RSLT_OK;
  }

#if RFM69_STATISTICS
  rfm69PacketAckFailed++;
#endif
  return RF69_RSLT_NOK;
}

/*
 * Send packet with retry
 * Replies usually take only 5..8ms at 50kbps@915MHz
 */
int8_t rfm69SendWithRetry(uint16_t toAddress, const void* buffer, uint8_t bufferSize, uint8_t retries) {

  for (uint8_t i = 0; i <= retries; i++) {
    if (rfm69Send( toAddress, buffer, bufferSize, true) == RF69_RSLT_OK) return RF69_RSLT_OK;
  }
  return RF69_RSLT_NOK;
}

/*
 * Put transceiver in sleep mode to save battery.
 * To wake or resume receiving just call receiveDone()
 */
void rfm69Sleep(void) {
  rfm69SetMode(RF69_MODE_SLEEP);
}

/*
 * Get temperature
 *
 * @return Centigrade
 */
int8_t rfm69ReadTemperature(uint8_t calFactor) {
  int8_t retVal;

  rfm69SetMode(RF69_MODE_STANDBY);

  rfm69WriteRegister(REG_TEMP1, RF_TEMP1_MEAS_START);
  while ((rfm69ReadRegister(REG_TEMP1) & RF_TEMP1_MEAS_RUNNING));
  // 'complement' corrects the slope, rising temp = rising val
  retVal = ~rfm69ReadRegister(REG_TEMP2) + COURSE_TEMP_COEF + calFactor;

  rfm69SetMode(RF69_MODE_RX);
  return retVal;
}

/*
 * For RFM69HW only. Must be called setHighPower(true),
 * after initialize() or else transmission won't work.
 */
void rfm69SetHighPower(bool onOff) {
  rfm69IsHW = onOff;
  rfm69WriteRegister(REG_OCP, rfm69IsHW ? RF_OCP_OFF : RF_OCP_ON);

  if (rfm69IsHW) // turning ON
    rfm69WriteRegister(REG_PALEVEL, (rfm69ReadRegister(REG_PALEVEL) & 0x1F) | RF_PALEVEL_PA1_ON | RF_PALEVEL_PA2_ON); // enable P1 & P2 amplifier stages
  else
    rfm69WriteRegister(REG_PALEVEL, RF_PALEVEL_PA0_ON | RF_PALEVEL_PA1_OFF | RF_PALEVEL_PA2_OFF | rfm69PowerLevel); // enable P0 only

  // Allow rfm69SetHighPowerRegs to be set
  rfm69SetMode(RF69_MODE_RX);
}

/*
 * To enable encryption: radio.encrypt("ABCDEFGHIJKLMNOP");
 * To disable encryption: radio.encrypt(null) or radio.encrypt(0)
 * KEY HAS TO BE 16 bytes !!!
 */
void rfm69Encrypt(const char* key) {
  uint8_t txBuffer[17];

  rfm69SetMode(RF69_MODE_STANDBY);
  if (key != 0) {
    txBuffer[0] = REG_AESKEY1 | 0x80;
    memcpy(&txBuffer[1], key, 16);   // Copy text to buffer

    spiAcquireBus(rfm69Config.spidp);
    spiSelect(rfm69Config.spidp);
    spiSend(rfm69Config.spidp, 17, txBuffer);
    spiUnselect(rfm69Config.spidp);
    spiReleaseBus(rfm69Config.spidp);
  }
  rfm69WriteRegister(REG_PACKETCONFIG2, (rfm69ReadRegister(REG_PACKETCONFIG2) & 0xFE) | (key ? 1 : 0));

  rfm69SetMode(RF69_MODE_RX);
}

void rfm69Start(rfm69Config_t *rfm69cfg) {
  uint8_t temp;

  rfm69Config = *rfm69cfg;
  rfm69PowerLevel = 31;

  // Semaphores
  chBSemObjectInit(&rfm69AckReceived, true);
  chBSemObjectInit(&rfm69DataReceived, true);
  chBSemObjectInit(&rfm69Lock, false);

  // Start spi
  spiStart(rfm69Config.spidp, rfm69Config.spiConfig);

  // Set interrupt pin
  palEnableLineEvent(rfm69Config.irqLine, PAL_EVENT_MODE_RISING_EDGE);
  palSetLineCallback(rfm69Config.irqLine, rfm69IsrCb, NULL);

  // registers
  const uint8_t rfm69Registers[][2] = {
    /* 0x01 */ { REG_OPMODE, RF_OPMODE_SEQUENCER_ON | RF_OPMODE_LISTEN_OFF | RF_OPMODE_STANDBY },
    /* 0x02 */ { REG_DATAMODUL, RF_DATAMODUL_DATAMODE_PACKET | RF_DATAMODUL_MODULATIONTYPE_FSK | RF_DATAMODUL_MODULATIONSHAPING_00 }, // no shaping
    /* 0x03 */ { REG_BITRATEMSB, RF_BITRATEMSB_55555}, // default: 4.8 KBPS
    /* 0x04 */ { REG_BITRATELSB, RF_BITRATELSB_55555},
    /* 0x05 */ { REG_FDEVMSB, RF_FDEVMSB_50000}, // default: 5KHz, (FDEV + BitRate / 2 <= 500KHz)
    /* 0x06 */ { REG_FDEVLSB, RF_FDEVLSB_50000},

    /* 0x07 */ { REG_FRFMSB, (uint8_t) (rfm69Config.freqBand==RF69_315MHZ ? RF_FRFMSB_315 : (rfm69Config.freqBand==RF69_433MHZ ? RF_FRFMSB_433 : (rfm69Config.freqBand==RF69_868MHZ ? RF_FRFMSB_868 : RF_FRFMSB_915))) },
    /* 0x08 */ { REG_FRFMID, (uint8_t) (rfm69Config.freqBand==RF69_315MHZ ? RF_FRFMID_315 : (rfm69Config.freqBand==RF69_433MHZ ? RF_FRFMID_433 : (rfm69Config.freqBand==RF69_868MHZ ? RF_FRFMID_868 : RF_FRFMID_915))) },
    /* 0x09 */ { REG_FRFLSB, (uint8_t) (rfm69Config.freqBand==RF69_315MHZ ? RF_FRFLSB_315 : (rfm69Config.freqBand==RF69_433MHZ ? RF_FRFLSB_433 : (rfm69Config.freqBand==RF69_868MHZ ? RF_FRFLSB_868 : RF_FRFLSB_915))) },

    // looks like PA1 and PA2 are not implemented on RFM69W/CW, hence the max output power is 13dBm
    // +17dBm and +20dBm are possible on RFM69HW
    // +13dBm formula: Pout = -18 + OutputPower (with PA0 or PA1**)
    // +17dBm formula: Pout = -14 + OutputPower (with PA1 and PA2)**
    // +20dBm formula: Pout = -11 + OutputPower (with PA1 and PA2)** and high power PA settings (section 3.3.7 in datasheet)
    ///* 0x11 */ { REG_PALEVEL, RF_PALEVEL_PA0_ON | RF_PALEVEL_PA1_OFF | RF_PALEVEL_PA2_OFF | RF_PALEVEL_OUTPUTPOWER_11111},
    ///* 0x13 */ { REG_OCP, RF_OCP_ON | RF_OCP_TRIM_95 }, // over current protection (default is 95mA)

    // RXBW defaults are { REG_RXBW, RF_RXBW_DCCFREQ_010 | RF_RXBW_MANT_24 | RF_RXBW_EXP_5} (RxBw: 10.4KHz)
    /* 0x19 */ { REG_RXBW, RF_RXBW_DCCFREQ_010 | RF_RXBW_MANT_16 | RF_RXBW_EXP_2 }, // (BitRate < 2 * RxBw)
    //for BR-19200: /* 0x19 */ { REG_RXBW, RF_RXBW_DCCFREQ_010 | RF_RXBW_MANT_24 | RF_RXBW_EXP_3 },
    /* 0x25 */ { REG_DIOMAPPING1, RF_DIOMAPPING1_DIO0_01 }, // DIO0 is the only IRQ we're using
    /* 0x26 */ { REG_DIOMAPPING2, RF_DIOMAPPING2_CLKOUT_OFF }, // DIO5 ClkOut disable for power saving
    /* 0x28 */ { REG_IRQFLAGS2, RF_IRQFLAGS2_FIFOOVERRUN }, // writing to this bit ensures that the FIFO & status flags are reset
    /* 0x29 */ { REG_RSSITHRESH, 220 }, // must be set to dBm = (-Sensitivity / 2), default is 0xE4 = 228 so -114dBm
    ///* 0x2D */ { REG_PREAMBLELSB, RF_PREAMBLESIZE_LSB_VALUE } // default 3 preamble bytes 0xAAAAAA
    /* 0x2E */ { REG_SYNCCONFIG, RF_SYNC_ON | RF_SYNC_FIFOFILL_AUTO | RF_SYNC_SIZE_2 | RF_SYNC_TOL_0 },
    /* 0x2F */ { REG_SYNCVALUE1, 0x2D },      // attempt to make this compatible with sync1 byte of RFM12B lib
    /* 0x30 */ { REG_SYNCVALUE2, rfm69Config.networkID }, // NETWORK ID
    //* 0x31 */ { REG_SYNCVALUE3, 0xAA },
    //* 0x31 */ { REG_SYNCVALUE4, 0xBB },
    /* 0x37 */ { REG_PACKETCONFIG1, RF_PACKET1_FORMAT_VARIABLE | RF_PACKET1_DCFREE_OFF | RF_PACKET1_CRC_ON | RF_PACKET1_CRCAUTOCLEAR_ON | RF_PACKET1_ADRSFILTERING_OFF },
    /* 0x38 */ { REG_PAYLOADLENGTH, 66 }, // in variable length mode: the max frame size, not used in TX
    ///* 0x39 */ { REG_NODEADRS, nodeID }, // turned off because we're not using address filtering
    /* 0x3C */ { REG_FIFOTHRESH, RF_FIFOTHRESH_TXSTART_FIFONOTEMPTY | RF_FIFOTHRESH_VALUE }, // TX on FIFO not empty
    /* 0x3D */ { REG_PACKETCONFIG2, RF_PACKET2_RXRESTARTDELAY_2BITS | RF_PACKET2_AUTORXRESTART_ON | RF_PACKET2_AES_OFF }, // RXRESTARTDELAY must match transmitter PA ramp-down time (bitrate dependent)
    //for BR-19200: /* 0x3D */ { REG_PACKETCONFIG2, RF_PACKET2_RXRESTARTDELAY_NONE | RF_PACKET2_AUTORXRESTART_ON | RF_PACKET2_AES_OFF }, // RXRESTARTDELAY must match transmitter PA ramp-down time (bitrate dependent)
    /* 0x6F */ { REG_TESTDAGC, RF_DAGC_IMPROVED_LOWBETA0 }, // run DAGC continuously in RX mode for Fading Margin Improvement, recommended default for AfcLowBetaOn=0
    {255, 0}
  };

  for (temp = 0; rfm69Registers[temp][0] != 255; temp++) {
    rfm69WriteRegister(rfm69Registers[temp][0], rfm69Registers[temp][1]);
  }

  // Set mode
  rfm69SetMode(RF69_MODE_RX);

  // Wait for ModeReady
  do {
   // Endless loop
  } while ((rfm69ReadRegister(REG_IRQFLAGS1) & RF_IRQFLAGS1_MODEREADY) == 0x00);

#if RFM69_STATISTICS
  rfm69PacketSent = 0;
  rfm69PacketReceived = 0;
  rfm69PacketAckFailed = 0;
#endif

}

void rfm69Stop(rfm69Config_t *rfm69cfg) {
  spiStop(rfm69cfg->spidp);

}

void rfm69ReadAllRegs(void) {
  uint8_t regVal, address;

  DBG("Address - HEX - DEC\r\n");
  for (uint8_t regAddr = 1; regAddr <= 0x4F; regAddr++) {
    address = regAddr & 0x7F;

    spiAcquireBus(rfm69Config.spidp);
    spiSelect(rfm69Config.spidp);
    spiSend(rfm69Config.spidp, 1, (void *)&address);
    spiReceive(rfm69Config.spidp, 1, (void *)&regVal);
    spiUnselect(rfm69Config.spidp);
    spiReleaseBus(rfm69Config.spidp);

    DBG("%x - %x - %u\r\n", regAddr, regVal, regVal);
  }
}
