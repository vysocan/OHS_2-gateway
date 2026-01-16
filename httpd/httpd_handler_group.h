/*
 * httpd_handler_group.h
 *
 *  Created on: 16. 1. 2026
 *      Author: vysocan
 */

#ifndef HTTPD_HANDLER_GROUP_H_
#define HTTPD_HANDLER_GROUP_H_


/*
 * @brief HTTP group page handler
 * @param chp Pointer to the output stream
 */
static void fs_open_custom_group(BaseSequentialStream *chp) {
  uint16_t logAddress;
  chprintf(chp, "%s#", html_tr_th);
  chprintf(chp, "%s%s", html_e_th_th, text_Name);
  chprintf(chp, "%s%s", html_e_th_th, text_On);
  chprintf(chp, "%s%s %s", html_e_th_th, text_Auto, text_arm);
  chprintf(chp, "%s%s %s", html_e_th_th, text_Arm, text_chain);
  chprintf(chp, "%s%s %s", html_e_th_th, text_Disarm, text_chain);
  chprintf(chp, "%s%ss", html_e_th_th, text_Zone);
  chprintf(chp, "%s%ss", html_e_th_th, text_Authentication);
  chprintf(chp, "%s%ss", html_e_th_th, text_Sensor);
  chprintf(chp, "%s%ss", html_e_th_th, text_Contact);
  chprintf(chp, "%s%s", html_e_th_th, text_Siren);
  chprintf(chp, "%s%s", html_e_th_th, text_MQTT);
  chprintf(chp, "%s%s", html_e_th_th, text_HAD);
  chprintf(chp, "%s%s", html_e_th_th, text_Armed);
  chprintf(chp, "%s%s%s\r\n", html_e_th_th, text_Status, html_e_th_e_tr);
  // Information table
  for (uint8_t i = 0; i < ALARM_GROUPS; i++) {
    chprintf(chp, "%s%u.%s", html_tr_td, i + 1, html_e_td_td);
    chprintf(chp, "%s%s", conf.group[i].name, html_e_td_td);
    printOkNok(chp, GET_CONF_GROUP_ENABLED(conf.group[i].setting));
    chprintf(chp, "%s", html_e_td_td);
    if (GET_CONF_GROUP_ENABLED(conf.group[i].setting))
      printOkNok(chp, GET_CONF_GROUP_AUTO_ARM(conf.group[i].setting));
    chprintf(chp, "%s", html_e_td_td);
    if (GET_CONF_GROUP_ENABLED(conf.group[i].setting))
      printGroup(chp, GET_CONF_GROUP_ARM_CHAIN(conf.group[i].setting));
    chprintf(chp, "%s", html_e_td_td);
    if (GET_CONF_GROUP_ENABLED(conf.group[i].setting))
      printGroup(chp, GET_CONF_GROUP_DISARM_CHAIN(conf.group[i].setting));
    chprintf(chp, "%s", html_e_td_td);
    if (GET_CONF_GROUP_ENABLED(conf.group[i].setting)) {
      logAddress = 0; // Just temp. var.
      for (uint8_t j = 0; j < ALARM_ZONES; j++) {
        if ((GET_CONF_ZONE_ENABLED(conf.zone[j])) &&
            (GET_CONF_ZONE_IS_PRESENT(conf.zone[j])) &&
            (GET_CONF_ZONE_GROUP(conf.zone[j]) == i)) {
          if (logAddress > 0) chprintf(chp, "%s", html_br);
          chprintf(chp, "%u. %s", j+1, conf.zoneName[j]);
          logAddress++;
        }
      }
    }
    chprintf(chp, "%s", html_e_td_td);
    if (GET_CONF_GROUP_ENABLED(conf.group[i].setting)) {
      logAddress = 0; // Just temp. var.
      for (uint8_t j = 0; j < NODE_SIZE; j++) {
        if (GET_NODE_ENABLED(node[j].setting) &&
            (GET_NODE_GROUP(node[j].setting) == i) &&
            (node[j].type == 'K')) {
          if (logAddress > 0) chprintf(chp, "%s", html_br);
          chprintf(chp, "%u. %s", j+1, node[j].name);
          logAddress++;
        }
      }
    }
    chprintf(chp, "%s", html_e_td_td);
    if (GET_CONF_GROUP_ENABLED(conf.group[i].setting)) {
      logAddress = 0; // Just temp. var.
      for (uint8_t j = 0; j < NODE_SIZE; j++) {
        if (GET_NODE_ENABLED(node[j].setting) &&
            (GET_NODE_GROUP(node[j].setting) == i) &&
            (node[j].type == 'S')) {
          if (logAddress > 0) chprintf(chp, "%s", html_br);
          chprintf(chp, "%u. %s", j+1, node[j].name);
          logAddress++;
        }
      }
    }
    chprintf(chp, "%s", html_e_td_td);
    if (GET_CONF_GROUP_ENABLED(conf.group[i].setting)) {
      logAddress = 0; // Just temp. var.
      for (uint8_t j = 0; j < CONTACTS_SIZE; j++) {
        if (GET_CONF_CONTACT_ENABLED(conf.contact[j].setting) &&
            ((GET_CONF_CONTACT_GROUP(conf.contact[j].setting) == i) ||
             (GET_CONF_CONTACT_IS_GLOBAL(conf.contact[j].setting)))){
          if (logAddress > 0) chprintf(chp, "%s", html_br);
          chprintf(chp, "%s", conf.contact[j].name);
          logAddress++;
        }
      }
    }
    chprintf(chp, "%s", html_e_td_td); // show relays
    if (GET_CONF_GROUP_ENABLED(conf.group[i].setting)) {
      chprintf(chp, "%s:", text_Alarm);
      if (GET_CONF_GROUP_PIR1(conf.group[i].setting)) chprintf(chp, " %s 1", text_relay);
      if (GET_CONF_GROUP_PIR2(conf.group[i].setting)) chprintf(chp, " %s 2", text_relay);
      chprintf(chp, "%s%s:", html_br, text_Tamper);
      if (GET_CONF_GROUP_TAMPER1(conf.group[i].setting)) chprintf(chp, " %s 1", text_relay);
      if (GET_CONF_GROUP_TAMPER2(conf.group[i].setting)) chprintf(chp, " %s 2", text_relay);
      // Remote Siren/Horn
      for (uint8_t j=0; j < NODE_SIZE; j++) {
        if ((node[j].type == 'H') && (GET_NODE_GROUP(node[j].setting) == i)){
          chprintf(chp, "%s", html_br);
          printNodeAddress(chp, node[j].address, node[j].type, node[j].function, node[j].number, 1);
        }
      }
    }
    chprintf(chp, "%s", html_e_td_td);
    if (GET_CONF_GROUP_ENABLED(conf.group[i].setting))
      printOkNok(chp, GET_CONF_GROUP_MQTT(conf.group[i].setting));
    chprintf(chp, "%s", html_e_td_td);
    if (GET_CONF_GROUP_ENABLED(conf.group[i].setting))
      printOkNok(chp, GET_CONF_GROUP_MQTT_HAD(conf.group[i].setting));
    chprintf(chp, "%s", html_e_td_td);
    if (GET_CONF_GROUP_ENABLED(conf.group[i].setting)) {
      if (GET_GROUP_ARMED(group[i].setting)) {
        if GET_GROUP_ARMED_HOME(group[i].setting) { chprintf(chp, "%s", html_i_home); }
        else                                      { chprintf(chp, "%s", html_i_OK); }
      } else {
        if (group[i].armDelay > 0) { chprintf(chp, "%s", html_i_starting); }
        else                       { chprintf(chp, "%s", html_i_disabled); }
      }
    }
    chprintf(chp, "%s", html_e_td_td);
    if (GET_CONF_GROUP_ENABLED(conf.group[i].setting)) {
      if (GET_GROUP_ALARM(group[i].setting) == 0) {
        if (GET_GROUP_WAIT_AUTH(group[i].setting)) { chprintf(chp, "%s", html_i_starting); }
        else                                       { chprintf(chp, "%s", html_i_OK); }
      } else { chprintf(chp, "%s", html_i_alarm); }
    }
    chprintf(chp, "%s", html_e_td_e_tr);
  } // end for ...
  chprintf(chp, "%s%s", html_e_table, html_table);
  chprintf(chp, "%s%s%s", html_tr_td, text_Group, html_e_td_td);
  chprintf(chp, "%sP%s", html_select_submit, html_e_tag);
  for (uint8_t i = 0; i < ALARM_GROUPS; i++) {
    chprintf(chp, "%s%u", html_option, i);
    if (webGroup == i) { chprintf(chp, "%s", html_selected); }
    else               { chprintf(chp, "%s", html_e_tag); }
    chprintf(chp, "%u. %s%s", i + 1, conf.group[i].name, html_e_option);
  }
  chprintf(chp, "%s%s", html_e_select, html_e_td_e_tr_tr_td);
  chprintf(chp, "%s%s", text_Name, html_e_td_td);
  printTextInput(chp, 'n', conf.group[webGroup].name, NAME_LENGTH);
  chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_Group, text_is, html_e_td_td);
  printOnOffButton(chp, "0", GET_CONF_GROUP_ENABLED(conf.group[webGroup].setting));
  chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_Auto, text_arm, html_e_td_td);
  printOnOffButton(chp, "5", GET_CONF_GROUP_AUTO_ARM(conf.group[webGroup].setting));
  chprintf(chp, "%s%s %ss %s 1%s", html_e_td_e_tr_tr_td,  text_Alarm, text_trigger, text_relay, html_e_td_td);
  printOnOffButton(chp, "4", GET_CONF_GROUP_PIR1(conf.group[webGroup].setting));
  chprintf(chp, "%s%s %ss %s 2%s", html_e_td_e_tr_tr_td,  text_Alarm, text_trigger, text_relay, html_e_td_td);
  printOnOffButton(chp, "3", GET_CONF_GROUP_PIR2(conf.group[webGroup].setting));
  chprintf(chp, "%s%s %ss %s 1%s", html_e_td_e_tr_tr_td,  text_Tamper, text_trigger, text_relay, html_e_td_td);
  printOnOffButton(chp, "2", GET_CONF_GROUP_TAMPER1(conf.group[webGroup].setting));
  chprintf(chp, "%s%s %ss %s 2%s", html_e_td_e_tr_tr_td,  text_Tamper, text_trigger, text_relay, html_e_td_td);
  printOnOffButton(chp, "1", GET_CONF_GROUP_TAMPER2(conf.group[webGroup].setting));
  chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td,  text_MQTT, text_publish, html_e_td_td);
  printOnOffButton(chp, "7", GET_CONF_GROUP_MQTT(conf.group[webGroup].setting));
  chprintf(chp, "%s%s %s %s%s", html_e_td_e_tr_tr_td,  text_MQTT, text_HA, text_Discovery, html_e_td_td);
  printOnOffButton(chp, "6", GET_CONF_GROUP_MQTT_HAD(conf.group[webGroup].setting));
  chprintf(chp, "%s%s %s %s%s", html_e_td_e_tr_tr_td, text_Arm, text_group, text_chain, html_e_td_td);
  selectGroup(chp, GET_CONF_GROUP_ARM_CHAIN(conf.group[webGroup].setting), 'a');
  chprintf(chp, "%s%s %s %s%s", html_e_td_e_tr_tr_td, text_Disarm, text_group, text_chain, html_e_td_td);
  selectGroup(chp, GET_CONF_GROUP_DISARM_CHAIN(conf.group[webGroup].setting), 'd');
  chprintf(chp, "%s%s", html_e_td_e_tr, html_e_table);
  // Buttons
  chprintf(chp, "%s%s%s", html_Apply, html_Save, html_Disarm);
}


#endif /* HTTPD_HANDLER_GROUP_H_ */
