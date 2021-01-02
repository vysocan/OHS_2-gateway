/*
 * ohs_th_sensor.h
 *
 *  Created on: 22. 2. 2020
 *      Author: adam
 */

#ifndef OHS_TH_SENSOR_H_
#define OHS_TH_SENSOR_H_

#ifndef SENSOR_DEBUG
#define SENSOR_DEBUG 0
#endif

#if SENSOR_DEBUG
#define DBG_SENSOR(...) {chprintf((console, __VA_ARGS__);}
#else
#define DBG_SENSOR(...)
#endif

/*
 * Sensor thread
 */
static THD_WORKING_AREA(waSensorThread, 320);
static THD_FUNCTION(SensorThread, arg) {
  chRegSetThreadName(arg);
  msg_t    msg;
  sensorEvent_t *inMsg;
  triggerEvent_t *outMsgTrig;
  uint8_t  nodeIndex;
  uint8_t  lastNode = DUMMY_NO_VALUE;
  uint32_t lastNodeTime = 0;
  time_t   timeNow;

  while (true) {
    msg = chMBFetchTimeout(&sensor_mb, (msg_t*)&inMsg, TIME_INFINITE);
    if (msg == MSG_OK) {
      // Get current time
      timeNow = getTimeUnixSec();

      nodeIndex = getNodeIndex(inMsg->address, inMsg->type, inMsg->function, inMsg->number);
      if (nodeIndex != DUMMY_NO_VALUE) {
        DBG_SENSOR("Sensor data for node %c-%c\r\n", inMsg->type, inMsg->function);
        //  node enabled
        if (GET_NODE_ENABLED(node[nodeIndex].setting)) {
          node[nodeIndex].value   = inMsg->value;
          node[nodeIndex].lastOK = timeNow;  // Get timestamp
          // MQTT
          if (GET_NODE_MQTT(node[nodeIndex].setting)) pushToMqtt(typeSensor, nodeIndex, functionValue);
          // Global battery check
          if ((node[nodeIndex].function == 'B') && !(GET_NODE_BATT_LOW(node[nodeIndex].setting)) && (node[nodeIndex].value < 3.6)){
            SET_NODE_BATT_LOW(node[nodeIndex].setting); // switch ON  battery low flag
            tmpLog[0] = 'R'; tmpLog[1] = 'A'; tmpLog[2] = DUMMY_NO_VALUE; tmpLog[3] = nodeIndex; pushToLog(tmpLog, 4);
          }
          if ((node[nodeIndex].function == 'B') && (GET_NODE_BATT_LOW(node[nodeIndex].setting)) && (node[nodeIndex].value > 4.16)){
            tmpLog[0] = 'R'; tmpLog[1] = 'D'; tmpLog[2] = DUMMY_NO_VALUE; tmpLog[3] = nodeIndex; pushToLog(tmpLog, 4);
            CLEAR_NODE_BATT_LOW(node[nodeIndex].setting); // switch OFF battery low flag
          }
          // Triggers
          outMsgTrig = chPoolAlloc(&trigger_pool);
          if (outMsgTrig != NULL) {
            //memcpy(outMsg, inMsg, sizeof(sensorEvent_t));
            outMsgTrig->address = inMsg->address;
            outMsgTrig->function = inMsg->function;
            outMsgTrig->number = inMsg->number;
            outMsgTrig->type = inMsg->type;
            outMsgTrig->value = inMsg->value;
            msg = chMBPostTimeout(&trigger_mb, (msg_t)outMsgTrig, TIME_IMMEDIATE);
            if (msg != MSG_OK) {
              //chprintf(console, "S-MB full %d\r\n", temp);
            }
          } else {
            pushToLogText("FT"); // Trigger queue is full
          }
        } // node enabled
      } else {
        // Let's call same unknown node for re-registration only once a while,
        // or we send many packets if multiple sensor data come in
        if ((lastNode != inMsg->address) || (timeNow > lastNodeTime)) {
          DBG_SENSOR("Unregistered sensor\r\n");
          chThdSleepMilliseconds(5);  // This is needed for sleeping battery nodes, or they wont see reg. command.
          nodeIndex = sendCmd(inMsg->address, NODE_CMD_REGISTRATION); // call this address to register
          lastNode = inMsg->address;
          lastNodeTime = timeNow + 1; // add 1-2 second(s)
        }
      }
    } else {
      DBG_SENSOR("Sensor ERROR\r\n");
    }

    chPoolFree(&sensor_pool, inMsg);
  }
}


#endif /* OHS_TH_SENSOR_H_ */
