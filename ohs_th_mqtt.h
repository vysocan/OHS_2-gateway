/*
 * ohs_th_mqtt.h
 *
 *  Created on: Dec 5, 2020
 *      Author: vysocan
 */

#ifndef OHS_TH_MQTT_H_
#define OHS_TH_MQTT_H_

#ifndef MQTT_DEBUG
#define MQTT_DEBUG 0
#endif

#if MQTT_DEBUG
#define DBG_MQTT(...) {chprintf(console, __VA_ARGS__);}
#else
#define DBG_MQTT(...)
#endif
/*
 * MQTT thread
 *
 * Publish topics: MQTT_MAIN_TOPIC
 * /state {On, Off} - Indicates if system is on
 * /group
 *   /{#} - index of group
 *     /name
 *     /state {disarmed, arming, armed_home, armed_away, triggered, disarming}
 * /zone
 *   /{#} - index of zone
 *     /name
 *     /state {OK, alarm, tamper}
 * /sensor
 *   /{address} - node address like W:2:K:i:0
 *     /name
 *     /type
 *     /function
 *     /value
 */
static THD_WORKING_AREA(waMqttThread, 384);
static THD_FUNCTION(MqttThread, arg) {
  chRegSetThreadName(arg);
  err_t err; // lwip error type
  msg_t msg;
  mqttEvent_t *inMsg;
  uint8_t qos;    // At most once (0); At least once (1); Exactly once (2)
  uint8_t retain; // Retain (1) or not (0)
  char topic[40], payload[20];

  while (true) {
    msg = chMBFetchTimeout(&mqtt_mb, (msg_t*)&inMsg, TIME_INFINITE);
    if (msg == MSG_OK) {
      DBG_MQTT("MQTT %d", inMsg);
      DBG_MQTT(", Type: %d", (uint8_t)inMsg->type);
      DBG_MQTT(", # %d", inMsg->number);
      DBG_MQTT(", Function: %d\r\n", (uint8_t)inMsg->function);

      // Wait for free MQTT semaphore
      if (chBSemWaitTimeout(&mqttSem, TIME_MS2I(100)) == MSG_OK) {
        if (mqtt_client_is_connected(&mqtt_client)) {
          // Prepare message
          switch (inMsg->type) {
            case typeSystem:
              qos = 1; retain = 1;
              chsnprintf(topic, sizeof(topic), "%s", MQTT_MAIN_TOPIC);
              switch (inMsg->function) {
                default: // State
                  strncat(topic, &text_state[0], LWIP_MIN(strlen(text_state), (sizeof(topic)-strlen(topic))));
                  // Payload based on state 0/1
                  if (inMsg->number)  chsnprintf(payload, sizeof(payload), "%s", text_On);
                  // covered by will // else chsnprintf(payload, sizeof(payload), "%s", text_Off);
                  break;
              }
              break;
            case typeGroup:
              qos = 1; retain = 0;
              chsnprintf(topic, sizeof(topic), "%s%s/%d/", MQTT_MAIN_TOPIC, text_group, inMsg->number + 1);
              switch (inMsg->function) {
                case functionName:
                  strncat(topic, &text_name[0], LWIP_MIN(strlen(text_name), (sizeof(topic)-strlen(topic))));
                  chsnprintf(payload, sizeof(payload), "%s", conf.group[inMsg->number].name);
                  break;
                default: // State
                  strncat(topic, &text_state[0], LWIP_MIN(strlen(text_state), (sizeof(topic)-strlen(topic))));
                  // Payload based on state
                  if (GET_GROUP_ALARM(group[inMsg->number].setting) == 0) {
                    if (GET_GROUP_WAIT_AUTH(group[inMsg->number].setting)) {
                      chsnprintf(payload, sizeof(payload), "%s", text_disarming);
                    } else {
                      if (GET_GROUP_ARMED(group[inMsg->number].setting)) {
                        if GET_GROUP_ARMED_HOME(group[inMsg->number].setting) {
                          chsnprintf(payload, sizeof(payload), "%s_%s", text_armed, text_home);
                        } else {
                          chsnprintf(payload, sizeof(payload), "%s_%s", text_armed, text_away);
                        }
                      } else {
                        if (group[inMsg->number].armDelay > 0) {
                          chsnprintf(payload, sizeof(payload), "%s", text_arming);
                        } else {
                          chsnprintf(payload, sizeof(payload), "%s", text_disarmed);
                        }
                      }
                    }
                  } else {
                    chsnprintf(payload, sizeof(payload), "%s", text_triggered); // Alarm
                  }
                  break;
              }
              break;
            case typeZone:
              qos = 1; retain = 0;
              chsnprintf(topic, sizeof(topic), "%s%s/%d/", MQTT_MAIN_TOPIC, text_zone, inMsg->number + 1);
              switch (inMsg->function) {
                case functionName:
                  strncat(topic, &text_name[0], LWIP_MIN(strlen(text_name), (sizeof(topic)-strlen(topic))));
                  chsnprintf(payload, sizeof(payload), "%s", conf.zoneName[inMsg->number]);
                  break;
                default: // State
                  strncat(topic, &text_state[0], LWIP_MIN(strlen(text_state), (sizeof(topic)-strlen(topic))));
                  // Payload based on lastEvent
                  switch (zone[inMsg->number].lastEvent) {
                    case 'O': chsnprintf(payload, sizeof(payload), "%s", text_OK);
                      break;
                    case 'P': chsnprintf(payload, sizeof(payload), "%s", text_alarm);
                      break;
                    default: chsnprintf(payload, sizeof(payload), "%s", text_tamper);
                      break;
                  }
                  break;
              }
              break;
            case typeSensor:
              qos = 0; retain = 0;
              chsnprintf(topic, sizeof(topic), "%s%s/%c:%u:%c:%c:%u/", MQTT_MAIN_TOPIC, text_sensor,
                (node[inMsg->number].address < RADIO_UNIT_OFFSET) ? 'W' : 'R',
                (node[inMsg->number].address < RADIO_UNIT_OFFSET) ? node[inMsg->number].address : (node[inMsg->number].address - RADIO_UNIT_OFFSET),
                node[inMsg->number].type, node[inMsg->number].function,
                node[inMsg->number].number);

              switch (inMsg->function) {
                case functionName:
                  strncat(topic, &text_name[0], LWIP_MIN(strlen(text_name), (sizeof(topic)-strlen(topic))));
                  chsnprintf(payload, sizeof(payload), "%s", node[inMsg->number].name);
                  break;
                default: // Value
                  strncat(topic, &text_value[0], LWIP_MIN(strlen(text_value), (sizeof(topic)-strlen(topic))));
                  // Keys are string rest is float
                  if (node[inMsg->number].type == 'K') {
                    chsnprintf(payload, sizeof(payload), "%s", conf.contact[conf.key[(uint8_t)node[inMsg->number].value].contact].name);
                  } else {
                    chsnprintf(payload, sizeof(payload), "%.2f", node[inMsg->number].value);
                  }
                  break;
              }
              break;
            default:
              DBG_MQTT("MQTT publish undefined!\r\n");
              break;
          }
          // publish
          err = mqtt_publish(&mqtt_client, &topic[0], &payload[0], strlen(payload),
                             qos, retain, mqttPubRequestCB, NULL);
          if(err != ERR_OK) {
            DBG_MQTT("MQTT publish err: %d\n", err);
            pushToLogText("QEP"); // Publish error
          }
        } // MQTT client connected
      } else {
        DBG_MQTT("MQTT publish semaphore timeout\r\n");
        // Reset the semaphore to allow next publish
        chBSemReset(&mqttSem, false);
      }
    } else {
      DBG_MQTT("MQTT MB ERROR\r\n");
    }
    chPoolFree(&mqtt_pool, inMsg);
  }
}

#endif /* OHS_TH_MQTT_H_ */
