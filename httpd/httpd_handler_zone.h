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
  chprintf(chp, "%s%s %s", html_e_th_th, text_Arm, text_Home);
  chprintf(chp, "%s%s %s", html_e_th_th, text_Open, text_Alarm);
  chprintf(chp, "%s%s %s", html_e_th_th, text_As, text_Tamper);
  chprintf(chp, "%s%s", html_e_th_th, text_Needed);
  chprintf(chp, "%s%s", html_e_th_th, text_Delay);
  chprintf(chp, "%s%s %s", html_e_th_th, text_Last, text_Alarm);
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
      printOkNok(chp, GET_CONF_ZONE_NEEDED(conf.zone[i]));
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
  chprintf(chp, "%s%s %s %s%s", html_e_td_e_tr_tr_td, text_Needed, text_to, text_arm, html_e_td_td);
  printOnOffButton(chp, "b", GET_CONF_ZONE_NEEDED(conf.zone[webZone]));
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

/*
 * @brief HTTP zone POST handler
 * @param postDataP Pointer to POST data string
 */
static void httpd_post_custom_zone(char **postDataP) {
  uint16_t number, valueLen = 0;
  int8_t resp;
  char name[3];
  uint8_t message[REG_PACKET_SIZE + 1];
  bool repeat;
  char *valueP;

  do {
    repeat = getPostData(postDataP, &name[0], sizeof(name), &valueP, &valueLen);
    DBG_HTTP("Parse: %s = '%.*s' (%u)\r\n", name, valueLen, valueP, valueLen);
    switch(name[0]) {
      case 'P': // select
        number = strtol(valueP, NULL, 10);
        if (number != webZone) { webZone = number; repeat = 0; }
      break;
      case 'A': // Apply, for remote zone send packet.
        if (GET_CONF_ZONE_ENABLED(conf.zone[webZone]) &&
            GET_CONF_ZONE_MQTT_PUB(conf.zone[webZone])) {
          pushToMqtt(typeZone, webZone, functionState);
        }
        // Send remote zone changes
        if (webZone >= HW_ZONES) {
          message[0] = 'R';
          message[1] = 'Z';
          message[2] = (GET_CONF_ZONE_TYPE(conf.zone[webZone])) ? 'A' : 'D';
          message[3] = webZone + 1;
          message[4] = (uint8_t)((conf.zone[webZone] >> 8) & 0b11111111);;
          message[5] = (uint8_t)(conf.zone[webZone] & 0b11111111);
          memcpy(&message[6], conf.zoneName[webZone], NAME_LENGTH);
          resp = sendData(conf.zoneAddress[webZone-HW_ZONES], message, REG_PACKET_SIZE + 1);
        }
      break;
      case 'n': // name
        // Calculate resp for MQTT
        if (GET_CONF_ZONE_ENABLED(conf.zone[webZone]) &&
            GET_CONF_ZONE_MQTT_PUB(conf.zone[webZone])) {
          if (strlen(conf.zoneName[webZone]) != valueLen) resp = 1;
          else resp = strncmp(conf.zoneName[webZone], valueP, LWIP_MIN(valueLen, NAME_LENGTH - 1));
        } else resp = 0;
        // Replace name
        strncpy(conf.zoneName[webZone], valueP, LWIP_MIN(valueLen, NAME_LENGTH - 1));
        conf.zoneName[webZone][LWIP_MIN(valueLen, NAME_LENGTH - 1)] = 0;
        // MQTT
        if (resp) {
          pushToMqtt(typeZone, webZone, functionName);
          // HAD
          if (GET_CONF_ZONE_MQTT_HAD(conf.zone[webZone])) {
            pushToMqttHAD(typeZone, webZone, functionHAD, 1);
          }
        }
      break;
      case 'D': // delay
        SET_CONF_ZONE_AUTH_TIME(conf.zone[webZone], (valueP[0] - 48));
      break;
      case '0' ... '9': // Handle all single radio buttons for settings
        if (valueP[0] == '0') conf.zone[webZone] &= ~(1 << (name[0]-48));
        else                  conf.zone[webZone] |=  (1 << (name[0]-48));
      break;
      case 'a' ... 'd': // Handle all single radio buttons for settings 10 -> 13
        resp = GET_CONF_ZONE_MQTT_HAD(conf.zone[webZone]);
        if (valueP[0] == '0') conf.zone[webZone] &= ~(1 << (name[0]-87)); // a(97) - 10
        else                  conf.zone[webZone] |=  (1 << (name[0]-87));
        // Handle HAD change
        if (GET_CONF_ZONE_ENABLED(conf.zone[webZone]) &&
            (resp != GET_CONF_ZONE_MQTT_HAD(conf.zone[webZone]))) {
          pushToMqttHAD(typeZone, webZone, functionHAD, GET_CONF_ZONE_MQTT_HAD(conf.zone[webZone]));
        }
      break;
      case 'g': // group
        number = strtol(valueP, NULL, 10);
        SET_CONF_ZONE_GROUP(conf.zone[webZone], number);
      break;
      case 'e': // save
        writeToBkpSRAM((uint8_t*)&conf, sizeof(config_t), 0);
      break;
    }
  } while (repeat);
}


#endif /* HTTPD_HANDLER_ZONE_H_ */
