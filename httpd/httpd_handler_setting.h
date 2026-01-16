/*
 * httpd_handler_setting.h
 *
 *  Created on: 16. 1. 2026
 *      Author: vysocan
 */

#ifndef HTTPD_HANDLER_SETTING_H_
#define HTTPD_HANDLER_SETTING_H_


static void fs_open_custom_setting(BaseSequentialStream *chp) {
  // Information table
  chprintf(chp, "%s%s / %s %s%s", html_tr_td, text_Arm, text_Authentication, text_time,
           html_e_td_td);
  printIntInput(chp, 'C', conf.armDelay / 4, 3, 5, 60);
  chprintf(chp, " %s%s%s %s %s %s%s", durationSelect[0], html_e_td_e_tr_tr_td, text_Auto,
           text_arm, text_zone, text_delay, html_e_td_td);
  printIntInput(chp, 'E', conf.autoArm, 3, 1, 240);
  chprintf(chp, " %s%s%s %s %s %s%s", durationSelect[1], html_e_td_e_tr_tr_td, text_Zone,
           text_open, text_alarm, text_delay, html_e_td_td);
  printIntInput(chp, 'F', conf.openAlarm, 3, 1, 240);
  chprintf(chp, " %s%s%s %s%s", durationSelect[1], html_e_td_e_tr_tr_td, text_Admin,
           text_user, html_e_td_td);
  printTextInputWMin(chp, 'u', conf.user, NAME_LENGTH, MIN_PASS_LNEGTH);
  chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_Admin, text_password,
           html_e_td_td);
  printPassInput(chp, 'p', conf.password, NAME_LENGTH, MIN_PASS_LNEGTH); chprintf(chp, "%s", html_br);
  printPassInput(chp, 'P', conf.password, NAME_LENGTH, MIN_PASS_LNEGTH);
  chprintf(chp, "%s%s", html_e_td_e_tr, html_e_table);

  chprintf(chp, "<h1>%s</h1>\r\n%s", text_Radio, html_table);
  chprintf(chp, "%s%s%s", html_tr_td, text_Key, html_e_td_td);
  printTextInputWMin(chp, 'K', conf.radioKey, RADIO_KEY_SIZE - 1, RADIO_KEY_SIZE - 1);
  chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, text_Frequency, html_e_td_td);
  printTwoButton(chp, "i", GET_CONF_SYSTEM_FLAG_RADIO_FREQ(conf.systemFlags), 0, 0b00,
                           "868 MHz", "915 MHz");
  chprintf(chp, "%s%s", html_e_td_e_tr, html_e_table);

  chprintf(chp, "<h1>%s</h1>\r\n%s", text_SMTP, html_table);
  chprintf(chp, "%s%s %s%s", html_tr_td, text_Server, text_address, html_e_td_td);
  printTextInput(chp, 'a', conf.SMTPAddress, URL_LENGTH);
  chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_Server, text_port, html_e_td_td);
  printIntInput(chp, 'b', conf.SMTPPort, 5, 0, 65535);
  chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_User, text_name, html_e_td_td);
  printTextInput(chp, 'c', conf.SMTPUser, EMAIL_LENGTH);
  chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_User, text_password, html_e_td_td);
  printPassInput(chp, 'd', conf.SMTPPassword, NAME_LENGTH, MIN_PASS_LNEGTH);
  // chprintf(chp, "%s", html_br); printPassInput(chp, 'D', conf.SMTPPassword, NAME_LENGTH, MIN_PASS_LNEGTH);
  chprintf(chp, "%s%s", html_e_td_e_tr, html_e_table);

  chprintf(chp, "<h1>%s</h1>\r\n%s", text_MQTT, html_table);
  chprintf(chp, "%s%s %s%s", html_tr_td, text_Server, text_address, html_e_td_td);
  printTextInput(chp, 'y', conf.mqtt.address, URL_LENGTH);
  chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_Server, text_port, html_e_td_td);
  printIntInput(chp, 'q', conf.mqtt.port, 5, 0, 65535);
  chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_User, text_name, html_e_td_td);
  printTextInput(chp, 't', conf.mqtt.user, NAME_LENGTH);
  chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_User, text_password, html_e_td_td);
  printPassInput(chp, 'r', conf.mqtt.password, NAME_LENGTH, MIN_PASS_LNEGTH);
  // chprintf(chp, "%s", html_br); printPassInput(chp, 'R', conf.mqtt.password, NAME_LENGTH, MIN_PASS_LNEGTH);
  chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, text_Connected, html_e_td_td);
  printOkNok(chp, !(GET_CONF_MQTT_CONNECT_ERROR(conf.mqtt.setting) | GET_CONF_MQTT_ADDRESS_ERROR(conf.mqtt.setting)));
  chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_Global, text_Subscribe, html_e_td_td);
  printOnOffButton(chp, "0", GET_CONF_MQTT_SUBSCRIBE(conf.mqtt.setting));
  chprintf(chp, "%s%s %s %s %s%s", html_e_td_e_tr_tr_td, text_Global, text_Home, text_Assistant, text_Discovery, html_e_td_td);
  printOnOffButton(chp, "1", GET_CONF_MQTT_HAD(conf.mqtt.setting));
  chprintf(chp, "%s%s", html_e_td_e_tr, html_e_table);
  chprintf(chp, "<h1>%s</h1>\r\n%s", text_NTP, html_table);
  chprintf(chp, "%s%s %s%s", html_tr_td, text_Server, text_address,
           html_e_td_td);
  printTextInput(chp, 'f', conf.SNTPAddress, URL_LENGTH);

  chprintf(chp, "%s%s %s%s", html_tr_td, text_DS, text_start, html_e_td_td);
  chprintf(chp, "%sW%s", html_select, html_e_tag);
  for (uint8_t i = 0; i < ARRAY_SIZE(weekNumber); i++) {
    chprintf(chp, "%s%u", html_option, i);
    if (conf.timeDstWeekNum == i) { chprintf(chp, "%s", html_selected); }
    else                          { chprintf(chp, "%s", html_e_tag); }
    chprintf(chp, "%s%s", weekNumber[i], html_e_option);
  }
  chprintf(chp, "%s", html_e_select);
  chprintf(chp, "%sS%s", html_select, html_e_tag);
  for (uint8_t i = 0; i < ARRAY_SIZE(weekDay); i++) {
    chprintf(chp, "%s%u", html_option, i);
    if (conf.timeDstDow == i) { chprintf(chp, "%s", html_selected); }
    else                      { chprintf(chp, "%s", html_e_tag); }
    chprintf(chp, "%s%s", weekDay[i], html_e_option);
  }
  chprintf(chp, "%s %s ", html_e_select, text_of);
  chprintf(chp, "%sM%s", html_select, html_e_tag);
  for (uint8_t i = 0; i < ARRAY_SIZE(monthName); i++) {
    chprintf(chp, "%s%u", html_option, i);
    if (conf.timeDstMonth == i) { chprintf(chp, "%s", html_selected); }
    else                        { chprintf(chp, "%s", html_e_tag); }
    chprintf(chp, "%s%s", monthName[i], html_e_option);
  }
  chprintf(chp, "%s %s ", html_e_select, text_at);
  printIntInput(chp, 'h', conf.timeDstHour , 2, 0, 23);
  chprintf(chp, " %s", text_oclock);
  chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_DS, text_offset, html_e_td_td);
  printIntInput(chp, 'O', conf.timeDstOffset, 5, -1440, 1440);
  chprintf(chp, " %s%s", durationSelect[1], html_e_td_e_tr_tr_td);

  chprintf(chp, "%s %s%s", text_DS, text_end, html_e_td_td);
  chprintf(chp, "%sw%s", html_select, html_e_tag);
  for (uint8_t i = 0; i < ARRAY_SIZE(weekNumber); i++) {
    chprintf(chp, "%s%u", html_option, i);
    if (conf.timeStdWeekNum == i) { chprintf(chp, "%s", html_selected); }
    else                          { chprintf(chp, "%s", html_e_tag); }
    chprintf(chp, "%s%s", weekNumber[i], html_e_option);
  }
  chprintf(chp, "%s", html_e_select);
  chprintf(chp, "%ss%s", html_select, html_e_tag);
  for (uint8_t i = 0; i < ARRAY_SIZE(weekDay); i++) {
    chprintf(chp, "%s%u", html_option, i);
    if (conf.timeStdDow == i) { chprintf(chp, "%s", html_selected); }
    else                      { chprintf(chp, "%s", html_e_tag); }
    chprintf(chp, "%s%s", weekDay[i], html_e_option);
  }
  chprintf(chp, "%s %s ", html_e_select, text_of);
  chprintf(chp, "%sm%s", html_select, html_e_tag);
  for (uint8_t i = 0; i < ARRAY_SIZE(monthName); i++) {
    chprintf(chp, "%s%u", html_option, i);
    if (conf.timeStdMonth == i) { chprintf(chp, "%s", html_selected); }
    else                        { chprintf(chp, "%s", html_e_tag); }
    chprintf(chp, "%s%s", monthName[i], html_e_option);
  }
  chprintf(chp, "%s %s ", html_e_select, text_at);
  printIntInput(chp, 'h', conf.timeStdHour , 2, 0, 23);
  chprintf(chp, " %s", text_oclock);
  chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td,
           text_Standard, text_offset, html_e_td_td);
  printIntInput(chp, 'o', conf.timeStdOffset, 5, -1440, 1440);
  chprintf(chp, " %s%s", durationSelect[1], html_e_td_e_tr_tr_td);
  chprintf(chp, "%s %s%s", text_Time, text_format, html_e_td_td);
  printTextInput(chp, 'g', conf.dateTimeFormat, NAME_LENGTH);
  chprintf(chp, "%s%s", html_e_td_e_tr, html_e_table);

  // JavaScript
  chprintf(chp, "%s%s%s", html_script, JSCredential, html_e_script);
  chprintf(chp, "%sPass.js'>%s", html_script_src, html_e_script);
  // Buttons
  chprintf(chp, "%s%s", html_ApplyValPass, html_Save);
}


#endif /* HTTPD_HANDLER_SETTING_H_ */
