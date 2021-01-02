/*
 * ohs_functions.h
 *
 *  Created on: 16. 12. 2019
 *      Author: adam
 */

#ifndef OHS_FUNCTIONS_H_
#define OHS_FUNCTIONS_H_

// Logger related
#define FRAM_MSG_SIZE     16
#define FRAM_HEADER_SIZE  4
volatile uint16_t FRAMWritePos = 0;
volatile uint16_t FRAMReadPos  = 0;
// FRAM buffers for decode
static char rxBuffer[FRAM_MSG_SIZE];
static char txBuffer[FRAM_HEADER_SIZE + FRAM_MSG_SIZE];

/*
 * Get current timestamp with DST
 */
time_t getTimeUnixSec(void) {
  time_t timeSec;

  rtcGetTime(&RTCD1, &timespec);
  timeSec = convertRTCDateTimeToUnixSecond(&timespec);
  if ((timeSec >= calculateDST(timespec.year, conf.timeDstMonth, conf.timeDstWeekNum, conf.timeDstDow, conf.timeDstHour)) &&
      (timeSec <= calculateDST(timespec.year, conf.timeStdMonth, conf.timeStdWeekNum, conf.timeStdDow, conf.timeStdHour))) {
    timeSec += conf.timeDstOffset * SECONDS_PER_MINUTE;
  } else {
    timeSec += conf.timeStdOffset * SECONDS_PER_MINUTE;
  }

  return timeSec;
}
/*
 * Logger
 */
void pushToLog(char *what, uint8_t size) {
  loggerEvent_t *outMsg = chPoolAlloc(&logger_pool);

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
    chprintf(console, "Log pool full!\r\n");
  }
}
/*
 * Logger for text only messages
 */
void pushToLogText(char *what) {
  uint8_t len = strlen(what);
  osalDbgCheck(len <= LOGGER_MSG_LENGTH);
  pushToLog(what, len);
}
/*
 * MQTT publish
 */
void pushToMqtt(mqttPubType_t type, uint8_t number, mqttPubFunction_t function) {
  mqttEvent_t *outMsg = chPoolAlloc(&mqtt_pool);

  if (outMsg != NULL) {
    outMsg->type = type;
    outMsg->number = number;
    outMsg->function = function;

    msg_t msg = chMBPostTimeout(&mqtt_mb, (msg_t)outMsg, TIME_IMMEDIATE);
    if (msg != MSG_OK) {
      //chprintf(console, "MB full %d\r\n", temp);
    }
  } else {
    chprintf(console, "MQTT pool full!\r\n");
    pushToLogText("FM"); // MQTT queue is full
  }
}
/*
 * Send data to node
 */
int8_t sendData(uint8_t address, const uint8_t *data, uint8_t length){
  int8_t resp;

  // RS485
  if (address <= RADIO_UNIT_OFFSET) {
    RS485Msg_t rs485Data;

    chprintf(console, "RS485 Send data to address: %d\r\n", address);
    rs485Data.address = address;
    rs485Data.length = length;
    memcpy(&rs485Data.data[0], data, length);
    for(uint8_t ii = 0; ii < length; ii++) {
      chprintf(console, "%d-%x, ", ii, rs485Data.data[ii]);
    } chprintf(console, "\r\n");
    if (rs485SendMsgWithACK(&RS485D2, &rs485Data, 5) == MSG_OK) resp = 1;
    else resp = -1;
  }
  // Radio
  if (address >= RADIO_UNIT_OFFSET) {
    chprintf(console, "Radio Send data to address: %d\r\n", address - RADIO_UNIT_OFFSET);
    resp = rfm69SendWithRetry(address - RADIO_UNIT_OFFSET, data, length, 5);
  }
  return resp;
}
/*
 * Send a command to node
 */
int8_t sendCmd(uint8_t address, uint8_t command) {
  int8_t resp;

  // RS485
  if (address <= RADIO_UNIT_OFFSET) {
    RS485Cmd_t rs485Cmd;

    chprintf(console, "RS485 send cmd: %d to address: %d\r\n", command, address);
    rs485Cmd.address = address;
    rs485Cmd.length = command;
    if (rs485SendCmdWithACK(&RS485D2, &rs485Cmd, 3) == MSG_OK) resp = 1;
    else resp = -1;
  }
  // Radio
  if (address >= RADIO_UNIT_OFFSET) {
    char radioCmd[] = {'C', command};

    if (address == RADIO_UNIT_OFFSET) {
      chprintf(console, "Radio send cmd: %d to broadcast.\r\n", command);
      resp = rfm69Send(255, radioCmd, sizeof(radioCmd), false);
    } else {
      chprintf(console, "Radio send cmd: %d to address: %d\r\n", command, address - RADIO_UNIT_OFFSET);
      resp = rfm69Send(address - RADIO_UNIT_OFFSET, radioCmd, sizeof(radioCmd), true);
    }
  }
  return resp;
}
/*
 * Send a command to all members of a group
 */
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
/*
 * Find existing node index
 */
uint8_t getNodeIndex(uint8_t address, char type, char function, uint8_t number){
  for (uint8_t i=0; i < NODE_SIZE; i++) {
    //chprintf(console, "getNodeIndex: %d,T %d-%d,A %d-%d,F %d-%d,N %d-%d\r\n", i, type, node[i].type, address, node[i].address, function, node[i].function, number, node[i].number);
    if (node[i].type     == type &&
        node[i].address  == address &&
        node[i].function == function &&
        node[i].number   == number) { return i; }
  }
  return DUMMY_NO_VALUE;
}
/*
 * Get first free node index
 */
uint8_t getNodeFreeIndex(void){
  for (uint8_t i=0; i < NODE_SIZE; i++) {
    //chprintf(console, "getNodeFreeIndex: %d, %d\r\n", i, node[i].address);
    if (node[i].address == 0) { return i; }
  }
  return DUMMY_NO_VALUE;
}
/*
 * Arm a group
 */
void armGroup(uint8_t groupNum, uint8_t master, armType_t armType, uint8_t hop) {
  uint8_t resp = 0;

  // if group enabled arm group or log error to log.
  if (GET_CONF_GROUP_ENABLED(conf.group[groupNum].setting)){
    // Group not armed already
    if (!GET_GROUP_ARMED(group[groupNum].setting)){
      if (armType == armAway) {
        group[groupNum].armDelay = conf.armDelay * 4; // set arm delay * 0.250 seconds
      } else {
        SET_GROUP_ARMED_HOME(group[groupNum].setting);
        group[groupNum].armDelay = 8; // Just 2 seconds to indicate arm home
      }
      sendCmdToGrp(groupNum, NODE_CMD_ARMING, 'K'); // Send arm cmd to all Key nodes
      // MQTT
      if (GET_CONF_GROUP_MQTT(conf.group[groupNum].setting)) pushToMqtt(typeGroup, groupNum, functionState);
      // Save group state, here we save armDelay and armType
      writeToBkpRTC((uint8_t*)&group, sizeof(group), 0);
      // Triggers
      triggerEvent_t *outMsgTrig = chPoolAlloc(&trigger_pool);
      if (outMsgTrig != NULL) {
        outMsgTrig->type = 'G';
        outMsgTrig->address = 0;
        outMsgTrig->function = ' ';
        outMsgTrig->number = groupNum;
        // As defined in groupState[], 0 = disarmed
        outMsgTrig->value = (float)(armType + 1);
        msg_t msg = chMBPostTimeout(&trigger_mb, (msg_t)outMsgTrig, TIME_IMMEDIATE);
        if (msg != MSG_OK) {
          //chprintf(console, "S-MB full %d\r\n", temp);
        }
      } else {
        pushToLogText("FT"); // Trigger queue is full
      }
    }
  }
  else { tmpLog[0] = 'G'; tmpLog[1] = 'F'; tmpLog[2] = groupNum;  pushToLog(tmpLog, 3); }
  // If Arm another group is set and another group is not original(master)
  // and hop is lower then ALR_GROUPS
  resp = GET_CONF_GROUP_ARM_CHAIN(conf.group[groupNum].setting); // Temp variable
  if ((resp != DUMMY_GROUP) &&
      (resp != master) &&
      (master != DUMMY_NO_VALUE) &&
      (hop <= ALARM_GROUPS)) {
    hop++; // Increase hop
    armGroup(resp, master, armType, hop);
  }
}
/*
 * Disarm a group
 */
void disarmGroup(uint8_t groupNum, uint8_t master, uint8_t hop) {
  uint8_t resp = 0;

  // we have alarm
  if (GET_GROUP_ALARM(group[groupNum].setting)) {
    CLEAR_GROUP_ALARM(group[groupNum].setting); // Set this group alarm off
    /* *** TODO OHS: add bitwise reset of OUTs instead of full reset ? */
    //+++OUTs = 0; // Reset outs
    // Turn off relays
    palClearPad(GPIOB, GPIOB_RELAY_1);
    palClearPad(GPIOB, GPIOB_RELAY_2);
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
  // MQTT
  if (GET_CONF_GROUP_MQTT(conf.group[groupNum].setting)) pushToMqtt(typeGroup, groupNum, functionState);
  tmpLog[0] = 'G'; tmpLog[1] = 'D'; tmpLog[2] = groupNum; pushToLog(tmpLog, 3); // Group disarmed
  // Save group state
  writeToBkpRTC((uint8_t*)&group, sizeof(group), 0);
  // Triggers
  triggerEvent_t *outMsgTrig = chPoolAlloc(&trigger_pool);
  if (outMsgTrig != NULL) {
    outMsgTrig->type = 'G';
    outMsgTrig->address = 0;
    outMsgTrig->function = ' ';
    outMsgTrig->number = groupNum;
    // As defined in groupState[], 0 = disarmed
    outMsgTrig->value = 0;
    msg_t msg = chMBPostTimeout(&trigger_mb, (msg_t)outMsgTrig, TIME_IMMEDIATE);
    if (msg != MSG_OK) {
      //chprintf(console, "S-MB full %d\r\n", temp);
    }
  } else {
    pushToLogText("FT"); // Trigger queue is full
  }

  // If Disarm another group is set and another group is not original(master)
  // and hop is lower then ALR_GROUPS
  resp = GET_CONF_GROUP_DISARM_CHAIN(conf.group[groupNum].setting); // Temp variable
  if ((resp != DUMMY_GROUP) &&
      (resp != master) &&
      (master != DUMMY_NO_VALUE) &&
      (hop <= ALARM_GROUPS)) {
    hop++; // Increase hop
    disarmGroup(resp, master, hop);
  }
}
/*
 * sdbm hash - http://www.cse.yorku.ca/~oz/hash.html
 */
uint32_t sdbmHash(uint8_t *toHash, uint8_t length) {
  uint32_t hash = 0;
  uint8_t  c;

  while (length) {
    c = *toHash++;
    hash = c + (hash << 6) + (hash << 16) - hash;
    length--;
  }
  return hash;
}
/*
 * Check key value to saved keys
 */
uint8_t checkKey(uint8_t groupNum, armType_t armType, uint8_t *key, uint8_t length){
  uint8_t resp = DUMMY_NO_VALUE;

  // Group is allowed and enabled
  chprintf(console, "Check key for group: %u, type: %s\r\n", groupNum, groupState[armType + 1]); // 0 = disarmed
  if ((groupNum < ALARM_GROUPS) && (GET_CONF_GROUP_ENABLED(conf.group[groupNum].setting))) {
    // Check all keys
    uint32_t keyHash = sdbmHash(key, length);
    for (uint8_t i=0; i < KEYS_SIZE; i++){
      //chprintf(console, "Key check: %d\r\n", i);
      //for(uint8_t ii = 0; ii < KEY_LENGTH; ii++) { chprintf(console, "%d-%x, ", ii, key[ii]); } chprintf(console, "\r\n");
      //for(uint8_t ii = 0; ii < KEY_LENGTH; ii++) { chprintf(console, "%d-%x, ", ii, conf.keyValue[i][ii]); } chprintf(console, "\r\n");
      if (conf.key[i].value == keyHash) { // key matched
        chprintf(console, "Key matched: %d\r\n", i);
        resp = i; // Assign matched key
        //  key enabled && user enabled && (group = contact_group || contact_key = global)
        if (GET_CONF_KEY_ENABLED(conf.key[i].setting) &&
            GET_CONF_CONTACT_ENABLED(conf.contact[conf.key[i].contact].setting) &&
           (groupNum == GET_CONF_CONTACT_GROUP(conf.contact[conf.key[i].contact].setting) ||
            GET_CONF_CONTACT_IS_GLOBAL(conf.contact[conf.key[i].contact].setting))) {
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
          break; // no need to try other
        }
      } // key matched
      else if (i == KEYS_SIZE-1) {
        // Log unknown keys
        tmpLog[0] = 'A'; tmpLog[1] = 'U'; memcpy(&tmpLog[2], &keyHash, KEY_LENGTH); pushToLog(tmpLog, 10);
      }
    } // for
  } else {
    tmpLog[0] = 'G'; tmpLog[1] = 'F'; tmpLog[2] = groupNum;  pushToLog(tmpLog, 3);
  }
  return resp;
}
/*
 * Set timer according defined rules
 */
void setTimer(const uint8_t timerIndex, const bool restart) {
  uint8_t day, found;
  time_t tempTime, addTime;
  RTCDateTime tempTimeSpec;

  // To accommodate DST use this double conversion
  tempTime = getTimeUnixSec();
  convertUnixSecondToRTCDateTime(&tempTimeSpec, tempTime);

  // Calendar
  if (GET_CONF_TIMER_TYPE(conf.timer[timerIndex].setting)) {
    chprintf(console, "Calendar: %u", timerIndex + 1);
    day = tempTimeSpec.dayofweek; found = 0; addTime = 0;
    chprintf(console, ", today: %u", day);
    for (uint8_t i = 0; i < 8; i++) {
      // day 1 = Monday, 7 = Sunday
      if ((conf.timer[timerIndex].setting >> (9 - day)) & 0b1) {
        if (day != tempTimeSpec.dayofweek) {
          found = 1; break;
        } else {
          // Today start_time has passed already?
          chprintf(console, ", time: %u - %u", tempTimeSpec.millisecond / 1000, conf.timer[timerIndex].startTime * SECONDS_PER_MINUTE);
          if (((tempTimeSpec.millisecond / 1000) > (conf.timer[timerIndex].startTime * SECONDS_PER_MINUTE)) &&
                (addTime == 0)) {
            addTime++;
          } else {
            found = 1; break;
          }
        }
      } else {
        addTime++;
      }
      day++; if (day == 8) day = 1;
      chprintf(console, ", day: %u", day);
    }
    if (found) {
      chprintf(console, ", addTime: %u", addTime);
      // Calculate On time
      tempTimeSpec.millisecond = 0;
      conf.timer[timerIndex].nextOn = convertRTCDateTimeToUnixSecond(&tempTimeSpec)
          + ((uint32_t)conf.timer[timerIndex].startTime * SECONDS_PER_MINUTE)
          + ((uint32_t)addTime * SECONDS_PER_DAY);
    } else {
      conf.timer[timerIndex].nextOn = 0;
    }
    chprintf(console, "\r\n");
  } else {
    // Periods
    chprintf(console, "Period: %u", timerIndex + 1);
    switch(GET_CONF_TIMER_PERIOD_TYPE(conf.timer[timerIndex].setting)) {
      case 0:  addTime = (uint32_t)conf.timer[timerIndex].periodTime; break;
      case 1:  addTime = (uint32_t)conf.timer[timerIndex].periodTime * SECONDS_PER_MINUTE; break;
      case 2:  addTime = (uint32_t)conf.timer[timerIndex].periodTime * SECONDS_PER_HOUR; break;
      default: addTime = (uint32_t)conf.timer[timerIndex].periodTime * SECONDS_PER_DAY; break;
    }
    chprintf(console, " addTime: %u", addTime);
    if (addTime > 0) {
      // Request come from Web interface, recalculate nextOn
      if (restart) {
        tempTimeSpec.millisecond = 0;
        conf.timer[timerIndex].nextOn = convertRTCDateTimeToUnixSecond(&tempTimeSpec)
            + ((uint32_t)conf.timer[timerIndex].startTime * SECONDS_PER_MINUTE);
        chprintf(console, " next_on: %u", conf.timer[timerIndex].nextOn);
        // if next_on is in past calculate new next_on
        if (tempTime > conf.timer[timerIndex].nextOn) {
          tempTime = (tempTime - conf.timer[timerIndex].nextOn) / addTime;
          chprintf(console, " tempTime: %u", tempTime);
          conf.timer[timerIndex].nextOn += (tempTime + 1) * addTime;
        }
      } else {
        conf.timer[timerIndex].nextOn += addTime;
      }
      chprintf(console, " next_on: %u\r\n", conf.timer[timerIndex].nextOn);
    }
  }
  // Set Off time
  conf.timer[timerIndex].nextOff = conf.timer[timerIndex].nextOn;
  switch(GET_CONF_TIMER_RUN_TYPE(conf.timer[timerIndex].setting)){
    case 0:  conf.timer[timerIndex].nextOff += (uint32_t)conf.timer[timerIndex].runTime; break;
    case 1:  conf.timer[timerIndex].nextOff += (uint32_t)conf.timer[timerIndex].runTime * SECONDS_PER_MINUTE; break;
    case 2:  conf.timer[timerIndex].nextOff += (uint32_t)conf.timer[timerIndex].runTime * SECONDS_PER_HOUR; break;
    default: conf.timer[timerIndex].nextOff += (uint32_t)conf.timer[timerIndex].runTime * SECONDS_PER_DAY; break;
  }
  CLEAR_CONF_TIMER_TRIGGERED(conf.timer[timerIndex].setting); // switch OFF Is triggered
}
/*
 * Is char* a valid phone number
 * valid 0..9+ && len => 9
 */
bool isPhoneNum(const char* num) {
  bool ret = true;
  uint8_t len = 0;

  while (*num) {
    if (((*num < '0') || (*num > '9')) && (*num != '+')) ret = false;
    num++; len++;
  }
  if (len < 9) ret = false;

  return ret;
}
/*
 * Print node type
 */
void printNodeType(BaseSequentialStream *chp, const char type) {
  switch(type){
    case 'K': chprintf(chp, "%s", text_Authentication); break;
    case 'S': chprintf(chp, "%s", text_Sensor); break;
    case 'I': chprintf(chp, "%s", text_Output); break;
    default: chprintf(chp, "%s", text_Undefined); break;
  }
}
/*
 * Print node function
 */
void printNodeFunction(BaseSequentialStream *chp, const char function) {
  switch(function){
    case 'i': chprintf(chp, "%s", text_iButton); break;
    case 'T': chprintf(chp, "%s", text_Temperature); break;
    case 'H': chprintf(chp, "%s", text_Humidity); break;
    case 'P': chprintf(chp, "%s", text_Pressure); break;
    case 'V': chprintf(chp, "%s", text_Voltage); break;
    case 'B': chprintf(chp, "%s", text_Battery); break;
    case 'D': chprintf(chp, "%s", text_Digital); break;
    case 'A': chprintf(chp, "%s", text_Analog); break;
    case 'F': chprintf(chp, "%s", text_Float); break;
    case 'X': chprintf(chp, "%s", text_TX_Power); break;
    case 'G': chprintf(chp, "%s", text_Gas); break;
    default : chprintf(chp, "%s", text_Undefined); break;
  }
}
/*
 * Print node address and node name or NOT_SET.
 * printName: If name is known print also name or not_found
 */
void printNodeAddress(BaseSequentialStream *chp, const uint8_t address, const char type,
                      const char function, const uint8_t number, const bool printName) {
  uint8_t nodeIndex = 0;
  // If address is defined
  if (address) {
    if (address < RADIO_UNIT_OFFSET) { chprintf(chp, "W:%u:", address); }
    else                             { chprintf(chp, "R:%u:", address-RADIO_UNIT_OFFSET); }
    chprintf(chp, "%c:%c:%u", type, function, number);
    if (printName) {
      nodeIndex = getNodeIndex(address, type, function, number);
      if (nodeIndex != DUMMY_NO_VALUE) chprintf(chp, " - %s", node[nodeIndex].name);
      else chprintf(chp, " - %s", text_not_found);
    }
  } else {
    chprintf(chp, "%s", NOT_SET);
  }
}
/*
 * Print formated time stamp according to user defined strftime
 */
void printFrmTimestamp(BaseSequentialStream *chp, time_t *value) {
  struct tm *ptm;
  char   dateTime[30];

  // 0xffffffff is time of empty FRAM cell, make it 0
  if (*value == 0xffffffff) *value = 0;

  ptm = gmtime(value);
  // Check if return is 0 then format is invalid
  if (strftime(dateTime, 30, conf.dateTimeFormat, ptm) != 0) chprintf(chp, "%s", dateTime);
  else chprintf(chp, "%s", text_unknown);
}
/*
 * Print formated up time as days and time
 */
void printFrmUpTime(BaseSequentialStream *chp, time_t *value) {
  uint16_t days = *value / (time_t)SECONDS_PER_DAY;
  *value -= (days * (time_t)SECONDS_PER_DAY);
  uint8_t hours = *value / (time_t)SECONDS_PER_HOUR;
  *value -= (hours * (time_t)SECONDS_PER_HOUR);
  uint8_t minutes = *value / (time_t)SECONDS_PER_MINUTE;
  *value -= (minutes * (time_t)SECONDS_PER_MINUTE);

  chprintf(chp, "%u day(s), %02u:%02u:%02u", days, hours, minutes, (uint32_t)*value);
}
/*
 * Print key HEX value
 */
void printKey(BaseSequentialStream *chp, const char *value){
  for (uint8_t i = KEY_LENGTH; i > 0 ; i--) {
    chprintf(chp, "%02x", value[i - 1]);
  }
}
/*
 * Print group number and name, or NOT_SET
 */
void printGroup(BaseSequentialStream *chp, const uint8_t value) {
  if (value < ALARM_GROUPS) {
    chprintf(chp, "%u. %s ", value + 1, conf.group[value].name);
  } else chprintf(chp, "%s ", NOT_SET);
}
/*
 * Print zone number and name
 */
void printZone(BaseSequentialStream *chp, const uint8_t value) {
  if (value < ALARM_ZONES) {
    chprintf(chp, "%u. %s ", value + 1, conf.zoneName[value]);
  } else chprintf(chp, "%s ", NOT_SET);
}
/*
 * Decode log entries to string
 * full: decode full string, or just short version for alerts.html
 */
static uint8_t decodeLog(char *in, char *out, bool full){
  uint8_t groupNum = DUMMY_NO_VALUE;
  memset(&out[0], 0x0, LOG_TEXT_LENGTH);
  MemoryStream ms;
  BaseSequentialStream *chp;
  // Memory stream object to be used as a string writer, reserving one byte for the final zero.
  msObjectInit(&ms, (uint8_t *)out, LOG_TEXT_LENGTH-1, 0);
  // Performing the print operation using the common code.
  chp = (BaseSequentialStream *)(void *)&ms;


  switch(in[0]){
    case 'S': // System
      chprintf(chp, "%s ", text_System);
      switch(in[1]){
        case 's': chprintf(chp, "%s", text_started); break; // boot
        case 'S': chprintf(chp, "%s %s", text_monitoring, text_started); break; // Zone thread start
        case 'X': chprintf(chp, "%s", text_alarm);
          if (full) chprintf(chp, "! %s %u.%s", text_Group, (uint8_t)in[2] + 1, conf.group[(uint8_t)in[2]].name);
          groupNum = (uint8_t)in[2];
          break;
        case 'B': chprintf(chp, "%s ", text_battery);
          if (full) {
            switch(in[2]){
              case 'L': chprintf(chp, "%s", text_low); break;
              default:  chprintf(chp, "%s", text_OK); break;
            }
          } else {
            chprintf(chp, " %s", text_state);
          }
          break;
        case 'A': chprintf(chp, "%s %s ", text_main, text_power);
          if (full) {
            switch(in[2]){
              case 'L': chprintf(chp, "%s", text_Off); break;
              default:  chprintf(chp, "%s", text_On); break;
            }
          } else {
            chprintf(chp, "%s", text_state);
          }
          break;
        case 'R': chprintf(chp, "%s %s ", text_RTC, text_battery);
          if (full) {
            switch(in[2]){
              case 'L': chprintf(chp, "%s", text_low); break;
              default:  chprintf(chp, "%s", text_OK); break;
            }
          } else {
            chprintf(chp, " %s", text_state);
          }
          break;
        default: chprintf(chp, "%s", text_unknown); break; // unknown
      }
    break;
    case 'N': // Remote nodes
      printNodeType(chp, in[3]); chprintf(chp, ":");
      printNodeFunction(chp, in[4]);
      chprintf(chp, " %s ", text_address);
      printNodeAddress(chp, (uint8_t)in[2], (uint8_t)in[3], (uint8_t)in[4], (uint8_t)in[5], false);
      if (in[1] != 'E') {chprintf(chp, " %s ", text_is);}
      switch(in[1]){
        case 'Z' : chprintf(chp, "%s", text_removed); break;
        case 'F' : chprintf(chp, "%s", text_disabled); break;
        case 'R' : chprintf(chp, "%s", text_registered); break;
        case 'r' : chprintf(chp, "%s%s", text_re, text_registered); break;
        default : chprintf(chp, "%s %s", text_registration, text_error); break; // 'E'
      }
    break;
    case 'M': // Modem
      chprintf(chp, "%s ", text_Modem);
      if ((uint8_t)in[1] <= 5) {
        chprintf(chp, "%s ", text_network);
        switch(in[1]){
          case 0 : chprintf(chp, "%s %s", text_not, text_registered); break;
          case 1 : chprintf(chp, "%s", text_registered); break;
          case 2 : chprintf(chp, "%s", text_searching); break;
          case 3 : chprintf(chp, "%s %s", text_registration, text_denied); break;
          case 5 : chprintf(chp, "%s", text_roaming); break;
          default : chprintf(chp, "%s", text_unknown); break; // 4 = unknown
        }
        chprintf(chp, "%s%s %u%%", text_cosp, text_strength, (uint8_t)in[2]);
      } else {
        chprintf(chp, "%s ", text_power);
        switch(in[1]){
          case 'O' : chprintf(chp, "%s", text_On); break;
          case 'F' : chprintf(chp, "%s", text_Off); break;
          default : chprintf(chp, "%s", text_failure); break;
        }
      }
    break;
    case 'G': // Group related
      chprintf(chp, "%s ", text_Group);
      if (full) {
        printGroup(chp, (uint8_t)in[2]);
      }
      switch(in[1]){
        case 'F': chprintf(chp, "%s %s", text_is, text_disabled); break;
        case 'S': chprintf(chp, "%s", text_armed); break;
        case 'D': chprintf(chp, "%s", text_disarmed); break;
        case 'A': chprintf(chp, "%s %s", text_auto, text_armed); break;
        default: chprintf(chp, "%s", text_unknown); break;
      }
      groupNum = (uint8_t)in[2];
    break;
    case 'Z': // Zone
      chprintf(chp, "%s ", text_Zone);
      if (full) {
        if ((uint8_t)in[2] < ALARM_ZONES) {
          chprintf(chp, "%u %s ", (uint8_t)in[2] + 1, conf.zoneName[(uint8_t)in[2]]);
        } else {
          chprintf(chp, "%s ", text_unknown);
        }
      }
      switch(in[1]){
        case 'P': chprintf(chp, "%s", text_alarm); break;
        case 'T': chprintf(chp, "%s", text_tamper); break;
        case 'O': chprintf(chp, "%s", text_open); break;
        case 'R': chprintf(chp, "%s", text_registered); break;
        case 'r': chprintf(chp, "%s%s", text_re, text_registered); break;
        case 'E': chprintf(chp, "%s %s", text_registration, text_error); break;
        case 'e': chprintf(chp, "%s, %s %s ", text_error, text_address, text_not);
          switch(in[3]){
            case 'M': chprintf(chp, "%s", text_matched); break;
            default : chprintf(chp, "%s", text_allowed); break;
          }
        break;
        default: chprintf(chp, "%s", text_unknown); break;
      }
      groupNum = GET_CONF_ZONE_GROUP((uint8_t)in[2]);
    break;
    case 'A': // Authentication
      chprintf(chp, "%s ", text_Key);
      if (full) {
        if (in[1] != 'U') {
          chprintf(chp, "#%u, %s ", (uint8_t)in[2] + 1, text_linked_to);
          if (conf.key[(uint8_t)in[2]].contact == DUMMY_NO_VALUE) chprintf(chp, "%s ", NOT_SET);
          else chprintf(chp, "%s ", conf.contact[(conf.key[(uint8_t)in[2]].contact)]);
          groupNum = GET_CONF_CONTACT_GROUP(conf.key[(uint8_t)in[2]].contact);
        }
      }
      switch(in[1]){
        case 'D': chprintf(chp, "%s", text_disarmed); break;
        case 'A': chprintf(chp, "%s %s", text_armed, text_away); break;
        case 'H': chprintf(chp, "%s %s", text_armed, text_home); break;
        case 'U': chprintf(chp, "%s %s ", text_is, text_unknown);
          if (full) {
            printKey(chp, &in[2]);
          }
          break;
        case 'F': chprintf(chp, "%s %s", text_is, text_disabled); break;
        default : chprintf(chp, "%s", text_unknown); break;
      }

    break;
    case 'F': // Fifos
      switch(in[1]){
        case 'S' : chprintf(chp, "%s", text_Sensor); break;
        case 'T' : chprintf(chp, "%s", text_Trigger); break;
        case 'R' : chprintf(chp, "%s", text_Registration); break;
        case 'A' : chprintf(chp, "%s", text_Alarm); break;
        case 'N' : chprintf(chp, "%s", text_Node); break;
        case 'L' : chprintf(chp, "%s", text_Script); break;
        case 'M' : chprintf(chp, "%s", text_MQTT); break;
        default : chprintf(chp, "%s", text_unknown); break;
      }
      chprintf(chp, " %s %s", text_queue, text_full);
    break;
    case 'R': // Triggers
      chprintf(chp, "%s %u. %s", text_Trigger, (uint8_t)in[2], conf.trigger[(uint8_t)in[2]].name);
      switch(in[1]){
        case 'A' : chprintf(chp, "%s", text_activated); break;
        case 'N' : chprintf(chp, "de%s", text_activated); break;
        default : chprintf(chp, "%s", text_unknown); break;
      }
      chprintf(chp, " %s %s", text_queue, text_full);
    break;
    case 'Q': // MQTT
      chprintf(chp, "%s ", text_MQTT);
      switch(in[1]){
        case 'E' : chprintf(chp, "%s, ", text_error);
          switch(in[2]) {
            case 'R' : chprintf(chp, "%s %s %s %s", text_unable, text_to, text_resolve, text_address); break;
            case 'C' : chprintf(chp, "%s %s %s", text_unable, text_to, text_connect); break;
            case 'P' : chprintf(chp, "%s %s", text_publish, text_error); break;
            case 'S' : chprintf(chp, "%s %s", text_subscribe, text_error); break;
            // MQTT 1 - 7 callback reason errors
            case '1' : chprintf(chp, "%s %s %s", text_refused, text_protocol, text_version); break;
            case '2' : chprintf(chp, "%s %s", text_refused, text_identifier); break;
            case '3' : chprintf(chp, "%s %s", text_refused, text_server); break;
            case '4' : chprintf(chp, "%s %s/%s", text_refused, text_user, text_password); break;
            case '5' : chprintf(chp, "%s %s", text_not, text_authorized); break;
            case '6' : // 256
              chprintf(chp, "%s", text_disconnected); break;
            case '7' : // 257
              chprintf(chp, "%s", text_timeout); break;
            default : chprintf(chp, "%s", text_unknown); break;
          }
          break;
        case 'C' : chprintf(chp, "%s", text_connected); break;
        default : chprintf(chp, "%s", text_unknown); break;
      }
      break;
    case 0xff:
      chprintf(chp, "%s", text_Empty);
    break;
    default: chprintf(chp, "%s", text_Undefined);
      for(uint16_t ii = 0; ii < LOGGER_MSG_LENGTH; ii++) {
        chprintf(chp, "-%x", in[ii], in[ii]);
      }
    break; // unknown
  }
  //chprintf(chp, "."); // "." as end

  return groupNum;
}

#endif /* OHS_FUNCTIONS_H_ */
