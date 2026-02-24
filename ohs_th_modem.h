/*
 * ohs_th_modem.h
 *
 *  Created on: 11. 2. 2020
 *      Author: adam
 */

#ifndef OHS_TH_MODEM_H_
#define OHS_TH_MODEM_H_

#include <string.h>
#include "cmd_dispatcher.h"

#ifndef MODEM_DEBUG
#define MODEM_DEBUG 0
#endif

#if MODEM_DEBUG
#define DBG_MODEM(...) {chprintf(console, __VA_ARGS__);}
#else
#define DBG_MODEM(...)
#endif

// Modem power on/off definitions in milliseconds
#define GPRS_PWR_DELAY     250   // Power On delay
#define GPRS_PWR_ON_TIME   20000 // Power On wait time, SIM7600 typical 16s
#define GPRS_PWR_ON_PULSE  500   // Power On impulse, SIM7600 typical 0.5 sec.
#define GPRS_PWR_OFF_TIME  35000 // Power Off wait time, SIM7600 typical 28s
#define GPRS_PWR_OFF_PULSE 3000  // Power Off impulse, SIM7600 min 2.5 sec.
// Sanity checks
#if (GPRS_PWR_ON_TIME/GPRS_PWR_DELAY) > 255
  #error Start up time is larger then uint8_t size!
#endif
#if (GPRS_PWR_OFF_TIME/GPRS_PWR_DELAY) > 255
  #error Stop time is larger then uint8_t size!
#endif
// SMS commands handlers
#include <sms_cmd_handler.h>
/*
 * Modem services thread
 */
static THD_WORKING_AREA(waModemThread, 256);
static THD_FUNCTION ( ModemThread, arg) {
  chRegSetThreadName(arg);
  uint8_t counter = 0, tmp;
  uint8_t gprsLastStatus = DUMMY_NO_VALUE; // get status on start
  int8_t resp = 0;
  uint8_t smsIndex, contactIndex;
  uint8_t tempText[16]; // Need to hold whole response of AT command
  char *pch;

  /* Initialize help handler with command table */
  cmdInitHelp(smsTopCommands, ARRAY_COUNT(smsTopCommands));

  while (true) {
    // Check is GPRS is free
    if (chBSemWaitTimeout(&gprsSem, TIME_IMMEDIATE) == MSG_OK) {

      // Start modem
      if ((counter == 15) && (modemStatus != gprs_OK) && (modemStatus != gprs_Failed)) {
        // GPIOD_GSM_STATUS check
        if (palReadPad(GPIOD, GPIOD_GSM_STATUS)) {
          DBG_MODEM("Starting modem: ");
          palSetPad(GPIOD, GPIOD_GSM_PWRKEY);
          chThdSleepMilliseconds(GPRS_PWR_ON_PULSE);
          palClearPad(GPIOD, GPIOD_GSM_PWRKEY);

          // Wait for status high
          tmp = 0;
          do {
            DBG_MODEM(".");
            chThdSleepMilliseconds(GPRS_PWR_DELAY);
            tmp++;
          } while ((palReadPad(GPIOD, GPIOD_GSM_STATUS)) && (tmp < (uint8_t) (GPRS_PWR_ON_TIME / GPRS_PWR_DELAY)));
          if (!palReadPad(GPIOD, GPIOD_GSM_STATUS)) {
            modemStatus = gprs_OK;
            DBG_MODEM(" started.\r\n");
            pushToLogText("MO");
          } else {
            modemStatus = gprs_Failed;
            DBG_MODEM(" failed!\r\n");
            pushToLogText("ME");
          }
        } else {
          DBG_MODEM("Modem already on.\r\n");
          modemStatus = gprs_OK;
        }
      }

      // Dummy query to initialize modem UART at start, since first AT reply is null
      if ((counter == 25) && (gprsLastStatus == DUMMY_NO_VALUE) && (modemStatus == gprs_OK)) {
        resp = gprsSendCmd(AT_is_alive);
        chThdSleepMilliseconds(AT_DELAY);
        gprsFlushRX();
      }

      // AT checks
      if ((counter == 30) && (modemStatus == gprs_OK)) {
        DBG_MODEM("Modem alive check: ");
        modemIsAlive = gprsSendCmd(AT_is_alive);
        if (modemIsAlive == 1) {
          DBG_MODEM("OK.\r\n");
          if (modemSetSMS != 1) {
            modemSetSMS = gprsSendCmd(AT_set_sms_to_text); // Set modem to text SMS format
            //DBG_MODEM("AT_set_sms_to_text: %d\r\n", gprsSetSMS);
            if (modemSetSMS == 1) modemSetSMS = gprsSendCmd(AT_set_sms_store); // Set modem SMS receive mode
            resp = gprsSendCmdWR(AT_model_info, (uint8_t*) modemModelInfo, sizeof(modemModelInfo)); // Get model
//            resp = gprsSendCmdWR (AT_get_sms_storage, (uint8_t*) modemSmsText,
//                                  sizeof(modemSmsText)); // Get SMS storage info
//            DBG_MODEM("SMS Storage: %s, %d\r\n", modemSmsText, resp);
            resp = gprsSendCmd(AT_set_ATD);
          }
          resp = gprsSendCmdWRI(AT_registered, tempText, sizeof(tempText), 3);
          if (resp > 0) modemReg = strtol((char*) tempText, NULL, 10);
          DBG_MODEM("Modem reg: %d, (%d)\r\n", modemReg, resp);
          resp = gprsSendCmdWRI(AT_signal_strength, tempText, sizeof(tempText), 2);
          if (resp > 0) {
            resp = (strtol((char*) tempText, NULL, 10));
            if (resp > 31) modemSigStrength = 0;
            else modemSigStrength = 7 + (resp * 3);
          }
          //DBG_MODEM("gprsStrength: %d, %d\r\n", gprsStrength, resp);
          resp = gprsSendCmdWR(AT_system_info, (uint8_t*) modemSystemInfo, sizeof(modemSystemInfo));
          //DBG_MODEM("System: %s, %d\r\n", gprsSystemInfo, resp);
        } else {
          modemReg = 4;
          modemSigStrength = 0;
          modemSetSMS = 0;
          modemStatus = gprs_ForceReset;
          DBG_MODEM("Not responding.\r\n");
        }

        // if modem registration changes log it
        if (gprsLastStatus != modemReg) {
          gprsLastStatus = modemReg;
          tmpLog[0] = 'M';
          tmpLog[1] = modemReg;
          tmpLog[2] = modemSigStrength;
          pushToLog(tmpLog, 3);
        }
      }

      // Stop modem if requested
      if (modemStatus == gprs_ForceReset) {
        DBG_MODEM("Stopping modem: ");
        palSetPad(GPIOD, GPIOD_GSM_PWRKEY);
        chThdSleepMilliseconds(GPRS_PWR_OFF_PULSE);
        palClearPad(GPIOD, GPIOD_GSM_PWRKEY);

        // Wait for status low
        tmp = 0;
        do {
          DBG_MODEM(".");
          chThdSleepMilliseconds(GPRS_PWR_DELAY);
          tmp++;
        } while ((!palReadPad(GPIOD, GPIOD_GSM_STATUS)) && (tmp < (uint8_t) (GPRS_PWR_OFF_TIME / GPRS_PWR_DELAY)));
        gprsLastStatus = DUMMY_NO_VALUE;
        if (tmp != 0) {
          DBG_MODEM(" stopped.\r\n");
          modemStatus = gprs_NotInitialized;
          pushToLogText("MF");
        } else {
          DBG_MODEM(" failed!\r\n");
          modemStatus = gprs_Failed;
          pushToLogText("ME");
        }
      }

      // Read incoming SMS or missed messages
      while (gprsReadMsg((uint8_t*) modemSmsText, sizeof(modemSmsText))) {
        DBG_MODEM("Modem: %s.\r\n", modemSmsText);

        // Check for incoming SMS
        if (memcmp(AT_SMS_received, modemSmsText, strlen(AT_SMS_received)) == 0) {
          // Get index
          pch = strtok((char*) modemSmsText, " ,.-");
          resp = 1;
          while (pch != NULL) {
            if (resp == 3) break;
            pch = strtok(NULL, " ,.-");
            resp++;
          }
          smsIndex = strtol(pch, NULL, 10);
          DBG_MODEM("New SMS at: %d\r\n", smsIndex);
          // Read SMS
          resp = gprsGetSMS(smsIndex, (uint8_t*) tempText, sizeof(tempText), (uint8_t*) modemSmsText,
              sizeof(modemSmsText));
          DBG_MODEM("SMS number: %s, text: %s, resp: %d\r\n", tempText, modemSmsText, resp);
          // Check SMS number is authorized
          contactIndex = isPhoneNumberAuthorized((char*) tempText);
          // Process SMS if authorized and SMS text set
          if ((resp >= 0) && (modemSetSMS)) {
            DBG_MODEM("SMS from contact: %d\r\n", contactIndex + 1); // Index at 0
            // Process SMS text
            resp = cmdProcess(modemSmsText, smsTopCommands, ARRAY_COUNT(smsTopCommands), logText, LOG_TEXT_LENGTH);
            DBG_MODEM("SMS command processed, status: %d, response: %s\r\n", resp, logText);
            // If not an ERROR send reply
            if (resp != CMD_ERROR) {
              // Send reply SMS
              resp = sendSMSToContact(contactIndex, logText);
              DBG_MODEM("Reply SMS sent, status: %d\r\n", resp);
            }
          } else {
            DBG_MODEM("SMS number NOT authorized!\r\n");
          }
          // Delete SMS anyway
          resp = gprsDeleteSMS(smsIndex); // Delete SMS after read
          DBG_MODEM("Delete SMS index %d, resp: %d\r\n", smsIndex, resp);
        }
      }

      chBSemSignal(&gprsSem);
    } // Semaphore is free

    chThdSleepMilliseconds(1000);
    counter++;
  }
}

#endif /* OHS_TH_MODEM_H_ */
