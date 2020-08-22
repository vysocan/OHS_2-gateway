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

// STM32 UID as Ethernet MAC
#define STM32_UUID ((uint32_t *)UID_BASE)

#define OHS_MAJOR        1
#define OHS_MINOR        2

#define BACKUP_SRAM_SIZE 0x1000 // 4kB SRAM size
#define BACKUP_RTC_SIZE  80     // 80 bytes

#define ALARM_GROUPS     10     // # of groups
#define ALARM_ZONES      30     // # of zones
#define HW_ZONES         11     // # of hardware zones on gateway
#define CONTACTS_SIZE    10     // # of contacts
#define KEYS_SIZE        20     // # of keys
#define TIMER_SIZE       10     // # of timers
#define TRIGGER_SIZE     10     // # of timers
#define KEY_LENGTH       4      // sizeof(uint32_t) / size of hash
#define NAME_LENGTH      16     //
#define PHONE_LENGTH     14     //
#define EMAIL_LENGTH     30     //
#define URL_LENGTH       32     // URL address
#define NOT_SET          "not set"

#define ALARM_PIR        3400   // (3380 = 15.2V)
#define ALARM_PIR_LOW    3050
#define ALARM_PIR_HI     3650
#define ALARM_OK         1850   // (1850 = 15.2V)
#define ALARM_OK_LOW     1500
#define ALARM_OK_HI      2100
#define ALARM_TAMPER     0
#define ALARM_UNBALANCED 500

#define RADIO_KEY_SIZE    17    // 16 + 1 for null termination
#define RADIO_UNIT_OFFSET 15
#define REGISTRATION_SIZE 22
#define NODE_SIZE         50    // Number of nodes

#define DUMMY_NO_VALUE    255
#define DUMMY_GROUP       15

#define AC_POWER_DELAY    60    // seconds

#define LOGGER_MSG_LENGTH 11
#define LOGGER_OUTPUT_LEN 25    // How many entries to show

// Parameter checks
#if NODE_SIZE >= DUMMY_NO_VALUE
#error "NODE_SIZE is set to high!"
#endif

// Time related
#define SECONDS_PER_DAY     86400U
#define SECONDS_PER_HOUR    3600U
#define SECONDS_PER_MINUTE  60U
#define MINUTES_PER_HOUR    60U
#define RTC_LEAP_YEAR(year) ((((year) % 4 == 0) && ((year) % 100 != 0)) || ((year) % 400 == 0))
#define RTC_DAYS_IN_YEAR(x) RTC_LEAP_YEAR(x) ? 366 : 365
#define RTC_OFFSET_YEAR     1970

// Node commands
#define NODE_CMD_ACK          0
#define NODE_CMD_REGISTRATION 1
#define NODE_CMD_PING         2
#define NODE_CMD_PONG         3
#define NODE_CMD_ARMING       10
#define NODE_CMD_ALARM        11
#define NODE_CMD_AUTH_1       12
#define NODE_CMD_AUTH_2       13
#define NODE_CMD_AUTH_3       14
#define NODE_CMD_ARMED        15
#define NODE_CMD_DISARM       16

// Bit wise macros for various settings
#define GET_CONF_ZONE_ENABLED(x)     ((x) & 0b1)
#define GET_CONF_ZONE_GROUP(x)       ((x >> 1U) & 0b1111)
#define GET_CONF_ZONE_AUTH_TIME(x)   ((x >> 5U) & 0b11)
#define GET_CONF_ZONE_ARM_HOME(x)    ((x >> 7U) & 0b1)
#define GET_CONF_ZONE_OPEN_ALARM(x)  ((x >> 8U) & 0b1)
#define GET_CONF_ZONE_PIR_AS_TMP(x)  ((x >> 9U) & 0b1)
#define GET_CONF_ZONE_BALANCED(x)    ((x >> 10U) & 0b1)
#define GET_CONF_ZONE_IS_BATTERY(x)  ((x >> 11U) & 0b1)
#define GET_CONF_ZONE_IS_REMOTE(x)   ((x >> 12U) & 0b1)
#define GET_CONF_ZONE_IS_PRESENT(x)  ((x >> 14U) & 0b1)
#define GET_CONF_ZONE_TYPE(x)        ((x >> 15U) & 0b1)
#define SET_CONF_ZONE_ENABLED(x)     x |= 1
#define SET_CONF_ZONE_GROUP(x,y)     x = (((x)&(0b1111111111100001))|(((y & 0b1111) << 1U)&(0b0000000000011110)))
#define SET_CONF_ZONE_AUTH_TIME(x,y) x = (((x)&(0b1111111110011111))|(((y & 0b11) << 5U)&(0b0000000001100000)))
#define SET_CONF_ZONE_ARM_HOME(x)    x |= (1 << 7U)
#define SET_CONF_ZONE_OPEN_ALARM(x)  x |= (1 << 8U)
#define SET_CONF_ZONE_PIR_AS_TMP(x)  x |= (1 << 9U)
#define SET_CONF_ZONE_BALANCED(x)    x |= (1 << 10U)
#define SET_CONF_ZONE_IS_BATTERY(x)  x |= (1 << 11U)
#define SET_CONF_ZONE_IS_REMOTE(x)   x |= (1 << 12U)
#define SET_CONF_ZONE_IS_PRESENT(x)  x |= (1 << 14U)
#define SET_CONF_ZONE_TYPE(x)        x |= (1 << 15U)
#define CLEAR_CONF_ZONE_ENABLED(x)     x &= ~1
#define CLEAR_CONF_ZONE_ARM_HOME(x)    x &= ~(1 << 7U)
#define CLEAR_CONF_ZONE_STILL_OPEN(x)  x &= ~(1 << 8U)
#define CLEAR_CONF_ZONE_PIR_AS_TMP(x)  x &= ~(1 << 9U)
#define CLEAR_CONF_ZONE_BALANCED(x)    x &= ~(1 << 10U)
#define CLEAR_CONF_ZONE_IS_BATTERY(x)  x &= ~(1 << 11U)
#define CLEAR_CONF_ZONE_IS_REMOTE(x)   x &= ~(1 << 12U)
#define CLEAR_CONF_ZONE_IS_PRESENT(x)  x &= ~(1 << 14U)
#define CLEAR_CONF_ZONE_TYPE(x)        x &= ~(1 << 15U)

#define GET_CONF_GROUP_ENABLED(x)      ((x) & 0b1)
#define GET_CONF_GROUP_TAMPER2(x)      ((x >> 1U) & 0b1)
#define GET_CONF_GROUP_TAMPER1(x)      ((x >> 2U) & 0b1)
#define GET_CONF_GROUP_PIR2(x)         ((x >> 3U) & 0b1)
#define GET_CONF_GROUP_PIR1(x)         ((x >> 4U) & 0b1)
#define GET_CONF_GROUP_AUTO_ARM(x)     ((x >> 5U) & 0b1)
#define GET_CONF_GROUP_MQTT_PUB(x)     ((x >> 7U) & 0b1)
#define GET_CONF_GROUP_ARM_CHAIN(x)    ((x >> 8U) & 0b1111)
#define GET_CONF_GROUP_DISARM_CHAIN(x) ((x >> 12U) & 0b1111)
#define SET_CONF_GROUP_ENABLED(x)        x |= 1
#define SET_CONF_GROUP_TAMPER2(x)        x |= (1 << 1U)
#define SET_CONF_GROUP_TAMPER1(x)        x |= (1 << 2U)
#define SET_CONF_GROUP_PIR2(x)           x |= (1 << 3U)
#define SET_CONF_GROUP_PIR1(x)           x |= (1 << 4U)
#define SET_CONF_GROUP_AUTO_ARM(x)       x |= (1 << 5U)
#define SET_CONF_GROUP_MQTT_PUB(x)       x |= (1 << 7U)
#define SET_CONF_GROUP_ARM_CHAIN(x,y)    x = (((x)&(0b1111000011111111))|(((y & 0b1111) << 8U)&(0b0000111100000000)))
#define SET_CONF_GROUP_DISARM_CHAIN(x,y) x = (((x)&(0b0000111111111111))|(((y & 0b1111) << 12U)&(0b1111000000000000)))
#define CLEAR_CONF_GROUP_ENABLED(x)      x &= ~1
#define CLEAR_CONF_GROUP_TAMPER2(x)      x &= ~(1 << 1U)
#define CLEAR_CONF_GROUP_TAMPER1(x)      x &= ~(1 << 2U)
#define CLEAR_CONF_GROUP_PIR2(x)         x &= ~(1 << 3U)
#define CLEAR_CONF_GROUP_PIR1(x)         x &= ~(1 << 4U)
#define CLEAR_CONF_GROUP_AUTO_ARM(x)     x &= ~(1 << 5U)
#define CLEAR_CONF_GROUP_MQTT_PUB(x)     x &= ~(1 << 7U)

#define GET_CONF_CONTACT_ENABLED(x)     ((x) & 0b1)
#define GET_CONF_CONTACT_GROUP(x)       ((x >> 1U) & 0b1111)
#define GET_CONF_CONTACT_IS_GLOBAL(x)   ((x >> 5U) & 0b1)
#define SET_CONF_CONTACT_ENABLED(x)     x |= 1
#define SET_CONF_CONTACT_GROUP(x,y)     x = (((x)&(0b11100001))|(((y & 0b1111) << 1U)&(0b00011110)))
#define SET_CONF_CONTACT_IS_GLOBAL(x)   x |= (1 << 5U)
#define CLEAR_CONF_CONTACT_ENABLED(x)   x &= ~1
#define CLEAR_CONF_CONTACT_IS_GLOBAL(x) x &= ~(1 << 5U)

#define GET_CONF_KEY_ENABLED(x)     ((x) & 0b1)
#define SET_CONF_KEY_ENABLED(x)     x |= 1
#define CLEAR_CONF_KEY_ENABLED(x)   x &= ~1

#define GET_CONF_SYSTEM_FLAG_RTC_LOW(x)      ((x) & 0b1)
#define GET_CONF_SYSTEM_FLAG_RADIO_FREQ(x)   ((x >> 1U) & 0b1)
#define SET_CONF_SYSTEM_FLAG_RTC_LOW(x)      x |= 1
#define SET_CONF_SYSTEM_FLAG_RADIO_FREQ(x)   x |= (1 << 1U)
#define CLEAR_CONF_SYSTEM_FLAG_RTC_LOW(x)    x &= ~1
#define CLEAR_CONF_SYSTEM_FLAG_RADIO_FREQ(x) x &= ~(1 << 1U)

#define GET_CONF_TIMER_ENABLED(x)     ((x) & 0b1)
#define GET_CONF_TIMER_TYPE(x)        ((x >> 1U) & 0b1)
#define GET_CONF_TIMER_SU(x)          ((x >> 2U) & 0b1)
#define GET_CONF_TIMER_SA(x)          ((x >> 3U) & 0b1)
#define GET_CONF_TIMER_FR(x)          ((x >> 4U) & 0b1)
#define GET_CONF_TIMER_TH(x)          ((x >> 5U) & 0b1)
#define GET_CONF_TIMER_WE(x)          ((x >> 6U) & 0b1)
#define GET_CONF_TIMER_TU(x)          ((x >> 7U) & 0b1)
#define GET_CONF_TIMER_MO(x)          ((x >> 8U) & 0b1)
#define GET_CONF_TIMER_RESULT(x)      ((x >> 9U) & 0b1)
#define GET_CONF_TIMER_EVALUATED(x)   ((x >> 10U) & 0b1)
#define GET_CONF_TIMER_TRIGGERED(x)   ((x >> 11U) & 0b1)
#define GET_CONF_TIMER_PERIOD_TYPE(x) ((x >> 12U) & 0b11)
#define GET_CONF_TIMER_RUN_TYPE(x)    ((x >> 14U) & 0b11)
#define SET_CONF_TIMER_ENABLED(x)     x |= 1
#define SET_CONF_TIMER_TYPE(x)        x |= (1 << 1U)
#define SET_CONF_TIMER_SA(x)          x |= (1 << 2U)
#define SET_CONF_TIMER_FR(x)          x |= (1 << 3U)
#define SET_CONF_TIMER_TH(x)          x |= (1 << 4U)
#define SET_CONF_TIMER_WE(x)          x |= (1 << 5U)
#define SET_CONF_TIMER_TU(x)          x |= (1 << 6U)
#define SET_CONF_TIMER_MO(x)          x |= (1 << 7U)
#define SET_CONF_TIMER_SU(x)          x |= (1 << 8U)
#define SET_CONF_TIMER_RESULT(x)      x |= (1 << 9U)
#define SET_CONF_TIMER_EVALUATED(x)   x |= (1 << 10U)
#define SET_CONF_TIMER_TRIGGERED(x)   x |= (1 << 11U)
#define SET_CONF_TIMER_PERIOD_TYPE(x,y) x = (((x)&(0b1100111111111111))|(((y & 0b11) << 12U)&(0b0011000000000000)))
#define SET_CONF_TIMER_RUN_TYPE(x,y)    x = (((x)&(0b0011111111111111))|(((y & 0b11) << 14U)&(0b1100000000000000)))
#define CLEAR_CONF_TIMER_ENABLED(x)   x &= ~1
#define CLEAR_CONF_TIMER_TYPE(x)      x &= ~(1 << 1U)
#define CLEAR_CONF_TIMER_SA(x)        x &= ~(1 << 2U)
#define CLEAR_CONF_TIMER_FR(x)        x &= ~(1 << 3U)
#define CLEAR_CONF_TIMER_TH(x)        x &= ~(1 << 4U)
#define CLEAR_CONF_TIMER_WE(x)        x &= ~(1 << 5U)
#define CLEAR_CONF_TIMER_TU(x)        x &= ~(1 << 6U)
#define CLEAR_CONF_TIMER_MO(x)        x &= ~(1 << 7U)
#define CLEAR_CONF_TIMER_SU(x)        x &= ~(1 << 8U)
#define CLEAR_CONF_TIMER_RESULT(x)    x &= ~(1 << 9U)
#define CLEAR_CONF_TIMER_EVALUATED(x) x &= ~(1 << 10U)
#define CLEAR_CONF_TIMER_TRIGGERED(x) x &= ~(1 << 11U)

#define GET_CONF_TRIGGER_ENABLED(x)     ((x) & 0b1)
#define GET_CONF_TRIGGER_PASS_VALUE(x)  ((x >> 1U) & 0b1)
#define GET_CONF_TRIGGER_PASS(x)        ((x >> 2U) & 0b11)
#define GET_CONF_TRIGGER_PASSED(x)      ((x >> 4U) & 0b1)
#define GET_CONF_TRIGGER_TRIGGERED(x)   ((x >> 5U) & 0b1)
#define GET_CONF_TRIGGER_ALERT(x)       ((x >> 6U) & 0b1)
#define GET_CONF_TRIGGER_PASS_OFF(x)    ((x >> 7U) & 0b11)
#define GET_CONF_TRIGGER_RESULT(x)      ((x >> 9U) & 0b1)
#define GET_CONF_TRIGGER_OFF_PERIOD(x)  ((x >> 14U) & 0b11)

#define SET_CONF_TRIGGER_ENABLED(x)      x |= 1
#define SET_CONF_TRIGGER_PASS_VALUE(x)   x |= (1 << 1U)
#define SET_CONF_TRIGGER_PASS(x,y)       x = (((x)&(0b1111111111110011))|(((y & 0b11) << 2U)&(0b0000000000001100)))
#define SET_CONF_TRIGGER_PASSED(x)       x |= (1 << 4U)
#define SET_CONF_TRIGGER_TRIGGERED(x)    x |= (1 << 5U)
#define SET_CONF_TRIGGER_ALERT(x)        x |= (1 << 6U)
#define SET_CONF_TRIGGER_PASS_OFF(x,y)   x = (((x)&(0b1111111001111111))|(((y & 0b11) << 7U)&(0b0000000110000000)))
#define SET_CONF_TRIGGER_RESULT(x)       x |= (1 << 9U)
#define SET_CONF_TRIGGER_OFF_PERIOD(x,y) x = (((x)&(0b0011111111111111))|(((y & 0b11) << 14U)&(0b1100000000000000)))

#define CLEAR_CONF_TRIGGER_ENABLED(x)    x &= ~1
#define CLEAR_CONF_TRIGGER_PASS_VALUE(x) x &= ~(1 << 1U)
#define CLEAR_CONF_TRIGGER_PASSED(x)     x &= ~(1 << 4U)
#define CLEAR_CONF_TRIGGER_TRIGGERED(x)  x &= ~(1 << 5U)
#define CLEAR_CONF_TRIGGER_ALERT(x)      x &= ~(1 << 6U)
#define CLEAR_CONF_TRIGGER_RESULT(x)     x &= ~(1 << 9U)

#define GET_ZONE_ALARM(x)     ((x >> 1U) & 0b1)
#define GET_ZONE_ERROR(x)     ((x >> 5U) & 0b1)
#define GET_ZONE_QUEUED(x)    ((x >> 6U) & 0b1)
#define GET_ZONE_FULL_FIFO(x) ((x >> 7U) & 0b1)
#define SET_ZONE_ALARM(x)     x |= (1 << 1U)
#define SET_ZONE_ERROR(x)     x |= (1 << 5U)
#define SET_ZONE_QUEUED(x)    x |= (1 << 6U)
#define SET_ZONE_FULL_FIFO(x) x |= (1 << 7U)
#define CLEAR_ZONE_ALARM(x)     x &= ~(1 << 1U)
#define CLEAR_ZONE_ERROR(x)     x &= ~(1 << 5U)
#define CLEAR_ZONE_QUEUED(x)    x &= ~(1 << 6U)
#define CLEAR_ZONE_FULL_FIFO(x) x &= ~(1 << 7U)

#define GET_GROUP_ARMED(x)         ((x) & 0b1)
#define GET_GROUP_ALARM(x)         ((x >> 1U) & 0b1)
#define GET_GROUP_WAIT_AUTH(x)     ((x >> 2U) & 0b1)
#define GET_GROUP_ARMED_HOME(x)    ((x >> 3U) & 0b1)
#define GET_GROUP_DISABLED_FLAG(x) ((x >> 7U) & 0b1)
#define SET_GROUP_ARMED(x)         x |= 1
#define SET_GROUP_ALARM(x)         x |= (1 << 1U)
#define SET_GROUP_WAIT_AUTH(x)     x |= (1 << 2U)
#define SET_GROUP_ARMED_HOME(x)    x |= (1 << 3U)
#define SET_GROUP_DISABLED_FLAG(x) x |= (1 << 7U)
#define CLEAR_GROUP_ARMED(x)         x &= ~1
#define CLEAR_GROUP_ALARM(x)         x &= ~(1 << 1U)
#define CLEAR_GROUP_WAIT_AUTH(x)     x &= ~(1 << 2U)
#define CLEAR_GROUP_ARMED_HOME(x)    x &= ~(1 << 3U)
#define CLEAR_GROUP_DISABLED_FLAG(x) x &= ~(1 << 7U)

#define GET_NODE_ENABLED(x)  ((x) & 0b1)
#define GET_NODE_GROUP(x)    ((x >> 1U) & 0b1111)
#define GET_NODE_BATT_LOW(x) ((x >> 5U) & 0b1)
#define GET_NODE_MQTT_PUB(x) ((x >> 7U) & 0b1)
#define SET_NODE_ENABLED(x)  x |= 1
#define SET_NODE_GROUP(x,y)  x = (((x)&(0b1111111111100001))|(((y & 0b1111) << 1U)&(0b0000000000011110)))
#define SET_NODE_BATT_LOW(x) x |= (1 << 5U)
#define SET_NODE_MQTT_PUB(x) x |= (1 << 7U)
#define CLEAR_NODE_ENABLED(x)  x &= ~1
#define CLEAR_NODE_BATT_LOW(x) x &= ~(1 << 5U)
#define CLEAR_NODE_MQTT_PUB(x) x &= ~(1 << 7U)

// Helper macros. Do not use in functions parameter!
#define ARRAY_SIZE(x) sizeof(x)/sizeof(x[0])

// Global vars
char tmpLog[LOGGER_MSG_LENGTH]; // Temporary logger string
// RTC related
static RTCDateTime timespec;
time_t startTime;  // OHS start timestamp variable
// RTC last Vbat value
float rtcVbat;
// Ethernet
uint8_t macAddr[6];

// Arm type enum
typedef enum {
  armAway = 0,
  armHome = 1
} armType_t;

// time_t conversion
union time_tag {
  char   ch[4];
  time_t val;
} timeConv;

// float conversion
union float_tag {
  uint8_t byte[4];
  float   val;
} floatConv;

// uint32_t conversion
union uint32_tag {
  uint8_t  byte[4];
  uint32_t val;
} uint32Conv;

// Zones alarm events
#define ALARMEVENT_FIFO_SIZE 10
typedef struct {
  uint16_t zone;
  char     type;
} alarmEvent_t;

// Logger events
#define LOGGER_FIFO_SIZE 20
typedef struct {
  time_t timestamp;
  char   text[LOGGER_MSG_LENGTH];
} loggerEvent_t;

// Alert events
#define ALERT_FIFO_SIZE 5
typedef struct {
  char    text[LOGGER_MSG_LENGTH];
  uint8_t flag;
} alertEvent_t;

// Registration events
#define REG_FIFO_SIZE 6
#define REG_PACKET_HEADER_SIZE 5
#define REG_PACKET_SIZE 21
typedef struct {
  char     type;
  uint8_t  address;
  char     function;
  uint8_t  number;
  uint16_t setting;
  char     name[NAME_LENGTH];
  uint16_t dummyAlign;
} registrationEvent_t;

// Sensor events
#define SENSOR_FIFO_SIZE 10
#define SENSOR_PACKET_SIZE 7
typedef struct {
  char    type;     // = 'S';
  uint8_t address;  // = 0;
  char    function; // = ' ';
  uint8_t number;   // = 0;
  float   value;    // = 0.0;
} sensorEvent_t;

// Trigger events
#define TRIGGER_FIFO_SIZE 20 // To accommodate various sources like zones, groups, sensors
typedef struct {
  char    type;     // = 'S';
  uint8_t address;  // = 0;
  char    function; // = ' ';
  uint8_t number;   // = 0;
  float   value;    // = 0.0;
} triggerEvent_t;

// TCL callback
typedef void (*script_cb_t) (char *result);
void script_cb(script_cb_t ptrFunc(char *result), char *result) {
  ptrFunc(result);
}
// Script events
#define SCRIPT_FIFO_SIZE 5
typedef struct {
  char   *cmdP;
  uint8_t flags;
  void   *callback;
  void  **result;
} scriptEvent_t;

// Script ll
struct scriptLL_t{
  char              *name;
  char              *cmd;
  struct scriptLL_t *next;
};
struct scriptLL_t *scriptLL = NULL; // Holds LL
struct scriptLL_t *scriptp = NULL;  // Used as temp pointer

// Alerts
typedef struct {
  char    name[6];
} alertType_t;

// Logger keeps info about this as bit flags of uint8_t, maximum number of alert types is 8 bits(uint8_t).
const alertType_t alertType[] = {
  // 1234567890
  { "SMS" },
  { "Page" },
  { "Email" }
};
// Check alertType size
typedef char check_alertType[ARRAY_SIZE(alertType) <= 8 ? 1 : -1];

// Logger message text to match alert, maximum number of alerts is number of bits in uint32_t
const char alertDef[][3] = {
  "SS", "SX", "SB", "SA", "SR",
  "GS", "GD", "GA",
  "ZP", "ZT", "ZO",
  "AA", "AH", "AD", "AU", "AF"
};
// Check alertDef size
typedef char check_alertDef[ARRAY_SIZE(alertDef) <= 32 ? 1 : -1];

/*
 * Mailboxes
 */
static msg_t        alarmEvent_mb_buffer[ALARMEVENT_FIFO_SIZE];
static MAILBOX_DECL(alarmEvent_mb, alarmEvent_mb_buffer, ALARMEVENT_FIFO_SIZE);

static msg_t        logger_mb_buffer[LOGGER_FIFO_SIZE];
static MAILBOX_DECL(logger_mb, logger_mb_buffer, LOGGER_FIFO_SIZE);

static msg_t        registration_mb_buffer[REG_FIFO_SIZE];
static MAILBOX_DECL(registration_mb, registration_mb_buffer, REG_FIFO_SIZE);

static msg_t        sensor_mb_buffer[SENSOR_FIFO_SIZE];
static MAILBOX_DECL(sensor_mb, sensor_mb_buffer, SENSOR_FIFO_SIZE);

static msg_t        alert_mb_buffer[ALERT_FIFO_SIZE];
static MAILBOX_DECL(alert_mb, alert_mb_buffer, ALERT_FIFO_SIZE);

static msg_t        script_mb_buffer[SCRIPT_FIFO_SIZE];
static MAILBOX_DECL(script_mb, script_mb_buffer, SCRIPT_FIFO_SIZE);

static msg_t        trigger_mb_buffer[TRIGGER_FIFO_SIZE];
static MAILBOX_DECL(trigger_mb, trigger_mb_buffer, TRIGGER_FIFO_SIZE);
/*
 * Pools
 */
static alarmEvent_t alarmEvent_pool_queue[ALARMEVENT_FIFO_SIZE];
static MEMORYPOOL_DECL(alarmEvent_pool, sizeof(alarmEvent_t), PORT_NATURAL_ALIGN, NULL);

static loggerEvent_t logger_pool_queue[LOGGER_FIFO_SIZE];
static MEMORYPOOL_DECL(logger_pool, sizeof(loggerEvent_t), PORT_NATURAL_ALIGN, NULL);

static registrationEvent_t registration_pool_queue[REG_FIFO_SIZE];
static MEMORYPOOL_DECL(registration_pool, sizeof(registrationEvent_t), PORT_NATURAL_ALIGN, NULL);

static sensorEvent_t sensor_pool_queue[SENSOR_FIFO_SIZE];
static MEMORYPOOL_DECL(sensor_pool, sizeof(sensorEvent_t), PORT_NATURAL_ALIGN, NULL);

static alertEvent_t alert_pool_queue[ALERT_FIFO_SIZE];
static MEMORYPOOL_DECL(alert_pool, sizeof(alertEvent_t), PORT_NATURAL_ALIGN, NULL);

static scriptEvent_t script_pool_queue[SCRIPT_FIFO_SIZE];
static MEMORYPOOL_DECL(script_pool, sizeof(scriptEvent_t), PORT_NATURAL_ALIGN, NULL);

static sensorEvent_t trigger_pool_queue[TRIGGER_FIFO_SIZE];
static MEMORYPOOL_DECL(trigger_pool, sizeof(triggerEvent_t), PORT_NATURAL_ALIGN, NULL);

// Triggers
typedef struct {
//                             |- Off time: 0 Seconds, 01 Minutes,
//                             ||-          10 Hours, 11 Days
//                             |||-
//                             ||||-
//                             |||||-
//                             ||||||-
//                             |||||||- Script evaluated
//                             ||||||||- Pass off 00 no , 01 yes , 10 timer
//                             ||||||||         |-
//                             ||||||||         ||- Logging enabled
//                             ||||||||         |||- Is triggered
//                             ||||||||         ||||- Passed
//                             ||||||||         |||||- Pass once / pass always
//                             ||||||||         ||||||- Pass
//                             ||||||||         |||||||- Pass value or constant
//                             ||||||||         ||||||||- Enabled
//                             54321098         76543210
  uint16_t setting;       //0b000000000 << 8 | B00000000;
  uint8_t  address;       //
  char     type;          //
  char     function;      //
  uint8_t  number;        //
  uint8_t  condition;     //
  float    value;         //
  float    constantOn;    //
  float    constantOff;   //
  uint8_t  toAddress;     //
  char     toFunction;    //
  uint8_t  toNumber;      //
  uint8_t  offTime;       //
  uint32_t nextOff;       //
  char     name[NAME_LENGTH];
  char     evalScript[NAME_LENGTH];
  float    hysteresis;    //
} trigger_t;

const char triggerCondition[][4] = {
  "any", "=" , "<>", "<", ">"
};

const char triggerPassOffType[][6] = {
  "no", "yes", "timer"
};

const char triggerPassType[][5] = {
  "no", "yes", "once"
};

const char groupState[][11] = {
// 12345678901234567890
  "disarmed",
  "armed away",
  "armed home"
};

const char zoneState[][7] = {
// 1234567890
  "OK",
  "alarm",
  "tamper"
};

// Timers
typedef struct {
//                         |- Run type: 0 Seconds, 01 Minutes,
//                         ||-          10 Hours, 11 Days
//                         |||- Period type: 0 Seconds, 01 Minutes,
//                         ||||-             10 Hours, 11 Days
//                         |||||- Triggered
//                         ||||||- Script evaluated
//                         |||||||- Script result
//                         ||||||||- Monday
//                         ||||||||         |- Tuesday
//                         ||||||||         ||- Wednesday
//                         ||||||||         |||- Thursday
//                         ||||||||         ||||- Friday
//                         ||||||||         |||||- Saturday
//                         ||||||||         ||||||- Sunday
//                         ||||||||         |||||||- Calendar / Period
//                         ||||||||         ||||||||-  Enabled
//                         54321098         76543210
  uint16_t setting;     //B00000000 << 8 | B00000000;
  uint8_t  periodTime;  // period interval
  uint16_t startTime;   // for calendar timer in minutes 0 - 1440
  uint8_t  runTime;     // runtime interval
  float    constantOn;  // value to pass
  float    constantOff; // value to pass
  uint8_t  toAddress;
  char     toFunction;
  uint8_t  toNumber;
  uint32_t nextOn;
  uint32_t nextOff;
  char     name[NAME_LENGTH];
  char     evalScript[NAME_LENGTH];
} calendar_t; // timer_t used by ChibiOS

// Configuration struct
typedef struct {
  uint8_t  versionMajor;
  uint8_t  versionMinor;

  uint16_t logOffset; // FRAM position
  uint8_t  armDelay;
  uint8_t  autoArm; // minutes
  uint8_t  openAlarm; // minutes
  char     dateTimeFormat[NAME_LENGTH];

  // TODO OHS Dynamic zones should not be stored in conf? Or just clear IS PRESENT flag on start.
  uint16_t zone[ALARM_ZONES];
  char     zoneName[ALARM_ZONES][NAME_LENGTH];
  uint8_t  zoneAddress[ALARM_ZONES-HW_ZONES]; // Only for remote zone address

  uint16_t group[ALARM_GROUPS];
  char     groupName[ALARM_GROUPS][NAME_LENGTH];

  uint8_t  contact[CONTACTS_SIZE];
  char     contactName[CONTACTS_SIZE][NAME_LENGTH];
  char     contactPhone[CONTACTS_SIZE][PHONE_LENGTH];
  char     contactEmail[CONTACTS_SIZE][EMAIL_LENGTH];

  uint8_t  keySetting[KEYS_SIZE];
  uint32_t keyValue[KEYS_SIZE];
  uint8_t  keyContact[KEYS_SIZE];

  uint32_t alert[ARRAY_SIZE(alertType)];

  char     SNTPAddress[URL_LENGTH];
  uint8_t  timeStdWeekNum;//First, Second, Third, Fourth, or Last week of the month
  uint8_t  timeStdDow;    //day of week, 1=Sun, 2=Mon, ... 7=Sat
  uint8_t  timeStdMonth;  //1=Jan, 2=Feb, ... 12=Dec
  uint8_t  timeStdHour;   //0-23
  int16_t  timeStdOffset; //offset from UTC in minutes
  uint8_t  timeDstWeekNum;//First, Second, Third, Fourth, or Last week of the month
  uint8_t  timeDstDow;    //day of week, 1=Sun, 2=Mon, ... 7=Sat
  uint8_t  timeDstMonth;  //1=Jan, 2=Feb, ... 12=Dec
  uint8_t  timeDstHour;   //0-23
  int16_t  timeDstOffset; //offset from UTC in minutes

  char     SMTPAddress[URL_LENGTH];
  uint16_t SMTPPort;
  char     SMTPUser[EMAIL_LENGTH];
  char     SMTPPassword[NAME_LENGTH];

  char     user[NAME_LENGTH];
  char     password[NAME_LENGTH];

  uint8_t  tclSetting;    // TCL flags
  uint16_t tclIteration;  // Number of allowed loops

  uint8_t  systemFlags;

  calendar_t timer[TIMER_SIZE];

  trigger_t  trigger[TRIGGER_SIZE];

  char     radioKey[RADIO_KEY_SIZE];

} config_t;
config_t conf __attribute__((section(".ram4")));
// Check conf size fits to backup SRAM
typedef char check_conf[sizeof(conf) <= BACKUP_SRAM_SIZE ? 1 : -1];

// Group runtime variables
typedef struct {
  uint8_t setting;
  uint8_t armDelay;
} group_t;
group_t group[ALARM_GROUPS] __attribute__((section(".ram4")));
// Check conf size fits to backup SRAM
typedef char check_group[sizeof(group) <= BACKUP_RTC_SIZE ? 1 : -1];

// Zone runtime variables
typedef struct {
  time_t  lastPIR;
  time_t  lastOK;
  char    lastEvent;
  uint8_t setting;
} zone_t;
zone_t zone[ALARM_ZONES] __attribute__((section(".ram4")));

// Flags runtime variables
/*
typedef struct {
  uint16_t flags;
} flags_t;
flags_t flags;
*/

// Dynamic nodes
typedef struct {
  uint8_t address; //= 0;
  char    type;    //= 'K/S/I';
  char    function;//= ' ';
  uint8_t number;  //= 0;
   //                    |- MQTT publish
   //                    ||- Free
   //                    |||- Battery low flag, for battery type node
   //                    |||||||- Group number
   //                    |||||||- 0 .. 15
   //                    |||||||-
   //                    |||||||-
   //                    ||||||||-  Enabled
   //                    76543210
  uint16_t setting;// = B00011110;  // 2 bytes to store also zone setting
  float    value;  // = 0;
  time_t lastOK;   // = 0;
  uint8_t  queue;  //   = DUMMY_NO_VALUE 255; // No queue
  char name[NAME_LENGTH]; // = "";
} node_t;
node_t node[NODE_SIZE] __attribute__((section(".ram4")));
/*
 * Initialize node runtime struct
 */
void initRuntimeNodes(void){
  for(uint8_t i = 0; i < NODE_SIZE; i++) {
    node[i].address  = 0;
    node[i].function = '\0';
    node[i].lastOK  = 0;
    node[i].name[0]  = '\0';
    node[i].number   = 0;
    node[i].queue    = DUMMY_NO_VALUE;
    node[i].setting  = 0b00011110;
    node[i].type     = '\0';
    node[i].value    = 0;
  }
}
/*
 * Initialize group runtime struct
 */
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
/*
 * Initialize zone runtime struct
 */
void initRuntimeZones(void){
  for(uint8_t i = 0; i < ALARM_ZONES; i++) {
    zone[i].lastPIR   = startTime;
    zone[i].lastOK    = startTime;
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
    // Force disconnected to all remote zones
    if (i >= HW_ZONES) {
      CLEAR_CONF_ZONE_IS_PRESENT(conf.zone[i]);
    }
  }
}
/*
 * Write to backup SRAM
 */
int16_t writeToBkpSRAM(uint8_t *data, uint16_t size, uint16_t offset){
  if (size + offset >= BACKUP_SRAM_SIZE) osalSysHalt("SRAM out of region"); // Data out of BACKUP_SRAM_SIZE region
  uint16_t i = 0;
  uint8_t *baseAddress = (uint8_t *) BKPSRAM_BASE;
  for(i = 0; i < size; i++) {
    *(baseAddress + offset + i) = *(data + i);
  }
  return i;
}
/*
 * Read from backup SRAM
 */
int16_t readFromBkpSRAM(uint8_t *data, uint16_t size, uint16_t offset){
  if (size + offset >= BACKUP_SRAM_SIZE) osalSysHalt("SRAM out of region");; // Data out of BACKUP_SRAM_SIZE region
  uint16_t i = 0;
  uint8_t *baseAddress = (uint8_t *) BKPSRAM_BASE;
  for(i = 0; i < size; i++) {
    *(data + i) = *(baseAddress + offset + i);
  }
  return i;
}
/*
 * Write to backup RTC
 */
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
/*
 * Read from backup RTC
 */
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
/*
 * Set conf struct default values
 */
void setConfDefault(void){
  conf.versionMajor   = OHS_MAJOR;
  conf.versionMinor   = OHS_MINOR;
  conf.logOffset      = 0;
  conf.armDelay       = 80;
  conf.autoArm        = 20;
  conf.openAlarm      = 20;
  strcpy(conf.dateTimeFormat, "%T %a %d.%m.%Y");

  for(uint8_t i = 0; i < ALARM_ZONES; i++) {
    // Zones setup
    //                    |- HW type Digital 0/ Analog 1
    //                    ||- Present - connected
    //                    |||- ~ Free ~
    //                    ||||- Remote zone
    //                    |||||- Battery powered zone, they don't send OK, only PIR or Tamper.
    //                    ||||||- Logical type balanced 1/ unbalanced 0. Only Analog zones can be balanced.
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
         conf.zone[i] = 0b11000100 << 8 | 0b00011110; // Analog sensor
        break;
      case  10      :
         conf.zone[i] = 0b01000010 << 8 | 0b00011110; // Tamper
        break;
      default:
         conf.zone[i] = 0b00000000 << 8 | 0b00011110; // Other zones
         conf.zoneAddress[i-HW_ZONES] = 0;
        break;
    }
    //strcpy(conf.zoneName[i], NOT_SET);
    memset(&conf.zoneName[i][0], 0x00, NAME_LENGTH);
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
    conf.group[i] = 0b11111111 << 8 | 0b00000000;
    //strcpy(conf.groupName[i], NOT_SET);
    memset(&conf.groupName[i][0], 0x00, NAME_LENGTH);
  }

  for(uint8_t i = 0; i < CONTACTS_SIZE; i++) {
    // group 16 and disabled
    conf.contact[i] = 0b00011110;
    memset(&conf.contactName[i][0], 0x00, NAME_LENGTH);
    memset(&conf.contactPhone[i][0], 0x00, PHONE_LENGTH);
    memset(&conf.contactEmail[i][0], 0x00, EMAIL_LENGTH);
  }

  for(uint8_t i = 0; i < KEYS_SIZE; i++) {
    // disabled
    conf.keySetting[i] = 0b00000000;
    conf.keyValue[i]   = 0xFFFFFFFF;  // Set key value to FF
    conf.keyContact[i] = DUMMY_NO_VALUE;
  }

  for(uint8_t i = 0; i < ARRAY_SIZE(alertType); i++) {
    conf.alert[i] = 0;
  }

  strcpy(conf.SNTPAddress, "time.google.com");
  conf.timeStdWeekNum = 0;  //First, Second, Third, Fourth, or Last week of the month
  conf.timeStdDow = 0;      //day of week, 0=Sun, 1=Mon, ... 6=Sat
  conf.timeStdMonth = 10;   //1=Jan, 2=Feb, ... 12=Dec
  conf.timeStdHour = 3;     //0-23
  conf.timeStdOffset = 60;  //offset from UTC in minutes
  conf.timeDstWeekNum = 0;  //First, Second, Third, Fourth, or Last week of the month
  conf.timeDstDow = 0;      //day of week, 0=Sun, 1=Mon, ... 6=Sat
  conf.timeDstMonth = 3;    //1=Jan, 2=Feb, ... 12=Dec
  conf.timeDstHour = 2;     //0-23
  conf.timeDstOffset = 120; //offset from UTC in minutes

  strcpy(conf.SMTPAddress, "mail.smtp2go.com");
  conf.SMTPPort = 2525;
  strcpy(conf.SMTPUser, NOT_SET);
  strcpy(conf.SMTPPassword, NOT_SET);

  strcpy(conf.user, "admin");
  strcpy(conf.password, "pass");

  //                  |||||-
  //                  ||||||-
  //                  |||||||- Show warnings
  //                  ||||||||- Restart environment each time.
  conf.tclSetting = 0b00000001;
  conf.tclIteration = 5000;

  //                   |||||-
  //                   ||||||-
  //                   |||||||- Radio frequency
  //                   ||||||||- RTC low flag, let's check on power On.
  conf.systemFlags = 0b00000000;

  for(uint8_t i = 0; i < TIMER_SIZE; i++) {
    conf.timer[i].setting = 0;
    conf.timer[i].periodTime = 1;
    conf.timer[i].startTime = 0;
    conf.timer[i].runTime = 1;
    conf.timer[i].constantOn = 1;
    conf.timer[i].constantOff = 0;
    conf.timer[i].toAddress = 0;
    conf.timer[i].toFunction = ' ';
    conf.timer[i].toNumber = 0;
    conf.timer[i].nextOn = 0;
    conf.timer[i].nextOff = 0;
    strcpy(conf.timer[i].name, "");
    strcpy(conf.timer[i].evalScript, "");
  }

  for(uint8_t i = 0; i < TRIGGER_SIZE; i++) {
    conf.trigger[i].setting = 0;
    conf.trigger[i].address = 0;
    conf.trigger[i].type = ' ';
    conf.trigger[i].function = ' ';
    conf.trigger[i].number = 0;
    conf.trigger[i].condition = 0;
    conf.trigger[i].value = 0;
    conf.trigger[i].constantOn = 1;
    conf.trigger[i].constantOff = 0;
    conf.trigger[i].toAddress  = 0;
    conf.trigger[i].toFunction = ' ';
    conf.trigger[i].toNumber   = 0;
    conf.trigger[i].offTime = 1;
    conf.trigger[i].nextOff = 0;
    strcpy(conf.trigger[i].name, "");
    strcpy(conf.trigger[i].evalScript, "");
    conf.trigger[i].hysteresis = 0;
  }

  memset(&conf.radioKey[0], 0, RADIO_KEY_SIZE);
}
/*
 * Load scripts form uBS to UMM heap.
 */
void initScripts(struct scriptLL_t **pointer) {
  char     blockName[NAME_LENGTH] = {0};
  uint32_t blockAddress = 0;
  uint16_t cmdSize;
  struct scriptLL_t* var;

  while (uBSSeekAll(&blockAddress, &blockName, NAME_LENGTH)) {
    var = umm_malloc(sizeof(struct scriptLL_t));
    if (var != NULL) {
      // name
      var->name = umm_malloc(NAME_LENGTH);
      if (var->name == NULL) {
        umm_free(var);
        chprintf(console, "CMD heap full (name)!\r\n");
      } else {
        strncpy(var->name, &blockName[0], NAME_LENGTH);
        // cmd
        cmdSize = TCL_SCRIPT_LENGTH;
        if (uBSRead(&blockName[0], NAME_LENGTH, &tclCmd[0], &cmdSize) != UBS_RSLT_OK) {
          umm_free(var);
          chprintf(console, "uBS storage needs format!\r\n");
          break;
        }
        var->cmd = umm_malloc(cmdSize + 1);
        if (var->cmd == NULL) {
          umm_free(var->name);
          umm_free(var);
          chprintf(console, "CMD heap full (cmd)!\r\n");
        } else {
          strncpy(var->cmd, &tclCmd[0], cmdSize);
          // Link it
          var->next = *pointer;
          *pointer = var;
        }
      }
    } else {
      chprintf(console, "CMD heap full!\r\n");
    }
  }
  // Clear tclCmd on end
  memset(&tclCmd[0], '\0', TCL_SCRIPT_LENGTH);
}

#endif /* OHS_CONF_H_ */
