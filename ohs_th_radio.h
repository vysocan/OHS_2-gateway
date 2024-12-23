/*
 * ohs_th_radio.h
 *
 *  Created on: 31. 3. 2020
 *      Author: adam
 */

#ifndef OHS_TH_RADIO_H_
#define OHS_TH_RADIO_H_

#ifndef RADIO_DEBUG
#define RADIO_DEBUG 0
#endif

#if RADIO_DEBUG
#define DBG_RADIO(...) {chprintf(console, __VA_ARGS__);}
#else
#define DBG_RADIO(...)
#endif

/*
 * RFM69 thread
 */
static THD_WORKING_AREA(waRadioThread, 256);
static THD_FUNCTION(RadioThread, arg) {
  chRegSetThreadName(arg);
  msg_t resp;
  uint8_t nodeIndex, index;
  time_t timeNow;

  while (true) {
    // Wait for packet
    resp = chBSemWaitTimeout(&rfm69DataReceived, TIME_INFINITE);

    // Process packet
    if ((resp == MSG_OK) && (rfm69GetData() == RF69_RSLT_OK)) {
      DBG_RADIO("Radio from: %u, RSSI: %d, Data: ", rfm69Data.senderId, rfm69Data.rssi);
      for(uint8_t i = 0; i < rfm69Data.length; i++) { DBG_RADIO("%x, ", rfm69Data.data[i]); }
      DBG_RADIO("\r\n");

      // Do some logic on received packet
      switch(rfm69Data.data[0]) {
        case 'C': // Commands
          switch((uint8_t)rfm69Data.data[1]) {
            case NODE_CMD_PING: // Nodes should do periodic ping to stay alive/registered
              timeNow = getTimeUnixSec();
              index = 0; // Just any temp variable
              for (nodeIndex = 0; nodeIndex < NODE_SIZE; nodeIndex++) {
                if (node[nodeIndex].address == rfm69Data.senderId + RADIO_UNIT_OFFSET) {
                  node[nodeIndex].lastOK = timeNow;
                  index++;
                  DBG_RADIO("Node TS # %u updated\r\n", nodeIndex);
                }
              }
              // If not found, call this node to register.
              if (index == 0) {
                // This is needed for sleeping battery nodes, or they wont see reg. command.
                chThdSleepMilliseconds(5);
                resp = sendCmd(rfm69Data.senderId + RADIO_UNIT_OFFSET, NODE_CMD_REGISTRATION); // call this address to register
                DBG_RADIO("Unregistered node ping, resp: %d\r\n", resp);
              } else {
                DBG_RADIO("Node %d ping\r\n", rfm69Data.senderId);
              }
            break;
          }
          break;
        case 'R': // Registration
          index = 1; // Skip 'R'
          do {
            registrationEvent_t *outMsg = chPoolAlloc(&registration_pool);
            if (outMsg != NULL) {
              // node setting
              outMsg->address  = rfm69Data.senderId + RADIO_UNIT_OFFSET;
              outMsg->type     = (char)rfm69Data.data[index];
              outMsg->function = (char)rfm69Data.data[index+1];
              outMsg->number   = rfm69Data.data[index+2];
              outMsg->setting  = (rfm69Data.data[index+3] << 8) | (rfm69Data.data[index+4]);
              memcpy(&outMsg->name[0], &rfm69Data.data[index+5], NAME_LENGTH);  // Copy string

              msg_t msg = chMBPostTimeout(&registration_mb, (msg_t)outMsg, TIME_IMMEDIATE);
              if (msg != MSG_OK) {
                //DBG_RADIO("R-MB full %d\r\n", temp);
              }
            } else {
              pushToLogText("FR"); // Registration queue is full
            }
            index += REG_PACKET_SIZE;
          } while (index < rfm69Data.length);
          break;
        case 'K': // iButtons keys
          nodeIndex = getNodeIndex(rfm69Data.senderId + RADIO_UNIT_OFFSET, rfm69Data.data[0],
                                   rfm69Data.data[1], rfm69Data.data[2] - (rfm69Data.data[2] % 2));
          DBG_RADIO("Received Key, node index: %d\r\n", nodeIndex);
          // Node index found
          if (nodeIndex != DUMMY_NO_VALUE) {
            node[nodeIndex].lastOK = getTimeUnixSec(); // Update timestamp
            //  Node is enabled
            if (GET_NODE_ENABLED(node[nodeIndex].setting)) {
              node[nodeIndex].value = (float)checkKey(GET_NODE_GROUP(node[nodeIndex].setting),
                       (rfm69Data.data[2] % 2), &rfm69Data.data[3], rfm69Data.length - 4);
              // MQTT
              if (GET_NODE_MQTT(node[nodeIndex].setting)) {
                pushToMqtt(typeSensor, nodeIndex, functionValue);
              }
            } else {
              // log disabled remote nodes
              tmpLog[0] = 'N'; tmpLog[1] = 'F'; tmpLog[2] = rfm69Data.senderId + RADIO_UNIT_OFFSET;
              tmpLog[3] = rfm69Data.data[0]; tmpLog[4] = rfm69Data.data[1];
              tmpLog[5] = rfm69Data.data[2];  pushToLog(tmpLog, 6);
            }
          } else { // node not found
            chThdSleepMilliseconds(5);  // This is needed for sleeping battery nodes, or they wont see reg. command.
            resp = sendCmd(rfm69Data.senderId + RADIO_UNIT_OFFSET, NODE_CMD_REGISTRATION); // call this address to register
          }
          break;
        case 'S': // Sensor data
          index = 1; // skip 'S'
          do {
            sensorEvent_t *outMsg = chPoolAlloc(&sensor_pool);
            if (outMsg != NULL) {
              // node setting
              outMsg->address  = rfm69Data.senderId + RADIO_UNIT_OFFSET;
              outMsg->function = (char)rfm69Data.data[index];
              outMsg->number   = rfm69Data.data[index+1];
              floatConv.byte[0] = rfm69Data.data[index+2]; floatConv.byte[1] = rfm69Data.data[index+3];
              floatConv.byte[2] = rfm69Data.data[index+4]; floatConv.byte[3] = rfm69Data.data[index+5];
              outMsg->value = floatConv.val;

              msg_t msg = chMBPostTimeout(&sensor_mb, (msg_t)outMsg, TIME_IMMEDIATE);
              if (msg != MSG_OK) {
                //DBG_RADIO("S-MB full %d\r\n", temp);
              }
            } else {
              pushToLogText("FS"); // Sensor queue is full
            }
            index += SENSOR_PACKET_SIZE;
          } while (index < rfm69Data.length);
          break;
        case 'Z': // Zone
          index = 1; // Skip 'Z'
          DBG_RADIO("Zone");
          do {
            nodeIndex = rfm69Data.data[index] - 1; // Used here as temporary zone number
            DBG_RADIO(": %d", nodeIndex);
            // Zone allowed
            if ((nodeIndex >= HW_ZONES) && (nodeIndex < ALARM_ZONES)) {
              // Zone enabled
              if (GET_CONF_ZONE_ENABLED(conf.zone[nodeIndex])) {
                // Zone address and sender address match = zone is remote zone
                if (conf.zoneAddress[nodeIndex-HW_ZONES] == (rfm69Data.senderId) + RADIO_UNIT_OFFSET){
                  DBG_RADIO(" = %c", rfm69Data.data[index+1]);
                  // Check for valid lastEvent
                  if ((rfm69Data.data[index+1] == 'O') || (rfm69Data.data[index+1] == 'P') ||
                      (rfm69Data.data[index+1] == 'T')) {
                    zone[nodeIndex].lastEvent = rfm69Data.data[index+1];
                    if (rfm69Data.data[index+1] == 'O') {
                      zone[nodeIndex].lastOK = getTimeUnixSec();  // update current timestamp
                    } else {
                      zone[nodeIndex].lastPIR = getTimeUnixSec(); // update current timestamp
                    }
                  }
                } else {
                  // Log error just once
                  if (!GET_ZONE_ERROR(zone[nodeIndex].setting)) {
                    tmpLog[0] = 'Z'; tmpLog[1] = 'e'; tmpLog[2] = nodeIndex; tmpLog[3] = 'M'; pushToLog(tmpLog, 4);
                    SET_ZONE_ERROR(zone[nodeIndex].setting); // Set error flag
                  }
                }
              } else {
                // Log error just once
                if (!GET_ZONE_ERROR(zone[nodeIndex].setting)) {
                  tmpLog[0] = 'Z'; tmpLog[1] = 'e'; tmpLog[2] = nodeIndex; tmpLog[3] = 'N'; pushToLog(tmpLog, 4);
                  SET_ZONE_ERROR(zone[nodeIndex].setting); // Set error flag
                }
              } // else / Zone enabled
            } // Zone allowed
            index += ZONE_PACKET_SIZE;
          } while (index < rfm69Data.length);
          DBG_RADIO("\r\n");
          break;
      } // switch case

      // Data queue for sleeping nodes
      for (nodeIndex = 0; nodeIndex < NODE_SIZE; nodeIndex++) {
        if ((node[nodeIndex].address == (rfm69Data.senderId + RADIO_UNIT_OFFSET)) &&
            (node[nodeIndex].queue != NULL)) {
          // Do copy of node[].queue to rfm69Data.data, UMM in CCM not accessible by DMA
          memcpy(&rfm69Data.data[0], node[nodeIndex].queue, REG_PACKET_SIZE + 1);
          resp = sendData(node[nodeIndex].address, &rfm69Data.data[0], REG_PACKET_SIZE + 1);
          DBG_RADIO("Send Q 0x%x, resp %d\r\n", (const uint8_t *)node[nodeIndex].queue, resp);
          // Sent ?
          if (resp == 1) {
            umm_free(node[nodeIndex].queue);
            node[nodeIndex].queue = NULL;
          }
        }
      }
    } // received
  }
}

#endif /* OHS_TH_RADIO_H_ */
