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

    outMsg->timestamp = GetTimeUnixSec();  // Set timestamp
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
  RS485Cmd_t rs485Cmd;

  chprintf(console, "RS485 Send cmd: %d to address: %d\r\n", command, address);
  rs485Cmd.address = address;
  rs485Cmd.length = command;
  return rs485SendCmdWithACK(&RS485D2, &rs485Cmd, 3);
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

// Check key value to saved keys
void checkKey(uint8_t nodeIndex, uint8_t *key){
  uint8_t _group;
  // Check all keys
  for (uint8_t i=0; i < KEYS_SIZE; i++){
    chprintf(console, "Match :%d\r\n", i);
    //for(uint8_t ii = 0; ii < KEY_LENGTH; ii++) { chprintf(console, "%d-%x, ", ii, key[ii]); } chprintf(console, "\r\n");
    //for(uint8_t ii = 0; ii < KEY_LENGTH; ii++) { chprintf(console, "%d-%x, ", ii, conf.keyValue[i][ii]); } chprintf(console, "\r\n");
    if (memcmp(key, &conf.keyValue[i], KEY_LENGTH) == 0) { // key matched
      _group = GET_NODE_GROUP(node[nodeIndex].setting);
      chprintf(console, "Key matched, group: %d\r\n", _group);
      //  key enabled && (group = key_group || key = global)
      if (GET_CONF_KEY_ENABLED(conf.keySetting[i]) &&
         (_group == GET_CONF_KEY_GROUP(conf.keySetting[i]) || GET_CONF_KEY_IS_GLOBAL(conf.keySetting[i]))) {
        // We have alarm or group is armed
        if  (GET_GROUP_ALARM(group[_group].setting) || GET_GROUP_ARMED(group[_group].setting)) {
          tmpLog[0] = 'A'; tmpLog[1] = 'D'; tmpLog[2] = i; pushToLog(tmpLog, 3); // Key
          //***disarmGroup(_group, _group); // Disarm group and all chained groups
        } else { // Just do arm
          tmpLog[0] = 'A'; tmpLog[1] = 'A'; tmpLog[2] = i; pushToLog(tmpLog, 3);
          //***armGroup(_group, _group); // Arm group and all chained groups
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
}



#endif /* OHS_FUNCTIONS_H_ */
