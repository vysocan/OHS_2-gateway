/*
 * ohs_mqtt_functions.h
 *
 *  Created on: Dec 8, 2020
 *      Author: vysocan
 */
#ifndef OHS_MQTT_FUNCTIONS_H_
#define OHS_MQTT_FUNCTIONS_H_

#ifndef MQTT_FUNC_DEBUG
#define MQTT_FUNC_DEBUG 1
#endif

#if MQTT_FUNC_DEBUG
#define DBG_MQTT_FUNC(...) {chprintf(console, __VA_ARGS__);}
#else
#define DBG_MQTT_FUNC(...)
#endif

// MQTT test IP address
static ip_addr_t mqtt_ip; // = IPADDR4_INIT_BYTES(10,10,10,127);
struct mqtt_client_s mqtt_client;
#define MQTT_TOPIC_LENGTH   40
#define MQTT_PAYLOAD_LENGTH 40
char mqttInTopic[MQTT_TOPIC_LENGTH], mqttInPayload[MQTT_PAYLOAD_LENGTH];

// MQTT client information
struct mqtt_connect_client_info_t mqttCI = {
  OHS_NAME,
  NULL, /* user */
  NULL, /* pass */
  120,  /* keep alive */
  MQTT_MAIN_TOPIC MQTT_WILL_TOPIC, /* will_topic */
  &text_Off[0], /* will_msg */
  2,    /* will_qos */
  1     /* will_retain */
#if LWIP_ALTCP && LWIP_ALTCP_TLS
  , NULL
#endif
};
/*
 * Called when publish is complete either with success or failure
 */
static void mqttPubRequestCB(void *arg, err_t result) {
  LWIP_UNUSED_ARG(arg);
  LWIP_UNUSED_ARG(result);

  chBSemSignal(&mqttSem);

  if(result != ERR_OK) {
    DBG_MQTT_FUNC("MQTT Publish error: %d\n", result);
    pushToLogText("QEP"); // Publish error
  }
}
/*
 * MQTT subscribe topic callback
 */
static void mqttIncomingPublishCB(void *arg, const char *topic, u32_t tot_len) {

  DBG_MQTT_FUNC("Incoming publish at topic %s with total length %u\r\n", topic, (unsigned int)tot_len);
  // Clear mqttInTopic passed to arg
  memset(arg, 0, MQTT_TOPIC_LENGTH);
  // Copy last part of topic after OHS/set/ to *arg
  strncpy(arg, &topic[strlen(MQTT_MAIN_TOPIC MQTT_SET_TOPIC)], MQTT_TOPIC_LENGTH - 1);
}
/*
 * MQTT subscribe data callback and processing
 *
 * Subscribe topics: MQTT_MAIN_TOPIC MQTT_SET_TOPIC #
 * /group
 *   /{#} - index of group {1 .. ALARM_GROUPS}
 *     /state {disarm, arm_home, arm_away}
 * /sensor
 *   /{address} - node address like W:2:K:i:0
 *     /state
 */
static void mqttIncomingDataCB(void *arg, const u8_t *data, u16_t len, u8_t flags) {
  char *pch, *addressIndex[NODE_ADDRESS_SIZE];
  uint8_t index = 0;
  uint8_t message[6];

  DBG_MQTT_FUNC("Incoming publish payload with length %d, flags %u. Arg: %s\r\n", len, (unsigned int)flags, arg);

  // Clear mqttInPayload
  memset(mqttInPayload, 0, MQTT_PAYLOAD_LENGTH);
  // Copy data payload to our mqttInPayload
  strncpy(mqttInPayload, (const char *)data, LWIP_MIN(len, MQTT_PAYLOAD_LENGTH - 1));

  // Last fragment of payload received (or whole part if payload fits receive buffer
  // See MQTT_VAR_HEADER_BUFFER_LEN)
  if ((arg != NULL) && (flags & MQTT_DATA_FLAG_LAST)) {
    // Decode topic string
    pch = strtok(arg, "/");
    DBG_MQTT_FUNC("Parse: %s", pch);
    if (pch != NULL) {
      // Groups
      if (strcmp(pch, text_group) == 0) {
        // Get group index from topic
        pch = strtok(NULL, "/");
        if (pch != NULL) {
          index = strtoul(pch, NULL, 0) - 1;
          if ((index < ALARM_GROUPS) &&
              (GET_CONF_GROUP_ENABLED(conf.group[index].setting)) &&
              (GET_CONF_GROUP_MQTT(conf.group[index].setting))) {
            DBG_MQTT_FUNC(", group # %d", index + 1);
            pch = strtok(NULL, "/");
            DBG_MQTT_FUNC(", topic: %s", pch);
            if (pch != NULL) {
              // State
              if (strcmp(pch, text_state) == 0) {
                DBG_MQTT_FUNC(", payload %s =", mqttInPayload);
                if (strcmp(mqttInPayload, text_arm_home) == 0) {
                  DBG_MQTT_FUNC(" %s", text_arm_home);
                  armGroup(index, index, armHome, 0);
                } else if (strcmp(mqttInPayload, text_arm_away) == 0) {
                  DBG_MQTT_FUNC(" %s", text_arm_away);
                  armGroup(index, index, armAway, 0);
                } else if (strcmp(mqttInPayload, text_disarm) == 0) {
                  DBG_MQTT_FUNC(" %s", text_disarm);
                  disarmGroup(index, index, 0);
                } else {
                  DBG_MQTT_FUNC(" %s", text_unknown);
                }
              }
            }
          }
        }
      } // Groups
      // Sensors
      else if (strcmp(pch, text_sensor) == 0) {
        // Get node address from topic
        pch = strtok(NULL, ":");
        while ((pch != NULL) && (index < NODE_ADDRESS_SIZE)){
          //DBG_MQTT_FUNC(">%u>%s<\r\n", indexNum, pch);
          addressIndex[index] = pch;
          index++;
          if (index < (NODE_ADDRESS_SIZE - 1)) pch = strtok(NULL, ":");
          else pch = strtok(NULL, "/");
        }
        if (index == NODE_ADDRESS_SIZE) {
          index = getNodeIndex((*addressIndex[0] == 'R' ? RADIO_UNIT_OFFSET : 0) + strtoul(addressIndex[1], NULL, 0),
                                *addressIndex[2], *addressIndex[3], strtoul(addressIndex[4], NULL, 0));
          if ((index != DUMMY_NO_VALUE) &&
              (GET_NODE_ENABLED(node[index].setting)) &&
              (GET_NODE_MQTT(node[index].setting))) {
            DBG_MQTT_FUNC(", node index: %d", index);
            if (pch != NULL) {
              DBG_MQTT_FUNC(", topic: %s", pch);
              // Value
              //if ((strcmp(pch, text_value) == 0) && (node[index].function == 'I')) {
              if (strcmp(pch, text_value) == 0) {
                DBG_MQTT_FUNC(" = '%.*s'", len, (const char *)mqttInPayload);
                floatConv.val = strtof((const char *)mqttInPayload, NULL);
                message[0] = node[index].function;
                message[1] = node[index].number;
                message[2] = floatConv.byte[0]; message[3] = floatConv.byte[1];
                message[4] = floatConv.byte[2]; message[5] = floatConv.byte[3];
                DBG_MQTT_FUNC(", message: %c-%d-%.2f", message[0], message[1], floatConv.val);
                if (sendData(node[index].address, message, 6) == 1) {
                  node[index].lastOK = getTimeUnixSec();
                  node[index].value = floatConv.val;
                }
              }
            }
          }
        }
      } // Sensors
      else {
        DBG_MQTT_FUNC(" %s", text_unknown);
      }
    } // !NULL
    DBG_MQTT_FUNC(".\r\n");
  } // else { }
    // Handle fragmented payload, store in buffer, write to file or whatever
}
/*
 * MQTT subscribe topic callback
 */
static void mqttSubscribeCB(void *arg, err_t result) {
  LWIP_UNUSED_ARG(arg);

  /* Just print the result code here for simplicity,
     normal behavior would be to take some action if subscribe fails like
     notifying user, retry subscribe or disconnect from server */
  // TODO OHS Add subscribe error handling mechanism if subscribe fails.
  if(result != ERR_OK) {
    DBG_MQTT_FUNC("MQTT CB Subscibe error: %d\r\n", result);
    SET_CONF_MQTT_SUBSCRIBE_ERROR(conf.mqtt.setting);
    pushToLogText("QES"); // Subscribe error
  }
}
/*
 * MQTT connection callback
 */
static void mqttConnectionCB(mqtt_client_t *client, void *arg, mqtt_connection_status_t status) {
  LWIP_UNUSED_ARG(client);
  LWIP_UNUSED_ARG(arg);

  err_t err;
  if(status == MQTT_CONNECT_ACCEPTED) {
    DBG_MQTT_FUNC("MQTT connect CB: Connected\r\n");
    CLEAR_CONF_MQTT_CONNECT_ERROR(conf.mqtt.setting);
    CLEAR_CONF_MQTT_CONNECT_ERROR_LOG(conf.mqtt.setting);
    pushToLogText("QC"); // MQTT connected
    // Reset the semaphore to allow next publish
    chBSemReset(&mqttSem, false);

    // Subscribe to set topic
    if (GET_CONF_MQTT_SUBSCRIBE(conf.mqtt.setting)) {
      // Setup callback for incoming publish requests
      mqtt_set_inpub_callback(client, mqttIncomingPublishCB, mqttIncomingDataCB, &mqttInTopic);
      // Subscribe to a topic
      err = mqtt_subscribe(client, MQTT_MAIN_TOPIC MQTT_SET_TOPIC "#", 1, mqttSubscribeCB, arg);

      if(err != ERR_OK) {
        DBG_MQTT_FUNC("MQTT connect CB: Subscribe error: %d\r\n", err);
        SET_CONF_MQTT_SUBSCRIBE_ERROR(conf.mqtt.setting);
        pushToLogText("QES"); // Subscribe error
      }
    } // Subscribe to set topic
  } else {
    DBG_MQTT_FUNC("MQTT CB: Disconnected, reason: %d\r\n", status);
    SET_CONF_MQTT_CONNECT_ERROR(conf.mqtt.setting);
    if (!GET_CONF_MQTT_CONNECT_ERROR_LOG(conf.mqtt.setting)) {
      tmpLog[0] = 'Q'; tmpLog[1] = 'E';
      // As defined in lwip for mqtt_connection_status_t
      if (status <= 5 ) tmpLog[2] = '0' + status;
      else              tmpLog[2] = '6' + (uint8_t)(status-256);
      pushToLog(tmpLog, 3);
      SET_CONF_MQTT_CONNECT_ERROR_LOG(conf.mqtt.setting);
    }
  }
}
/*
 * MQTT connect
 */
void mqttDoConnect(mqtt_client_t *client) {
  err_t err;

  // Resolve mqtt.address to IP address
  if (!(ipaddr_aton(conf.mqtt.address, &mqtt_ip))) {
    // Address is not in IP address format
    err = netconn_gethostbyname_addrtype(conf.mqtt.address, &mqtt_ip, IPADDR_TYPE_V4);
    if (err != ERR_OK) {
      // Host not found via DNS
      SET_CONF_MQTT_ADDRESS_ERROR(conf.mqtt.setting);
      pushToLogText("QER");
    } else {
      // Host found we have IP
      CLEAR_CONF_MQTT_ADDRESS_ERROR(conf.mqtt.setting);
    }
  } else {
    // Address is in IP address format
    CLEAR_CONF_MQTT_ADDRESS_ERROR(conf.mqtt.setting);
  }

  // Add user & pass
  if (conf.mqtt.user[0]) mqttCI.client_user = &conf.mqtt.user[0];
  else mqttCI.client_user = NULL;
  if (conf.mqtt.password[0]) mqttCI.client_pass = &conf.mqtt.password[0];
  else mqttCI.client_pass = NULL;

  // If we have presumably valid IP address, try to connect
  if (!GET_CONF_MQTT_ADDRESS_ERROR(conf.mqtt.setting)) {
    err = mqtt_client_connect(client, &mqtt_ip, conf.mqtt.port, mqttConnectionCB,
                              LWIP_CONST_CAST(void*, &mqttCI), &mqttCI);

    // Check immediate return code
    if(err != ERR_OK) {
      DBG_MQTT_FUNC("MQTT Connect error: %d\r\n", err);
      SET_CONF_MQTT_CONNECT_ERROR(conf.mqtt.setting);
      if (!GET_CONF_MQTT_CONNECT_ERROR_LOG(conf.mqtt.setting)) {
        pushToLogText("QEC");
        SET_CONF_MQTT_CONNECT_ERROR_LOG(conf.mqtt.setting);
      }
    }
  }
}

#endif /* OHS_MQTT_FUNCTIONS_H_ */
