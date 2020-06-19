/*
 * ohs_th_modem.h
 *
 *  Created on: 11. 2. 2020
 *      Author: adam
 */

#ifndef OHS_TH_MODEM_H_
#define OHS_TH_MODEM_H_

#define GPRS_PWR_ON_DELAY  500  // Power On impulse
#define GPRS_PWR_OFF_DELAY 3000 // Power Off impulse should be > 2.5 sec.
/*
 * Modem services thread
 */
static THD_WORKING_AREA(waModemThread, 256);
static THD_FUNCTION(ModemThread, arg) {
  chRegSetThreadName(arg);
  uint8_t counter = 0, tmp;
  uint8_t gprsLastStatus = DUMMY_NO_VALUE; // get status on start
  int8_t resp = 0;
  uint8_t tempText[16]; // Need to hold whole response of AT command

  while (true) {
    // Check is GPRS is free
    if (chBSemWaitTimeout(&gprsSem, TIME_IMMEDIATE) == MSG_OK) {

      // Start modem
      if ((counter == 15) && (gsmStatus != gprs_OK) && (gsmStatus != gprs_Failed)) {
        // GPIOD_GSM_STATUS check
        if (palReadPad(GPIOD, GPIOD_GSM_STATUS)) {
          chprintf(console, "Starting modem: ");
          palSetPad(GPIOD, GPIOD_GSM_PWRKEY);
          chThdSleepMilliseconds(GPRS_PWR_ON_DELAY);
          palClearPad(GPIOD, GPIOD_GSM_PWRKEY);

          // Wait for status high
          tmp = 0;
          do {
            chprintf(console, ".");
            chThdSleepMilliseconds(AT_DELAY);
            tmp++;
          } while ((palReadPad(GPIOD, GPIOD_GSM_STATUS)) && (tmp != 0));
          if (tmp != 0) {
            gsmStatus = gprs_OK;
            chprintf(console, " started.\r\n");
            pushToLogText("MO");
          } else {
            gsmStatus = gprs_Failed;
            chprintf(console, " failed!\r\n");
            pushToLogText("ME");
          }
        } else {
          chprintf(console, "Modem already on.\r\n");
          gsmStatus = gprs_OK;
        }
      }

      // Dummy query to initialize modem UART at start, since first AT reply is null
      if ((counter == 25) && (gprsLastStatus == DUMMY_NO_VALUE) && (gsmStatus == gprs_OK)) {
        resp = gprsSendCmd(AT_is_alive);
        chThdSleepMilliseconds(AT_DELAY);
        gprsFlushRX();
      }

      // AT checks
      if ((counter == 30) && (gsmStatus == gprs_OK)) {
        chprintf(console, "Modem alive check.\r\n"); chThdSleepMilliseconds(50);
        gprsIsAlive = gprsSendCmd(AT_is_alive);
        if (gprsIsAlive == 1) {
          if (gprsSetSMS != 1) {
            gprsSetSMS = gprsSendCmd(AT_set_sms_to_text);                      // Set modem to text SMS format
            chprintf(console, "AT_set_sms_to_text: %d\r\n", gprsSetSMS);
            if (gprsSetSMS == 1) gprsSetSMS = gprsSendCmd(AT_set_sms_receive); // Set modem to dump SMS to serial
            resp = gprsSendCmdWR(AT_model_info, (uint8_t*)gprsModemInfo, sizeof(gprsModemInfo));      // Get model
            resp = gprsSendCmd(AT_set_ATD);
          }
          resp = gprsSendCmdWRI(AT_registered, tempText, sizeof(tempText), 3);
          if (resp > 0) gprsReg = strtol((char*)tempText, NULL, 10);
          chprintf(console, "gprsReg: %d, %d\r\n", gprsReg, resp);
          resp = gprsSendCmdWRI(AT_signal_strength, tempText, sizeof(tempText), 2);
          if (resp > 0) gprsStrength = 4 + ((strtol((char*)tempText, NULL, 10)) * 3);
          chprintf(console, "gprsStrength: %d, %d\r\n", gprsStrength, resp);
          resp = gprsSendCmdWR(AT_system_info, (uint8_t*)gprsSystemInfo, sizeof(gprsSystemInfo));
          chprintf(console, "System: %s, %d\r\n", gprsSystemInfo, resp);
        } else {
          gprsReg = 4; gprsStrength = 0; gprsSetSMS = 0;
          gsmStatus = gprs_ForceReset;
          chprintf(console, "Modem not responding.\r\n");
        }

        // if modem registration changes log it
        if (gprsLastStatus != gprsReg) {
          gprsLastStatus = gprsReg;
          tmpLog[0] = 'M'; tmpLog[1] = gprsReg; tmpLog[2] = gprsStrength;  pushToLog(tmpLog, 3);
        }
      }

      // Stop modem if requested
      if (gsmStatus == gprs_ForceReset) {
        chprintf(console, "Stopping modem: ");
        palSetPad(GPIOD, GPIOD_GSM_PWRKEY);
        chThdSleepMilliseconds(GPRS_PWR_OFF_DELAY);
        palClearPad(GPIOD, GPIOD_GSM_PWRKEY);

        // Wait for status low
        tmp = 0;
        do {
          chprintf(console, ".");
          chThdSleepMilliseconds(AT_DELAY*2); // It takes twice much time to stop
          tmp++;
        } while ((!palReadPad(GPIOD, GPIOD_GSM_STATUS)) && (tmp != 0));
        gsmStatus = gprs_NotInitialized;
        gprsLastStatus = DUMMY_NO_VALUE;
        if (tmp != 0) {
          chprintf(console, " stopped.\r\n");
          pushToLogText("MF");
        } else {
          chprintf(console, " failed!\r\n");
          pushToLogText("ME");
        }
      }

      // Read incoming SMS or missed messages
      while(gprsReadMsg((uint8_t*)gprsSmsText, sizeof(gprsSmsText))) {
        chprintf(console, "Modem: %s.\r\n", gprsSmsText);
      }

      chBSemSignal(&gprsSem);
    } // Semaphore is free

    chThdSleepMilliseconds(1000);
    counter++;
  }
}

#endif /* OHS_TH_MODEM_H_ */
