/*
 * httpd_handler_alert.h
 *
 *  Created on: 16. 1. 2026
 *      Author: vysocan
 */

#ifndef HTTPD_HANDLER_ALERT_H_
#define HTTPD_HANDLER_ALERT_H_

/*
 * @brief HTTP alert page handler
 * @param chp Pointer to the output stream
 */
static void fs_open_custom_alert(BaseSequentialStream *chp) {
  char temp[3] = "";
  chprintf(chp, "%s%s", html_tr_th, text_Name);
  for (uint8_t j = 0; j < ARRAY_SIZE(alertType); j++) {
    chprintf(chp, "%s%s", html_e_th_th, alertType[j].name);
  }
  chprintf(chp, "%s\r\n", html_e_th_e_tr);
  // Information table
  for (uint8_t i = 0; i < ARRAY_SIZE(alertDef); i++) {
    chprintf(chp, "%s", html_tr_td);
    decodeLog((char*)alertDef[i], logText, 0);
    chprintf(chp, "%s%s", logText, html_e_td_td);
    for (uint8_t j = 0; j < ARRAY_SIZE(alertType); j++) {
      temp[0] = '0' + j;
      temp[1] = 'A' + i;
      printOnOffButton(chp, temp, (conf.alert[j] >> i) & 0b1);
      if (j < (ARRAY_SIZE(alertType) - 1)) chprintf(chp, "%s", html_e_td_td);
    }
  }
  chprintf(chp, "%s%s", html_e_td_e_tr, html_e_table);
  // Buttons
  chprintf(chp, "%s%s%s", html_Apply, html_Save, html_SendTest);
}

/*
 * @brief HTTP alert POST handler
 * @param postDataP Pointer to POST data string
 */
static void httpd_post_custom_alert(char **postDataP) {
  uint16_t valueLen = 0;
  char name[3];
  bool repeat;
  char *valueP;

  do {
    repeat = getPostData(postDataP, &name[0], sizeof(name), &valueP, &valueLen);
    DBG_HTTP("Parse: %s = '%.*s' (%u)\r\n", name, valueLen, valueP, valueLen);
    switch(name[0]) {
      case '0' ... ('0' + ARRAY_SIZE(alertType)): // Handle all radio buttons in groups 0 .. #, A .. #
        if (valueP[0] == '0') conf.alert[name[0]-48] &= ~(1 << (name[1]-65));
        else conf.alert[name[0]-48] |= (1 << (name[1]-65));
      break;
      case 'e': // save
        writeToBkpSRAM((uint8_t*)&conf, sizeof(config_t), 0);
      break;
      case 'M': // dummy alert
        pushToLogText("D");
      break;
    }
  } while (repeat);
}


#endif /* HTTPD_HANDLER_ALERT_H_ */
