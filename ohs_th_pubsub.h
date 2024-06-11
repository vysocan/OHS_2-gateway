/*
 * ohs_th_pubsub.h
 *
 *  Created on: Mar 13, 2024
 *      Author: vysocan
 */

#ifndef OHS_TH_PUBSUB_H_
#define OHS_TH_PUBSUB_H_

#ifndef OHS_PUBSUB_DEBUG
#define OHS_PUBSUB_DEBUG 1
#endif

#if OHS_PUBSUB_DEBUG
#define DBG_PUBSUB(...) {chprintf(console, __VA_ARGS__);}
#else
#define DBG_PUBSUB(...)
#endif

#define MAX(x , y)  (((x) > (y)) ? (x) : (y))
#define MIN(x , y)  (((x) < (y)) ? (x) : (y))

// FIXME Make this meaningful!
void cmpErr(void){
  DBG_PUBSUB("CMP: %s.", cmp_strerror(&cmp))
}

/*
 * PubSub TX thread
 *
 * Publish topics:
 * /run
 *   /zone
 *     /{#} - index of zone
 *       state {OK, alarm, tamper}
 *
 * /conf
 *   /zone
 *     /{#} - index of zone
 *       conf.zoneName
 *       conf.zone - internal settings
 *
 * ***
 * /state {On, Off} - Indicates if system is on
 * /group
 *   /{#} - index of group
 *     /name
 *     /state {disarmed, arming, armed_home, armed_away, triggered, disarming}
 * /sensor
 *   /{address} - node address like W:2:K:i:0
 *     /name
 *     /type
 *     /function
 *     /value
 */
static THD_WORKING_AREA(waPubSubTxThread, 512);
static THD_FUNCTION(PubSubTxThread, arg) {
  chRegSetThreadName(arg);

  uint8_t resp;
  msg_t msg;
  pubsubEvent_t *inMsg;
  char topic[30];

  while (true) {
    msg = chMBFetchTimeout(&pubsub_mb, (msg_t*)&inMsg, TIME_INFINITE);

    // We have message
    if (msg == MSG_OK) {
      DBG_PUBSUB("PubSub TX type: %d", (uint8_t)inMsg->type);
      DBG_PUBSUB(", # %d", inMsg->number);
      DBG_PUBSUB(", func: %d\r\n", (uint8_t)inMsg->function);

      // Wait for free PubSub semaphore
      DBG_PUBSUB("PubSub TX ");
      if (chBSemWaitTimeout(&pubsubSemTx, TIME_MS2I(100)) == MSG_OK) {
        DBG_PUBSUB("go ");
        // Prepare message
        cmp_mem_access_set_pos(&cmp_mem, 0);
        resp = 1;
        switch (inMsg->type) {
          case typeZone:
            chsnprintf(topic, sizeof(topic), "/run/%s/%d", text_zone, inMsg->number);
            switch (inMsg->function) {
              case functionState:
                if (!cmp_write_map(&cmp, 1)) { cmpErr(); }
                if (!cmp_write_str(&cmp, text_state, strlen(text_state))) { cmpErr(); }
                switch (zone[inMsg->number].lastEvent) {
                  case 'O':
                    if (!cmp_write_str(&cmp, text_OK, strlen(text_OK))) { cmpErr(); }
                    break;
                  case 'P':
                    if (!cmp_write_str(&cmp, text_alarm, strlen(text_alarm))) { cmpErr(); }
                    break;
                  default: // Tamper
                    if (!cmp_write_str(&cmp, text_tamper, strlen(text_tamper))) { cmpErr(); }
                  break;
                }
                break;
              default:
                DBG_PUBSUB(" typeZone undefined! ");
                break;
            }
            break; // typeZone
          case typeConfZone:
            chsnprintf(topic, sizeof(topic), "/conf/%s/%d", text_zone, inMsg->number);
            // Payload
            if (!cmp_write_map(&cmp, 11)) {cmpErr();}
            if (!cmp_write_str(&cmp, text_name, strlen(text_name))) { cmpErr(); }
            if (!cmp_write_str(&cmp, conf.zoneName[inMsg->number], strlen(conf.zoneName[inMsg->number]))) { cmpErr(); }
            if (!cmp_write_str(&cmp, text_type, strlen(text_type))) { cmpErr(); }
            if (!cmp_write_bool(&cmp, GET_CONF_ZONE_TYPE(conf.zone[inMsg->number]))) { cmpErr(); }
            if (!cmp_write_str(&cmp, text_present, strlen(text_present))) { cmpErr(); }
            if (!cmp_write_bool(&cmp, GET_CONF_ZONE_IS_PRESENT(conf.zone[inMsg->number]))) { cmpErr(); }
            if (!cmp_write_str(&cmp, text_mqtt, strlen(text_mqtt))) { cmpErr(); }
            if (!cmp_write_bool(&cmp, GET_CONF_ZONE_MQTT_PUB(conf.zone[inMsg->number]))) { cmpErr(); }
            if (!cmp_write_str(&cmp, text_balanced, strlen(text_balanced))) { cmpErr(); }
            if (!cmp_write_bool(&cmp, GET_CONF_ZONE_BALANCED(conf.zone[inMsg->number]))) { cmpErr(); }
            if (!cmp_write_str(&cmp, text_pir, strlen(text_pir))) { cmpErr(); }
            if (!cmp_write_bool(&cmp, GET_CONF_ZONE_PIR_AS_TMP(conf.zone[inMsg->number]))) { cmpErr(); }
            if (!cmp_write_str(&cmp, text_open, strlen(text_open))) { cmpErr(); }
            if (!cmp_write_bool(&cmp, GET_CONF_ZONE_OPEN_ALARM(conf.zone[inMsg->number]))) { cmpErr(); }
            if (!cmp_write_str(&cmp, text_home, strlen(text_home))) { cmpErr(); }
            if (!cmp_write_bool(&cmp, GET_CONF_ZONE_ARM_HOME(conf.zone[inMsg->number]))) { cmpErr(); }
            if (!cmp_write_str(&cmp, text_auth, strlen(text_auth))) { cmpErr(); }
            if (!cmp_write_uinteger(&cmp, GET_CONF_ZONE_AUTH_TIME(conf.zone[inMsg->number]))) { cmpErr(); }
            if (!cmp_write_str(&cmp, text_group, strlen(text_group))) { cmpErr(); }
            if (!cmp_write_uinteger(&cmp, GET_CONF_ZONE_GROUP(conf.zone[inMsg->number]))) { cmpErr(); }
            if (!cmp_write_str(&cmp, text_on, strlen(text_on))) { cmpErr(); }
            if (!cmp_write_bool(&cmp, GET_CONF_ZONE_ENABLED(conf.zone[inMsg->number]))) { cmpErr(); }
            break; // typeConfZone
          case typeConfUser:
            chsnprintf(topic, sizeof(topic), "/conf/%s/%d", text_user, inMsg->number);
            // Payload
            if (!cmp_write_map(&cmp, 6)) {cmpErr();}
            if (!cmp_write_str(&cmp, text_name, strlen(text_name))) { cmpErr(); }
            if (!cmp_write_str(&cmp, conf.contact[inMsg->number].name, strlen(conf.contact[inMsg->number].name))) { cmpErr(); }
            if (!cmp_write_str(&cmp, text_phone, strlen(text_phone))) { cmpErr(); }
            if (!cmp_write_str(&cmp, conf.contact[inMsg->number].phone, strlen(conf.contact[inMsg->number].phone))) { cmpErr(); }
            if (!cmp_write_str(&cmp, text_email, strlen(text_email))) { cmpErr(); }
            if (!cmp_write_str(&cmp, conf.contact[inMsg->number].email, strlen(conf.contact[inMsg->number].email))) { cmpErr(); }
            if (!cmp_write_str(&cmp, text_group, strlen(text_group))) { cmpErr(); }
            if (!cmp_write_uinteger(&cmp, GET_CONF_CONTACT_GROUP(conf.contact[inMsg->number].setting))) { cmpErr(); }
            if (!cmp_write_str(&cmp, text_on, strlen(text_all))) { cmpErr(); }
            if (!cmp_write_bool(&cmp, GET_CONF_CONTACT_IS_GLOBAL(conf.contact[inMsg->number].setting))) { cmpErr(); }
            if (!cmp_write_str(&cmp, text_on, strlen(text_on))) { cmpErr(); }
            if (!cmp_write_bool(&cmp, GET_CONF_CONTACT_ENABLED(conf.contact[inMsg->number].setting))) { cmpErr(); }
            break; // typeConfUser
          default:
            resp = 0;
            DBG_PUBSUB("PubSub publish undefined!\r\n");
            break;
        }
        // Do Publish
        if (resp == 1) {
          //chprintf((BaseSequentialStream*)&SD1, "%s=%s", topic, payload);
          resp = pubsubSend((uint8_t *)topic, strlen(topic), (uint8_t *)cmp_buffer, cmp_mem_access_get_pos(&cmp_mem));
          DBG_PUBSUB("resp: %d, topic: %s, payload: %.*s\r\n", resp, topic, cmp_mem.index, cmp_buffer);
        } else {
          // release semaphore
          chBSemSignal(&pubsubSemTx);
        }
        chPoolFree(&pubsub_pool, inMsg);
      } else {// semaphore
        DBG_PUBSUB("semaphore timeout!\r\n");
      }
    } else {
      DBG_PUBSUB("PubSub MB ERROR\r\n");
    }

  }
}
/*
 *
 */
// receiveZone
void receiveZone(uint8_t index) {
  char tmpStr[NAME_LENGTH];
  uint32_t size;
  uint8_t tmpU8;
  bool tmpBool;

  DBG_PUBSUB(": %d", index);
  if (!cmp_read_map(&cmp, &size)) { cmpErr(); return; }
  if (size != 11) { DBG_PUBSUB("CMP: map size not match."); return;}

  size = NAME_LENGTH;
  if (!cmp_read_str(&cmp, tmpStr, &size)) { cmpErr(); return; }
  if (strncmp(tmpStr, text_name, strlen(text_name)) != 0) {
    DBG_PUBSUB("CMP: %s not received.", text_name);
    return;
  }
  size = NAME_LENGTH;
  if (!cmp_read_str(&cmp, tmpStr, &size)) { cmpErr(); return; }
  if (size > NAME_LENGTH) { DBG_PUBSUB("CMP: %s too big", text_name); return;}
  else { strncpy(conf.zoneName[index], tmpStr, size); }

  size = NAME_LENGTH;
  if (!cmp_read_str(&cmp, tmpStr, &size)) { cmpErr(); return; }
  if (strncmp(tmpStr, text_type, strlen(text_type)) != 0) {
    DBG_PUBSUB("CMP: %s not received.", text_type);
    return;
  }
  if (!cmp_read_bool(&cmp, &tmpBool)) { cmpErr(); return; }
  else {
    if (tmpBool) { SET_CONF_ZONE_TYPE(conf.zone[index]); }
    else         { CLEAR_CONF_ZONE_TYPE(conf.zone[index]); }
  }

  size = NAME_LENGTH;
  if (!cmp_read_str(&cmp, tmpStr, &size)) { cmpErr(); return; }
  if (strncmp(tmpStr, text_present, strlen(text_present)) != 0) {
    DBG_PUBSUB("CMP: %s not received.", text_present);
    return;
  }
  if (!cmp_read_bool(&cmp, &tmpBool)) { cmpErr(); return; }
  else {
    if (tmpBool) { SET_CONF_ZONE_IS_PRESENT(conf.zone[index]); }
    else         { CLEAR_CONF_ZONE_IS_PRESENT(conf.zone[index]); }
  }

  size = NAME_LENGTH;
  if (!cmp_read_str(&cmp, tmpStr, &size)) { cmpErr(); return; }
  if (strncmp(tmpStr, text_mqtt, strlen(text_mqtt)) != 0) {
    DBG_PUBSUB("CMP: %s not received.", text_mqtt);
    return;
  }
  if (!cmp_read_bool(&cmp, &tmpBool)) { cmpErr(); return; }
  else {
    if (tmpBool) { SET_CONF_ZONE_MQTT_PUB(conf.zone[index]); }
    else         { CLEAR_CONF_ZONE_MQTT_PUB(conf.zone[index]); }
  }

  size = NAME_LENGTH;
  if (!cmp_read_str(&cmp, tmpStr, &size)) { cmpErr(); return; }
  if (strncmp(tmpStr, text_balanced, strlen(text_balanced)) != 0) {
    DBG_PUBSUB("CMP: %s not received.", text_balanced);
    return;
  }
  if (!cmp_read_bool(&cmp, &tmpBool)) { cmpErr(); return; }
  else {
    if (tmpBool) { SET_CONF_ZONE_BALANCED(conf.zone[index]); }
    else         { CLEAR_CONF_ZONE_BALANCED(conf.zone[index]); }
  }

  size = NAME_LENGTH;
  if (!cmp_read_str(&cmp, tmpStr, &size)) { cmpErr(); return; }
  if (strncmp(tmpStr, text_pir, strlen(text_pir)) != 0) {
    DBG_PUBSUB("CMP: %s not received.", text_pir);
    return;
  }
  if (!cmp_read_bool(&cmp, &tmpBool)) { cmpErr(); return; }
  else {
    if (tmpBool) { SET_CONF_ZONE_PIR_AS_TMP(conf.zone[index]); }
    else         { CLEAR_CONF_ZONE_PIR_AS_TMP(conf.zone[index]); }
  }

  size = NAME_LENGTH;
  if (!cmp_read_str(&cmp, tmpStr, &size)) { cmpErr(); return; }
  if (strncmp(tmpStr, text_open, strlen(text_open)) != 0) {
    DBG_PUBSUB("CMP: %s not received.", text_open);
    return;
  }
  if (!cmp_read_bool(&cmp, &tmpBool)) { cmpErr(); return; }
  else {
    if (tmpBool) { SET_CONF_ZONE_OPEN_ALARM(conf.zone[index]); }
    else         { CLEAR_CONF_ZONE_OPEN_ALARM(conf.zone[index]); }
  }

  size = NAME_LENGTH;
  if (!cmp_read_str(&cmp, tmpStr, &size)) { cmpErr(); return; }
  if (strncmp(tmpStr, text_home, strlen(text_home)) != 0) {
    DBG_PUBSUB("CMP: %s not received.", text_home);
    return;
  }
  if (!cmp_read_bool(&cmp, &tmpBool)) { cmpErr(); return; }
  else {
    if (tmpBool) { SET_CONF_ZONE_ARM_HOME(conf.zone[index]); }
    else         { CLEAR_CONF_ZONE_ARM_HOME(conf.zone[index]); }
  }

  size = NAME_LENGTH;
  if (!cmp_read_str(&cmp, tmpStr, &size)) { cmpErr(); return; }
  if (strncmp(tmpStr, text_auth, strlen(text_auth)) != 0) {
    DBG_PUBSUB("CMP: %s not received.", text_auth);
    return;
  }
  if (!cmp_read_u8(&cmp, &tmpU8)) { cmpErr(); return; }
  else {
    if (tmpU8 < 4) {
      SET_CONF_ZONE_AUTH_TIME(conf.zone[index], tmpU8);
    }
  }

  size = NAME_LENGTH;
  if (!cmp_read_str(&cmp, tmpStr, &size)) { cmpErr(); return; }
  if (strncmp(tmpStr, text_group, strlen(text_group)) != 0) {
    DBG_PUBSUB("CMP: %s not received.", text_group);
    return;
  }
  if (!cmp_read_u8(&cmp, &tmpU8)) { cmpErr(); return; }
  else {
    if (tmpU8 < ALARM_GROUPS) {
      SET_CONF_ZONE_GROUP(conf.zone[index], tmpU8);
    }
  }

  size = NAME_LENGTH;
  if (!cmp_read_str(&cmp, tmpStr, &size)) { cmpErr(); return; }
  if (strncmp(tmpStr, text_on, strlen(text_on)) != 0) {
    DBG_PUBSUB("CMP: %s not received.", text_on);
    return;
  }
  if (!cmp_read_bool(&cmp, &tmpBool)) { cmpErr(); return; }
  else {
    if (tmpBool) { SET_CONF_ZONE_ENABLED(conf.zone[index]); }
    else         { CLEAR_CONF_ZONE_ENABLED(conf.zone[index]); }
  }
}
// receiveUser
void receiveUser(uint8_t index) {
  char tmpStr[EMAIL_LENGTH];
  uint32_t size;
  uint8_t tmpU8;
  bool tmpBool;

  DBG_PUBSUB(": %d", index);
  if (!cmp_read_map(&cmp, &size)) { cmpErr(); return; }
  if (size != 6) { DBG_PUBSUB("CMP: map size not match."); return;}

  size = EMAIL_LENGTH;
  if (!cmp_read_str(&cmp, tmpStr, &size)) { cmpErr(); return; }
  if (strncmp(tmpStr, text_name, strlen(text_name)) != 0) {
    DBG_PUBSUB("CMP: %s not received.", text_name);
    return;
  }
  size = EMAIL_LENGTH;
  if (!cmp_read_str(&cmp, tmpStr, &size)) { cmpErr(); return; }
  if (size > NAME_LENGTH) { DBG_PUBSUB("CMP: %s too big", text_name); return;}
  else { strncpy(conf.contact[index].name, tmpStr, size); }

  size = EMAIL_LENGTH;
  if (!cmp_read_str(&cmp, tmpStr, &size)) { cmpErr(); return; }
  if (strncmp(tmpStr, text_phone, strlen(text_phone)) != 0) {
    DBG_PUBSUB("CMP: %s not received.", text_phone);
    return;
  }
  size = EMAIL_LENGTH;
  if (!cmp_read_str(&cmp, tmpStr, &size)) { cmpErr(); return; }
  if (size > PHONE_LENGTH) { DBG_PUBSUB("CMP: %s too big", text_phone); return;}
  else { strncpy(conf.contact[index].phone, tmpStr, size); }

  size = EMAIL_LENGTH;
  if (!cmp_read_str(&cmp, tmpStr, &size)) { cmpErr(); return; }
  if (strncmp(tmpStr, text_email, strlen(text_email)) != 0) {
    DBG_PUBSUB("CMP: %s not received.", text_email);
    return;
  }
  size = EMAIL_LENGTH;
  if (!cmp_read_str(&cmp, tmpStr, &size)) { cmpErr(); return; }
  if (size > EMAIL_LENGTH) { DBG_PUBSUB("CMP: %s too big", text_email); return;}
  else { strncpy(conf.contact[index].email, tmpStr, size); }

  size = EMAIL_LENGTH;
  if (!cmp_read_str(&cmp, tmpStr, &size)) { cmpErr(); return; }
  if (strncmp(tmpStr, text_group, strlen(text_group)) != 0) {
    DBG_PUBSUB("CMP: %s not received.", text_group);
    return;
  }
  if (!cmp_read_u8(&cmp, &tmpU8)) { cmpErr(); return; }
  else {
    if (tmpU8 < ALARM_GROUPS) {
      SET_CONF_CONTACT_GROUP(conf.contact[index].setting, tmpU8);
    }
  }

  size = EMAIL_LENGTH;
  if (!cmp_read_str(&cmp, tmpStr, &size)) { cmpErr(); return; }
  if (strncmp(tmpStr, text_all, strlen(text_all)) != 0) {
    DBG_PUBSUB("CMP: %s not received.", text_all);
    return;
  }
  if (!cmp_read_bool(&cmp, &tmpBool)) { cmpErr(); return; }
  else {
    if (tmpBool) { SET_CONF_CONTACT_IS_GLOBAL(conf.contact[index].setting); }
    else         { CLEAR_CONF_CONTACT_IS_GLOBAL(conf.contact[index].setting); }
  }

  size = EMAIL_LENGTH;
  if (!cmp_read_str(&cmp, tmpStr, &size)) { cmpErr(); return; }
  if (strncmp(tmpStr, text_on, strlen(text_on)) != 0) {
    DBG_PUBSUB("CMP: %s not received.", text_on);
    return;
  }
  if (!cmp_read_bool(&cmp, &tmpBool)) { cmpErr(); return; }
  else {
    if (tmpBool) { SET_CONF_CONTACT_ENABLED(conf.contact[index].setting); }
    else         { CLEAR_CONF_CONTACT_ENABLED(conf.contact[index].setting); }
  }
}
/*
 *
 */
static THD_WORKING_AREA(waPubSubRxThread, 512);
static THD_FUNCTION(PubSubRxThread, arg) {
  chRegSetThreadName(arg);
  uint8_t thisTopic[30];
  uint8_t resp, index;
  char *pch;
  time_t now;
  RTCDateTime ts;

  while (true) {
    if (chBSemWaitTimeout(&pubsubSemRx, TIME_INFINITE) == MSG_OK) {
      DBG_PUBSUB("PubSub RX: ");
      cmp_mem_access_set_pos(&cmp_mem, 0);
      resp = pubsubReceive(&thisTopic[0], sizeof(thisTopic), (uint8_t *)&cmp_buffer[0], sizeof(cmp_buffer));
      DBG_PUBSUB("%d, topic: %s, payload: %.*s\r\n", resp, thisTopic, cmp_mem.index, cmp_buffer);
      // Valid data
      if (resp) {
        // Decode topic string
        pch = strtok((char *)thisTopic, "/");
        DBG_PUBSUB("Parse: %s", pch);
        if (pch != NULL) {
          // Set topic
          if (strcmp(pch, text_set) == 0) {
            // Get next index from topic
            pch = strtok(NULL, "/");
            DBG_PUBSUB(" %s", pch);
            if (pch != NULL) {
              if (strcmp(pch, text_time) == 0) {
                // Set Time
                now = strtoul(&cmp_buffer[0], NULL, 0);
                convertUnixSecondToRTCDateTime(&ts, now);
                rtcSetTime(&RTCD1, &ts);
                DBG_PUBSUB(": %d", now);
              } else if (strcmp(pch, text_zone) == 0) {
                // Set conf zone
                pch = strtok(NULL, "/");
                if (pch != NULL) {
                  index = strtoul(pch, NULL, 0);
                  if (index < ALARM_ZONES) {
                    // call CMP parser
                    receiveZone(index);
                  }
                }
              } else if (strcmp(pch, text_user) == 0) {
                // Set conf user
                pch = strtok(NULL, "/");
                if (pch != NULL) {
                  index = strtoul(pch, NULL, 0);
                  if (index < CONTACTS_SIZE) {
                    // call CMP parser
                    receiveUser(index);
                  }
                }
              } else {
                DBG_PUBSUB(" %s", text_unknown);
              }
            }
          }
          // Get topic
          else if (strcmp(pch, text_get) == 0) {
            // Get next index from topic
            pch = strtok(NULL, "/");
            DBG_PUBSUB(" %s", pch);
            if (pch != NULL) {
              if (strcmp(pch, text_zone) == 0) {
                // Zone
                pch = strtok(NULL, "/");
                if (pch != NULL) {
                  resp = strtoul(pch, NULL, 0) - 1;
                  if (resp < ALARM_ZONES) {
                    DBG_PUBSUB(", # %d", resp + 1);
                    pushToPubSub(typeConfZone, resp, functionAll);
                    pushToPubSub(typeZone, resp, functionState);
                  }
                }
              } else if (strcmp(pch, text_zones) == 0) {
                // Zones
                for (uint8_t i=0; i < ALARM_ZONES ; i++) {
                  // Send all present zones
                  if (GET_CONF_ZONE_IS_PRESENT(conf.zone[i])) {
                    pushToPubSub(typeConfZone, i, functionAll);
                    pushToPubSub(typeZone, i, functionState);
                  }
                }
              } else if (strcmp(pch, text_users) == 0) {
                // Zones
                for (uint8_t i=0; i < CONTACTS_SIZE; i++) {
                  // Send all contacts
                  pushToPubSub(typeConfUser, i, functionAll);
                }
              } else {
                DBG_PUBSUB(" %s", text_unknown);
              }
            }
            // ... topic
          }
          // Unknown topics
          else {
            DBG_PUBSUB(" %s", text_unknown);
          }
        } // !NULL
      } // resp = 1
      // End of parse
      DBG_PUBSUB(".\r\n");
    } else {// semaphore
      DBG_PUBSUB("semaphore timeout!\r\n");
    }
  }
}

#endif /* OHS_TH_PUBSUB_H_ */
