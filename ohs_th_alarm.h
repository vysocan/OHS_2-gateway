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
  uint8_t groupNum, wait;
  uint16_t count;

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
        // Authentication On && time of one alarm period && NOT group has alarm already
        // (count < 8 * (conf.armDelay/4)) -> (count < (uint16_t)(2*conf.armDelay)
        // conf.armDelay must be divided by 4
        while ((GET_GROUP_WAIT_AUTH(group[groupNum].setting)) &&
               (count < (uint16_t)(2*conf.armDelay)) &&
               !(GET_GROUP_ALARM(group[groupNum].setting))) {
          chThdSleepMilliseconds(125);
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
          if (GET_CONF_GROUP_PIR1(conf.group[groupNum].setting)) palSetPad(GPIOB, GPIOB_RELAY_1);
          if (GET_CONF_GROUP_PIR2(conf.group[groupNum].setting)) palSetPad(GPIOB, GPIOB_RELAY_2);
        } else {
          if (GET_CONF_GROUP_TAMPER1(conf.group[groupNum].setting)) palSetPad(GPIOB, GPIOB_RELAY_1);
          if (GET_CONF_GROUP_TAMPER2(conf.group[groupNum].setting)) palSetPad(GPIOB, GPIOB_RELAY_2);
        }
        // TODO OHS create alarms with delays, some countries require not continuous sirens
        tmpLog[0] = 'S'; tmpLog[1] = 'X';  tmpLog[2] = groupNum;  pushToLog(tmpLog, 3); // ALARM no auth.
        // MQTT
        if (GET_CONF_GROUP_MQTT(conf.group[groupNum].setting)) pushToMqtt(typeGroup, groupNum, functionState);
      }
    } else {
      chprintf(console, "%s -> ERROR\r\n", arg);
    }
    chPoolFree(&alarmEvent_pool, inMsg);
  }
}

#endif /* OHS_TH_ALARM_H_ */
