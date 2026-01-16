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
  chprintf(chp, "%s#", html_tr_th);
  chprintf(chp, "%s%s", html_e_th_th, text_Address);
  chprintf(chp, "%s%s", html_e_th_th, text_On);
  chprintf(chp, "%s%s", html_e_th_th, text_MQTT);
  chprintf(chp, "%s%s", html_e_th_th, text_HAD);
  chprintf(chp, "%s%s %s", html_e_th_th, text_Last, text_message);
  chprintf(chp, "%s%s", html_e_th_th, text_Queued);
  chprintf(chp, "%s%s", html_e_th_th, text_Type);
  chprintf(chp, "%s%s", html_e_th_th, text_Function);
  chprintf(chp, "%s%s", html_e_th_th, text_Value);
  chprintf(chp, "%s%s%s\r\n", html_e_th_th, text_Group, html_e_th_e_tr);
  // Information table
  for (uint8_t i = 0; i < NODE_SIZE; i++) {
    if (node[i].address != 0) {
      chprintf(chp, "%s%u.%s", html_tr_td, i + 1, html_e_td_td);
      printNodeAddress(chp, node[i].address, node[i].type, node[i].function, node[i].number, true);
      chprintf(chp, "%s", html_e_td_td);
      printOkNok(chp, GET_NODE_ENABLED(node[i].setting));
      chprintf(chp, "%s", html_e_td_td);
      printOkNok(chp, GET_NODE_MQTT(node[i].setting));
      chprintf(chp, "%s", html_e_td_td);
      printOkNok(chp, GET_NODE_MQTT_HAD(node[i].setting));
      chprintf(chp, "%s", html_e_td_td);
      printFrmTimestamp(chp, &node[i].lastOK);
      chprintf(chp, "%s", html_e_td_td);
      if (node[i].queue) number = 1;
      else number = 0;
      printOkNok(chp, number); // queued
      chprintf(chp, "%s", html_e_td_td);
      chprintf(chp, "%s", getNodeTypeString(node[i].type));
      chprintf(chp, "%s", html_e_td_td);
      printNodeFunction(chp, node[i].function);
      chprintf(chp, "%s", html_e_td_td);
      printNodeValue(chp, i);
      chprintf(chp, "%s", html_e_td_td);
      printGroup(chp, GET_NODE_GROUP(node[i].setting));
      chprintf(chp, "%s", html_e_td_e_tr);
    }
  }
  chprintf(chp, "%s", html_e_table);
  chprintf(chp, "%s", html_table);
  chprintf(chp, "%s%s%s", html_tr_td, text_Node, html_e_td_td);
  chprintf(chp, "%sP%s", html_select_submit, html_e_tag);
  for (uint8_t i = 0; i < NODE_SIZE; i++) {
    if (node[i].address != 0) {
      chprintf(chp, "%s%u", html_option, i);
      if (webNode == i) { chprintf(chp, "%s", html_selected); }
      else              { chprintf(chp, "%s", html_e_tag); }
      chprintf(chp, "%u. %s%s", i + 1, node[i].name, html_e_option);
    }
  }
  chprintf(chp, "%s%s", html_e_select, html_e_td_e_tr_tr_td);
  chprintf(chp, "%s%s", text_Name, html_e_td_td);
  printTextInput(chp, 'n', node[webNode].name, NAME_LENGTH);
  chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, text_Address, html_e_td_td);
  printNodeAddress(chp, node[webNode].address, node[webNode].type, node[webNode].function, node[webNode].number, true);
  chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, text_Type, html_e_td_td);
  chprintf(chp, "%s", getNodeTypeString(node[webNode].type));
  chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, text_Function, html_e_td_td);
  printNodeFunction(chp, node[webNode].function);
  chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_Node, text_is, html_e_td_td);
  printOnOffButton(chp, "0", GET_NODE_ENABLED(node[webNode].setting));
  chprintf(chp, "%s%s %s %s%s", html_e_td_e_tr_tr_td, text_MQTT, text_HA, text_Discovery, html_e_td_td);
  printOnOffButton(chp, "6", GET_NODE_MQTT_HAD(node[webNode].setting));
  chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_MQTT, text_publish, html_e_td_td);
  printOnOffButton(chp, "7", GET_NODE_MQTT(node[webNode].setting));
  chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, text_Group, html_e_td_td);
  selectGroup(chp, GET_NODE_GROUP(node[webNode].setting), 'g');
  chprintf(chp, "%s%s", html_e_td_e_tr, html_e_table);
  // Buttons
  chprintf(chp, "%s%s", html_Apply, html_Reregister);
}


#endif /* HTTPD_HANDLER_NODE_H_ */
