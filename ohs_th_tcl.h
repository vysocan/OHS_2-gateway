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

#if SERVICE_DEBUG
#define DBG_TCL(...) {DBG_TCL(__VA_ARGS__);}
#else
#define DBG_TCL(...)
#endif

/*
 * TCL command node
 */
#define TCL_CMD_NODE_ADDRESS_SIZE 5
static int tcl_cmd_node(struct tcl* tcl, tcl_value_t* args, void* arg) {
  (void)arg;
  char *pch, *index[TCL_CMD_NODE_ADDRESS_SIZE];
  uint8_t indexNum = 0;
  int ret;
  char buf[10];

  tcl_value_t* nodeAddress = tcl_list_at(args, 1);
  //DBG_TCL("*tcl_cmd_node*: %s.\r\n", nodeAddress);

  // Get index
  pch = strtok((char*)nodeAddress,":");
  while ((pch != NULL) && (indexNum < TCL_CMD_NODE_ADDRESS_SIZE)){
    index[indexNum] = pch;
    pch = strtok(NULL, ":");
    indexNum++;
  }
  if (indexNum == TCL_CMD_NODE_ADDRESS_SIZE) {
    indexNum = getNodeIndex((*index[0] == 'R' ? RADIO_UNIT_OFFSET : 0) + strtoul(index[1], NULL, 0),
                            *index[2], *index[3], strtoul(index[4], NULL, 0));
    if (indexNum != DUMMY_NO_VALUE) {
      //DBG_TCL("*tcl_cmd_node*: %d.\r\n", indexNum);
      chsnprintf(&buf[0], sizeof(buf), "%.2f", node[indexNum].value);
      ret = tcl_result(tcl, FNORMAL, tcl_alloc(&buf[0], strlen(buf)));
    } else {
      indexNum = DUMMY_NO_VALUE;
    }
  }
  // Else error
  if (indexNum == DUMMY_NO_VALUE) {
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
  int ret;
  uint8_t groupNum;

  tcl_value_t* groupNumber = tcl_list_at(args, 1);
  tcl_value_t* groupCommand = tcl_list_at(args, 2);

  groupNum = strtoul(groupNumber, NULL, 0) - 1;
  //DBG_TCL("groupNum: %u.\r\n", groupNum);
  //DBG_TCL("*groupCommand: %c.\r\n", *groupCommand);

  if ((groupNum < ALARM_GROUPS) && (GET_CONF_GROUP_ENABLED(conf.group[groupNum]))) {
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
      default: SUBCMDERROR("a_rmed|s_tatus"); break;
    }
  } else {
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
 * Seems it needs a lot of thread memory, + 512B
 */
static int tcl_cmd_clock(struct tcl* tcl, tcl_value_t* args, void* arg) {
  (void)arg;
  int ret;

  tcl_value_t* sub_cmd = tcl_list_at(args, 1);

  if (SUBCMD(sub_cmd, "seconds")) {
    ARITY((tcl_list_length(args) == 2), sub_cmd, 0);
    char buf[11];
    uint8_t resp = chsnprintf(&buf[0], sizeof(buf), "%u", (uint32_t)getTimeUnixSec());
    ret = tcl_result(tcl, FNORMAL, tcl_alloc(buf, resp));
  } else if (SUBCMD(sub_cmd, "format")) {
    ARITY((tcl_list_length(args) == 3), sub_cmd, 1);
    tcl_value_t* val = tcl_list_at(args, 2);
    // Temporary variables
    time_t tmp = strtoul(val, NULL, 0);
    struct tm *ptm = gmtime(&tmp);
    char   buf[30];
    // Check if return is 0 then format is invalid
    if (strftime(buf, sizeof(buf), conf.dateTimeFormat, ptm) != 0)
      ret = tcl_result(tcl, FNORMAL, tcl_alloc(buf, strlen(buf)));
    else
      ret = tcl_result(tcl, FERROR, tcl_alloc("", 0));
    // free
    tcl_free(val);
  } else if (SUBCMD(sub_cmd, "add")) {
    /*
     * If more precision, or month/year is needed,
     * then convertUnixSecondToRTCDateTime<>convertRTCDateTimeToUnixSecond can be used.
     */
    ARITY((tcl_list_length(args) == 5), sub_cmd, 3);
    tcl_value_t* val = tcl_list_at(args, 2);
    tcl_value_t* add = tcl_list_at(args, 3);
    tcl_value_t* scale = tcl_list_at(args, 4);
    // Temporary variables
    time_t tmpVal = strtoul(val, NULL, 0);
    time_t tmpValOld = tmpVal;
    time_t tmpAdd = strtoul(add, NULL, 0);
    char   buf[30];
    switch(*scale) {
      case 's': tmpVal += tmpAdd; break;
      case 'm': tmpVal += tmpAdd * SECONDS_PER_MINUTE; break;
      case 'h': tmpVal += tmpAdd * SECONDS_PER_HOUR; break;
      case 'd': tmpVal += tmpAdd * SECONDS_PER_DAY; break;
      case 'w': tmpVal += tmpAdd * SECONDS_PER_DAY * 7; break;
      default: SUBCMDERROR("clock add $ s_econds|m_inutes|h_ours|d_ays|w_eeks"); break;
    }
    // Check if return is 0 then format is invalid
    struct tm *ptm = gmtime(&tmpVal);
    if ((tmpVal != tmpValOld) && (strftime(buf, sizeof(buf), conf.dateTimeFormat, ptm) != 0))
      ret = tcl_result(tcl, FNORMAL, tcl_alloc(buf, strlen(buf)));
    else {
      ret = tcl_result(tcl, FERROR, tcl_alloc("", 0));
      if (tmpVal == 0) TCL_ERROR("Invalid base value");
      if (tmpAdd == 0) TCL_ERROR("Invalid add value");
    }
    // free
    tcl_free(scale);
    tcl_free(add);
    tcl_free(val);
  } else {
    SUBCMDERROR("seconds|format|add");
    ret = tcl_result(tcl, FERROR, tcl_alloc("", 0));
  }

  tcl_free(sub_cmd);
  return ret;
}

/*
 * TCL execution thread
 */
static THD_WORKING_AREA(waTclThread, 2048);
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
               "return value of given node");
  tcl_register(&tcl, "group", tcl_cmd_group, 3, NULL,
               "return value of given group");
  tcl_register(&tcl, "clock", tcl_cmd_clock, 0, NULL,
               "time and date manipulation");

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
      DBG_TCL("Script MB ERROR\r\n");
    }
    chPoolFree(&script_pool, inMsg);
  }

  tcl_destroy(&tcl);
}


#endif /* OHS_TH_TCL_H_ */
