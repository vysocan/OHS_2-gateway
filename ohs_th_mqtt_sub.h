/*
 * ohs_th_sub_mqtt.h
 *
 *  Created on: Jan 3, 2026
 *      Author: vysocan
 */

#ifndef OHS_TH_MQTT_SUB_H_
#define OHS_TH_MQTT_SUB_H_

#ifndef OHS_MQTT_SUB_DEBUG
#define OHS_MQTT_SUB_DEBUG 0
#endif

#if OHS_MQTT_SUB_DEBUG
#define DBG_MQTT_SUB(...) {chprintf(console, __VA_ARGS__);}
#else
#define DBG_MQTT_SUB(...)
#endif

/*
 * Handle MQTT group command
 */
static void handleMqttSubGroup(char *args, char *savePtr, const char *payload) {
  uint32_t index = 0;
  char *pch;

  // Parse Index
  if (!safeStrtoul(args, &index, 10)) return;
  index--; // Adjust to 0-based
  DBG_MQTT_SUB(", # %d", index + 1);

  if (index >= ALARM_GROUPS ||
      !GET_CONF_GROUP_ENABLED(conf.group[index].setting) ||
      !GET_CONF_GROUP_MQTT(conf.group[index].setting)) {
    return;
  }

  // Parse Function (e.g., "state")
  pch = strtok_r(NULL, "/", &savePtr);
  if (pch == NULL) return;
  DBG_MQTT_SUB(", func: %s", pch);

  if (safeStrcmp1(pch, MQTT_SUB_PAYLOAD_LENGTH, TEXT_state) == 0) {
    DBG_MQTT_SUB(", payload = %s", payload);

    // Dispatch Action
    if (safeStrcmp1(payload, MQTT_SUB_PAYLOAD_LENGTH, TEXT_arm_home) == 0) {
      armGroup(index, index, armHome, 0);
    } else if (safeStrcmp1(payload, MQTT_SUB_PAYLOAD_LENGTH, TEXT_arm_away) == 0) {
      armGroup(index, index, armAway, 0);
    } else if (safeStrcmp1(payload, MQTT_SUB_PAYLOAD_LENGTH, TEXT_disarm) == 0) {
      disarmGroup(index, index, 0);
    }
  } else if (safeStrcmp1(args, MQTT_SUB_PAYLOAD_LENGTH, TEXT_refresh) == 0) {
    mqttRefreshGroupsState();
  } else {
    DBG_MQTT_SUB(", unknown group command");
  }
}
/*
 * Handle MQTT sensor command
 */
static void handleMqttSubSensor(char *args, char *savePtr, const char *payload) {
  char *addressParts[NODE_ADDRESS_SIZE];
  char *pch = args;
  uint8_t msgBuf[6];
  uint32_t index = 0;

  // Parse Address Parts (Addr0:Addr1:Addr2:Addr3)
  // Logic: First parts split by ':', last part split by '/' or end

  // We already have the first token in 'args' from the caller
  while (pch != NULL && index < NODE_ADDRESS_SIZE) {
    addressParts[index++] = pch;
    // Switch delimiter based on position
    const char *delim = (index < NODE_ADDRESS_SIZE - 1) ? ":" : "/";
    pch = strtok_r(NULL, delim, &savePtr);
  }

  if (index != NODE_ADDRESS_SIZE) return;

  // Resolve Node Index
  index = getNodeIndex (
      (*addressParts[0] == 'R' ? RADIO_UNIT_OFFSET : 0)
      + strtoul(addressParts[1], NULL, 0), *addressParts[2],
      *addressParts[3], strtoul(addressParts[4], NULL, 0)
  );

  DBG_MQTT_SUB(", node index: %d", index);

  if (index == DUMMY_NO_VALUE ||
      !GET_NODE_ENABLED(node[index].setting) ||
      !GET_NODE_MQTT(node[index].setting)) {
    return;
  }

  // Parse Function (e.g., "value")
  // pch now holds the token after the address (from the loop above)
  if (pch != NULL) {
    DBG_MQTT_SUB(", topic: %s", pch);

    if (safeStrcmp1(pch, MQTT_SUB_PAYLOAD_LENGTH, TEXT_value) == 0 &&
        node[index].type == 'I') {

      if (safeStrtof(payload, &floatConv.val)) {
        DBG_MQTT_SUB(", value: %.2f", floatConv.val);

        // Prepare Message
        msgBuf[0] = node[index].type;
        msgBuf[1] = node[index].number;
        memcpy(&msgBuf[2], floatConv.byte, 4); // Cleaner than manual assignment

        // Send Data
        if (sendData(node[index].address, msgBuf, 6) == 1) {
           node[index].lastOK = getTimeUnixSec();
           node[index].value = floatConv.val;
           if (GET_NODE_MQTT(node[index].setting)) {
             pushToMqtt(typeSensor, index, functionValue);
           }
        }
      } else {
        DBG_MQTT_SUB(", float parse error");
      }
    }
  }
}
/*
 * Handle MQTT zone command
 */
static void handleMqttSubZone(char *args, char *savePtr) {
  (void)savePtr;
  if (args == NULL) return;

  if (safeStrcmp1(args, MQTT_SUB_PAYLOAD_LENGTH, TEXT_refresh) == 0) {
    DBG_MQTT_SUB(", refresh");
    mqttRefreshZonesState();
  } else {
    DBG_MQTT_SUB(", unknown zone command");
  }
}
/*
 * Handle MQTT sms command
 */
static void handleMqttSubSms(char *args, char *savePtr, const char *payload) {
  uint32_t index;
  int8_t resp;
  char *pch;

  if (args == NULL) return;

  if (safeStrcmp1(args, MQTT_SUB_PAYLOAD_LENGTH, TEXT_contact) == 0) {
    DBG_MQTT_SUB(", contact");

    // Parse Index
    pch = strtok_r(NULL, "/", &savePtr);
    if (pch == NULL) return;
    if (!safeStrtoul(pch, &index, 10)) return;
    index--;// Adjust to 0-based

    DBG_MQTT_SUB(" # %d", index + 1);
    if (index >= CONTACTS_SIZE ||
        !GET_CONF_CONTACT_ENABLED(conf.contact[index].setting)) {
      return;
    }

    // Parse Function (e.g., "state")
    pch = strtok_r(NULL, "/", &savePtr);
    if (pch == NULL) return;
    DBG_MQTT_SUB(", func: %s", pch);

    if (safeStrcmp1 (pch, MQTT_SUB_PAYLOAD_LENGTH, TEXT_message) == 0) {
      DBG_MQTT_SUB(", payload = %s", payload);
      // Do not send if GPRS is busy, wait up to 1 second
      if (chBSemWaitTimeout (&gprsSem, TIME_I2S(1)) == MSG_OK) {
        resp = sendSMSToContact(index, payload);
        chBSemSignal (&gprsSem);
        DBG_MQTT_SUB(", SMS status: %u", resp);
      }
    }
  } else if (safeStrcmp1 (args, MQTT_SUB_PAYLOAD_LENGTH, TEXT_group) == 0) {
    DBG_MQTT_SUB(", group");

    // Parse Index
    pch = strtok_r(NULL, "/", &savePtr);
    if (pch == NULL) return;
    if (!safeStrtoul(pch, &index, 10)) return;
    index--;// Adjust to 0-based

    DBG_MQTT_SUB(" # %d", index + 1);
    if (index >= ALARM_GROUPS ||
        !GET_CONF_GROUP_ENABLED (conf.group[index].setting)) {
      return;
    }

    // Parse Function (e.g., "state")
    pch = strtok_r (NULL, "/", &savePtr);
    if (pch == NULL) return;
    DBG_MQTT_SUB (", func: %s", pch);

    if (safeStrcmp1 (pch, MQTT_SUB_PAYLOAD_LENGTH, TEXT_message) == 0) {
      DBG_MQTT_SUB (", payload = %s", payload);

      // Send SMS to all contacts in group
      for (uint8_t i = 0; i < CONTACTS_SIZE; i++) {
        if (GET_CONF_CONTACT_ENABLED (conf.contact[i].setting)
            && ((GET_CONF_CONTACT_GROUP (conf.contact[i].setting) == index)
                || (GET_CONF_CONTACT_IS_GLOBAL (conf.contact[i].setting)))) {
          // Do not send if GPRS is busy, wait up to 1 second
          if (chBSemWaitTimeout (&gprsSem, TIME_I2S (1)) == MSG_OK) {
            resp = sendSMSToContact (i, payload);
            chBSemSignal (&gprsSem);
            DBG_MQTT_SUB(", SMS to contact %d status: %u ", i + 1, resp);
          }
        }
      }
    }
  } else {
    DBG_MQTT_SUB(", unknown SMS command");
  }
}

/*
 *
 * Subscribe topics: MQTT_MAIN_TOPIC MQTT_SET_TOPIC
 * /group
 *   /{#} - index of group {1 .. ALARM_GROUPS}
 *     /state - allowed commands {disarm, arm_home, arm_away}
 *   /refresh - request for all groups status republish {no payload}
 * /sensor - allowed only  for 'I'nput nodes
 *   /{address} - node address like W:2:I:D:0
 *     /value - float value
 * /zone
 *   /refresh - request for all zones status republish  {no payload}
 * /SMS - send SMS to contact
 *   /contact - send SMS to specific contact
 *     /{#} - index of user {1 .. CONTACTS_SIZE}
 *       /message - message text in payload
 *   /group - send SMS to all contacts of group
 *     /{#} - index of group {1 .. ALARM_GROUPS}
 *       /message - message text in payload
 */
static THD_WORKING_AREA(waMqttSubThread, 512);
static THD_FUNCTION(MqttSubThread, arg) {
  chRegSetThreadName(arg);

  mqttSubEvent_t *inMsg;
  char *pch;
  char *savePtr;
  char *payloadStr;

  while (true) {
    msg_t msg = chMBFetchTimeout(&mqtt_sub_mb, (msg_t*)&inMsg, TIME_INFINITE);

    if (msg != MSG_OK) {
      continue; // Should not happen with TIME_INFINITE, but safe practice
    }

    payloadStr = &inMsg->payload[0];
    DBG_MQTT_SUB("MQTT Sub TH: Topic: %s, Payload: %s\r\n", &inMsg->topic[0], payloadStr);

    // Parse Root Topic
    pch = strtok_r(&inMsg->topic[0], "/", &savePtr);
    DBG_MQTT_SUB("Parse: %s", pch ? pch : "NULL");

    if (pch != NULL) {
      // Dispatch to Handlers
      // Note: strtok_r saves state in savePtr, so handlers continue from where we left off

      // GROUP
      if (safeStrcmp1(pch, MQTT_SUB_PAYLOAD_LENGTH, TEXT_group) == 0) {
        // Get next token immediately to pass to handler
        char *nextTok = strtok_r(NULL, "/", &savePtr);
        if (nextTok) {
          if (safeStrcmp1(nextTok, MQTT_SUB_PAYLOAD_LENGTH, TEXT_refresh) == 0) {
             DBG_MQTT_SUB(", refresh");
             mqttRefreshGroupsState();
          } else {
             // Assume it's an index number
             handleMqttSubGroup(nextTok, savePtr, payloadStr);
          }
        }
      }
      // SENSOR
      else if (safeStrcmp1(pch, MQTT_SUB_PAYLOAD_LENGTH, TEXT_sensor) == 0) {
        // Sensor format: "sensor/ADDR:ADDR:ADDR:ADDR/value"
        // We need to switch delimiter logic inside the handler
        char *nextTok = strtok_r(NULL, ":", &savePtr); // First part of address
        if (nextTok) handleMqttSubSensor(nextTok, savePtr, payloadStr);
      }
      // ZONE
      else if (safeStrcmp1(pch, MQTT_SUB_PAYLOAD_LENGTH, TEXT_zone) == 0) {
        char *nextTok = strtok_r(NULL, "/", &savePtr);
        if (nextTok) handleMqttSubZone(nextTok, savePtr);
      }
      // SMS
      else if (safeStrcmp1 (pch, MQTT_SUB_PAYLOAD_LENGTH, TEXT_SMS) == 0) {
          char *nextTok = strtok_r(NULL, "/", &savePtr);
          if (nextTok) handleMqttSubSms(nextTok, savePtr, payloadStr);
      }
      else {
        DBG_MQTT_SUB(" %s", TEXT_unknown);
      }
    }

    DBG_MQTT_SUB(".\r\n");
    chPoolFree(&mqtt_sub_pool, inMsg);
  }
}

#endif /* OHS_TH_MQTT_SUB_H_ */
