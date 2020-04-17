/*
 * ohs_th_rs485.h
 *
 *  Created on: 22. 2. 2020
 *      Author: adam
 */

#ifndef OHS_TH_RS485_H_
#define OHS_TH_RS485_H_


/*
 * RS485 thread
 */
static THD_WORKING_AREA(waRS485Thread, 256);
static THD_FUNCTION(RS485Thread, arg) {
  chRegSetThreadName(arg);
  event_listener_t serialListener;
  eventmask_t evt;
  msg_t resp;
  RS485Msg_t rs485Msg;
  uint8_t pos;
  int8_t nodeIndex;

  // Register
  // ++ old chEvtRegister((event_source_t *)chnGetEventSource(&RS485D2.event), &serialListener, EVENT_MASK(0));
  chEvtRegister((event_source_t *)&RS485D2.event, &serialListener, EVENT_MASK(0));

  while (true) {
    evt = chEvtWaitAny(ALL_EVENTS);
    (void)evt;

    eventflags_t flags = chEvtGetAndClearFlags(&serialListener);
    chprintf(console, "RS485: %u, %u-%u\r\n", flags, RS485D2.trcState, RS485D2.ibHead);
    //resp = chBSemWait(&RS485D2.received);
    if (flags & RS485_MSG_RECEIVED){
      resp = rs485GetMsg(&RS485D2, &rs485Msg);
      chprintf(console, "RS485 received: %u, ", resp);
      chprintf(console, "from %u, ", rs485Msg.address);
      chprintf(console, "ctrl %u, ", rs485Msg.ctrl);
      chprintf(console, "length %u\r\n", rs485Msg.length);
      //chprintf(console, "ib %d, ob %d\r\n", RS485D2.ib[1], RS485D2.ob[1]);
      chprintf(console, "Data: ");
      for(uint8_t i = 0; i < rs485Msg.length; i++) {
        chprintf(console, "%u-%x, ", i, rs485Msg.data[i]);
      }
      chprintf(console, ".\r\n");
      //chThdSleepMilliseconds(100);
      /*
      for(uint8_t i = RS485_HEADER_SIZE; i < rs485Msg.length + RS485_HEADER_SIZE + RS485_CRC_SIZE; i++) {
        chprintf(console, "%d-%x, ", i,  RS485D2.ib[i]);
      }
      chprintf(console, "%x - %x\r\n", RS485D2.crc >> 8, RS485D2.crc & 0b11111111);
      */

      if (resp == MSG_OK) {
        if (rs485Msg.ctrl == RS485_FLAG_CMD) {
          switch(rs485Msg.length) {
            case NODE_CMD_PING: // Nodes should do periodic ping to stay alive/registered
              for (uint8_t nodeIndex=0; nodeIndex < NODE_SIZE; nodeIndex++) {
                if (node[nodeIndex].address == rs485Msg.address)
                  node[nodeIndex].last_OK = getTimeUnixSec();
              }
            break;
          }
        }
        if (rs485Msg.ctrl == RS485_FLAG_DTA) {
          switch(rs485Msg.data[0]) {
            case 'R': // Registration
              pos = 0;
              do {
                pos++; // Skip 'R'
                registration_t *outMsg = chPoolAlloc(&registration_pool);
                if (outMsg != NULL) {
                  // node setting
                  outMsg->address  = rs485Msg.address;
                  outMsg->type     = (char)rs485Msg.data[pos];
                  outMsg->function = (char)rs485Msg.data[pos+1];
                  outMsg->number   = rs485Msg.data[pos+2];
                  outMsg->setting  = (rs485Msg.data[pos+3] << 8) | (rs485Msg.data[pos+4]);
                  memcpy(&outMsg->name[0], &rs485Msg.data[pos+5], NAME_LENGTH);  // Copy string

                  msg_t msg = chMBPostTimeout(&registration_mb, (msg_t)outMsg, TIME_IMMEDIATE);
                  if (msg != MSG_OK) {
                    //chprintf(console, "R-MB full %d\r\n", temp);
                  }
                } else {
                  pushToLogText("FR"); // Registration queue is full
                }
                pos += REG_PACKET_SIZE;
              } while (pos < rs485Msg.length);
              break;
            case 'K': // iButtons keys
              nodeIndex = getNodeIndex(rs485Msg.address, rs485Msg.data[0],
                                       rs485Msg.data[1], rs485Msg.data[2] - (rs485Msg.data[2] % 2));
              chprintf(console, "Received Key, node index: %d\r\n", nodeIndex);
              // Node index found
              if (nodeIndex != -1) {
                node[nodeIndex].last_OK = getTimeUnixSec(); // Update timestamp
                //  Node is enabled
                if (GET_NODE_ENABLED(node[nodeIndex].setting)) {
                  checkKey(GET_NODE_GROUP(node[nodeIndex].setting), (rs485Msg.data[2] % 2), &rs485Msg.data[3]);
                } else {
                  // log disabled remote nodes
                  tmpLog[0] = 'N'; tmpLog[1] = 'F'; tmpLog[2] = rs485Msg.address;
                  tmpLog[3] = rs485Msg.data[2]; tmpLog[4] = rs485Msg.data[0];
                  tmpLog[5] = rs485Msg.data[1];  pushToLog(tmpLog, 6);
                }
              } else { // node not found
                chThdSleepMilliseconds(5);  // This is needed for sleeping battery nodes, or they wont see reg. command.
                resp = sendCmd(rs485Msg.address, NODE_CMD_REGISTRATION); // call this address to register
              }
              break;
            case 'S': // Sensor data
              pos = 0;
              do {
                sensor_t *outMsg = chPoolAlloc(&sensor_pool);
                if (outMsg != NULL) {
                  // node setting
                  outMsg->address  = rs485Msg.address;
                  outMsg->type     = (char)rs485Msg.data[pos];
                  outMsg->function = (char)rs485Msg.data[pos+1];
                  outMsg->number   = rs485Msg.data[pos+2];
                  floatConv.byte[0] = rs485Msg.data[pos+3]; floatConv.byte[1] = rs485Msg.data[pos+4];
                  floatConv.byte[2] = rs485Msg.data[pos+5]; floatConv.byte[3] = rs485Msg.data[pos+6];
                  outMsg->value = floatConv.val;

                  msg_t msg = chMBPostTimeout(&sensor_mb, (msg_t)outMsg, TIME_IMMEDIATE);
                  if (msg != MSG_OK) {
                    //chprintf(console, "S-MB full %d\r\n", temp);
                  }
                } else {
                  pushToLogText("FS"); // Sensor queue is full
                }
                pos += SENSOR_PACKET_SIZE;
              } while (pos < rs485Msg.length);
              break;
          } // switch case
        } // data
      }
    } // (flags & RS485_MSG_RECEIVED)
  }
}



#endif /* OHS_TH_RS485_H_ */
