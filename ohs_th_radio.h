/*
 * ohs_th_radio.h
 *
 *  Created on: 31. 3. 2020
 *      Author: adam
 */

#ifndef OHS_TH_RADIO_H_
#define OHS_TH_RADIO_H_

/*
 * RFM69 thread
 */
static THD_WORKING_AREA(waRadioThread, 512);
static THD_FUNCTION(RadioThread, arg) {
  chRegSetThreadName(arg);
  msg_t resp;
  uint8_t pos;
  int8_t nodeIndex;

  while (true) {
    resp = chBSemWaitTimeout(&rfm69DataReceived, TIME_INFINITE);
    chprintf(console, "@%u\r\n", chVTGetSystemTimeX());

    if ((resp == MSG_OK) && (rfm69GetData() == RF69_RSLT_OK)) {
      chprintf(console, "Time %u\r\n", chVTGetSystemTimeX());
      chprintf(console, "Radio sender: %u, RSSI: %d, Data: ", rfm69Data.senderId, rfm69Data.rssi);
      for(uint8_t i = 0; i < rfm69Data.length; i++) {
        chprintf(console, "%u, ", rfm69Data.data[i]);
      }
      chprintf(console, "\r\n");

      // Do some logic on received packet
      switch(rfm69Data.data[0]) {
        case 'C': // Commands
          switch((uint8_t)rfm69Data.data[1]) {
            case NODE_CMD_PING: // Nodes should do periodic ping to stay alive/registered
              for (uint8_t nodeIndex=0; nodeIndex < NODE_SIZE; nodeIndex++) {
                if (node[nodeIndex].address == rfm69Data.senderId)
                  node[nodeIndex].last_OK = getTimeUnixSec();
              }
            break;
          }
          break;
        case 'R': // Registration
          pos = 0;
          do {
            pos++; // Skip 'R'
            registration_t *outMsg = chPoolAlloc(&registration_pool);
            if (outMsg != NULL) {
              // node setting
              outMsg->address  = rfm69Data.senderId;
              outMsg->type     = (char)rfm69Data.data[pos];
              outMsg->function = (char)rfm69Data.data[pos+1];
              outMsg->number   = rfm69Data.data[pos+2];
              outMsg->setting  = (rfm69Data.data[pos+3] << 8) | (rfm69Data.data[pos+4]);
              memcpy(&outMsg->name[0], &rfm69Data.data[pos+5], NAME_LENGTH);  // Copy string

              msg_t msg = chMBPostTimeout(&registration_mb, (msg_t)outMsg, TIME_IMMEDIATE);
              if (msg != MSG_OK) {
                //chprintf(console, "R-MB full %d\r\n", temp);
              }
            } else {
              pushToLogText("FR"); // Registration queue is full
            }
            pos += REG_PACKET_SIZE;
          } while (pos < rfm69Data.length);
          break;
        case 'K': // iButtons keys
          nodeIndex = getNodeIndex(rfm69Data.senderId, rfm69Data.data[0],
                                   rfm69Data.data[1], rfm69Data.data[2] - (rfm69Data.data[2] % 2));
          chprintf(console, "Received Key, node index: %d\r\n", nodeIndex);
          // Node index found
          if (nodeIndex != -1) {
            node[nodeIndex].last_OK = getTimeUnixSec(); // Update timestamp
            //  Node is enabled
            if (GET_NODE_ENABLED(node[nodeIndex].setting)) {
              checkKey(GET_NODE_GROUP(node[nodeIndex].setting), (rfm69Data.data[2] % 2), &rfm69Data.data[3]);
            } else {
              // log disabled remote nodes
              tmpLog[0] = 'N'; tmpLog[1] = 'F'; tmpLog[2] = rfm69Data.senderId;
              tmpLog[3] = rfm69Data.data[2]; tmpLog[4] = rfm69Data.data[0];
              tmpLog[5] = rfm69Data.data[1];  pushToLog(tmpLog, 6);
            }
          } else { // node not found
            chThdSleepMilliseconds(5);  // This is needed for sleeping battery nodes, or they wont see reg. command.
            resp = sendCmd(rfm69Data.senderId, NODE_CMD_REGISTRATION); // call this address to register
          }
          break;
        case 'S': // Sensor data
          pos = 0;
          do {
            sensor_t *outMsg = chPoolAlloc(&sensor_pool);
            if (outMsg != NULL) {
              // node setting
              outMsg->address  = rfm69Data.senderId;
              outMsg->type     = (char)rfm69Data.data[pos];
              outMsg->function = (char)rfm69Data.data[pos+1];
              outMsg->number   = rfm69Data.data[pos+2];
              floatConv.byte[0] = rfm69Data.data[pos+3]; floatConv.byte[1] = rfm69Data.data[pos+4];
              floatConv.byte[2] = rfm69Data.data[pos+5]; floatConv.byte[3] = rfm69Data.data[pos+6];
              outMsg->value = floatConv.val;

              msg_t msg = chMBPostTimeout(&sensor_mb, (msg_t)outMsg, TIME_IMMEDIATE);
              if (msg != MSG_OK) {
                //chprintf(console, "S-MB full %d\r\n", temp);
              }
            } else {
              pushToLogText("FS"); // Sensor queue is full
            }
            pos += SENSOR_PACKET_SIZE;
          } while (pos < rfm69Data.length);
          break;
      } // switch case
    } // received
  }
}



#endif /* OHS_TH_RADIO_H_ */
