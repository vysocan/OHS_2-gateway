/*
 * ohs_th_zone.h
 *
 *  Created on: 23. 2. 2020
 *      Author: vysocan
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

  // Manage group MQTT publish
  for (uint8_t i=0; i < ALARM_GROUPS ; i++) {
    if (GET_CONF_GROUP_ENABLED(conf.group[i].setting) && GET_CONF_GROUP_MQTT(conf.group[i].setting)) {
      pushToMqtt(typeGroup, i, functionName);
      pushToMqtt(typeGroup, i, functionState);
    }
  }

  // Delay to allow MQTT to proceed
  chThdSleepSeconds(1);

  // Manage zone MQTT publish
  for (uint8_t i=0; i < ALARM_ZONES ; i++) {
    if (GET_CONF_ZONE_ENABLED(conf.zone[i]) && GET_CONF_ZONE_MQTT_PUB(conf.zone[i])) {
      pushToMqtt(typeZone, i, functionName);
    }
  }

  // Manage PubSuBSerial publish
  for (uint8_t i=0; i < ALARM_ZONES ; i++) {
    pushToPubSub(typeConfZone, i, functionAll);
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
      // Lower or higher then ~ 2.8V, (2.8V / ADC_SCALING_VBAT = 875)
      if ((adcSamples[10] < ADC_VBAT_LOW) &&
          !GET_CONF_SYSTEM_FLAG_RTC_LOW(conf.systemFlags)) {
        pushToLogText("SRL");
        SET_CONF_SYSTEM_FLAG_RTC_LOW(conf.systemFlags);
      }
      // Lower or higher then ~ 2.9V, (2.8V / ADC_SCALING_VBAT = 906)
      if ((adcSamples[10] > ADC_VBAT_HIGH) &&
          GET_CONF_SYSTEM_FLAG_RTC_LOW(conf.systemFlags)) {
        pushToLogText("SRH");
        CLEAR_CONF_SYSTEM_FLAG_RTC_LOW(conf.systemFlags);
      }
    }
    vBatCounter++;

#if ZONE_DEBUG
    for(uint8_t i = 0; i < ADC_GRP1_NUM_CHANNELS; i++) { DBG_ZONE(" > %d", adcSamples[i]); }
    DBG_ZONE("\r\n");
#endif

    for(uint8_t zoneNum = 0; zoneNum < ALARM_ZONES; zoneNum++) {
      if (GET_CONF_ZONE_ENABLED(conf.zone[zoneNum])){
        // Remote zone
        if (zoneNum >= HW_ZONES) {
          // Zone is connected
          if (conf.zoneAddress[zoneNum - HW_ZONES] != 0) {
            // Switch remote balanced zone from PIR back to OK after 2 seconds, as they don't send OK
            if ((GET_CONF_ZONE_BALANCED(conf.zone[zoneNum])) && (zone[zoneNum].lastEvent == 'P') &&
                ((zone[zoneNum].lastPIR + 2) < getTimeUnixSec())) {
              zone[zoneNum].lastEvent = 'O';
              zone[zoneNum].lastOK = getTimeUnixSec();    // update current timestamp
            }
            switch(zone[zoneNum].lastEvent) {
              case 'O': val = ALARM_OK; break;
              case 'P': val = ALARM_PIR; break;
              default:  val = ALARM_TAMPER; break;
            }
          }
        } else { // Local HW
          // Digital 0, Analog 1
          if (GET_CONF_ZONE_TYPE(conf.zone[zoneNum])){
            val = adcSamples[zoneNum];
            // Force unbalanced for analog zones
            if (!GET_CONF_ZONE_BALANCED(conf.zone[zoneNum])){
              if (val < ALARM_UNBALANCED) val = ALARM_OK;
              else                        val = ALARM_PIR;
            }
          } else {
            switch(zoneNum) {
              case 10: if (palReadPad(GPIOE, GPIOE_DIN_BOX)) val = ALARM_PIR;
                       else                                  val = ALARM_OK;
              break;
              default: break;
            }
          }
        }

        //    alarm as tamper                            is PIR                                        make it tamper
        if ((GET_CONF_ZONE_PIR_AS_TMP(conf.zone[zoneNum])) && (val >= ALARM_PIR_LOW && val <= ALARM_PIR_HI)) val = ALARM_TAMPER;

        // get current zone group
        groupNum = GET_CONF_ZONE_GROUP(conf.zone[zoneNum]);

        // Decide zone state
        switch((uint16_t)(val)){
          case ALARM_OK_LOW ... ALARM_OK_HI:
            // All but remote balanced node, they will not send OK only PIR and Tamper
            if (!((zoneNum >= HW_ZONES) && (GET_CONF_ZONE_BALANCED(conf.zone[zoneNum])))) {
              zone[zoneNum].lastEvent = 'O';
              zone[zoneNum].lastOK = getTimeUnixSec();    // update current timestamp
            }
            break;
          case ALARM_PIR_LOW ... ALARM_PIR_HI:
            //     zone not have alarm                 group delay is 0
            if (!(GET_ZONE_ALARM(zone[zoneNum].setting)) && (group[groupNum].armDelay == 0)){
              // if group not enabled log error to log.
              if (!(GET_CONF_GROUP_ENABLED(conf.group[groupNum].setting))) {
                if (!(GET_GROUP_DISABLED_FLAG(group[groupNum].setting)) && (groupNum < ALARM_GROUPS)) {
                  SET_GROUP_DISABLED_FLAG(group[groupNum].setting); // Set logged disabled bit On
                  tmpLog[0] = 'G'; tmpLog[1] = 'F'; tmpLog[2] = groupNum;  pushToLog(tmpLog, 3);
                }
              } else {
                // group armed
                if (GET_GROUP_ARMED(group[groupNum].setting) && (zone[zoneNum].lastEvent == 'P')) {
                  // Group is not armed home or is armed home and also is flagged as home zone
                  if ((!GET_GROUP_ARMED_HOME(group[groupNum].setting)) ||
                      ((GET_GROUP_ARMED_HOME(group[groupNum].setting)) &&
                       (GET_CONF_ZONE_ARM_HOME(conf.zone[zoneNum])))) {
                    alarmEvent_t *outMsgA = chPoolAlloc(&alarmEvent_pool);
                    if (outMsgA == NULL) {
                      if (!(GET_ZONE_FULL_FIFO(zone[zoneNum].setting))) {
                        tmpLog[0] = 'Z'; tmpLog[1] = 'P'; tmpLog[2] = zoneNum;  pushToLog(tmpLog, 3);
                        pushToLogText("FA"); // Alarm queue is full
                      }
                      SET_ZONE_FULL_FIFO(zone[zoneNum].setting); // Set On Alarm queue is full
                      continue; // Continue if no free space.
                    }
                    tmpLog[0] = 'Z'; tmpLog[1] = 'P'; tmpLog[2] = zoneNum;  pushToLog(tmpLog, 3);
                    SET_ZONE_ALARM(zone[zoneNum].setting); // Set alarm bit On
                    CLEAR_ZONE_FULL_FIFO(zone[zoneNum].setting); // Set Off Alarm queue is full
                    outMsgA->zone = zoneNum; outMsgA->type = zone[zoneNum].lastEvent;
                    msg = chMBPostTimeout(&alarmEvent_mb, (msg_t)outMsgA, TIME_IMMEDIATE);
                    if (msg != MSG_OK) pushToLogText("FA"); // Alarm queue is full
                  }
                }
              }
            }
            // All but remote balanced node, they will not send OK only PIR and Tamper
            if (!((zoneNum >= HW_ZONES) && (GET_CONF_ZONE_BALANCED(conf.zone[zoneNum])))) {
              zone[zoneNum].lastEvent = 'P';
              zone[zoneNum].lastPIR = getTimeUnixSec();    // update current timestamp
            }
            break;
          default: // Line is cut or short or tamper, no difference to alarm event
            //  zone not have alarm
            if (!(GET_ZONE_ALARM(zone[zoneNum].setting))){
              // if group not enabled log error to log.
              if (!(GET_CONF_GROUP_ENABLED(conf.group[groupNum].setting))) {
                if (!(GET_GROUP_DISABLED_FLAG(group[groupNum].setting)) && (groupNum < ALARM_GROUPS)) {
                  SET_GROUP_DISABLED_FLAG(group[groupNum].setting); // Set logged disabled bit On
                  tmpLog[0] = 'G'; tmpLog[1] = 'F'; tmpLog[2] = groupNum;  pushToLog(tmpLog, 3);
                }
              } else {
                if (zone[zoneNum].lastEvent == 'T') {
                  alarmEvent_t *outMsg = chPoolAlloc(&alarmEvent_pool);
                  if (outMsg == NULL) {
                    if (!(GET_ZONE_FULL_FIFO(zone[zoneNum].setting))) {
                      tmpLog[0] = 'Z'; tmpLog[1] = 'T'; tmpLog[2] = zoneNum;  pushToLog(tmpLog, 3);
                      pushToLogText("FA"); // Alarm queue is full
                    }
                    SET_ZONE_FULL_FIFO(zone[zoneNum].setting); // Set On Alarm queue is full
                    continue; // Continue if no free space.
                  }
                  tmpLog[0] = 'Z'; tmpLog[1] = 'T'; tmpLog[2] = zoneNum;  pushToLog(tmpLog, 3);
                  SET_ZONE_ALARM(zone[zoneNum].setting); // Set alarm bit On
                  CLEAR_ZONE_FULL_FIFO(zone[zoneNum].setting); // Set Off Alarm queue is full
                  outMsg->zone = zoneNum; outMsg->type = zone[zoneNum].lastEvent;
                  msg = chMBPostTimeout(&alarmEvent_mb, (msg_t)outMsg, TIME_IMMEDIATE);
                  if (msg != MSG_OK) pushToLogText("FA"); // Alarm queue is full
                }
              }
            }
            // All but remote balanced node, they will not send OK only PIR and Tamper
            if (!((zoneNum >= HW_ZONES) && (GET_CONF_ZONE_BALANCED(conf.zone[zoneNum])))) {
              zone[zoneNum].lastEvent = 'T';
              zone[zoneNum].lastPIR = getTimeUnixSec();    // update current timestamp
            }
            break;
        }
        if (zone[zoneNum].eventSent != zone[zoneNum].lastEvent) {
          zone[zoneNum].eventSent = zone[zoneNum].lastEvent;
          // Triggers
          outMsgTrig = chPoolAlloc(&trigger_pool);
          if (outMsgTrig != NULL) {
            outMsgTrig->type = 'Z';
            outMsgTrig->address = 0;
            outMsgTrig->function = ' ';
            outMsgTrig->number = zoneNum;
            // As defined in zoneState[], 0 = OK
            if (zone[zoneNum].lastEvent == 'O') outMsgTrig->value = 0;
            else if (zone[zoneNum].lastEvent == 'P') outMsgTrig->value = 1;
            else outMsgTrig->value = 2;
            msg = chMBPostTimeout(&trigger_mb, (msg_t)outMsgTrig, TIME_IMMEDIATE);
            if (msg != MSG_OK) {
              //DBG_ZONE("S-MB full %d\r\n", temp);
            }
          } else {
            pushToLogText("FT"); // Trigger queue is full
          }
          // MQTT
          if (GET_CONF_ZONE_MQTT_PUB(conf.zone[zoneNum])) pushToMqtt(typeZone, zoneNum, functionState);
          // PubSub
          pushToPubSub(typeZone, zoneNum, functionState);
        }
      } // zone enabled
    } // for each alarm zone
  } // while true
}

#endif /* OHS_TH_ZONE_H_ */
