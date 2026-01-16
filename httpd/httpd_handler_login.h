/*
 * httpd_handler_login.h
 *
 *  Created on: 16. 1. 2026
 *      Author: vysocan
 */

#ifndef HTTPD_HANDLER_LOGIN_H_
#define HTTPD_HANDLER_LOGIN_H_


static void fs_open_custom_login(BaseSequentialStream *chp) {
  // Information table
  chprintf(chp, "%s%s%s", html_tr_td, text_User, html_e_td_td);
  printTextInputWMin(chp, 'u', "", NAME_LENGTH, MIN_PASS_LNEGTH);
  chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, text_Password, html_e_td_td);
  printPassInput(chp, 'p', "", NAME_LENGTH, MIN_PASS_LNEGTH);
  chprintf(chp, "%s%s", html_e_td_e_tr, html_e_table);
  // Buttons
  chprintf(chp, "%s", html_Submit);
}


#endif /* HTTPD_HANDLER_LOGIN_H_ */
