/*
 * httpd_handler_setting.h
 *
 *  Created on: 16. 1. 2026
 *      Author: vysocan
 */

#ifndef HTTPD_HANDLER_SETTING_H_
#define HTTPD_HANDLER_SETTING_H_


/*
 * @brief HTTP setting page handler
 * @param chp Pointer to the output stream
 */
static void fs_open_custom_setting(BaseSequentialStream *chp) {
  // Information table
  chprintf(chp, "%s%s / %s %s%s", HTML_tr_td, TEXT_Arm, TEXT_Authentication, TEXT_time,
           HTML_e_td_td);
  printIntInput(chp, 'C', conf.armDelay / 4, 3, 5, 60);
  chprintf(chp, " %s%s%s %s %s %s%s", durationSelect[0], HTML_e_td_e_tr_tr_td, TEXT_Auto,
           TEXT_arm, TEXT_zone, TEXT_delay, HTML_e_td_td);
  printIntInput(chp, 'E', conf.autoArm, 3, 1, 240);
  chprintf(chp, " %s%s%s %s %s %s%s", durationSelect[1], HTML_e_td_e_tr_tr_td, TEXT_Zone,
           TEXT_open, TEXT_alarm, TEXT_delay, HTML_e_td_td);
  printIntInput(chp, 'F', conf.openAlarm, 3, 1, 240);
  chprintf(chp, " %s%s%s %s%s", durationSelect[1], HTML_e_td_e_tr_tr_td, TEXT_Admin,
           TEXT_user, HTML_e_td_td);
  printTextInputWMin(chp, 'u', conf.user, NAME_LENGTH, MIN_PASS_LNEGTH);
  chprintf(chp, "%s%s %s%s", HTML_e_td_e_tr_tr_td, TEXT_Admin, TEXT_password,
           HTML_e_td_td);
  printPassInput(chp, 'p', conf.password, NAME_LENGTH, MIN_PASS_LNEGTH); chprintf(chp, "%s", HTML_br);
  printPassInput(chp, 'P', conf.password, NAME_LENGTH, MIN_PASS_LNEGTH);
  chprintf(chp, "%s%s", HTML_e_td_e_tr, HTML_e_table);

  chprintf(chp, "<h1>%s</h1>\r\n%s", TEXT_Radio, HTML_table);
  chprintf(chp, "%s%s%s", HTML_tr_td, TEXT_Key, HTML_e_td_td);
  printTextInputWMin(chp, 'K', conf.radioKey, RADIO_KEY_SIZE - 1, RADIO_KEY_SIZE - 1);
  chprintf(chp, "%s%s%s", HTML_e_td_e_tr_tr_td, TEXT_Frequency, HTML_e_td_td);
  printTwoButton(chp, "i", GET_CONF_SYSTEM_FLAG_RADIO_FREQ(conf.systemFlags), 0, 0b00,
                           "868 MHz", "915 MHz");
  chprintf(chp, "%s%s", HTML_e_td_e_tr, HTML_e_table);

  chprintf(chp, "<h1>%s</h1>\r\n%s", TEXT_SMTP, HTML_table);
  chprintf(chp, "%s%s %s%s", HTML_tr_td, TEXT_Server, TEXT_address, HTML_e_td_td);
  printTextInput(chp, 'a', conf.SMTPAddress, URL_LENGTH);
  chprintf(chp, "%s%s %s%s", HTML_e_td_e_tr_tr_td, TEXT_Server, TEXT_port, HTML_e_td_td);
  printIntInput(chp, 'b', conf.SMTPPort, 5, 0, 65535);
  chprintf(chp, "%s%s %s%s", HTML_e_td_e_tr_tr_td, TEXT_User, TEXT_name, HTML_e_td_td);
  printTextInput(chp, 'c', conf.SMTPUser, EMAIL_LENGTH);
  chprintf(chp, "%s%s %s%s", HTML_e_td_e_tr_tr_td, TEXT_User, TEXT_password, HTML_e_td_td);
  printPassInput(chp, 'd', conf.SMTPPassword, NAME_LENGTH, MIN_PASS_LNEGTH);
  // chprintf(chp, "%s", HTML_br); printPassInput(chp, 'D', conf.SMTPPassword, NAME_LENGTH, MIN_PASS_LNEGTH);
  chprintf(chp, "%s%s", HTML_e_td_e_tr, HTML_e_table);

  chprintf(chp, "<h1>%s</h1>\r\n%s", TEXT_MQTT, HTML_table);
  chprintf(chp, "%s%s %s%s", HTML_tr_td, TEXT_Server, TEXT_address, HTML_e_td_td);
  printTextInput(chp, 'y', conf.mqtt.address, URL_LENGTH);
  chprintf(chp, "%s%s %s%s", HTML_e_td_e_tr_tr_td, TEXT_Server, TEXT_port, HTML_e_td_td);
  printIntInput(chp, 'q', conf.mqtt.port, 5, 0, 65535);
  chprintf(chp, "%s%s %s%s", HTML_e_td_e_tr_tr_td, TEXT_User, TEXT_name, HTML_e_td_td);
  printTextInput(chp, 't', conf.mqtt.user, NAME_LENGTH);
  chprintf(chp, "%s%s %s%s", HTML_e_td_e_tr_tr_td, TEXT_User, TEXT_password, HTML_e_td_td);
  printPassInput(chp, 'r', conf.mqtt.password, NAME_LENGTH, MIN_PASS_LNEGTH);
  // chprintf(chp, "%s", HTML_br); printPassInput(chp, 'R', conf.mqtt.password, NAME_LENGTH, MIN_PASS_LNEGTH);
  chprintf(chp, "%s%s%s", HTML_e_td_e_tr_tr_td, TEXT_Connected, HTML_e_td_td);
  printOkNok(chp, !(GET_CONF_MQTT_CONNECT_ERROR(conf.mqtt.setting) | GET_CONF_MQTT_ADDRESS_ERROR(conf.mqtt.setting)));
  chprintf(chp, "%s%s %s%s", HTML_e_td_e_tr_tr_td, TEXT_Global, TEXT_Subscribe, HTML_e_td_td);
  printOnOffButton(chp, "0", GET_CONF_MQTT_SUBSCRIBE(conf.mqtt.setting));
  chprintf(chp, "%s%s %s %s %s%s", HTML_e_td_e_tr_tr_td, TEXT_Global, TEXT_Home, TEXT_Assistant, TEXT_Discovery, HTML_e_td_td);
  printOnOffButton(chp, "1", GET_CONF_MQTT_HAD(conf.mqtt.setting));
  chprintf(chp, "%s%s", HTML_e_td_e_tr, HTML_e_table);
  chprintf(chp, "<h1>%s</h1>\r\n%s", TEXT_NTP, HTML_table);
  chprintf(chp, "%s%s %s%s", HTML_tr_td, TEXT_Server, TEXT_address,
           HTML_e_td_td);
  printTextInput(chp, 'f', conf.SNTPAddress, URL_LENGTH);

  chprintf(chp, "%s%s %s%s", HTML_tr_td, TEXT_DS, TEXT_start, HTML_e_td_td);
  chprintf(chp, "%sW%s", HTML_select, HTML_e_tag);
  for (uint8_t i = 0; i < ARRAY_SIZE(weekNumber); i++) {
    chprintf(chp, "%s%u", HTML_option, i);
    if (conf.timeDstWeekNum == i) { chprintf(chp, "%s", HTML_selected); }
    else                          { chprintf(chp, "%s", HTML_e_tag); }
    chprintf(chp, "%s%s", weekNumber[i], HTML_e_option);
  }
  chprintf(chp, "%s", HTML_e_select);
  chprintf(chp, "%sS%s", HTML_select, HTML_e_tag);
  for (uint8_t i = 0; i < ARRAY_SIZE(weekDay); i++) {
    chprintf(chp, "%s%u", HTML_option, i);
    if (conf.timeDstDow == i) { chprintf(chp, "%s", HTML_selected); }
    else                      { chprintf(chp, "%s", HTML_e_tag); }
    chprintf(chp, "%s%s", weekDay[i], HTML_e_option);
  }
  chprintf(chp, "%s %s ", HTML_e_select, TEXT_of);
  chprintf(chp, "%sM%s", HTML_select, HTML_e_tag);
  for (uint8_t i = 0; i < ARRAY_SIZE(monthName); i++) {
    chprintf(chp, "%s%u", HTML_option, i);
    if (conf.timeDstMonth == i) { chprintf(chp, "%s", HTML_selected); }
    else                        { chprintf(chp, "%s", HTML_e_tag); }
    chprintf(chp, "%s%s", monthName[i], HTML_e_option);
  }
  chprintf(chp, "%s %s ", HTML_e_select, TEXT_at);
  printIntInput(chp, 'h', conf.timeDstHour , 2, 0, 23);
  chprintf(chp, " %s", TEXT_oclock);
  chprintf(chp, "%s%s %s%s", HTML_e_td_e_tr_tr_td, TEXT_DS, TEXT_offset, HTML_e_td_td);
  printIntInput(chp, 'O', conf.timeDstOffset, 5, -1440, 1440);
  chprintf(chp, " %s%s", durationSelect[1], HTML_e_td_e_tr_tr_td);

  chprintf(chp, "%s %s%s", TEXT_DS, TEXT_end, HTML_e_td_td);
  chprintf(chp, "%sw%s", HTML_select, HTML_e_tag);
  for (uint8_t i = 0; i < ARRAY_SIZE(weekNumber); i++) {
    chprintf(chp, "%s%u", HTML_option, i);
    if (conf.timeStdWeekNum == i) { chprintf(chp, "%s", HTML_selected); }
    else                          { chprintf(chp, "%s", HTML_e_tag); }
    chprintf(chp, "%s%s", weekNumber[i], HTML_e_option);
  }
  chprintf(chp, "%s", HTML_e_select);
  chprintf(chp, "%ss%s", HTML_select, HTML_e_tag);
  for (uint8_t i = 0; i < ARRAY_SIZE(weekDay); i++) {
    chprintf(chp, "%s%u", HTML_option, i);
    if (conf.timeStdDow == i) { chprintf(chp, "%s", HTML_selected); }
    else                      { chprintf(chp, "%s", HTML_e_tag); }
    chprintf(chp, "%s%s", weekDay[i], HTML_e_option);
  }
  chprintf(chp, "%s %s ", HTML_e_select, TEXT_of);
  chprintf(chp, "%sm%s", HTML_select, HTML_e_tag);
  for (uint8_t i = 0; i < ARRAY_SIZE(monthName); i++) {
    chprintf(chp, "%s%u", HTML_option, i);
    if (conf.timeStdMonth == i) { chprintf(chp, "%s", HTML_selected); }
    else                        { chprintf(chp, "%s", HTML_e_tag); }
    chprintf(chp, "%s%s", monthName[i], HTML_e_option);
  }
  chprintf(chp, "%s %s ", HTML_e_select, TEXT_at);
  printIntInput(chp, 'h', conf.timeStdHour , 2, 0, 23);
  chprintf(chp, " %s", TEXT_oclock);
  chprintf(chp, "%s%s %s%s", HTML_e_td_e_tr_tr_td,
           TEXT_Standard, TEXT_offset, HTML_e_td_td);
  printIntInput(chp, 'o', conf.timeStdOffset, 5, -1440, 1440);
  chprintf(chp, " %s%s", durationSelect[1], HTML_e_td_e_tr_tr_td);
  chprintf(chp, "%s %s%s", TEXT_Time, TEXT_format, HTML_e_td_td);
  printTextInput(chp, 'g', conf.dateTimeFormat, NAME_LENGTH);
  chprintf(chp, "%s%s", HTML_e_td_e_tr, HTML_e_table);

  // JavaScript
  chprintf(chp, "%s%s%s", HTML_script, JS_Credential, HTML_e_script);
  chprintf(chp, "%sPass.js'>%s", HTML_script_src, HTML_e_script);
  // Buttons
  chprintf(chp, "%s%s", HTML_ApplyValPass, HTML_Save);
}

/*
 * @brief HTTP setting POST handler
 * @param postDataP Pointer to POST data string
 */
static void httpd_post_custom_setting(char **postDataP) {
  uint16_t valueLen = 0;
  int8_t resp;
  char name[3];
  bool repeat;
  char *valueP;

  do {
    repeat = getPostData(postDataP, &name[0], sizeof(name), &valueP, &valueLen);
    DBG_HTTP("Parse: %s = '%.*s' (%u)\r\n", name, valueLen, valueP, valueLen);
    switch(name[0]) {
      case 'A': // Apply
        // SMTP
        smtp_set_server_addr(conf.SMTPAddress);
        smtp_set_server_port(conf.SMTPPort);
        smtp_set_auth(conf.SMTPUser, conf.SMTPPassword);
        // SNTP
        sntp_setservername(0, conf.SNTPAddress);
      break;
      case 'C':
        conf.armDelay = strtol(valueP, NULL, 10) * 4;
      break;
      case 'E':
        conf.autoArm = strtol(valueP, NULL, 10);
      break;
      case 'F':
        conf.openAlarm = strtol(valueP, NULL, 10);
      break;
      case 'u': // user
        strncpy(conf.user, valueP, LWIP_MIN(valueLen, NAME_LENGTH - 1));
        conf.user[LWIP_MIN(valueLen, NAME_LENGTH - 1)] = 0;
      break;
      case 'p': // password
        strncpy(conf.password, valueP, LWIP_MIN(valueLen, NAME_LENGTH - 1));
        conf.password[LWIP_MIN(valueLen, NAME_LENGTH - 1)] = 0;
      break;
      case 'a': // SMTP server
        strncpy(conf.SMTPAddress, valueP, LWIP_MIN(valueLen, URL_LENGTH - 1));
        conf.SMTPAddress[LWIP_MIN(valueLen, URL_LENGTH - 1)] = 0;
      break;
      case 'b': // SMTP port
        conf.SMTPPort = strtol(valueP, NULL, 10);
      break;
      case 'c': // SMTP user
        strncpy(conf.SMTPUser, valueP, LWIP_MIN(valueLen, EMAIL_LENGTH - 1));
        conf.SMTPUser[LWIP_MIN(valueLen, EMAIL_LENGTH - 1)] = 0;
      break;
      case 'd': // SMTP password
        strncpy(conf.SMTPPassword, valueP, LWIP_MIN(valueLen, NAME_LENGTH - 1));
        conf.SMTPPassword[LWIP_MIN(valueLen, NAME_LENGTH - 1)] = 0;
      break;
      case 'y': // MQTT server
        // Compute resp
        if (strlen(conf.mqtt.address) != valueLen) resp = 1;
        else resp = strncmp(conf.mqtt.address, valueP, LWIP_MIN(valueLen, NAME_LENGTH - 1));
        // Copy name
        strncpy(conf.mqtt.address, valueP, LWIP_MIN(valueLen, URL_LENGTH - 1));
        conf.mqtt.address[LWIP_MIN(valueLen, URL_LENGTH - 1)] = 0;
        // Clear MQTT resolve flag
        if (resp) CLEAR_CONF_MQTT_ADDRESS_ERROR(conf.mqtt.setting);
      break;
      case 'q': // MQTT port
        conf.mqtt.port = strtol(valueP, NULL, 10);
      break;
      case 't': // MQTT user
        strncpy(conf.mqtt.user, valueP, LWIP_MIN(valueLen, NAME_LENGTH - 1));
        conf.mqtt.user[LWIP_MIN(valueLen, NAME_LENGTH - 1)] = 0;
      break;
      case 'r': // MQTT password
        strncpy(conf.mqtt.password, valueP, LWIP_MIN(valueLen, NAME_LENGTH - 1));
        conf.mqtt.password[LWIP_MIN(valueLen, NAME_LENGTH - 1)] = 0;
      break;
      case '0' ... '1': // MQTT handle all single radio buttons for settings
        resp = GET_CONF_MQTT_HAD(conf.mqtt.setting);
        if (valueP[0] == '0') conf.mqtt.setting &= ~(1 << (name[0]-48));
        else                  conf.mqtt.setting |=  (1 << (name[0]-48));
        // Handle HAD change
        if (resp != GET_CONF_MQTT_HAD(conf.mqtt.setting)) {
          mqttGlobalHAD(GET_CONF_MQTT_HAD(conf.mqtt.setting));
        }
      break;
      case 'f': // NTP server
        strncpy(conf.SNTPAddress, valueP, LWIP_MIN(valueLen, URL_LENGTH - 1));
        conf.SNTPAddress[LWIP_MIN(valueLen, URL_LENGTH - 1)] = 0;
      break;
      case 'w':
        conf.timeStdWeekNum = strtol(valueP, NULL, 10);
      break;
      case 's':
        conf.timeStdDow = strtol(valueP, NULL, 10);
      break;
      case 'm':
        conf.timeStdMonth = strtol(valueP, NULL, 10);
      break;
      case 'h':
        conf.timeStdHour = strtol(valueP, NULL, 10);
      break;
      case 'o':
        conf.timeStdOffset = strtol(valueP, NULL, 10);
      break;
      case 'W':
        conf.timeDstWeekNum = strtol(valueP, NULL, 10);
      break;
      case 'S':
        conf.timeDstDow = strtol(valueP, NULL, 10);
      break;
      case 'M':
        conf.timeDstMonth = strtol(valueP, NULL, 10);
      break;
      case 'H':
        conf.timeDstHour = strtol(valueP, NULL, 10);
      break;
      case 'O':
        conf.timeDstOffset = strtol(valueP, NULL, 10);
      break;
      case 'g': // time format
        strncpy(conf.dateTimeFormat, valueP, LWIP_MIN(valueLen, NAME_LENGTH - 1));
        conf.dateTimeFormat[LWIP_MIN(valueLen, NAME_LENGTH - 1)] = 0;
      break;
      case 'K': // radio key
        strncpy(conf.radioKey, valueP, LWIP_MIN(valueLen, RADIO_KEY_SIZE - 1));
        conf.radioKey[LWIP_MIN(valueLen, RADIO_KEY_SIZE - 1)] = 0;
      break;
      case 'e': // save
        writeToBkpSRAM((uint8_t*)&conf, sizeof(config_t), 0);
      break;
    }
  } while (repeat);
}


#endif /* HTTPD_HANDLER_SETTING_H_ */
