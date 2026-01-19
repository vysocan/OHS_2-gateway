/*
 * httpd_handler_trigger.h
 *
 *  Created on: 16. 1. 2026
 *      Author: vysocan
 */

#ifndef HTTPD_HANDLER_TRIGGER_H_
#define HTTPD_HANDLER_TRIGGER_H_


/*
 * @brief HTTP trigger page handler
 * @param chp Pointer to the output stream
 */
static void fs_open_custom_trigger(BaseSequentialStream *chp) {
  uint16_t logAddress;
  uint8_t number;
  chprintf(chp, "%s#", HTML_tr_th);
  chprintf(chp, "%s%s", HTML_e_th_th, TEXT_Name);
  chprintf(chp, "%s%s", HTML_e_th_th, TEXT_On);
  chprintf(chp, "%s%s", HTML_e_th_th, TEXT_Type);
  chprintf(chp, "%s%s", HTML_e_th_th, TEXT_Condition);
  chprintf(chp, "%s%s", HTML_e_th_th, TEXT_Value);
  chprintf(chp, "%s%s", HTML_e_th_th, TEXT_Hysteresis);
  chprintf(chp, "%s%s", HTML_e_th_th, TEXT_Script);
  chprintf(chp, "%s%s", HTML_e_th_th, TEXT_Alert);
  chprintf(chp, "%s%s", HTML_e_th_th, TEXT_Pass);
  chprintf(chp, "%s%s %s", HTML_e_th_th, TEXT_Pass, TEXT_Off);
  chprintf(chp, "%s%s", HTML_e_th_th, TEXT_To);
  chprintf(chp, "%s%s", HTML_e_th_th, TEXT_On);
  chprintf(chp, "%s%s", HTML_e_th_th, TEXT_Off);
  chprintf(chp, "%s%s%s\r\n", HTML_e_th_th, TEXT_Status, HTML_e_th_e_tr);
  // Information table
  for (uint8_t i = 0; i < TRIGGER_SIZE; i++) {
    chprintf(chp, "%s%u.%s", HTML_tr_td, i + 1, HTML_e_td_td);
    chprintf(chp, "%s%s", conf.trigger[i].name, HTML_e_td_td);
    printOkNok(chp, GET_CONF_TRIGGER_ENABLED(conf.trigger[i].setting));
    chprintf(chp, "%s", HTML_e_td_td);
    if (GET_CONF_TRIGGER_ENABLED(conf.trigger[i].setting)) {
      switch (conf.trigger[i].type) {
        case 'G': chprintf(chp, "%s - ", TEXT_Group);
          printGroup(chp, conf.trigger[i].number);
          break;
        case 'Z': chprintf(chp, "%s - ", TEXT_Zone);
          printZone(chp, conf.trigger[i].number);
          break;
        default: printNodeAddress(chp, conf.trigger[i].address, 'S', conf.trigger[i].function,
                                  conf.trigger[i].number, true);
          break;
      }
    }
    chprintf(chp, "%s", HTML_e_td_td);
    if (GET_CONF_TRIGGER_ENABLED(conf.trigger[i].setting))
      chprintf(chp, "%s", triggerCondition[conf.trigger[i].condition]);
    chprintf(chp, "%s", HTML_e_td_td);
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
    chprintf(chp, "%s", HTML_e_td_td);
    if (GET_CONF_TRIGGER_ENABLED(conf.trigger[i].setting)) {
      if (conf.trigger[i].type == 'S') chprintf(chp, "%.2f", conf.trigger[i].hysteresis);
    }
    chprintf(chp, "%s", HTML_e_td_td);
    if (GET_CONF_TRIGGER_ENABLED(conf.trigger[i].setting)) {
      if (conf.trigger[i].evalScript[0]) chprintf(chp, "%s", conf.trigger[i].evalScript);
      else chprintf(chp, "%s", NOT_SET);
    }
    chprintf(chp, "%s", HTML_e_td_td);
    if (GET_CONF_TRIGGER_ENABLED(conf.trigger[i].setting))
      printOkNok(chp, GET_CONF_TRIGGER_ALERT(conf.trigger[i].setting));
    chprintf(chp, "%s", HTML_e_td_td);

    if (GET_CONF_TRIGGER_ENABLED(conf.trigger[i].setting)) {
      if (GET_CONF_TRIGGER_PASS(conf.trigger[i].setting)) {
        if (GET_CONF_TRIGGER_PASS_VALUE(conf.trigger[i].setting)) chprintf(chp, "%s", TEXT_value);
        else chprintf(chp, "%s", TEXT_constant);
        // once
        if (GET_CONF_TRIGGER_PASS(conf.trigger[i].setting) > 1) {
          chprintf(chp, " %s", triggerPassType[GET_CONF_TRIGGER_PASS(conf.trigger[i].setting)]);
        }
      }
    }
    chprintf(chp, "%s", HTML_e_td_td);
    // Pass off
    if (GET_CONF_TRIGGER_ENABLED(conf.trigger[i].setting)) {
      if (GET_CONF_TRIGGER_PASS(conf.trigger[i].setting)) {
        if (GET_CONF_TRIGGER_PASS_OFF(conf.trigger[i].setting) < 2) {
          chprintf(chp, "%s", triggerPassOffType[GET_CONF_TRIGGER_PASS_OFF(conf.trigger[i].setting)]);
        } else {
          chprintf(chp, "%s %u %s", TEXT_after, conf.trigger[i].offTime,
          durationSelect[GET_CONF_TRIGGER_OFF_PERIOD(conf.trigger[i].setting)]);
        }
      }
    }
    chprintf(chp, "%s", HTML_e_td_td);
    if (GET_CONF_TRIGGER_ENABLED(conf.trigger[i].setting)) {
      printNodeAddress(chp, conf.trigger[i].toAddress, 'I',  conf.trigger[i].toFunction,
                       conf.trigger[i].toNumber, true);
    }
    chprintf(chp, "%s", HTML_e_td_td);
    chprintf(chp, "%.2f%s", conf.trigger[i].constantOn, HTML_e_td_td);
    chprintf(chp, "%.2f%s", conf.trigger[i].constantOff, HTML_e_td_td);
    printOkNok(chp, GET_CONF_TRIGGER_TRIGGERED(conf.trigger[i].setting));
    chprintf(chp, "%s", HTML_e_td_e_tr);
  }
  chprintf(chp, "%s%s", HTML_e_table, HTML_table);
  // Trigger options
  chprintf(chp, "%s%s%s", HTML_tr_td, TEXT_Trigger, HTML_e_td_td);
  chprintf(chp, "%sP%s", HTML_select_submit, HTML_e_tag);
  for (uint8_t i = 0; i < TRIGGER_SIZE; i++) {
    chprintf(chp, "%s%u", HTML_option, i);
    if (webTrigger == i) { chprintf(chp, "%s", HTML_selected); }
    else                 { chprintf(chp, "%s", HTML_e_tag); }
    chprintf(chp, "%u. %s%s", i + 1, conf.trigger[i].name, HTML_e_option);
  }
  chprintf(chp, "%s%s", HTML_e_select, HTML_e_td_e_tr_tr_td);
  chprintf(chp, "%s%s", TEXT_Name, HTML_e_td_td);
  printTextInput(chp, 'n', conf.trigger[webTrigger].name, NAME_LENGTH);
  chprintf(chp, "%s%s %s%s", HTML_e_td_e_tr_tr_td, TEXT_Trigger, TEXT_is, HTML_e_td_td);
  printOnOffButton(chp, "B", GET_CONF_TRIGGER_ENABLED(conf.trigger[webTrigger].setting));
  chprintf(chp, "%s%s%s", HTML_e_td_e_tr_tr_td, TEXT_Type, HTML_e_td_td);
  chprintf(chp, "%sy%sy' onchange='sd(this)%s", HTML_select, HTML_id_tag, HTML_e_tag);
  logAddress = 0; number = 0;
  for (uint8_t i = 0; i < ALARM_GROUPS; i++) {
    if (GET_CONF_GROUP_ENABLED(conf.group[i].setting)) {
      chprintf(chp, "%s%u", HTML_option, logAddress);
      if ((conf.trigger[webTrigger].type == 'G') && (conf.trigger[webTrigger].number == i)) {
        chprintf(chp, "%s", HTML_selected);
        number = 1;
      } else { chprintf(chp, "%s", HTML_e_tag); }
      chprintf(chp, "%s - ", TEXT_Group);
      printGroup(chp, i);
      chprintf(chp, "%s", HTML_e_option);
    }
    logAddress++;
  }
  for (uint8_t i = 0; i < ALARM_ZONES; i++) {
    if (GET_CONF_ZONE_ENABLED(conf.zone[i])) {
      chprintf(chp, "%s%u", HTML_option, logAddress);
      if ((conf.trigger[webTrigger].type == 'Z') && (conf.trigger[webTrigger].number == i)) {
        chprintf(chp, "%s", HTML_selected);
        number = 1;
      } else { chprintf(chp, "%s", HTML_e_tag); }
      chprintf(chp, "%s - ", TEXT_Zone);
      printZone(chp, i);
      chprintf(chp, "%s", HTML_e_option);
    }
    logAddress++;
  }
  for (uint8_t i = 0; i < NODE_SIZE; i++) {
    if (node[i].type == 'S') {
      chprintf(chp, "%s%u", HTML_option, logAddress);
      if ((conf.trigger[webTrigger].type == node[i].type) &&
          (conf.trigger[webTrigger].number == node[i].number) &&
          (conf.trigger[webTrigger].function == node[i].function) &&
          (conf.trigger[webTrigger].address == node[i].address)) {
        chprintf(chp, "%s", HTML_selected);
        number = 1;
      } else { chprintf(chp, "%s", HTML_e_tag); }
      chprintf(chp, "%s - ", TEXT_Sensor);
      printNodeAddress(chp, node[i].address, node[i].type, node[i].function, node[i].number, true);
      chprintf(chp, "%s", HTML_e_option);
    }
    logAddress++;
  }
  chprintf(chp, "%s%u", HTML_option, DUMMY_NO_VALUE);
  if (!number) chprintf(chp, "%s", HTML_selected);
  else         chprintf(chp, "%s", HTML_e_tag);
  if ((conf.trigger[webTrigger].type == 'S') && (!number)) chprintf(chp, "%s", TEXT_not_found);
  else                                      chprintf(chp, "%s", NOT_SET);
  chprintf(chp, "%s%s", HTML_e_option, HTML_e_select);
  // Condition
  chprintf(chp, "%s%s%s", HTML_e_td_e_tr_tr_td, TEXT_Condition, HTML_e_td_td);
  chprintf(chp, "%sc%sc%s", HTML_select, HTML_id_tag, HTML_e_tag);
  for (uint8_t i = 0; i < ARRAY_SIZE(triggerCondition); i++) {
     chprintf(chp, "%s%u", HTML_option, i);
     if (conf.trigger[webTrigger].condition == i) { chprintf(chp, "%s", HTML_selected); }
     else                                         { chprintf(chp, "%s", HTML_e_tag); }
     chprintf(chp, "%s%s", triggerCondition[i], HTML_e_option);
   }
  chprintf(chp, "%s", HTML_e_select);
  // Value
  chprintf(chp, "%s%s%s", HTML_e_td_e_tr_tr_td, TEXT_Value, HTML_e_td_td);
  chprintf(chp, "%s1%s", HTML_div_id_1, HTML_div_id_2);
  chprintf(chp, "%sg%sg%s", HTML_select, HTML_id_tag, HTML_e_tag);
  for (uint8_t i = 0; i < ARRAY_SIZE(groupState); i++) {
    chprintf(chp, "%s%u", HTML_option, i);
    if ((uint8_t)conf.trigger[webTrigger].value == i) { chprintf(chp, "%s", HTML_selected); }
    else                                              { chprintf(chp, "%s", HTML_e_tag); }
    chprintf(chp, "%s%s", groupState[i], HTML_e_option);
  }
  chprintf(chp, "%s", HTML_e_select);
  chprintf(chp, "%s%s2%s", HTML_div_e, HTML_div_id_1, HTML_div_id_2);
  chprintf(chp, "%sz%sz%s", HTML_select, HTML_id_tag, HTML_e_tag);
  for (uint8_t i = 0; i < ARRAY_SIZE(zoneState); i++) {
    chprintf(chp, "%s%u", HTML_option, i);
    if ((uint8_t)conf.trigger[webTrigger].value == i) { chprintf(chp, "%s", HTML_selected); }
    else                                              { chprintf(chp, "%s", HTML_e_tag); }
    chprintf(chp, "%s%s", zoneState[i], HTML_e_option);
  }
  chprintf(chp, "%s", HTML_e_select);
  chprintf(chp, "%s%s3%s", HTML_div_e, HTML_div_id_1, HTML_div_id_2);
  printFloatInput(chp, 'v', conf.trigger[webTrigger].value);
  chprintf(chp, "%s", HTML_div_e);
  chprintf(chp, "%s%s%s", HTML_e_td_e_tr_tr_td, TEXT_Hysteresis, HTML_e_td_td);
  printFloatInput(chp, 'h', conf.trigger[webTrigger].hysteresis);

  chprintf(chp, "%s%s %s%s", HTML_e_td_e_tr_tr_td, TEXT_Evaluate, TEXT_script, HTML_e_td_td);
  chprintf(chp, "%sp%s", HTML_select, HTML_e_tag);
  logAddress = 1;
  for (scriptp = scriptLL; scriptp != NULL; scriptp = scriptp->next) {
    chprintf(chp, "%s%s", HTML_option, scriptp->name);
    if (strcmp(&conf.trigger[webTrigger].evalScript[0], scriptp->name) == 0) {
      chprintf(chp, "%s", HTML_selected);
      logAddress = 0;
    } else {
      chprintf(chp, "%s", HTML_e_tag);
    }
    chprintf(chp, "%s%s", scriptp->name, HTML_e_option);
  }
  chprintf(chp, "%s", HTML_option);
  if (logAddress) { chprintf(chp, "%s", HTML_selected); }
  else            { chprintf(chp, "%s", HTML_e_tag); }
  chprintf(chp, "%s%s%s", NOT_SET, HTML_e_option, HTML_e_select);

  chprintf(chp, "%s%s%s", HTML_e_td_e_tr_tr_td, TEXT_Alert, HTML_e_td_td);
  printOnOffButton(chp, "H", GET_CONF_TRIGGER_ALERT(conf.trigger[webTrigger].setting));
  chprintf(chp, "%s%s%s", HTML_e_td_e_tr_tr_td, TEXT_Pass, HTML_e_td_td);
  printTwoButton(chp, "C", GET_CONF_TRIGGER_PASS_VALUE(conf.trigger[webTrigger].setting),
                 0, 0b00, TEXT_constant, TEXT_value);
  chprintf(chp, "%s", HTML_br);
  printThreeButton(chp, "s", GET_CONF_TRIGGER_PASS(conf.trigger[webTrigger].setting),
                   1, 0b110, triggerPassType[0], triggerPassType[1], triggerPassType[2], 0);
  chprintf(chp, "%s%s %s%s", HTML_e_td_e_tr_tr_td, TEXT_Pass, TEXT_Off,HTML_e_td_td);
  printThreeButton(chp, "S", GET_CONF_TRIGGER_PASS_OFF(conf.trigger[webTrigger].setting),
                   2, 0b100, triggerPassOffType[0], triggerPassOffType[1], triggerPassOffType[2], 0);
  chprintf(chp, "%s%s %s%s", HTML_e_td_e_tr_tr_td, TEXT_Off, TEXT_timer, HTML_e_td_td);
  printIntInput(chp, 't', conf.trigger[webTrigger].offTime, 3, 1, 255);
  printDurationSelect(chp, 'T', GET_CONF_TRIGGER_OFF_PERIOD(conf.trigger[webTrigger].setting));
  chprintf(chp, "%s%s%s", HTML_tr_td, TEXT_Address, HTML_e_td_td);
  chprintf(chp, "%sa%sa%s", HTML_select, HTML_id_tag, HTML_e_tag);
  for (uint8_t i = 0; i <= NODE_SIZE; i++) {
    if (i < NODE_SIZE) {
      if (node[i].type == 'I') {
        chprintf(chp, "%s%u", HTML_option, i);
        if ((node[i].address  == conf.trigger[webTrigger].toAddress) &&
            (node[i].function == conf.trigger[webTrigger].toFunction) &&
            (node[i].number   == conf.trigger[webTrigger].toNumber))
             { chprintf(chp, "%s", HTML_selected); }
        else { chprintf(chp, "%s", HTML_e_tag); }
        printNodeAddress(chp, node[i].address, node[i].type, node[i].function, node[i].number, true);
        chprintf(chp, "%s", HTML_e_option);
      }
    } else {
      chprintf(chp, "%s%u", HTML_option, DUMMY_NO_VALUE);
      if ((conf.trigger[webTrigger].toAddress  == 0) &&
          (conf.trigger[webTrigger].toFunction == ' ') &&
          (conf.trigger[webTrigger].toNumber   == 0))
           { chprintf(chp, "%s", HTML_selected); }
      else { chprintf(chp, "%s", HTML_e_tag); }
      chprintf(chp, "%s%s", NOT_SET, HTML_e_option);
    }
  }
  chprintf(chp, "%s%s %s%s", HTML_e_td_e_tr_tr_td, TEXT_On, TEXT_value, HTML_e_td_td);
  printFloatInput(chp, 'o', conf.trigger[webTrigger].constantOn);
  chprintf(chp, "%s%s %s%s", HTML_e_td_e_tr_tr_td, TEXT_Off, TEXT_value, HTML_e_td_td);
  printFloatInput(chp, 'f', conf.trigger[webTrigger].constantOff);
  chprintf(chp, "%s%s", HTML_e_td_e_tr, HTML_e_table);
  // JavaScript
  chprintf(chp, "%s%s%s", HTML_script, JS_Trigger, HTML_e_script);
  chprintf(chp, "%s%s%u", HTML_script, JS_TriggerSel_1, ALARM_GROUPS);
  chprintf(chp, "%s%u%s%u", JS_TriggerSel_2, ALARM_GROUPS, JS_TriggerSel_3, ALARM_GROUPS + ALARM_ZONES);
  chprintf(chp, "%s%s", JS_TriggerSel_4, HTML_e_script);
  // Buttons
  chprintf(chp, "%s%s", HTML_Apply, HTML_Save);
}

/*
 * @brief HTTP trigger POST handler
 * @param postDataP Pointer to POST data string
 */
static void httpd_post_custom_trigger(char **postDataP) {
  uint16_t number, valueLen = 0, triggerNum;
  char name[3];
  bool repeat;
  char *valueP;

  do {
    repeat = getPostData(postDataP, &name[0], sizeof(name), &valueP, &valueLen);
    DBG_HTTP("Parse: %s = '%.*s' (%u)\r\n", name, valueLen, valueP, valueLen);
    switch(name[0]) {
      case 'P': // select
        number = strtol(valueP, NULL, 10);
        if (number != webTrigger) { webTrigger = number; repeat = 0; }
      break;
      case 'A': // Apply
      break;
      case 'n': // name
        strncpy(conf.trigger[webTrigger].name, valueP, LWIP_MIN(valueLen, NAME_LENGTH - 1));
        conf.trigger[webTrigger].name[LWIP_MIN(valueLen, NAME_LENGTH - 1)] = 0;
      break;
      case 'a': // node aaddress
        number = strtol(valueP, NULL, 10);
        if (number == DUMMY_NO_VALUE) {
          conf.trigger[webTrigger].toAddress  = 0;
          conf.trigger[webTrigger].toFunction = ' ';
          conf.trigger[webTrigger].toNumber   = 0;
        } else {
          conf.trigger[webTrigger].toAddress  = node[number].address;
          conf.trigger[webTrigger].toFunction = node[number].function;
          conf.trigger[webTrigger].toNumber   = node[number].number;
        }
      break;
      case 'y': // type
        triggerNum = number = strtol(valueP, NULL, 10);
        if (number < ALARM_GROUPS) {
          conf.trigger[webTrigger].type       = 'G';
          conf.trigger[webTrigger].address  = 0;
          conf.trigger[webTrigger].function = ' ';
          conf.trigger[webTrigger].number   = number;
        } else if ((number >= ALARM_GROUPS) && (number < (ALARM_ZONES + ALARM_GROUPS))) {
          conf.trigger[webTrigger].type       = 'Z';
          conf.trigger[webTrigger].address  = 0;
          conf.trigger[webTrigger].function = ' ';
          conf.trigger[webTrigger].number   = number - ALARM_GROUPS;
        } else if (number >= (ALARM_ZONES + ALARM_GROUPS)) {
          conf.trigger[webTrigger].type       = 'S';
          conf.trigger[webTrigger].address  = node[number - ALARM_ZONES - ALARM_GROUPS].address;
          conf.trigger[webTrigger].function = node[number - ALARM_ZONES - ALARM_GROUPS].function;
          conf.trigger[webTrigger].number   = node[number - ALARM_ZONES - ALARM_GROUPS].number;
        } else {
          conf.trigger[webTrigger].type     = ' ';
          conf.trigger[webTrigger].address  = 0;
          conf.trigger[webTrigger].function = ' ';
          conf.trigger[webTrigger].number   = 0;
        }
      break;
      case 'c': // condition
        conf.trigger[webTrigger].condition = strtol(valueP, NULL, 10);
      break;
      case 't': // off timer
        conf.trigger[webTrigger].offTime = strtol(valueP, NULL, 10);
      break;
      case 'T': // period
        SET_CONF_TRIGGER_OFF_PERIOD(conf.trigger[webTrigger].setting, strtol(valueP, NULL, 10));
      break;
      case 's': // Pass no yes once
        SET_CONF_TRIGGER_PASS(conf.trigger[webTrigger].setting, strtol(valueP, NULL, 10));
      break;
      case 'S': // Pass off no yes timer
        SET_CONF_TRIGGER_PASS_OFF(conf.trigger[webTrigger].setting, strtol(valueP, NULL, 10));
      break;
      case 'o':
        conf.trigger[webTrigger].constantOn = strtof(valueP, NULL);
      break;
      case 'f':
        conf.trigger[webTrigger].constantOff = strtof(valueP, NULL);
      break;
      case 'g': // Value for Group
        if (triggerNum < ALARM_GROUPS)
          conf.trigger[webTrigger].value = strtof(valueP, NULL);
      break;
      case 'z': // Value for Zone
        if ((triggerNum >= (ALARM_GROUPS)) && (triggerNum < (ALARM_ZONES + ALARM_GROUPS)))
          conf.trigger[webTrigger].value = strtof(valueP, NULL);
      break;
      case 'v': // Value for Sensor
        if (triggerNum >= (ALARM_ZONES + ALARM_GROUPS))
          conf.trigger[webTrigger].value = strtof(valueP, NULL);
      break;
      case 'p': // script
        strncpy(conf.trigger[webTrigger].evalScript, valueP, LWIP_MIN(valueLen, NAME_LENGTH - 1));
        conf.trigger[webTrigger].evalScript[LWIP_MIN(valueLen, NAME_LENGTH - 1)] = 0;
      break;
      case 'B' ... 'H': // Handle all single radio buttons for settings B(66)=0
        if (valueP[0] == '0') conf.trigger[webTrigger].setting &= ~(1 << (name[0]-66));
        else                  conf.trigger[webTrigger].setting |=  (1 << (name[0]-66));
      break;
      case 'e': // save
        writeToBkpSRAM((uint8_t*)&conf, sizeof(config_t), 0);
      break;
    }
  } while (repeat);
}


#endif /* HTTPD_HANDLER_TRIGGER_H_ */
