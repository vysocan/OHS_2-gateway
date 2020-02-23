/*
 * ohs_th_service.h
 *
 *  Created on: 23. 2. 2020
 *      Author: adam
 */

#ifndef OHS_TH_SERVICE_H_
#define OHS_TH_SERVICE_H_


/*
 * Service thread
 */
static THD_WORKING_AREA(waServiceThread, 256);
static THD_FUNCTION(ServiceThread, arg) {
  chRegSetThreadName(arg);

  while (true) {
    chThdSleepMilliseconds(1000);
    // Remove zombie nodes
    for (uint8_t nodeIndex=0; nodeIndex < NODE_SIZE; nodeIndex++) {
      if ((node[nodeIndex].address != 0) &&
          (node[nodeIndex].last_OK + SECONDS_PER_HOUR < GetTimeUnixSec())) {
        chprintf(console, "Zombie node: %u,A %u,T %u,F %u,N %u\r\n", nodeIndex, node[nodeIndex].address,
                 node[nodeIndex].type, node[nodeIndex].function, node[nodeIndex].number);
        tmpLog[0] = 'N'; tmpLog[1] = 'Z'; tmpLog[2] = node[nodeIndex].address;
        tmpLog[3] = node[nodeIndex].number; tmpLog[4] = node[nodeIndex].type;
        tmpLog[5] = node[nodeIndex].function; pushToLog(tmpLog, 6);
        // Set whole struct to 0
        memset(&node[nodeIndex].address, 0, sizeof(node[0]));
        //0, '\0', '\0', 0, 0b00011110, 0, 0, 255, ""
        //node[nodeIndex].address  = 0;
        //node[nodeIndex].type     = '\0';
        //node[nodeIndex].function = '\0';
        //node[nodeIndex].number   = 0;
        //node[nodeIndex].setting  = 0;
        //node[nodeIndex].value    = 0;
        //node[nodeIndex].last_OK  = 0;
        node[nodeIndex].queue    = 255;
        //memset(&node[nodeIndex].name, 0, NAME_LENGTH);
      }
    }

    //

  }
}


#endif /* OHS_TH_SERVICE_H_ */
