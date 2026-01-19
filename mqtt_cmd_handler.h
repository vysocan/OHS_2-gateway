/*
 * mqtt_cmd_handler.h
 *
 *  Created on: Dec 29, 2025
 *      Author: vysocan
 */

#ifndef MQTT_CMD_HANDLER_H_
#define MQTT_CMD_HANDLER_H_



/* === FORWARD DECLARATIONS ================================================ */
static cmdStatus_t handle_group_state(const char *tokens[], uint8_t count,
    char *result, uint16_t result_len);
static cmdStatus_t handle_sensor_value(const char *tokens[], uint8_t count,
    char *result, uint16_t result_len);
static cmdStatus_t handle_zone_refresh(const char *tokens[], uint8_t count,
    char *result, uint16_t result_len);

/* === TABLES - MQTT COMMAND STRUCTURE ====================================== */
/* /group/{#}* */
static const cmdEntry_t group_sub[] = {
  { TEXT_state, handle_group_state, "Group state control", NULL, 0 }
};

/* /sensor/{address}* */
static const cmdEntry_t sensor_sub[] = {
  { TEXT_value, handle_sensor_value, "Sensor data", NULL, 0 }
};

/* /zone/refresh */
static const cmdEntry_t zone_sub[] = {
  { TEXT_refresh, handle_zone_refresh, "Refresh zone states", NULL, 0 }
};

/* Top-level MQTT commands */
const cmdEntry_t mqtt_top_commands[] = {
  { TEXT_Group, NULL, "Group control", group_sub, ARRAY_COUNT (group_sub) },
  { TEXT_Sensor, NULL, "Sensor data", sensor_sub, ARRAY_COUNT (sensor_sub) },
  { TEXT_Zone, NULL, "Zone management", zone_sub, ARRAY_COUNT (zone_sub) },
  { TEXT_Help, cmdHandleHelp, "Show this help", NULL, 0 }
};

/* === HANDLERS ============================================================= */
static cmdStatus_t handle_group_state(const char *tokens[], uint8_t count,
    char *result, uint16_t result_len) {

  if (count < 2) {
    chsnprintf (result, result_len, "Usage: GROUP <id> STATE <command>");
    return CMD_INVALID_ARGS;
  }

  uint8_t index = strtoul(tokens[0], NULL, 0) - 1;
  if ((index >= ALARM_GROUPS) ||
      !GET_CONF_GROUP_ENABLED(conf.group[index].setting) ||
      !GET_CONF_GROUP_MQTT(conf.group[index].setting)) {
    chsnprintf (result, result_len, "Invalid group");
    return CMD_INVALID_ARGS;
  }

  // State command
  if (strcmp (tokens[1], TEXT_state) != 0) {
    chsnprintf (result, result_len, "Usage: GROUP <id> STATE <command>");
    return CMD_INVALID_ARGS;
  }

  // Parse state command from payload
  if (strcmp (mqttInPayload, TEXT_arm_home) == 0) {
    armGroup (index, index, armHome, 0);
    chsnprintf (result, result_len, "Group %d armed home", index + 1);
  } else if (strcmp (mqttInPayload, TEXT_arm_away) == 0) {
    armGroup (index, index, armAway, 0);
    chsnprintf (result, result_len, "Group %d armed away", index + 1);
  } else if (strcmp (mqttInPayload, TEXT_disarm) == 0) {
    disarmGroup (index, index, 0);
    chsnprintf (result, result_len, "Group %d disarmed", index + 1);
  } else {
    chsnprintf (result, result_len, "Unknown command: %s", mqttInPayload);
    return CMD_INVALID_ARGS;
  }
  return CMD_OK;
}

static cmdStatus_t handle_sensor_value(const char *tokens[], uint8_t count,
    char *result, uint16_t result_len) {

  if (count < 1) {
    chsnprintf (result, result_len, "Usage: SENSOR <address> VALUE <value>");
    return CMD_INVALID_ARGS;
  }

  // Parse node address and update sensor value
  // tokens[0] contains node address like "W:2:I:D:0"
  chsnprintf (result, result_len, "Sensor updated: %s = %s", tokens[0],
      mqttInPayload);
  return CMD_OK;
}

static cmdStatus_t handle_zone_refresh(const char *tokens[], uint8_t count,
    char *result, uint16_t result_len) {
  (void)tokens;
  (void)count;

  mqttRefreshZonesState();
  chsnprintf (result, result_len, "Zone states refreshed");
  return CMD_OK;
}

#endif /* MQTT_CMD_HANDLER_H_ */
