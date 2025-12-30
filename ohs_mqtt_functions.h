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

/* MQTT module static data */
static ip_addr_t mqtt_ip; // = IPADDR4_INIT_BYTES(10,10,10,127);
struct mqtt_client_s mqtt_client;
char mqttInTopic[MQTT_SUB_TOPIC_LENGTH];

//#include "cmd_dispatcher.h"
//#include "mqtt_cmd_handler.h"

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

  if(result != ERR_OK) {
    DBG_MQTT_FUNC("mqttPubReqCB error: %d\r\n", result);
    // Publish error callback
    tmpLog[0] = 'Q'; tmpLog[1] = 'E'; tmpLog[2] = 'p'; tmpLog[3] = abs(result); pushToLog(tmpLog, 4);
  } else {
    DBG_MQTT_FUNC("mqttPubReqCB OK\r\n");
  }

  // Signal semaphore to allow next publish
  chBSemSignal(&mqttSem);
}
/*
 * MQTT subscribe topic callback
 */
static void mqttSubTopicCB(void *arg, const char *topic, u32_t tot_len) {
  LWIP_UNUSED_ARG(tot_len);

  if (arg == NULL || topic == NULL) {
    DBG_MQTT_FUNC("mqttIncomingPublishCB: NULL pointer\r\n");
    return;
  }

  DBG_MQTT_FUNC("MQTT IncomingPublishCB: %s, payload length: %u\r\n", topic, (unsigned int)tot_len);
  // Clear mqttInTopic passed to arg
  memset(arg, 0, MQTT_SUB_TOPIC_LENGTH);
  // Copy last part of topic after OHS/set/ to *arg
  strncpy(arg, &topic[strlen(MQTT_MAIN_TOPIC MQTT_SET_TOPIC)], MQTT_SUB_TOPIC_LENGTH - 1);
}
/*
 * MQTT subscribe data callback and processing
 *
 * Subscribe topics: MQTT_MAIN_TOPIC MQTT_SET_TOPIC
 * /group
 *   /{#} - index of group {1 .. ALARM_GROUPS}
 *     /state - allowed commands {disarm, arm_home, arm_away}
 * /sensor - allowed only  for 'I'nput nodes
 *   /{address} - node address like W:2:I:D:0
 *     /value - float value
 * /zone
 *   /refresh - request for all zones status republish
 *
 * ToDo:
 * /SMS - send SMS to contact
 *   /{#} - index of user {1 .. CONTACTS_SIZE}
 *     /text - string to send
 */
//typedef struct {
//  char *str;
//  uint8_t strLen;
//  uint8_t pos;
//  const char delimiter;
//} StringParser_t;
///*
// * @brief Parse next token from string parser, modifies input string to NULL-terminate tokens
// * @param parser Pointer to StringParser_t structure
// * @return Pointer to token string or NULL
// *
// */
//static const char* parseNextToken(StringParser_t *parser) {
//  // Skip leading delimiters
//  while ((parser->pos < parser->strLen) && (parser->str[parser->pos] == parser->delimiter)) {
//    parser->pos++;
//  }
//
//  // No more tokens if at end or no non-delimiter chars left
//  if (parser->pos >= parser->strLen) {
//    return NULL;
//  }
//
//  // Find end of token (next delimiter or end of buffer)
//  uint8_t start = parser->pos;
//  while ((parser->pos < parser->strLen) && (parser->str[parser->pos] != parser->delimiter)) {
//    parser->pos++;
//  }
//
//  // NULL terminate the token (safe - overwrites delimiter or points past end)
//  if (parser->pos < parser->strLen) {
//    parser->str[parser->pos] = '\0';
//    parser->pos++;  // Skip the delimiter we just NULL'd
//  } else {
//    // At end of buffer - ensure NULL terminated (should already be)
//    parser->str[parser->pos] = '\0';
//  }
//
//  return (parser->pos > start) ? (&parser->str[start]) : NULL;
//}
///*
// * MQTT incoming data callback
// */
//static void mqttIncomingDataCB(void *arg, const u8_t *data, u16_t len, u8_t flags) {
//  StringParser_t parser = {
//    (char *)arg, strlen((char *)data), 0, '/'
//  };
//  const char *token;
//  uint8_t index = 0;
//  uint8_t message[6];
//
//  if (arg == NULL || data == NULL) {
//    DBG_MQTT_FUNC("mqttIncomingDataCB: NULL pointer\r\n");
//    return;
//  }
//
//  memset(mqttInPayload, 0, MQTT_IN_PAYLOAD_LENGTH);
//  strncpy(mqttInPayload, (const char *)data, LWIP_MIN(len, MQTT_IN_PAYLOAD_LENGTH - 1));
//
//  if (flags & MQTT_DATA_FLAG_LAST) {
//    token = parseNextToken(&parser);
//
//    if (token != NULL) {
//      if (strcmp(token, text_group) == 0) {
//        token = parseNextToken(&parser);
//        if (token != NULL) {
//          index = strtoul(token, NULL, 0) - 1;
//          if ((index < ALARM_GROUPS) && (GET_CONF_GROUP_ENABLED(conf.group[index].setting))) {
//            token = parseNextToken(&parser);
//            if (token != NULL && strcmp(token, text_state) == 0) {
//              // Handle group state commands
//              if (strcmp(mqttInPayload, text_arm_home) == 0) {
//                armGroup(index, index, armHome, 0);
//              } else if (strcmp(mqttInPayload, text_arm_away) == 0) {
//                armGroup(index, index, armAway, 0);
//              } else if (strcmp(mqttInPayload, text_disarm) == 0) {
//                disarmGroup(index, index, 0);
//              }
//            }
//          }
//        } else {
//          return; // No group index
//        }
//      } else if (strcmp(token, text_sensor) == 0) {
//        // Similar parsing for sensor tokens
//      } else if (strcmp(token, text_zone) == 0) {
//        token = parseNextToken(&parser);
//        if (token != NULL && strcmp(token, text_refresh) == 0) {
//          mqttRefreshZonesState();
//        }
//      }
//    }
//  }
//}
static void mqttSubDataCB(void *arg, const u8_t *data, u16_t len, u8_t flags) {

  if(arg == NULL || data == NULL) {
    DBG_MQTT_FUNC("mqttIncomingDataCB: NULL pointer\r\n");
    return;
  }

  DBG_MQTT_FUNC("MQTT IncomingDataCB len: %d, flags: %d. Arg: %s<\r\n", len, (u8_t)flags, arg);

  // Last fragment of payload received (or whole part if payload fits receive buffer
  // See MQTT_VAR_HEADER_BUFFER_LEN)
  if (flags & MQTT_DATA_FLAG_LAST) {
    // Pass data to Subscribe topic worker
    mqttSubEvent_t *outMsg = chPoolAlloc(&mqtt_sub_pool);

    if (outMsg != NULL) {
      memset(outMsg, 0, sizeof(mqttSubEvent_t));
      strncpy(&outMsg->topic[0], (const char *)arg, LWIP_MIN(strlen((const char *)arg), MQTT_SUB_TOPIC_LENGTH - 1));
      strncpy(&outMsg->payload[0], (const char *)data, LWIP_MIN(len, MQTT_SUB_PAYLOAD_LENGTH - 1));

      msg_t msg = chMBPostTimeout(&mqtt_sub_mb, (msg_t)outMsg, TIME_IMMEDIATE);
      if (msg != MSG_OK) {
        //chprintf(console, "MB full %d\r\n", temp);
      }
    } else {
      chprintf(console, "MQTT Sub pool full!\r\n");
      pushToLogText("Fm"); // MQTT queue is full
    }
  } // else { }
    // Handle fragmented payload, store in buffer, write to file or whatever
}
/*
 * MQTT subscribe topic callback
 */
static void mqttSubscribedCB(void *arg, err_t result) {
  LWIP_UNUSED_ARG(arg);

  /* Just print the result code here for simplicity,
     normal behavior would be to take some action if subscribe fails like
     notifying user, retry subscribe or disconnect from server */
  // TODO OHS Add subscribe error handling mechanism if subscribe fails.
  if(result != ERR_OK) {
    DBG_MQTT_FUNC("MQTT Subscribed CB error: %d\r\n", result);
    SET_CONF_MQTT_SUBSCRIBE_ERROR(conf.mqtt.setting);
    pushToLogText("QES"); // Subscribe error
  } else {
    DBG_MQTT_FUNC("MQTT Subscribed CB OK\r\n");
    CLEAR_CONF_MQTT_SUBSCRIBE_ERROR(conf.mqtt.setting);
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
    DBG_MQTT_FUNC("MQTT ConnectionCB: Connected\r\n");
    CLEAR_CONF_MQTT_CONNECT_ERROR(conf.mqtt.setting);
    CLEAR_CONF_MQTT_CONNECT_ERROR_LOG(conf.mqtt.setting);
    pushToLogText("QC"); /* MQTT connected */

    /* Reset the semaphore to allow next publish */
    chBSemReset(&mqttSem, false);

    /* Subscribe to set topic if enabled */
    if (GET_CONF_MQTT_SUBSCRIBE(conf.mqtt.setting)) {
      /* Setup callback for incoming publish requests */
      mqtt_set_inpub_callback(client, mqttSubTopicCB, mqttSubDataCB, (void*)&mqttInTopic);

      /* Subscribe to topic with wildcard */
      err = mqtt_subscribe(client, MQTT_MAIN_TOPIC MQTT_SET_TOPIC "#", 1, mqttSubscribedCB, arg);

      if(err != ERR_OK) {
        DBG_MQTT_FUNC("MQTT ConnectionCB: Subscribe error: %d\r\n", err);
        SET_CONF_MQTT_SUBSCRIBE_ERROR(conf.mqtt.setting);
        pushToLogText("QES"); /* Subscribe error */
      } else {
        DBG_MQTT_FUNC("MQTT ConnectionCB: Subscribe OK\r\n");
        CLEAR_CONF_MQTT_SUBSCRIBE_ERROR(conf.mqtt.setting);
      }
    }

    /* Publish system state */
    pushToMqtt(typeSystem, 1, functionState);

    /* MQTT Home Assistant Discovery (if enabled) */
    if (GET_CONF_MQTT_HAD(conf.mqtt.setting)) {
      pushToMqttHAD(typeSystem, 0, functionHAD, 1);
    }

    /* Re-publish zone states */
    mqttRefreshZonesState();

  } else {
    /* Connection failed */
    DBG_MQTT_FUNC("MQTT ConnectionCB: Disconnected, reason: %d\r\n", status);
    SET_CONF_MQTT_CONNECT_ERROR(conf.mqtt.setting);

    if (!GET_CONF_MQTT_CONNECT_ERROR_LOG(conf.mqtt.setting)) {
      tmpLog[0] = 'Q'; tmpLog[1] = 'E';
      /* lwip mqtt_connection_status_t error code mapping */
      if (status <= 5 ) {
        tmpLog[2] = '0' + status;
      } else {
        tmpLog[2] = '6' + (uint8_t)(status - 256);
      }
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

  if(client == NULL) {
    DBG_MQTT_FUNC("MQTT mqttDoConnect: NULL client pointer\r\n");
    return;
  }

  DBG_MQTT_FUNC("MQTT mqttDoConnect\r\n");

  /* Resolve mqtt.address to IP address */
  if (!(ipaddr_aton(conf.mqtt.address, &mqtt_ip))) {
    /* Address is not in IP address format, try DNS resolution */
    err = netconn_gethostbyname_addrtype(conf.mqtt.address, &mqtt_ip, IPADDR_TYPE_V4);
    if (err != ERR_OK) {
      /* Host not found via DNS */
      DBG_MQTT_FUNC("MQTT DNS resolution failed: %d\r\n", err);
      SET_CONF_MQTT_ADDRESS_ERROR(conf.mqtt.setting);
      pushToLogText("QER");
      return;
    } else {
      DBG_MQTT_FUNC("MQTT Host resolved via DNS\r\n");
      CLEAR_CONF_MQTT_ADDRESS_ERROR(conf.mqtt.setting);
    }
  } else {
    /* Address is valid IP address format */
    DBG_MQTT_FUNC("MQTT Using IP address format\r\n");
    CLEAR_CONF_MQTT_ADDRESS_ERROR(conf.mqtt.setting);
  }

  /* Configure authentication if provided */
  if (conf.mqtt.user[0] != '\0') {
    mqttCI.client_user = &conf.mqtt.user[0];
  } else {
    mqttCI.client_user = NULL;
  }

  if (conf.mqtt.password[0] != '\0') {
    mqttCI.client_pass = &conf.mqtt.password[0];
  } else {
    mqttCI.client_pass = NULL;
  }

  /* Attempt MQTT connection */
  LOCK_TCPIP_CORE();
  err = mqtt_client_connect(client, &mqtt_ip, conf.mqtt.port, mqttConnectionCB,
                            LWIP_CONST_CAST(void*, &mqttCI), &mqttCI);
  UNLOCK_TCPIP_CORE();

  /* Check immediate return code */
  if(err != ERR_OK) {
    DBG_MQTT_FUNC("MQTT Connect error: %d\r\n", err);
    SET_CONF_MQTT_CONNECT_ERROR(conf.mqtt.setting);
    if (!GET_CONF_MQTT_CONNECT_ERROR_LOG(conf.mqtt.setting)) {
      pushToLogText("QEC");
      SET_CONF_MQTT_CONNECT_ERROR_LOG(conf.mqtt.setting);
    }
  } else {
    DBG_MQTT_FUNC("MQTT Connect initiated\r\n");
  }
}

#endif /* OHS_MQTT_FUNCTIONS_H_ */
