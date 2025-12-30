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
 * Publish topics:
 * MQTT_MAIN_TOPIC - /OHS
 *   /state {On, Off} - Indicates if system is on
 *   /group
 *     /{#} - index of group
 *       /name
 *       /state {disarmed, arming, armed_home, armed_away, triggered, disarming}
 *   /zone
 *     /{#} - index of zone
 *       /name
 *       /state {OK, alarm, tamper}
 *   /sensor
 *     /{address} - node address like W:2:K:i:0
 *       /name
 *       /type
 *       /function
 *       /value
 *
 * MQTT_HAD_MAIN_TOPIC - /homeassistant
 *   /binary_sensor/{UID}/config - System State
 *     - JSON payload with full identifier for Home Assistant MQTT auto discovery.
 *   /alarm_control_panel/{UID-G#}/config - Group(s)
 *     - JSON payload with identifier for specific group # as HA alarm_control_panel.
 *   /binary_sensor/{UID-Z#}/config - Zone(s)
 *     - JSON payload with identifier for specific zone # as HA binary_sensor.
 *
 */
static THD_WORKING_AREA(waMqttPubThread, 1024);
static THD_FUNCTION(MqttPubThread, arg) {
  chRegSetThreadName(arg);
  uint8_t counterMQTT = 225; // Force faster connect on start, 255-30 seconds.
  err_t err; // lwip error type
  msg_t msg;
  mqttPubEvent_t *inMsg;
  uint8_t qos;    // MQTT QoS: at most once(0); At least once (1); Exactly once (2)
  uint8_t retain; // MQTT retain messages: yes(1); no(0)
  char topic[60];

  while (true) {
    msg = chMBFetchTimeout(&mqtt_pub_mb, (msg_t*)&inMsg, TIME_S2I(1));

    // We have message
    if (msg == MSG_OK) {
      DBG_MQTT("MQTT Type: %d", (uint8_t)inMsg->type);
      DBG_MQTT(", Func: %d", (uint8_t)inMsg->function);
      DBG_MQTT(", # %d >> ", inMsg->number);

      LOCK_TCPIP_CORE();
      retain = mqtt_client_is_connected(&mqtt_client); // retain as temp value
      UNLOCK_TCPIP_CORE();
      if (retain) {
        // Wait for free MQTT semaphore
        DBG_MQTT("publish");
        if (chBSemWaitTimeout(&mqttSem, TIME_MS2I(100)) == MSG_OK) {
          // Prepare message
          switch (inMsg->type) {
            case typeSystem:
              qos = 0; retain = 1;
              chsnprintf(topic, sizeof(topic), "%s", MQTT_MAIN_TOPIC);
              switch (inMsg->function) {
                case functionState: // State
                  strncat(topic, &text_state[0], LWIP_MIN(strlen(text_state), (sizeof(topic)-strlen(topic))));
                  // Payload based on state 0/1
                  if (inMsg->number)  chsnprintf(mqttPayload, sizeof(mqttPayload), "%s", text_On);
                  // covered by will // else chsnprintf(mqttPayload, sizeof(mqttPayload), "%s", text_Off);
                  break;
                case functionHAD:
                  chsnprintf(topic, sizeof(topic), "%sbinary_sensor/%s/%s", MQTT_HAD_MAIN_TOPIC, mqttHadUid, MQTT_HAD_CONFIG_TOPIC);
                  // Payload based on state 0/1
                  if (inMsg->extra) {
                    chsnprintf(mqttPayload, sizeof(mqttPayload),
                      "{\"name\":\"%s %s\",\"obj_id\":\"%s\",\"uniq_id\":\"%s\","
                        "\"ent_cat\":\"diagnostic\",\"ic\":\"mdi:security-network\","
                        "\"pl_on\":\"%s\",\"pl_off\":\"%s\",\"stat_t\":\"%s%s\","
                        "\"dev\":{\"name\":\"%s\",\"ids\":\"%s\",\"mf\":\"vysocan\","
                        "\"mdl\":\"Open Home Security\","
                        "\"hw\":\"2.0.x\",\"sw\":\"%u.%u.%u\",\"cu\":\"http://%s\"}}",
                      text_System, text_State, text_state, mqttHadUid, text_On, text_Off, MQTT_MAIN_TOPIC, text_state,
                        OHS_NAME, mqttHadUid, OHS_MAJOR, OHS_MINOR, OHS_MOD, ip4addr_ntoa((ip4_addr_t *)&netInfo.ip));
                  } else {
                    chsnprintf(mqttPayload, sizeof(mqttPayload),""); // Empty payload to remove it
                  }
                  break;
                default:
                  break;
              }
              break;
            case typeGroup:
              qos = 0; retain = 1;
              chsnprintf(topic, sizeof(topic), "%s%s/%d/", MQTT_MAIN_TOPIC, text_group, inMsg->number + 1);
              switch (inMsg->function) {
                case functionName:
                  strncat(topic, &text_name[0], LWIP_MIN(strlen(text_name), (sizeof(topic)-strlen(topic))));
                  chsnprintf(mqttPayload, sizeof(mqttPayload), "%s", conf.group[inMsg->number].name);
                  break;
                case functionHAD:
                  chsnprintf(topic, sizeof(topic), "%salarm_control_panel/%s-G%d/%s",
                             MQTT_HAD_MAIN_TOPIC, mqttHadUid, inMsg->number + 1, MQTT_HAD_CONFIG_TOPIC);
                  if (inMsg->extra) {
                    chsnprintf(mqttPayload, sizeof(mqttPayload),
                      "{\"name\":\"%s: %s\",\"obj_id\":\"%s\",\"uniq_id\":\"%s-G%d\","
                        "\"sup_feat\":[\"arm_home\",\"arm_away\"],\"pl_arm_away\":\"arm_away\","
                        "\"pl_arm_home\":\"arm_home\",\"pl_disarm\":\"disarm\","
                        "\"cod_arm_req\":\"false\",\"stat_t\":\"%sgroup/%d/state\","
                        "\"cmd_t\":\"%sset/group/%d/state\","
                        "\"dev\":{\"ids\":\"%s\"}}",
                      text_Group, conf.group[inMsg->number].name, text_state, mqttHadUid, inMsg->number + 1,
                        MQTT_MAIN_TOPIC, inMsg->number + 1, MQTT_MAIN_TOPIC, inMsg->number + 1,
                        mqttHadUid);
                  } else {
                    chsnprintf(mqttPayload, sizeof(mqttPayload),""); // Empty payload to remove it
                  }
                  break;
                default: // State
                  strncat(topic, &text_state[0], LWIP_MIN(strlen(text_state), (sizeof(topic)-strlen(topic))));
                  // Payload based on state
                  if (GET_GROUP_ALARM(group[inMsg->number].setting) == 0) {
                    if (GET_GROUP_WAIT_AUTH(group[inMsg->number].setting)) {
                      chsnprintf(mqttPayload, sizeof(mqttPayload), "%s", text_disarming);
                    } else {
                      if (GET_GROUP_ARMED(group[inMsg->number].setting)) {
                        if GET_GROUP_ARMED_HOME(group[inMsg->number].setting) {
                          chsnprintf(mqttPayload, sizeof(mqttPayload), "%s_%s", text_armed, text_home);
                        } else {
                          chsnprintf(mqttPayload, sizeof(mqttPayload), "%s_%s", text_armed, text_away);
                        }
                      } else {
                        if (group[inMsg->number].armDelay > 0) {
                          chsnprintf(mqttPayload, sizeof(mqttPayload), "%s", text_arming);
                        } else {
                          chsnprintf(mqttPayload, sizeof(mqttPayload), "%s", text_disarmed);
                        }
                      }
                    }
                  } else {
                    chsnprintf(mqttPayload, sizeof(mqttPayload), "%s", text_triggered); // Alarm
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
                  chsnprintf(mqttPayload, sizeof(mqttPayload), "%s", conf.zoneName[inMsg->number]);
                  break;
                case functionHAD:
                  chsnprintf(topic, sizeof(topic), "%sbinary_sensor/%s-Z%d/%s",
                             MQTT_HAD_MAIN_TOPIC, mqttHadUid, inMsg->number + 1, MQTT_HAD_CONFIG_TOPIC);
                  if (inMsg->extra) {
                    chsnprintf(mqttPayload, sizeof(mqttPayload),
                      "{\"name\":\"%s: %s\",\"obj_id\":\"%s\",\"uniq_id\":\"%s-Z%d\","
                        "\"pl_on\":\"alarm\",\"pl_off\":\"OK\","
                        "\"stat_t\":\"%szone/%d/state\",\"ic\":\"mdi:motion-sensor\","
                        "\"dev\":{\"ids\":\"%s\"}}",
                      text_Zone, conf.zoneName[inMsg->number], text_state, mqttHadUid, inMsg->number + 1,
                        MQTT_MAIN_TOPIC, inMsg->number + 1, mqttHadUid);
                  } else {
                    chsnprintf(mqttPayload, sizeof(mqttPayload),""); // Empty payload to remove it
                  }
                  break;
                default: // State
                  retain = 0;
                  strncat(topic, &text_state[0], LWIP_MIN(strlen(text_state), (sizeof(topic)-strlen(topic))));
                  // Payload based on lastEvent
                  switch (zone[inMsg->number].lastEvent) {
                    case 'O': chsnprintf(mqttPayload, sizeof(mqttPayload), "%s", text_OK);
                      break;
                    case 'P': chsnprintf(mqttPayload, sizeof(mqttPayload), "%s", text_alarm);
                      break;
                    default: chsnprintf(mqttPayload, sizeof(mqttPayload), "%s", text_tamper);
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
                  chsnprintf(mqttPayload, sizeof(mqttPayload), "%s", node[inMsg->number].name);
                  break;
                case functionHAD:
                  chsnprintf(topic, sizeof(topic), "%s%s/%s-%c%u%c%c%u/%s",
                             MQTT_HAD_MAIN_TOPIC, text_sensor, mqttHadUid,
                             (node[inMsg->number].address < RADIO_UNIT_OFFSET) ? 'W' : 'R',
                             (node[inMsg->number].address < RADIO_UNIT_OFFSET) ? node[inMsg->number].address : (node[inMsg->number].address - RADIO_UNIT_OFFSET),
                             node[inMsg->number].type, node[inMsg->number].function,
                             node[inMsg->number].number, MQTT_HAD_CONFIG_TOPIC);
                  if (inMsg->extra) {
                    chsnprintf(mqttPayload, sizeof(mqttPayload),
                      "{\"name\":\"%s: %s\",\"obj_id\":\"%s\","
                        "\"uniq_id\":\"%s-%c%u%c%c%u/%s\","
                        "\"stat_t\":\"%s%s/%c:%u:%c:%c:%u/%s\","
                        "\"dev_cla\":\"%s\","
                        "\"unit_of_meas\":\"%s\"",
                        "\"dev\":{\"ids\":\"%s\"}}",
                        getNodeFunctionString(node[inMsg->number].function), node[inMsg->number].name,
                        text_state, mqttHadUid, inMsg->number + 1,
                        (node[inMsg->number].address < RADIO_UNIT_OFFSET) ? 'W' : 'R',
                        (node[inMsg->number].address < RADIO_UNIT_OFFSET) ? node[inMsg->number].address : (node[inMsg->number].address - RADIO_UNIT_OFFSET),
                        node[inMsg->number].type, node[inMsg->number].function,
                        node[inMsg->number].number,
                        MQTT_MAIN_TOPIC, text_sensor,
                        (node[inMsg->number].address < RADIO_UNIT_OFFSET) ? 'W' : 'R',
                        (node[inMsg->number].address < RADIO_UNIT_OFFSET) ? node[inMsg->number].address : (node[inMsg->number].address - RADIO_UNIT_OFFSET),
                        node[inMsg->number].type, node[inMsg->number].function,
                        node[inMsg->number].number, text_value,
                        getNodeFunctionHAClass(node[inMsg->number].function),
                        getNodeFunctionHAClassUnit(node[inMsg->number].function),
                        mqttHadUid);
                  } else {
                    chsnprintf(mqttPayload, sizeof(mqttPayload),""); // Empty payload to remove it
                  }
                  break;
                default: // Value
                  retain = 0;
                  strncat(topic, &text_value[0], LWIP_MIN(strlen(text_value), (sizeof(topic)-strlen(topic))));
                  // Keys are string rest is float
                  if (node[inMsg->number].type == 'K') {
                    // Check if key index is valid
                    if ((uint8_t)node[inMsg->number].value < KEYS_SIZE) {
                      chsnprintf(mqttPayload, sizeof(mqttPayload), "%s", conf.contact[conf.key[(uint8_t)node[inMsg->number].value].contact].name);
                    } else {
                      chsnprintf(mqttPayload, sizeof(mqttPayload), "%s", text_unknown);
                    }
                  } else {
                    chsnprintf(mqttPayload, sizeof(mqttPayload), "%.2f", node[inMsg->number].value);
                  }
                  break;
              }
              break;
            default:
              DBG_MQTT(" undefined!\r\n");
              break;
          }
          // publish
          LOCK_TCPIP_CORE();
          err = mqtt_publish(&mqtt_client, &topic[0], &mqttPayload[0], strlen(mqttPayload),
                             qos, retain, mqttPubRequestCB, NULL);
          UNLOCK_TCPIP_CORE();
          if(err != ERR_OK) {
            DBG_MQTT(" error: %d\r\n", err);
            // Publish error
            tmpLog[0] = 'Q'; tmpLog[1] = 'E'; tmpLog[2] = 'P'; tmpLog[3] = abs(err); pushToLog(tmpLog, 4);
            // Release semaphore in case of publish error, as CB is not called
            chBSemSignal(&mqttSem);
          } else {
            DBG_MQTT(" OK\r\n");
            CLEAR_CONF_MQTT_SEMAPHORE_ERROR_LOG(conf.mqtt.setting);
          }
        } else {
          DBG_MQTT(" semaphore timeout!\r\n");
          // Log this event
          if (!GET_CONF_MQTT_SEMAPHORE_ERROR_LOG(conf.mqtt.setting)) {
            pushToLogText("QET");
            SET_CONF_MQTT_SEMAPHORE_ERROR_LOG(conf.mqtt.setting);
          }
          // Reset the semaphore to allow next publish
          //chBSemReset(&mqttSem, false);
        }
      } // MQTT client connected
      else {
        DBG_MQTT("not connected\r\n");
      }
      chPoolFree(&mqtt_pub_pool, inMsg);

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
/*
 *
 */
static THD_WORKING_AREA(waMqttSubThread, 512);
static THD_FUNCTION(MqttSubThread, arg) {
  chRegSetThreadName(arg);

  char *pch, *addressIndex[NODE_ADDRESS_SIZE];
  char *payloadStr;
  uint8_t index = 0;
  uint8_t message[6]; // message to node buffer
  msg_t msg;
  mqttSubEvent_t *inMsg;

  while (true) {
    msg = chMBFetchTimeout(&mqtt_sub_mb, (msg_t*)&inMsg, TIME_INFINITE);

    // We have message
    if (msg == MSG_OK) {
      payloadStr = &inMsg->payload[0];
      DBG_MQTT_FUNC("MQTT Sub TH: Topic: %s, Payload: %s\r\n", &inMsg->topic[0], payloadStr);

      // Decode topic string
      pch = strtok (&inMsg->topic[0], "/");
      DBG_MQTT_FUNC ("Parse: %s", pch);
      if (pch != NULL) {
        // Groups
        if (strcmp (pch, text_group) == 0) {
          // Get group index from topic
          pch = strtok (NULL, "/");
          if (pch != NULL) {
            index = strtoul (pch, NULL, 0) - 1;
            if ((index < ALARM_GROUPS)
                && (GET_CONF_GROUP_ENABLED (conf.group[index].setting))
                && (GET_CONF_GROUP_MQTT (conf.group[index].setting))) {
              DBG_MQTT_FUNC (", # %d", index + 1);
              pch = strtok (NULL, "/");
              DBG_MQTT_FUNC (", func: %s", pch);
              if (pch != NULL) {
                // State
                if (strcmp (pch, text_state) == 0) {
                  DBG_MQTT_FUNC (", payload = %s", payloadStr);
                  if (strcmp (payloadStr, text_arm_home) == 0) {
                    armGroup (index, index, armHome, 0);
                  } else if (strcmp (payloadStr, text_arm_away) == 0) {
                    armGroup (index, index, armAway, 0);
                  } else if (strcmp (payloadStr, text_disarm) == 0) {
                    disarmGroup (index, index, 0);
                  }
                }
              }
            }
          }
        } // Groups
        // Sensors
        else if (strcmp (pch, text_sensor) == 0) {
          // Get node address from topic
          pch = strtok (NULL, ":");
          while ((pch != NULL) && (index < NODE_ADDRESS_SIZE)) {
            //DBG_MQTT_FUNC(">%u>%s<\r\n", indexNum, pch);
            addressIndex[index] = pch;
            index++;
            if (index < (NODE_ADDRESS_SIZE - 1)) pch = strtok (NULL, ":");
            else pch = strtok (NULL, "/");
          }
          if (index == NODE_ADDRESS_SIZE) {
            index = getNodeIndex (
                (*addressIndex[0] == 'R' ? RADIO_UNIT_OFFSET : 0)
                    + strtoul (addressIndex[1], NULL, 0), *addressIndex[2],
                *addressIndex[3], strtoul (addressIndex[4], NULL, 0));
            if ((index != DUMMY_NO_VALUE)
                && (GET_NODE_ENABLED (node[index].setting))
                && (GET_NODE_MQTT (node[index].setting))) {
              DBG_MQTT_FUNC (", node index: %d", index);
              if (pch != NULL) {
                DBG_MQTT_FUNC (", topic: %s", pch);
                // Value only for Input nodes
                if ((strcmp (pch, text_value) == 0)
                    && (node[index].type == 'I')) {
                  //DBG_MQTT_FUNC (" = '%.*s'", len, (const char*) payloadStr);
                  floatConv.val = strtof ((const char*) payloadStr, NULL);
                  message[0] = node[index].type;
                  message[1] = node[index].number;
                  message[2] = floatConv.byte[0];
                  message[3] = floatConv.byte[1];
                  message[4] = floatConv.byte[2];
                  message[5] = floatConv.byte[3];
                  DBG_MQTT_FUNC (", message: %c-%d-%.2f", message[0],
                      message[1], floatConv.val);
                  if (sendData (node[index].address, message, 6) == 1) {
                    node[index].lastOK = getTimeUnixSec ();
                    node[index].value = floatConv.val;
                    // MQTT pub
                    if (GET_NODE_MQTT (node[index].setting)) pushToMqtt (
                        typeSensor, index, functionValue);
                  }
                }
              }
            }
          }
        } // Sensors
        // Zones
        else if (strcmp (pch, text_zone) == 0) {
          pch = strtok (NULL, "/");
          if (pch != NULL) {
            // refresh
            if (strcmp (pch, text_refresh) == 0) {
              DBG_MQTT_FUNC (", refresh");
              mqttRefreshZonesState ();
            }
          }
        } // Zones
        else {
          DBG_MQTT_FUNC (" %s", text_unknown);
        }
      } // !NULL
      DBG_MQTT_FUNC (".\r\n");
    }

    chPoolFree(&mqtt_sub_pool, inMsg);
  }
}

#endif /* OHS_TH_MQTT_H_ */
