/*
 * ohs_cmd_handler.h
 *
 *  Created on: Dec 26, 2025
 *      Author: vysocan
 */

#ifndef OHS_CMD_HANDLER_H_
#define OHS_CMD_HANDLER_H_

/*
 * Modem command handlers
 */
/* === FORWARD DECLARATIONS ================================================ */
static cmd_status_t handle_get_system_status(const char *tokens[],
    uint8_t count, char *result, uint16_t result_len);
static cmd_status_t handle_get_zone_status(const char *tokens[], uint8_t count,
    char *result, uint16_t result_len);
static cmd_status_t handle_set_generic(const char *tokens[], uint8_t count,
    char *result, uint16_t result_len);

/* === TABLES - PARENT REFERENCES ONLY ====================================== */
/* Leaf tables - sub_count = 0 (no subcommands) */
static const cmd_entry_t get_system_sub[] = { { text_Status,
    handle_get_system_status, "Get system status", NULL, 0 } };
static const cmd_entry_t get_zone_sub[] = { { text_Status,
    handle_get_zone_status, "Get zone status", NULL, 0 } };

/* Parent tables - use ARRAY_COUNT(child) */
static const cmd_entry_t get_sub[] = { { text_System, NULL,
    "System information", get_system_sub, ARRAY_COUNT (get_system_sub) }, {
    text_Zone, NULL, "Zone information", get_zone_sub, ARRAY_COUNT (
        get_zone_sub) } };

/* Top-level */
const cmd_entry_t top_commands[] = { { text_Get, NULL, "Read status", get_sub,
    ARRAY_COUNT (get_sub) }, { text_Set, handle_set_generic,
    "Modify configuration", NULL, 0 }, { text_Help, cmdHandleHelp,
    "Show this help", NULL, 0 } };

/* === HANDLERS ============================================================= */
/*
 * Handle GET SYSTEM STATUS
 */
static cmd_status_t handle_get_system_status(const char *tokens[],
    uint8_t count, char *result, uint16_t result_len) {
  (void) tokens;
  (void) count;

  chsnprintf (result, result_len, "SYSTEM RUNNING | Uptime: 1234s");
  return CMD_OK;
}
/*
 * Handle GET ZONE STATUS
 */
static cmd_status_t handle_get_zone_status(const char *tokens[], uint8_t count,
    char *result, uint16_t result_len) {
  if (count < 1) {
    chsnprintf (result, result_len, "Usage: GET ZONE STATUS <zone_id>");
    return CMD_INVALID_ARGS;
  }

  const char *zone_id = tokens[0];
  chsnprintf (result, result_len,
      "ZONE %s STATUS | State: ACTIVE | Sensor: 23.5C", zone_id);
  return CMD_OK;
}
/*
 * Handle SET GENERIC
 */
static cmd_status_t handle_set_generic(const char *tokens[], uint8_t count,
    char *result, uint16_t result_len) {

  chsnprintf (result, result_len, "SET command executed: ");
  for (uint8_t i = 0; i < count; i++) {
    strncat (result, tokens[i],
        LWIP_MIN (strlen (tokens[i]), (sizeof(result) - strlen (result))));
  }
  return CMD_OK;
}

#endif /* OHS_CMD_HANDLER_H_ */
