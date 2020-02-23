/*
 * ohs_th_sensor.h
 *
 *  Created on: 22. 2. 2020
 *      Author: adam
 */

#ifndef OHS_TH_SENSOR_H_
#define OHS_TH_SENSOR_H_


/*
 * Sensor thread
 */
static THD_WORKING_AREA(waSensorThread, 256);
static THD_FUNCTION(SensorThread, arg) {
  chRegSetThreadName(arg);
  msg_t    msg;
  sensor_t *inMsg;
  int8_t   nodeIndex;
  uint8_t  lastNode = 255;
  uint32_t lastNodeTime = 0;

  while (true) {
    msg = chMBFetchTimeout(&sensor_mb, (msg_t*)&inMsg, TIME_INFINITE);
    if (msg == MSG_OK) {
      nodeIndex = getNodeIndex(inMsg->address, inMsg->type, inMsg->function, inMsg->number);
      if (nodeIndex != -1) {
        chprintf(console, "Sensor data for node %c-%c\r\n", inMsg->type, inMsg->function);
        //  node enabled
        if (GET_NODE_ENABLED(node[nodeIndex].setting)) {
          node[nodeIndex].value   = inMsg->value;
          node[nodeIndex].last_OK = GetTimeUnixSec();  // Get timestamp
          //++publishNode(nodeIndex); // MQTT
          // Triggers
          //++processTriggers(node[nodeIndex].address, node[nodeIndex].type, node[nodeIndex].number, node[nodeIndex].value);
          // Global battery check
          if ((node[nodeIndex].function == 'B') && !(GET_NODE_BATT_LOW(node[nodeIndex].setting)) && (node[nodeIndex].value < 3.6)){
            SET_NODE_BATT_LOW(node[nodeIndex].setting); // switch ON  battery low flag
            tmpLog[0] = 'R'; tmpLog[1] = 'A'; tmpLog[2] = 255; tmpLog[3] = nodeIndex; pushToLog(tmpLog, 4);
          }
          if ((node[nodeIndex].function == 'B') && (GET_NODE_BATT_LOW(node[nodeIndex].setting)) && (node[nodeIndex].value > 4.16)){
            tmpLog[0] = 'R'; tmpLog[1] = 'D'; tmpLog[2] = 255; tmpLog[3] = nodeIndex; pushToLog(tmpLog, 4);
            CLEAR_NODE_BATT_LOW(node[nodeIndex].setting); // switch OFF battery low flag
          }
        } // node enabled
      } else {
        // Let's call same unknown node for re-registrtion only once a while or we send many packets if multiple sensor data come in
        if ((lastNode != inMsg->address) || (GetTimeUnixSec() > lastNodeTime)) {
          chprintf(console, "Unregistered sensor\r\n");
          chThdSleepMilliseconds(5);  // This is needed for sleeping battery nodes, or they wont see reg. command.
          nodeIndex = sendCmd(inMsg->address, NODE_CMD_REGISTRATION); // call this address to register
          lastNode = inMsg->address;
          lastNodeTime = GetTimeUnixSec() + 1; // add 1-2 second(s)
        }
      }
    }else {
      chprintf(console, "Sensor ERROR\r\n");
    }
    chPoolFree(&sensor_pool, inMsg);
  }
}


#endif /* OHS_TH_SENSOR_H_ */
