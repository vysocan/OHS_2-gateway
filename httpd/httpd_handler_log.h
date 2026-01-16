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
  chprintf(chp, "%s#", html_tr_th);
  chprintf(chp, "%s%s", html_e_th_th, text_Date);
  chprintf(chp, "%s%s", html_e_th_th, text_Entry);
  chprintf(chp, "%s%s%s\r\n", html_e_th_th, text_Alert, html_e_th_e_tr);

  // Information table
  spiAcquireBus(&SPID1);              // Acquire ownership of the bus.
  for (uint8_t i = 0; i < LOGGER_OUTPUT_LEN; i++) {
    logAddress = webLog + (i * FRAM_MSG_SIZE);
    chprintf(chp, "%s%u.%s", html_tr_td, (logAddress / FRAM_MSG_SIZE) + 1 , html_e_td_td);

    getLogEntry(logAddress);
    memcpy(&timeConv.ch[0], &rxBuffer[0], sizeof(timeConv.ch)); // Prepare timestamp
    decodeLog(&rxBuffer[4], logText, 1);

    printFrmTimestamp(chp, &timeConv.val);
    chprintf(chp, "%s%s.", html_e_td_td, logText);
    chprintf(chp, "%s", html_e_td_td);
    for (uint8_t j = 0; j < ARRAY_SIZE(alertType); j++) {
      if ((((uint8_t)rxBuffer[FRAM_MSG_SIZE-1] >> j) & 0b1) == 1)
        chprintf(chp, "%s ", alertType[j].name);
    }
  }
  spiReleaseBus(&SPID1);              // Ownership release.
  chprintf(chp, "%s%s", html_e_td_e_tr, html_e_table);
  // Buttons
  chprintf(chp, "%s%s%s", html_FR, html_Now, html_FF);
}


#endif /* HTTPD_HANDLER_LOG_H_ */
