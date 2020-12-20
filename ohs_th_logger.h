/*
 * ohs_th_logger.h
 *
 *  Created on: 23. 2. 2020
 *      Author: adam
 */

#ifndef OHS_TH_LOGGER_H_
#define OHS_TH_LOGGER_H_

/*
 * Logger thread
 */
static THD_WORKING_AREA(waLoggerThread, 128);
static THD_FUNCTION(LoggerThread, arg) {
  chRegSetThreadName(arg);
  msg_t    msg;
  loggerEvent_t *inMsg;
  char     buffer[FRAM_MSG_SIZE + FRAM_HEADER_SIZE];
  uint8_t  flag;

  while (true) {
    msg = chMBFetchTimeout(&logger_mb, (msg_t*)&inMsg, TIME_INFINITE);
    if (msg == MSG_OK) {
      // Check for alerts and set flag
      flag = 0;
      for (uint8_t i = 0; i < ARRAY_SIZE(alertDef); i++) {
        if (memcmp(&inMsg->text[0], alertDef[i], strlen(alertDef[i])) == 0) {
          for(uint8_t j = 0; j < ARRAY_SIZE(alertType); j++) {
            // Combine all alerts flags into flag(uint8_t) as bits
            flag |= (((conf.alert[j] >> i) & 0b1) << j);
          }
        }
      }
      /*
      chprintf(console, "Log %d", inMsg);
      chprintf(console, ", T %d", inMsg->timestamp);
      chprintf(console, ", %s", inMsg->text);
      chprintf(console, ", %u\r\n", flag);
      */
      // SPI
      spiAcquireBus(&SPID1);

      spiSelect(&SPID1);
      buffer[0] = CMD_25AA_WREN;
      spiSend(&SPID1, 1, buffer);
      spiUnselect(&SPID1);

      buffer[0] = CMD_25AA_WRITE;
      buffer[1] = 0;
      buffer[2] = (FRAMWritePos >> 8) & 0xFF;
      buffer[3] = FRAMWritePos & 0xFF;

      timeConv.val = inMsg->timestamp;
      memcpy(&buffer[FRAM_HEADER_SIZE], &timeConv.ch[0], sizeof(timeConv.ch));  // Copy time to buffer
      memcpy(&buffer[FRAM_HEADER_SIZE + 4], &inMsg->text[0], LOGGER_MSG_LENGTH); // Copy text to buffer
      buffer[FRAM_MSG_SIZE + FRAM_HEADER_SIZE - 1] = flag; // Set flag

      spiSelect(&SPID1);
      spiSend(&SPID1, FRAM_MSG_SIZE + FRAM_HEADER_SIZE, buffer);
      spiUnselect(&SPID1);
      spiReleaseBus(&SPID1);

      FRAMWritePos += FRAM_MSG_SIZE; // uint16_t will overflow by itself or FRAM address registers are ignored

      // Alerts
      if (flag > 0) {
        alertEvent_t *outMsg = chPoolAlloc(&alert_pool);
        if (outMsg != NULL) {
          memcpy(&outMsg->text[0], &inMsg->text[0], LOGGER_MSG_LENGTH);  // Copy string
          outMsg->flag = flag;

          msg_t msg = chMBPostTimeout(&alert_mb, (msg_t)outMsg, TIME_IMMEDIATE);
          if (msg != MSG_OK) {
            //chprintf(console, "MB full %d\r\n", temp);
          }
        } else {
          chprintf(console, "Alert FIFO full %d \r\n", outMsg);
        }
      }

    } else {
      chprintf(console, "Log ERROR\r\n");
    }
    chPoolFree(&logger_pool, inMsg);
  }
}

#endif /* OHS_TH_LOGGER_H_ */
