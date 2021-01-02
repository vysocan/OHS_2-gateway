/*
 * ohs_th_zone.h
 *
 *  Created on: 23. 2. 2020
 *      Author: adam
 */

#ifndef OHS_TH_ZONE_H_
#define OHS_TH_ZONE_H_

#ifndef ZONE_DEBUG
#define ZONE_DEBUG 0
#endif

#if ZONE_DEBUG
#define DBG_ZONE(...) {chprintf(console, __VA_ARGS__);}
#else
#define DBG_ZONE(...)
#endif

/*
 * Zone thread
 */
static THD_WORKING_AREA(waZoneThread, 256);
static THD_FUNCTION(ZoneThread, arg) {
  chRegSetThreadName(arg);

  msg_t    msg;
  uint16_t val, vBatCounter = 0;
  uint8_t  groupNum = DUMMY_NO_VALUE;
  triggerEvent_t *outMsgTrig;

  // Delay to allow PIR to settle up during power up
  chThdSleepSeconds(SECONDS_PER_MINUTE);

  // Monitoring started
  pushToLogText("SS");
  pushToMqtt(typeSystem, 1, functionState);

  // Manage group MQTT publish
  for (uint8_t i=0; i < ALARM_GROUPS ; i++) {
    if (GET_CONF_GROUP_ENABLED(conf.group[i].setting) && GET_CONF_GROUP_MQTT(conf.group[i].setting)) {
      pushToMqtt(typeGroup, i, functionName);
      pushToMqtt(typeGroup, i, functionState);
    }
  }
  // Delay to allow MQTT to proceed
  chThdSleepSeconds(1);
  // Manage group MQTT publish
  for (uint8_t i=0; i < ALARM_ZONES ; i++) {
    if (GET_CONF_ZONE_ENABLED(conf.zone[i]) && GET_CONF_ZONE_MQTT_PUB(conf.zone[i])) {
      pushToMqtt(typeZone, i, functionName);
    }
  }

  while (true) {
    chThdSleepMilliseconds(250); // time is used also for arm delay and others ...

    // Manage arm delay for each group
    for (uint8_t i=0; i < ALARM_GROUPS ; i++) {
      if (group[i].armDelay > 0) {
        group[i].armDelay--;
        if (group[i].armDelay == 0) {
          SET_GROUP_ARMED(group[i].setting); // Arm group
          sendCmdToGrp(i, NODE_CMD_ARMED, 'K');  // Send arm message to all nodes
          tmpLog[0] = 'G'; tmpLog[1] = 'S'; tmpLog[2] = i;  pushToLog(tmpLog, 3);
          // Save group state
          writeToBkpRTC((uint8_t*)&group, sizeof(group), 0);
          if (GET_CONF_GROUP_MQTT(conf.group[i].setting)) pushToMqtt(typeGroup, i, functionState);
        }
      }
    }

    // RTC VBat
    if (vBatCounter == 0) {
      adcSTM32EnableVBATE(); // Enable VBAT pin
    }
    adcConvert(&ADCD1, &adcgrpcfg1, adcSamples, ADC_GRP1_BUF_DEPTH); // Do ADC
    // RTC VBat is measured only on vBatCounter overflow.
    // ADC thresholds valid for 1/4 scaling factor of STM32F437.
    if (vBatCounter == 3) {
      adcSTM32DisableVBATE(); // Disable VBAT pin
      // VBAT does not measure under 1V, (1V / ADC_SCALING_VBAT = 310)
      if (adcSamples[10] < 310) rtcVbat = 0;
      else rtcVbat = (float)adcSamples[10] * ADC_SCALING_VBAT;
      // Lower or higher then ~ 2.5V, (2.5V / ADC_SCALING_VBAT = 775)
      if ((adcSamples[10] < 775) && !GET_CONF_SYSTEM_FLAG_RTC_LOW(conf.systemFlags)) {
        pushToLogText("SRL");
        SET_CONF_SYSTEM_FLAG_RTC_LOW(conf.systemFlags);
      }
      // Lower or higher then ~ 2.8V, (2.8V / ADC_SCALING_VBAT = 870)
      if ((adcSamples[10] > 870) && GET_CONF_SYSTEM_FLAG_RTC_LOW(conf.systemFlags)) {
        pushToLogText("SRH");
        CLEAR_CONF_SYSTEM_FLAG_RTC_LOW(conf.systemFlags);
      }
    }
    vBatCounter++;

#if ZONE_DEBUG
    for(uint8_t i = 0; i < ADC_GRP1_NUM_CHANNELS; i++) { DBG_ZONE(" > %d", adcSamples[i]); }
    DBG_ZONE("\r\n");
#endif

    for(uint8_t i = 0; i < ALARM_ZONES; i++) {
      if (GET_CONF_ZONE_ENABLED(conf.zone[i])){
        // Remote zone
        if (i > HW_ZONES) {
          // Switch remote balanced zone back to OK after 2 seconds, as don't send OK
          if ((i > HW_ZONES) && (GET_CONF_ZONE_BALANCED(conf.zone[i])) &&
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
            // All but remote balanced node, they will not send OK only PIR and Tamper
            if (!((i > HW_ZONES) && (GET_CONF_ZONE_BALANCED(conf.zone[i])))) {
              zone[i].lastEvent = 'O';
              zone[i].lastOK = getTimeUnixSec();    // update current timestamp
            }
            break;
          case ALARM_PIR_LOW ... ALARM_PIR_HI:
            //     zone not have alarm                 group delay is 0
            if (!(GET_ZONE_ALARM(zone[i].setting)) && (group[groupNum].armDelay == 0)){
              // if group not enabled log error to log.
              if (!(GET_CONF_GROUP_ENABLED(conf.group[groupNum].setting))) {
                if (!(GET_GROUP_DISABLED_FLAG(group[groupNum].setting)) && (groupNum < ALARM_GROUPS)) {
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
            // All but remote balanced node, they will not send OK only PIR and Tamper
            if (!((i > HW_ZONES) && (GET_CONF_ZONE_BALANCED(conf.zone[i])))) {
              zone[i].lastEvent = 'P';
              zone[i].lastPIR = getTimeUnixSec();    // update current timestamp
            }
            break;
          default: // Line is cut or short or tamper, no difference to alarm event
            //  zone not have alarm
            if (!(GET_ZONE_ALARM(zone[i].setting))){
              // if group not enabled log error to log.
              if (!(GET_CONF_GROUP_ENABLED(conf.group[groupNum].setting))) {
                if (!(GET_GROUP_DISABLED_FLAG(group[groupNum].setting)) && (groupNum < ALARM_GROUPS)) {
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
            // All but remote balanced node, they will not send OK only PIR and Tamper
            if (!((i > HW_ZONES) && (GET_CONF_ZONE_BALANCED(conf.zone[i])))) {
              zone[i].lastEvent = 'T';
              zone[i].lastPIR = getTimeUnixSec();    // update current timestamp
            }
            break;
        }
        if (zone[i].eventSent != zone[i].lastEvent) {
          zone[i].eventSent = zone[i].lastEvent;
          // Triggers
          outMsgTrig = chPoolAlloc(&trigger_pool);
          if (outMsgTrig != NULL) {
            outMsgTrig->type = 'Z';
            outMsgTrig->address = 0;
            outMsgTrig->function = ' ';
            outMsgTrig->number = i;
            // As defined in zoneState[], 0 = OK
            if (zone[i].lastEvent == 'O') outMsgTrig->value = 0;
            else if (zone[i].lastEvent == 'P') outMsgTrig->value = 1;
            else outMsgTrig->value = 2;
            msg = chMBPostTimeout(&trigger_mb, (msg_t)outMsgTrig, TIME_IMMEDIATE);
            if (msg != MSG_OK) {
              //DBG_ZONE("S-MB full %d\r\n", temp);
            }
          } else {
            pushToLogText("FT"); // Trigger queue is full
          }
          // MQTT
          if (GET_CONF_ZONE_MQTT_PUB(conf.zone[i])) pushToMqtt(typeZone, i, functionState);
        }
      } // zone enabled
    } // for each alarm zone
  } // while true
}

#endif /* OHS_TH_ZONE_H_ */
