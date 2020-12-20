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
static THD_WORKING_AREA(waAlertThread, 320);
static THD_FUNCTION(AlertThread, arg) {
  chRegSetThreadName(arg);
  msg_t msg;
  alertEvent_t *inMsg;
  uint8_t groupNum;
  int8_t resp;

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
                    emailReqest.static_data = 1;
                    smtp_send_mail_int(&emailReqest);
                    // Release semaphore semaphore is done in callback
                  } else {
                    chprintf(console, "Email error, pending.\r\n");
                  }
                }
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
