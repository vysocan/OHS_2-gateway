/*
 * ohs_th_alarm.h
 *
 *  Created on: 23. 2. 2020
 *      Author: adam
 */

#ifndef OHS_TH_ALARM_H_
#define OHS_TH_ALARM_H_



/*
 * Alarm event threads
 */
static THD_WORKING_AREA(waAEThread1, 256);
static THD_WORKING_AREA(waAEThread2, 256);
static THD_WORKING_AREA(waAEThread3, 256);
static THD_FUNCTION(AEThread, arg) {
  chRegSetThreadName(arg);
  msg_t msg;
  alarmEvent_t *inMsg;
  uint8_t groupNum, wait, count;

  while (true) {
    msg = chMBFetchTimeout(&alarmEvent_mb, (msg_t*)&inMsg, TIME_INFINITE);
    if (msg == MSG_OK) {
      // Lookup group
      groupNum = GET_CONF_ZONE_GROUP(conf.zone[inMsg->zone]);

      // Group has alarm already nothing to do!
      if (GET_GROUP_ALARM(group[groupNum].setting)) {
        chPoolFree(&alarmEvent_pool, inMsg);
        continue;
      }
      // Set authentication On
      SET_GROUP_WAIT_AUTH(group[groupNum].setting);
      // Set wait time
      if (inMsg->type == 'P') wait = GET_CONF_ZONE_AUTH_TIME(conf.zone[inMsg->zone]);
      else                    wait = 0; // Tamper has no wait time
      //      wait > 0    NOT group has alarm already                authentication On
      while ((wait > 0) && !(GET_GROUP_ALARM(group[groupNum].setting)) && (GET_GROUP_WAIT_AUTH(group[groupNum].setting))) {
        sendCmdToGrp(groupNum, NODE_CMD_ALARM + wait, 'K');
        count = 0;
        //       Authentication On                    time of one alarm period      NOT group has alarm already
        while (GET_GROUP_WAIT_AUTH(group[groupNum].setting) && (count < (10*conf.armDelay)) && !(GET_GROUP_ALARM(group[groupNum].setting))) {
          chThdSleepMilliseconds(100);
          count++;
        }
        //  Authentication On
        if (GET_GROUP_WAIT_AUTH(group[groupNum].setting)) wait--;
      }
      //   wait = 0   NOT group has alarm already
      if ((!wait) && !(GET_GROUP_ALARM(group[groupNum].setting))) {
        SET_GROUP_ALARM(group[groupNum].setting); // Set alarm bit On
        sendCmdToGrp(groupNum, NODE_CMD_ALARM, 'K');
        // Combine alarms, so that next alarm will not disable ongoing one
        if (inMsg->type == 'P') {
          //++OUTs = ((((conf.group[groupNum] >> 4) & B1) | (OUTs >> 0) & B1) | (((conf.group[groupNum] >> 3) & B1) | (OUTs >> 1) & B1) << 1);
        } else {
          //++OUTs = ((((conf.group[groupNum] >> 2) & B1) | (OUTs >> 0) & B1) | (((conf.group[groupNum] >> 1) & B1) | (OUTs >> 1) & B1) << 1);
        }
        // Trigger OUT 1 & 2
        //++pinOUT1.write(((OUTs >> 0) & B1));
        //++pinOUT2.write(((OUTs >> 1) & B1));
        tmpLog[0] = 'S'; tmpLog[1] = 'X';  tmpLog[2] = groupNum;  pushToLog(tmpLog, 3); // ALARM no auth.
        //++publishGroup(groupNum, 'T');
      }
    } else {
      chprintf(console, "%s -> ERROR\r\n", arg);
    }
    chPoolFree(&alarmEvent_pool, inMsg);
  }
}


#endif /* OHS_TH_ALARM_H_ */
