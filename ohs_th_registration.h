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
  uint8_t nodeIndex, groupNum;

  while (true) {
    msg = chMBFetchTimeout(&registration_mb, (msg_t*)&inMsg, TIME_INFINITE);
    if (msg == MSG_OK) {
      chprintf(console, "Registration for %c-%c\r\n", inMsg->type, inMsg->function);
      switch(inMsg->type){
        case 'K':
        case 'S':
        case 'I':
        case 'H':
          nodeIndex = getNodeIndex(inMsg->address, inMsg->type, inMsg->function, inMsg->number);
          // Node exists
          if (nodeIndex != DUMMY_NO_VALUE ) {
            node[nodeIndex].setting = inMsg->setting;
            node[nodeIndex].lastOK  = getTimeUnixSec();
            memcpy(&node[nodeIndex].name, &inMsg->name, NAME_LENGTH);
            // Reset value, 0 or dummy for authentication
            if (node[nodeIndex].function == 'i') node[nodeIndex].value = DUMMY_NO_VALUE;
            else                                 node[nodeIndex].value = 0;
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
              // Set value, state upon registration.
              if (node[nodeIndex].type == 'K') {
                // Reset node value to dummy for authentication nodes
                node[nodeIndex].value = DUMMY_NO_VALUE;
                // Send initial state to Key node.
                groupNum = GET_NODE_GROUP(inMsg->setting);
//                if (GET_GROUP_ALARM(group[groupNum].setting) == 0) {
//                  if (GET_GROUP_WAIT_AUTH(group[groupNum].setting)) {
//                    sendCmd(inMsg->address, NODE_CMD_AUTH_1);
//                  } else {
//                    if (GET_GROUP_ARMED(group[groupNum].setting)) {
//                      sendCmd(inMsg->address, NODE_CMD_ARMED_AWAY);
//                    } else {
//                      if (group[groupNum].armDelay > 0) {
//                        sendCmd(inMsg->address, NODE_CMD_ARMING);
//                      } else {
//                        sendCmd(inMsg->address, NODE_CMD_DISARM);
//                      }
//                    }
//                  }
//                } else {
//                  sendCmd(inMsg->address, NODE_CMD_ALARM);
//                }
              } else {
                // Reset node value to 0
                node[nodeIndex].value = 0;
              }
              memcpy(&node[nodeIndex].name, &inMsg->name, NAME_LENGTH);
              tmpLog[0] = 'N'; tmpLog[1] = 'R'; tmpLog[2] = inMsg->address; tmpLog[3] = inMsg->type;
                tmpLog[4] = inMsg->function; tmpLog[5] = inMsg->number;  pushToLog(tmpLog, 6);
              chprintf(console, "Registered as: %d\r\n", nodeIndex);
            }
          }
          // MQTT publish name
          if (GET_NODE_MQTT(node[nodeIndex].setting)) pushToMqtt(typeSensor, nodeIndex, functionName);
          break;
          case 'Z': // Zone
            tmpLog[0] = 'Z'; // Log data
            inMsg->number--; // Array starts form 0, we subtract one from zone for clarity
            // Check if zone number is allowed
            if ((inMsg->number < ALARM_ZONES) && (inMsg->number >= HW_ZONES)) {
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
              SET_CONF_ZONE_IS_PRESENT(conf.zone[inMsg->number]); // force "Present - connected"
              if (inMsg->function == 'A') SET_CONF_ZONE_TYPE(conf.zone[inMsg->number]); // force "Analog"
              else                        CLEAR_CONF_ZONE_TYPE(conf.zone[inMsg->number]); // force "Digital"
              conf.zoneAddress[inMsg->number-HW_ZONES] = inMsg->address; // copy address
              memcpy(&conf.zoneName[inMsg->number], &inMsg->name, NAME_LENGTH);
              chprintf(console, "Registered zone: %d. %s, address index %d\r\n",
                       (inMsg->number)+1, conf.zoneName[inMsg->number], conf.zoneAddress[inMsg->number-HW_ZONES]);
              // MQTT publish name
              if (GET_CONF_ZONE_MQTT_PUB(conf.zone[inMsg->number])) pushToMqtt(typeZone, inMsg->number, functionName);
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
