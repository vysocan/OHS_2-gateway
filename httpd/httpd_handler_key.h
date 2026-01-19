/*
 * httpd_handler_key.h
 *
 *  Created on: 16. 1. 2026
 *      Author: vysocan
 */

#ifndef HTTPD_HANDLER_KEY_H_
#define HTTPD_HANDLER_KEY_H_


/*
 * @brief HTTP key page handler
 * @param chp Pointer to the output stream
 */
static void fs_open_custom_key(BaseSequentialStream *chp) {
  chprintf(chp, "%s#", HTML_tr_th);
  chprintf(chp, "%s%s", HTML_e_th_th, TEXT_User);
  chprintf(chp, "%s%s", HTML_e_th_th, TEXT_On);
  chprintf(chp, "%s%s%s\r\n", HTML_e_th_th, TEXT_Hash, HTML_e_th_e_tr);
  // Information table
  for (uint8_t i = 0; i < KEYS_SIZE; i++) {
    chprintf(chp, "%s%u.%s", HTML_tr_td, i + 1, HTML_e_td_td);
    if (conf.key[i].contact == DUMMY_NO_VALUE) chprintf(chp, "%s", NOT_SET);
    else chprintf(chp, "%s", conf.contact[conf.key[i].contact].name);
    chprintf(chp, "%s", HTML_e_td_td);
    printOkNok(chp, GET_CONF_KEY_ENABLED(conf.key[i].setting));
    chprintf(chp, "%s", HTML_e_td_td);
    uint32Conv.val = conf.key[i].value;
    printKey(chp, (uint8_t *)&uint32Conv.byte[0]);
    chprintf(chp, "%s", HTML_e_td_e_tr);
  }
  chprintf(chp, "%s", HTML_e_table);
  chprintf(chp, "%s", HTML_table);
  chprintf(chp, "%s%s%s", HTML_tr_td, TEXT_Key, HTML_e_td_td);
  chprintf(chp, "%sP%s", HTML_select_submit, HTML_e_tag);
  for (uint8_t i = 0; i < KEYS_SIZE; i++) {
    chprintf(chp, "%s%u", HTML_option, i);
    if (webKey == i) { chprintf(chp, "%s", HTML_selected); }
    else             { chprintf(chp, "%s", HTML_e_tag); }
    chprintf(chp, "%u.%s", i + 1, HTML_e_option);
  }
  chprintf(chp, "%s%s", HTML_e_select, HTML_e_td_e_tr_tr_td);
  chprintf(chp, "%s%s", TEXT_User, HTML_e_td_td);
  chprintf(chp, "%sc%s", HTML_select, HTML_e_tag);
  for (uint8_t i = 0; i <= CONTACTS_SIZE; i++) {
    if (i < CONTACTS_SIZE) {
      chprintf(chp, "%s%u", HTML_option, i);
      if (conf.key[webKey].contact == i) { chprintf(chp, "%s", HTML_selected); }
      else                               { chprintf(chp, "%s", HTML_e_tag); }
      chprintf(chp, "%u. %s%s", i + 1, conf.contact[i].name, HTML_e_option);
    } else {
      chprintf(chp, "%s%u", HTML_option, DUMMY_NO_VALUE);
      if (conf.key[webKey].contact == DUMMY_NO_VALUE) { chprintf(chp, "%s", HTML_selected); }
      else                                            { chprintf(chp, "%s", HTML_e_tag); }
      chprintf(chp, "%s%s", NOT_SET, HTML_e_option);
    }
  }
  chprintf(chp, "%s%s %s%s", HTML_e_td_e_tr_tr_td, TEXT_Key, TEXT_is, HTML_e_td_td);
  printOnOffButton(chp, "0", GET_CONF_KEY_ENABLED(conf.key[webKey].setting));
  chprintf(chp, "%s%s%s", HTML_e_td_e_tr_tr_td, TEXT_Hash, HTML_e_td_td);
  chprintf(chp, "%s%u%s%u", HTML_t_tag_1, KEY_LENGTH * 2, HTML_s_tag_2, KEY_LENGTH * 2);
  chprintf(chp, "%sk%s", HTML_s_tag_3, HTML_m_tag);
  uint32Conv.val = conf.key[webKey].value;
  printKey(chp, (uint8_t *)&uint32Conv.byte[0]);
  chprintf(chp, "%s%k%s", HTML_id_tag, HTML_e_tag);
  chprintf(chp, "%s%s", HTML_e_td_e_tr, HTML_e_table);
  // Buttons
  chprintf(chp, "%s%s", HTML_Apply, HTML_Save);
}

/*
 * @brief HTTP key POST handler
 * @param postDataP Pointer to POST data string
 */
static void httpd_post_custom_key(char **postDataP) {
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
        if (number != webKey) { webKey = number; repeat = 0; }
      break;
      case 'c': // Contact ID
        conf.key[webKey].contact = strtol(valueP, NULL, 10);
      break;
      case 'k': // key
        safeStrtoul(valueP, &conf.key[webKey].value , 16); // as unsigned long int
      break;
      case '0' ... '7': // Handle all single radio buttons for settings
        if (valueP[0] == '0') conf.key[webKey].setting &= ~(1 << (name[0]-48));
        else                  conf.key[webKey].setting |=  (1 << (name[0]-48));
      break;
      case 'e': // save
        writeToBkpSRAM((uint8_t*)&conf, sizeof(config_t), 0);
      break;
    }
  } while (repeat);
}


#endif /* HTTPD_HANDLER_KEY_H_ */
