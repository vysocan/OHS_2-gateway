/*
 * ohs_mqtt_functions.h
 *
 *  Created on: Dec 8, 2020
 *      Author: vysocan
 */
#ifndef OHS_MQTT_FUNCTIONS_H_
#define OHS_MQTT_FUNCTIONS_H_

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
    chprintf(console, "MQTT Publish error: %d\n", result);
    pushToLogText("QEP"); // Publish error
  }
}
/*
 * MQTT subscribe topic callback
 */
static void mqttIncomingPublishCB(void *arg, const char *topic, u32_t tot_len) {

  chprintf(console, "Incoming publish at topic %s with total length %u\r\n", topic, (unsigned int)tot_len);
  strncpy(arg, topic, LWIP_MIN(strlen(topic), MQTT_TOPIC_LENGTH - 1));
}
/*
 * MQTT subscribe data callback
 */
static void mqttIncomingDataCB(void *arg, const u8_t *data, u16_t len, u8_t flags) {

  chprintf(console, "Incoming publish payload with length %d, flags %u. Arg: %s\r\n", len, (unsigned int)flags, arg);

  // Last fragment of payload received (or whole part if payload fits receive buffer
  // See MQTT_VAR_HEADER_BUFFER_LEN)
  if(flags & MQTT_DATA_FLAG_LAST) {
    // Decode topic string
    // TODO OHS Make some useful actions based on topic.
    if(strcmp(arg, "print_payload") == 0) {
      chprintf(console, "MQTT subscribe data: %s\r\n", (const char *)data);
    }
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
    chprintf(console, "MQTT CB Subscibe error: %d\r\n", result);
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
    chprintf(console, "MQTT connect CB: Connected\r\n");
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
      err = mqtt_subscribe(client, MQTT_MAIN_TOPIC MQTT_SET_TOPIC , 1, mqttSubscribeCB, arg);

      if(err != ERR_OK) {
        chprintf(console, "MQTT connect CB: Subscribe error: %d\r\n", err);
        SET_CONF_MQTT_SUBSCRIBE_ERROR(conf.mqtt.setting);
        pushToLogText("QES"); // Subscribe error
      }
    } // Subscribe to set topic
  } else {
    chprintf(console, "MQTT CB: Disconnected, reason: %d\r\n", status);
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
      chprintf(console, "MQTT Connect error: %d\r\n", err);
      SET_CONF_MQTT_CONNECT_ERROR(conf.mqtt.setting);
      if (!GET_CONF_MQTT_CONNECT_ERROR_LOG(conf.mqtt.setting)) {
        pushToLogText("QEC");
        SET_CONF_MQTT_CONNECT_ERROR_LOG(conf.mqtt.setting);
      }
    }
  }
}

#endif /* OHS_MQTT_FUNCTIONS_H_ */
