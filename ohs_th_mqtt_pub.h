/*
 * ohs_th_mqtt.h
 *
 *  Created on: Dec 5, 2020
 *      Author: vysocan
 */

#ifndef OHS_TH_MQTT_PUB_H_
#define OHS_TH_MQTT_PUB_H_

#ifndef OHS_MQTT_PUB_DEBUG
#define OHS_MQTT_PUB_DEBUG 0
#endif

#if OHS_MQTT_PUB_DEBUG
#define DBG_MQTT_PUB(...) {chprintf(console, __VA_ARGS__);}
#else
#define DBG_MQTT_PUB(...)
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
/**
 * Handle typeSystem publish events
 * @param inMsg Message pointer
 * @param topic Output topic buffer
 * @param topic_size Topic buffer size
 * @param payload Output payload buffer
 * @param payload_size Payload buffer size
 * @return qos, retain set by reference
 */
static void handleMqttPubSystem(mqttPubEvent_t *inMsg, char *topic,
    size_t topic_size, char *payload, size_t payload_size, uint8_t *qos,
    uint8_t *retain) {
  *qos = 0;
  *retain = 1;

  chsnprintf(topic, topic_size, "%s", MQTT_MAIN_TOPIC);

  switch (inMsg->function) {
    case functionState:
      // Append "/state" to topic
      strncat (topic, &text_state[0],
          LWIP_MIN (strlen (text_state), (topic_size - strlen (topic) - 1)));

      if (inMsg->number) {
        chsnprintf (payload, payload_size, "%s", text_On);
      }
      // else: covered by will message, leave empty
      break;

    case functionHAD:
      chsnprintf (topic, topic_size, "%sbinary_sensor/%s/%s",
          MQTT_HAD_MAIN_TOPIC, mqttHadUid, MQTT_HAD_CONFIG_TOPIC);

      if (inMsg->extra) {
        chsnprintf (payload, payload_size,
            "{\"name\":\"%s %s\",\"def_ent_id\":\"%s.%s\",\"uniq_id\":\"%s\","
            "\"ent_cat\":\"diagnostic\",\"ic\":\"mdi:security-network\","
            "\"pl_on\":\"%s\",\"pl_off\":\"%s\",\"stat_t\":\"%s%s\","
            "\"dev\":{\"name\":\"%s\",\"ids\":\"%s\",\"mf\":\"vysocan\","
            "\"mdl\":\"Open Home Security\","
            "\"hw\":\"2.0.x\",\"sw\":\"%u.%u.%u\",\"cu\":\"http://%s\"}}",
            text_System, text_State,
            text_binary_sensor, text_state,
            mqttHadUid, text_On, text_Off,
            MQTT_MAIN_TOPIC, text_state, OHS_NAME, mqttHadUid, OHS_MAJOR,
            OHS_MINOR, OHS_MOD, ip4addr_ntoa ((ip4_addr_t*) &netInfo.ip));
      } else {
        chsnprintf (payload, payload_size, ""); // Empty to remove
      }
      break;

    default:
      break;
  }
}

/**
 * Handle typeGroup publish events
 */
static void handleMqttPubGroup(mqttPubEvent_t *inMsg, char *topic,
    size_t topic_size, char *payload, size_t payload_size, uint8_t *qos,
    uint8_t *retain) {
  *qos = 0;
  *retain = 1;

  if (inMsg->number >= ALARM_GROUPS) return;

  chsnprintf (topic, topic_size, "%s%s/%u/", MQTT_MAIN_TOPIC, text_group,
      inMsg->number + 1);

  switch (inMsg->function) {
    case functionName:
      strncat (topic, &text_name[0],
          LWIP_MIN (strlen (text_name), (topic_size - strlen (topic) - 1)));
      chsnprintf (payload, payload_size, "%s", conf.group[inMsg->number].name);
      break;

    case functionHAD:
      chsnprintf (topic, topic_size, "%salarm_control_panel/%s-G%u/%s",
          MQTT_HAD_MAIN_TOPIC, mqttHadUid, inMsg->number + 1, MQTT_HAD_CONFIG_TOPIC);

      if (inMsg->extra) {
        chsnprintf (payload, payload_size,
            "{\"name\":\"%s: %s\",\"def_ent_id\":\"%s.%s\",\"uniq_id\":\"%s-G%u\","
            "\"sup_feat\":[\"arm_home\",\"arm_away\"],\"pl_arm_away\":\"arm_away\","
            "\"pl_arm_home\":\"arm_home\",\"pl_disarm\":\"disarm\","
            "\"cod_arm_req\":\"false\",\"stat_t\":\"%sgroup/%u/state\","
            "\"cmd_t\":\"%sset/group/%u/state\","
            "\"dev\":{\"ids\":\"%s\"}}",
            text_Group, conf.group[inMsg->number].name,
            text_alarm_control_panel, text_state,
            mqttHadUid, inMsg->number + 1,
            MQTT_MAIN_TOPIC, inMsg->number + 1, MQTT_MAIN_TOPIC, inMsg->number + 1,
            mqttHadUid);
      } else {
        chsnprintf (payload, payload_size, ""); // Empty to remove
      }
      break;

    default: // functionState
      strncat (topic, &text_state[0],
          LWIP_MIN (strlen (text_state), (topic_size - strlen (topic) - 1)));

      // State decode: Alarm → Disarm → Arm logic
      if (GET_GROUP_ALARM (group[inMsg->number].setting) == 0) {
        if (GET_GROUP_WAIT_AUTH (group[inMsg->number].setting)) {
          chsnprintf (payload, payload_size, "%s", text_disarming);
        } else {
          if (GET_GROUP_ARMED (group[inMsg->number].setting)) {
            if (GET_GROUP_ARMED_HOME (group[inMsg->number].setting)) {
              chsnprintf (payload, payload_size, "%s_%s", text_armed,
                  text_home);
            } else {
              chsnprintf (payload, payload_size, "%s_%s", text_armed,
                  text_away);
            }
          } else {
            if (group[inMsg->number].armDelay > 0) {
              chsnprintf (payload, payload_size, "%s", text_arming);
            } else {
              chsnprintf (payload, payload_size, "%s", text_disarmed);
            }
          }
        }
      } else {
        chsnprintf (payload, payload_size, "%s", text_triggered); // Alarm
      }
      break;
  }
}

/**
 * Handle typeZone publish events
 */
static void handleMqttPubZone(mqttPubEvent_t *inMsg, char *topic,
    size_t topic_size, char *payload, size_t payload_size, uint8_t *qos,
    uint8_t *retain) {
  *qos = 0;
  *retain = 0; // Default, override below for specific functions

  if (inMsg->number >= ALARM_ZONES) return;

  chsnprintf (topic, topic_size, "%s%s/%u/", MQTT_MAIN_TOPIC, text_zone,
      inMsg->number + 1);

  switch (inMsg->function) {
    case functionName:
      *retain = 1;
      strncat (topic, &text_name[0],
          LWIP_MIN (strlen (text_name), (topic_size - strlen (topic) - 1)));
      chsnprintf (payload, payload_size, "%s", conf.zoneName[inMsg->number]);
      break;

    case functionHAD:
      chsnprintf (topic, topic_size, "%sbinary_sensor/%s-Z%u/%s",
          MQTT_HAD_MAIN_TOPIC, mqttHadUid, inMsg->number + 1,
          MQTT_HAD_CONFIG_TOPIC);

      if (inMsg->extra) {
        chsnprintf (payload, payload_size,
            "{\"name\":\"%s: %s\",\"def_ent_id\":\"%s.%s\",\"uniq_id\":\"%s-Z%u\","
            "\"pl_on\":\"alarm\",\"pl_off\":\"OK\","
            "\"stat_t\":\"%szone/%u/state\",\"ic\":\"mdi:motion-sensor\","
            "\"dev\":{\"ids\":\"%s\"}}",
            text_Zone, conf.zoneName[inMsg->number],
            text_binary_sensor, text_state,
            mqttHadUid, inMsg->number + 1, MQTT_MAIN_TOPIC, inMsg->number + 1,
            mqttHadUid);
      } else {
        chsnprintf (payload, payload_size, ""); // Empty to remove
      }
      break;

    default: // functionState
      *retain = 0;
      strncat (topic, &text_state[0],
          LWIP_MIN (strlen (text_state), (topic_size - strlen (topic) - 1)));

      // Map zone event to state string
      switch (zone[inMsg->number].lastEvent) {
        case 'O':
          chsnprintf (payload, payload_size, "%s", text_OK);
          break;
        case 'P':
          chsnprintf (payload, payload_size, "%s", text_alarm);
          break;
        case 'N': // Initial state
          chsnprintf (payload, payload_size, "%s", text_unknown);
          break;
        default:
          chsnprintf (payload, payload_size, "%s", text_tamper);
          break;
      }
      break;
  }
}

/*
 * Handle typeSensor publish events
 * Sensor address format: "W:2:K:i:0" or "R:2:K:i:0"
 */
static void handleMqttPubSensor(mqttPubEvent_t *inMsg, char *topic,
    size_t topicSize, char *payload, size_t payloadSize, uint8_t *qos,
    uint8_t *retain) {
  *qos = 0;
  *retain = 0; // Default, override for Name/HAD

  if (inMsg->number >= NODE_SIZE) return;

  chsnprintf(topic, topicSize, "%s%s/%c:%u:%c:%c:%u/", MQTT_MAIN_TOPIC,
      text_sensor,
      (node[inMsg->number].address < RADIO_UNIT_OFFSET) ? 'W' : 'R',
      (node[inMsg->number].address < RADIO_UNIT_OFFSET) ? node[inMsg->number].address :
          (node[inMsg->number].address - RADIO_UNIT_OFFSET),
      node[inMsg->number].type, node[inMsg->number].function,
      node[inMsg->number].number);

  switch (inMsg->function) {
    case functionName:
      *retain = 1;
      strncat (topic, &text_name[0],
          LWIP_MIN (strlen (text_name), (topicSize - strlen (topic) - 1)));
      chsnprintf(payload, payloadSize, "%s", node[inMsg->number].name);
      break;

    case functionHAD: {
      *retain = 1;
      chsnprintf(topic, topicSize, "%s%s/%s-%c%u%c%c%u/%s",
          MQTT_HAD_MAIN_TOPIC, text_sensor, mqttHadUid,
          (node[inMsg->number].address < RADIO_UNIT_OFFSET) ? 'W' : 'R',
          (node[inMsg->number].address < RADIO_UNIT_OFFSET) ?
              node[inMsg->number].address :
              (node[inMsg->number].address - RADIO_UNIT_OFFSET),
          node[inMsg->number].type, node[inMsg->number].function,
          node[inMsg->number].number, MQTT_HAD_CONFIG_TOPIC);

      if (inMsg->extra) {
        if (getNodeFunctionHAClass (node[inMsg->number].function)[0] == '\0') {
          // Unsupported function for HA auto discovery
          chsnprintf (payload, payloadSize, "unsupported");
          break;
        }
        // Supported function, prepare JSON payload
        chsnprintf (mqttPayload, payloadSize,
            "{\"name\":\"%s: %s\","
              "\"def_ent_id\":\"%s.%s\","
              "\"uniq_id\":\"%s-%c%u%c%c%u\","
              "\"stat_t\":\"%s%s/%c:%u:%c:%c:%u/%s\","
              "\"dev_cla\":\"%s\","
              "\"unit_of_meas\":\"%s\","
              "\"dev\":{\"ids\":\"%s\"}"
            "}",
            text_Sensor, node[inMsg->number].name,
            text_sensor, text_value,
            mqttHadUid, (node[inMsg->number].address < RADIO_UNIT_OFFSET) ? 'W' : 'R',
            (node[inMsg->number].address < RADIO_UNIT_OFFSET) ? node[inMsg->number].address : (node[inMsg->number].address - RADIO_UNIT_OFFSET),
            node[inMsg->number].type, node[inMsg->number].function,
            node[inMsg->number].number
            , MQTT_MAIN_TOPIC, text_sensor,
            (node[inMsg->number].address < RADIO_UNIT_OFFSET) ? 'W' : 'R',
            (node[inMsg->number].address < RADIO_UNIT_OFFSET) ?
                node[inMsg->number].address :
                (node[inMsg->number].address - RADIO_UNIT_OFFSET),
            node[inMsg->number].type, node[inMsg->number].function,
            node[inMsg->number].number, text_value,
            getNodeFunctionHAClass(node[inMsg->number].function),
            getNodeFunctionHAClassUnit(node[inMsg->number].function),
            mqttHadUid);
      } else {
        chsnprintf (payload, payloadSize, ""); // Empty to remove
      }
      break;
    }

    default: // functionValue
      *retain = 0;
      strncat (topic, &text_value[0],
          LWIP_MIN (strlen (text_value), (topicSize - strlen (topic) - 1)));

      // Key sensors: output contact name; others: float value
      if (node[inMsg->number].type == 'K') {
        uint8_t key_idx = (uint8_t) node[inMsg->number].value;
        if (key_idx < KEYS_SIZE) {
          chsnprintf (payload, payloadSize, "%s",
              conf.contact[conf.key[key_idx].contact].name);
        } else {
          chsnprintf (payload, payloadSize, "%s", text_unknown);
        }
      } else {
        chsnprintf (payload, payloadSize, "%.2f", node[inMsg->number].value);
      }
      break;
  }
}

/**
 * Publish message via MQTT client
 * @param topic MQTT topic
 * @param payload MQTT payload
 * @param qos Quality of Service (0, 1, or 2)
 * @param retain Retain flag (0 or 1)
 * @return ERR_OK on success, error code otherwise
 */
static err_t mqttPublish(const char *topic, const char *payload,
                         uint8_t qos, uint8_t retain) {
  err_t err;

  LOCK_TCPIP_CORE();
  err = mqtt_publish(&mqtt_client, (char*)topic, (char*)payload, strlen(payload),
                     qos, retain, mqttPubRequestCB, NULL);
  UNLOCK_TCPIP_CORE();

  DBG_MQTT_PUB(
      "Publish Topic: %s, Payload: %s, QoS: %d, Retain: %d, Err: %d\r\n", topic,
      payload, qos, retain, err);

  return err;
}

/*
 * Main MQTT Publish Thread
 */
#define MQTT_TH_PUB_TOPIC_SIZE 60
static THD_WORKING_AREA(waMqttPubThread, 1024);
static THD_FUNCTION( MqttPubThread, arg) {
  chRegSetThreadName (arg);

  uint8_t counterMQTT = 225; // Force faster connect on start, 255-30 seconds
  err_t err;
  msg_t msg;
  mqttPubEvent_t *inMsg;
  uint8_t qos;
  uint8_t retain;
  char topic[MQTT_TH_PUB_TOPIC_SIZE];
  uint8_t is_connected;

  while (true) {
    msg = chMBFetchTimeout (&mqtt_pub_mb, (msg_t*) &inMsg, TIME_S2I (1));

    if (msg != MSG_OK) {
      // Handle timeout: periodic MQTT connection check
      if (msg == MSG_TIMEOUT) {
        // Only attempt if IPv4 is valid
        if (!(netInfo.status & LWIP_NSC_IPV4_ADDR_VALID)) goto next_cycle;

        LOCK_TCPIP_CORE ();
        is_connected = mqtt_client_is_connected (&mqtt_client);
        UNLOCK_TCPIP_CORE ();

        if (!is_connected) {
          // Force retry every counterMQTT overflow (allows exponential backoff)
          if (counterMQTT == 0) {
            CLEAR_CONF_MQTT_CONNECT_ERROR (conf.mqtt.setting);
          }

          // Attempt connection if no error state
          if (!GET_CONF_MQTT_CONNECT_ERROR (conf.mqtt.setting)
              && !GET_CONF_MQTT_ADDRESS_ERROR (conf.mqtt.setting)) {
            mqttDoConnect (&mqtt_client);
          }
          counterMQTT++;
        }
      } else {
        DBG_MQTT_PUB("MQTT MB ERROR\r\n");
      }
      goto next_cycle;
    }

    // We have a message to publish
    DBG_MQTT_PUB("MQTT Type: %d, Func: %d, # %d >> ",
        (uint8_t)inMsg->type, (uint8_t)inMsg->function, inMsg->number);

    // Check if MQTT is connected
    LOCK_TCPIP_CORE ();
    is_connected = mqtt_client_is_connected (&mqtt_client);
    UNLOCK_TCPIP_CORE ();

    if (!is_connected) {
      DBG_MQTT_PUB("not connected\r\n");
      goto free_msg;
    }

    // Wait for free MQTT semaphore
    if (chBSemWaitTimeout (&mqttSem, TIME_MS2I (100)) != MSG_OK) {
      DBG_MQTT_PUB(" semaphore timeout!\r\n");
      // Log this event
      if (!GET_CONF_MQTT_SEMAPHORE_ERROR_LOG (conf.mqtt.setting)) {
        pushToLogText ("QET");
        SET_CONF_MQTT_SEMAPHORE_ERROR_LOG (conf.mqtt.setting);
      }
      goto free_msg;
    }

    // Initialize buffers
    memset (topic, 0, MQTT_TH_PUB_TOPIC_SIZE);
    memset (mqttPayload, 0, MQTT_PAYLOAD_LENGTH);

    // Route to type-specific handler
    switch (inMsg->type) {
      case typeSystem:
        handleMqttPubSystem (inMsg, topic, MQTT_TH_PUB_TOPIC_SIZE, mqttPayload,
            MQTT_PAYLOAD_LENGTH, &qos, &retain);
        break;

      case typeGroup:
        handleMqttPubGroup (inMsg, topic, MQTT_TH_PUB_TOPIC_SIZE, mqttPayload,
            MQTT_PAYLOAD_LENGTH, &qos, &retain);
        break;

      case typeZone:
        handleMqttPubZone (inMsg, topic, MQTT_TH_PUB_TOPIC_SIZE, mqttPayload,
            MQTT_PAYLOAD_LENGTH, &qos, &retain);
        break;

      case typeSensor:
        handleMqttPubSensor (inMsg, topic, MQTT_TH_PUB_TOPIC_SIZE, mqttPayload,
            MQTT_PAYLOAD_LENGTH, &qos, &retain);
        break;

      default:
        DBG_MQTT_PUB(" undefined!\r\n");
        chBSemSignal (&mqttSem); // Release semaphore on error
        goto free_msg;
    }

    // Publish message
    err = mqttPublish (topic, mqttPayload, qos, retain);

    if (err != ERR_OK) {
      DBG_MQTT_PUB(" error: %d\r\n", err);
      // Log error
      tmpLog[0] = 'Q'; tmpLog[1] = 'E'; tmpLog[2] = 'P'; tmpLog[3] = abs(err);
      pushToLog (tmpLog, 4);
      // Release semaphore in case of publish error (callback won't be called)
      chBSemSignal (&mqttSem);
    } else {
      DBG_MQTT_PUB(" OK\r\n");
      CLEAR_CONF_MQTT_SEMAPHORE_ERROR_LOG (conf.mqtt.setting);
    }

    free_msg:
    // Free message in all paths
    chPoolFree (&mqtt_pub_pool, inMsg);

    next_cycle: ; // Null statement for label
  }
}

#endif /* OHS_TH_MQTT_PUB_H_ */
