/*
 * ohs_th_service.h
 *
 *  Created on: 23. 2. 2020
 *      Author: adam
 */

#ifndef OHS_TH_SERVICE_H_
#define OHS_TH_SERVICE_H_

#ifndef SERVICE_DEBUG
#define SERVICE_DEBUG 0
#endif

#if SERVICE_DEBUG
#define DBG_SERVICE(...) {chprintf((BaseSequentialStream*)&SD3, __VA_ARGS__);}
#else
#define DBG_SERVICE(...)
#endif

/*
 * Service thread
 * Perform various housekeeping services
 */
static THD_WORKING_AREA(waServiceThread, 384);
static THD_FUNCTION(ServiceThread, arg) {
  chRegSetThreadName(arg);
  time_t  tempTime, timeNow;
  uint8_t counterAC = 0, nodeIndex;
  int8_t  resp;
  uint8_t message[6];

  while (true) {
    chThdSleepMilliseconds(1000);
    // Get current time
    timeNow = getTimeUnixSec();

    // Remove zombie nodes
    for (uint8_t nodeIndex=0; nodeIndex < NODE_SIZE; nodeIndex++) {
      if ((node[nodeIndex].address != 0) &&
          (node[nodeIndex].last_OK + SECONDS_PER_HOUR < timeNow)) {
        DBG_SERVICE("Zombie node: %u,A %u,T %u,F %u,N %u\r\n", nodeIndex, node[nodeIndex].address,
                 node[nodeIndex].type, node[nodeIndex].function, node[nodeIndex].number);
        tmpLog[0] = 'N'; tmpLog[1] = 'Z'; tmpLog[2] = node[nodeIndex].address;
        tmpLog[3] = node[nodeIndex].type; tmpLog[4] = node[nodeIndex].function;
        tmpLog[5] = node[nodeIndex].number; pushToLog(tmpLog, 6);
        // Set whole struct to 0
        memset(&node[nodeIndex].address, 0, sizeof(node[0]));
        //0, '\0', '\0', 0, 0b00011110, 0, 0, DUMMY_NO_VALUE, ""
        //node[nodeIndex].address  = 0;
        //node[nodeIndex].type     = '\0';
        //node[nodeIndex].function = '\0';
        //node[nodeIndex].number   = 0;
        //node[nodeIndex].setting  = 0;
        //node[nodeIndex].value    = 0;
        //node[nodeIndex].last_OK  = 0;
        node[nodeIndex].queue    = DUMMY_NO_VALUE;
        //memset(&node[nodeIndex].name, 0, NAME_LENGTH);
      }
    }


    // Battery check - The signal is "Low" when the voltage of battery is under 11V
    if (palReadPad(GPIOD, GPIOD_BAT_OK) == 0) {
      pushToLogText("SBL"); // Battery low
      pushToLogText("SCP"); // Configuration saved
      // Wait for alert mb to be empty
      do {
        chThdSleepMilliseconds(100);
      } while (chMBGetFreeCountI(&alert_mb) != ALERT_FIFO_SIZE);
      // Backup
      writeToBkpSRAM((uint8_t*)&conf, sizeof(config_t), 0);
      writeToBkpRTC((uint8_t*)&group, sizeof(group), 0);
      // Lock RTOS
      chSysLock();
      // Battery is at low level but it might oscillate, so we wait for AC start
      while (palReadPad(GPIOD, GPIOD_AC_OFF)) { // The signal turns to be "High" when the power supply turns OFF
        // do nothing, wait for power supply shutdown
      }
      // Power is restored we go on
      chSysUnlock();
      pushToLogText("SBH"); // Battery high
    }

    // AC power check - The signal turns to be "High" when the power supply turns OFF
    resp = palReadPad(GPIOD, GPIOD_AC_OFF);
    if (!resp && (counterAC > 1)) counterAC--;
    if (!resp && (counterAC == 1)) {
      counterAC--;
      pushToLogText("SAH"); // AC ON
    }
    if (resp && (counterAC < AC_POWER_DELAY)) counterAC++;
    if (resp && (counterAC == AC_POWER_DELAY)) {
      counterAC++;
      pushToLogText("SAL"); // AC OFF
    }

    // Group auto arm
    for (uint8_t groupNum=0; groupNum < ALARM_GROUPS ; groupNum++){
      if (GET_CONF_GROUP_ENABLED(conf.group[groupNum]) && GET_CONF_GROUP_AUTO_ARM(conf.group[groupNum])) {
        tempTime = 0;
        // List through zones
        for (uint8_t zoneNum=0; zoneNum < ALARM_ZONES ; zoneNum++){
          if (GET_CONF_ZONE_ENABLED(conf.zone[zoneNum]) && GET_CONF_ZONE_GROUP(conf.zone[zoneNum])==groupNum){
            // Get latest PIR
            if (zone[zoneNum].lastPIR > tempTime) {
              tempTime = zone[zoneNum].lastPIR;
            }
          }
        }
        // Group has at least one zone && time has passed
        if ((tempTime != 0) && ((tempTime + (conf.autoArm * SECONDS_PER_MINUTE)) <= timeNow)) {
          // Only if group not armed or arming
          if ((!GET_GROUP_ARMED(group[groupNum].setting)) && (group[groupNum].armDelay == 0)) {
            tmpLog[0] = 'G'; tmpLog[1] = 'A'; tmpLog[2] = groupNum; pushToLog(tmpLog, 3);
            armGroup(groupNum, DUMMY_NO_VALUE, armAway, 0);
          }
        }
      }
    }

    // Zone open alarm
    for (uint8_t zoneNum=0; zoneNum < ALARM_ZONES ; zoneNum++){
      //   Zone enabled      and   open alarm enabled
      if (GET_CONF_ZONE_ENABLED(conf.zone[zoneNum]) && GET_CONF_ZONE_OPEN_ALARM(conf.zone[zoneNum])){
        if (timeNow >= (zone[zoneNum].lastOK + (conf.openAlarm * SECONDS_PER_MINUTE))) {
          tmpLog[0] = 'Z'; tmpLog[1] = 'O'; tmpLog[2] = zoneNum; pushToLog(tmpLog, 3);
          zone[zoneNum].lastOK = timeNow; // update current timestamp
        }
      }
    }

    // Timers
    for (uint8_t i=0; i < TIMER_SIZE; i++){
      //   Timer enabled                                   timer next on is set
      if (GET_CONF_TIMER_ENABLED(conf.timer[i].setting) && conf.timer[i].nextOn > 0) {
        //   Start time has passed               NOT triggered yet
        if ((timeNow >= conf.timer[i].nextOn) && (!GET_CONF_TIMER_TRIGGERED(conf.timer[i].setting))) {
          //
          if (conf.timer[i].evalScript != DUMMY_NO_VALUE) {
            script_t *outMsg = chPoolAlloc(&script_pool);
            if (outMsg != NULL) {
              outMsg->callback = NULL;
              outMsg->result = NULL;
              outMsg->flags = 1;
              outMsg->index = 0;
              msg_t msg = chMBPostTimeout(&script_mb, (msg_t)outMsg, TIME_IMMEDIATE);
              if (msg != MSG_OK) {
                //DBG_SERVICE("MB full %d\r\n", temp);
              }
            } else {
              DBG_SERVICE("CB full %d \r\n", outMsg);
            }
          }
          nodeIndex = getNodeIndex(conf.timer[i].toAddress, 'I', conf.timer[i].toFunction, conf.timer[i].toNumber);
          if (nodeIndex != DUMMY_NO_VALUE) {
            message[0] = 'I'; // 'I'nput only
            message[1] = conf.timer[i].toNumber;
            floatConv.val = conf.timer[i].constantOn;
            message[2] = floatConv.byte[0]; message[3] = floatConv.byte[1];
            message[4] = floatConv.byte[2]; message[5] = floatConv.byte[3];
            if (sendData(conf.timer[i].toAddress, message, 6) == 1) {
              SET_CONF_TIMER_TRIGGERED(conf.timer[i].setting);
              node[nodeIndex].last_OK = timeNow; // update receiving node current timestamp
              node[nodeIndex].value   = conf.timer[i].constantOn; // update receiving node value
              // *** publishNode(_update_node); // MQTT
            }
          }
        }
        //  End time has passed
        if (timeNow >= conf.timer[i].nextOff) {
          nodeIndex = getNodeIndex(conf.timer[i].toAddress, 'I', conf.timer[i].toFunction, conf.timer[i].toNumber);
          if (nodeIndex != DUMMY_NO_VALUE) {
            message[0] = 'I'; // 'I'nput only
            message[1] = conf.timer[i].toNumber;
            floatConv.val = conf.timer[i].constantOff;
            message[2] = floatConv.byte[0]; message[3] = floatConv.byte[1];
            message[4] = floatConv.byte[2]; message[5] = floatConv.byte[3];
            if (sendData(conf.timer[i].toAddress, message, 6) == 1) {
              SET_CONF_TIMER_TRIGGERED(conf.timer[i].setting);
              node[nodeIndex].last_OK = timeNow; // update receiving node current timestamp
              node[nodeIndex].value   = conf.timer[i].constantOff; // update receiving node value
              // *** publishNode(_update_node); // MQTT
            }
          }
          setTimer(i, false); // set next start time for this timer even if not triggered
        }
      }
    }

    //

  }
}


#endif /* OHS_TH_SERVICE_H_ */
