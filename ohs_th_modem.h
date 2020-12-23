/*
 * ohs_th_modem.h
 *
 *  Created on: 11. 2. 2020
 *      Author: adam
 */

#ifndef OHS_TH_MODEM_H_
#define OHS_TH_MODEM_H_

#ifndef MODEM_DEBUG
#define MODEM_DEBUG 1
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
          } while ((palReadPad(GPIOD, GPIOD_GSM_STATUS)) &&
                   (tmp < (uint8_t)(GPRS_PWR_ON_TIME/GPRS_PWR_DELAY)));
          if (tmp != 0) {
            gsmStatus = gprs_OK;
            DBG_MODEM(" started.\r\n");
            pushToLogText("MO");
          } else {
            gsmStatus = gprs_Failed;
            DBG_MODEM(" failed!\r\n");
            pushToLogText("ME");
          }
        } else {
          DBG_MODEM("Modem already on.\r\n");
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
        DBG_MODEM("Modem alive check: ");
        gprsIsAlive = gprsSendCmd(AT_is_alive);
        if (gprsIsAlive == 1) {
          DBG_MODEM("OK.\r\n");
          if (gprsSetSMS != 1) {
            gprsSetSMS = gprsSendCmd(AT_set_sms_to_text);                      // Set modem to text SMS format
            //DBG_MODEM("AT_set_sms_to_text: %d\r\n", gprsSetSMS);
            if (gprsSetSMS == 1) gprsSetSMS = gprsSendCmd(AT_set_sms_receive); // Set modem to dump SMS to serial
            resp = gprsSendCmdWR(AT_model_info, (uint8_t*)gprsModemInfo, sizeof(gprsModemInfo));      // Get model
            resp = gprsSendCmd(AT_set_ATD);
          }
          resp = gprsSendCmdWRI(AT_registered, tempText, sizeof(tempText), 3);
          if (resp > 0) gprsReg = strtol((char*)tempText, NULL, 10);
          //DBG_MODEM("gprsReg: %d, %d\r\n", gprsReg, resp);
          resp = gprsSendCmdWRI(AT_signal_strength, tempText, sizeof(tempText), 2);
          if (resp > 0) gprsStrength = 4 + ((strtol((char*)tempText, NULL, 10)) * 3);
          //DBG_MODEM("gprsStrength: %d, %d\r\n", gprsStrength, resp);
          resp = gprsSendCmdWR(AT_system_info, (uint8_t*)gprsSystemInfo, sizeof(gprsSystemInfo));
          //DBG_MODEM("System: %s, %d\r\n", gprsSystemInfo, resp);
        } else {
          gprsReg = 4; gprsStrength = 0; gprsSetSMS = 0;
          gsmStatus = gprs_ForceReset;
          DBG_MODEM("Not responding.\r\n");
        }

        // if modem registration changes log it
        if (gprsLastStatus != gprsReg) {
          gprsLastStatus = gprsReg;
          tmpLog[0] = 'M'; tmpLog[1] = gprsReg; tmpLog[2] = gprsStrength;  pushToLog(tmpLog, 3);
        }
      }

      // Stop modem if requested
      if (gsmStatus == gprs_ForceReset) {
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
        } while ((!palReadPad(GPIOD, GPIOD_GSM_STATUS)) &&
                 (tmp < (uint8_t)(GPRS_PWR_OFF_TIME/GPRS_PWR_DELAY)));
        gprsLastStatus = DUMMY_NO_VALUE;
        if (tmp != 0) {
          DBG_MODEM(" stopped.\r\n");
          gsmStatus = gprs_NotInitialized;
          pushToLogText("MF");
        } else {
          DBG_MODEM(" failed!\r\n");
          gsmStatus = gprs_Failed;
          pushToLogText("ME");
        }
      }

      // Read incoming SMS or missed messages
      while(gprsReadMsg((uint8_t*)gprsSmsText, sizeof(gprsSmsText))) {
        DBG_MODEM("Modem: %s.\r\n", gprsSmsText);
      }

      chBSemSignal(&gprsSem);
    } // Semaphore is free

    chThdSleepMilliseconds(1000);
    counter++;
  }
}

#endif /* OHS_TH_MODEM_H_ */
