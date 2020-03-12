/*
 * ohs_th_service.h
 *
 *  Created on: 23. 2. 2020
 *      Author: adam
 */

#ifndef OHS_TH_SERVICE_H_
#define OHS_TH_SERVICE_H_


/*
 * Service thread
 * Perform various housekeeping services
 */
static THD_WORKING_AREA(waServiceThread, 256);
static THD_FUNCTION(ServiceThread, arg) {
  chRegSetThreadName(arg);
  time_t tempTime;

  while (true) {
    chThdSleepMilliseconds(1000);
    // Remove zombie nodes
    for (uint8_t nodeIndex=0; nodeIndex < NODE_SIZE; nodeIndex++) {
      if ((node[nodeIndex].address != 0) &&
          (node[nodeIndex].last_OK + SECONDS_PER_HOUR < getTimeUnixSec())) {
        chprintf(console, "Zombie node: %u,A %u,T %u,F %u,N %u\r\n", nodeIndex, node[nodeIndex].address,
                 node[nodeIndex].type, node[nodeIndex].function, node[nodeIndex].number);
        tmpLog[0] = 'N'; tmpLog[1] = 'Z'; tmpLog[2] = node[nodeIndex].address;
        tmpLog[3] = node[nodeIndex].number; tmpLog[4] = node[nodeIndex].type;
        tmpLog[5] = node[nodeIndex].function; pushToLog(tmpLog, 6);
        // Set whole struct to 0
        memset(&node[nodeIndex].address, 0, sizeof(node[0]));
        //0, '\0', '\0', 0, 0b00011110, 0, 0, 255, ""
        //node[nodeIndex].address  = 0;
        //node[nodeIndex].type     = '\0';
        //node[nodeIndex].function = '\0';
        //node[nodeIndex].number   = 0;
        //node[nodeIndex].setting  = 0;
        //node[nodeIndex].value    = 0;
        //node[nodeIndex].last_OK  = 0;
        node[nodeIndex].queue    = 255;
        //memset(&node[nodeIndex].name, 0, NAME_LENGTH);
      }
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
        if ((tempTime != 0) && ((tempTime + (conf.autoArm * SECONDS_PER_MINUTE)) <= getTimeUnixSec())) {
          // Only if group not armed or arming
          if ((!GET_GROUP_ARMED(group[groupNum].setting)) && (group[groupNum].armDelay == 0)) {
            tmpLog[0] = 'G'; tmpLog[1] = 'A'; tmpLog[2] = groupNum; pushToLog(tmpLog, 3);
            armGroup(groupNum, ARM_GROUP_CHAIN_NONE, armAway, 0);

          }
        }
      }
    }

    // Zone open alarm
    for (uint8_t zoneNum=0; zoneNum < ALARM_ZONES ; zoneNum++){
      //   Zone enabled      and   open alarm enabled
      if (GET_CONF_ZONE_ENABLED(conf.zone[zoneNum]) && GET_CONF_ZONE_OPEN_ALARM(conf.zone[zoneNum])){
        if (getTimeUnixSec() >= (zone[zoneNum].lastOK + (conf.openAlarm * SECONDS_PER_MINUTE))) {
          tmpLog[0] = 'Z'; tmpLog[1] = 'O'; tmpLog[2] = zoneNum; pushToLog(tmpLog, 3);
          zone[zoneNum].lastOK = getTimeUnixSec(); // update current timestamp
        }
      }
    }

  }
}


#endif /* OHS_TH_SERVICE_H_ */
