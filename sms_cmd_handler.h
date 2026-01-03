
/*
 * sms_cmd_handler.h
 *
 *  Created on: Dec 26, 2025
 *      Author: vysocan
 */

#ifndef SMS_CMD_HANDLER_H_
#define SMS_CMD_HANDLER_H_

/*
 * Modem command handlers
 */
/* === FORWARD DECLARATIONS ================================================ */
static cmdStatus_t handleGetGroupStatus(const char *tokens[], uint8_t count,
    BaseSequentialStream *chp);
static cmdStatus_t handleGetSystemStatus(const char *tokens[], uint8_t count,
    BaseSequentialStream *chp);
static cmdStatus_t handleGetZoneStatus(const char *tokens[], uint8_t count,
    BaseSequentialStream *chp);
static cmdStatus_t handleSetGeneric(const char *tokens[], uint8_t count,
    BaseSequentialStream *chp);

/* === TABLES - PARENT REFERENCES ONLY ====================================== */
/* Leaf tables - sub_count = 0 (no subcommands) */
static const cmdEntry_t smsGetGroupSub[] = {
  { text_Status, handleGetGroupStatus, "Get group status", NULL, 0 }
};
static const cmdEntry_t smsGetSystemSub[] = {
  { text_Status, handleGetSystemStatus, "Get system status", NULL, 0 }
};
static const cmdEntry_t smsGetZoneSub[] = {
  { text_Status, handleGetZoneStatus, "Get zone status", NULL, 0 }
};

/* Parent tables - use ARRAY_COUNT(child) */
static const cmdEntry_t smsGetSub[] = {
  { text_Group, NULL, "Group information", smsGetGroupSub, ARRAY_COUNT(smsGetGroupSub) },
  { text_System, NULL, "System information", smsGetSystemSub, ARRAY_COUNT(smsGetSystemSub) },
  { text_Zone, NULL, "Zone information", smsGetZoneSub, ARRAY_COUNT(smsGetZoneSub) }
};

/* Top-level */
const cmdEntry_t smsTopCommands[] = {
  { text_Get, NULL, "Read status", smsGetSub, ARRAY_COUNT(smsGetSub) },
  { text_Set, handleSetGeneric, "Modify configuration", NULL, 0 },
  { text_Help, cmdHandleHelp, "Show this help", NULL, 0 } // Help command has internal handler, no need to implement
};

/* === HANDLERS =============================================================
 * Implement your command handlers below
 */
/*
 * Handle GET GROUP STATUS
 */
static cmdStatus_t handleGetGroupStatus(const char *tokens[], uint8_t count,
    BaseSequentialStream *chp) {

  // Check arguments
  if (count < 1) {
    chprintf(chp, "Usage: GET GROUP STATUS <group_id>\n");
    return CMD_INVALID_ARGS;
  }

  // Get group index
  uint32_t groupIndex = DUMMY_NO_VALUE;
  if (safeStrtoul(tokens[0], &groupIndex, 10)) {
    groupIndex--; // Convert to 0-based index

    // Check valid group
    if (groupIndex < ALARM_GROUPS) {
      if (GET_CONF_GROUP_ENABLED(conf.group[groupIndex].setting) == 0) {
        chprintf (chp, "Group %d.%s is DISABLED", groupIndex + 1,
            conf.group[groupIndex].name);
        return CMD_OK;
      } else {
        if (GET_GROUP_ARMED(group[groupIndex].setting)) {
          chprintf (chp, "Group %d.%s is %s", groupIndex + 1,
              conf.group[groupIndex].name,
              GET_GROUP_ARMED_HOME(group[groupIndex].setting) ? "ARMED HOME" : "ARMED AWAY");
          return CMD_OK;
        } else {
          chprintf (chp, "Group %d.%s is DISARMED", groupIndex + 1,
              conf.group[groupIndex].name);
          return CMD_OK;
        }
      }
    }
  }

  // Fallthrough invalid group
  chprintf (chp, "Invalid group ID: %s", tokens[0]);
  return CMD_INVALID_ARGS;
}
/*
 * Handle GET SYSTEM STATUS
 */
static cmdStatus_t handleGetSystemStatus(const char *tokens[], uint8_t count,
    BaseSequentialStream *chp) {
  (void) tokens;
  (void) count;

  chprintf(chp, "SYSTEM RUNNING | Uptime: 1234s");
  return CMD_OK;
}
/*
 * Handle GET ZONE STATUS
 */
static cmdStatus_t handleGetZoneStatus(const char *tokens[], uint8_t count,
    BaseSequentialStream *chp) {

  if (count < 1) {
    chprintf(chp, "Usage: GET ZONE STATUS <zone_id>");
    return CMD_INVALID_ARGS;
  }

  // Get zone index
  uint8_t zoneIndex = DUMMY_NO_VALUE;
  if (safeStrtoul (tokens[0], (unsigned long*) &zoneIndex, 10)) {
    zoneIndex--; // Convert to 0-based index

    // Check valid zone
    if (zoneIndex < ALARM_ZONES) {
      if (GET_CONF_ZONE_ENABLED (conf.zone[zoneIndex]) == 0) {
        chprintf (chp, "Zone %d.%s is DISABLED", zoneIndex + 1,
            conf.zoneName[zoneIndex]);
        return CMD_OK;
      } else {
        chprintf (chp, "Zone %d.%s is ACTIVE", zoneIndex + 1,
            conf.zoneName[zoneIndex]);
        return CMD_OK;
      }
    }
  }

  // Fallthrough invalid zone
  chprintf (chp, "Invalid zone ID: %s", tokens[0]);
  return CMD_INVALID_ARGS;
}
/*
 * Handle SET GENERIC
 */
static cmdStatus_t handleSetGeneric(const char *tokens[], uint8_t count,
    BaseSequentialStream *chp) {

  chprintf(chp, "SET command executed: ");
  for (uint8_t i = 0; i < count; i++) {
    chprintf(chp, "%s", tokens[i]);
  }
  return CMD_OK;
}

#endif /* SMS_CMD_HANDLER_H_ */
