/*
 * httpd_handler_home.h
 *
 *  Created on: 16. 1. 2026
 *      Author: vysocan
 */

#ifndef HTTPD_HANDLER_HOME_H_
#define HTTPD_HANDLER_HOME_H_


/*
 * @brief HTTP home page handler
 * @param chp Pointer to the output stream
 */
static void fs_open_custom_home(BaseSequentialStream *chp) {
  time_t tempTime;
  // Information table
  chprintf(chp, "%s%s%s", html_tr_td, text_Time, html_e_td_td);
  tempTime = getTimeUnixSec(); printFrmTimestamp(chp, &tempTime);
  chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_Start, text_time, html_e_td_td);
  printFrmTimestamp(chp, &startTime);
  chprintf(chp, "%s%s%s%s", html_e_td_e_tr_tr_td, text_Up, text_time, html_e_td_td);
  tempTime -= startTime; printFrmUpTime(chp, &tempTime);
  chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_AC, text_power, html_e_td_td);
  printOkNok(chp, !(palReadPad(GPIOD, GPIOD_AC_OFF)));
  chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, text_Battery, html_e_td_td);
  printOkNok(chp, palReadPad(GPIOD, GPIOD_BAT_OK));
  chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_RTC, text_battery, html_e_td_td);
  printOkNok(chp, ((rtcVbat > ADC_VBAT_HIGH_VOLTAGE)?1:0));
  chprintf(chp, " (%.2f V)", rtcVbat);
  chprintf(chp, "%s%s", html_e_td_e_tr, html_e_table);
  chprintf(chp, "<h1>%s</h1>\r\n", text_Modem);
  chprintf(chp, "%s%s", html_table, html_tr_td);
  chprintf(chp, "%s%s%s", text_Type, html_e_td_td, modemModelInfo);
  chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, text_On, html_e_td_td);
  printOkNok(chp, !palReadPad(GPIOD, GPIOD_GSM_STATUS));
  chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, text_Alive, html_e_td_td);
  printOkNok(chp, modemIsAlive);
  chprintf(chp, "%s%sed%s", html_e_td_e_tr_tr_td, text_Register, html_e_td_td);
  switch(modemReg){
    case 0 : chprintf(chp, "%s", html_i_OK); break;
    case 1 : chprintf(chp, "%s", html_i_home); break;
    case 2 : chprintf(chp, "%s", html_i_starting); break;
    case 3 : chprintf(chp, "%s", html_i_disabled); break;
    case 5 : chprintf(chp, "%s", html_i_globe); break;
    default : chprintf(chp, "%s", html_i_question);; break; // case 4
  }
  chprintf(chp, "%s%s %s%s%u%%", html_e_td_e_tr_tr_td, text_Signal, text_strength, html_e_td_td, modemSigStrength);
  // +CPSI: GSM,Online,230-02,0x0726,4285,69 // remove +CPSI: &[7]
  chprintf(chp, "%s%s %s%s%s%", html_e_td_e_tr_tr_td, text_System, text_info, html_e_td_td, &modemSystemInfo[7]);
  chprintf(chp, "%s%s", html_e_td_e_tr, html_e_table);
  // Buttons
  chprintf(chp, "%s%s", html_LoadDefault, html_Save);
  // TODO OHS add configuration download/upload
  //chprintf(chp, "<button type=\"submit\" onclick=\"window.open('/config.bin')\">Download</button>");
}

/*
 * @brief HTTP home POST handler
 * @param postDataP Pointer to POST data string
 */
static void httpd_post_custom_home(char **postDataP) {
  uint16_t valueLen = 0;
  char name[3];
  bool repeat;
  char *valueP;

  do {
    repeat = getPostData(postDataP, &name[0], sizeof(name), &valueP, &valueLen);
    DBG_HTTP("Parse: %s = '%.*s' (%u)\r\n", name, valueLen, valueP, valueLen);
    switch(name[0]) {
      case 'e': // save
        writeToBkpSRAM((uint8_t*)&conf, sizeof(config_t), 0);
      break;
      case 'D': // Load defaults
        setConfDefault();    // Load OHS default conf.
        initRuntimeGroups(); // Initialize runtime variables
      break;
    }
  } while (repeat);
}


#endif /* HTTPD_HANDLER_HOME_H_ */
