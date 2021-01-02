/*
 * ohs_th_tcl.h
 *
 *  Created on: 18. 4. 2020
 *      Author: adam
 */

#ifndef OHS_TH_TCL_H_
#define OHS_TH_TCL_H_

#ifndef TCL_DEBUG
#define TCL_DEBUG 0
#endif

#if TCL_DEBUG
#define DBG_TCL(...) {chprintf((console, __VA_ARGS__);}
#else
#define DBG_TCL(...)
#endif

/*
 * TCL command node
 */
static int tcl_cmd_node(struct tcl* tcl, tcl_value_t* args, void* arg) {
  (void)arg;
  char *pch, *addressIndex[NODE_ADDRESS_SIZE];
  uint8_t nodeIndex = 0;
  uint8_t ret;
  char buf[10];

  tcl_value_t* nodeAddress = tcl_list_at(args, 1);
  //DBG_TCL("*tcl_cmd_node*: %s.\r\n", nodeAddress);

  // Get index
  pch = strtok((char*)nodeAddress,":");
  while ((pch != NULL) && (nodeIndex < NODE_ADDRESS_SIZE)){
    addressIndex[nodeIndex] = pch;
    pch = strtok(NULL, ":");
    nodeIndex++;
  }
  if (nodeIndex == NODE_ADDRESS_SIZE) {
    nodeIndex = getNodeIndex((*addressIndex[0] == 'R' ? RADIO_UNIT_OFFSET : 0) + strtoul(addressIndex[1], NULL, 0),
                            *addressIndex[2], *addressIndex[3], strtoul(addressIndex[4], NULL, 0));
    if (nodeIndex != DUMMY_NO_VALUE) {
      //DBG_TCL("*tcl_cmd_node*: %d.\r\n", indexNum);
      chsnprintf(&buf[0], sizeof(buf), "%.2f", node[nodeIndex].value);
      ret = tcl_result(tcl, FNORMAL, tcl_alloc(&buf[0], strlen(buf)));
    } //else { indexNum = DUMMY_NO_VALUE; }
  }
  // Else error
  if (nodeIndex == DUMMY_NO_VALUE) {
    TCL_ERROR("Node %s not found", nodeAddress);
    ret = tcl_result(tcl, FERROR, tcl_alloc("", 0));
  }
  // free
  tcl_free(nodeAddress);
  return ret;
}
/*
 * TCL command group
 */
static int tcl_cmd_group(struct tcl* tcl, tcl_value_t* args, void* arg) {
  (void)arg;
  uint8_t ret;

  tcl_value_t* groupNumber = tcl_list_at(args, 1);
  tcl_value_t* groupCommand = tcl_list_at(args, 2);

  uint8_t groupNum = strtoul(groupNumber, NULL, 0) - 1;
  //DBG_TCL("groupNum: %u.\r\n", groupNum);
  //DBG_TCL("*groupCommand: %c.\r\n", *groupCommand);

  if ((groupNum < ALARM_GROUPS) && (GET_CONF_GROUP_ENABLED(conf.group[groupNum].setting))) {
    switch (*groupCommand) {
      case 'a': // Armed
        if (GET_GROUP_ARMED(group[groupNum].setting)) {
          if GET_GROUP_ARMED_HOME(group[groupNum].setting) {
            ret = tcl_result(tcl, FNORMAL, tcl_alloc("2", 1)); // Armed home
          } else {
            ret = tcl_result(tcl, FNORMAL, tcl_alloc("3", 1)); // Armed away
          }
        } else {
          if (group[groupNum].armDelay > 0) {
            ret = tcl_result(tcl, FNORMAL, tcl_alloc("1", 1)); // Arming
          } else {
            ret = tcl_result(tcl, FNORMAL, tcl_alloc("0", 1)); // Disarmed
          }
        }
        break;
      case 's': // Status
        if (GET_GROUP_ALARM(group[groupNum].setting) == 0) {
          if (GET_GROUP_WAIT_AUTH(group[groupNum].setting)) {
            ret = tcl_result(tcl, FNORMAL, tcl_alloc("1", 1)); // Awaiting authorization
          } else {
            ret = tcl_result(tcl, FNORMAL, tcl_alloc("2", 1)); // OK
          }
        } else {
          ret = tcl_result(tcl, FNORMAL, tcl_alloc("0", 1)); // Alarm
        }
        break;
      default: // error
        SUBCMDERROR("a_rmed|s_tatus");
        ret = tcl_result(tcl, FERROR, tcl_alloc("", 0));
        break;
    }
  } else {
    TCL_ERROR("Group #%s not enabled", groupNumber);
    ret = tcl_result(tcl, FERROR, tcl_alloc("", 0));
  }
  // free
  tcl_free(groupNumber);
  tcl_free(groupCommand);
  return ret;
}
/*
 * TCL command clock
 *
 * Seems it needs to add thread memory, + 512B, due to struct tm, strftime
 */
static int tcl_cmd_clock(struct tcl* tcl, tcl_value_t* args, void* arg) {
  (void)arg;

  uint8_t ret;
  char buf[30];
  tcl_value_t* val;

  // Allocate sub_cmd
  tcl_value_t* sub_cmd = tcl_list_at(args, 1);

  switch (*sub_cmd) {
    case 's': // seconds
      ARITY((tcl_list_length(args) == 2), sub_cmd, 0);
      uint8_t resp = chsnprintf(&buf[0], sizeof(buf), "%u", (uint32_t)getTimeUnixSec());
      ret = tcl_result(tcl, FNORMAL, tcl_alloc(buf, resp));
      break;
    case 'f': // format
      ARITY((tcl_list_length(args) == 3), sub_cmd, 1);
      val = tcl_list_at(args, 2);
      // Temporary variables
      time_t tmp = strtoul(val, NULL, 0);
      struct tm* ptm = gmtime(&tmp);
      // Check if return is 0 then format is invalid
      if (strftime(buf, sizeof(buf), conf.dateTimeFormat, ptm) != 0)
        ret = tcl_result(tcl, FNORMAL, tcl_alloc(buf, strlen(buf)));
      else
        ret = tcl_result(tcl, FERROR, tcl_alloc("", 0));
      // free
      tcl_free(val);
      break;
    case 'a': // add
      /*
       * If more precision, or month/year is needed,
       * then convertUnixSecondToRTCDateTime<>convertRTCDateTimeToUnixSecond can be used.
       */
      ARITY((tcl_list_length(args) == 5), sub_cmd, 3);
      val = tcl_list_at(args, 2);
      tcl_value_t* add = tcl_list_at(args, 3);
      tcl_value_t* scale = tcl_list_at(args, 4);
      // Temporary variables
      uint32_t tmpVal = strtoul(val, NULL, 0);
      uint32_t tmpValOld = tmpVal;
      uint32_t tmpAdd = strtoul(add, NULL, 0);
      switch(*scale) {
        case 's': tmpVal += tmpAdd; break;
        case 'm': tmpVal += tmpAdd * SECONDS_PER_MINUTE; break;
        case 'h': tmpVal += tmpAdd * SECONDS_PER_HOUR; break;
        case 'd': tmpVal += tmpAdd * SECONDS_PER_DAY; break;
        case 'w': tmpVal += tmpAdd * SECONDS_PER_DAY * 7; break;
        default: SUBCMDERROR("clock add $ $ s_econds|m_inutes|h_ours|d_ays|w_eeks"); break;
      }
      // new value should be be same as new or new value should not be 0
      if ((tmpVal != tmpValOld) && (tmpVal != 0) && (tmpAdd != 0)) {
        chsnprintf(&buf[0], sizeof(buf), "%d", tmpVal);
        ret = tcl_result(tcl, FNORMAL, tcl_alloc(buf, strlen(buf)));
      } else {
        ret = tcl_result(tcl, FERROR, tcl_alloc("", 0));
        if (tmpVal == 0) TCL_ERROR("Result value is 0");
        if (tmpAdd == 0) TCL_ERROR("Add value is 0");
        if (tmpVal == tmpValOld) TCL_ERROR("Result equal to original value");
      }
      // free
      tcl_free(scale);
      tcl_free(add);
      tcl_free(val);
      break;
    default: // error
      SUBCMDERROR("s_econds|f_ormat|a_dd");
      ret = tcl_result(tcl, FERROR, tcl_alloc("", 0));
      break;
  }
  // free
  tcl_free(sub_cmd);
  return ret;
}
/*
 * TCL timer command
 */
static int tcl_cmd_timer(struct tcl* tcl, tcl_value_t* args, void* arg) {
  (void)arg;
  uint8_t ret;

  tcl_value_t* timerNumber = tcl_list_at(args, 1);
  uint8_t timerNum = strtoul(timerNumber, NULL, 0) - 1;

  if ((timerNum < TIMER_SIZE) && (GET_CONF_TIMER_ENABLED(conf.timer[timerNum].setting))) {
    if (GET_CONF_TIMER_TRIGGERED(conf.timer[timerNum].setting)) {
      ret = tcl_result(tcl, FNORMAL, tcl_alloc("1", 1));
    } else {
      ret = tcl_result(tcl, FNORMAL, tcl_alloc("0", 1));
    }
  } else {
    TCL_ERROR("Timer #%s not enabled", timerNumber);
    ret = tcl_result(tcl, FERROR, tcl_alloc("", 0));
  }
  // free
  tcl_free(timerNumber);
  return ret;
}
/*
 * TCL trigger command
 */
static int tcl_cmd_trigger(struct tcl* tcl, tcl_value_t* args, void* arg) {
  (void)arg;
  uint8_t ret;

  tcl_value_t* triggerNumber = tcl_list_at(args, 1);
  uint8_t triggerNum = strtoul(triggerNumber, NULL, 0) - 1;

  if ((triggerNum < TRIGGER_SIZE) && (GET_CONF_TRIGGER_ENABLED(conf.trigger[triggerNum].setting))) {
    if (GET_CONF_TRIGGER_TRIGGERED(conf.trigger[triggerNum].setting)) {
      ret = tcl_result(tcl, FNORMAL, tcl_alloc("1", 1));
    } else {
      ret = tcl_result(tcl, FNORMAL, tcl_alloc("0", 1));
    }
  } else {
    TCL_ERROR("Trigger #%s not enabled", triggerNumber);
    ret = tcl_result(tcl, FERROR, tcl_alloc("", 0));
  }
  // free
  tcl_free(triggerNumber);
  return ret;
}
/*
 * TCL find index command
 */
static int tcl_cmd_findex(struct tcl* tcl, tcl_value_t* args, void* arg) {
  (void)arg;
  uint8_t ret, resp = 0;
  char buf[4];

  // Allocate
  tcl_value_t* sub_cmd = tcl_list_at(args, 1);
  tcl_value_t* name = tcl_list_at(args, 2);

  switch (*sub_cmd++) {
     case 'z': // zone
       for (uint8_t zoneNum=0; zoneNum < ALARM_ZONES ; zoneNum++){
         if (!(strcmp(name, conf.zoneName[zoneNum]))) {
           resp = chsnprintf(&buf[0], sizeof(buf), "%u", zoneNum + 1);
           ret = tcl_result(tcl, FNORMAL, tcl_alloc(buf, resp));
           break;
         }
       }
       // Not found
       if (!resp) ret = tcl_result(tcl, FERROR, tcl_alloc("0", 1));
       break;
     case 'g': // group
       for (uint8_t groupNum=0; groupNum < ALARM_ZONES ; groupNum++){
         if (!(strcmp(name, conf.group[groupNum].name))) {
           resp = chsnprintf(&buf[0], sizeof(buf), "%u", groupNum + 1);
           ret = tcl_result(tcl, FNORMAL, tcl_alloc(buf, resp));
           break;
         }
       }
       // Not found
       if (!resp) ret = tcl_result(tcl, FERROR, tcl_alloc("0", 1));
       break;
     case 't': // trigger or timer
       if (*sub_cmd == 'r') {
         for (uint8_t trigNum=0; trigNum < TRIGGER_SIZE ; trigNum++){
           if (!(strcmp(name, conf.trigger[trigNum].name))) {
             resp = chsnprintf(&buf[0], sizeof(buf), "%u", trigNum + 1);
             ret = tcl_result(tcl, FNORMAL, tcl_alloc(buf, resp));
             break;
           }
         }
       } else if (*sub_cmd == 'i') {
         for (uint8_t timerNum=0; timerNum < TIMER_SIZE ; timerNum++){
           if (!(strcmp(name, conf.timer[timerNum].name))) {
             resp = chsnprintf(&buf[0], sizeof(buf), "%u", timerNum + 1);
             ret = tcl_result(tcl, FNORMAL, tcl_alloc(buf, resp));
             break;
           }
         }
       } else {
         SUBCMDERROR("z_one|g_roup|ti_mer|tr_igger");
         ret = tcl_result(tcl, FERROR, tcl_alloc("", 0));
       }
       // Not found
       if (!resp) ret = tcl_result(tcl, FERROR, tcl_alloc("0", 1));
       break;
     default: // error
       SUBCMDERROR("z_one|g_roup|ti_mer|tr_igger");
       ret = tcl_result(tcl, FERROR, tcl_alloc("", 0));
       break;
   }

  // free
  tcl_free(name);
  tcl_free(sub_cmd);
  return ret;
}

/*
 * TCL execution thread
 */
static THD_WORKING_AREA(waTclThread, 2176);
static THD_FUNCTION(tclThread, arg) {
  chRegSetThreadName(arg);
  msg_t  msg;
  scriptEvent_t *inMsg;
  systime_t runTime;

  // Load script(s)
  initScripts(&scriptLL);

  MemoryStream ms;
  BaseSequentialStream *tclChp;
  msObjectInit(&ms, (uint8_t *)tclOutput, TCL_OUTPUT_LENGTH-1, 0);
  tclChp = (BaseSequentialStream *)(void *)&ms;

  tcl_init(&tcl, conf.tclIteration, tclChp);
  tcl_register(&tcl, "node", tcl_cmd_node, 2, NULL,
               "return value of given node. (node $(address))");
  tcl_register(&tcl, "group", tcl_cmd_group, 3, NULL,
               "return value of given group. (group $(number) a_rmed|s_tatus)");
  tcl_register(&tcl, "clock", tcl_cmd_clock, 0, NULL,
               "time and date manipulation. (clock seconds|format|add)");
  tcl_register(&tcl, "timer", tcl_cmd_timer, 2, NULL,
               "timer status. (timer $(number))");
  tcl_register(&tcl, "trigger", tcl_cmd_trigger, 2, NULL,
                 "trigger status. (trigger $(number))");
  tcl_register(&tcl, "findex", tcl_cmd_findex, 3, NULL,
                   "find index. (findex z_one|g_roup|ti_mer|tr_igger $(name)");

  // Process umm info
  umm_info(&ohsUmmHeap[0], true);

  while (true) {
    msg = chMBFetchTimeout(&script_mb, (msg_t*)&inMsg, TIME_INFINITE);
    if (msg == MSG_OK) {

      // Prepare for run
      memset(&tclOutput[0], '\0', TCL_OUTPUT_LENGTH);
      ms.eos = ms.offset = 0;
      tcl_iteration = conf.tclIteration;
      runTime = chVTGetSystemTimeX();

      // Run TCL
      DBG_TCL("Script: %s, length %u\r\n", inMsg->cmdP, strlen(inMsg->cmdP));
      if (tcl_eval(&tcl, inMsg->cmdP, strlen(inMsg->cmdP)) != FERROR) {
        chprintf(tclChp, "\r\n%s: %.*s", text_Result, tcl_length(tcl.result), tcl_string(tcl.result));
      } else {
        chprintf(tclChp, "\r\n%s: %s", text_Result, text_error);
      }
      chprintf(tclChp, "\r\nElapsed: %u ms\r\n", TIME_I2MS(chVTGetSystemTimeX() - runTime));

      // Pass result
      if (inMsg->result) {
        *inMsg->result = tcl_string(tcl.result);
      }
      // Do callback if requested
      if (inMsg->callback) {
        script_cb(inMsg->callback, tcl_string(tcl.result));
      }
      // Process umm info
      umm_info(&ohsUmmHeap[0], true);
    } else {
      chprintf(console, "Script MB ERROR\r\n");
    }
    chPoolFree(&script_pool, inMsg);
  }

  tcl_destroy(&tcl);
}


#endif /* OHS_TH_TCL_H_ */
