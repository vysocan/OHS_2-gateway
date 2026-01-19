/*
 * httpd_handler_log.h
 *
 *  Created on: 16. 1. 2026
 *      Author: vysocan
 */

#ifndef HTTPD_HANDLER_LOG_H_
#define HTTPD_HANDLER_LOG_H_


/*
 * @brief HTTP log page handler
 * @param chp Pointer to the output stream
 */
static void fs_open_custom_log(BaseSequentialStream *chp) {
  uint16_t logAddress;
  chprintf(chp, "%s#", HTML_tr_th);
  chprintf(chp, "%s%s", HTML_e_th_th, TEXT_Date);
  chprintf(chp, "%s%s", HTML_e_th_th, TEXT_Entry);
  chprintf(chp, "%s%s%s\r\n", HTML_e_th_th, TEXT_Alert, HTML_e_th_e_tr);

  // Information table
  spiAcquireBus(&SPID1);              // Acquire ownership of the bus.
  for (uint8_t i = 0; i < LOGGER_OUTPUT_LEN; i++) {
    logAddress = webLog + (i * FRAM_MSG_SIZE);
    chprintf(chp, "%s%u.%s", HTML_tr_td, (logAddress / FRAM_MSG_SIZE) + 1 , HTML_e_td_td);

    getLogEntry(logAddress);
    memcpy(&timeConv.ch[0], &rxBuffer[0], sizeof(timeConv.ch)); // Prepare timestamp
    decodeLog(&rxBuffer[4], logText, 1);

    printFrmTimestamp(chp, &timeConv.val);
    chprintf(chp, "%s%s.", HTML_e_td_td, logText);
    chprintf(chp, "%s", HTML_e_td_td);
    for (uint8_t j = 0; j < ARRAY_SIZE(alertType); j++) {
      if ((((uint8_t)rxBuffer[FRAM_MSG_SIZE-1] >> j) & 0b1) == 1)
        chprintf(chp, "%s ", alertType[j].name);
    }
  }
  spiReleaseBus(&SPID1);              // Ownership release.
  chprintf(chp, "%s%s", HTML_e_td_e_tr, HTML_e_table);
  // Buttons
  chprintf(chp, "%s%s%s", HTML_FR, HTML_Now, HTML_FF);
}

/*
 * @brief HTTP log POST handler
 * @param postDataP Pointer to POST data string
 */
static void httpd_post_custom_log(char **postDataP) {
  uint16_t valueLen = 0;
  char name[3];
  bool repeat;
  char *valueP;

  do {
    repeat = getPostData(postDataP, &name[0], sizeof(name), &valueP, &valueLen);
    DBG_HTTP("Parse: %s = '%.*s' (%u)\r\n", name, valueLen, valueP, valueLen);
    switch(name[0]) {
      case 'N': // Now
        webLog = FRAMWritePos - (LOGGER_OUTPUT_LEN * FRAM_MSG_SIZE);
      break;
      case 'R': // Reverse
        webLog -= LOGGER_OUTPUT_LEN * FRAM_MSG_SIZE;
      break;
      case 'F': // Forward
        webLog += LOGGER_OUTPUT_LEN * FRAM_MSG_SIZE;
      break;
    }
  } while (repeat);
}


#endif /* HTTPD_HANDLER_LOG_H_ */
