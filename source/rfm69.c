/*
 * rfm69.c
 *
 *  Created on: 26. 3. 2020
 *      Author: adam
 */

#include <rfm69.h>
#include <rfm69Registers.h>
#include <string.h>
#include "hal.h"
#include "chprintf.h"

rfm69Config_t rfm69Config;
rfm69data_t rfm69Data;

volatile bool haveData;
rfm69Transceiver_t transceiverMode;

// Semaphores
binary_semaphore_t rfm69DataReceived;
binary_semaphore_t rfm69Received;

/*
 * Read a RFM69 register value.
 *
 * @param address The register address to be read
 * @return The value of the register
 */
uint8_t rfm69ReadRegister(uint8_t address) {
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
void rfm69WriteRegister(uint8_t address, uint8_t value) {
  uint8_t txBuffer[2] = { address | 0x80, value};

  spiAcquireBus(rfm69Config.spidp);
  spiSelect(rfm69Config.spidp);
  spiSend(rfm69Config.spidp, 2, (void *)&txBuffer[0]);
  spiUnselect(rfm69Config.spidp);
  spiReleaseBus(rfm69Config.spidp);
}

void rfm69SetHighPowerRegs(bool onOff) {
  rfm69WriteRegister(REG_TESTPA1, onOff ? 0x5D : 0x55);
  rfm69WriteRegister(REG_TESTPA2, onOff ? 0x7C : 0x70);
}

/*
 * for RFM69HW only: you must call setHighPower(true) after initialize() or else transmission won't work
 */
void rfm69SetHighPower(bool onOff) {
  rfm69Config.isRfm69HW = onOff;
  rfm69WriteRegister(REG_OCP, rfm69Config.isRfm69HW ? RF_OCP_OFF : RF_OCP_ON);

  if (rfm69Config.isRfm69HW) // turning ON
    rfm69WriteRegister(REG_PALEVEL, (rfm69ReadRegister(REG_PALEVEL) & 0x1F) | RF_PALEVEL_PA1_ON | RF_PALEVEL_PA2_ON); // enable P1 & P2 amplifier stages
  else
    rfm69WriteRegister(REG_PALEVEL, RF_PALEVEL_PA0_ON | RF_PALEVEL_PA1_OFF | RF_PALEVEL_PA2_OFF | rfm69Config.powerLevel); // enable P0 only
}

/*
 * Set RFM69 mode
 */
void rfm69SetMode(uint8_t newMode) {

  if (newMode == transceiverMode) return;

  switch (newMode) {
    case RF69_MODE_TX:
    case RF69_MODE_TX_ACK:
    case RF69_MODE_TX_WAIT_ACK:
      rfm69WriteRegister(REG_OPMODE, (rfm69ReadRegister(REG_OPMODE) & 0xE3) | RF_OPMODE_TRANSMITTER);
      if (rfm69Config.isRfm69HW) rfm69SetHighPowerRegs(true);
      break;
    case RF69_MODE_RX:
    case RF69_MODE_RX_WAIT_ACK:
      rfm69WriteRegister(REG_OPMODE, (rfm69ReadRegister(REG_OPMODE) & 0xE3) | RF_OPMODE_RECEIVER);
      if (rfm69Config.isRfm69HW) rfm69SetHighPowerRegs(false);
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

  // we are using packet mode, so this check is not really needed
  // but waiting for mode ready is necessary when going from sleep because the FIFO may not be immediately available from previous mode
  // wait for ModeReady
  while ((transceiverMode == RF69_MODE_SLEEP) &&
         ((rfm69ReadRegister(REG_IRQFLAGS1) & RF_IRQFLAGS1_MODEREADY) == 0x00));

  transceiverMode = newMode;
  chprintf((BaseSequentialStream*)&SD3, "RFM M: %u\r\n", transceiverMode);
}

/*
 * Get temperature
 *
 * @return Centigrade
 */
int8_t rfm69ReadTemperature(uint8_t calFactor) {
  rfm69SetMode(RF69_MODE_STANDBY);
  rfm69WriteRegister(REG_TEMP1, RF_TEMP1_MEAS_START);
  while ((rfm69ReadRegister(REG_TEMP1) & RF_TEMP1_MEAS_RUNNING));
  // 'complement' corrects the slope, rising temp = rising val
  return ~rfm69ReadRegister(REG_TEMP2) + COURSE_TEMP_COEF + calFactor;
}

/*
 * Put transceiver in sleep mode to save battery.
 * To wake or resume receiving just call receiveDone()
 */
void rfm69Sleep(void) {
  rfm69SetMode(RF69_MODE_SLEEP);
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
void SetPowerLevel(uint8_t powerLevel) {
  rfm69Config.powerLevel = (powerLevel > 31 ? 31 : powerLevel);
  if (rfm69Config.isRfm69HW) powerLevel /= 2;
  rfm69WriteRegister(REG_PALEVEL, (rfm69ReadRegister(REG_PALEVEL) & 0xE0) | powerLevel);
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
}

/*
 * Get the received signal strength indicator (RSSI)
 * @return RSSI
 */
int16_t rfm69ReadRSSI(void) {
  int16_t rssi = 0;

  // *** missing force trigger part
  rssi = -rfm69ReadRegister(REG_RSSIVALUE);
  rssi >>= 1;
  return rssi;
}

bool rfm69CanSend(void) {
  // if signal stronger than -100dBm is detected assume channel activity
  chprintf((BaseSequentialStream*)&SD3, "RFM CanSend: mode %u, pl %u, rssi %d\r\n",
           transceiverMode, rfm69Data.payloadLength, rfm69ReadRSSI());
  if ((transceiverMode == RF69_MODE_RX) &&
      (rfm69Data.payloadLength == 0) &&
      (rfm69ReadRSSI() < CSMA_LIMIT)) {
    rfm69SetMode(RF69_MODE_STANDBY);

    return true;
  }
  return false;
}

/*
 *
 */
void rfm69ReceiveBegin(void) {
  rfm69Data.length = 0;
  rfm69Data.senderId = 0;
  rfm69Data.targetId = 0;
  rfm69Data.payloadLength = 0;
  rfm69Data.ackRequested = 0;
  rfm69Data.ackReceived = 0;
  rfm69Data.rssi = 0;
  haveData = false; //*** Added

  if (rfm69ReadRegister(REG_IRQFLAGS2) & RF_IRQFLAGS2_PAYLOADREADY) {
    // avoid RX deadlocks
    rfm69WriteRegister(REG_PACKETCONFIG2, (rfm69ReadRegister(REG_PACKETCONFIG2) & 0xFB) | RF_PACKET2_RXRESTART);
  }
  // set DIO0 to "PAYLOADREADY" in receive mode
  rfm69WriteRegister(REG_DIOMAPPING1, RF_DIOMAPPING1_DIO0_01);
  if (transceiverMode == RF69_MODE_TX_WAIT_ACK) rfm69SetMode(RF69_MODE_RX_WAIT_ACK);
  else if(transceiverMode != RF69_MODE_RX_WAIT_ACK) rfm69SetMode(RF69_MODE_RX);
}

/*
 *
 */
#define RFM69_SEND_HEADER_SIZE 5
int8_t rfm69SendFrame(uint16_t toAddress, const void* buffer, uint8_t bufferSize, bool requestACK, bool sendACK) {
  uint8_t CTLbyte;
  uint16_t temp;
  uint8_t txBuffer[RFM69_SEND_HEADER_SIZE];

  chprintf((BaseSequentialStream*)&SD3, "RFM SendFrame start\r\n");

  rfm69SetMode(RF69_MODE_RX);
  // avoid RX deadlocks
  rfm69WriteRegister(REG_PACKETCONFIG2, (rfm69ReadRegister(REG_PACKETCONFIG2) & 0xFB) | RF_PACKET2_RXRESTART);
  // Wait for clear line
  while ((rfm69ReadRSSI() > CSMA_LIMIT) && ((temp * 1) < RF69_CSMA_LIMIT_MS)) {
    chThdSleepMilliseconds(1);
    temp++;
    chprintf((BaseSequentialStream*)&SD3, "RFM SendWA RSSI: %d\r\n", rfm69ReadRSSI());
  }
  // Line is busy
  if (temp == RF69_CSMA_LIMIT_MS) return RF69_RSLT_BUSY;

  chprintf((BaseSequentialStream*)&SD3, "RFM SendFrame CSMA: %u\r\n", temp);

  // turn off receiver to prevent reception while filling fifo
  rfm69SetMode(RF69_MODE_STANDBY);
  // wait for ModeReady
  while ((rfm69ReadRegister(REG_IRQFLAGS1) & RF_IRQFLAGS1_MODEREADY) == 0x00) {}

  chprintf((BaseSequentialStream*)&SD3, "RFM SendFrame MR: %x\r\n", rfm69ReadRegister(REG_IRQFLAGS1));
  //writeReg(REG_DIOMAPPING1, RF_DIOMAPPING1_DIO0_00); // DIO0 is "Packet Sent"

  // Force maximum size
  if (bufferSize > RF69_MAX_DATA_LEN) bufferSize = RF69_MAX_DATA_LEN;
  // Force flags
  if (toAddress == RF69_BROADCAST_ADDR) {
    requestACK = false;
    sendACK = false;
  }

  // control byte
  CTLbyte = 0x00;
  if (sendACK)         CTLbyte = RFM69_CTL_SENDACK;
  else if (requestACK) CTLbyte = RFM69_CTL_REQACK;

  if (toAddress > 0xFF) CTLbyte |= (toAddress & 0x300) >> 6; //assign last 2 bits of address if > 255
  if (rfm69Config.nodeID > 0xFF)  CTLbyte |= (rfm69Config.nodeID & 0x300) >> 8; //assign last 2 bits of address if > 255

  txBuffer[0] = REG_FIFO | 0x80;
  txBuffer[1] = bufferSize + 3;
  txBuffer[2] = (uint8_t)toAddress;
  txBuffer[3] = (uint8_t)rfm69Config.nodeID;
  txBuffer[4] = CTLbyte;

  spiAcquireBus(rfm69Config.spidp);
  spiSelect(rfm69Config.spidp);
  spiSend(rfm69Config.spidp, RFM69_SEND_HEADER_SIZE, txBuffer);
  // Transfer data
  if (bufferSize > 0) spiSend(rfm69Config.spidp, bufferSize, buffer);
  spiUnselect(rfm69Config.spidp);
  spiReleaseBus(rfm69Config.spidp);

  // No need to wait for transmit mode to be ready since its handled by the radio
  if (sendACK)         rfm69SetMode(RF69_MODE_TX_ACK);
  else if (requestACK) rfm69SetMode(RF69_MODE_TX_WAIT_ACK);
  else                 rfm69SetMode(RF69_MODE_TX);
  // wait for PacketSent
  CTLbyte = 0;
  while ((rfm69ReadRegister(REG_IRQFLAGS2) & RF_IRQFLAGS2_PACKETSENT) == 0x00) {
    CTLbyte++;
    chThdSleepMilliseconds(1);
  }
  chprintf((BaseSequentialStream*)&SD3, "RFM SendFrame PS: wait %u, flag %x\r\n", CTLbyte, rfm69ReadRegister(REG_IRQFLAGS2));

  //rfm69ReceiveBegin(); // *** replaced old rfm69SetMode(RF69_MODE_STANDBY);

  if (transceiverMode == RF69_MODE_TX_WAIT_ACK) rfm69SetMode(RF69_MODE_RX_WAIT_ACK);
  else rfm69SetMode(RF69_MODE_RX);

  // Enable receiving
  if (rfm69ReadRegister(REG_IRQFLAGS2) & RF_IRQFLAGS2_PAYLOADREADY) {
    // avoid RX deadlocks
    rfm69WriteRegister(REG_PACKETCONFIG2, (rfm69ReadRegister(REG_PACKETCONFIG2) & 0xFB) | RF_PACKET2_RXRESTART);
  }
  // set DIO0 to "PAYLOADREADY" in receive mode
  rfm69WriteRegister(REG_DIOMAPPING1, RF_DIOMAPPING1_DIO0_01);

  return RF69_RSLT_OK;
}

/*
 * ISR
 */
void rfm69IsrCb(void *arg){
  (void)arg;

  chSysLockFromISR();
  haveData = true;
  if (transceiverMode == RF69_MODE_RX) chBSemSignalI(&rfm69DataReceived);
  if (transceiverMode == RF69_MODE_RX_WAIT_ACK) chBSemSignalI(&rfm69Received);
  chSysUnlockFromISR();
}

 /*
  * Internal function
  * Interrupt gets called when a packet is received
  */
#define RFM69_INTERRUPT_DATA_SIZE 4
int8_t rfm69GetData(void) {
  static char rxBuffer[RFM69_INTERRUPT_DATA_SIZE];

  chprintf((BaseSequentialStream*)&SD3, "RFM GD: datamode %u, flag %x\r\n", transceiverMode, rfm69ReadRegister(REG_IRQFLAGS2));

  //if (((transceiverMode == RF69_MODE_RX) || (transceiverMode == RF69_MODE_RX_WAIT_ACK)) &&

  if (rfm69ReadRegister(REG_IRQFLAGS2) & RF_IRQFLAGS2_PAYLOADREADY) {
    rfm69SetMode(RF69_MODE_STANDBY);

    uint8_t txBuffer[1] = {REG_FIFO & 0x7F};

    spiAcquireBus(rfm69Config.spidp);
    spiSelect(rfm69Config.spidp);
    spiSend(rfm69Config.spidp, 1, (void *)&txBuffer[0]);
    spiReceive(rfm69Config.spidp, RFM69_INTERRUPT_DATA_SIZE, rxBuffer);

    rfm69Data.payloadLength = rxBuffer[0] > 66 ? 66 : rxBuffer[0]; // precaution
    rfm69Data.targetId = rxBuffer[1] | (((uint16_t)rxBuffer[3] & 0x0C) << 6); //10 bit address (most significant 2 bits stored in bits(2,3) of CTL byte
    rfm69Data.senderId = rxBuffer[2] | (((uint16_t)rxBuffer[3] & 0x03) << 8); //10 bit address (most significant 2 bits stored in bits(0,1) of CTL byte

    chprintf((BaseSequentialStream*)&SD3, "RFM GD: F:%u, T:%u, L:%u\r\n", rfm69Data.senderId, rfm69Data.targetId, rfm69Data.payloadLength);

    // Match this node's address, or broadcast address
    // Address situation could receive packets that are malformed and don't fit this libraries extra fields
    if (!(rfm69Data.targetId == rfm69Config.nodeID || rfm69Data.targetId == RF69_BROADCAST_ADDR) ||
        (rfm69Data.payloadLength < 3)) {
      rfm69Data.payloadLength = 0;
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
      rfm69SetMode(RF69_MODE_RX);
      // Avoid RX deadlocks
      rfm69WriteRegister(REG_PACKETCONFIG2, (rfm69ReadRegister(REG_PACKETCONFIG2) & 0xFB) | RF_PACKET2_RXRESTART);
      return RF69_RSLT_NOK;
    }

    rfm69Data.length = rfm69Data.payloadLength - 3;
    rfm69Data.ackReceived = rxBuffer[3] & RFM69_CTL_SENDACK; // extract ACK-received flag
    rfm69Data.ackRequested = rxBuffer[3] & RFM69_CTL_REQACK; // extract ACK-requested flag
    chprintf((BaseSequentialStream*)&SD3, "RFM GD: dl:%u, are:%u, arq:%u\r\n",
             rfm69Data.length, rfm69Data.ackReceived, rfm69Data.ackRequested);
    // *** ATC rfm69InterruptHook(rxBuffer[3]); // TWS: hook to derived class interrupt function
    // Get data, if they are present
    if (rfm69Data.length > 0) {
      spiReceive(rfm69Config.spidp, rfm69Data.length, rfm69Data.data);
      rfm69Data.data[rfm69Data.length] = 0; // Add null at end of string
    }

    spiUnselect(rfm69Config.spidp);
    spiReleaseBus(rfm69Config.spidp);

    rfm69Data.rssi = rfm69ReadRSSI();

    if ((rfm69Data.ackRequested) && (rfm69Data.targetId == rfm69Config.nodeID))  {
      chprintf((BaseSequentialStream*)&SD3, "RFM ACK start\r\n");
      chThdSleepMilliseconds(10);
      // Send the frame
      return rfm69SendFrame(rfm69Data.senderId, "", 0, false, true);
    }

    rfm69SetMode(RF69_MODE_RX);
    // Avoid RX deadlocks
    rfm69WriteRegister(REG_PACKETCONFIG2, (rfm69ReadRegister(REG_PACKETCONFIG2) & 0xFB) | RF_PACKET2_RXRESTART);

    return RF69_RSLT_OK;
  }

  return RF69_RSLT_NOK;
}

/*
 * Checks if a packet was received and/or puts transceiver in receive (ie RX or listen) mode
 */
bool rfm69ReceiveDone(void) {
  if (haveData) {
    chprintf((BaseSequentialStream*)&SD3, "RFM RD haveData\r\n");
    haveData = false;
    rfm69GetData();
  }

  if ((transceiverMode == RF69_MODE_RX) && (rfm69Data.payloadLength > 0)) {
    rfm69SetMode(RF69_MODE_STANDBY); // ??? enables interrupts
    chprintf((BaseSequentialStream*)&SD3, "RFM RD payload\r\n");
    return true;
  } else if (transceiverMode == RF69_MODE_RX) {// already in RX no payload yet
    chprintf((BaseSequentialStream*)&SD3, "RFM RD not yet\r\n");
    return false;
  }

  rfm69ReceiveBegin();
  chprintf((BaseSequentialStream*)&SD3, "RFM RD end\r\n");
  return false;
}

/*
 * Should be polled immediately after sending a packet with ACK request
 */
bool rfm69ACKReceived(uint16_t fromNodeID) {
  if (rfm69ReceiveDone()) return (rfm69Data.senderId == fromNodeID || fromNodeID == RF69_BROADCAST_ADDR) && rfm69Data.ackReceived;
  return false;
}

/*
 * Check whether an ACK was requested in the last received packet (non-broadcasted packet)
 */
bool rfm69ACKRequested(void) {
  return rfm69Data.ackRequested && (rfm69Data.targetId == rfm69Config.nodeID);
}

/*
 * should be called immediately after reception in case sender wants ACK
 */
bool rfm69SendACK(void) {
  uint16_t temp = 0;

  chprintf((BaseSequentialStream*)&SD3, "RFM SendACK start\r\n");

  rfm69Data.ackRequested = 0;   // TWS added to make sure we don't end up in a timing race and infinite loop sending Acks
  uint16_t sender = rfm69Data.senderId;
  int16_t rssi = rfm69Data.rssi; // save payload received RSSI value

  // avoid RX deadlocks
  rfm69WriteRegister(REG_PACKETCONFIG2, (rfm69ReadRegister(REG_PACKETCONFIG2) & 0xFB) | RF_PACKET2_RXRESTART);

  // Wait for clear line
  rfm69SetMode(RF69_MODE_RX);
  while ((rfm69ReadRSSI() > CSMA_LIMIT) && ((temp * 1) < RF69_CSMA_LIMIT_MS)) {
    chThdSleepMilliseconds(1);
    temp++;
    chprintf((BaseSequentialStream*)&SD3, "RFM SendWA RSSI: %d\r\n", rfm69ReadRSSI());
  }
  // Line is busy
  if (temp == RF69_CSMA_LIMIT_MS) return false;

  chprintf((BaseSequentialStream*)&SD3, "RFM SendACK: %u\r\n", temp);

  rfm69Data.senderId = sender;    // TWS: Restore SenderID after it gets wiped out by receiveDone()

  rfm69SendFrame(sender, "", 0, false, true);

  rfm69Data.rssi = rssi; // restore payload RSSI

  return true;
}

/*
 * Send packet
 */
int8_t rfm69Send(uint16_t toAddress, const void* buffer, uint8_t bufferSize, bool requestAck) {
  uint16_t temp = 0;
  msg_t resp;

  chprintf((BaseSequentialStream*)&SD3, "RFM Send start\r\n");

  // Send the frame
  if (rfm69SendFrame(toAddress, buffer, bufferSize, requestAck, false) == RF69_RSLT_BUSY)
    return RF69_RSLT_BUSY;

  if (!requestAck) {
    rfm69SetMode(RF69_MODE_RX);
    return RF69_RSLT_OK;
  }

  chprintf((BaseSequentialStream*)&SD3, "RFM SendWA wait ACK\r\n");
  chprintf((BaseSequentialStream*)&SD3, "Time %u\r\n", chVTGetSystemTimeX());

  // Wait ACK
  chBSemReset(&rfm69Received, true);
  resp = chBSemWaitTimeout(&rfm69Received, TIME_MS2I(RFM69_ACK_TIMEOUT_MS));
  chprintf((BaseSequentialStream*)&SD3, "Time %u\r\n", chVTGetSystemTimeX());
  if (resp == MSG_OK) {
    haveData = false;
    rfm69GetData();
    rfm69SetMode(RF69_MODE_RX);
  } else {
    rfm69SetMode(RF69_MODE_RX);
    return RF69_RSLT_NOK;
  }

  chprintf((BaseSequentialStream*)&SD3, "RFM SendWA received ACK\r\n");

  if ((rfm69Data.senderId == toAddress || toAddress == RF69_BROADCAST_ADDR) && rfm69Data.ackReceived)
    return RF69_RSLT_OK;
  else return RF69_RSLT_NOK;
}

/*
 *
 * Replies usually take only 5..8ms at 50kbps@915MHz
 */
bool rfm69SendWithRetry(uint16_t toAddress, const void* buffer, uint8_t bufferSize, uint8_t retries) {

  for (uint8_t i = 0; i <= retries; i++) {
    //rfm69Send(toAddress, buffer, bufferSize, true);
    chThdSleepMilliseconds(RFM69_ACK_TIMEOUT_MS);
    if (rfm69ACKReceived(toAddress)) return true;
  }
  return false;
}

void rfm69Start(rfm69Config_t *rfm69cfg) {
  uint8_t temp;

  rfm69Config = *rfm69cfg;

  // Semaphores
  chBSemObjectInit(&rfm69Received, true);
  chBSemObjectInit(&rfm69DataReceived, true);

  // Start spi
  spiStart(rfm69Config.spidp, rfm69Config.spiConfig);
  // Force mode
  transceiverMode = RF69_MODE_STANDBY;

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

  temp = 0;
  do {
    rfm69WriteRegister(REG_SYNCVALUE1, 0xAA);
    chThdSleepMilliseconds(1);
    temp++;
  } while ((rfm69ReadRegister(REG_SYNCVALUE1) != 0xAA) && (temp < 10));
  chprintf((BaseSequentialStream*)&SD3, "temp: %d, %x\r\n", temp, rfm69ReadRegister(REG_SYNCVALUE1));

  temp = 0;
  do {
    rfm69WriteRegister(REG_SYNCVALUE1, 0x55);
    chThdSleepMilliseconds(1);
    temp++;
  } while ((rfm69ReadRegister(REG_SYNCVALUE1) != 0x55) && (temp < 10));
  chprintf((BaseSequentialStream*)&SD3, "temp: %d, %x\r\n", temp, rfm69ReadRegister(REG_SYNCVALUE1));

  for (temp = 0; rfm69Registers[temp][0] != 255; temp++) {
    rfm69WriteRegister(rfm69Registers[temp][0], rfm69Registers[temp][1]);
  }

  // Encryption is persistent between resets and can trip you up during debugging.
  // Disable it during initialization so we always start from a known state.
  // ***  encrypt(0);

  // Called regardless if it's a RFM69W or RFM69HW
  rfm69SetHighPower(rfm69Config.isRfm69HW);
  // Set mode
  rfm69SetMode(transceiverMode);

  // Wait for ModeReady
  temp = 0;
  do {
    chThdSleepMilliseconds(1);
    temp++;
  } while (((rfm69ReadRegister(REG_IRQFLAGS1) & RF_IRQFLAGS1_MODEREADY) == 0x00) && (temp < 10));
  chprintf((BaseSequentialStream*)&SD3, "temp: %d, %x\r\n", temp, rfm69ReadRegister(REG_IRQFLAGS1));

}

void rfm69Stop(rfm69Config_t *rfm69cfg) {
  spiStop(rfm69cfg->spidp);

}

void rfm69ReadAllRegs(void) {
  uint8_t regVal, address;

/*
#if REGISTER_DETAIL
  int capVal;

  //... State Variables for intelligent decoding
  uint8_t modeFSK = 0;
  int bitRate = 0;
  int freqDev = 0;
  long freqCenter = 0;
#endif
*/

  chprintf((BaseSequentialStream*)&SD3, "Address - HEX - BIN\r\n");
  for (uint8_t regAddr = 1; regAddr <= 0x4F; regAddr++) {
    address = regAddr & 0x7F;

    spiAcquireBus(rfm69Config.spidp);
    spiSelect(rfm69Config.spidp);
    spiSend(rfm69Config.spidp, 1, (void *)&address);
    spiReceive(rfm69Config.spidp, 1, (void *)&regVal);
    spiUnselect(rfm69Config.spidp);
    spiReleaseBus(rfm69Config.spidp);

    chprintf((BaseSequentialStream*)&SD3, "%x - %x - %u\r\n", regAddr, regVal, regVal);
  }
}
