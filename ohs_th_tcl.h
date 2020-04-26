/*
 * ohs_th_tcl.h
 *
 *  Created on: 18. 4. 2020
 *      Author: adam
 */

#ifndef OHS_TH_TCL_H_
#define OHS_TH_TCL_H_
/*
 * TCL custom commands
 */
#define TCL_CMD_NODE_ADDRESS_SIZE 5
static int tcl_cmd_node(struct tcl* tcl, tcl_value_t* args, void* arg) {
  (void)arg;
  char *pch, *index[TCL_CMD_NODE_ADDRESS_SIZE];
  uint8_t indexNum = 0;
  int ret;
  char buf[16];

  tcl_value_t* location = tcl_list_at(args, 1);
  //chprintf((BaseSequentialStream*)&SD3, "*tcl_cmd_node*: %s.\r\n", location);

  // Get index
  pch = strtok((char*)location,":");
  while ((pch != NULL) && (indexNum < TCL_CMD_NODE_ADDRESS_SIZE)){
    index[indexNum] = pch;
    pch = strtok(NULL, ":");
    indexNum++;
  }
  if (indexNum == TCL_CMD_NODE_ADDRESS_SIZE) {
    indexNum = getNodeIndex((*index[0] == 'R' ? RADIO_UNIT_OFFSET : 0) + atoi(index[1]),
                            *index[2], *index[3], atoi(index[4]));
    if (indexNum != DUMMY_NO_VALUE) {
      //chprintf((BaseSequentialStream*)&SD3, "*tcl_cmd_node*: %d.\r\n", indexNum);
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
  tcl_free(location);
  return ret;
}

/*
 * TCL execution thread
 */
static THD_WORKING_AREA(waTclThread, 2048);
static THD_FUNCTION(tclThread, arg) {
  chRegSetThreadName(arg);
  msg_t    msg;
  script_t *inMsg;
  systime_t runTime;

  memset(&tclOutput[0], '\0', TCL_SCRIPT_LENGTH);
  MemoryStream ms;
  BaseSequentialStream *chp;
  // Memory stream object to be used as a string writer, reserving one byte for the final zero.
  msObjectInit(&ms, (uint8_t *)tclOutput, TCL_SCRIPT_LENGTH-1, 0);
  // Performing the print operation using the common code.
  chp = (BaseSequentialStream *)(void *)&ms;

  tcl_init(&tcl, conf.tclIteration, chp);
  tcl_register(&tcl, "node", tcl_cmd_node, 2, NULL);

  while (true) {
    msg = chMBFetchTimeout(&script_mb, (msg_t*)&inMsg, TIME_INFINITE);
    if (msg == MSG_OK) {

      // Prepare for run
      memset(&tclOutput[0], '\0', TCL_SCRIPT_LENGTH);
      ms.eos = ms.offset = 0;
      tcl_iteration = conf.tclIteration;
      runTime = chVTGetSystemTimeX();

      if (tcl_eval(&tcl, &tclCmd[0], strlen(tclCmd)) != FERROR) {
        chprintf(chp, "Result: %.*s\r\n", tcl_length(tcl.result), tcl_string(tcl.result));
      } else {
        chprintf(chp, "Script error\r\n");
      }

      chprintf(chp, "Elapsed: %u ms\r\n", TIME_I2MS(chVTGetSystemTimeX() - runTime));

      // Do callback if requested
      if (inMsg->callback) {
        script_cb(inMsg->callback, tcl_string(tcl.result));
      }

      umm_info(&my_umm_heap[0], true);
    } else {
      chprintf(console, "Script MB ERROR\r\n");
    }
    chPoolFree(&script_pool, inMsg);
  }

  tcl_destroy(&tcl);
}


#endif /* OHS_TH_TCL_H_ */
