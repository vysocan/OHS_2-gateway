/*
 * httpd_handler_trigger.h
 *
 *  Created on: 16. 1. 2026
 *      Author: vysocan
 */

#ifndef HTTPD_HANDLER_TRIGGER_H_
#define HTTPD_HANDLER_TRIGGER_H_


static void fs_open_custom_trigger(BaseSequentialStream *chp) {
  uint16_t logAddress;
  uint8_t number;
  chprintf(chp, "%s#", html_tr_th);
  chprintf(chp, "%s%s", html_e_th_th, text_Name);
  chprintf(chp, "%s%s", html_e_th_th, text_On);
  chprintf(chp, "%s%s", html_e_th_th, text_Type);
  chprintf(chp, "%s%s", html_e_th_th, text_Condition);
  chprintf(chp, "%s%s", html_e_th_th, text_Value);
  chprintf(chp, "%s%s", html_e_th_th, text_Hysteresis);
  chprintf(chp, "%s%s", html_e_th_th, text_Script);
  chprintf(chp, "%s%s", html_e_th_th, text_Alert);
  chprintf(chp, "%s%s", html_e_th_th, text_Pass);
  chprintf(chp, "%s%s %s", html_e_th_th, text_Pass, text_Off);
  chprintf(chp, "%s%s", html_e_th_th, text_To);
  chprintf(chp, "%s%s", html_e_th_th, text_On);
  chprintf(chp, "%s%s", html_e_th_th, text_Off);
  chprintf(chp, "%s%s%s\r\n", html_e_th_th, text_Status, html_e_th_e_tr);
  // Information table
  for (uint8_t i = 0; i < TRIGGER_SIZE; i++) {
    chprintf(chp, "%s%u.%s", html_tr_td, i + 1, html_e_td_td);
    chprintf(chp, "%s%s", conf.trigger[i].name, html_e_td_td);
    printOkNok(chp, GET_CONF_TRIGGER_ENABLED(conf.trigger[i].setting));
    chprintf(chp, "%s", html_e_td_td);
    if (GET_CONF_TRIGGER_ENABLED(conf.trigger[i].setting)) {
      switch (conf.trigger[i].type) {
        case 'G': chprintf(chp, "%s - ", text_Group);
          printGroup(chp, conf.trigger[i].number);
          break;
        case 'Z': chprintf(chp, "%s - ", text_Zone);
          printZone(chp, conf.trigger[i].number);
          break;
        default: printNodeAddress(chp, conf.trigger[i].address, 'S', conf.trigger[i].function,
                                  conf.trigger[i].number, true);
          break;
      }
    }
    chprintf(chp, "%s", html_e_td_td);
    if (GET_CONF_TRIGGER_ENABLED(conf.trigger[i].setting))
      chprintf(chp, "%s", triggerCondition[conf.trigger[i].condition]);
    chprintf(chp, "%s", html_e_td_td);
    if (GET_CONF_TRIGGER_ENABLED(conf.trigger[i].setting)) {
      switch (conf.trigger[i].type) {
        case 'G': chprintf(chp, "%s", groupState[(uint8_t)conf.trigger[i].value]);
          break;
        case 'Z': chprintf(chp, "%s", zoneState[(uint8_t)conf.trigger[i].value]);
          break;
        default: chprintf(chp, "%.2f", conf.trigger[i].value);
          break;
      }
    }
    chprintf(chp, "%s", html_e_td_td);
    if (GET_CONF_TRIGGER_ENABLED(conf.trigger[i].setting)) {
      if (conf.trigger[i].type == 'S') chprintf(chp, "%.2f", conf.trigger[i].hysteresis);
    }
    chprintf(chp, "%s", html_e_td_td);
    if (GET_CONF_TRIGGER_ENABLED(conf.trigger[i].setting)) {
      if (conf.trigger[i].evalScript[0]) chprintf(chp, "%s", conf.trigger[i].evalScript);
      else chprintf(chp, "%s", NOT_SET);
    }
    chprintf(chp, "%s", html_e_td_td);
    if (GET_CONF_TRIGGER_ENABLED(conf.trigger[i].setting))
      printOkNok(chp, GET_CONF_TRIGGER_ALERT(conf.trigger[i].setting));
    chprintf(chp, "%s", html_e_td_td);

    if (GET_CONF_TRIGGER_ENABLED(conf.trigger[i].setting)) {
      if (GET_CONF_TRIGGER_PASS(conf.trigger[i].setting)) {
        if (GET_CONF_TRIGGER_PASS_VALUE(conf.trigger[i].setting)) chprintf(chp, "%s", text_value);
        else chprintf(chp, "%s", text_constant);
        // once
        if (GET_CONF_TRIGGER_PASS(conf.trigger[i].setting) > 1) {
          chprintf(chp, " %s", triggerPassType[GET_CONF_TRIGGER_PASS(conf.trigger[i].setting)]);
        }
      }
    }
    chprintf(chp, "%s", html_e_td_td);
    // Pass off
    if (GET_CONF_TRIGGER_ENABLED(conf.trigger[i].setting)) {
      if (GET_CONF_TRIGGER_PASS(conf.trigger[i].setting)) {
        if (GET_CONF_TRIGGER_PASS_OFF(conf.trigger[i].setting) < 2) {
          chprintf(chp, "%s", triggerPassOffType[GET_CONF_TRIGGER_PASS_OFF(conf.trigger[i].setting)]);
        } else {
          chprintf(chp, "%s %u %s", text_after, conf.trigger[i].offTime,
          durationSelect[GET_CONF_TRIGGER_OFF_PERIOD(conf.trigger[i].setting)]);
        }
      }
    }
    chprintf(chp, "%s", html_e_td_td);
    if (GET_CONF_TRIGGER_ENABLED(conf.trigger[i].setting)) {
      printNodeAddress(chp, conf.trigger[i].toAddress, 'I',  conf.trigger[i].toFunction,
                       conf.trigger[i].toNumber, true);
    }
    chprintf(chp, "%s", html_e_td_td);
    chprintf(chp, "%.2f%s", conf.trigger[i].constantOn, html_e_td_td);
    chprintf(chp, "%.2f%s", conf.trigger[i].constantOff, html_e_td_td);
    printOkNok(chp, GET_CONF_TRIGGER_TRIGGERED(conf.trigger[i].setting));
    chprintf(chp, "%s", html_e_td_e_tr);
  }
  chprintf(chp, "%s%s", html_e_table, html_table);
  // Trigger options
  chprintf(chp, "%s%s%s", html_tr_td, text_Trigger, html_e_td_td);
  chprintf(chp, "%sP%s", html_select_submit, html_e_tag);
  for (uint8_t i = 0; i < TRIGGER_SIZE; i++) {
    chprintf(chp, "%s%u", html_option, i);
    if (webTrigger == i) { chprintf(chp, "%s", html_selected); }
    else                 { chprintf(chp, "%s", html_e_tag); }
    chprintf(chp, "%u. %s%s", i + 1, conf.trigger[i].name, html_e_option);
  }
  chprintf(chp, "%s%s", html_e_select, html_e_td_e_tr_tr_td);
  chprintf(chp, "%s%s", text_Name, html_e_td_td);
  printTextInput(chp, 'n', conf.trigger[webTrigger].name, NAME_LENGTH);
  chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_Trigger, text_is, html_e_td_td);
  printOnOffButton(chp, "B", GET_CONF_TRIGGER_ENABLED(conf.trigger[webTrigger].setting));
  chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, text_Type, html_e_td_td);
  chprintf(chp, "%sy%sy' onchange='sd(this)%s", html_select, html_id_tag, html_e_tag);
  logAddress = 0; number = 0;
  for (uint8_t i = 0; i < ALARM_GROUPS; i++) {
    if (GET_CONF_GROUP_ENABLED(conf.group[i].setting)) {
      chprintf(chp, "%s%u", html_option, logAddress);
      if ((conf.trigger[webTrigger].type == 'G') && (conf.trigger[webTrigger].number == i)) {
        chprintf(chp, "%s", html_selected);
        number = 1;
      } else { chprintf(chp, "%s", html_e_tag); }
      chprintf(chp, "%s - ", text_Group);
      printGroup(chp, i);
      chprintf(chp, "%s", html_e_option);
    }
    logAddress++;
  }
  for (uint8_t i = 0; i < ALARM_ZONES; i++) {
    if (GET_CONF_ZONE_ENABLED(conf.zone[i])) {
      chprintf(chp, "%s%u", html_option, logAddress);
      if ((conf.trigger[webTrigger].type == 'Z') && (conf.trigger[webTrigger].number == i)) {
        chprintf(chp, "%s", html_selected);
        number = 1;
      } else { chprintf(chp, "%s", html_e_tag); }
      chprintf(chp, "%s - ", text_Zone);
      printZone(chp, i);
      chprintf(chp, "%s", html_e_option);
    }
    logAddress++;
  }
  for (uint8_t i = 0; i < NODE_SIZE; i++) {
    if (node[i].type == 'S') {
      chprintf(chp, "%s%u", html_option, logAddress);
      if ((conf.trigger[webTrigger].type == node[i].type) &&
          (conf.trigger[webTrigger].number == node[i].number) &&
          (conf.trigger[webTrigger].function == node[i].function) &&
          (conf.trigger[webTrigger].address == node[i].address)) {
        chprintf(chp, "%s", html_selected);
        number = 1;
      } else { chprintf(chp, "%s", html_e_tag); }
      chprintf(chp, "%s - ", text_Sensor);
      printNodeAddress(chp, node[i].address, node[i].type, node[i].function, node[i].number, true);
      chprintf(chp, "%s", html_e_option);
    }
    logAddress++;
  }
  chprintf(chp, "%s%u", html_option, DUMMY_NO_VALUE);
  if (!number) chprintf(chp, "%s", html_selected);
  else         chprintf(chp, "%s", html_e_tag);
  if ((conf.trigger[webTrigger].type == 'S') && (!number)) chprintf(chp, "%s", text_not_found);
  else                                      chprintf(chp, "%s", NOT_SET);
  chprintf(chp, "%s%s", html_e_option, html_e_select);
  // Condition
  chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, text_Condition, html_e_td_td);
  chprintf(chp, "%sc%sc%s", html_select, html_id_tag, html_e_tag);
  for (uint8_t i = 0; i < ARRAY_SIZE(triggerCondition); i++) {
     chprintf(chp, "%s%u", html_option, i);
     if (conf.trigger[webTrigger].condition == i) { chprintf(chp, "%s", html_selected); }
     else                                         { chprintf(chp, "%s", html_e_tag); }
     chprintf(chp, "%s%s", triggerCondition[i], html_e_option);
   }
  chprintf(chp, "%s", html_e_select);
  // Value
  chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, text_Value, html_e_td_td);
  chprintf(chp, "%s1%s", html_div_id_1, html_div_id_2);
  chprintf(chp, "%sg%sg%s", html_select, html_id_tag, html_e_tag);
  for (uint8_t i = 0; i < ARRAY_SIZE(groupState); i++) {
    chprintf(chp, "%s%u", html_option, i);
    if ((uint8_t)conf.trigger[webTrigger].value == i) { chprintf(chp, "%s", html_selected); }
    else                                              { chprintf(chp, "%s", html_e_tag); }
    chprintf(chp, "%s%s", groupState[i], html_e_option);
  }
  chprintf(chp, "%s", html_e_select);
  chprintf(chp, "%s%s2%s", html_div_e, html_div_id_1, html_div_id_2);
  chprintf(chp, "%sz%sz%s", html_select, html_id_tag, html_e_tag);
  for (uint8_t i = 0; i < ARRAY_SIZE(zoneState); i++) {
    chprintf(chp, "%s%u", html_option, i);
    if ((uint8_t)conf.trigger[webTrigger].value == i) { chprintf(chp, "%s", html_selected); }
    else                                              { chprintf(chp, "%s", html_e_tag); }
    chprintf(chp, "%s%s", zoneState[i], html_e_option);
  }
  chprintf(chp, "%s", html_e_select);
  chprintf(chp, "%s%s3%s", html_div_e, html_div_id_1, html_div_id_2);
  printFloatInput(chp, 'v', conf.trigger[webTrigger].value);
  chprintf(chp, "%s", html_div_e);
  chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, text_Hysteresis, html_e_td_td);
  printFloatInput(chp, 'h', conf.trigger[webTrigger].hysteresis);

  chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_Evaluate, text_script, html_e_td_td);
  chprintf(chp, "%sp%s", html_select, html_e_tag);
  logAddress = 1;
  for (scriptp = scriptLL; scriptp != NULL; scriptp = scriptp->next) {
    chprintf(chp, "%s%s", html_option, scriptp->name);
    if (strcmp(&conf.trigger[webTrigger].evalScript[0], scriptp->name) == 0) {
      chprintf(chp, "%s", html_selected);
      logAddress = 0;
    } else {
      chprintf(chp, "%s", html_e_tag);
    }
    chprintf(chp, "%s%s", scriptp->name, html_e_option);
  }
  chprintf(chp, "%s", html_option);
  if (logAddress) { chprintf(chp, "%s", html_selected); }
  else            { chprintf(chp, "%s", html_e_tag); }
  chprintf(chp, "%s%s%s", NOT_SET, html_e_option, html_e_select);

  chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, text_Alert, html_e_td_td);
  printOnOffButton(chp, "H", GET_CONF_TRIGGER_ALERT(conf.trigger[webTrigger].setting));
  chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, text_Pass, html_e_td_td);
  printTwoButton(chp, "C", GET_CONF_TRIGGER_PASS_VALUE(conf.trigger[webTrigger].setting),
                 0, 0b00, text_constant, text_value);
  chprintf(chp, "%s", html_br);
  printThreeButton(chp, "s", GET_CONF_TRIGGER_PASS(conf.trigger[webTrigger].setting),
                   1, 0b110, triggerPassType[0], triggerPassType[1], triggerPassType[2], 0);
  chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_Pass, text_Off,html_e_td_td);
  printThreeButton(chp, "S", GET_CONF_TRIGGER_PASS_OFF(conf.trigger[webTrigger].setting),
                   2, 0b100, triggerPassOffType[0], triggerPassOffType[1], triggerPassOffType[2], 0);
  chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_Off, text_timer, html_e_td_td);
  printIntInput(chp, 't', conf.trigger[webTrigger].offTime, 3, 1, 255);
  printDurationSelect(chp, 'T', GET_CONF_TRIGGER_OFF_PERIOD(conf.trigger[webTrigger].setting));
  chprintf(chp, "%s%s%s", html_tr_td, text_Address, html_e_td_td);
  chprintf(chp, "%sa%sa%s", html_select, html_id_tag, html_e_tag);
  for (uint8_t i = 0; i <= NODE_SIZE; i++) {
    if (i < NODE_SIZE) {
      if (node[i].type == 'I') {
        chprintf(chp, "%s%u", html_option, i);
        if ((node[i].address  == conf.trigger[webTrigger].toAddress) &&
            (node[i].function == conf.trigger[webTrigger].toFunction) &&
            (node[i].number   == conf.trigger[webTrigger].toNumber))
             { chprintf(chp, "%s", html_selected); }
        else { chprintf(chp, "%s", html_e_tag); }
        printNodeAddress(chp, node[i].address, node[i].type, node[i].function, node[i].number, true);
        chprintf(chp, "%s", html_e_option);
      }
    } else {
      chprintf(chp, "%s%u", html_option, DUMMY_NO_VALUE);
      if ((conf.trigger[webTrigger].toAddress  == 0) &&
          (conf.trigger[webTrigger].toFunction == ' ') &&
          (conf.trigger[webTrigger].toNumber   == 0))
           { chprintf(chp, "%s", html_selected); }
      else { chprintf(chp, "%s", html_e_tag); }
      chprintf(chp, "%s%s", NOT_SET, html_e_option);
    }
  }
  chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_On, text_value, html_e_td_td);
  printFloatInput(chp, 'o', conf.trigger[webTrigger].constantOn);
  chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_Off, text_value, html_e_td_td);
  printFloatInput(chp, 'f', conf.trigger[webTrigger].constantOff);
  chprintf(chp, "%s%s", html_e_td_e_tr, html_e_table);
  // JavaScript
  chprintf(chp, "%s%s%s", html_script, JSTrigger, html_e_script);
  chprintf(chp, "%s%s%u", html_script, JSTriggerSel_1, ALARM_GROUPS);
  chprintf(chp, "%s%u%s%u", JSTriggerSel_2, ALARM_GROUPS, JSTriggerSel_3, ALARM_GROUPS + ALARM_ZONES);
  chprintf(chp, "%s%s", JSTriggerSel_4, html_e_script);
  // Buttons
  chprintf(chp, "%s%s", html_Apply, html_Save);
}


#endif /* HTTPD_HANDLER_TRIGGER_H_ */
