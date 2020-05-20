/*
 * ohs_th_zone.h
 *
 *  Created on: 23. 2. 2020
 *      Author: adam
 */

#ifndef OHS_TH_ZONE_H_
#define OHS_TH_ZONE_H_

/*
 * Zone thread
 */
static THD_WORKING_AREA(waZoneThread, 256);
static THD_FUNCTION(ZoneThread, arg) {
  chRegSetThreadName(arg);

  msg_t    msg;
  uint16_t val, vBatCounter = 0;
  uint8_t  groupNum = DUMMY_NO_VALUE;

  // Delay to allow PIR to settle up during power on
  chThdSleepSeconds(SECONDS_PER_MINUTE);
  pushToLogText("SS");

  while (true) {
    chThdSleepMilliseconds(250); // time is used also for arm delay and others ...

    // Manage arm delay for each group
    for (uint8_t i=0; i < ALARM_GROUPS ; i++){
      if (group[i].armDelay > 0) {
        group[i].armDelay--;
        if (group[i].armDelay == 0) {
          SET_GROUP_ARMED(group[i].setting); // Arm group
          sendCmdToGrp(i, NODE_CMD_ARMED, 'K');  // Send arm message to all nodes
          //++ publishGroup(i, 'A');
          tmpLog[0] = 'G'; tmpLog[1] = 'S'; tmpLog[2] = i;  pushToLog(tmpLog, 3);
          // Save group state
          writeToBkpRTC((uint8_t*)&group, sizeof(group), 0);
        }
      }
    }

    // RTC VBat
    if (vBatCounter == 0) {
     adcSTM32EnableTSVREFE();     // Enable
     adcSTM32EnableVBATE();       // Enable VBAT pin
    }
    adcConvert(&ADCD1, &adcgrpcfg1, adcSamples, ADC_GRP1_BUF_DEPTH); // Do ADC
    // RTC VBat is measured only on vBatCounter overflow
    if (vBatCounter == 3) {
      adcSTM32DisableVBATE();     // Disable VBAT pin
      adcSTM32DisableTSVREFE();   // Disable
      // VBAT does not measure under 1 V
      if (adcSamples[10] < 700) rtcVbat = 0;
      else rtcVbat = (float)adcSamples[10] * ADC_SCALING_VBAT;
      // Lower or higher then ~ 2.5V
      if ((adcSamples[10] < 1500) && !GET_CONF_SYSTEM_FLAG_RTC_LOW(conf.systemFlags)) {
        pushToLogText("SRL");
        SET_CONF_SYSTEM_FLAG_RTC_LOW(conf.systemFlags);
      }
      if ((adcSamples[10] > 1600) && GET_CONF_SYSTEM_FLAG_RTC_LOW(conf.systemFlags)) {
        pushToLogText("SRH");
        CLEAR_CONF_SYSTEM_FLAG_RTC_LOW(conf.systemFlags);
      }
    }
    vBatCounter++;

    /*
    for(uint8_t i = 0; i < ADC_GRP1_NUM_CHANNELS; i++) {
      chprintf(console, " > %d", adcSamples[i]);
    }
    chprintf(console, "\r\n");
    */

    for(uint8_t i = 0; i < ALARM_ZONES; i++) {
      if (GET_CONF_ZONE_ENABLED(conf.zone[i])){
        // Remote zone
        if (GET_CONF_ZONE_IS_REMOTE(conf.zone[i])) {
          // Switch battery zone back to OK after 2 seconds, as battery nodes may not send OK
          if ((GET_CONF_ZONE_IS_BATTERY(conf.zone[i])) &&
              (zone[i].lastEvent != 'O') && (zone[i].lastPIR + 2 < getTimeUnixSec())) {
            zone[i].lastEvent = 'O';
            zone[i].lastOK = getTimeUnixSec();    // update current timestamp
          }
          switch(zone[i].lastEvent) {
            case 'O': val = ALARM_OK; break;
            case 'P': val = ALARM_PIR; break;
            default:  val = ALARM_TAMPER; break;
          }
        } else { // Local HW
          // Digital 0, Analog 1
          if (GET_CONF_ZONE_TYPE(conf.zone[i])){
            val = adcSamples[i];
            // Force unbalanced for analog zones
            if (!GET_CONF_ZONE_BALANCED(conf.zone[i])){
              if (val < ALARM_UNBALANCED) val = ALARM_OK;
              else                        val = ALARM_PIR;
            }
          } else {
            switch(i) {
              case 10: if (palReadPad(GPIOE, GPIOE_DIN_BOX)) val = ALARM_PIR;
                       else                                  val = ALARM_OK;
              break;
              default: break;
            }
          }
        }

        //    alarm as tamper                            is PIR                                        make it tamper
        if ((GET_CONF_ZONE_PIR_AS_TMP(conf.zone[i])) && (val >= ALARM_PIR_LOW && val <= ALARM_PIR_HI)) val = ALARM_TAMPER;

        // get current zone group
        groupNum = GET_CONF_ZONE_GROUP(conf.zone[i]);

        // Decide zone state
        switch((uint16_t)(val)){
          case ALARM_OK_LOW ... ALARM_OK_HI:
            // Battery node, they will not send OK only PIR and Tamper
            if (GET_CONF_ZONE_IS_BATTERY(conf.zone[i]) != 1) {
              zone[i].lastEvent = 'O';
              zone[i].lastOK = getTimeUnixSec();    // update current timestamp
            }
            break;
          case ALARM_PIR_LOW ... ALARM_PIR_HI:
            //     zone not have alarm                 group delay is 0
            if (!(GET_ZONE_ALARM(zone[i].setting)) && (group[groupNum].armDelay == 0)){
              // if group not enabled log error to log.
              if (!(GET_CONF_GROUP_ENABLED(conf.group[groupNum]))) {
                if (!(GET_GROUP_DISABLED_FLAG(group[groupNum].setting))) {
                  SET_GROUP_DISABLED_FLAG(group[groupNum].setting); // Set logged disabled bit On
                  tmpLog[0] = 'G'; tmpLog[1] = 'F'; tmpLog[2] = groupNum;  pushToLog(tmpLog, 3);
                }
              } else {
                // group armed
                if (GET_GROUP_ARMED(group[groupNum].setting) && (zone[i].lastEvent == 'P')) {
                  // Group is not armed home or is armed gome and also is flaged as home zone
                  if ((!GET_GROUP_ARMED_HOME(group[groupNum].setting)) ||
                      ((GET_GROUP_ARMED_HOME(group[groupNum].setting)) &&
                       (GET_CONF_ZONE_ARM_HOME(conf.zone[i])))) {
                    alarmEvent_t *outMsgA = chPoolAlloc(&alarmEvent_pool);
                    if (outMsgA == NULL) {
                      if (!(GET_ZONE_FULL_FIFO(zone[i].setting))) {
                        tmpLog[0] = 'Z'; tmpLog[1] = 'P'; tmpLog[2] = i;  pushToLog(tmpLog, 3);
                        pushToLogText("FA"); // Alarm queue is full
                      }
                      SET_ZONE_FULL_FIFO(zone[i].setting); // Set On Alarm queue is full
                      continue; // Continue if no free space.
                    }
                    tmpLog[0] = 'Z'; tmpLog[1] = 'P'; tmpLog[2] = i;  pushToLog(tmpLog, 3);
                    SET_ZONE_ALARM(zone[i].setting); // Set alarm bit On
                    CLEAR_ZONE_FULL_FIFO(zone[i].setting); // Set Off Alarm queue is full
                    outMsgA->zone = i; outMsgA->type = zone[i].lastEvent;
                    msg = chMBPostTimeout(&alarmEvent_mb, (msg_t)outMsgA, TIME_IMMEDIATE);
                    if (msg != MSG_OK) pushToLogText("FA"); // Alarm queue is full
                  }
                }
              }
            }
            // Battery node, they will not send OK only PIR and Tamper
            if (GET_CONF_ZONE_IS_BATTERY(conf.zone[i]) != 1) {
              zone[i].lastEvent = 'P';
              zone[i].lastPIR = getTimeUnixSec();    // update current timestamp
            }
            break;
          default: // Line is cut or short or tamper, no difference to alarm event
            //  zone not have alarm
            if (!(GET_ZONE_ALARM(zone[i].setting))){
              // if group not enabled log error to log.
              if (!(GET_CONF_GROUP_ENABLED(conf.group[groupNum]))) {
                if (!(GET_GROUP_DISABLED_FLAG(group[groupNum].setting))) {
                  SET_GROUP_DISABLED_FLAG(group[groupNum].setting); // Set logged disabled bit On
                  tmpLog[0] = 'G'; tmpLog[1] = 'F'; tmpLog[2] = groupNum;  pushToLog(tmpLog, 3);
                }
              } else {
                if (zone[i].lastEvent == 'T') {
                  alarmEvent_t *outMsg = chPoolAlloc(&alarmEvent_pool);
                  if (outMsg == NULL) {
                    if (!(GET_ZONE_FULL_FIFO(zone[i].setting))) {
                      tmpLog[0] = 'Z'; tmpLog[1] = 'T'; tmpLog[2] = i;  pushToLog(tmpLog, 3);
                      pushToLogText("FA"); // Alarm queue is full
                    }
                    SET_ZONE_FULL_FIFO(zone[i].setting); // Set On Alarm queue is full
                    continue; // Continue if no free space.
                  }
                  tmpLog[0] = 'Z'; tmpLog[1] = 'T'; tmpLog[2] = i;  pushToLog(tmpLog, 3);
                  SET_ZONE_ALARM(zone[i].setting); // Set alarm bit On
                  CLEAR_ZONE_FULL_FIFO(zone[i].setting); // Set Off Alarm queue is full
                  outMsg->zone = i; outMsg->type = zone[i].lastEvent;
                  msg = chMBPostTimeout(&alarmEvent_mb, (msg_t)outMsg, TIME_IMMEDIATE);
                  if (msg != MSG_OK) pushToLogText("FA"); // Alarm queue is full
                }
              }
            }
            // Battery node, they will not send OK only PIR and Tamper
            if (GET_CONF_ZONE_IS_BATTERY(conf.zone[i]) != 1) {
              zone[i].lastEvent = 'T';
              zone[i].lastPIR = getTimeUnixSec();    // update current timestamp
            }
            break;
        }
        // Triggers
        sensorEvent_t *outMsgT = chPoolAlloc(&trigger_pool);
        if (outMsgT != NULL) {
          outMsgT->type = 'Z';
          outMsgT->address = 0;
          outMsgT->function = ' ';
          outMsgT->number = i;
          // As defined in zoneState[], 0 = OK
          if (zone[i].lastEvent == 'O') outMsgT->value = 0;
          else if (zone[i].lastEvent == 'P') outMsgT->value = 1;
          else outMsgT->value = 2;
          msg = chMBPostTimeout(&trigger_mb, (msg_t)outMsgT, TIME_IMMEDIATE);
          if (msg != MSG_OK) {
            //chprintf(console, "S-MB full %d\r\n", temp);
          }
        } else {
          pushToLogText("FT"); // Trigger queue is full
        }
      } // zone enabled
    } // for each alarm zone
  } // while true
}


#endif /* OHS_TH_ZONE_H_ */
