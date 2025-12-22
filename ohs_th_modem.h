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
 *
 */
/* Leaf handlers - now receive zone id as last token */
/* GET SYSTEM STATUS */
static cmd_status_t handle_get_system_status (const char *tokens[],
                                              uint8_t count, char *result,
                                              uint16_t result_len) {
  (void) tokens;
  (void) count;
  uint16_t len = 0;
  safe_strcat (result, &len, result_len,
               "SYSTEM RUNNING | Uptime: 1234s | Temp: 42C | CPU: 45%");
  return CMD_OK;
}

/* GET ZONE STATUS <zone_id> */
static cmd_status_t handle_get_zone_status (const char *tokens[], uint8_t count,
                                            char *result, uint16_t result_len) {
  if (count < 1) {
    uint16_t len = 0;
    safe_strcat (result, &len, result_len, "Usage: GET ZONE STATUS <zone_id>");
    return CMD_INVALID_ARGS;
  }

  const char *zone_id = tokens[0];
  uint16_t len = 0;
  safe_strcat (result, &len, result_len, "ZONE ");
  safe_strcat (result, &len, result_len, zone_id);
  safe_strcat (result, &len, result_len,
               " STATUS | State: ACTIVE | Sensor: 23.5C");
  return CMD_OK;
}

/* GET ZONE SENSOR <zone_id> */
static cmd_status_t handle_get_zone_sensor (const char *tokens[], uint8_t count,
                                            char *result, uint16_t result_len) {
  if (count < 1) {
    uint16_t len = 0;
    safe_strcat (result, &len, result_len, "Usage: GET ZONE SENSOR <zone_id>");
    return CMD_INVALID_ARGS;
  }

  const char *zone_id = tokens[0];
  uint16_t len = 0;
  safe_strcat (result, &len, result_len, "ZONE ");
  safe_strcat (result, &len, result_len, zone_id);
  safe_strcat (result, &len, result_len,
               " SENSOR | Raw: 1023 | Filtered: 23.5C");
  return CMD_OK;
}

/* SET <anything> */
static cmd_status_t handle_set_generic (const char *tokens[], uint8_t count,
                                        char *result, uint16_t result_len) {
  uint16_t len = 0;
  safe_strcat (result, &len, result_len, "SET ");
  for (uint8_t i = 0; i < count; i++) {
    safe_strcat (result, &len, result_len, tokens[i]);
    safe_strcat (result, &len, result_len, " ");
  }
  safe_strcat (result, &len, result_len, "| Configuration updated");
  return CMD_OK;
}
/* === HELP SYSTEM - Recursive tree traversal ================================= */

/* Forward declaration of top_commands */
extern const cmd_entry_t top_commands[];

/* Append help for a single command entry */
static void append_cmd_help (const cmd_entry_t *cmd, uint8_t indent_level,
                             char *result, uint16_t *result_len,
                             uint16_t result_max) {
  /* Add indentation */
  for (uint8_t i = 0; i < indent_level; i++) {
    safe_strcat (result, result_len, result_max, "  ");
  }

  safe_strcat (result, result_len, result_max, cmd->name);

  /* Add help text if available */
  if (cmd->help && cmd->help[0]) {
    safe_strcat (result, result_len, result_max, " - ");
    safe_strcat (result, result_len, result_max, cmd->help);
  }
  safe_strcat (result, result_len, result_max, "\n");
}

/* Recursively traverse command tree and append help */
static void append_tree_help (const cmd_entry_t *table, uint8_t table_count,
                              uint8_t indent_level, char *result,
                              uint16_t *result_len, uint16_t result_max) {
  for (uint8_t i = 0; i < table_count && *result_len < result_max - 100; i++) {
    const cmd_entry_t *cmd = &table[i];

    /* Add this command */
    append_cmd_help (cmd, indent_level, result, result_len, result_max);

    /* Recurse into subcommands */
    if (cmd->sub && cmd->sub_count > 0) {
      append_tree_help (cmd->sub, cmd->sub_count, indent_level + 1, result,
                        result_len, result_max);
    }
  }
}

/* Main HELP handler - traverses entire command tree */
static cmd_status_t handle_help (const char *tokens[], uint8_t count,
                                 char *result, uint16_t result_len) {
  (void) tokens;
  (void) count;

  uint16_t len = 0;

  safe_strcat (result, &len, result_len, "=== Available Commands ===\n\n");

  /* Traverse top-level commands */
  append_tree_help (top_commands, 3, 0, result, &len, result_len);

  safe_strcat (result, &len, result_len, "\n");
  safe_strcat (result, &len, result_len,
               "Use partial commands for subcommand help (e.g. 'GET ZONE')\n");

  return CMD_OK;
}

/* === Static command tables ================================================= */
/* SYSTEM subcommands */
static const cmd_entry_t get_system_sub[] = { { "STATUS",
    handle_get_system_status, "Get system status", NULL, 0 } };

/* ZONE subcommands */
static const cmd_entry_t get_zone_sub[] = { { "STATUS", handle_get_zone_status,
    "Get zone status", NULL, 0 }, { "SENSOR", handle_get_zone_sensor,
    "Get zone sensor data", NULL, 0 } };

/* GET subcommands */
static const cmd_entry_t get_sub[] = { { "SYSTEM", NULL, "System information",
    get_system_sub, 1 }, { "ZONE", NULL,
    "Zone information (GET ZONE <subcmd> <zone_id>)", get_zone_sub, 2 } };

/* Top-level commands */
const cmd_entry_t top_commands[] = {
    { "GET",  NULL,         "Read status/configuration", get_sub, 2 },
    { "SET",  handle_set_generic, "Modify configuration", NULL, 0 },
    { "HELP", handle_help,  "Show this help", NULL, 0 }
};

const cmd_dispatcher_t dispatcher = {
    .commands = top_commands,
    .cmd_count = 3,
    .context = "ROOT"
};
/*
 * Modem services thread
 */
static THD_WORKING_AREA(waModemThread, 256);
static THD_FUNCTION ( ModemThread, arg) {
  chRegSetThreadName (arg);
  uint8_t counter = 0, tmp;
  uint8_t gprsLastStatus = DUMMY_NO_VALUE; // get status on start
  int8_t resp = 0;
  uint8_t smsIndex;
  uint8_t tempText[16]; // Need to hold whole response of AT command
  char *pch;

  while (true) {
    // Check is GPRS is free
    if (chBSemWaitTimeout (&gprsSem, TIME_IMMEDIATE) == MSG_OK) {

      // Start modem
      if ((counter == 15) && (gsmStatus != gprs_OK)
          && (gsmStatus != gprs_Failed)) {
        // GPIOD_GSM_STATUS check
        if (palReadPad (GPIOD, GPIOD_GSM_STATUS)) {
          DBG_MODEM("Starting modem: ");
          palSetPad (GPIOD, GPIOD_GSM_PWRKEY);
          chThdSleepMilliseconds (GPRS_PWR_ON_PULSE);
          palClearPad (GPIOD, GPIOD_GSM_PWRKEY);

          // Wait for status high
          tmp = 0;
          do {
            DBG_MODEM(".");
            chThdSleepMilliseconds (GPRS_PWR_DELAY);
            tmp++;
          } while ((palReadPad (GPIOD, GPIOD_GSM_STATUS))
              && (tmp < (uint8_t) (GPRS_PWR_ON_TIME / GPRS_PWR_DELAY)));
          if (!palReadPad (GPIOD, GPIOD_GSM_STATUS)) {
            gsmStatus = gprs_OK;
            DBG_MODEM(" started.\r\n");
            pushToLogText ("MO");
          } else {
            gsmStatus = gprs_Failed;
            DBG_MODEM(" failed!\r\n");
            pushToLogText ("ME");
          }
        } else {
          DBG_MODEM("Modem already on.\r\n");
          gsmStatus = gprs_OK;
        }
      }

      // Dummy query to initialize modem UART at start, since first AT reply is null
      if ((counter == 25) && (gprsLastStatus == DUMMY_NO_VALUE)
          && (gsmStatus == gprs_OK)) {
        resp = gprsSendCmd (AT_is_alive);
        chThdSleepMilliseconds (AT_DELAY);
        gprsFlushRX ();
      }

      // AT checks
      if ((counter == 30) && (gsmStatus == gprs_OK)) {
        DBG_MODEM("Modem alive check: ");
        gprsIsAlive = gprsSendCmd (AT_is_alive);
        if (gprsIsAlive == 1) {
          DBG_MODEM("OK.\r\n");
          if (gprsSetSMS != 1) {
            gprsSetSMS = gprsSendCmd (AT_set_sms_to_text); // Set modem to text SMS format
            //DBG_MODEM("AT_set_sms_to_text: %d\r\n", gprsSetSMS);
            if (gprsSetSMS == 1) gprsSetSMS = gprsSendCmd (AT_set_sms_store); // Set modem SMS receive mode
            resp = gprsSendCmdWR (AT_model_info, (uint8_t*) gprsModemInfo,
                                  sizeof(gprsModemInfo)); // Get model
            resp = gprsSendCmdWR (AT_get_sms_storage, (uint8_t*) gprsSmsText,
                                  sizeof(gprsSmsText)); // Get SMS storage info
            DBG_MODEM("SMS Storage: %s, %d\r\n", gprsSmsText, resp);
            resp = gprsSendCmd (AT_set_ATD);
          }
          resp = gprsSendCmdWRI (AT_registered, tempText, sizeof(tempText), 3);
          if (resp > 0) gprsReg = strtol ((char*) tempText, NULL, 10);
          DBG_MODEM("gprsReg: %d, %d\r\n", gprsReg, resp);
          resp = gprsSendCmdWRI (AT_signal_strength, tempText, sizeof(tempText),
                                 2);
          if (resp > 0) {
            resp = (strtol ((char*) tempText, NULL, 10));
            if (resp > 31) gprsStrength = 0;
            else gprsStrength = 7 + (resp * 3);
          }
          //DBG_MODEM("gprsStrength: %d, %d\r\n", gprsStrength, resp);
          resp = gprsSendCmdWR (AT_system_info, (uint8_t*) gprsSystemInfo,
                                sizeof(gprsSystemInfo));
          //DBG_MODEM("System: %s, %d\r\n", gprsSystemInfo, resp);
        } else {
          gprsReg = 4;
          gprsStrength = 0;
          gprsSetSMS = 0;
          gsmStatus = gprs_ForceReset;
          DBG_MODEM("Not responding.\r\n");
        }

        // if modem registration changes log it
        if (gprsLastStatus != gprsReg) {
          gprsLastStatus = gprsReg;
          tmpLog[0] = 'M';
          tmpLog[1] = gprsReg;
          tmpLog[2] = gprsStrength;
          pushToLog (tmpLog, 3);
        }
      }

      // Stop modem if requested
      if (gsmStatus == gprs_ForceReset) {
        DBG_MODEM("Stopping modem: ");
        palSetPad (GPIOD, GPIOD_GSM_PWRKEY);
        chThdSleepMilliseconds (GPRS_PWR_OFF_PULSE);
        palClearPad (GPIOD, GPIOD_GSM_PWRKEY);

        // Wait for status low
        tmp = 0;
        do {
          DBG_MODEM(".");
          chThdSleepMilliseconds (GPRS_PWR_DELAY);
          tmp++;
        } while ((!palReadPad (GPIOD, GPIOD_GSM_STATUS))
            && (tmp < (uint8_t) (GPRS_PWR_OFF_TIME / GPRS_PWR_DELAY)));
        gprsLastStatus = DUMMY_NO_VALUE;
        if (tmp != 0) {
          DBG_MODEM(" stopped.\r\n");
          gsmStatus = gprs_NotInitialized;
          pushToLogText ("MF");
        } else {
          DBG_MODEM(" failed!\r\n");
          gsmStatus = gprs_Failed;
          pushToLogText ("ME");
        }
      }

      // Read incoming SMS or missed messages
      while (gprsReadMsg ((uint8_t*) gprsSmsText, sizeof(gprsSmsText))) {
        DBG_MODEM("Modem: %s.\r\n", gprsSmsText);
        // Check for incoming SMS
        if (memcmp (AT_SMS_received, gprsSmsText, strlen (AT_SMS_received))
            == 0) {
          // Get index
          pch = strtok ((char*) gprsSmsText, " ,.-");
          resp = 1;
          while (pch != NULL) {
            DBG_MODEM(">i:%u>%s<\r\n", resp, pch);
            if (resp == 3) break;
            pch = strtok (NULL, " ,.-");
            resp++;
          }
          smsIndex = strtol (pch, NULL, 10);
          DBG_MODEM("New SMS index: %d\r\n", smsIndex);
          // Read SMS
          resp = gprsGetSMS (smsIndex, (uint8_t*) tempText, sizeof(tempText),
                             (uint8_t*) gprsSmsText, sizeof(gprsSmsText));
          DBG_MODEM("SMS number: %s \r\n", tempText);
          DBG_MODEM("SMS text: %s| resp: %d\r\n", gprsSmsText, resp);
          // Process SMS

          // Check SMS number is authorized
          resp = isPhoneNumberAuthorized ((char*) tempText);
//          if (resp < 0) {
          DBG_MODEM("SMS number authorized. Contact: %d\r\n", resp);
          // Process SMS text
          chsnprintf (gprsSmsText, 128, "GET SYSTEM STATUS"); // For test purposes"
          resp = cmd_process ((char*) gprsSmsText, &dispatcher, logText,
                              LOG_TEXT_LENGTH);
          DBG_MODEM("SMS command processed, status: %d, %s\r\n", resp, logText);
//          } else {
//            DBG_MODEM("SMS number NOT authorized!\r\n");
//          }

          // Delete SMS
          resp = gprsDeleteSMS (smsIndex); // Delete SMS after read
          DBG_MODEM("Delete SMS index %d, resp: %d\r\n", smsIndex, resp);
        }
      }

      chBSemSignal (&gprsSem);
    } // Semaphore is free

    chThdSleepMilliseconds (1000);
    counter++;
  }
}

#endif /* OHS_TH_MODEM_H_ */
