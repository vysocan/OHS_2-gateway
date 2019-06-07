/*
 * ohs_conf.h
 *
 *  Created on: 26. 9. 2018
 *      Author: Adam
 */

#ifndef OHS_CONF_H_
#define OHS_CONF_H_

#if STM32_BKPRAM_ENABLE == STM32_NO_INIT
#error "In mcuconf.h STM32_BKPRAM_ENABLE must be TRUE!"
#endif

#define OHS_MAJOR        0
#define OHS_MINOR        0

#define BACKUP_SRAM_SIZE 0x1000 // 4kB SRAM size
#define BACKUP_RTC_SIZE  80     // 80 bytes

#define ALARM_ZONES      30     // Maximum zones
#define ALARM_GROUPS     8      // Maximum groups
#define HW_ZONES         11     // # of hardware zones on gateway
#define CONTACTS_SIZE    10     // Maximum contacts
#define KEYS_SIZE        20     // Maximum keys
#define KEY_LENGTH       8      //
#define NAME_LENGTH      16     //
#define PHONE_LENGTH     14     //
#define EMAIL_LENGTH     30     //

#define ALARM_PIR        3400 //(3380 = 15.2V)
#define ALARM_PIR_LOW    3050
#define ALARM_PIR_HI     3650
#define ALARM_OK         1850 //(1850 = 15.2V)
#define ALARM_OK_LOW     1500
#define ALARM_OK_HI      2100
#define ALARM_TAMPER     0

#define RADIO_UNIT_OFFSET 15



#define NOT_SET          "not set"

// Configuration struct
typedef struct {
  uint8_t  versionMajor;
  uint8_t  versionMinor;

  uint16_t logOffset;
  uint8_t  alarmTime;
  char     dateTimeFormat[NAME_LENGTH];

  uint16_t zone[ALARM_ZONES];
  char     zoneName[ALARM_ZONES][NAME_LENGTH];
  uint8_t  zoneAddress[ALARM_ZONES-HW_ZONES];          // Remote zone address

  uint16_t group[ALARM_GROUPS];
  char     groupName[ALARM_GROUPS][NAME_LENGTH];

  uint8_t  contact[CONTACTS_SIZE];
  char     contactName[CONTACTS_SIZE][NAME_LENGTH];
  char     contactPhone[CONTACTS_SIZE][PHONE_LENGTH];
  char     contactEmail[CONTACTS_SIZE][EMAIL_LENGTH];

  uint8_t  key[KEYS_SIZE];
  char     keyValue[KEYS_SIZE][KEY_LENGTH];
  char     keyName[KEYS_SIZE][NAME_LENGTH];



  char     NTPAddress[32];
  uint8_t  time_std_week;   //First, Second, Third, Fourth, or Last week of the month
  uint8_t  time_std_dow;    //day of week, 1=Sun, 2=Mon, ... 7=Sat
  uint8_t  time_std_month;  //1=Jan, 2=Feb, ... 12=Dec
  uint8_t  time_std_hour;   //0-23
  int16_t  time_std_offset; //offset from UTC in minutes

  uint8_t  time_dst_week;   //First, Second, Third, Fourth, or Last week of the month
  uint8_t  time_dst_dow;    //day of week, 1=Sun, 2=Mon, ... 7=Sat
  uint8_t  time_dst_month;  //1=Jan, 2=Feb, ... 12=Dec
  uint8_t  time_dst_hour;   //0-23
  int16_t  time_dst_offset; //offset from UTC in minutes
} config_t;
config_t conf;

// Group runtime variables
typedef struct {
  uint8_t setting;
  uint8_t armDelay;
} group_t;
group_t group[ALARM_GROUPS];

// Zone runtime variables
typedef struct {
  time_t  lastPIR;
  time_t  lastOK;
  char    lastEvent;
  uint8_t setting;
} zone_t;
zone_t zone[ALARM_ZONES];

// Flags runtime variables
typedef struct {
  uint16_t flags;
} flags_t;
flags_t flags;

// Set default to runtime structs
void initRuntimeGroups(void){
  for(uint8_t i = 0; i < ALARM_GROUPS; i++) {
  //                      |- Disabled group log once flag
  //                      ||- Free
  //                      |||- Free
  //                      ||||- Free
  //                      |||||- Armed Home
  //                      ||||||-  Waiting for authorization
  //                      |||||||-  Alarm
  //                      ||||||||-  Armed
  //                      76543210
    group[i].setting  = 0b00000000;
    group[i].armDelay = 0;
  }
}
void initRuntimeZones(void){
  for(uint8_t i = 0; i < ALARM_ZONES; i++) {
    zone[i].lastPIR   = 0;
    zone[i].lastOK    = 0;
    zone[i].lastEvent = 'N';
    //                     |- Full FIFO queue flag
    //                     ||- Message queue
    //                     |||- Error flag, for remote zone
    //                     ||||- Free
    //                     |||||- Free
    //                     ||||||- Free
    //                     |||||||- Alarm
    //                     ||||||||- Free
    //                     76543210
    zone[i].setting    = 0b00000000;
  }
}


/*
 * SRAM/RTC backup related functions
 */
// Write to backup SRAM
int16_t writeToBkpSRAM(uint8_t *data, uint16_t size, uint16_t offset){
  if (size + offset >= BACKUP_SRAM_SIZE) osalSysHalt("SRAM out of region"); // Data out of BACKUP_SRAM_SIZE region
  uint16_t i = 0;
  uint8_t *baseAddress = (uint8_t *) BKPSRAM_BASE;
  for(i = 0; i < size; i++) {
    *(baseAddress + offset + i) = *(data + i);
  }
  return i;
}
// Read from backup SRAM
int16_t readFromBkpSRAM(uint8_t *data, uint16_t size, uint16_t offset){
  if (size + offset >= BACKUP_SRAM_SIZE) osalSysHalt("SRAM out of region");; // Data out of BACKUP_SRAM_SIZE region
  uint16_t i = 0;
  uint8_t *baseAddress = (uint8_t *) BKPSRAM_BASE;
  for(i = 0; i < size; i++) {
    *(data + i) = *(baseAddress + offset + i);
  }
  return i;
}

// Write to backup RTC
int16_t writeToBkpRTC(uint8_t *data, uint8_t size, uint8_t offset){
  if (size + offset >= BACKUP_RTC_SIZE) osalSysHalt("RTC out of region"); // Data out of BACKUP_RTC_SIZE region
  if (offset % 4) osalSysHalt("RCT misaligned");                          // Offset is not aligned to to unint32_t registers
  uint8_t i = 0;
  volatile uint32_t *RTCBaseAddress = &(RTC->BKP0R);
  uint32_t tmp = 0;
  for(i = 0; i < size; i++) {
    tmp |= (*(data + i) << ((i % 4)*8));
    if (((i % 4) == 3) || ((i + 1) == size)) {
      *(RTCBaseAddress + offset + (i/4)) = tmp;
      tmp = 0;
    }
  }
  return i;
}
// Read from backup RTC
int16_t readFromBkpRTC(uint8_t *data, uint8_t size, uint8_t offset){
  if (size + offset >= BACKUP_RTC_SIZE) osalSysHalt("RTC out of region"); // Data out of BACKUP_RTC_SIZE region
  if (offset % 4) osalSysHalt("RCT misaligned");                          // Offset is not aligned to to unint32_t registers
  uint8_t i = 0;
  volatile uint32_t *RTCBaseAddress = &(RTC->BKP0R);
  uint32_t tmp = 0;
  for(i = 0; i < size; i++) {
    if ((i % 4) == 0) { tmp = *(RTCBaseAddress + offset + (i/4)); }
    *(data + i) = (tmp >> ((i % 4)*8)) & 0xFF;
  }
  return i;
}


// Set conf default values
void setConfDefault(void){
  conf.versionMajor   = OHS_MAJOR;
  conf.versionMinor   = OHS_MINOR;
  conf.logOffset      = 0;
  conf.alarmTime      = 10;
  strcpy(conf.dateTimeFormat, "%T %a %F");

  for(uint8_t i = 0; i < ALARM_ZONES; i++) {
    // Zones setup
    //                    |- Digital 0/ Analog 1
    //                    ||- Present - connected
    //                    |||- ~ Free ~ TWI zone
    //                    ||||- Remote zone
    //                    |||||- Battery node, they dont send OK, only PIR or Tamper.
    //                    ||||||- Free
    //                    |||||||- PIR as Tamper
    //                    ||||||||- Still open alarm
    //                    ||||||||         |- Arm Home zone
    //                    ||||||||         |||- Auth time
    //                    ||||||||         |||- 0-3x the default time
    //                    ||||||||         |||||||- Group number
    //                    ||||||||         |||||||- 0 .. 15
    //                    ||||||||         |||||||-
    //                    ||||||||         |||||||-
    //                    ||||||||         ||||||||-  Enabled
    //                    54321098         76543210
    switch(i){
      case  0 ...  9:
         conf.zone[i] = 0b11000000 << 8 | 0b00011110; // Analog sensor
        break;
      case  10      :
         conf.zone[i] = 0b01000010 << 8 | 0b00011110; // Tamper
        break;
      default:
         conf.zone[i] = 0b00000000 << 8 | 0b00011110; // Other zones
         conf.zoneAddress[i-HW_ZONES] = 0;
        break;
    }
    strcpy(conf.zoneName[i], NOT_SET);
  }

  for(uint8_t i = 0; i < ALARM_GROUPS; i++) {
    //                  ||||- disarm chain
    //                  ||||
    //                  ||||
    //                  ||||
    //                  ||||||||- arm chain
    //                  ||||||||
    //                  ||||||||
    //                  ||||||||
    //                  ||||||||         |- MQTT publish
    //                  ||||||||         ||- Free
    //                  ||||||||         |||- Auto arm
    //                  ||||||||         ||||- PIR signal output 1
    //                  ||||||||         |||||- PIR signal output 2
    //                  ||||||||         ||||||-  Tamper signal output 1
    //                  ||||||||         |||||||-  Tamper signal output 2
    //                  ||||||||         ||||||||-  Enabled
    //                  54321098         76543210
    conf.group[i] = i << 12 | i << 8 | 0b00000000;
    strcpy(conf.groupName[i], NOT_SET);
  }

  for(uint8_t i = 0; i < CONTACTS_SIZE; i++) {
    // group 16 and disabled
    conf.contact[i] = 0b00011110;
    strcpy(conf.contactName[i], NOT_SET);
    strcpy(conf.contactPhone[i], NOT_SET);
    strcpy(conf.contactEmail[i], NOT_SET);
  }

  for(uint8_t i = 0; i < KEYS_SIZE; i++) {
    // group 16 and disabled
    conf.key[i] = 0b00011110;
    strcpy(conf.keyName[i], NOT_SET);
    strcpy(conf.keyValue[i], NOT_SET);
  }

  strcpy(conf.NTPAddress, "time.google.com");
  conf.time_std_week = 0;     //First, Second, Third, Fourth, or Last week of the month
  conf.time_std_dow = 0;      //day of week, 0=Sun, 1=Mon, ... 6=Sat
  conf.time_std_month = 10;   //1=Jan, 2=Feb, ... 12=Dec
  conf.time_std_hour = 3;     //0-23
  conf.time_std_offset = 60;  //offset from UTC in minutes
  conf.time_dst_week = 0;     //First, Second, Third, Fourth, or Last week of the month
  conf.time_dst_dow = 0;      //day of week, 0=Sun, 1=Mon, ... 6=Sat
  conf.time_dst_month = 3;    //1=Jan, 2=Feb, ... 12=Dec
  conf.time_dst_hour = 2;     //0-23
  conf.time_dst_offset = 120; //offset from UTC in minutes
}

//Year = 20**
//1=First,2=Second,3=Third,4=Fourth, or 0=Last week of the month
//day of week, 0=Sun, 1=Mon, ... 6=Sat
//1=Jan, 2=Feb, ... 12=Dec
//0-23=hour

#define SECS_PER_DAY 86400UL
time_t calculateDST(uint16_t year, uint8_t month, uint8_t week, uint8_t dow,  uint8_t hour){
  struct tm* ptm;
  time_t rawtime;
  uint8_t _month = month, _week = week;  //temp copies of month and week

  ptm = gmtime(0);

  if (_week == 0) {       //Last week = 0
    if (_month++ > 12) {  //for "Last", go to the next month
      _month = 1;  year++;
    }
    _week = 1;            //and treat as first week of next month, subtract 7 days later
  }
  //first day of the month, or first day of next month for "Last" rules
  ptm->tm_year = year - 1900;
  ptm->tm_mon = _month - 1;
  ptm->tm_mday = 1;
  ptm->tm_hour = hour;
  ptm->tm_min = 0;
  ptm->tm_sec = 0;
  // Do DST
  rawtime = mktime(ptm) + ((7 * (_week - 1) + (dow - ptm->tm_wday + 7) % 7) * SECS_PER_DAY);

  //back up a week if this is a "Last" rule
  if (week == 0) {
    rawtime = rawtime - (7 * SECS_PER_DAY);
  }

  return rawtime;
}

#endif /* OHS_CONF_H_ */
