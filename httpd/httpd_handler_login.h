/*
 * httpd_handler_login.h
 *
 *  Created on: 16. 1. 2026
 *      Author: vysocan
 */

#ifndef HTTPD_HANDLER_LOGIN_H_
#define HTTPD_HANDLER_LOGIN_H_


/*
 * @brief HTTP login page handler
 * @param chp Pointer to the output stream
 */
static void fs_open_custom_login(BaseSequentialStream *chp) {
  // Information table
  chprintf(chp, "%s%s%s", HTML_tr_td, TEXT_User, HTML_e_td_td);
  printTextInputWMin(chp, 'u', "", NAME_LENGTH, MIN_PASS_LNEGTH);
  chprintf(chp, "%s%s%s", HTML_e_td_e_tr_tr_td, TEXT_Password, HTML_e_td_td);
  printPassInput(chp, 'p', "", NAME_LENGTH, MIN_PASS_LNEGTH);
  chprintf(chp, "%s%s", HTML_e_td_e_tr, HTML_e_table);
  // Buttons
  chprintf(chp, "%s", HTML_Submit);
}

/*
 * @brief HTTP login POST handler
 * @param postDataP Pointer to POST data string
 * @param connection Connection pointer for authentication
 */
static void httpd_post_custom_login(char **postDataP, void *connection) {
  uint16_t number = 1, valueLen = 0; // number used to hold state of authentication
  char name[3];
  bool repeat;
  char *valueP;

  do {
    repeat = getPostData(postDataP, &name[0], sizeof(name), &valueP, &valueLen);
    DBG_HTTP("Parse: %s = '%.*s' (%u)\r\n", name, valueLen, valueP, valueLen);
    switch(name[0]) {
      case 'u': // user
        number = strcmp(valueP, conf.user);
      break;
      case 'p': // password
        if (!number &&(!strcmp(valueP, conf.password))) {
          verifiedConn = connection;
          authorizedConn.id = STM32_UUID[0] + rand();
          authorizedConn.conn = (void *)connection;
        } else {
          chsnprintf(httpAlert.msg, HTTP_ALERT_MSG_SIZE, "User or password not valid!");
          httpAlert.type = ALERT_ERROR;
        }
      break;
    }
  } while (repeat);
}


#endif /* HTTPD_HANDLER_LOGIN_H_ */
