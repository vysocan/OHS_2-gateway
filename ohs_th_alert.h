/*
 * ohs_th_alert.h
 *
 *  Created on: 22. 2. 2020
 *      Author: adam
 */

#ifndef OHS_TH_ALERT_H_
#define OHS_TH_ALERT_H_

struct smtp_send_request emailReqest;

// Explain a bit the result state
static const char *smtpResult[] = {
  "OK",
  "Err_UNKNOWN",
  "Err_CONNECT",
  "Err_HOSTNAME",
  "Err_CLOSED",
  "Err_TIMEOUT",
  "Err_SVR_RESP",
  "Err_MEM"
};
/*
 * SMTP result call back
 */
void my_smtp_result_fn(void *arg, u8_t smtp_result, u16_t srv_err, err_t err){
  if (smtp_result > ARRAY_SIZE(smtpResult)) smtp_result = 1;
  chprintf(console, "Mail (%u) sent with result: %s. Server response: %d, 0x%08x\r\n", arg,
           smtpResult[smtp_result], srv_err, err);
  chBSemSignal(&emailSem);
}
/*
 * Alert handling thread
 */
static THD_WORKING_AREA(waAlertThread, 512);
static THD_FUNCTION(AlertThread, arg) {
  chRegSetThreadName(arg);
  msg_t msg;
  alertEvent_t *inMsg;
  uint8_t groupNum;
  int8_t resp;
  err_t err; // lwip error type

  while (true) {
    msg = chMBFetchTimeout(&alert_mb, (msg_t*)&inMsg, TIME_INFINITE);
    if (msg == MSG_OK) {
      chprintf(console, "Alert: %s, Flags: %u, Text: ", inMsg->text, inMsg->flag);
      groupNum = decodeLog(inMsg->text, gprsSmsText, true);
      chprintf(console, "%s\r\n", gprsSmsText);

      // Do alerts by type
      for(uint8_t i = 0; i < ARRAY_SIZE(alertType); i++) {
        if (((inMsg->flag >> i) & 0b1) == 1) {
          switch (i) {
            case 0: // SMS
              for (uint8_t j = 0; j < CONTACTS_SIZE; j++) {
                if (GET_CONF_CONTACT_ENABLED(conf.contact[j].setting) &&
                    (isPhoneNum(&conf.contact[j].phone[0])) &&
                    ((GET_CONF_CONTACT_GROUP(conf.contact[j].setting) == groupNum) ||
                     (GET_CONF_CONTACT_IS_GLOBAL(conf.contact[j].setting)))) {
                  // Wait for GPRS
                  if (chBSemWait(&gprsSem) == MSG_OK) {
                    resp = gprsSendSMSBegin(conf.contact[j].phone);
                    chprintf(console, "SMS begin: %d\r\n", resp);
                    if (resp == 1) {
                      chprintf(console, "SMS end: %d\r\n", gprsSendSMSEnd(gprsSmsText));
                    }
                    chBSemSignal(&gprsSem); // Release semaphore
                  }
                }
              }
              break;
            case 1: // Page
              for (uint8_t j = 0; j < CONTACTS_SIZE; j++) {
                if (GET_CONF_CONTACT_ENABLED(conf.contact[j].setting) &&
                    (isPhoneNum(&conf.contact[j].phone[0])) &&
                    ((GET_CONF_CONTACT_GROUP(conf.contact[j].setting) == groupNum) ||
                     (GET_CONF_CONTACT_IS_GLOBAL(conf.contact[j].setting)))) {
                  // Wait for GPRS
                  if (chBSemWait(&gprsSem) == MSG_OK) {
                    chsnprintf(gprsSmsText, sizeof(gprsSmsText), "%s%s;", AT_D, conf.contact[j].phone);
                    resp = gprsSendCmd(gprsSmsText);
                    chprintf(console, "Page begin: %d\r\n", resp);
                    if (resp == 1) {
                      chThdSleepSeconds(25); // RING ... RING ...
                      chprintf(console, "Page end: %d\r\n", gprsSendCmd(AT_H));
                    }
                    chBSemSignal(&gprsSem); // Release semaphore
                  }
                }
              }
              break;
            case 2: // Email
              for (uint8_t j = 0; j < CONTACTS_SIZE; j++) {
                if (GET_CONF_CONTACT_ENABLED(conf.contact[j].setting) &&
                    ((GET_CONF_CONTACT_GROUP(conf.contact[j].setting) == groupNum) ||
                     (GET_CONF_CONTACT_IS_GLOBAL(conf.contact[j].setting)))) {
                  if (chBSemWaitTimeout(&emailSem, TIME_IMMEDIATE) == MSG_OK) {
                    emailReqest.from = &conf.contact[j].email[0];
                    emailReqest.to   = &conf.contact[j].email[0];
                    emailReqest.subject = &gprsSmsText[0];
                    emailReqest.body = &gprsSmsText[0];
                    emailReqest.callback_fn = my_smtp_result_fn;
                    emailReqest.callback_arg = NULL;
                    emailReqest.static_data = 1;
                    LOCK_TCPIP_CORE();
                    smtp_send_mail_int(&emailReqest);
                    UNLOCK_TCPIP_CORE();
                    // Release semaphore semaphore is done in callback
                  } else {
                    chprintf(console, "Email error, pending.\r\n");
                  }
                }
              }
              break;
            case 3: // MQTT
              LOCK_TCPIP_CORE();
              resp = mqtt_client_is_connected(&mqtt_client); // retain as temp value
              UNLOCK_TCPIP_CORE();
              if (resp) {
                // Wait for free MQTT semaphore
                if (chBSemWaitTimeout(&mqttSem, TIME_MS2I(100)) == MSG_OK) {
                  // publish
                  LOCK_TCPIP_CORE();
                  err = mqtt_publish(&mqtt_client, MQTT_ALERT_TOPIC, &gprsSmsText[0], strlen(gprsSmsText),
                                     0, 0, mqttPubRequestCB, NULL);
                  UNLOCK_TCPIP_CORE();
                  if(err != ERR_OK) {
                    chprintf(console, "MQTT error: %d\r\n", err);
                    // Publish error
                    tmpLog[0] = 'Q'; tmpLog[1] = 'E'; tmpLog[2] = 'P'; tmpLog[3] = abs(err); pushToLog(tmpLog, 4);
                    // Release semaphore in case of publish error, as CB is not called
                    chBSemSignal(&mqttSem);
                  } else {
                    chprintf(console, "MQTT OK\r\n");
                    CLEAR_CONF_MQTT_SEMAPHORE_ERROR_LOG(conf.mqtt.setting);
                  }
                } else {
                  chprintf(console, "MQTT timeout!\r\n");
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
                chprintf(console, "MQTT not connected\r\n");
              }
              break;
            default: // NOP
              break;
          }
        }
      }
    } else {
      chprintf(console, "Alert ERROR\r\n");
    }
    chPoolFree(&alert_pool, inMsg);
  }
}


#endif /* OHS_TH_ALERT_H_ */
