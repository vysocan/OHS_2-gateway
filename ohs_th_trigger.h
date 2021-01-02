/*
 * ohs_thi.h
 *
 *  Created on: 15. 5. 2020
 *      Author: adam
 */

#ifndef OHS_THi_H_
#define OHS_THi_H_

#ifndef TRIGGER_DEBUG
#define TRIGGER_DEBUG 1
#endif

#if TRIGGER_DEBUG
#define DBG_TRIG(...) {chprintf(console, __VA_ARGS__);}
#else
#define DBG_TRIG(...)
#endif

/*
 * Callback for Timer TCL script
 */
void cbTrigger (char *result) {
  (void)result;
  // Result is ready
  chBSemSignal(&cbTriggerSem);
}
/*
 * Trigger thread
 */
static THD_WORKING_AREA(waTriggerThread, 320);
static THD_FUNCTION(TriggerThread, arg) {
  chRegSetThreadName(arg);
  sensorEvent_t *inMsg;
  msg_t    msg;
  float    tmpFloat;
  uint8_t  nodeIndex, found, message[6];
  uint32_t add;
  time_t   timeNow;
  char    *pResult;
  struct scriptLL_t *scrP = NULL;

  while (true) {
    msg = chMBFetchTimeout(&trigger_mb, (msg_t*)&inMsg, TIME_INFINITE);
    timeNow = getTimeUnixSec();

    if (msg == MSG_OK) {
      // loop through triggers
      for (uint8_t i = 0; i < TRIGGER_SIZE; i++) {
        //  trigger enabled
        if (GET_CONF_TRIGGER_ENABLED(conf.trigger[i].setting)) {
          if (conf.trigger[i].type     == inMsg->type &&
              conf.trigger[i].address  == inMsg->address &&
              conf.trigger[i].function == inMsg->function &&
              conf.trigger[i].number   == inMsg->number) {
            DBG_TRIG("Trigger: %u", i);
            // check value
            found = 0;
            switch(conf.trigger[i].condition){
              //case 0: found = 1; break; // Always
              case 1: if (inMsg->value == conf.trigger[i].value) found = 1; break;
              case 2: if (inMsg->value != conf.trigger[i].value) found = 1; break;
              case 3:
                if (GET_CONF_TRIGGER_TRIGGERED(conf.trigger[i].setting)) {
                  tmpFloat = conf.trigger[i].value + conf.trigger[i].hysteresis;
                } else {
                  tmpFloat = conf.trigger[i].value - conf.trigger[i].hysteresis;
                }
                if (inMsg->value < tmpFloat) found = 1;
                //WS.print(F("Trigger ")); WS.print(inMsg->value); WS.print(F(" < ")); WS.println(tmpFloat);
                break;
              case 4:
                if (GET_CONF_TRIGGER_TRIGGERED(conf.trigger[i].setting)) {
                  tmpFloat = conf.trigger[i].value - conf.trigger[i].hysteresis;
                } else {
                  tmpFloat = conf.trigger[i].value + conf.trigger[i].hysteresis;
                }
                if (inMsg->value > tmpFloat) found = 1;
                break;
              default: found = 1; break; // Always
            }
            DBG_TRIG(" , cond: %s, y/n: %u", triggerCondition[conf.trigger[i].condition], found);
            if (found) {
              // Logging enabled & not triggered
              if ((GET_CONF_TRIGGER_ALERT(conf.trigger[i].setting)) &&
                  (!GET_CONF_TRIGGER_TRIGGERED(conf.trigger[i].setting))) {
                tmpLog[0] = 'R'; tmpLog[1] = 'A'; tmpLog[2] = i; pushToLog(tmpLog, 3);
              }
              SET_CONF_TRIGGER_TRIGGERED(conf.trigger[i].setting);
              // Script evaluation
              if (conf.trigger[i].evalScript[0] != 0) {
                CLEAR_CONF_TRIGGER_RESULT(conf.trigger[i].setting); // Force not evaluated
                scriptEvent_t *outMsg = chPoolAlloc(&script_pool);
                if (outMsg != NULL) {
                  // Find pointer to script
                  for (scrP = scriptLL; scrP != NULL; scrP = scrP->next) {
                    if (strcmp(scrP->name, &conf.trigger[i].evalScript[0]) == 0) break;
                  }
                  // Script name exists
                  if (scrP != NULL) {
                    pResult = (char *)&message; // Just any char[] as temp variable
                    outMsg->callback = cbTrigger;
                    outMsg->result = (void **)&pResult;
                    outMsg->flags = 1;
                    outMsg->cmdP = scrP->cmd;
                    // Reset semaphore
                    chBSemReset(&cbTriggerSem, true);
                    // Run script
                    msg_t msg = chMBPostTimeout(&script_mb, (msg_t)outMsg, TIME_IMMEDIATE);
                    if (msg != MSG_OK) {
                      //DBG_SERVICE("MB full %d\r\n", temp);
                    }
                    // Wait for result
                    if (chBSemWaitTimeout(&cbTriggerSem, TIME_MS2I(300)) == MSG_OK) {
                      if (strtoul(pResult, NULL, 0) > 0) SET_CONF_TRIGGER_RESULT(conf.trigger[i].setting);
                    }
                  }
                } else {
                  DBG_SERVICE("CB full %d \r\n", outMsg);
                }
              }
              // Pass
              if ((GET_CONF_TRIGGER_PASS(conf.trigger[i].setting)) &&
                  ((GET_CONF_TRIGGER_RESULT(conf.trigger[i].setting)) || (conf.trigger[i].evalScript[0] == 0))) {
                // Select input node to update its value and timestamp
                nodeIndex = getNodeIndex(conf.trigger[i].toAddress, 'I', conf.trigger[i].toFunction, conf.trigger[i].toNumber);
                if (nodeIndex != DUMMY_NO_VALUE) {
                  DBG_TRIG(" , pass to node index: %u", nodeIndex + 1);
                  //  Pass off,  time
                  if (GET_CONF_TRIGGER_PASS_OFF(conf.trigger[i].setting) == 2) {
                    switch(GET_CONF_TRIGGER_OFF_PERIOD(conf.trigger[i].setting)) {
                      case 0:  add = (uint32_t)conf.trigger[i].offTime; break;
                      case 1:  add = (uint32_t)conf.trigger[i].offTime * SECONDS_PER_MINUTE; break;
                      case 2:  add = (uint32_t)conf.trigger[i].offTime * SECONDS_PER_HOUR; break;
                      default: add = (uint32_t)conf.trigger[i].offTime * SECONDS_PER_DAY; break;
                    }
                    conf.trigger[i].nextOff = timeNow + add;
                    DBG_TRIG(" , nextOff at: %u", conf.trigger[i].nextOff);
                  }
                  // Not passed
                  if (!GET_CONF_TRIGGER_PASSED(conf.trigger[i].setting)) {
                    message[0] = 'I'; // 'I'nput only
                    message[1] = conf.trigger[i].toNumber;
                    if (GET_CONF_TRIGGER_PASS_VALUE(conf.trigger[i].setting)) {
                      floatConv.val = inMsg->value;
                    } else {
                      floatConv.val = conf.trigger[i].constantOn;
                    }
                    message[2] = floatConv.byte[0]; message[3] = floatConv.byte[1];
                    message[4] = floatConv.byte[2]; message[5] = floatConv.byte[3];
                    if (sendData(conf.trigger[i].toAddress, message, 6)) {
                      node[nodeIndex].lastOK = timeNow; // update receiving node current timestamp
                      // update receiving node value
                      if (GET_CONF_TRIGGER_PASS_VALUE(conf.trigger[i].setting)) {
                        node[nodeIndex].value = inMsg->value;
                      } else {
                        node[nodeIndex].value = conf.trigger[i].constantOn;
                      }
                      // Pass only once
                      if (GET_CONF_TRIGGER_PASS(conf.trigger[i].setting) > 1) {
                        SET_CONF_TRIGGER_PASSED(conf.trigger[i].setting);
                      }
                      // MQTT
                      if (GET_NODE_MQTT(node[nodeIndex].setting)) pushToMqtt(typeSensor, nodeIndex, functionValue);
                    }
                  }
                } // Node found
              }
            } else { // Off trigger
              // Logging enabled & triggered
              if ((GET_CONF_TRIGGER_ALERT(conf.trigger[i].setting)) &&
                  (GET_CONF_TRIGGER_TRIGGERED(conf.trigger[i].setting))) {
                tmpLog[0] = 'R'; tmpLog[1] = 'D'; tmpLog[2] = i; pushToLog(tmpLog, 3);
              }
              if ((GET_CONF_TRIGGER_PASS_OFF(conf.trigger[i].setting) == 1) &&
                  (GET_CONF_TRIGGER_TRIGGERED(conf.trigger[i].setting))) {
                // Select input node to update its value and timestamp
                nodeIndex = getNodeIndex(inMsg->address, 'I', inMsg->function, inMsg->number);
                if (nodeIndex != DUMMY_NO_VALUE) {
                  // Pass Off value
                  message[0] = 'I'; // 'I'nput only
                  message[1] = conf.trigger[i].toNumber;
                  if (GET_CONF_TRIGGER_PASS_VALUE(conf.trigger[i].setting)) {
                    floatConv.val = inMsg->value;
                  } else {
                    floatConv.val = conf.trigger[i].constantOff;
                  }
                  message[2] = floatConv.byte[0]; message[3] = floatConv.byte[1];
                  message[4] = floatConv.byte[2]; message[5] = floatConv.byte[3];
                  if (sendData(conf.trigger[i].toAddress, message, 6)) {
                    node[nodeIndex].lastOK = timeNow; // update receiving node current timestamp
                    // update receiving node value
                    // update receiving node value
                    if (GET_CONF_TRIGGER_PASS_VALUE(conf.trigger[i].setting)) {
                      node[nodeIndex].value = inMsg->value;
                    } else {
                      node[nodeIndex].value = conf.trigger[i].constantOff;
                    }
                    CLEAR_CONF_TRIGGER_TRIGGERED(conf.trigger[i].setting);
                    CLEAR_CONF_TRIGGER_PASSED(conf.trigger[i].setting);
                    // MQTT
                    if (GET_NODE_MQTT(node[nodeIndex].setting)) pushToMqtt(typeSensor, nodeIndex, functionValue);
                  }
                } //found
              } // if
              // Pass Off = NO, but alerted/triggered
              if ((GET_CONF_TRIGGER_PASS_OFF(conf.trigger[i].setting) == 0) &&
                  (GET_CONF_TRIGGER_TRIGGERED(conf.trigger[i].setting))) {
                // Turn OFF trigger switches
                CLEAR_CONF_TRIGGER_TRIGGERED(conf.trigger[i].setting);
                CLEAR_CONF_TRIGGER_PASSED(conf.trigger[i].setting);
              }
            } // else
            DBG_TRIG("\r\n");
          } // trigger match
        } // trigger enabled
      } // for (trigger)
    }else {
      DBG_TRIG("Trigger ERROR\r\n");
    }
    chPoolFree(&trigger_pool, inMsg);
  }
}

#endif /* OHS_THi_H_ */
