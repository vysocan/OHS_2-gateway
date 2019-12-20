/*
 * ohs_functions.h
 *
 *  Created on: 16. 12. 2019
 *      Author: adam
 */

#ifndef OHS_FUNCTIONS_H_
#define OHS_FUNCTIONS_H_

// Send data to node
int8_t sendData(uint8_t address, const uint8_t *data, uint8_t length){
  RS485Msg_t rs485Data;

  chprintf(console, "RS485 Send data to address: %d\r\n", address);
  rs485Data.address = address;
  rs485Data.length = length;
  memcpy(&rs485Data.data[0], data, length);
  for(uint8_t ii = 0; ii < length; ii++) {
    chprintf(console, "%d-%x, ", ii, rs485Data.data[ii]);
  } chprintf(console, "\r\n");
  return rs485SendMsgWithACK(&RS485D2, &rs485Data, 5);
}

// Send a command to node
int8_t sendCmd(uint8_t address, uint8_t command) {
  RS485Cmd_t rs485Cmd;

  chprintf(console, "RS485 Send cmd: %d to address: %d\r\n", command, address);
  rs485Cmd.address = address;
  rs485Cmd.length = command;
  return rs485SendCmdWithACK(&RS485D2, &rs485Cmd, 3);
}




#endif /* OHS_FUNCTIONS_H_ */
