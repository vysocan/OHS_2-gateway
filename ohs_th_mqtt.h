/*
 * ohs_th_mqtt.h
 *
 *  Created on: Dec 5, 2020
 *      Author: vysocan
 */

#ifndef OHS_TH_MQTT_H_
#define OHS_TH_MQTT_H_

#ifndef OHS_MQTT_DEBUG
#define OHS_MQTT_DEBUG 0
#endif

#if OHS_MQTT_DEBUG
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
static THD_WORKING_AREA(waMqttThread, 832);
static THD_FUNCTION(MqttThread, arg) {
  chRegSetThreadName(arg);
  uint8_t counterMQTT = 225; // Force connect on start 255-30 seconds
  err_t err; // lwip error type
  msg_t msg;
  mqttEvent_t *inMsg;
  uint8_t qos;    // At most once (0); At least once (1); Exactly once (2)
  uint8_t retain; // Retain (1) or not (0)
  char topic[40], payload[20];

  while (true) {
    msg = chMBFetchTimeout(&mqtt_mb, (msg_t*)&inMsg, TIME_S2I(1));

    // We have message
    if (msg == MSG_OK) {
      DBG_MQTT("MQTT Type: %d", (uint8_t)inMsg->type);
      DBG_MQTT(", # %d", inMsg->number);
      DBG_MQTT(", Function: %d\r\n", (uint8_t)inMsg->function);

      LOCK_TCPIP_CORE();
      retain = mqtt_client_is_connected(&mqtt_client); // retain as temp value
      UNLOCK_TCPIP_CORE();
      if (retain) {
        // Wait for free MQTT semaphore
        DBG_MQTT("MQTT ");
        if (chBSemWaitTimeout(&mqttSem, TIME_MS2I(100)) == MSG_OK) {
          DBG_MQTT("publish\r\n");
          // Prepare message
          switch (inMsg->type) {
            case typeSystem:
              qos = 0; retain = 1;
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
              qos = 0; retain = 1;
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
              qos = 0;
              chsnprintf(topic, sizeof(topic), "%s%s/%d/", MQTT_MAIN_TOPIC, text_zone, inMsg->number + 1);
              switch (inMsg->function) {
                case functionName:
                  retain = 1;
                  strncat(topic, &text_name[0], LWIP_MIN(strlen(text_name), (sizeof(topic)-strlen(topic))));
                  chsnprintf(payload, sizeof(payload), "%s", conf.zoneName[inMsg->number]);
                  break;
                default: // State
                  retain = 0;
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
              qos = 0;
              chsnprintf(topic, sizeof(topic), "%s%s/%c:%u:%c:%c:%u/", MQTT_MAIN_TOPIC, text_sensor,
                (node[inMsg->number].address < RADIO_UNIT_OFFSET) ? 'W' : 'R',
                (node[inMsg->number].address < RADIO_UNIT_OFFSET) ? node[inMsg->number].address : (node[inMsg->number].address - RADIO_UNIT_OFFSET),
                node[inMsg->number].type, node[inMsg->number].function,
                node[inMsg->number].number);

              switch (inMsg->function) {
                case functionName:
                  retain = 1;
                  strncat(topic, &text_name[0], LWIP_MIN(strlen(text_name), (sizeof(topic)-strlen(topic))));
                  chsnprintf(payload, sizeof(payload), "%s", node[inMsg->number].name);
                  break;
                default: // Value
                  retain = 0;
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
          LOCK_TCPIP_CORE();
          err = mqtt_publish(&mqtt_client, &topic[0], &payload[0], strlen(payload),
                             qos, retain, mqttPubRequestCB, NULL);
          UNLOCK_TCPIP_CORE();
          if(err != ERR_OK) {
            DBG_MQTT("MQTT publish err: %d\n", err);
            // Publish error
            tmpLog[0] = 'Q'; tmpLog[1] = 'E'; tmpLog[2] = 'P'; tmpLog[3] = abs(err); pushToLog(tmpLog, 4);
          }
        } else {
          DBG_MQTT("MQTT publish semaphore timeout!\r\n");
          pushToLogText("QET");
          // Reset the semaphore to allow next publish
          //chBSemReset(&mqttSem, false);
        }
      } // MQTT client connected
      chPoolFree(&mqtt_pool, inMsg);

    } else if (msg == MSG_TIMEOUT) {
      // Handle MQTT connection
      if (netInfo.status & LWIP_NSC_IPV4_ADDR_VALID) {
        LOCK_TCPIP_CORE();
        retain = mqtt_client_is_connected(&mqtt_client); // retain here as tmp variable
        UNLOCK_TCPIP_CORE();
        if (!retain) {
          // Force try connection every counterMQTT overflow
          if (counterMQTT == 0) CLEAR_CONF_MQTT_CONNECT_ERROR(conf.mqtt.setting);
          // Try to connect if that makes sense
          if (!GET_CONF_MQTT_CONNECT_ERROR(conf.mqtt.setting) &&
              !GET_CONF_MQTT_ADDRESS_ERROR(conf.mqtt.setting)) {
            mqttDoConnect(&mqtt_client);
          }
          counterMQTT++;
        }
      }
    } else {
      DBG_MQTT("MQTT MB ERROR\r\n");
    }

  }
}

#endif /* OHS_TH_MQTT_H_ */
