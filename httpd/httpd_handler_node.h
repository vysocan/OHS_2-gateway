/*
 * httpd_handler_node.h
 *
 *  Created on: 16. 1. 2026
 *      Author: vysocan
 */

#ifndef HTTPD_HANDLER_NODE_H_
#define HTTPD_HANDLER_NODE_H_


/*
 * @brief HTTP node page handler
 * @param chp Pointer to the output stream
 */
static void fs_open_custom_node(BaseSequentialStream *chp) {
  uint8_t number;
  chprintf(chp, "%s#", HTML_tr_th);
  chprintf(chp, "%s%s", HTML_e_th_th, TEXT_Address);
  chprintf(chp, "%s%s", HTML_e_th_th, TEXT_On);
  chprintf(chp, "%s%s", HTML_e_th_th, TEXT_MQTT);
  chprintf(chp, "%s%s", HTML_e_th_th, TEXT_HAD);
  chprintf(chp, "%s%s %s", HTML_e_th_th, TEXT_Last, TEXT_message);
  chprintf(chp, "%s%s", HTML_e_th_th, TEXT_Queued);
  chprintf(chp, "%s%s", HTML_e_th_th, TEXT_Type);
  chprintf(chp, "%s%s", HTML_e_th_th, TEXT_Function);
  chprintf(chp, "%s%s", HTML_e_th_th, TEXT_Value);
  chprintf(chp, "%s%s%s\r\n", HTML_e_th_th, TEXT_Group, HTML_e_th_e_tr);
  // Information table
  for (uint8_t i = 0; i < NODE_SIZE; i++) {
    if (node[i].address != 0) {
      chprintf(chp, "%s%u.%s", HTML_tr_td, i + 1, HTML_e_td_td);
      printNodeAddress(chp, node[i].address, node[i].type, node[i].function, node[i].number, true);
      chprintf(chp, "%s", HTML_e_td_td);
      printOkNok(chp, GET_NODE_ENABLED(node[i].setting));
      chprintf(chp, "%s", HTML_e_td_td);
      printOkNok(chp, GET_NODE_MQTT(node[i].setting));
      chprintf(chp, "%s", HTML_e_td_td);
      printOkNok(chp, GET_NODE_MQTT_HAD(node[i].setting));
      chprintf(chp, "%s", HTML_e_td_td);
      printFrmTimestamp(chp, &node[i].lastOK);
      chprintf(chp, "%s", HTML_e_td_td);
      if (node[i].queue) number = 1;
      else number = 0;
      printOkNok(chp, number); // queued
      chprintf(chp, "%s", HTML_e_td_td);
      chprintf(chp, "%s", getNodeTypeString(node[i].type));
      chprintf(chp, "%s", HTML_e_td_td);
      printNodeFunction(chp, node[i].function);
      chprintf(chp, "%s", HTML_e_td_td);
      printNodeValue(chp, i);
      chprintf(chp, "%s", HTML_e_td_td);
      printGroup(chp, GET_NODE_GROUP(node[i].setting));
      chprintf(chp, "%s", HTML_e_td_e_tr);
    }
  }
  chprintf(chp, "%s", HTML_e_table);
  chprintf(chp, "%s", HTML_table);
  chprintf(chp, "%s%s%s", HTML_tr_td, TEXT_Node, HTML_e_td_td);
  chprintf(chp, "%sP%s", HTML_select_submit, HTML_e_tag);
  for (uint8_t i = 0; i < NODE_SIZE; i++) {
    if (node[i].address != 0) {
      chprintf(chp, "%s%u", HTML_option, i);
      if (webNode == i) { chprintf(chp, "%s", HTML_selected); }
      else              { chprintf(chp, "%s", HTML_e_tag); }
      chprintf(chp, "%u. %s%s", i + 1, node[i].name, HTML_e_option);
    }
  }
  chprintf(chp, "%s%s", HTML_e_select, HTML_e_td_e_tr_tr_td);
  chprintf(chp, "%s%s", TEXT_Name, HTML_e_td_td);
  printTextInput(chp, 'n', node[webNode].name, NAME_LENGTH);
  chprintf(chp, "%s%s%s", HTML_e_td_e_tr_tr_td, TEXT_Address, HTML_e_td_td);
  printNodeAddress(chp, node[webNode].address, node[webNode].type, node[webNode].function, node[webNode].number, true);
  chprintf(chp, "%s%s%s", HTML_e_td_e_tr_tr_td, TEXT_Type, HTML_e_td_td);
  chprintf(chp, "%s", getNodeTypeString(node[webNode].type));
  chprintf(chp, "%s%s%s", HTML_e_td_e_tr_tr_td, TEXT_Function, HTML_e_td_td);
  printNodeFunction(chp, node[webNode].function);
  if ((node[webNode].type == 'K') && (node[webNode].function == 'f')) {
    chprintf(chp, "%s%s%s", HTML_e_td_e_tr_tr_td, TEXT_Fingerprint, HTML_e_td_td);
    printIntInput(chp, 'r', webEnroll, 2, 1, KEYS_SIZE);
  }
  chprintf(chp, "%s%s %s%s", HTML_e_td_e_tr_tr_td, TEXT_Node, TEXT_is, HTML_e_td_td);
  printOnOffButton(chp, "0", GET_NODE_ENABLED(node[webNode].setting));
  chprintf(chp, "%s%s %s %s%s", HTML_e_td_e_tr_tr_td, TEXT_MQTT, TEXT_HA, TEXT_Discovery, HTML_e_td_td);
  printOnOffButton(chp, "6", GET_NODE_MQTT_HAD(node[webNode].setting));
  chprintf(chp, "%s%s %s%s", HTML_e_td_e_tr_tr_td, TEXT_MQTT, TEXT_publish, HTML_e_td_td);
  printOnOffButton(chp, "7", GET_NODE_MQTT(node[webNode].setting));
  chprintf(chp, "%s%s%s", HTML_e_td_e_tr_tr_td, TEXT_Group, HTML_e_td_td);
  selectGroup(chp, GET_NODE_GROUP(node[webNode].setting), 'g');
  chprintf(chp, "%s%s", HTML_e_td_e_tr, HTML_e_table);
  // Buttons
  chprintf(chp, "%s%s", HTML_Apply, HTML_Reregister);
  if ((node[webNode].type == 'K') && (node[webNode].function == 'f')) {
    chprintf(chp, "%s", HTML_Enroll);
    chprintf(chp, "%s", HTML_Delete);
  }
}

/*
 * @brief HTTP node POST handler
 * @param postDataP Pointer to POST data string
 */
static void httpd_post_custom_node(char **postDataP) {
  uint16_t number, valueLen = 0;
  int8_t resp;
  char name[3];
  uint8_t message[REG_PACKET_SIZE + 1];
  bool repeat;
  char *valueP;

  do {
    repeat = getPostData(postDataP, &name[0], sizeof(name), &valueP, &valueLen);
    DBG_HTTP("Parse: %s = '%.*s' (%u)\r\n", name, valueLen, valueP, valueLen);
    switch(name[0]) {
      case 'P': // select
        number = strtol(valueP, NULL, 10);
        if (number != webNode) { webNode = number; repeat = 0; }
      break;
      case 'R': // Re-registration
        resp = sendCmd(RADIO_UNIT_OFFSET, NODE_CMD_REGISTRATION); // Broadcast to register
      break;
      case 'A': // Apply
        message[0] = 'R';
        message[1] = (uint8_t)node[webNode].type;
        message[2] = (uint8_t)node[webNode].function;
        message[3] = node[webNode].number;
        message[4] = (uint8_t)((node[webNode].setting >> 8) & 0b11111111);;
        message[5] = (uint8_t)(node[webNode].setting & 0b11111111);
        memcpy(&message[6], node[webNode].name, NAME_LENGTH);
        resp = sendData(node[webNode].address, message, REG_PACKET_SIZE + 1);
        // Queue data if no response
        if (resp != 1) {
          // Not queued, allocate new
          if (node[webNode].queue == NULL) {
            node[webNode].queue = umm_malloc(REG_PACKET_SIZE + 1);
          }
          // Copy new message to queue pointer
          if (node[webNode].queue != NULL) {
            memcpy(node[webNode].queue, &message[0], REG_PACKET_SIZE + 1);
          }
          // Show warning
          chsnprintf(httpAlert.msg, HTTP_ALERT_MSG_SIZE, "Packet not delivered, but queued.");
          httpAlert.type = ALERT_WARN;
        }
      break;
      case 'n': // name
        // Calculate resp for MQTT
        if (GET_NODE_ENABLED(node[webNode].setting) &&
            GET_NODE_MQTT(node[webNode].setting)) {
          if (strlen(node[webNode].name) != valueLen) resp = 1;
          else resp = strncmp(node[webNode].name, valueP, LWIP_MIN(valueLen, NAME_LENGTH - 1));
        } else resp = 0;
        // Replace name
        strncpy(node[webNode].name, valueP, LWIP_MIN(valueLen, NAME_LENGTH - 1));
        node[webNode].name[LWIP_MIN(valueLen, NAME_LENGTH - 1)] = 0;
        // MQTT
        if (resp) {
          pushToMqtt(typeSensor, webNode, functionName);
          // HAD
          if (GET_NODE_MQTT_HAD(node[webNode].setting)) {
            pushToMqttHAD(typeSensor, webNode, functionHAD, 1);
          }
        }
      break;
      case '0' ... '7': // Handle all single radio buttons for settings
        if (valueP[0] == '0') node[webNode].setting &= ~(1 << (name[0]-48));
        else                  node[webNode].setting |=  (1 << (name[0]-48));
      break;
      case 'g': // group
        number = strtol(valueP, NULL, 10);
        SET_NODE_GROUP(node[webNode].setting, number);
      break;
      case 'r': // enroll number
        webEnroll = strtol(valueP, NULL, 10);
        break;
      case 'E': // enroll fingerprint
        message[0] = 'F';
        message[1] = 'E';
        message[2] = (uint8_t)webEnroll;
        resp = sendData(node[webNode].address, message, 3);
        break;
      case 'D': // delete fingerprint
        message[0] = 'F';
        message[1] = 'D';
        message[2] = (uint8_t) webEnroll;
        resp = sendData(node[webNode].address, message, 3);
        break;
      case 'e': // save
        writeToBkpSRAM((uint8_t*)&conf, sizeof(config_t), 0);
      break;
    }
  } while (repeat);
}


#endif /* HTTPD_HANDLER_NODE_H_ */
