/*
 * ohs_httpdhandler.h
 *
 *  Created on: 6. 12. 2019
 *      Author: vysocan
 */

#ifndef OHS_HTTPD_HANDLER_H_
#define OHS_HTTPD_HANDLER_H_

#include "lwip/opt.h"
#include "lwip/apps/fs.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include <stdio.h>
#include <string.h>
#include "memstreams.h"
#include "ohs_http_const.h"
#include "ohs_http_print.h"

#if !LWIP_HTTPD_CUSTOM_FILES
#error This needs LWIP_HTTPD_CUSTOM_FILES
#endif
#if !LWIP_HTTPD_FILE_EXTENSION
#error This needs LWIP_HTTPD_FILE_EXTENSION
#endif
#if !LWIP_HTTPD_DYNAMIC_HEADERS
#error This needs LWIP_HTTPD_DYNAMIC_HEADERS
#endif
#if !LWIP_HTTPD_SUPPORT_POST
#error This needs LWIP_HTTPD_SUPPORT_POST
#endif
#if !LWIP_HTTPD_SUPPORT_COOKIES
#error This needs LWIP_HTTPD_SUPPORT_COOKIES
#endif
#if !LWIP_HTTPD_SUPPORT_FS_OPEN_AUTH
#error This needs LWIP_HTTPD_SUPPORT_FS_OPEN_AUTH
#endif

#ifndef HTTP_DEBUG
#define HTTP_DEBUG 0
#endif

#if HTTP_DEBUG
#define DBG_HTTP(...) {chprintf(console, __VA_ARGS__);}
#else
#define DBG_HTTP(...)
#endif

#define HTTP_POST_DATA_SIZE 1024
#define HTTP_ALERT_MSG_SIZE 80
#define HTTP_SET_COOKIE_SIZE 48
static void *currentConn;
char current_uri[LWIP_HTTPD_MAX_REQUEST_URI_LEN] __attribute__((section(".ram4")));
char postData[HTTP_POST_DATA_SIZE] __attribute__((section(".ram4")));
char httpAlertMsg[HTTP_ALERT_MSG_SIZE] __attribute__((section(".ram4")));
char setCookie[HTTP_SET_COOKIE_SIZE] __attribute__((section(".ram4")));
static uint16_t postDataLen = 0;
void *verifiedConn = NULL;
typedef struct {
  uint32_t id;
  void     *conn;
} authorizedConn_t;
static authorizedConn_t authorizedConn = {0, 0};

// Size of dynamic HTML page
#define HTML_PAGE_SIZE 1024 * 28
#if HTML_PAGE_SIZE > (MEM_SIZE - 1600)
#error HTML_PAGE_SIZE needs to be smaller then lwip MEM_SIZE!
#endif


#define PAGE_HOME       0
#define PAGE_SETTING    1
#define PAGE_ZONE       2
#define PAGE_GROUP      3
#define PAGE_USER       4
#define PAGE_KEY        5
#define PAGE_ALERT      6
#define PAGE_NODE       7
#define PAGE_LOG        8
#define PAGE_TIMER      9
#define PAGE_TRIGGER    10
#define PAGE_TCL        11
#define PAGE_LOGIN      12

typedef struct {
  char    link[14];
  char    name[9];
} webPage_t;

static const webPage_t webPage[] = {
//  123456789012345  123456789012345
  {"/index.html",   "Home"},
  {"/setting.html", "Settings"},
  {"/zone.html",    "Zones"},
  {"/group.html",   "Groups"},
  {"/user.html",    "Users"},
  {"/key.html",     "Keys"},
  {"/alert.html",   "Alerts"},
  {"/node.html",    "Nodes"},
  {"/log.html",     "Log"},
  {"/timer.html",   "Timers"},
  {"/trigger.html", "Triggers"},
  {"/tcl.html",     "Scripts"},
  {"/login.html",   "Login"} // keep as last page
};
// HTML pages global variables to remember elements user works with last
static uint8_t webNode = 0, webContact = 0, webKey = 0, webZone = 0,
    webGroup = 0, webTimer = 0, webScript = DUMMY_NO_VALUE, webTrigger = 0;
static char scriptName[NAME_LENGTH];
static uint16_t webLog = 0;
/*
 * LWIP custom file init.
 */
void genfiles_ex_init(void) {
  /* nothing to do here yet */
}
/*
 * LWIP open custom file
 */
#include "httpd/httpd_handler_node.h"
#include "httpd/httpd_handler_user.h"
#include "httpd/httpd_handler_key.h"
#include "httpd/httpd_handler_zone.h"
#include "httpd/httpd_handler_alert.h"
#include "httpd/httpd_handler_log.h"
#include "httpd/httpd_handler_group.h"
#include "httpd/httpd_handler_home.h"
#include "httpd/httpd_handler_setting.h"
#include "httpd/httpd_handler_tcl.h"
#include "httpd/httpd_handler_timer.h"
#include "httpd/httpd_handler_trigger.h"
#include "httpd/httpd_handler_login.h"

int fs_open_custom(struct fs_file *file, const char *name){

  for (uint8_t htmlPage = 0; htmlPage < ARRAY_SIZE(webPage); ++htmlPage) {
    if (!strcmp(name, webPage[htmlPage].link)) {
      /* initialize fs_file correctly */
      memset(file, 0, sizeof(struct fs_file));
      file->pextension = mem_malloc(HTML_PAGE_SIZE);

      MemoryStream ms;
      BaseSequentialStream *chp;
      // Memory stream object to be used as a string writer, reserving one byte for the final zero.
      msObjectInit(&ms, (uint8_t *)file->pextension, HTML_PAGE_SIZE-1, 0);
      // Performing the print operation using the common code.
      chp = (BaseSequentialStream *)(void *)&ms;

      // Common html page start
      chprintf(chp, "<!DOCTYPE html><html lang='en'><head><meta charset='utf-8'><title>Open home security</title>\r\n");
      chprintf(chp, "<link rel='stylesheet' href='/css/OHS.css'>\r\n");
      chprintf(chp, "%sEnDis.js'>%s", html_script_src, html_e_script);
      chprintf(chp, "</head>\r\n<body onload=\"");
      // JavaScript enable/disable on body load
      switch (htmlPage) {
        case PAGE_USER:
          GET_CONF_CONTACT_IS_GLOBAL(conf.contact[webContact].setting) ? chprintf(chp, JSen1) : chprintf(chp, JSdis1);
          break;
        case PAGE_ZONE:
          GET_CONF_ZONE_TYPE(conf.zone[webZone]) ? chprintf(chp, JSen1) : chprintf(chp, JSdis1);
          break;
        case PAGE_TIMER:
          GET_CONF_TIMER_TYPE(conf.timer[webTimer].setting) ? chprintf(chp, JSen1) : chprintf(chp, JSdis1);
          break;
        case PAGE_TRIGGER:
          GET_CONF_TRIGGER_PASS(conf.trigger[webTrigger].setting) ? chprintf(chp, JSen1) : chprintf(chp, JSdis1);
          GET_CONF_TRIGGER_PASS_OFF(conf.trigger[webTrigger].setting) ? chprintf(chp, JSen2) : chprintf(chp, JSdis2);
          chprintf(chp, "sd(document.getElementById('y'));"); // Type select
          break;
        case PAGE_TCL:
        case PAGE_LOGIN:
          chprintf(chp, "ca();"); // Close alerts
          break;
      }
      chprintf(chp, "\"><div class='wrp'><div class='sb'>\r\n");
      chprintf(chp, "<div class='tt'>OHS %u.%u.%u</div>\r\n", OHS_MAJOR, OHS_MINOR, OHS_MOD);
      // Navigation
      chprintf(chp, "<ul class='nav'>\r\n");
      // Loop through pages in given order, except last one (login.html).
      for (uint8_t i = 0; i < (ARRAY_SIZE(webPage) - 1); ++i) {
        if (htmlPage == i) chprintf(chp, "<li><a class='active' href='%s'>%s</a></li>\r\n", webPage[i].link, webPage[i].name);
        else chprintf(chp, "<li><a href='%s'>%s</a></li>\r\n", webPage[i].link, webPage[i].name);
      }
      // Main Body
      chprintf(chp, "</ul></div><div class='mb'>\r\n");
      // Alert div
      if (strlen(httpAlertMsg)) {
        chprintf(chp, "<div class='alrt' id='at'><span class='cbtn' onclick=\"this.parentElement.style.display='none';\">&times;</span>");
        chprintf(chp, "<b>Error!</b><br><br>");
        chprintf(chp, "%s.%s", httpAlertMsg, html_div_e);
        memset(httpAlertMsg, 0 , HTTP_ALERT_MSG_SIZE); // Empty alert message
      }
      // Header
      chprintf(chp, "<h1>%s</h1>\r\n", webPage[htmlPage].name);
      chprintf(chp, "%s%s%s%s", html_form_1, webPage[htmlPage].link, html_form_2, html_table);
      // Common html page end

      // Custom html page start
      switch (htmlPage) {
        case PAGE_NODE:
          fs_open_custom_node(chp);
          break;
        case PAGE_USER:
          fs_open_custom_user(chp);
          break;
        case PAGE_KEY:
          fs_open_custom_key(chp);
          break;
        case PAGE_ZONE:
          fs_open_custom_zone(chp);
          break;
        case PAGE_ALERT:
          fs_open_custom_alert(chp);
          break;
        case PAGE_LOG:
          fs_open_custom_log(chp);
          break;
        case PAGE_GROUP:
          fs_open_custom_group(chp);
          break;
        case PAGE_HOME:
          fs_open_custom_home(chp);
          break;
        case PAGE_SETTING:
          fs_open_custom_setting(chp);
          break;
        case PAGE_TCL:
          fs_open_custom_tcl(chp);
          break;
        case PAGE_TIMER:
          fs_open_custom_timer(chp);
          break;
        case PAGE_TRIGGER:
          fs_open_custom_trigger(chp);
          break;
        case PAGE_LOGIN:
          fs_open_custom_login(chp);
          break;
        default:
          break;
      }

      // Custom end
      chprintf(chp, "</form></div></div></body></html>\r\n");

      if (file->pextension != NULL) {
        file->data = (const char *)file->pextension;
        file->len = ms.eos;
        file->index = file->len;
        // allow persistent connections
        file->flags = FS_FILE_FLAGS_HEADER_PERSISTENT;
        return 1;
      }
    }
  }
  // Other files
  if (strcmp(name, "/config.bin") == 0) {
    // Serve
    file->data = (const char *)&conf;
    file->len = sizeof(conf);
    file->index = file->len;
    // allow persistent connections
    file->flags = FS_FILE_FLAGS_HEADER_PERSISTENT;
    return 1;
  }
  return 0;
}
/*
 * LWIP close custom file
 */
void fs_close_custom(struct fs_file *file)
{
  if (file && file->pextension) {
    mem_free(file->pextension);
    file->pextension = NULL;
  }
}
/*
 * Receiving POST
 */
err_t httpd_post_begin(void *connection, const char *uri, const char *http_request,
                 u16_t http_request_len, int content_len, char *response_uri,
                 u16_t response_uri_len, u8_t *post_auto_wnd) {
  LWIP_UNUSED_ARG(connection);
  LWIP_UNUSED_ARG(http_request);
  LWIP_UNUSED_ARG(http_request_len);
  LWIP_UNUSED_ARG(content_len);
  LWIP_UNUSED_ARG(post_auto_wnd);

  DBG_HTTP("-PB-connection: %u\r\n", (uint32_t *)connection);
  //DBG_HTTP("-PB-uri: %s\r\n", (char *)uri);
  //DBG_HTTP("-PB-http_request: %s\r\n", (char *)http_request);
  //DBG_HTTP("-PB-http_request_len: %u\r\n", http_request_len);
  //DBG_HTTP("-PB-content_len: %u\r\n", content_len);
  //DBG_HTTP("-PB-response_uri: %s\r\n", (char *)response_uri);
  //DBG_HTTP("-PB-response_uri_len: %u\r\n", response_uri_len);
  //DBG_HTTP("-PB-post_auto_wnd: %u\r\n", *post_auto_wnd);

  if (currentConn != connection) {
    memset(current_uri, 0 , LWIP_HTTPD_MAX_REQUEST_URI_LEN);
    currentConn = connection;
    chsnprintf(response_uri, response_uri_len, uri);
    chsnprintf(current_uri, response_uri_len, uri);
    //memset(postData, 0, HTTP_POST_DATA_SIZE); // Empty POST data buffer
    postDataLen = 0;
    postData[0] = 0;
    return ERR_OK;
  }
  return ERR_VAL;
}

/*
 * Parse POST data.
 *
 * Parsing and processing modifies *pPostData buffer.
 */
bool getPostData(char **pPostData, char *pName, uint8_t nameLen, char **pValue, uint16_t *pValueLen){
  uint8_t ch, ch1, ch2;
  char *src = *pPostData;
  char *dst = *pPostData;
  bool isValue = false;

  *pValueLen = 0;
  if (nameLen > 0) *pName = 0;  // Initialize name
  *pValue = NULL;

  while (*src != '\0') {
    ch = *src++;

    if (ch == '+') {
      ch = ' ';
    } else if (ch == '%') {
      ch1 = *src;
      if (ch1 != '\0') {
        src++;
        ch2 = *src;
        if (ch2 != '\0') {
          src++;
          // Fast hex to int conversion
          ch1 = (ch1 >= 'a') ? (ch1 - 'a' + 10) : (ch1 >= 'A' ? (ch1 - 'A' + 10) : (ch1 - '0'));
          ch2 = (ch2 >= 'a') ? (ch2 - 'a' + 10) : (ch2 >= 'A' ? (ch2 - 'A' + 10) : (ch2 - '0'));
          ch = (ch1 << 4) | ch2;
        } else {
           *dst++ = 0; // Malformed
           break;
        }
      } else {
         *dst++ = 0; // Malformed
         break;
      }
    } else if (ch == '&') {
      *dst = 0;
      *pPostData = src;
      return true;
    } else if (ch == '=' && !isValue) {
      isValue = true;
      *dst++ = 0; // Terminate potential name in buffer (optional but clean)
      *pValue = dst; // Start of value in the modified buffer
      continue;
    }

    *dst++ = ch;

    if (!isValue) {
      if (nameLen > 1) {
        *pName++ = ch;
        *pName = 0;
        nameLen--;
      }
    } else {
      (*pValueLen)++;
    }
  }

  *dst = 0;
  *pPostData = src;
  return false;
}
/*
 * Receiving POST
 */
err_t httpd_post_receive_data(void *connection, struct pbuf *p) {

  DBG_HTTP("-PD-connection: %u\r\n", (uint32_t *)connection);
  if (currentConn == connection) {
    //DBG_HTTP("p->payload: '%.*s'\r\n", p->len, p->payload);
    
    // Iterate through pbuf chain and copy data
    if (p != NULL) {
      struct pbuf *q = p;
      do {
        // Calculate space left
        uint16_t spaceLeft = sizeof(postData) - postDataLen - 1; // -1 for null terminator
        if (spaceLeft > 0) {
           uint16_t copyLen = LWIP_MIN(q->len, spaceLeft);
           memcpy(&postData[postDataLen], q->payload, copyLen);
           postDataLen += copyLen;
           postData[postDataLen] = 0; // Ensure null termination
        }
        q = q->next;
      } while(q != NULL);
      
      pbuf_free(p);
    }
    return ERR_OK;
  }
  if (p != NULL) pbuf_free(p);
  return ERR_VAL;
}
/*
 * When POST is finished.
 */
void httpd_post_finished(void *connection, char *response_uri, u16_t response_uri_len) {
  LWIP_UNUSED_ARG(connection);
  LWIP_UNUSED_ARG(response_uri_len);

  uint16_t number, valueLen = 0, triggerNum;
  int8_t resp;
  char name[3];
  uint8_t message[REG_PACKET_SIZE + 1];
  bool repeat;
  char *postDataP = &postData[0], *endP, *valueP;

  DBG_HTTP("-PE-connection: %u\r\n", (uint32_t *)connection);

  if (currentConn == connection) {
    for (uint8_t htmlPage = 0; htmlPage < ARRAY_SIZE(webPage); ++htmlPage) {
      if (!strcmp(current_uri, webPage[htmlPage].link)) {
        switch (htmlPage) {
          case PAGE_NODE:
            do{
              repeat = getPostData(&postDataP, &name[0], sizeof(name), &valueP, &valueLen);
              DBG_HTTP("Parse: %s = '%.*s' (%u)\r\n", name, valueLen, valueP, valueLen);
              switch(name[0]){
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
                    /* Store message length
                    // Not queued, allocate new
                    if (node[webNode].queue == NULL) {
                      node[webNode].queue = umm_malloc(REG_PACKET_SIZE + 2);
                    }
                    // Copy new message to queue pointer
                    if (node[webNode].queue != NULL) {
                      ((uint8_t *)node[webNode].queue)[0] = REG_PACKET_SIZE + 1;
                      node[webNode].queue++;
                      memcpy(node[webNode].queue, &message[0], REG_PACKET_SIZE + 1);
                    }
                     */
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
                case 'e': // save
                  writeToBkpSRAM((uint8_t*)&conf, sizeof(config_t), 0);
                break;
              }
            } while (repeat);
            break;
          case PAGE_USER:
            do{
              repeat = getPostData(&postDataP, &name[0], sizeof(name), &valueP, &valueLen);
              DBG_HTTP("Parse: %s = '%.*s' (%u)\r\n", name, valueLen, valueP, valueLen);
              switch(name[0]){
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
            break;
          case PAGE_KEY:
            do{
              repeat = getPostData(&postDataP, &name[0], sizeof(name), &valueP, &valueLen);
              DBG_HTTP("Parse: %s = '%.*s' (%u)\r\n", name, valueLen, valueP, valueLen);
              switch(name[0]){
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
            break;
          case PAGE_ZONE:
            do{
              repeat = getPostData(&postDataP, &name[0], sizeof(name), &valueP, &valueLen);
              DBG_HTTP("Parse: %s = '%.*s' (%u)\r\n", name, valueLen, valueP, valueLen);
              switch(name[0]){
                case 'P': // select
                  number = strtol(valueP, NULL, 10);
                  if (number != webZone) { webZone = number; repeat = 0; }
                break;
                case 'A': // Apply, for remote zone send packet.
                  if (GET_CONF_ZONE_ENABLED(conf.zone[webZone]) &&
                      GET_CONF_ZONE_MQTT_PUB(conf.zone[webZone])) {
                    pushToMqtt(typeZone, webZone, functionState);
                  }
                  // Send remote zone changes
                  if (webZone >= HW_ZONES) {
                    message[0] = 'R';
                    message[1] = 'Z';
                    message[2] = (GET_CONF_ZONE_TYPE(conf.zone[webZone])) ? 'A' : 'D';
                    message[3] = webZone + 1;
                    message[4] = (uint8_t)((conf.zone[webZone] >> 8) & 0b11111111);;
                    message[5] = (uint8_t)(conf.zone[webZone] & 0b11111111);
                    memcpy(&message[6], conf.zoneName[webZone], NAME_LENGTH);
                    resp = sendData(conf.zoneAddress[webZone-HW_ZONES], message, REG_PACKET_SIZE + 1);
                  }
                break;
                case 'n': // name
                  // Calculate resp for MQTT
                  if (GET_CONF_ZONE_ENABLED(conf.zone[webZone]) &&
                      GET_CONF_ZONE_MQTT_PUB(conf.zone[webZone])) {
                    if (strlen(conf.zoneName[webZone]) != valueLen) resp = 1;
                    else resp = strncmp(conf.zoneName[webZone], valueP, LWIP_MIN(valueLen, NAME_LENGTH - 1));
                  } else resp = 0;
                  // Replace name
                  strncpy(conf.zoneName[webZone], valueP, LWIP_MIN(valueLen, NAME_LENGTH - 1));
                  conf.zoneName[webZone][LWIP_MIN(valueLen, NAME_LENGTH - 1)] = 0;
                  // MQTT
                  if (resp) {
                    pushToMqtt(typeZone, webZone, functionName);
                    // HAD
                    if (GET_CONF_ZONE_MQTT_HAD(conf.zone[webZone])) {
                      pushToMqttHAD(typeZone, webZone, functionHAD, 1);
                    }
                  }
                break;
                case 'D': // delay
                  SET_CONF_ZONE_AUTH_TIME(conf.zone[webZone], (valueP[0] - 48));
                break;
                case '0' ... '9': // Handle all single radio buttons for settings
                  if (valueP[0] == '0') conf.zone[webZone] &= ~(1 << (name[0]-48));
                  else                  conf.zone[webZone] |=  (1 << (name[0]-48));
                break;
                case 'a': // Handle all single radio buttons for settings 10 ->
                case 'c':
                case 'd':
                  resp = GET_CONF_ZONE_MQTT_HAD(conf.zone[webZone]);
                  if (valueP[0] == '0') conf.zone[webZone] &= ~(1 << (name[0]-87)); // a(97) - 10
                  else                  conf.zone[webZone] |=  (1 << (name[0]-87));
                  // Handle HAD change
                  if (GET_CONF_ZONE_ENABLED(conf.zone[webZone]) &&
                      (resp != GET_CONF_ZONE_MQTT_HAD(conf.zone[webZone]))) {
                    pushToMqttHAD(typeZone, webZone, functionHAD, GET_CONF_ZONE_MQTT_HAD(conf.zone[webZone]));
                  }
                break;
                case 'g': // group
                  number = strtol(valueP, NULL, 10);
                  SET_CONF_ZONE_GROUP(conf.zone[webZone], number);
                break;
                case 'e': // save
                  writeToBkpSRAM((uint8_t*)&conf, sizeof(config_t), 0);
                break;
              }
            } while (repeat);
            break;
          case PAGE_ALERT:
            do{
              repeat = getPostData(&postDataP, &name[0], sizeof(name), &valueP, &valueLen);
              DBG_HTTP("Parse: %s = '%.*s' (%u)\r\n", name, valueLen, valueP, valueLen);
              switch(name[0]){
                case '0' ... ('0' + ARRAY_SIZE(alertType)): // Handle all radio buttons in groups 0 .. #, A .. #
                  if (valueP[0] == '0') conf.alert[name[0]-48] &= ~(1 << (name[1]-65));
                  else conf.alert[name[0]-48] |= (1 << (name[1]-65));
                break;
                case 'e': // save
                  writeToBkpSRAM((uint8_t*)&conf, sizeof(config_t), 0);
                break;
                case 'M': // dummy alert
                  pushToLogText("D");
                break;
              }
            } while (repeat);
            break;
          case PAGE_LOG:
            do{
              repeat = getPostData(&postDataP, &name[0], sizeof(name), &valueP, &valueLen);
              DBG_HTTP("Parse: %s = '%.*s' (%u)\r\n", name, valueLen, valueP, valueLen);
              switch(name[0]){
                case 'N': // Now
                  webLog = FRAMWritePos - (LOGGER_OUTPUT_LEN * FRAM_MSG_SIZE);
                break;
                case 'R': // Reverse
                  webLog -= LOGGER_OUTPUT_LEN * FRAM_MSG_SIZE;
                break;
                case 'F': // Forward
                  webLog += LOGGER_OUTPUT_LEN * FRAM_MSG_SIZE;
                break;
              }
            } while (repeat);
            break;
          case PAGE_GROUP:
            do{
              repeat = getPostData(&postDataP, &name[0], sizeof(name), &valueP, &valueLen);
              DBG_HTTP("Parse: %s = '%.*s' (%u)\r\n", name, valueLen, valueP, valueLen);
              switch(name[0]){
                case 'P': // select
                  number = strtol(valueP, NULL, 10);
                  if (number != webGroup) { webGroup = number; repeat = 0; }
                break;
                case 'A': // Apply
                  // MQTT publish state
                  if (GET_CONF_GROUP_ENABLED(conf.group[webGroup].setting) &&
                      GET_CONF_GROUP_MQTT(conf.group[webGroup].setting)) {
                    pushToMqtt(typeGroup, webGroup, functionState);
                  }
                break;
                case 'n': // name
                  // Calculate resp for MQTT
                  if (GET_CONF_GROUP_ENABLED(conf.group[webGroup].setting) &&
                      GET_CONF_GROUP_MQTT(conf.group[webGroup].setting)) {
                    if (strlen(conf.group[webGroup].name) != valueLen) resp = 1;
                    else resp = strncmp(conf.group[webGroup].name, valueP, LWIP_MIN(valueLen, NAME_LENGTH - 1));
                  } else resp = 0;
                  // Replace name
                  strncpy(conf.group[webGroup].name, valueP, LWIP_MIN(valueLen, NAME_LENGTH - 1));
                  conf.group[webGroup].name[LWIP_MIN(valueLen, NAME_LENGTH - 1)] = 0;
                  // MQTT
                  if (resp) {
                    pushToMqtt(typeGroup, webGroup, functionName);
                    // HAD
                    if (GET_CONF_GROUP_MQTT_HAD(conf.group[webGroup].setting)) {
                      pushToMqttHAD(typeGroup, webGroup, functionHAD, 1);
                    }
                  }
                break;
                case '0' ... '7': // Handle all single radio buttons for settings
                  resp = GET_CONF_GROUP_MQTT_HAD(conf.group[webGroup].setting);
                  if (valueP[0] == '0') conf.group[webGroup].setting &= ~(1 << (name[0]-48));
                  else                  conf.group[webGroup].setting |=  (1 << (name[0]-48));
                  // Handle HAD change
                  if (GET_CONF_GROUP_ENABLED(conf.group[webGroup].setting) &&
                      (resp != GET_CONF_GROUP_MQTT_HAD(conf.group[webGroup].setting))) {
                    pushToMqttHAD(typeGroup, webGroup, functionHAD, GET_CONF_GROUP_MQTT_HAD(conf.group[webGroup].setting));
                  }
                break;
                case 'a': // arm chain
                  number = strtol(valueP, NULL, 10);
                  SET_CONF_GROUP_ARM_CHAIN(conf.group[webGroup].setting, number);
                break;
                case 'd': // disarm chain
                  number = strtol(valueP, NULL, 10);
                  SET_CONF_GROUP_DISARM_CHAIN(conf.group[webGroup].setting, number);
                break;
                case 'e': // save
                  writeToBkpSRAM((uint8_t*)&conf, sizeof(config_t), 0);
                break;
                case 'D': // Disarm
                  disarmGroup(webGroup, DUMMY_NO_VALUE, 0); // Disarm just this group
                break;
              }
            } while (repeat);
            break;
          case PAGE_SETTING:
            do{
              repeat = getPostData(&postDataP, &name[0], sizeof(name), &valueP, &valueLen);
              DBG_HTTP("Parse: %s = '%.*s' (%u)\r\n", name, valueLen, valueP, valueLen);
              switch(name[0]){
                case 'A': // Apply
                  // SMTP
                  smtp_set_server_addr(conf.SMTPAddress);
                  smtp_set_server_port(conf.SMTPPort);
                  smtp_set_auth(conf.SMTPUser, conf.SMTPPassword);
                  // SNTP
                  sntp_setservername(0, conf.SNTPAddress);
                break;
                case 'C':
                  conf.armDelay = strtol(valueP, NULL, 10) * 4;
                break;
                case 'E':
                  conf.autoArm = strtol(valueP, NULL, 10);
                break;
                case 'F':
                  conf.openAlarm = strtol(valueP, NULL, 10);
                break;
                case 'u': // user
                  strncpy(conf.user, valueP, LWIP_MIN(valueLen, NAME_LENGTH - 1));
                  conf.user[LWIP_MIN(valueLen, NAME_LENGTH - 1)] = 0;
                break;
                case 'p': // password
                  strncpy(conf.password, valueP, LWIP_MIN(valueLen, NAME_LENGTH - 1));
                  conf.password[LWIP_MIN(valueLen, NAME_LENGTH - 1)] = 0;
                break;
                case 'a': // SMTP server
                  strncpy(conf.SMTPAddress, valueP, LWIP_MIN(valueLen, URL_LENGTH - 1));
                  conf.SMTPAddress[LWIP_MIN(valueLen, URL_LENGTH - 1)] = 0;
                break;
                case 'b': // SMTP port
                  conf.SMTPPort = strtol(valueP, NULL, 10);
                break;
                case 'c': // SMTP user
                  strncpy(conf.SMTPUser, valueP, LWIP_MIN(valueLen, EMAIL_LENGTH - 1));
                  conf.SMTPUser[LWIP_MIN(valueLen, EMAIL_LENGTH - 1)] = 0;
                break;
                case 'd': // SMTP password
                  strncpy(conf.SMTPPassword, valueP, LWIP_MIN(valueLen, NAME_LENGTH - 1));
                  conf.SMTPPassword[LWIP_MIN(valueLen, NAME_LENGTH - 1)] = 0;
                break;
                case 'y': // MQTT server
                  // Compute resp
                  if (strlen(conf.mqtt.address) != valueLen) resp = 1;
                  else resp = strncmp(conf.mqtt.address, valueP, LWIP_MIN(valueLen, NAME_LENGTH - 1));
                  // Copy name
                  strncpy(conf.mqtt.address, valueP, LWIP_MIN(valueLen, URL_LENGTH - 1));
                  conf.mqtt.address[LWIP_MIN(valueLen, URL_LENGTH - 1)] = 0;
                  // Clear MQTT resolve flag
                  if (resp) CLEAR_CONF_MQTT_ADDRESS_ERROR(conf.mqtt.setting);
                break;
                case 'q': // MQTT port
                  conf.mqtt.port = strtol(valueP, NULL, 10);
                break;
                case 't': // MQTT user
                  strncpy(conf.mqtt.user, valueP, LWIP_MIN(valueLen, NAME_LENGTH - 1));
                  conf.mqtt.user[LWIP_MIN(valueLen, NAME_LENGTH - 1)] = 0;
                break;
                case 'r': // MQTT password
                  strncpy(conf.mqtt.password, valueP, LWIP_MIN(valueLen, NAME_LENGTH - 1));
                  conf.mqtt.password[LWIP_MIN(valueLen, NAME_LENGTH - 1)] = 0;
                break;
                case '0' ... '1': // MQTT handle all single radio buttons for settings
                  resp = GET_CONF_MQTT_HAD(conf.mqtt.setting);
                  if (valueP[0] == '0') conf.mqtt.setting &= ~(1 << (name[0]-48));
                  else                  conf.mqtt.setting |=  (1 << (name[0]-48));
                  // Handle HAD change
                  if (resp != GET_CONF_MQTT_HAD(conf.mqtt.setting)) {
                    mqttGlobalHAD(GET_CONF_MQTT_HAD(conf.mqtt.setting));
                  }
                break;
                case 'f': // NTP server
                  strncpy(conf.SNTPAddress, valueP, LWIP_MIN(valueLen, URL_LENGTH - 1));
                  conf.SNTPAddress[LWIP_MIN(valueLen, URL_LENGTH - 1)] = 0;
                break;
                case 'w':
                  conf.timeStdWeekNum = strtol(valueP, NULL, 10);
                break;
                case 's':
                  conf.timeStdDow = strtol(valueP, NULL, 10);
                break;
                case 'm':
                  conf.timeStdMonth = strtol(valueP, NULL, 10);
                break;
                case 'h':
                  conf.timeStdHour = strtol(valueP, NULL, 10);
                break;
                case 'o':
                  conf.timeStdOffset = strtol(valueP, NULL, 10);
                break;
                case 'W':
                  conf.timeDstWeekNum = strtol(valueP, NULL, 10);
                break;
                case 'S':
                  conf.timeDstDow = strtol(valueP, NULL, 10);
                break;
                case 'M':
                  conf.timeDstMonth = strtol(valueP, NULL, 10);
                break;
                case 'H':
                  conf.timeDstHour = strtol(valueP, NULL, 10);
                break;
                case 'O':
                  conf.timeDstOffset = strtol(valueP, NULL, 10);
                break;
                case 'g': // time format
                  strncpy(conf.dateTimeFormat, valueP, LWIP_MIN(valueLen, NAME_LENGTH - 1));
                  conf.dateTimeFormat[LWIP_MIN(valueLen, NAME_LENGTH - 1)] = 0;
                break;
                case 'K': // radio key
                  strncpy(conf.radioKey, valueP, LWIP_MIN(valueLen, RADIO_KEY_SIZE - 1));
                  conf.radioKey[LWIP_MIN(valueLen, RADIO_KEY_SIZE - 1)] = 0;
                break;
                case 'e': // save
                  writeToBkpSRAM((uint8_t*)&conf, sizeof(config_t), 0);
                break;
              }
            } while (repeat);
            break;
          case PAGE_HOME:
            do{
              repeat = getPostData(&postDataP, &name[0], sizeof(name), &valueP, &valueLen);
              DBG_HTTP("Parse: %s = '%.*s' (%u)\r\n", name, valueLen, valueP, valueLen);
              switch(name[0]){
                case 'e': // save
                  writeToBkpSRAM((uint8_t*)&conf, sizeof(config_t), 0);
                break;
                case 'D': // Load defaults
                  setConfDefault();    // Load OHS default conf.
                  initRuntimeGroups(); // Initialize runtime variables
                break;
              }
            } while (repeat);
            break;
          case PAGE_TCL:
            do{
              repeat = getPostData(&postDataP, &name[0], sizeof(name), &valueP, &valueLen);
              DBG_HTTP("Parse: %s = '%.*s' (%u)\r\n", name, valueLen, valueP, valueLen);
              scriptEvent_t *outMsg;
              switch(name[0]){
                case 'P': // select
                  number = strtol(valueP, NULL, 10);
                  if (number != webScript) {
                    webScript = number; repeat = 0;
                    memset(&tclCmd[0], '\0', TCL_SCRIPT_LENGTH);
                    memset(&scriptName[0], '\0', NAME_LENGTH);
                    // For old script load values
                    if (webScript != DUMMY_NO_VALUE) {
                      number = 1;
                      for (scriptp = scriptLL; scriptp != NULL; scriptp = scriptp->next) {
                        if (number == webScript) break;
                        number++;
                      }
                      if (scriptp != NULL) {
                        strncpy(&scriptName[0], scriptp->name, NAME_LENGTH);
                        strncpy(&tclCmd[0], scriptp->cmd, TCL_SCRIPT_LENGTH);
                      }
                    }
                  }
                  break;
                case 'n': // name
                  // For existing scripts do rename when name differs
                  if ((webScript != DUMMY_NO_VALUE) && (memcmp(scriptName, valueP, LWIP_MIN(valueLen, strlen(scriptName))))) {
                    DBG_HTTP("Rename: '%s' -> '%.*s'; %d;%d\r\n", scriptName, valueLen, valueP);
                    number = 1;
                    // Find pointer to script
                    for (scriptp = scriptLL; scriptp != NULL; scriptp = scriptp->next) {
                      if (number == webScript) break;
                      number++;
                    }
                    memset(scriptp->name, 0, NAME_LENGTH); // Make sure all is 0, as it gets stored in uBS
                    strncpy(scriptp->name, valueP, LWIP_MIN(valueLen, NAME_LENGTH - 1));
                    // uBS rename
                    uBSRename(scriptName, strlen(scriptName),
                              valueP, LWIP_MIN(valueLen, NAME_LENGTH - 1));
                  }
                  memset(&scriptName[0], 0, NAME_LENGTH); // Make sure all is 0, as it gets stored in uBS
                  strncpy(scriptName, valueP, LWIP_MIN(valueLen, NAME_LENGTH - 1));
                  break;
                case 'R': // Run
                  outMsg = chPoolAlloc(&script_pool);
                  if (outMsg != NULL) {
                    outMsg->callback = NULL;
                    outMsg->result = NULL;
                    outMsg->flags = 1;
                    outMsg->cmdP = &tclCmd[0];
                    msg_t msg = chMBPostTimeout(&script_mb, (msg_t)outMsg, TIME_IMMEDIATE);
                    if (msg != MSG_OK) {
                      //chprintf(console, "MB full %d\r\n", temp);
                    }
                  } else {
                    chprintf(console, "CB full %d \r\n", outMsg);
                  }
                break;
                case 's': // script
                  strncpy(tclCmd, valueP, LWIP_MIN(valueLen, TCL_SCRIPT_LENGTH - 1));
                  tclCmd[LWIP_MIN(valueLen, TCL_SCRIPT_LENGTH - 1)] = 0;
                break;
                case 'e': // save
                  if (webScript == DUMMY_NO_VALUE) {
                    if (!strlen(scriptName)) {
                      chsnprintf(httpAlertMsg, HTTP_ALERT_MSG_SIZE, "Not allowed to save empty name");
                      break;
                    }
                    // For new script append linked list
                    scriptp = umm_malloc(sizeof(struct scriptLL_t));
                    if (scriptp == NULL) {
                      chsnprintf(httpAlertMsg, HTTP_ALERT_MSG_SIZE, "%s%s", text_error_free, text_heap);
                    } else {
                      //if (checkPointer(scriptp, html_noSpace)) {}
                      scriptp->name = umm_malloc(NAME_LENGTH);
                      if (scriptp->name == NULL) {
                        umm_free(scriptp);
                        chsnprintf(httpAlertMsg, HTTP_ALERT_MSG_SIZE, "%s%s", text_error_free, text_heap);
                      } else {
                        strncpy(scriptp->name, &scriptName[0], NAME_LENGTH);
                        number = strlen(tclCmd);
                        scriptp->cmd = umm_malloc(number + 1); // + NULL
                        if (scriptp->cmd == NULL) {
                          umm_free(scriptp->name);
                          umm_free(scriptp);
                          chsnprintf(httpAlertMsg, HTTP_ALERT_MSG_SIZE, "%s%s", text_error_free, text_heap);
                        } else {
                          strncpy(scriptp->cmd, &tclCmd[0], number);
                          memset(scriptp->cmd + number, 0, 1);
                          scriptp->next = scriptLL;
                          scriptLL = scriptp;
                          // uBS
                          if (uBSWrite(&scriptName[0], NAME_LENGTH, &tclCmd[0], strlen(tclCmd)) != UBS_RSLT_OK) {
                            chsnprintf(httpAlertMsg, HTTP_ALERT_MSG_SIZE, "%s%s", text_error_free, text_storage);
                          }
                          // new script is added to top of linked list, no need to do pointer check
                          webScript = 1;
                        }
                      }
                    }
                  } else {
                    // For old script replace values
                    number = 1;
                    // Find pointer to script
                    for (scriptp = scriptLL; scriptp != NULL; scriptp = scriptp->next) {
                      if (number == webScript) break;
                      number++;
                    }
                    if (scriptp != NULL) {
                      number = strlen(tclCmd);
                      //scriptp->cmd = umm_realloc(scriptp->cmd, number + 1);
                      umm_free(scriptp->cmd);
                      scriptp->cmd = umm_malloc(number + 1);
                      if (scriptp->cmd == NULL) {
                        chsnprintf(httpAlertMsg, HTTP_ALERT_MSG_SIZE, "%s%s", text_error_free, text_heap);
                      } else {
                        strncpy(scriptp->cmd, &tclCmd[0], number);
                        memset(scriptp->cmd + number, 0, 1);
                        for (int i = 0; i < UBS_NAME_SIZE; i++) { DBG_HTTP("%x;", scriptName[i]); }
                        DBG_HTTP("\r\n");
                        if (uBSWrite(&scriptName[0], NAME_LENGTH, &tclCmd[0], strlen(tclCmd)) != UBS_RSLT_OK) {
                          chsnprintf(httpAlertMsg, HTTP_ALERT_MSG_SIZE, "%s%s", text_error_free, text_storage);
                        }
                      }
                    }
                  }
                break;
              }
            } while (repeat);
            break;
          case PAGE_TIMER:
            do{
              repeat = getPostData(&postDataP, &name[0], sizeof(name), &valueP, &valueLen);
              DBG_HTTP("Parse: %s = '%.*s' (%u)\r\n", name, valueLen, valueP, valueLen);
              switch(name[0]){
                case 'P': // select
                  number = strtol(valueP, NULL, 10);
                  if (number != webTimer) { webTimer = number; repeat = 0; }
                break;
                case 'A': // Apply
                  setTimer(webTimer, true);
                break;
                case 'n': // name
                  strncpy(conf.timer[webTimer].name, valueP, LWIP_MIN(valueLen, NAME_LENGTH - 1));
                  conf.timer[webTimer].name[LWIP_MIN(valueLen, NAME_LENGTH - 1)] = 0;
                break;
                case 's': // period
                  conf.timer[webTimer].periodTime = strtol(valueP, NULL, 10);
                break;
                case 'S': // period
                  number = strtol(valueP, NULL, 10);
                  SET_CONF_TIMER_PERIOD_TYPE(conf.timer[webTimer].setting, number);
                break;
                case 'r': // run
                  conf.timer[webTimer].runTime = strtol(valueP, NULL, 10);
                break;
                case 'R': // run
                  number = strtol(valueP, NULL, 10);
                  SET_CONF_TIMER_RUN_TYPE(conf.timer[webTimer].setting, number);
                break;
                case 'a': // node aaddress
                  number = strtol(valueP, NULL, 10);
                  if (number == DUMMY_NO_VALUE) {
                    conf.timer[webTimer].toAddress  = 0;
                    conf.timer[webTimer].toFunction = ' ';
                    conf.timer[webTimer].toNumber   = 0;
                  } else {
                    conf.timer[webTimer].toAddress  = node[number].address;
                    conf.timer[webTimer].toFunction = node[number].function;
                    conf.timer[webTimer].toNumber   = node[number].number;
                  }
                break;
                case 'o':
                  conf.timer[webTimer].constantOn = strtof(valueP, NULL);
                break;
                case 'f':
                  conf.timer[webTimer].constantOff = strtof(valueP, NULL);
                break;
                case 't': // time
                  conf.timer[webTimer].startTime = strtol(valueP, &endP, 10) * MINUTES_PER_HOUR ;
                  conf.timer[webTimer].startTime += strtol(++endP, NULL, 10);
                break;
                case 'p': // script
                  strncpy(conf.timer[webTimer].evalScript, valueP, LWIP_MIN(valueLen, NAME_LENGTH - 1));
                  conf.timer[webTimer].evalScript[LWIP_MIN(valueLen, NAME_LENGTH - 1)] = 0;
                break;
                case 'B' ... 'J': // Handle all single radio buttons for settings B(66)=0
                  if (valueP[0] == '0') conf.timer[webTimer].setting &= ~(1 << (name[0]-66));
                  else                  conf.timer[webTimer].setting |=  (1 << (name[0]-66));
                break;
                case 'e': // save
                  writeToBkpSRAM((uint8_t*)&conf, sizeof(config_t), 0);
                break;
              }
            } while (repeat);
            break;
          case PAGE_TRIGGER:
            do{
              repeat = getPostData(&postDataP, &name[0], sizeof(name), &valueP, &valueLen);
              DBG_HTTP("Parse: %s = '%.*s' (%u)\r\n", name, valueLen, valueP, valueLen);
              switch(name[0]){
                case 'P': // select
                  number = strtol(valueP, NULL, 10);
                  if (number != webTrigger) { webTrigger = number; repeat = 0; }
                break;
                case 'A': // Apply
                break;
                case 'n': // name
                  strncpy(conf.trigger[webTrigger].name, valueP, LWIP_MIN(valueLen, NAME_LENGTH - 1));
                  conf.trigger[webTrigger].name[LWIP_MIN(valueLen, NAME_LENGTH - 1)] = 0;
                break;
                case 'a': // node aaddress
                  number = strtol(valueP, NULL, 10);
                  if (number == DUMMY_NO_VALUE) {
                    conf.trigger[webTrigger].toAddress  = 0;
                    conf.trigger[webTrigger].toFunction = ' ';
                    conf.trigger[webTrigger].toNumber   = 0;
                  } else {
                    conf.trigger[webTrigger].toAddress  = node[number].address;
                    conf.trigger[webTrigger].toFunction = node[number].function;
                    conf.trigger[webTrigger].toNumber   = node[number].number;
                  }
                break;
                case 'y': // type
                  triggerNum = number = strtol(valueP, NULL, 10);
                  if (number < ALARM_GROUPS) {
                    conf.trigger[webTrigger].type       = 'G';
                    conf.trigger[webTrigger].address  = 0;
                    conf.trigger[webTrigger].function = ' ';
                    conf.trigger[webTrigger].number   = number;
                  } else if ((number >= ALARM_GROUPS) && (number < (ALARM_ZONES + ALARM_GROUPS))) {
                    conf.trigger[webTrigger].type       = 'Z';
                    conf.trigger[webTrigger].address  = 0;
                    conf.trigger[webTrigger].function = ' ';
                    conf.trigger[webTrigger].number   = number - ALARM_GROUPS;
                  } else if (number >= (ALARM_ZONES + ALARM_GROUPS)) {
                    conf.trigger[webTrigger].type       = 'S';
                    conf.trigger[webTrigger].address  = node[number - ALARM_ZONES - ALARM_GROUPS].address;
                    conf.trigger[webTrigger].function = node[number - ALARM_ZONES - ALARM_GROUPS].function;
                    conf.trigger[webTrigger].number   = node[number - ALARM_ZONES - ALARM_GROUPS].number;
                  } else {
                    conf.trigger[webTrigger].type     = ' ';
                    conf.trigger[webTrigger].address  = 0;
                    conf.trigger[webTrigger].function = ' ';
                    conf.trigger[webTrigger].number   = 0;
                  }
                break;
                case 'c': // condition
                  conf.trigger[webTrigger].condition = strtol(valueP, NULL, 10);
                break;
                case 't': // off timer
                  conf.trigger[webTrigger].offTime = strtol(valueP, NULL, 10);
                break;
                case 'T': // period
                  SET_CONF_TRIGGER_OFF_PERIOD(conf.trigger[webTrigger].setting, strtol(valueP, NULL, 10));
                break;
                case 's': // Pass no yes once
                  SET_CONF_TRIGGER_PASS(conf.trigger[webTrigger].setting, strtol(valueP, NULL, 10));
                break;
                case 'S': // Pass off no yes timer
                  SET_CONF_TRIGGER_PASS_OFF(conf.trigger[webTrigger].setting, strtol(valueP, NULL, 10));
                break;
                case 'o':
                  conf.trigger[webTrigger].constantOn = strtof(valueP, NULL);
                break;
                case 'f':
                  conf.trigger[webTrigger].constantOff = strtof(valueP, NULL);
                break;
                case 'g': // Value for Group
                  if (triggerNum < ALARM_GROUPS)
                    conf.trigger[webTrigger].value = strtof(valueP, NULL);
                break;
                case 'z': // Value for Zone
                  if ((triggerNum >= (ALARM_GROUPS)) && (triggerNum < (ALARM_ZONES + ALARM_GROUPS)))
                    conf.trigger[webTrigger].value = strtof(valueP, NULL);
                break;
                case 'v': // Value for Sensor
                  if (triggerNum >= (ALARM_ZONES + ALARM_GROUPS))
                    conf.trigger[webTrigger].value = strtof(valueP, NULL);
                break;
                case 'p': // script
                  strncpy(conf.trigger[webTrigger].evalScript, valueP, LWIP_MIN(valueLen, NAME_LENGTH - 1));
                  conf.trigger[webTrigger].evalScript[LWIP_MIN(valueLen, NAME_LENGTH - 1)] = 0;
                break;
                case 'B' ... 'H': // Handle all single radio buttons for settings B(66)=0
                  if (valueP[0] == '0') conf.trigger[webTrigger].setting &= ~(1 << (name[0]-66));
                  else                  conf.trigger[webTrigger].setting |=  (1 << (name[0]-66));
                break;
                case 'e': // save
                  writeToBkpSRAM((uint8_t*)&conf, sizeof(config_t), 0);
                break;
              }
            } while (repeat);
            break;
          case PAGE_LOGIN:
            number = 1; // temp. variable to hold state of authentication
            do{
              repeat = getPostData(&postDataP, &name[0], sizeof(name), &valueP, &valueLen);
              DBG_HTTP("Parse: %s = '%.*s' (%u)\r\n", name, valueLen, valueP, valueLen);
              switch(name[0]){
                case 'u': // user
                  number = strcmp(valueP, conf.user);
                break;
                case 'p': // password
                  if (!number &&(!strcmp(valueP, conf.password))) {
                    verifiedConn = connection;
                    authorizedConn.id = STM32_UUID[0] + rand();
                    authorizedConn.conn = (void *)connection;
                  } else {
                    chsnprintf(httpAlertMsg, HTTP_ALERT_MSG_SIZE, "User or password not valid!");
                  }
                break;
              }
            } while (repeat);
            break;
          default:
            break;
        }
        break; // If found, no need to look for another page.
      }
    }

    // Change final uri if authenticated via /login.html
    if (verifiedConn == connection) {
      chsnprintf(response_uri, response_uri_len, webPage[0].link);
    } else {
      chsnprintf(response_uri, response_uri_len, current_uri);
    }
    DBG_HTTP("-PE-response_uri: %s\r\n", response_uri);
    currentConn = NULL;
  }
}
/*
 * HTTPD callback
 */
char *httpd_set_cookies(const void *connection, const char *uri) {
  LWIP_UNUSED_ARG(uri);

  DBG_HTTP("-SC: %u, %s\r\n", (uint32_t *)connection, uri);
  if (connection == verifiedConn) {
    verifiedConn = NULL; // Clear verified connection
    chsnprintf(setCookie, HTTP_SET_COOKIE_SIZE,
               HDR_HTTP_RES_SET_COOKIE "id=%08X\r\n", authorizedConn.id);
    DBG_HTTP("--SC: %s", setCookie);
    return setCookie;
  }
  return NULL;
}
/*
 * HTTPD callback
 */
void httpd_received_cookies(const void *connection, const char *cookies) {
  DBG_HTTP("-RC: %u, cookies: %s\r\n", (uint32_t *)connection, cookies);
  const char *sessionId = strstr(cookies, "id=");
  if (sessionId != NULL) {
     uint32_t value = strtol(sessionId+3, NULL, 16);
     if (value == authorizedConn.id) {
       authorizedConn.conn = (uint32_t *)connection;
       DBG_HTTP("--RC: %u\r\n", (uint32_t *)connection);
     }
  }
}
/*
 * HTTPD callback
 */
void httpd_authorize_fs_open(void *connection, const char **name){
  DBG_HTTP("-AFSO: %u, %s\r\n", (uint32_t *)connection, *name);
  if (strstr(*name, ".html")) {
    if (authorizedConn.conn != connection) *name = webPage[PAGE_LOGIN].link;
    DBG_HTTP("--AFSO: %u, %s\r\n", (uint32_t *)connection, *name);
  }
  authorizedConn.conn = NULL;
}

#endif /* OHS_HTTPD_HANDLER_H_ */
