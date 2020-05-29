/*
 * ohs_functions.h
 *
 *  Created on: 16. 12. 2019
 *      Author: adam
 */

#ifndef OHS_FUNCTIONS_H_
#define OHS_FUNCTIONS_H_

/*
 * Days in a month
 */
static const uint8_t TM_RTC_Months[2][12] = {
    {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},   /* Not leap year */
    {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}    /* Leap year */
};
/*
 * Convert RTCDateTime to seconds not using tm (ISO C `broken-down time' structure.)
 */
time_t convertRTCDateTimeToUnixSecond(RTCDateTime *dateTime) {
  uint32_t days = 0, seconds = 0;
  uint16_t i;
  uint16_t year = (uint16_t) (dateTime->year + 1980);

  // Year is below offset year
  if (year < RTC_OFFSET_YEAR) return 0;
  // Days in previus years
  for (i = RTC_OFFSET_YEAR; i < year; i++) {
    days += RTC_DAYS_IN_YEAR(i);
  }
  // Days in current year
  for (i = 1; i < dateTime->month; i++) {
    days += TM_RTC_Months[RTC_LEAP_YEAR(year)][i - 1];
  }
  // Day starts with 1
  days += dateTime->day - 1;
  seconds = days * SECONDS_PER_DAY;
  seconds += dateTime->millisecond / 1000;

  return seconds;
}
/*
 * Get Daylight Saving Time for particular rule
 *
 * @parm year - 1900
 * @parm month - 1=Jan, 2=Feb, ... 12=Dec
 * @parm week - 1=First, 2=Second, 3=Third, 4=Fourth, or 0=Last week of the month
 * @parm dow, Day of week - 0=Sun, 1=Mon, ... 6=Sat
 * @parm hour - 0 - 23
 * @retval seconds
 */
time_t calculateDST(uint16_t year, uint8_t month, uint8_t week, uint8_t dow, uint8_t hour){
  RTCDateTime dstDateTime;
  time_t rawtime;
  uint8_t _week = week; // Local copy

  if (_week == 0) {      //Last week = 0
    if (month++ > 12) {  //for "Last", go to the next month
      month = 1;
      year++;
    }
    _week = 1;            //and treat as first week of next month, subtract 7 days later
  }
  // First day of the month, or first day of next month for "Last" rules
  dstDateTime.year = year;
  dstDateTime.month = month ;
  dstDateTime.day = 1;
  dstDateTime.millisecond = hour * SECONDS_PER_HOUR * 1000;

  // Do DST
  rawtime = convertRTCDateTimeToUnixSecond(&dstDateTime);
  //chprintf(console, "DST: %d\r\n", rawtime);

  // Weekday function by Michael Keith and Tom Craver
  year += 1980;
  uint8_t weekday = (dstDateTime.day += month < 3 ?
      year-- : year - 2, 23 * month / 9 + dstDateTime.day + 4 + year/4 - year/100 + year/400)%7;
  //chprintf(console, "DST: %d\r\n", weekday);

  rawtime += ((7 * (_week - 1) + (dow - weekday + 7) % 7) * SECONDS_PER_DAY);
  //chprintf(console, "DST: %d\r\n", rawtime);

  //back up a week if this is a "Last" rule
  if (week == 0) {
    rawtime = rawtime - (7 * SECONDS_PER_DAY);
  }

  return rawtime;
}
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
 * Convert seconds to RTCDateTime not using tm (ISO C `broken-down time' structure.)
 */
void convertUnixSecondToRTCDateTime(RTCDateTime* dateTime, uint32_t unixSeconds) {
  uint16_t year;

  // Get milliseconds
  dateTime->millisecond = (unixSeconds % SECONDS_PER_DAY) * 1000;
  unixSeconds /= SECONDS_PER_DAY;
  // Get week day, Monday is day one
  dateTime->dayofweek = (unixSeconds + 3) % 7 + 1;
  // Get year
  year = 1970;
  while (true) {
    if (RTC_LEAP_YEAR(year)) {
      if (unixSeconds >= 366) {
        unixSeconds -= 366;
      } else {
        break;
      }
    } else if (unixSeconds >= 365) {
      unixSeconds -= 365;
    } else {
      break;
    }
    year++;
  }
  // Get year in xx format
  dateTime->year = year - 1980;
  // Get month
  for (dateTime->month = 0; dateTime->month < 12; dateTime->month++) {
    if (RTC_LEAP_YEAR(year)) {
      if (unixSeconds >= (uint32_t)TM_RTC_Months[1][dateTime->month]) {
        unixSeconds -= TM_RTC_Months[1][dateTime->month];
      } else {
        break;
      }
    } else if (unixSeconds >= (uint32_t)TM_RTC_Months[0][dateTime->month]) {
      unixSeconds -= TM_RTC_Months[0][dateTime->month];
    } else {
      break;
    }
  }
  // Month starts with 1
  dateTime->month++;
  // Get day, day starts with 1
  dateTime->day = unixSeconds + 1;
}
/*
 *
 */
void setTimeUnixSec(time_t unix_time) {
  struct tm tim;
  struct tm *canary;

  /* If the conversion is successful the function returns a pointer
     to the object the result was written into.*/
  canary = localtime_r(&unix_time, &tim);
  osalDbgCheck(&tim == canary);

  rtcConvertStructTmToDateTime(&tim, 0, &timespec);
  rtcSetTime(&RTCD1, &timespec);
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
    chprintf(console, "L P full %d \r\n", outMsg);
  }
}
/*
 * Logger for text only messages
 */
void pushToLogText(char *what) {
  uint8_t len = strlen(what);
  if (len > LOGGER_MSG_LENGTH) len = LOGGER_MSG_LENGTH;
  pushToLog(what, len);
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

    chprintf(console, "RS485 Send cmd: %d to address: %d\r\n", command, address);
    rs485Cmd.address = address;
    rs485Cmd.length = command;
    if (rs485SendCmdWithACK(&RS485D2, &rs485Cmd, 3) == MSG_OK) resp = 1;
    else resp = -1;
  }
  // Radio
  if (address >= RADIO_UNIT_OFFSET) {
    char radioCmd[] = {'C', command};

    chprintf(console, "Radio Send cmd: %d to address: %d\r\n", command, address - RADIO_UNIT_OFFSET);
    resp = rfm69Send(address - RADIO_UNIT_OFFSET, radioCmd, sizeof(radioCmd), true);
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
      // Save group state, here we save armDelay and armType
      writeToBkpRTC((uint8_t*)&group, sizeof(group), 0);
      // Triggers
      sensorEvent_t *outMsgT = chPoolAlloc(&trigger_pool);
      if (outMsgT != NULL) {
        outMsgT->type = 'G';
        outMsgT->address = 0;
        outMsgT->function = ' ';
        outMsgT->number = groupNum;
        // As defined in groupState[], 0 = disarmed
        outMsgT->value = (float)(armType + 1);
        msg_t msg = chMBPostTimeout(&trigger_mb, (msg_t)outMsgT, TIME_IMMEDIATE);
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
  resp = GET_CONF_GROUP_ARM_CHAIN(conf.group[groupNum]); // Temp variable
  if ((resp != 15) &&
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
  // Save group state
  writeToBkpRTC((uint8_t*)&group, sizeof(group), 0);
  // Triggers
  sensorEvent_t *outMsg = chPoolAlloc(&trigger_pool);
  if (outMsg != NULL) {
    outMsg->type = 'G';
    outMsg->address = 0;
    outMsg->function = ' ';
    outMsg->number = groupNum;
    // As defined in groupState[], 0 = disarmed
    outMsg->value = 0;
    msg_t msg = chMBPostTimeout(&trigger_mb, (msg_t)outMsg, TIME_IMMEDIATE);
    if (msg != MSG_OK) {
      //chprintf(console, "S-MB full %d\r\n", temp);
    }
  } else {
    pushToLogText("FT"); // Trigger queue is full
  }

  // If Disarm another group is set and another group is not original(master)
  // and hop is lower then ALR_GROUPS
  resp = GET_CONF_GROUP_DISARM_CHAIN(conf.group[groupNum]); // Temp variable
  if ((resp != 15) &&
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
void checkKey(uint8_t groupNum, armType_t armType, uint8_t *key, uint8_t length){
  // Group is allowed and enabled
  chprintf(console, "Check key for group: %u, arm type: %u\r\n", groupNum, armType);
  if ((groupNum < ALARM_GROUPS) && (GET_CONF_GROUP_ENABLED(conf.group[groupNum]))) {
    // Check all keys
    uint32_t keyHash = sdbmHash(key, length);
    for (uint8_t i=0; i < KEYS_SIZE; i++){
      //chprintf(console, "Key check: %d\r\n", i);
      //for(uint8_t ii = 0; ii < KEY_LENGTH; ii++) { chprintf(console, "%d-%x, ", ii, key[ii]); } chprintf(console, "\r\n");
      //for(uint8_t ii = 0; ii < KEY_LENGTH; ii++) { chprintf(console, "%d-%x, ", ii, conf.keyValue[i][ii]); } chprintf(console, "\r\n");
      if (conf.keyValue[i] == keyHash) { // key matched
        chprintf(console, "Key matched: %d\r\n", i);
        //  key enabled && user enabled && (group = contact_group || contact_key = global)
        if (GET_CONF_KEY_ENABLED(conf.keySetting[i]) &&
            GET_CONF_CONTACT_ENABLED(conf.contact[conf.keyContact[i]]) &&
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
          break; // no need to try other
        }
      } // key matched
      else if (i == KEYS_SIZE-1) {
        // Log unknown keys
        tmpLog[0] = 'A'; tmpLog[1] = 'U'; memcpy(&tmpLog[2], &keyHash, KEY_LENGTH); pushToLog(tmpLog, 10);
        lastKey = keyHash; // store last unknown key
      }
    } // for
  } else {
    tmpLog[0] = 'G'; tmpLog[1] = 'F'; tmpLog[2] = groupNum;  pushToLog(tmpLog, 3);
  }
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

#endif /* OHS_FUNCTIONS_H_ */
