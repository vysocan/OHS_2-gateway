/*
 * httpd_handler_user.h
 *
 *  Created on: 16. 1. 2026
 *      Author: vysocan
 */

#ifndef HTTPD_HANDLER_USER_H_
#define HTTPD_HANDLER_USER_H_


/*
 * @brief HTTP user page handler
 * @param chp Pointer to the output stream
 */
static void fs_open_custom_user(BaseSequentialStream *chp) {
  uint16_t logAddress;
  chprintf(chp, "%s#", html_tr_th);
  chprintf(chp, "%s%s", html_e_th_th, text_Name);
  chprintf(chp, "%s%s", html_e_th_th, text_On);
  chprintf(chp, "%s%s", html_e_th_th, text_Number);
  chprintf(chp, "%s%s", html_e_th_th, text_Email);
  chprintf(chp, "%s%s", html_e_th_th, text_Global);
  chprintf(chp, "%s%s(s)", html_e_th_th, text_Key);
  chprintf(chp, "%s%s%s\r\n", html_e_th_th, text_Group, html_e_th_e_tr);
  // Information table
  for (uint8_t i = 0; i < CONTACTS_SIZE; i++) {
    chprintf(chp, "%s%u.%s", html_tr_td, i + 1, html_e_td_td);
    chprintf(chp, "%s%s", conf.contact[i].name, html_e_td_td);
    printOkNok(chp, GET_CONF_CONTACT_ENABLED(conf.contact[i].setting));
    chprintf(chp, "%s%s", html_e_td_td, conf.contact[i].phone);
    chprintf(chp, "%s%s", html_e_td_td, conf.contact[i].email);
    chprintf(chp, "%s", html_e_td_td);
    printOkNok(chp, GET_CONF_CONTACT_IS_GLOBAL(conf.contact[i].setting));
    chprintf(chp, "%s", html_e_td_td);
    logAddress = 0; // Just temp. var.
    for (uint8_t j = 0; j < KEYS_SIZE; j++) {
      if (conf.key[j].contact == i) {
        logAddress++;
      }
    }
    chprintf(chp, "%u%s", logAddress, html_e_td_td);
    if (!GET_CONF_CONTACT_IS_GLOBAL(conf.contact[i].setting))
      printGroup(chp, GET_CONF_CONTACT_GROUP(conf.contact[i].setting));
    else chprintf(chp, "%s", text_all);
    chprintf(chp, "%s", html_e_td_e_tr);
  }
  chprintf(chp, "%s", html_e_table);
  chprintf(chp, "%s", html_table);
  chprintf(chp, "%s%s%s", html_tr_td, text_User, html_e_td_td);
  chprintf(chp, "%sP%s", html_select_submit, html_e_tag);
  for (uint8_t i = 0; i < CONTACTS_SIZE; i++) {
    chprintf(chp, "%s%u", html_option, i);
    if (webContact == i) { chprintf(chp, "%s", html_selected); }
    else                 { chprintf(chp, "%s", html_e_tag); }
    chprintf(chp, "%u. %s%s", i + 1, conf.contact[i].name, html_e_option);
  }
  chprintf(chp, "%s%s", html_e_select, html_e_td_e_tr_tr_td);
  chprintf(chp, "%s%s", text_Name, html_e_td_td);
  printTextInput(chp, 'n', conf.contact[webContact].name, NAME_LENGTH);
  chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_User, text_is, html_e_td_td);
  printOnOffButton(chp, "0", GET_CONF_CONTACT_ENABLED(conf.contact[webContact].setting));
  chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, text_Number, html_e_td_td);
  printTextInput(chp, 'p', conf.contact[webContact].phone, PHONE_LENGTH);
  chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, text_Email, html_e_td_td);
  printTextInput(chp, 'm', conf.contact[webContact].email, EMAIL_LENGTH);
  chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, text_Global, html_e_td_td);
  printOnOffButtonWJS(chp, "5", GET_CONF_CONTACT_IS_GLOBAL(conf.contact[webContact].setting), 1, 0b10);
  chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, text_Group, html_e_td_td);
  selectGroup(chp, GET_CONF_CONTACT_GROUP(conf.contact[webContact].setting), 'g');
  chprintf(chp, "%s%s", html_e_td_e_tr, html_e_table);
  // JavaScript
  chprintf(chp, "%s%s%s", html_script, JSContact, html_e_script);
  // Buttons
  chprintf(chp, "%s%s", html_Apply, html_Save);
}

/*
 * @brief HTTP user POST handler
 * @param postDataP Pointer to POST data string
 */
static void httpd_post_custom_user(char **postDataP) {
  uint16_t number, valueLen = 0;
  char name[3];
  bool repeat;
  char *valueP;

  do {
    repeat = getPostData(postDataP, &name[0], sizeof(name), &valueP, &valueLen);
    DBG_HTTP("Parse: %s = '%.*s' (%u)\r\n", name, valueLen, valueP, valueLen);
    switch(name[0]) {
      case 'P': // select
        number = strtol(valueP, NULL, 10);
        if (number != webContact) { webContact = number; repeat = 0; }
      break;
      case 'n': // name
        strncpy(conf.contact[webContact].name, valueP, LWIP_MIN(valueLen, NAME_LENGTH - 1));
        conf.contact[webContact].name[LWIP_MIN(valueLen, NAME_LENGTH - 1)] = 0;
        break;
      case 'p': // phone number
        strncpy(conf.contact[webContact].phone, valueP, LWIP_MIN(valueLen, PHONE_LENGTH - 1));
        conf.contact[webContact].phone[LWIP_MIN(valueLen, PHONE_LENGTH - 1)] = 0;
        break;
      case 'm': // email
        strncpy(conf.contact[webContact].email, valueP, LWIP_MIN(valueLen, EMAIL_LENGTH - 1));
        conf.contact[webContact].email[LWIP_MIN(valueLen, EMAIL_LENGTH - 1)] = 0;
      break;
      case '0' ... '7': // Handle all single radio buttons for settings
        if (valueP[0] == '0') conf.contact[webContact].setting &= ~(1 << (name[0]-48));
        else                  conf.contact[webContact].setting |=  (1 << (name[0]-48));
      break;
      case 'g': // group
        number = strtol(valueP, NULL, 10);
        SET_CONF_CONTACT_GROUP(conf.contact[webContact].setting, number);
      break;
      case 'e': // save
        writeToBkpSRAM((uint8_t*)&conf, sizeof(config_t), 0);
      break;
    }
  } while (repeat);
}


#endif /* HTTPD_HANDLER_USER_H_ */
