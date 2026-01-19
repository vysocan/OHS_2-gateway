/*
 * httpd_handler_timer.h
 *
 *  Created on: 16. 1. 2026
 *      Author: vysocan
 */

#ifndef HTTPD_HANDLER_TIMER_H_
#define HTTPD_HANDLER_TIMER_H_


/*
 * @brief HTTP timer page handler
 * @param chp Pointer to the output stream
 */
static void fs_open_custom_timer(BaseSequentialStream *chp) {
  time_t tempTime;
  uint16_t logAddress;
  chprintf(chp, "%s#", HTML_tr_th);
  chprintf(chp, "%s%s", HTML_e_th_th, TEXT_Name);
  chprintf(chp, "%s%s", HTML_e_th_th, TEXT_On);
  chprintf(chp, "%s%s", HTML_e_th_th, TEXT_Type);
  chprintf(chp, "%s%s", HTML_e_th_th, TEXT_Start);
  chprintf(chp, "%s%s", HTML_e_th_th, TEXT_Period);
  chprintf(chp, "%s%s %s", HTML_e_th_th, TEXT_Run, TEXT_time);
  chprintf(chp, "%s%s %s", HTML_e_th_th, TEXT_Next, TEXT_on);
  chprintf(chp, "%s%s %s", HTML_e_th_th, TEXT_Next, TEXT_off);
  chprintf(chp, "%s%s", HTML_e_th_th, TEXT_Address);
  chprintf(chp, "%s%s", HTML_e_th_th, TEXT_Script);
  chprintf(chp, "%s%s", HTML_e_th_th, TEXT_On);
  chprintf(chp, "%s%s", HTML_e_th_th, TEXT_Off);
  chprintf(chp, "%s%s%s\r\n", HTML_e_th_th, TEXT_Status, HTML_e_th_e_tr);
  // Information table
  for (uint8_t i = 0; i < TIMER_SIZE; i++) {
    chprintf(chp, "%s%u.%s", HTML_tr_td, i + 1, HTML_e_td_td);
    chprintf(chp, "%s%s", conf.timer[i].name, HTML_e_td_td);
    printOkNok(chp, GET_CONF_TIMER_ENABLED(conf.timer[i].setting));
    chprintf(chp, "%s", HTML_e_td_td);
    if (GET_CONF_TIMER_TYPE(conf.timer[i].setting)) {
      chprintf(chp, "%s %s", TEXT_Run, TEXT_on);
      for (uint8_t j = 0; j < ARRAY_SIZE(weekDayShort); j++) {
        if ((conf.timer[i].setting >> (8 - j)) & 0b1) {
          chprintf(chp, " %s", weekDayShort[j]);
        }
      }
    } else {
      chprintf(chp, "%s", TEXT_Period);
    }
    chprintf(chp, "%s", HTML_e_td_td);
    chprintf(chp, "%02u:%02u%s", conf.timer[i].startTime / SECONDS_PER_MINUTE,
             conf.timer[i].startTime % SECONDS_PER_MINUTE, HTML_e_td_td);
    if (!GET_CONF_TIMER_TYPE(conf.timer[i].setting)) {
      chprintf(chp, "%u %s", conf.timer[i].periodTime,
               durationSelect[GET_CONF_TIMER_PERIOD_TYPE(conf.timer[i].setting)]);
    }
    chprintf(chp, "%s%u %s", HTML_e_td_td,  conf.timer[i].runTime,
             durationSelect[GET_CONF_TIMER_RUN_TYPE(conf.timer[i].setting)]);
    chprintf(chp, "%s", HTML_e_td_td);
    tempTime = conf.timer[i].nextOn;
    if (GET_CONF_TIMER_ENABLED(conf.timer[i].setting)) printFrmTimestamp(chp, &tempTime);
    chprintf(chp, "%s", HTML_e_td_td);
    tempTime = conf.timer[i].nextOff;
    if (GET_CONF_TIMER_ENABLED(conf.timer[i].setting)) printFrmTimestamp(chp, &tempTime);
    chprintf(chp, "%s", HTML_e_td_td);
    printNodeAddress(chp, conf.timer[i].toAddress, 'I',  conf.timer[i].toFunction,
                     conf.timer[i].toNumber, true);
    chprintf(chp, "%s", HTML_e_td_td);
    if (conf.timer[i].evalScript[0]) chprintf(chp, "%s", conf.timer[i].evalScript);
    else chprintf(chp, "%s", NOT_SET);
    chprintf(chp, "%s", HTML_e_td_td);
    chprintf(chp, "%.2f%s", conf.timer[i].constantOn, HTML_e_td_td);
    chprintf(chp, "%.2f%s", conf.timer[i].constantOff, HTML_e_td_td);
    printOkNok(chp, GET_CONF_TIMER_TRIGGERED(conf.timer[i].setting));
    chprintf(chp, "%s", HTML_e_td_e_tr);
  }
  chprintf(chp, "%s%s", HTML_e_table, HTML_table);
  chprintf(chp, "%s%s%s", HTML_tr_td, TEXT_Timer, HTML_e_td_td);
  chprintf(chp, "%sP%s", HTML_select_submit, HTML_e_tag);
  for (uint8_t i = 0; i < TIMER_SIZE; i++) {
    chprintf(chp, "%s%u", HTML_option, i);
    if (webTimer == i) { chprintf(chp, "%s", HTML_selected); }
    else               { chprintf(chp, "%s", HTML_e_tag); }
    chprintf(chp, "%u. %s%s", i + 1, conf.timer[i].name, HTML_e_option);
  }
  chprintf(chp, "%s%s", HTML_e_select, HTML_e_td_e_tr_tr_td);
  chprintf(chp, "%s%s", TEXT_Name, HTML_e_td_td);
  printTextInput(chp, 'n', conf.timer[webTimer].name, NAME_LENGTH);
  chprintf(chp, "%s%s %s%s", HTML_e_td_e_tr_tr_td, TEXT_Timer, TEXT_is, HTML_e_td_td);
  printOnOffButton(chp, "B", GET_CONF_TIMER_ENABLED(conf.timer[webTimer].setting));
  chprintf(chp, "%s%s%s", HTML_e_td_e_tr_tr_td, TEXT_Type, HTML_e_td_td);
  printTwoButton(chp, "C", GET_CONF_TIMER_TYPE(conf.timer[webTimer].setting), 1, 0b10,
                 TEXT_Period, TEXT_Calendar);
  chprintf(chp, "%s%s%s", HTML_e_td_e_tr_tr_td, weekDay[0], HTML_e_td_td);
  printOnOffButton(chp, "J", GET_CONF_TIMER_MO(conf.timer[webTimer].setting));
  chprintf(chp, "%s%s%s", HTML_e_td_e_tr_tr_td, weekDay[1], HTML_e_td_td);
  printOnOffButton(chp, "I", GET_CONF_TIMER_TU(conf.timer[webTimer].setting));
  chprintf(chp, "%s%s%s", HTML_e_td_e_tr_tr_td, weekDay[2], HTML_e_td_td);
  printOnOffButton(chp, "H", GET_CONF_TIMER_WE(conf.timer[webTimer].setting));
  chprintf(chp, "%s%s%s", HTML_e_td_e_tr_tr_td, weekDay[3], HTML_e_td_td);
  printOnOffButton(chp, "G", GET_CONF_TIMER_TH(conf.timer[webTimer].setting));
  chprintf(chp, "%s%s%s", HTML_e_td_e_tr_tr_td, weekDay[4], HTML_e_td_td);
  printOnOffButton(chp, "F", GET_CONF_TIMER_FR(conf.timer[webTimer].setting));
  chprintf(chp, "%s%s%s", HTML_e_td_e_tr_tr_td, weekDay[5], HTML_e_td_td);
  printOnOffButton(chp, "E", GET_CONF_TIMER_SA(conf.timer[webTimer].setting));
  chprintf(chp, "%s%s%s", HTML_e_td_e_tr_tr_td, weekDay[6], HTML_e_td_td);
  printOnOffButton(chp, "D", GET_CONF_TIMER_SU(conf.timer[webTimer].setting));
  chprintf(chp, "%s%s %s%s", HTML_e_td_e_tr_tr_td, TEXT_Start, TEXT_time, HTML_e_td_td);
  printTimeInput(chp, 't', conf.timer[webTimer].startTime / 60, conf.timer[webTimer].startTime % 60);
  chprintf(chp, "%s%s %s%s", HTML_e_td_e_tr_tr_td, TEXT_Period, TEXT_duration, HTML_e_td_td);
  printIntInput(chp, 's', conf.timer[webTimer].periodTime, 3, 1, 255);
  printDurationSelect(chp, 'S', GET_CONF_TIMER_PERIOD_TYPE(conf.timer[webTimer].setting));
  chprintf(chp, "%s%s %s%s", HTML_e_td_e_tr_tr_td, TEXT_Run, TEXT_duration, HTML_e_td_td);
  printIntInput(chp, 'r', conf.timer[webTimer].runTime, 3, 1, 255);
  printDurationSelect(chp, 'R', GET_CONF_TIMER_RUN_TYPE(conf.timer[webTimer].setting));
  chprintf(chp, "%s%s%s", HTML_tr_td, TEXT_Address, HTML_e_td_td);
  chprintf(chp, "%sa%s", HTML_select, HTML_e_tag);
  for (uint8_t i = 0; i <= NODE_SIZE; i++) {
    if (i < NODE_SIZE) {
      if (node[i].type == 'I') {
        chprintf(chp, "%s%u", HTML_option, i);
        if ((node[i].address  == conf.timer[webTimer].toAddress) &&
            (node[i].function == conf.timer[webTimer].toFunction) &&
            (node[i].number   == conf.timer[webTimer].toNumber))
             { chprintf(chp, "%s", HTML_selected); }
        else { chprintf(chp, "%s", HTML_e_tag); }
        printNodeAddress(chp, node[i].address, node[i].type, node[i].function, node[i].number, true);
        chprintf(chp, "%s", HTML_e_option);
      }
    } else {
      chprintf(chp, "%s%u", HTML_option, DUMMY_NO_VALUE);
      if ((conf.timer[webTimer].toAddress  == 0) &&
          (conf.timer[webTimer].toFunction == ' ') &&
          (conf.timer[webTimer].toNumber   == 0))
           { chprintf(chp, "%s", HTML_selected); }
      else { chprintf(chp, "%s", HTML_e_tag); }
      chprintf(chp, "%s%s", NOT_SET, HTML_e_option);
    }
  }
  chprintf(chp, "%s", HTML_e_select);
  chprintf(chp, "%s%s %s%s", HTML_e_td_e_tr_tr_td, TEXT_Evaluate, TEXT_script, HTML_e_td_td);
  chprintf(chp, "%sp%s", HTML_select, HTML_e_tag);
  logAddress = 1;
  for (scriptp = scriptLL; scriptp != NULL; scriptp = scriptp->next) {
    chprintf(chp, "%s%s", HTML_option, scriptp->name);
    if (strcmp(&conf.timer[webTimer].evalScript[0], scriptp->name) == 0) {
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
  chprintf(chp, "%s%s %s%s", HTML_e_td_e_tr_tr_td, TEXT_On, TEXT_value, HTML_e_td_td);
  printFloatInput(chp, 'o', conf.timer[webTimer].constantOn);
  chprintf(chp, "%s%s %s%s", HTML_e_td_e_tr_tr_td, TEXT_Off, TEXT_value, HTML_e_td_td);
  printFloatInput(chp, 'f', conf.timer[webTimer].constantOff);
  chprintf(chp, "%s%s", HTML_e_td_e_tr, HTML_e_table);
  // JavaScript
  chprintf(chp, "%s%s%s", HTML_script, JS_Timer, HTML_e_script);
  // Buttons
  chprintf(chp, "%s%s", HTML_Apply, HTML_Save);
}

/*
 * @brief HTTP timer POST handler
 * @param postDataP Pointer to POST data string
 */
static void httpd_post_custom_timer(char **postDataP) {
  uint16_t number, valueLen = 0;
  char name[3];
  bool repeat;
  char *valueP, *endP;

  do {
    repeat = getPostData(postDataP, &name[0], sizeof(name), &valueP, &valueLen);
    DBG_HTTP("Parse: %s = '%.*s' (%u)\r\n", name, valueLen, valueP, valueLen);
    switch(name[0]) {
      case 'P': // select
        number = strtol(valueP, NULL, 10);
        if (number != webTimer) { webTimer = number; repeat = 0; }
      break;
      case 'A': // Apply
        setTimer(webTimer, true);
      break;
      case 'n': // name
        strncpy(conf.timer[webTimer].name, valueP, LWIP_MIN(valueLen, NAME_LENGTH - 1));
        conf.timer[webTimer].name[LWIP_MIN(valueLen, NAME_LENGTH - 1)] = 0;
      break;
      case 's': // period
        conf.timer[webTimer].periodTime = strtol(valueP, NULL, 10);
      break;
      case 'S': // period
        number = strtol(valueP, NULL, 10);
        SET_CONF_TIMER_PERIOD_TYPE(conf.timer[webTimer].setting, number);
      break;
      case 'r': // run
        conf.timer[webTimer].runTime = strtol(valueP, NULL, 10);
      break;
      case 'R': // run
        number = strtol(valueP, NULL, 10);
        SET_CONF_TIMER_RUN_TYPE(conf.timer[webTimer].setting, number);
      break;
      case 'a': // node aaddress
        number = strtol(valueP, NULL, 10);
        if (number == DUMMY_NO_VALUE) {
          conf.timer[webTimer].toAddress  = 0;
          conf.timer[webTimer].toFunction = ' ';
          conf.timer[webTimer].toNumber   = 0;
        } else {
          conf.timer[webTimer].toAddress  = node[number].address;
          conf.timer[webTimer].toFunction = node[number].function;
          conf.timer[webTimer].toNumber   = node[number].number;
        }
      break;
      case 'o':
        conf.timer[webTimer].constantOn = strtof(valueP, NULL);
      break;
      case 'f':
        conf.timer[webTimer].constantOff = strtof(valueP, NULL);
      break;
      case 't': // time
        conf.timer[webTimer].startTime = strtol(valueP, &endP, 10) * MINUTES_PER_HOUR ;
        conf.timer[webTimer].startTime += strtol(++endP, NULL, 10);
      break;
      case 'p': // script
        strncpy(conf.timer[webTimer].evalScript, valueP, LWIP_MIN(valueLen, NAME_LENGTH - 1));
        conf.timer[webTimer].evalScript[LWIP_MIN(valueLen, NAME_LENGTH - 1)] = 0;
      break;
      case 'B' ... 'J': // Handle all single radio buttons for settings B(66)=0
        if (valueP[0] == '0') conf.timer[webTimer].setting &= ~(1 << (name[0]-66));
        else                  conf.timer[webTimer].setting |=  (1 << (name[0]-66));
      break;
      case 'e': // save
        writeToBkpSRAM((uint8_t*)&conf, sizeof(config_t), 0);
      break;
    }
  } while (repeat);
}


#endif /* HTTPD_HANDLER_TIMER_H_ */
