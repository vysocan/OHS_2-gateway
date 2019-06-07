/*
    ChibiOS - Copyright (C) 2006..2018 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "ch.h"
#include "hal.h"

#include "rt_test_root.h"
#include "oslib_test_root.h"

// Added from ChibiOS
#include "shell.h"
#include "chprintf.h"
#include "usbcfg.h"

// Conf related
#include "ohs_conf.h"

// LWIP
#include "lwipthread.h"
#include "lwip/apps/httpd.h"
#include "lwip/apps/sntp.h"


// FRAM on SPI related
#define CMD_25AA_WRSR     0x01  // Write status register
#define CMD_25AA_WRITE    0x02
#define CMD_25AA_READ     0x03
#define CMD_25AA_WRDI     0x04  // Write Disable
#define CMD_25AA_RDSR     0x05  // Read Status Register
#define CMD_25AA_WREN     0x06  // Write Enable
#define CMD_25AA_RDID     0x9F  // Read FRAM ID
#define STATUS_25AA_WEL   0b00000010  // write enable latch (1 == write enable)
//#define STATUS_25AA_WIP   0b00000001  // write in progress
//#define FRAM_SIZE         32768
#define FRAM_MSG_SIZE     16
volatile uint16_t FRAMWritePos = 0;
volatile uint16_t FRAMReadPos  = 0;
char logText[50]; // To decode log text


// ADC related
#define ADC_GRP1_NUM_CHANNELS   10
#define ADC_GRP1_BUF_DEPTH      1
static adcsample_t adcSamples[ADC_GRP1_NUM_CHANNELS * ADC_GRP1_BUF_DEPTH];

// RTC related
static RTCDateTime timespec;
//static time_t unix_time;

// time_t conversion
union time_tag {
  char   ch[4];
  time_t val;
} timeConv;

// Zones alarm events
#define ALARMEVENT_FIFO_SIZE 10
typedef struct {
  uint16_t zone;
  char     type;
} alarmEvent_t;

// Logger events
#define LOGGER_FIFO_SIZE 20
#define LOGGER_MSG_LENGTH 11
typedef struct {
  time_t   timestamp;
  char     text[LOGGER_MSG_LENGTH];
} logger_t;
char tmpLog[LOGGER_MSG_LENGTH]; // Temporary logger string


/*
 * OHS Includes
 */
// OHS specific configuration for ADC
#include "ohs_adc.h"

// Registration events
#define REG_FIFO_SIZE 6
#define REG_PACKET_HEADER_SIZE 5
#define REG_PACKET_SIZE 21
typedef struct {
  uint8_t  address;
  char     node;
  char     type;
  uint8_t  number;
  uint16_t setting;
  char     name[NAME_LENGTH];
  uint16_t dummyAlign;
} registration_t;


// Dynamic nodes
#define NODES 100
typedef struct {
  uint8_t address; //= 0;
  char    function;//= ' ';
  char    type;    //= ' ';
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
  uint32_t last_OK;// = 0;
  uint8_t  queue;  //   = 255; // No queue
  char name[NAME_LENGTH]; // = "";
} node_t;
node_t node[NODES];

// Define debug console
BaseSequentialStream* console = (BaseSequentialStream*)&SD3;

/*
 * Mailboxes
 */
static msg_t        alarmEvent_mb_buffer[ALARMEVENT_FIFO_SIZE];
static MAILBOX_DECL(alarmEvent_mb, alarmEvent_mb_buffer, ALARMEVENT_FIFO_SIZE);

static msg_t        logger_mb_buffer[LOGGER_FIFO_SIZE];
static MAILBOX_DECL(logger_mb, logger_mb_buffer, LOGGER_FIFO_SIZE);

static msg_t        registration_mb_buffer[REG_FIFO_SIZE];
static MAILBOX_DECL(registration_mb, registration_mb_buffer, REG_FIFO_SIZE);
/*
 * Pools
 */
static alarmEvent_t    alarmEvent_pool_queue[ALARMEVENT_FIFO_SIZE];
static MEMORYPOOL_DECL(alarmEvent_pool, sizeof(alarmEvent_t), PORT_NATURAL_ALIGN, NULL);

static logger_t        logger_pool_queue[LOGGER_FIFO_SIZE];
static MEMORYPOOL_DECL(logger_pool, sizeof(logger_t), PORT_NATURAL_ALIGN, NULL);

static registration_t  registration_pool_queue[REG_FIFO_SIZE];
static MEMORYPOOL_DECL(registration_pool, sizeof(registration_t), PORT_NATURAL_ALIGN, NULL);

//static node_t          node_pool_queue[NODES];
//static MEMORYPOOL_DECL(node_pool, sizeof(node_t), PORT_NATURAL_ALIGN, NULL);


/*
 * Functions
 */
// Get time as Unix seconds
time_t GetTimeUnixSec(void) {
  struct tm timestamp;

  rtcGetTime(&RTCD1, &timespec);
  rtcConvertDateTimeToStructTm(&timespec, &timestamp, NULL);
  return mktime(&timestamp);
}

//
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

/*
 * Zone thread
 */
static THD_WORKING_AREA(waZoneThread, 128);
static THD_FUNCTION(ZoneThread, arg) {
  chRegSetThreadName(arg);

  msg_t    msg;
  uint16_t val;
  uint8_t  _group = 255;

  chThdSleepMilliseconds(250);
  pushToLogText("TestLogger");
  while (true) {
    chThdSleepMilliseconds(250); // time is used also for arm delay and others ...

    // Manage arm delay for each group
    for (uint8_t i=0; i < ALARM_GROUPS ; i++){
      if (group[i].armDelay > 0) {
        group[i].armDelay--;
        if (group[i].armDelay == 0) {
          //++ _resp = sendCmdToGrp(i, 15);  // send arm message to all nodes
          //++ publishGroup(i, 'A');
          tmpLog[0] = 'G'; tmpLog[1] = 'S'; tmpLog[2] = i;  pushToLog(tmpLog, 3);
        }
      }
    }

    adcConvert(&ADCD1, &adcgrpcfg1, adcSamples, ADC_GRP1_BUF_DEPTH); // Do ADC

    for(uint8_t i = 0; i < ADC_GRP1_NUM_CHANNELS; i++) {
      chprintf(console, " > %d", adcSamples[i]);
    }
    chprintf(console, "\r\n");


    for(uint8_t i = 0; i < ALARM_ZONES; i++) {
      // Zone enabled ?
      if (conf.zone[i] & 0b1){

        // Remote zone
        if ((conf.zone[i] >> 12) & 0b1) {
          // Switch battery zone back to OK after 2 seconds, as battery nodes will not send OK
          if ((((conf.zone[i] >> 11) & 0b1) == 1) &&
                (zone[i].lastEvent != 'O') && (zone[i].lastPIR + 2 < GetTimeUnixSec())) {
            zone[i].lastEvent = 'O';
            zone[i].lastOK = GetTimeUnixSec();    // update current timestamp
          }
          switch(zone[i].lastEvent) {
            case 'O': val = ALARM_OK; break;
            case 'P': val = ALARM_PIR; break;
            default:  val = 0; break;
          }
        // Local HW
        } else {
          if ((conf.zone[i] >> 15) & 0b1){ // Digital 0/ Analog 1
            val = adcSamples[i];
          } else {
            switch(i) {
              case 10: if (palReadPad(GPIOB, GPIOB_TAMPER)) val = ALARM_PIR;
                       else                                 val = ALARM_OK;
              break;
              default: break;
            }
          }
        }

        //    alarm as tamper              is PIR                                               make it tamper
        if (((conf.zone[i] >> 9) & 0b1) && (val >= ALARM_PIR_LOW && val <= ALARM_PIR_HI)) val = ALARM_TAMPER;
        // get current zone group
        _group = (conf.zone[i] >> 1) & 0b1111;

        // Decide zone state
        switch((uint16_t)(val)){
          case ALARM_OK_LOW ... ALARM_OK_HI:
            // Battery node, they will not send OK only PIR and Tamper
            if (((conf.zone[i] >> 11) & 0b1) != 1) {
              zone[i].lastEvent = 'O';
              zone[i].lastOK = GetTimeUnixSec();    // update current timestamp
            }
            break;
          case ALARM_PIR_LOW ... ALARM_PIR_HI:
            //     zone not have alarm              group delay is 0
            if (!((zone[i].setting >> 1) & 0b1) && (group[_group].armDelay == 0)){
              // if group not enabled log error to log.
              if (!(conf.group[_group] & 0b1)) {
                if (!((group[_group].setting >> 7) & 0b1)) {
                  group[_group].setting |= (1 << 7); // Set logged disabled bit On
                  tmpLog[0] = 'G'; tmpLog[1] = 'F'; tmpLog[2] = _group;  pushToLog(tmpLog, 3);
                }
              } else {
                //   group armed
                if ((group[_group].setting & 0b1) && (zone[i].lastEvent == 'P')) {
                  alarmEvent_t *outMsg = chPoolAlloc(&alarmEvent_pool);
                  if (outMsg != MSG_OK) {
                    if (!((zone[i].setting >> 7) & 0b1)) {
                      tmpLog[0] = 'Z'; tmpLog[1] = 'P'; tmpLog[2] = i;  pushToLog(tmpLog, 3);
                      pushToLogText("FA"); // Alarm queue is full
                    }
                    zone[i].setting |= (1 << 7); // Set On Alarm queue is full
                    continue; // Continue if no free space.
                  }
                  tmpLog[0] = 'Z'; tmpLog[1] = 'P'; tmpLog[2] = i;  pushToLog(tmpLog, 3);
                  zone[i].setting |=  (1 << 1); // Set alarm bit On
                  zone[i].setting &= ~(1 << 7); // Set Off Alarm queue is full
                  outMsg->zone = i; outMsg->type = zone[i].lastEvent;
                  msg = chMBPostTimeout(&alarmEvent_mb, (msg_t)outMsg, TIME_IMMEDIATE);
                  if (msg != MSG_OK) pushToLogText("FA"); // Alarm queue is full
                }
              }
            }
            // Battery node, they will not send OK only PIR and Tamper
            if (((conf.zone[i] >> 11) & 0b1) != 1) {
              zone[i].lastEvent = 'P';
              zone[i].lastPIR = GetTimeUnixSec();    // update current timestamp
            }
            break;
          default: // Line is cut or short or tamper, no difference to alarm event
            //  zone not have alarm
            if (!((zone[i].setting >> 1) & 0b1)){
              // if group not enabled log error to log.
              if (!(conf.group[_group] & 0b1)) {
                if (!((group[_group].setting >> 7) & 0b1)) {
                  group[_group].setting |= (1 << 7); // Set logged disabled bit On
                  tmpLog[0] = 'G'; tmpLog[1] = 'F'; tmpLog[2] = _group;  pushToLog(tmpLog, 3);
                }
              } else {
                if (zone[i].lastEvent == 'T') {
                  alarmEvent_t *outMsg = chPoolAlloc(&alarmEvent_pool);
                  if (outMsg != MSG_OK) {
                    if (!((zone[i].setting >> 7) & 0b1)) {
                      tmpLog[0] = 'Z'; tmpLog[1] = 'T'; tmpLog[2] = i;  pushToLog(tmpLog, 3);
                      pushToLogText("FA"); // Alarm queue is full
                    }
                    zone[i].setting |= (1 << 7); // Set On Alarm queue is full
                    continue; // Continue if no free space.
                  }
                  tmpLog[0] = 'Z'; tmpLog[1] = 'T'; tmpLog[2] = i;  pushToLog(tmpLog, 3);
                  zone[i].setting |=  (1 << 1); // Set alarm bit On
                  zone[i].setting &= ~(1 << 7); // Set Off Alarm queue is full
                  outMsg->zone = i; outMsg->type = zone[i].lastEvent;
                  msg = chMBPostTimeout(&alarmEvent_mb, (msg_t)outMsg, TIME_IMMEDIATE);
                  if (msg != MSG_OK) pushToLogText("FA"); // Alarm queue is full
                }
              }
            }
            // Battery node, they will not send OK only PIR and Tamper
            if (((conf.zone[i] >> 11) & 0b1) != 1) {
              zone[i].lastEvent = 'T';
              zone[i].lastPIR = GetTimeUnixSec();    // update current timestamp
            }
            break;
        }

        /* Example ***
        alarmEvent_t *outMsg = chPoolAlloc(&alarmEvent_pool);
        if (outMsg != NULL) {
          outMsg->zone = val;
          outMsg->type = 'P';
          //chprintf(console, "P OK %d %d\r\n", outMsg, outMsg->zone);
          msg = chMBPostTimeout(&alarmEvent_mb, (msg_t)outMsg, TIME_IMMEDIATE);
          if (msg == MSG_OK) {
            //chprintf(console, "MB put %d\r\n", temp);
          } else {
            //chprintf(console, "MB full %d\r\n", temp);
          }
        } else {
          //chprintf(console, "P full %d \r\n", outMsg);
        }
        */
      } // zone enabled
    } // for each alarm zone
  } // while true
}

/*
 * Alarm event threads
 */
static THD_WORKING_AREA(waAEThread1, 256);
static THD_WORKING_AREA(waAEThread2, 256);
static THD_WORKING_AREA(waAEThread3, 256);
static THD_FUNCTION(AEThread, arg) {
  chRegSetThreadName(arg);
  msg_t msg;
  alarmEvent_t *inMsg;
  uint8_t _group, _wait, _resp, _cnt;

  while (true) {
    msg = chMBFetchTimeout(&alarmEvent_mb, (msg_t*)&inMsg, TIME_INFINITE);
    if (msg == MSG_OK) {
      // Lookup group
      _group = (conf.zone[inMsg->zone] >> 1) & 0b1111;

      // Group has alarm already nothing to do!
      if ((group[_group].setting >> 1) & 0b1) {
        chPoolFree(&alarmEvent_pool, inMsg);
        continue;
      }
      // Set authentication On
      group[_group].setting |= (1 << 2);
      // Swt wait time
      if (inMsg->type == 'P') _wait = (conf.zone[inMsg->zone] >> 5) & 0b11;
      else                    _wait = 0; // Tamper have no wait time
      //       wait > 0    NOT group has alarm already                authentication On
      while ((_wait > 0) && !((group[_group].setting >> 1) & 0b1) && (group[_group].setting >> 2 & 0b1)) {
        //++_resp = sendCmdToGrp(_group, 11 + _wait);
        _cnt = 0;
        //       Authentication On                    time of one alarm period      NOT group has alarm already
        while ((group[_group].setting >> 2 & 0b1) && (_cnt < (10*conf.alarmTime)) && !((group[_group].setting >> 1) & 0b1)) {
          chThdSleepMilliseconds(100);;
          _cnt++;
        }
        //  Authentication On
        if (group[_group].setting >> 2 & 0b1) _wait--;
      }
      //   wait = 0   NOT group has alarm already
      if ((!_wait) && !((group[_group].setting >> 1) & 0b1)) {
        group[_group].setting |= (1 << 1); // Set alarm bit On
        //++_resp = sendCmdToGrp(_group, 11);  // ALARM COMMAND
        // Combine alarms, so that next alarm will not disable ongoing one
        if (inMsg->type == 'P') {
          //++OUTs = ((((conf.group[_group] >> 4) & B1) | (OUTs >> 0) & B1) | (((conf.group[_group] >> 3) & B1) | (OUTs >> 1) & B1) << 1);
        } else {
          //++OUTs = ((((conf.group[_group] >> 2) & B1) | (OUTs >> 0) & B1) | (((conf.group[_group] >> 1) & B1) | (OUTs >> 1) & B1) << 1);
        }
        // Trigger OUT 1 & 2
        //++pinOUT1.write(((OUTs >> 0) & B1));
        //++pinOUT2.write(((OUTs >> 1) & B1));
        //++publishGroup(_group, 'T');
        tmpLog[0] = 'S'; tmpLog[1] = 'X';  tmpLog[2] = _group;  pushToLog(tmpLog, 3); // ALARM no auth.
      }
    } else {
      chprintf(console, "%s -> ERROR\r\n", arg);
    }
    chPoolFree(&alarmEvent_pool, inMsg);
  }
}

/*
 * Logger thread
 */
static THD_WORKING_AREA(waLoggerThread, 256);
static THD_FUNCTION(LoggerThread, arg) {
  chRegSetThreadName(arg);
  msg_t    msg;
  logger_t *inMsg;
  char     buffer[FRAM_MSG_SIZE+3];

  while (true) {
    msg = chMBFetchTimeout(&logger_mb, (msg_t*)&inMsg, TIME_INFINITE);
    if (msg == MSG_OK) {
      /*
      chprintf(console, "Log %d", inMsg);
      chprintf(console, ", T %d", inMsg->timestamp);
      chprintf(console, ", %s", inMsg->text);
      chprintf(console, "\r\n");
      */
      spiAcquireBus(&SPID1);

      spiSelect(&SPID1);
      buffer[0] = CMD_25AA_WREN;
      spiSend(&SPID1, 1, buffer);
      spiUnselect(&SPID1);

      buffer[0] = CMD_25AA_WRITE;
      buffer[1] = (FRAMWritePos >> 8) & 0xFF;
      buffer[2] = FRAMWritePos & 0xFF;

      timeConv.val = inMsg->timestamp;
      memcpy(&buffer[3], &timeConv.ch[0], sizeof(timeConv.ch)); // Copy time to buffer
      memcpy(&buffer[7], &inMsg->text[0], LOGGER_MSG_LENGTH);   // Copy text to buffer
      buffer[FRAM_MSG_SIZE+2] = 0xFF;                           // Set flag

      /*
      chprintf(console, ">%s\r\n", &buffer[7]);
      for(uint8_t i = 0; i < FRAM_MSG_SIZE+3; i++) {
         chprintf(console, "%x %c-", buffer[i], buffer[i]);
      }
      chprintf(console, "\r\n");
      */

      spiSelect(&SPID1);
      spiSend(&SPID1, FRAM_MSG_SIZE+3, buffer);
      spiUnselect(&SPID1);
      spiReleaseBus(&SPID1);

      FRAMWritePos += FRAM_MSG_SIZE; // uint16_t will overflow by itself or FRAM address registers are ignored
    } else {
      chprintf(console, "Log ERROR\r\n");
    }
    chPoolFree(&logger_pool, inMsg);
  }
}


/*
 * RS485 thread
 */
static THD_WORKING_AREA(waRS485Thread, 256);
static THD_FUNCTION(RS485Thread, arg) {
  chRegSetThreadName(arg);
  event_listener_t serialListener;
  msg_t resp;
  RS485Msg_t rs485Msg;
  uint8_t _pos;

  // Register
  // ++ old chEvtRegister((event_source_t *)chnGetEventSource(&RS485D2.event), &serialListener, EVENT_MASK(0));
  chEvtRegister((event_source_t *)&RS485D2.myEvent, &serialListener, EVENT_MASK(0));

  while (true) {
    eventmask_t evt = chEvtWaitAny(ALL_EVENTS);
    (void)evt;

    eventflags_t flags = chEvtGetAndClearFlags(&serialListener);
    chprintf(console, "RS485: %d, %d-%d\r\n", flags, RS485D2.trcState, RS485D2.ibHead);
    //resp = chBSemWait(&RS485D2.received);
    if (flags & RS485_MSG_RECEIVED){
      resp = rs485GetMsg(&RS485D2, &rs485Msg);
      chprintf(console, "RS485 received: %d, ", resp);
      chprintf(console, "from %d, ", rs485Msg.address);
      chprintf(console, "ctrl %d, ", rs485Msg.ctrl);
      chprintf(console, "length %d\r\n", rs485Msg.length);
      chprintf(console, "Data: ");
      for(uint8_t i = 0; i < rs485Msg.length; i++) {
        chprintf(console, "%d-%x, ", i, rs485Msg.data[i]);
      }
      chprintf(console, "\r\n");
      /*
      for(uint8_t i = RS485_HEADER_SIZE; i < rs485Msg.length + RS485_HEADER_SIZE + RS485_CRC_SIZE; i++) {
        chprintf(console, "%d-%x, ", i,  RS485D2.ib[i]);
      }
      chprintf(console, "%x - %x\r\n", RS485D2.crc >> 8, RS485D2.crc & 0b11111111);
      */

      // Registration
      if ((rs485Msg.ctrl == RS485_FLAG_DTA) && (rs485Msg.data[0]=='R')) {
        _pos = 0;
        do {
          _pos++; // Skip 'R'
          registration_t *outMsg = chPoolAlloc(&registration_pool);
          if (outMsg != NULL) {
            // node setting
            outMsg->node    = rs485Msg.data[_pos];
            outMsg->address = rs485Msg.address;
            outMsg->type    = rs485Msg.data[_pos+1];
            outMsg->number  = (uint8_t)rs485Msg.data[_pos+2];
            outMsg->setting = ((uint8_t)rs485Msg.data[_pos+3] << 8) | ((uint8_t)rs485Msg.data[_pos+4]);
            memcpy(&outMsg->name[0], &rs485Msg.data[_pos+5], NAME_LENGTH);  // Copy string

            msg_t msg = chMBPostTimeout(&registration_mb, (msg_t)outMsg, TIME_IMMEDIATE);
            if (msg != MSG_OK) {
              //chprintf(console, "R-MB full %d\r\n", temp);
            }
          } else {
            pushToLogText("FR"); // Registration queue is full
          }
          _pos+=REG_PACKET_SIZE;
        } while (_pos < rs485Msg.length);
      }

    }



  }
}

/*
 * Registration thread
 */
static THD_WORKING_AREA(waRegistrationThread, 256);
static THD_FUNCTION(RegistrationThread, arg) {
  chRegSetThreadName(arg);
  msg_t    msg;
  registration_t *inMsg;
  uint8_t  _resp, _node;

  while (true) {
    msg = chMBFetchTimeout(&registration_mb, (msg_t*)&inMsg, TIME_INFINITE);
    if (msg == MSG_OK) {
      chprintf(console, "Incoming registration\r\n");

      chprintf(console, "Node %c", inMsg->node);
      chprintf(console, ", %c", inMsg->type);
      chprintf(console, "\r\n");


      switch(inMsg->node){
        case 'K':
        case 'S':
        case 'I':
          _resp = 0;
          for (_node=0; _node < NODES; _node++) {
            if (node[_node].address  == inMsg->address &&
                node[_node].type     == inMsg->type &&
                node[_node].number   == inMsg->number &&
                node[_node].function == inMsg->node) {
              _resp = 1;
              break;
            }
          }
          // if node not present already
          if (!_resp) {
            for (_node=0; _node < NODES; _node++) {
              // Find empty slot
              if (node[_node].address == 0) {
                _resp = 1;
                break;
              }
            }
            // if no empty slot
            if (!_resp) {
              pushToLogText("FN");
            } else {
              node[_node].address  = inMsg->address;
              node[_node].function = inMsg->node;
              node[_node].type     = inMsg->type;
              node[_node].number   = inMsg->number;
              node[_node].setting  = inMsg->setting;
              node[_node].last_OK  = GetTimeUnixSec();  // Get timestamp
              memcpy(&node[_node].name, &inMsg->name, NAME_LENGTH);// node[_node].name[NAME_LENGTH-1] = 0;
              tmpLog[0] = 'N'; tmpLog[1] = 'R'; tmpLog[2] = inMsg->address; tmpLog[3] = inMsg->number; tmpLog[4] = inMsg->node; tmpLog[5] = inMsg->type;  pushToLog(tmpLog, 6);
              chprintf(console, "Registration %d\r\n", _node);
            }
          } else {
            // there is match already
            node[_node].setting  = inMsg->setting;
            node[_node].last_OK  = GetTimeUnixSec();  // Get timestamp
            node[_node].value    = 0; // Reset value
            memcpy(&node[_node].name, &inMsg->name, NAME_LENGTH);// node[_node].name[NAME_LENGTH-1] = 0;
            chprintf(console, "Re-registration %d\r\n", _node);
          }
          break;
          default:
            tmpLog[0] = 'N'; tmpLog[1] = 'E'; tmpLog[2] = inMsg->address; tmpLog[3] = inMsg->number; tmpLog[4] = inMsg->node; tmpLog[5] = inMsg->type; pushToLog(tmpLog, 6);
          break;
      } // end switch
    } else {
      chprintf(console, "Registration ERROR\r\n");
    }
    chPoolFree(&registration_pool, inMsg);
  }
}


/*
 * This is a periodic thread that does absolutely nothing except flashing
 * a LED.
 */
static THD_WORKING_AREA(waThread1, 128);
static THD_FUNCTION(Thread1, arg) {
  chRegSetThreadName(arg);
  systime_t time;

  while (true) {
    time = serusbcfg.usbp->state == USB_ACTIVE ? 250 : 500;

    //palSetPad(GPIOB, GPIOB_RELAY_2);
    chThdSleepMilliseconds(time);
    //palClearPad(GPIOB, GPIOB_RELAY_2);
    chThdSleepMilliseconds(2000);
  }
}


/*
 * helper function
 */
static void GetTimeTm(struct tm *timp) {
  rtcGetTime(&RTCD1, &timespec);
  rtcConvertDateTimeToStructTm(&timespec, timp, NULL);
}

/*
 * helper function
 */
static void SetTimeUnixSec(time_t unix_time) {
  struct tm tim;
  struct tm *canary;

  /* If the conversion is successful the function returns a pointer
     to the object the result was written into.*/
  canary = localtime_r(&unix_time, &tim);
  osalDbgCheck(&tim == canary);

  rtcConvertStructTmToDateTime(&tim, 0, &timespec);
  rtcSetTime(&RTCD1, &timespec);
}


#include "ohs_shell.h"

/*===========================================================================*/
/* Command line related.                                                     */
/*===========================================================================*/

/*
 *
 */
static SerialConfig ser_cfg = {
    115200,
    0,
    0,
    0
};

// Peripherial Clock 42MHz SPI2 SPI3
// Peripherial Clock 84MHz SPI1                                SPI1        SPI2/3
#define SPI_BaudRatePrescaler_2         ((uint16_t)0x0000) //  42 MHz      21 MHZ
#define SPI_BaudRatePrescaler_4         ((uint16_t)0x0008) //  21 MHz      10.5 MHz
#define SPI_BaudRatePrescaler_8         ((uint16_t)0x0010) //  10.5 MHz    5.25 MHz
#define SPI_BaudRatePrescaler_16        ((uint16_t)0x0018) //  5.25 MHz    2.626 MHz
#define SPI_BaudRatePrescaler_32        ((uint16_t)0x0020) //  2.626 MHz   1.3125 MHz
#define SPI_BaudRatePrescaler_64        ((uint16_t)0x0028) //  1.3125 MHz  656.25 KHz
#define SPI_BaudRatePrescaler_128       ((uint16_t)0x0030) //  656.25 KHz  328.125 KHz
#define SPI_BaudRatePrescaler_256       ((uint16_t)0x0038) //  328.125 KHz 164.06 KHz
/*
 * Maximum speed SPI configuration (18MHz, CPHA=0, CPOL=0, MSb first).
 */
const SPIConfig spi1cfg = {
  false,
  NULL,
  GPIOD, // CS PORT
  GPIOD_SPI1_CS, // CS PIN
  SPI_BaudRatePrescaler_32,
  0
};

static RS485Config ser_mpc_cfg = {
    19200,          // speed
    0,              // address
    GPIOD,          // port
    GPIOD_USART2_DE // pad
};

msg_t resp;
time_t temptime;
struct tm *ptm;

/*
 * Application entry point.
 */
int main(void) {

  halInit();
  chSysInit();

  sdStart(&SD3,  &ser_cfg); // Debug port
  sdStart(&SD6,  &ser_cfg); // GSM modem
  chprintf(console, "\r\nOHS start\r\n");

  rs485Start(&RS485D2, &ser_mpc_cfg);
  chprintf(console, "RS485 timeout: %d/%d\r\n", RS485D2.oneByteTimeUS, RS485D2.oneByteTimeI);

  // Initializes a serial-over-USB CDC driver.
  sduObjectInit(&SDU1);
  sduStart(&SDU1, &serusbcfg);
  /*
   * Activates the USB driver and then the USB bus pull-up on D+.
   * Note, a delay is inserted in order to not have to disconnect the cable
   * after a reset.
   */
  usbDisconnectBus(serusbcfg.usbp);
  chThdSleepMilliseconds(1500);
  usbStart(serusbcfg.usbp, &usbcfg);
  usbConnectBus(serusbcfg.usbp);

  shellInit();

  // Creating the mailboxes.
  //chMBObjectInit(&alarmEvent_mb, alarmEvent_mb_buffer, ALARMEVENT_FIFO_SIZE);

  /* Pools */
  chPoolObjectInit(&alarmEvent_pool, sizeof(alarmEvent_t), NULL);
  chPoolObjectInit(&logger_pool, sizeof(logger_t), NULL);
  chPoolObjectInit(&registration_pool, sizeof(registration_t), NULL);
  //chPoolObjectInit(&node_pool, sizeof(node_t), NULL);
  //chPoolLoadArray(&alarmEvent_pool, alarmEvent_pool_queue, ALARMEVENT_FIFO_SIZE);
  for(uint8_t i = 0; i < ALARMEVENT_FIFO_SIZE; i++) { chPoolFree(&alarmEvent_pool, &alarmEvent_pool_queue[i]); }
  for(uint8_t i = 0; i < LOGGER_FIFO_SIZE; i++) { chPoolFree(&logger_pool, &logger_pool_queue[i]); }
  for(uint8_t i = 0; i < REG_FIFO_SIZE; i++) { chPoolFree(&registration_pool, &registration_pool_queue[i]); }
  //for(uint8_t i = 0; i < NODES; i++) { chPoolFree(&node_pool, &node_pool_queue[i]); }

  //i2cStart(&I2CD1, &i2cfg1); // I2C
  spiStart(&SPID1, &spi1cfg);  // SPI
  adcStart(&ADCD1, NULL);      // Activates the ADC1 driver

  /*
  palClearPad(GPIOE, GPIOE_ETH_RESET);
  chThdSleepMilliseconds(200);
  palSetPad(GPIOE, GPIOE_ETH_RESET);
  */

  /*
   * Creates thread(s).
   */
  chThdCreateStatic(waZoneThread, sizeof(waZoneThread), NORMALPRIO, ZoneThread, (void*)"Zone");
  chThdCreateStatic(waAEThread1, sizeof(waAEThread1), NORMALPRIO, AEThread, (void*)"AET 1");
  chThdCreateStatic(waAEThread2, sizeof(waAEThread2), NORMALPRIO, AEThread, (void*)"AET 2");
  chThdCreateStatic(waAEThread3, sizeof(waAEThread3), NORMALPRIO, AEThread, (void*)"AET 3");
  chThdCreateStatic(waLoggerThread, sizeof(waLoggerThread), NORMALPRIO, LoggerThread, (void*)"Logger");
  chThdCreateStatic(waRS485Thread, sizeof(waRS485Thread), NORMALPRIO, RS485Thread, (void*)"RS485");
  chThdCreateStatic(waRegistrationThread, sizeof(waRegistrationThread), NORMALPRIO, RegistrationThread, (void*)"Registration");
  chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, (void*)"LED");
//  shellInit();
  //chThdCreateStatic(waShell, sizeof(waShell), NORMALPRIO, shellThread, (void *)&shell_cfg1);
  lwipInit(NULL);

  /*
   * Conf. and runtime initialization.
   */
  // Read conf.
  readFromBkpSRAM((uint8_t*)&conf, sizeof(config_t), 0);
  // Check if we have new major version update
  if (conf.versionMajor != OHS_MAJOR) {
    setConfDefault(); // Load OHS default conf.
    initRuntimeGroups(); // Initialize runtime variables
    writeToBkpSRAM((uint8_t*)&conf, sizeof(config_t), 0);
    writeToBkpRTC((uint8_t*)&group, sizeof(group), 0);
  }

  // Read last group[] state
  readFromBkpRTC((uint8_t*)&group, sizeof(group), 0);
  // Initialize zone state
  initRuntimeZones();

  /*
   * Ethernet
   */
  httpd_init();           // Starts the HTTP server
  //sntp_setserver(0, "ntp1.sh.cvut.cz");
  sntp_init();


  chprintf(console, "Size of conf: %d, group: %d\r\n", sizeof(conf), sizeof(group));


  //uint8_t data;
  //uint32_t data32;
  //uint8_t *baseAddress = (uint8_t *) BKPSRAM_BASE;
  //volatile uint32_t *RTCBaseAddress = &(RTC->BKP0R);
  // for(uint16_t i = 0; i < 0x100; i++) { *(base_address + i) = 0x55; } //erase BKP_SRAM
  //for(uint16_t i = 0; i < 20; i++) { *(RTCBaseAddress + i) = (0x55 << 24) | (0x55 << 16) | (0x55 << 8) | (0x55 << 0);} // Erase RTC bkp
  //chprintf(console, "BRTC %d ", writeToBkpRTC((uint8_t*)&myStr, sizeof(myStr), 0));


  while (true) {
    if (SDU1.config->usbp->state == USB_ACTIVE) {
      thread_t *shelltp = chThdCreateFromHeap(NULL, SHELL_WA_SIZE,"shell", NORMALPRIO + 1, shellThread, (void *)&shell_cfg1);
      chThdWait(shelltp);               /* Waiting termination.             */
    }

    chThdSleepMilliseconds(10000);

    temptime = calculateDST(2019, 3, 0, 0, 2);
    chprintf(console, "DST s %d \r\n", temptime);
    ptm = gmtime(&temptime);
    chprintf(console, "DST s %s \r\n", asctime(ptm));

    temptime = calculateDST(2019, 10, 0, 0, 3);
    chprintf(console, "DST e %d \r\n", temptime);
    ptm = gmtime(&temptime);
    chprintf(console, "DST e %s \r\n", asctime(ptm));

    // Dump BKP SRAM
    /*
    for(uint16_t i = 0; i < 0x100; i++) {
      data = *(baseAddress + i);
      chprintf(console, "%x ", data);
      if (((i+1)%0x10) == 0) {
        chprintf(console, "\r\n");
        chThdSleepMilliseconds(2);
      }
    }
    */
    // Dump BKP RTC
    /*
    for(uint16_t i = 0; i < 20; i++) {
      data32 = *(RTCBaseAddress + i);
      chprintf(console, "%x %x %x %x ", (data32 >> 24) &0xFF, (data32 >> 16) &0xFF,(data32 >> 8) &0xFF,(data32 >> 0) &0xFF);
      if (((i+1)%4) == 0) {
        chprintf(console, "\r\n");
        chThdSleepMilliseconds(2);
      }
    }
    */
    // BKP RTC read test
    /*
    chprintf(console, ">%s, %d<\r\n", myStr, sizeof(myStr));
    chprintf(console, "Read >%d\r\n", readFromBkpRTC((uint8_t*)&myStr, sizeof(myStr), 0));
    chprintf(console, ">%s<\r\n", myStr);
    */

    //i2cEEGetData();

    /* Send RS485 registartion request
    RS485Msg_t rs485Msg;
    rs485Msg.address = 15;
    //rs485Msg.ctrl = RS485_FLAG_DTA;
    rs485Msg.ctrl = RS485_FLAG_CMD;
    //rs485Msg.length = 10;
    rs485Msg.length = 1;
    for (uint8_t i = 0; i < rs485Msg.length; i++) { rs485Msg.data[i] = i; }
    resp = rs485SendMsg(&RS485D2, &rs485Msg);
    chprintf(console, ">%d<\r\n", resp);
    */

  }
}
