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
  registration_t *inMsg;
  int8_t nodeIndex;

  while (true) {
    msg = chMBFetchTimeout(&registration_mb, (msg_t*)&inMsg, TIME_INFINITE);
    if (msg == MSG_OK) {
      chprintf(console, "Registration for node %c-%c\r\n", inMsg->type, inMsg->function);
      switch(inMsg->type){
        case 'K':
        case 'S':
        case 'I':
          nodeIndex = getNodeIndex(inMsg->address, inMsg->type, inMsg->function, inMsg->number);
          // Node exists
          if (nodeIndex >= 0 ) {
            node[nodeIndex].setting  = inMsg->setting;
            node[nodeIndex].last_OK  = GetTimeUnixSec();
            node[nodeIndex].value    = 0; // Reset value
            memcpy(&node[nodeIndex].name, &inMsg->name, NAME_LENGTH); // node[nodeIndex].name[NAME_LENGTH-1] = 0;
            chprintf(console, "Re-registred as: %d\r\n", nodeIndex);
          } else {
            nodeIndex = getNodeFreeIndex(); // Find empty slot
            if (nodeIndex == -1) {
              pushToLogText("FN"); // No empty slot
            } else {
              node[nodeIndex].type     = inMsg->type;
              node[nodeIndex].address  = inMsg->address;
              node[nodeIndex].function = inMsg->function;
              node[nodeIndex].number   = inMsg->number;
              node[nodeIndex].setting  = inMsg->setting;
              node[nodeIndex].last_OK  = GetTimeUnixSec();
              memcpy(&node[nodeIndex].name, &inMsg->name, NAME_LENGTH);
              tmpLog[0] = 'N'; tmpLog[1] = 'R'; tmpLog[2] = inMsg->address; tmpLog[3] = inMsg->number; tmpLog[4] = inMsg->type; tmpLog[5] = inMsg->function;  pushToLog(tmpLog, 6);
              chprintf(console, "Registered as: %d\r\n", nodeIndex);
            }
          }
          break;
          default:
            tmpLog[0] = 'N'; tmpLog[1] = 'E'; tmpLog[2] = inMsg->address; tmpLog[3] = inMsg->number; tmpLog[4] = inMsg->type; tmpLog[5] = inMsg->function; pushToLog(tmpLog, 6);
          break;
      } // end switch
    } else {
      chprintf(console, "Registration ERROR\r\n");
    }
    chPoolFree(&registration_pool, inMsg);
  }
}


#endif /* OHS_TH_REGISTRATION_H_ */
