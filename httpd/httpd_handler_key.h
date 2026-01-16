/*
 * httpd_handler_key.h
 *
 *  Created on: 16. 1. 2026
 *      Author: vysocan
 */

#ifndef HTTPD_HANDLER_KEY_H_
#define HTTPD_HANDLER_KEY_H_


static void fs_open_custom_key(BaseSequentialStream *chp) {
  chprintf(chp, "%s#", html_tr_th);
  chprintf(chp, "%s%s", html_e_th_th, text_User);
  chprintf(chp, "%s%s", html_e_th_th, text_On);
  chprintf(chp, "%s%s%s\r\n", html_e_th_th, text_Hash, html_e_th_e_tr);
  // Information table
  for (uint8_t i = 0; i < KEYS_SIZE; i++) {
    chprintf(chp, "%s%u.%s", html_tr_td, i + 1, html_e_td_td);
    if (conf.key[i].contact == DUMMY_NO_VALUE) chprintf(chp, "%s", NOT_SET);
    else chprintf(chp, "%s", conf.contact[conf.key[i].contact].name);
    chprintf(chp, "%s", html_e_td_td);
    printOkNok(chp, GET_CONF_KEY_ENABLED(conf.key[i].setting));
    chprintf(chp, "%s", html_e_td_td);
    uint32Conv.val = conf.key[i].value;
    printKey(chp, (uint8_t *)&uint32Conv.byte[0]);
    chprintf(chp, "%s", html_e_td_e_tr);
  }
  chprintf(chp, "%s", html_e_table);
  chprintf(chp, "%s", html_table);
  chprintf(chp, "%s%s%s", html_tr_td, text_Key, html_e_td_td);
  chprintf(chp, "%sP%s", html_select_submit, html_e_tag);
  for (uint8_t i = 0; i < KEYS_SIZE; i++) {
    chprintf(chp, "%s%u", html_option, i);
    if (webKey == i) { chprintf(chp, "%s", html_selected); }
    else             { chprintf(chp, "%s", html_e_tag); }
    chprintf(chp, "%u.%s", i + 1, html_e_option);
  }
  chprintf(chp, "%s%s", html_e_select, html_e_td_e_tr_tr_td);
  chprintf(chp, "%s%s", text_User, html_e_td_td);
  chprintf(chp, "%sc%s", html_select, html_e_tag);
  for (uint8_t i = 0; i <= CONTACTS_SIZE; i++) {
    if (i < CONTACTS_SIZE) {
      chprintf(chp, "%s%u", html_option, i);
      if (conf.key[webKey].contact == i) { chprintf(chp, "%s", html_selected); }
      else                               { chprintf(chp, "%s", html_e_tag); }
      chprintf(chp, "%u. %s%s", i + 1, conf.contact[i].name, html_e_option);
    } else {
      chprintf(chp, "%s%u", html_option, DUMMY_NO_VALUE);
      if (conf.key[webKey].contact == DUMMY_NO_VALUE) { chprintf(chp, "%s", html_selected); }
      else                                            { chprintf(chp, "%s", html_e_tag); }
      chprintf(chp, "%s%s", NOT_SET, html_e_option);
    }
  }
  chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_Key, text_is, html_e_td_td);
  printOnOffButton(chp, "0", GET_CONF_KEY_ENABLED(conf.key[webKey].setting));
  chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, text_Hash, html_e_td_td);
  chprintf(chp, "%s%u%s%u", html_t_tag_1, KEY_LENGTH * 2, html_s_tag_2, KEY_LENGTH * 2);
  chprintf(chp, "%sk%s", html_s_tag_3, html_m_tag);
  uint32Conv.val = conf.key[webKey].value;
  printKey(chp, (uint8_t *)&uint32Conv.byte[0]);
  chprintf(chp, "%s%k%s", html_id_tag, html_e_tag);
  chprintf(chp, "%s%s", html_e_td_e_tr, html_e_table);
  // Buttons
  chprintf(chp, "%s%s", html_Apply, html_Save);
}


#endif /* HTTPD_HANDLER_KEY_H_ */
