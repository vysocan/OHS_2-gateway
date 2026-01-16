/*
 * httpd_handler_zone.h
 *
 *  Created on: 16. 1. 2026
 *      Author: vysocan
 */

#ifndef HTTPD_HANDLER_ZONE_H_
#define HTTPD_HANDLER_ZONE_H_


/*
 * @brief HTTP zone page handler
 * @param chp Pointer to the output stream
 */
static void fs_open_custom_zone(BaseSequentialStream *chp) {
  chprintf(chp, "%s#", html_tr_th);
  chprintf(chp, "%s%s", html_e_th_th, text_Name);
  chprintf(chp, "%s%s", html_e_th_th, text_On);
  chprintf(chp, "%s%s", html_e_th_th, text_Type);
  chprintf(chp, "%s%s %s", html_e_th_th, text_Arm, text_home);
  chprintf(chp, "%s%s %s", html_e_th_th, text_Open, text_alarm);
  chprintf(chp, "%s%s %s %s", html_e_th_th, text_Alarm, text_as, text_tamper);
  chprintf(chp, "%s%s", html_e_th_th, text_Delay);
  chprintf(chp, "%s%s %s", html_e_th_th, text_Last, text_alarm);
  chprintf(chp, "%s%s %s", html_e_th_th, text_Last, text_OK);
  chprintf(chp, "%s%s", html_e_th_th, text_MQTT);
  chprintf(chp, "%s%s", html_e_th_th, text_HAD);
  chprintf(chp, "%s%s", html_e_th_th, text_Status);
  chprintf(chp, "%s%s%s\r\n", html_e_th_th, text_Group, html_e_th_e_tr);
  // Information table
  for (uint8_t i = 0; i < ALARM_ZONES; i++) {
    if (GET_CONF_ZONE_IS_PRESENT(conf.zone[i])) {
      chprintf(chp, "%s%u.%s", html_tr_td, i + 1, html_e_td_td);
      chprintf(chp, "%s%s", conf.zoneName[i], html_e_td_td);
      printOkNok(chp, GET_CONF_ZONE_ENABLED(conf.zone[i]));
      chprintf(chp, "%s", html_e_td_td);
      if (GET_CONF_ZONE_ENABLED(conf.zone[i])) {
        GET_CONF_ZONE_BALANCED(conf.zone[i]) ? chprintf(chp, "%s ", text_balanced) : chprintf(chp, "un%s ", text_balanced);
        (i >= HW_ZONES) ? chprintf(chp, "%s ", text_remote) : chprintf(chp, "%s ", text_local);
        GET_CONF_ZONE_TYPE(conf.zone[i]) ? chprintf(chp, "%s", text_analog) : chprintf(chp, "%s", text_digital);
      }
      chprintf(chp, "%s", html_e_td_td);
      printOkNok(chp, GET_CONF_ZONE_ARM_HOME(conf.zone[i]));
      chprintf(chp, "%s", html_e_td_td);
      printOkNok(chp, GET_CONF_ZONE_OPEN_ALARM(conf.zone[i]));
      chprintf(chp, "%s", html_e_td_td);
      printOkNok(chp, GET_CONF_ZONE_PIR_AS_TMP(conf.zone[i]));
      chprintf(chp, "%s", html_e_td_td);
      chprintf(chp, "%u %s%s", GET_CONF_ZONE_AUTH_TIME(conf.zone[i])*conf.armDelay/4,
               durationSelect[0], html_e_td_td);
      if (GET_CONF_ZONE_ENABLED(conf.zone[i])) printFrmTimestamp(chp, &zone[i].lastPIR);
      chprintf(chp, "%s", html_e_td_td);
      if (GET_CONF_ZONE_ENABLED(conf.zone[i])) printFrmTimestamp(chp, &zone[i].lastOK);
      chprintf(chp, "%s", html_e_td_td);
      if (GET_CONF_ZONE_ENABLED(conf.zone[i])) printOkNok(chp, GET_CONF_ZONE_MQTT_PUB(conf.zone[i]));
      chprintf(chp, "%s", html_e_td_td);
      if (GET_CONF_ZONE_ENABLED(conf.zone[i])) printOkNok(chp, GET_CONF_ZONE_MQTT_HAD(conf.zone[i]));
      chprintf(chp, "%s", html_e_td_td);
      if (GET_CONF_ZONE_ENABLED(conf.zone[i])) {
        switch(zone[i].lastEvent){
          case 'O': chprintf(chp, "%s", html_i_OK); break;
          case 'P': chprintf(chp, "%s", html_i_alarm); break;
          case 'N': chprintf(chp, "%s", html_i_starting); break;
          default: chprintf(chp, "%s", text_tamper); break;
        }
      } else { chprintf(chp, "%s", html_i_disabled); }
      chprintf(chp, "%s", html_e_td_td);
      printGroup(chp, GET_CONF_ZONE_GROUP(conf.zone[i]));
      chprintf(chp, "%s", html_e_td_e_tr);
    }
  }
  chprintf(chp, "%s", html_e_table);
  chprintf(chp, "%s", html_table);
  chprintf(chp, "%s%s%s", html_tr_td, text_Zone, html_e_td_td);
  chprintf(chp, "%sP%s", html_select_submit, html_e_tag);
  for (uint8_t i = 0; i < ALARM_ZONES; i++) {
    if (GET_CONF_ZONE_IS_PRESENT(conf.zone[i])) {
      chprintf(chp, "%s%u", html_option, i);
      if (webZone == i) { chprintf(chp, "%s", html_selected); }
      else             { chprintf(chp, "%s", html_e_tag); }
      chprintf(chp, "%u. %s%s", i + 1, conf.zoneName[i], html_e_option);
    }
  }
  chprintf(chp, "%s%s", html_e_select, html_e_td_e_tr_tr_td);
  chprintf(chp, "%s%s", text_Name, html_e_td_td);
  printTextInput(chp, 'n', conf.zoneName[webZone], NAME_LENGTH);
  chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_Zone, text_is, html_e_td_td);
  printOnOffButton(chp, "0", GET_CONF_ZONE_ENABLED(conf.zone[webZone]));
  chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_Arm, text_home, html_e_td_td);
  printOnOffButton(chp, "7", GET_CONF_ZONE_ARM_HOME(conf.zone[webZone]));
  chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_Open, text_alarm, html_e_td_td);
  printOnOffButton(chp, "8", GET_CONF_ZONE_OPEN_ALARM(conf.zone[webZone]));
  chprintf(chp, "%s%s %s %s%s", html_e_td_e_tr_tr_td,  text_Alarm, text_as, text_tamper, html_e_td_td);
  printOnOffButton(chp, "9", GET_CONF_ZONE_PIR_AS_TMP(conf.zone[webZone]));
  chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, text_Balanced, html_e_td_td);
  printOnOffButton(chp, "a", GET_CONF_ZONE_BALANCED(conf.zone[webZone]));
  chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_Authentication, text_delay, html_e_td_td);
  printFourButton(chp, "D", GET_CONF_ZONE_AUTH_TIME(conf.zone[webZone]), 0, 0b0000,
                  text_0x, text_1x, text_2x, text_3x, 0);
  chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_MQTT, text_publish, html_e_td_td);
            printOnOffButton(chp, "d", GET_CONF_ZONE_MQTT_PUB(conf.zone[webZone]));
  chprintf(chp, "%s%s %s %s%s", html_e_td_e_tr_tr_td, text_MQTT, text_HA, text_Discovery, html_e_td_td);
                      printOnOffButton(chp, "c", GET_CONF_ZONE_MQTT_HAD(conf.zone[webZone]));
  chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, text_Group, html_e_td_td);
  selectGroup(chp, GET_CONF_ZONE_GROUP(conf.zone[webZone]), 'g');
  chprintf(chp, "%s%s", html_e_td_e_tr, html_e_table);
  // JavaScript
  chprintf(chp, "%s%s%s", html_script, JSZone, html_e_script);
  // Buttons
  chprintf(chp, "%s%s", html_Apply, html_Save);
}


#endif /* HTTPD_HANDLER_ZONE_H_ */
