/*
 * ohs_functions.h
 *
 *  Created on: 16. 12. 2019
 *      Author: adam
 */

#ifndef OHS_FUNCTIONS_H_
#define OHS_FUNCTIONS_H_

// Logger
void pushToLog(char *what, uint8_t size) {
  logger_t *outMsg = chPoolAlloc(&logger_pool);
  if (outMsg != NULL) {
    memset(outMsg->text, 0, LOGGER_MSG_LENGTH);
    if (size > LOGGER_MSG_LENGTH) size = LOGGER_MSG_LENGTH;

    outMsg->timestamp = getTimeUnixSec();  // Set timestamp
    memcpy(&outMsg->text[0], what, size);  // Copy string
    //chprintf(console, "L msg %s %d\r\n", outMsg->text, size);

    msg_t msg = chMBPostTimeout(&logger_mb, (msg_t)outMsg, TIME_IMMEDIATE);
    if (msg != MSG_OK) {
      //chprintf(console, "MB full %d\r\n", temp);
    }
  } else {
    chprintf(console, "L P full %d \r\n", outMsg);
  }
}
void pushToLogText(char *what) {
  uint8_t len = strlen(what);
  if (len > LOGGER_MSG_LENGTH) len = LOGGER_MSG_LENGTH;
  pushToLog(what, len);
}

// Send data to node
int8_t sendData(uint8_t address, const uint8_t *data, uint8_t length){
  RS485Msg_t rs485Data;

  chprintf(console, "RS485 Send data to address: %d\r\n", address);
  rs485Data.address = address;
  rs485Data.length = length;
  memcpy(&rs485Data.data[0], data, length);
  for(uint8_t ii = 0; ii < length; ii++) {
    chprintf(console, "%d-%x, ", ii, rs485Data.data[ii]);
  } chprintf(console, "\r\n");
  return rs485SendMsgWithACK(&RS485D2, &rs485Data, 5);
}

// Send a command to node
int8_t sendCmd(uint8_t address, uint8_t command) {

  // RS485
  if (address <= RADIO_UNIT_OFFSET) {
    RS485Cmd_t rs485Cmd;

    chprintf(console, "RS485 Send cmd: %d to address: %d\r\n", command, address);
    rs485Cmd.address = address;
    rs485Cmd.length = command;
    return rs485SendCmdWithACK(&RS485D2, &rs485Cmd, 3);
  }
  // Radio
  if (address >= RADIO_UNIT_OFFSET) {
    char radioCmd[] = {'C', command};

    chprintf(console, "Radio Send cmd: %d to address: %d\r\n", command, address - RADIO_UNIT_OFFSET);
    rfm69Send(address - RADIO_UNIT_OFFSET, radioCmd, sizeof(radioCmd), true);
  }

}

// Send a command to all members of a group
void sendCmdToGrp(uint8_t groupNum, uint8_t command, char type) {

  // Go through all nodes
  for (int8_t i=0; i < NODE_SIZE; i++){
    if (GET_NODE_ENABLED(node[i].setting)) {
      // Auth. node belong to group                         type of node
      if ((GET_NODE_GROUP(node[i].setting) == groupNum) && (type == node[i].type)) {
        sendCmd(node[i].address, command);
      }
    }
  }
}

// Find existing node index
int8_t getNodeIndex(uint8_t address, char type, char function, uint8_t number){
  for (uint8_t i=0; i < NODE_SIZE; i++) {
    //chprintf(console, "getNodeIndex: %d,T %d-%d,A %d-%d,F %d-%d,N %d-%d\r\n", i, type, node[i].type, address, node[i].address, function, node[i].function, number, node[i].number);
    if (node[i].type     == type &&
        node[i].address  == address &&
        node[i].function == function &&
        node[i].number   == number) { return i; }
  }
  return -1;
}

// Get first free node index
int8_t getNodeFreeIndex(void){
  for (uint8_t i=0; i < NODE_SIZE; i++) {
    //chprintf(console, "getNodeFreeIndex: %d, %d\r\n", i, node[i].address);
    if (node[i].address == 0) { return i; }
  }
  return -1;
}

typedef enum {
  armAway = 0,
  armHome = 1
} armType_t;
// Arm a group
void armGroup(uint8_t groupNum, uint8_t master, armType_t armType, uint8_t hop) {
  uint8_t resp = 0;

  // if group enabled arm group or log error to log.
  if (GET_CONF_GROUP_ENABLED(conf.group[groupNum])){
    // Group not armed already
    if (!GET_GROUP_ARMED(group[groupNum].setting)){
      if (armType == armAway) {
        group[groupNum].armDelay = conf.armDelay * 4; // set arm delay * 0.250 seconds
      } else {
        SET_GROUP_ARMED_HOME(group[groupNum].setting);
        group[groupNum].armDelay = 8; // Just 2 seconds to indicate arm home
      }
      sendCmdToGrp(groupNum, NODE_CMD_ARMING, 'K'); // Send arm cmd to all Key nodes
      //+++publishGroup(groupNum, 'P');
    }
  }
  else { tmpLog[0] = 'G'; tmpLog[1] = 'F'; tmpLog[2] = groupNum;  pushToLog(tmpLog, 3); }
  // If Arm another group is set and another group is not original(master)
  // and hop is lower then ALR_GROUPS
  resp = GET_CONF_GROUP_ARM_CHAIN(conf.group[groupNum]); // Temp variable
  if ((resp != 15) &&
      (resp != master) &&
      (master != ARM_GROUP_CHAIN_NONE) &&
      (hop <= ALARM_GROUPS)) {
    hop++; // Increase hop
    armGroup(resp, master, armType, hop);
  }
}

void disarmGroup(uint8_t groupNum, uint8_t master, uint8_t hop) {
  uint8_t resp = 0;

  // we have alarm
  if (GET_GROUP_ALARM(group[groupNum].setting)) {
    CLEAR_GROUP_ALARM(group[groupNum].setting); // Set this group alarm off
    /* *** ToDo: add bitwise reset of OUTs instead of full reset ? */
    //+++OUTs = 0; // Reset outs
    //+++pinOUT1.write(LOW); pinOUT2.write(LOW); // Turn off OUT 1 & 2
  }
  // Set each member zone of this group
  for (uint8_t j=0; j < ALARM_ZONES; j++){
    if (GET_CONF_ZONE_GROUP(conf.zone[j]) == groupNum) {
      CLEAR_ZONE_ALARM(zone[j].setting); // Zone alarm off
    }
  }
  CLEAR_GROUP_ARMED(group[groupNum].setting);     // disarm group
  CLEAR_GROUP_ARMED_HOME(group[groupNum].setting);// disarm group
  CLEAR_GROUP_WAIT_AUTH(group[groupNum].setting); // Set auth bit off
  group[groupNum].armDelay = 0;    // Reset arm delay
  sendCmdToGrp(groupNum, NODE_CMD_DISARM, 'K'); // Send disarm cmd to all Key nodes
  //+++if (publish == 1) publishGroup(_group, 'D');
  tmpLog[0] = 'G'; tmpLog[1] = 'D'; tmpLog[2] = groupNum; pushToLog(tmpLog, 3); // Group disarmed
  // If Disarm another group is set and another group is not original(master)
  // and hop is lower then ALR_GROUPS
  resp = GET_CONF_GROUP_DISARM_CHAIN(conf.group[groupNum]); // Temp variable
  if ((resp != 15) &&
      (resp != master) &&
      (master != 255) &&
      (hop <= ALARM_GROUPS)) {
    hop++; // Increase hop
    disarmGroup(resp, master, hop);
  }
}

// Check key value to saved keys
void checkKey(uint8_t groupNum, armType_t armType, uint8_t *key){
  // Group is allowed and enabled
  chprintf(console, "Check key for group: %u, arm tupe: %u\r\n", groupNum, armType);
  if ((groupNum < ALARM_GROUPS) && (GET_CONF_GROUP_ENABLED(conf.group[groupNum]))) {
    // Check all keys
    for (uint8_t i=0; i < KEYS_SIZE; i++){
      chprintf(console, "Key match: %d", i);
      //for(uint8_t ii = 0; ii < KEY_LENGTH; ii++) { chprintf(console, "%d-%x, ", ii, key[ii]); } chprintf(console, "\r\n");
      //for(uint8_t ii = 0; ii < KEY_LENGTH; ii++) { chprintf(console, "%d-%x, ", ii, conf.keyValue[i][ii]); } chprintf(console, "\r\n");
      if (memcmp(key, &conf.keyValue[i], KEY_LENGTH) == 0) { // key matched
        chprintf(console, ", group: %d\r\n", groupNum);
        //  key enabled && (group = contact_group || contact_key = global)
        if (GET_CONF_KEY_ENABLED(conf.keySetting[i]) &&
           (groupNum == GET_CONF_CONTACT_GROUP(conf.contact[conf.keyContact[i]]) ||
            GET_CONF_CONTACT_IS_GLOBAL(conf.contact[conf.keyContact[i]]))) {
          // We have alarm or group is armed or arming
          if  (GET_GROUP_ALARM(group[groupNum].setting) ||
               GET_GROUP_ARMED(group[groupNum].setting) ||
               group[groupNum].armDelay > 0) {
            tmpLog[0] = 'A'; tmpLog[1] = 'D'; tmpLog[2] = i; pushToLog(tmpLog, 3); // Key
            disarmGroup(groupNum, groupNum, 0); // Disarm group and all chained groups
          } else { // Just do arm
            tmpLog[0] = 'A';
            if (armType == armAway) tmpLog[1] = 'A';
            else                    tmpLog[1] = 'H';
            tmpLog[2] = i; pushToLog(tmpLog, 3);
            armGroup(groupNum, groupNum, armType, 0); // Arm group and all chained groups
          }
          break; // no need to try other
        } else { // key is not enabled
          tmpLog[0] = 'A'; tmpLog[1] = 'F'; tmpLog[2] = i;  pushToLog(tmpLog, 3);
        }
      } // key matched
      else if (i == KEYS_SIZE-1) {
        // Log unknown keys
        tmpLog[0] = 'A'; tmpLog[1] = 'U'; memcpy(&tmpLog[2], key, KEY_LENGTH); pushToLog(tmpLog, 10);
        memcpy(&lastKey[0], key, KEY_LENGTH); // store last unknown key
      }
    } // for
  } else {
    tmpLog[0] = 'G'; tmpLog[1] = 'F'; tmpLog[2] = groupNum;  pushToLog(tmpLog, 3);
  }
}

#endif /* OHS_FUNCTIONS_H_ */
