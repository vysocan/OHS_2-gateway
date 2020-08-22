/*
 * ohs_th_registration.h
 *
 *  Created on: 22. 2. 2020
 *      Author: adam
 */

#ifndef OHS_TH_REGISTRATION_H_
#define OHS_TH_REGISTRATION_H_

/*
 * Registration thread
 */
static THD_WORKING_AREA(waRegistrationThread, 256);
static THD_FUNCTION(RegistrationThread, arg) {
  chRegSetThreadName(arg);
  msg_t msg;
  registrationEvent_t *inMsg;
  uint8_t nodeIndex;

  while (true) {
    msg = chMBFetchTimeout(&registration_mb, (msg_t*)&inMsg, TIME_INFINITE);
    if (msg == MSG_OK) {
      chprintf(console, "Registration for %c-%c\r\n", inMsg->type, inMsg->function);
      switch(inMsg->type){
        case 'K':
        case 'S':
        case 'I':
          nodeIndex = getNodeIndex(inMsg->address, inMsg->type, inMsg->function, inMsg->number);
          // Node exists
          if (nodeIndex != DUMMY_NO_VALUE ) {
            node[nodeIndex].setting  = inMsg->setting;
            node[nodeIndex].lastOK  = getTimeUnixSec();
            node[nodeIndex].value    = 0; // Reset value
            memcpy(&node[nodeIndex].name, &inMsg->name, NAME_LENGTH); // node[nodeIndex].name[NAME_LENGTH-1] = 0;
            chprintf(console, "Re-registered as: %d\r\n", nodeIndex);
          } else {
            nodeIndex = getNodeFreeIndex(); // Find empty slot
            if (nodeIndex == DUMMY_NO_VALUE) {
              pushToLogText("FN"); // No empty slot
            } else {
              node[nodeIndex].type     = inMsg->type;
              node[nodeIndex].address  = inMsg->address;
              node[nodeIndex].function = inMsg->function;
              node[nodeIndex].number   = inMsg->number;
              node[nodeIndex].setting  = inMsg->setting;
              node[nodeIndex].lastOK  = getTimeUnixSec();
              memcpy(&node[nodeIndex].name, &inMsg->name, NAME_LENGTH);
              tmpLog[0] = 'N'; tmpLog[1] = 'R'; tmpLog[2] = inMsg->address; tmpLog[3] = inMsg->type;
                tmpLog[4] = inMsg->function; tmpLog[5] = inMsg->number;  pushToLog(tmpLog, 6);
              chprintf(console, "Registered as: %d\r\n", nodeIndex);
            }
          }
          break;
          case 'Z': // Zone
            tmpLog[0] = 'Z'; // Log data
            // Check if zone number is allowed
            if ((inMsg->number <= ALARM_ZONES) && (inMsg->number > HW_ZONES)) {
              // Zone already connected
              if (GET_CONF_ZONE_IS_PRESENT(conf.zone[inMsg->number])) tmpLog[1] = 'r';
              else tmpLog[1] = 'R'; // Log data
              // Reset zone status
              zone[inMsg->number].lastEvent = 'O';
              zone[inMsg->number].lastOK    = getTimeUnixSec(); // update current timestamp
              zone[inMsg->number].lastPIR   = zone[inMsg->number].lastOK;

              CLEAR_ZONE_ERROR(zone[inMsg->number].setting); // reset remote zone error flag
              // Force and register
              conf.zone[inMsg->number] = inMsg->setting; // copy setting
              SET_CONF_ZONE_IS_REMOTE(conf.zone[inMsg->number]); // force "Remote zone"
              SET_CONF_ZONE_IS_PRESENT(conf.zone[inMsg->number]); // force "Present - connected"
              if (inMsg->type == 'A') SET_CONF_ZONE_TYPE(conf.zone[inMsg->number]); // force "Analog"
              else                    CLEAR_CONF_ZONE_TYPE(conf.zone[inMsg->number]); // force "Digital"
              conf.zoneAddress[inMsg->number-HW_ZONES] = inMsg->address; // copy address
              memcpy(&conf.zoneName[inMsg->number], &inMsg->name, NAME_LENGTH);
              chprintf(console, "Registered zone #%d - %s\r\n", inMsg->number, conf.zoneName[inMsg->number]);
            } else { tmpLog[1] = 'E'; } // Log data
            tmpLog[2] = inMsg->number; pushToLog(tmpLog, 3);
          break;
          default:
            tmpLog[0] = 'N'; tmpLog[1] = 'E'; tmpLog[2] = inMsg->address; tmpLog[3] = inMsg->type;
            tmpLog[4] = inMsg->function; tmpLog[5] = inMsg->number; pushToLog(tmpLog, 6);
          break;
      } // end switch
    } else {
      chprintf(console, "Registration ERROR\r\n");
    }
    chPoolFree(&registration_pool, inMsg);
  }
}

#endif /* OHS_TH_REGISTRATION_H_ */
