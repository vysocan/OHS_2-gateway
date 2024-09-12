/*
 * ohs_httpdhandler.h
 *
 *  Created on: 6. 12. 2019
 *      Author: vysocan
 */

#ifndef OHS_HTTPDHANDLER_H_
#define OHS_HTTPDHANDLER_H_

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
void *verifiedConn = NULL;
typedef struct {
  uint32_t id;
  void     *conn;
} authorizedConn_t;
static authorizedConn_t authorizedConn = {0, 0};

// Size of dynamic HTML page
#define HTML_PAGE_SIZE 1024 * 16
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
  {"/login.html",   "Login"}
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
int fs_open_custom(struct fs_file *file, const char *name){
  char temp[3] = "";
  uint16_t logAddress;
  uint8_t number;
  time_t tempTime;      // Temp time

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
          chprintf(chp, "%s#", html_tr_th);
          chprintf(chp, "%s%s", html_e_th_th, text_Address);
          chprintf(chp, "%s%s", html_e_th_th, text_On);
          chprintf(chp, "%s%s", html_e_th_th, text_MQTT);
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
              printFrmTimestamp(chp, &node[i].lastOK);
              chprintf(chp, "%s", html_e_td_td);
              if (node[i].queue) number = 1;
              else number = 0;
              printOkNok(chp, number); // queued
              chprintf(chp, "%s", html_e_td_td);
              printNodeType(chp, node[i].type);
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
          printNodeType(chp, node[webNode].type);
          chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, text_Function, html_e_td_td);
          printNodeFunction(chp, node[webNode].function);
          chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_Node, text_is, html_e_td_td);
          printOnOffButton(chp, "0", GET_NODE_ENABLED(node[webNode].setting));
          chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_MQTT, text_publish, html_e_td_td);
          printOnOffButton(chp, "7", GET_NODE_MQTT(node[webNode].setting));
          chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, text_Group, html_e_td_td);
          selectGroup(chp, GET_NODE_GROUP(node[webNode].setting), 'g');
          chprintf(chp, "%s%s", html_e_td_e_tr, html_e_table);
          // Buttons
          chprintf(chp, "%s%s", html_Apply, html_Reregister);
          break;
        case PAGE_USER:
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
          break;
        case PAGE_KEY:
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
            printKey(chp, (char *)&uint32Conv.byte[0]);
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
          printKey(chp, (char *)&uint32Conv.byte[0]);
          chprintf(chp, "%s%k%s", html_id_tag, html_e_tag);
          chprintf(chp, "%s%s", html_e_td_e_tr, html_e_table);
          // Buttons
          chprintf(chp, "%s%s", html_Apply, html_Save);
          break;
        case PAGE_ZONE:
          chprintf(chp, "%s#", html_tr_th);
          chprintf(chp, "%s%s", html_e_th_th, text_Name);
          chprintf(chp, "%s%s", html_e_th_th, text_On);
          chprintf(chp, "%s%s", html_e_th_th, text_Type);
          chprintf(chp, "%s%s %s", html_e_th_th, text_Arm, text_home);
          chprintf(chp, "%s%s %s", html_e_th_th, text_Open, text_alarm);
          chprintf(chp, "%s%s %s %s", html_e_th_th, text_Alarm, text_as, text_tamper);
          chprintf(chp, "%s%s", html_e_th_th, text_Delay);
          chprintf(chp, "%s%s %s", html_e_th_th, text_Last, text_alarm);
          chprintf(chp, "%s%s %s", html_e_th_th, text_Last, text_OK);
          chprintf(chp, "%s%s", html_e_th_th, text_MQTT);
          chprintf(chp, "%s%s", html_e_th_th, text_HAD);
          chprintf(chp, "%s%s", html_e_th_th, text_Status);
          chprintf(chp, "%s%s%s\r\n", html_e_th_th, text_Group, html_e_th_e_tr);
          // Information table
          for (uint8_t i = 0; i < ALARM_ZONES; i++) {
            if (GET_CONF_ZONE_IS_PRESENT(conf.zone[i])) {
              chprintf(chp, "%s%u.%s", html_tr_td, i + 1, html_e_td_td);
              chprintf(chp, "%s%s", conf.zoneName[i], html_e_td_td);
              printOkNok(chp, GET_CONF_ZONE_ENABLED(conf.zone[i]));
              chprintf(chp, "%s", html_e_td_td);
              if (GET_CONF_ZONE_ENABLED(conf.zone[i])) {
                GET_CONF_ZONE_BALANCED(conf.zone[i]) ? chprintf(chp, "%s ", text_balanced) : chprintf(chp, "un%s ", text_balanced);
                (i >= HW_ZONES) ? chprintf(chp, "%s ", text_remote) : chprintf(chp, "%s ", text_local);
                GET_CONF_ZONE_TYPE(conf.zone[i]) ? chprintf(chp, "%s", text_analog) : chprintf(chp, "%s", text_digital);
              }
              chprintf(chp, "%s", html_e_td_td);
              printOkNok(chp, GET_CONF_ZONE_ARM_HOME(conf.zone[i]));
              chprintf(chp, "%s", html_e_td_td);
              printOkNok(chp, GET_CONF_ZONE_OPEN_ALARM(conf.zone[i]));
              chprintf(chp, "%s", html_e_td_td);
              printOkNok(chp, GET_CONF_ZONE_PIR_AS_TMP(conf.zone[i]));
              chprintf(chp, "%s", html_e_td_td);
              chprintf(chp, "%u %s%s", GET_CONF_ZONE_AUTH_TIME(conf.zone[i])*conf.armDelay/4,
                       durationSelect[0], html_e_td_td);
              if (GET_CONF_ZONE_ENABLED(conf.zone[i])) printFrmTimestamp(chp, &zone[i].lastPIR);
              chprintf(chp, "%s", html_e_td_td);
              if (GET_CONF_ZONE_ENABLED(conf.zone[i])) printFrmTimestamp(chp, &zone[i].lastOK);
              chprintf(chp, "%s", html_e_td_td);
              if (GET_CONF_ZONE_ENABLED(conf.zone[i])) printOkNok(chp, GET_CONF_ZONE_MQTT_PUB(conf.zone[i]));
              chprintf(chp, "%s", html_e_td_td);
              if (GET_CONF_ZONE_ENABLED(conf.zone[i])) printOkNok(chp, GET_CONF_ZONE_MQTT_HAD(conf.zone[i]));
              chprintf(chp, "%s", html_e_td_td);
              if (GET_CONF_ZONE_ENABLED(conf.zone[i])) {
                switch(zone[i].lastEvent){
                  case 'O': chprintf(chp, "%s", html_i_OK); break;
                  case 'P': chprintf(chp, "%s", html_i_alarm); break;
                  case 'N': chprintf(chp, "%s", html_i_starting); break;
                  default: chprintf(chp, "%s", text_tamper); break;
                }
              } else { chprintf(chp, "%s", html_i_disabled); }
              chprintf(chp, "%s", html_e_td_td);
              printGroup(chp, GET_CONF_ZONE_GROUP(conf.zone[i]));
              chprintf(chp, "%s", html_e_td_e_tr);
            }
          }
          chprintf(chp, "%s", html_e_table);
          chprintf(chp, "%s", html_table);
          chprintf(chp, "%s%s%s", html_tr_td, text_Zone, html_e_td_td);
          chprintf(chp, "%sP%s", html_select_submit, html_e_tag);
          for (uint8_t i = 0; i < ALARM_ZONES; i++) {
            if (GET_CONF_ZONE_IS_PRESENT(conf.zone[i])) {
              chprintf(chp, "%s%u", html_option, i);
              if (webZone == i) { chprintf(chp, "%s", html_selected); }
              else             { chprintf(chp, "%s", html_e_tag); }
              chprintf(chp, "%u. %s%s", i + 1, conf.zoneName[i], html_e_option);
            }
          }
          chprintf(chp, "%s%s", html_e_select, html_e_td_e_tr_tr_td);
          chprintf(chp, "%s%s", text_Name, html_e_td_td);
          printTextInput(chp, 'n', conf.zoneName[webZone], NAME_LENGTH);
          chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_Zone, text_is, html_e_td_td);
          printOnOffButton(chp, "0", GET_CONF_ZONE_ENABLED(conf.zone[webZone]));
          chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_Arm, text_home, html_e_td_td);
          printOnOffButton(chp, "7", GET_CONF_ZONE_ARM_HOME(conf.zone[webZone]));
          chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_Open, text_alarm, html_e_td_td);
          printOnOffButton(chp, "8", GET_CONF_ZONE_OPEN_ALARM(conf.zone[webZone]));
          chprintf(chp, "%s%s %s %s%s", html_e_td_e_tr_tr_td,  text_Alarm, text_as, text_tamper, html_e_td_td);
          printOnOffButton(chp, "9", GET_CONF_ZONE_PIR_AS_TMP(conf.zone[webZone]));
          chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, text_Balanced, html_e_td_td);
          printOnOffButton(chp, "a", GET_CONF_ZONE_BALANCED(conf.zone[webZone]));
          chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_Authentication, text_delay, html_e_td_td);
          printFourButton(chp, "D", GET_CONF_ZONE_AUTH_TIME(conf.zone[webZone]), 0, 0b0000,
                          text_0x, text_1x, text_2x, text_3x, 0);
          chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_MQTT, text_publish, html_e_td_td);
                    printOnOffButton(chp, "d", GET_CONF_ZONE_MQTT_PUB(conf.zone[webZone]));
          chprintf(chp, "%s%s %s %s%s", html_e_td_e_tr_tr_td, text_MQTT, text_HAD, text_Discovery, html_e_td_td);
                              printOnOffButton(chp, "c", GET_CONF_ZONE_MQTT_HAD(conf.zone[webZone]));
          chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, text_Group, html_e_td_td);
          selectGroup(chp, GET_CONF_ZONE_GROUP(conf.zone[webZone]), 'g');
          chprintf(chp, "%s%s", html_e_td_e_tr, html_e_table);
          // JavaScript
          chprintf(chp, "%s%s%s", html_script, JSZone, html_e_script);
          // Buttons
          chprintf(chp, "%s%s", html_Apply, html_Save);
          break;
        case PAGE_ALERT:
          chprintf(chp, "%s%s", html_tr_th, text_Name);
          for (uint8_t j = 0; j < ARRAY_SIZE(alertType); j++) {
            chprintf(chp, "%s%s", html_e_th_th, alertType[j].name);
          }
          chprintf(chp, "%s\r\n", html_e_th_e_tr);
          // Information table
          for (uint8_t i = 0; i < ARRAY_SIZE(alertDef); i++) {
            chprintf(chp, "%s", html_tr_td);
            decodeLog((char*)alertDef[i], logText, 0);
            chprintf(chp, "%s%s", logText, html_e_td_td);
            for (uint8_t j = 0; j < ARRAY_SIZE(alertType); j++) {
              temp[0] = '0' + j;
              temp[1] = 'A' + i;
              printOnOffButton(chp, temp, (conf.alert[j] >> i) & 0b1);
              if (j < (ARRAY_SIZE(alertType) - 1)) chprintf(chp, "%s", html_e_td_td);
            }
          }
          chprintf(chp, "%s%s", html_e_td_e_tr, html_e_table);
          // Buttons
          chprintf(chp, "%s%s", html_Apply, html_Save);
          break;
        case PAGE_LOG:
          chprintf(chp, "%s#", html_tr_th);
          chprintf(chp, "%s%s", html_e_th_th, text_Date);
          chprintf(chp, "%s%s", html_e_th_th, text_Entry);
          chprintf(chp, "%s%s%s\r\n", html_e_th_th, text_Alert, html_e_th_e_tr);

          // Information table
          spiAcquireBus(&SPID1);              // Acquire ownership of the bus.
          for (uint8_t i = 0; i < LOGGER_OUTPUT_LEN; i++) {
            logAddress = webLog + (i * FRAM_MSG_SIZE);
            chprintf(chp, "%s%u.%s", html_tr_td, (logAddress / FRAM_MSG_SIZE) + 1 , html_e_td_td);

            txBuffer[0] = CMD_25AA_READ;
            txBuffer[1] = 0;
            txBuffer[2] = (logAddress >> 8) & 0xFF;
            txBuffer[3] = logAddress & 0xFF;

            spiSelect(&SPID1);                  // Slave Select assertion.
            spiSend(&SPID1, FRAM_HEADER_SIZE, txBuffer); // Send read command
            spiReceive(&SPID1, FRAM_MSG_SIZE, rxBuffer);
            spiUnselect(&SPID1);                // Slave Select de-assertion.

            memcpy(&timeConv.ch[0], &rxBuffer[0], sizeof(timeConv.ch)); // Prepare timestamp
            decodeLog(&rxBuffer[4], logText, 1);

            printFrmTimestamp(chp, &timeConv.val);
            chprintf(chp, "%s%s.", html_e_td_td, logText);
            chprintf(chp, "%s", html_e_td_td);
            for (uint8_t j = 0; j < ARRAY_SIZE(alertType); j++) {
              if ((((uint8_t)rxBuffer[FRAM_MSG_SIZE-1] >> j) & 0b1) == 1)
                chprintf(chp, "%s ", alertType[j].name);
            }
          }
          spiReleaseBus(&SPID1);              // Ownership release.
          chprintf(chp, "%s%s", html_e_td_e_tr, html_e_table);
          // Buttons
          chprintf(chp, "%s%s%s", html_FR, html_Now, html_FF);
          break;
        case PAGE_GROUP:
          chprintf(chp, "%s#", html_tr_th);
          chprintf(chp, "%s%s", html_e_th_th, text_Name);
          chprintf(chp, "%s%s", html_e_th_th, text_On);
          chprintf(chp, "%s%s %s", html_e_th_th, text_Auto, text_arm);
          chprintf(chp, "%s%s %s", html_e_th_th, text_Arm, text_chain);
          chprintf(chp, "%s%s %s", html_e_th_th, text_Disarm, text_chain);
          chprintf(chp, "%s%ss", html_e_th_th, text_Zone);
          chprintf(chp, "%s%ss", html_e_th_th, text_Authentication);
          chprintf(chp, "%s%ss", html_e_th_th, text_Sensor);
          chprintf(chp, "%s%ss", html_e_th_th, text_Contact);
          chprintf(chp, "%s%s", html_e_th_th, text_Siren);
          chprintf(chp, "%s%s", html_e_th_th, text_MQTT);
          chprintf(chp, "%s%s", html_e_th_th, text_HAD);
          chprintf(chp, "%s%s", html_e_th_th, text_Armed);
          chprintf(chp, "%s%s%s\r\n", html_e_th_th, text_Status, html_e_th_e_tr);
          // Information table
          for (uint8_t i = 0; i < ALARM_GROUPS; i++) {
            chprintf(chp, "%s%u.%s", html_tr_td, i + 1, html_e_td_td);
            chprintf(chp, "%s%s", conf.group[i].name, html_e_td_td);
            printOkNok(chp, GET_CONF_GROUP_ENABLED(conf.group[i].setting));
            chprintf(chp, "%s", html_e_td_td);
            if (GET_CONF_GROUP_ENABLED(conf.group[i].setting))
              printOkNok(chp, GET_CONF_GROUP_AUTO_ARM(conf.group[i].setting));
            chprintf(chp, "%s", html_e_td_td);
            if (GET_CONF_GROUP_ENABLED(conf.group[i].setting))
              printGroup(chp, GET_CONF_GROUP_ARM_CHAIN(conf.group[i].setting));
            chprintf(chp, "%s", html_e_td_td);
            if (GET_CONF_GROUP_ENABLED(conf.group[i].setting))
              printGroup(chp, GET_CONF_GROUP_DISARM_CHAIN(conf.group[i].setting));
            chprintf(chp, "%s", html_e_td_td);
            if (GET_CONF_GROUP_ENABLED(conf.group[i].setting)) {
              logAddress = 0; // Just temp. var.
              for (uint8_t j = 0; j < ALARM_ZONES; j++) {
                if ((GET_CONF_ZONE_ENABLED(conf.zone[j])) &&
                    (GET_CONF_ZONE_IS_PRESENT(conf.zone[j])) &&
                    (GET_CONF_ZONE_GROUP(conf.zone[j]) == i)) {
                  if (logAddress > 0) chprintf(chp, "%s", html_br);
                  chprintf(chp, "%u. %s", j+1, conf.zoneName[j]);
                  logAddress++;
                }
              }
            }
            chprintf(chp, "%s", html_e_td_td);
            if (GET_CONF_GROUP_ENABLED(conf.group[i].setting)) {
              logAddress = 0; // Just temp. var.
              for (uint8_t j = 0; j < NODE_SIZE; j++) {
                if (GET_NODE_ENABLED(node[j].setting) &&
                    (GET_NODE_GROUP(node[j].setting) == i) &&
                    (node[j].type == 'K')) {
                  if (logAddress > 0) chprintf(chp, "%s", html_br);
                  chprintf(chp, "%u. %s", j+1, node[j].name);
                  logAddress++;
                }
              }
            }
            chprintf(chp, "%s", html_e_td_td);
            if (GET_CONF_GROUP_ENABLED(conf.group[i].setting)) {
              logAddress = 0; // Just temp. var.
              for (uint8_t j = 0; j < NODE_SIZE; j++) {
                if (GET_NODE_ENABLED(node[j].setting) &&
                    (GET_NODE_GROUP(node[j].setting) == i) &&
                    (node[j].type == 'S')) {
                  if (logAddress > 0) chprintf(chp, "%s", html_br);
                  chprintf(chp, "%u. %s", j+1, node[j].name);
                  logAddress++;
                }
              }
            }
            chprintf(chp, "%s", html_e_td_td);
            if (GET_CONF_GROUP_ENABLED(conf.group[i].setting)) {
              logAddress = 0; // Just temp. var.
              for (uint8_t j = 0; j < CONTACTS_SIZE; j++) {
                if (GET_CONF_CONTACT_ENABLED(conf.contact[j].setting) &&
                    ((GET_CONF_CONTACT_GROUP(conf.contact[j].setting) == i) ||
                     (GET_CONF_CONTACT_IS_GLOBAL(conf.contact[j].setting)))){
                  if (logAddress > 0) chprintf(chp, "%s", html_br);
                  chprintf(chp, "%s", conf.contact[j].name);
                  logAddress++;
                }
              }
            }
            chprintf(chp, "%s", html_e_td_td); // show relays
            if (GET_CONF_GROUP_ENABLED(conf.group[i].setting)) {
              chprintf(chp, "%s:", text_Alarm);
              if (GET_CONF_GROUP_PIR1(conf.group[i].setting)) chprintf(chp, " %s 1", text_relay);
              if (GET_CONF_GROUP_PIR2(conf.group[i].setting)) chprintf(chp, " %s 2", text_relay);
              chprintf(chp, "%s%s:", html_br, text_Tamper);
              if (GET_CONF_GROUP_TAMPER1(conf.group[i].setting)) chprintf(chp, " %s 1", text_relay);
              if (GET_CONF_GROUP_TAMPER2(conf.group[i].setting)) chprintf(chp, " %s 2", text_relay);
              // Remote Siren/Horn
              for (uint8_t j=0; j < NODE_SIZE; j++) {
                if ((node[j].type == 'H') && (GET_NODE_GROUP(node[j].setting) == i)){
                  chprintf(chp, "%s", html_br);
                  printNodeAddress(chp, node[j].address, node[j].type, node[j].function, node[j].number, 1);
                }
              }
            }
            chprintf(chp, "%s", html_e_td_td);
            if (GET_CONF_GROUP_ENABLED(conf.group[i].setting))
              printOkNok(chp, GET_CONF_GROUP_MQTT(conf.group[i].setting));
            chprintf(chp, "%s", html_e_td_td);
            if (GET_CONF_GROUP_ENABLED(conf.group[i].setting))
              printOkNok(chp, GET_CONF_GROUP_MQTT_HAD(conf.group[i].setting));
            chprintf(chp, "%s", html_e_td_td);
            if (GET_CONF_GROUP_ENABLED(conf.group[i].setting)) {
              if (GET_GROUP_ARMED(group[i].setting)) {
                if GET_GROUP_ARMED_HOME(group[i].setting) { chprintf(chp, "%s", html_i_home); }
                else                                      { chprintf(chp, "%s", html_i_OK); }
              } else {
                if (group[i].armDelay > 0) { chprintf(chp, "%s", html_i_starting); }
                else                       { chprintf(chp, "%s", html_i_disabled); }
              }
            }
            chprintf(chp, "%s", html_e_td_td);
            if (GET_CONF_GROUP_ENABLED(conf.group[i].setting)) {
              if (GET_GROUP_ALARM(group[i].setting) == 0) {
                if (GET_GROUP_WAIT_AUTH(group[i].setting)) { chprintf(chp, "%s", html_i_starting); }
                else                                       { chprintf(chp, "%s", html_i_OK); }
              } else { chprintf(chp, "%s", html_i_alarm); }
            }
            chprintf(chp, "%s", html_e_td_e_tr);
          } // end for ...
          chprintf(chp, "%s%s", html_e_table, html_table);
          chprintf(chp, "%s%s%s", html_tr_td, text_Group, html_e_td_td);
          chprintf(chp, "%sP%s", html_select_submit, html_e_tag);
          for (uint8_t i = 0; i < ALARM_GROUPS; i++) {
            chprintf(chp, "%s%u", html_option, i);
            if (webGroup == i) { chprintf(chp, "%s", html_selected); }
            else               { chprintf(chp, "%s", html_e_tag); }
            chprintf(chp, "%u. %s%s", i + 1, conf.group[i].name, html_e_option);
          }
          chprintf(chp, "%s%s", html_e_select, html_e_td_e_tr_tr_td);
          chprintf(chp, "%s%s", text_Name, html_e_td_td);
          printTextInput(chp, 'n', conf.group[webGroup].name, NAME_LENGTH);
          chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_Group, text_is, html_e_td_td);
          printOnOffButton(chp, "0", GET_CONF_GROUP_ENABLED(conf.group[webGroup].setting));
          chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_Auto, text_arm, html_e_td_td);
          printOnOffButton(chp, "5", GET_CONF_GROUP_AUTO_ARM(conf.group[webGroup].setting));
          chprintf(chp, "%s%s %ss %s 1%s", html_e_td_e_tr_tr_td,  text_Alarm, text_trigger, text_relay, html_e_td_td);
          printOnOffButton(chp, "4", GET_CONF_GROUP_PIR1(conf.group[webGroup].setting));
          chprintf(chp, "%s%s %ss %s 2%s", html_e_td_e_tr_tr_td,  text_Alarm, text_trigger, text_relay, html_e_td_td);
          printOnOffButton(chp, "3", GET_CONF_GROUP_PIR2(conf.group[webGroup].setting));
          chprintf(chp, "%s%s %ss %s 1%s", html_e_td_e_tr_tr_td,  text_Tamper, text_trigger, text_relay, html_e_td_td);
          printOnOffButton(chp, "2", GET_CONF_GROUP_TAMPER1(conf.group[webGroup].setting));
          chprintf(chp, "%s%s %ss %s 2%s", html_e_td_e_tr_tr_td,  text_Tamper, text_trigger, text_relay, html_e_td_td);
          printOnOffButton(chp, "1", GET_CONF_GROUP_TAMPER2(conf.group[webGroup].setting));
          chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td,  text_MQTT, text_publish, html_e_td_td);
          printOnOffButton(chp, "7", GET_CONF_GROUP_MQTT(conf.group[webGroup].setting));
          chprintf(chp, "%s%s %s %s%s", html_e_td_e_tr_tr_td,  text_MQTT, text_HA, text_Discovery, html_e_td_td);
          printOnOffButton(chp, "6", GET_CONF_GROUP_MQTT_HAD(conf.group[webGroup].setting));
          chprintf(chp, "%s%s %s %s%s", html_e_td_e_tr_tr_td, text_Arm, text_group, text_chain, html_e_td_td);
          selectGroup(chp, GET_CONF_GROUP_ARM_CHAIN(conf.group[webGroup].setting), 'a');
          chprintf(chp, "%s%s %s %s%s", html_e_td_e_tr_tr_td, text_Disarm, text_group, text_chain, html_e_td_td);
          selectGroup(chp, GET_CONF_GROUP_DISARM_CHAIN(conf.group[webGroup].setting), 'd');
          chprintf(chp, "%s%s", html_e_td_e_tr, html_e_table);
          // Buttons
          chprintf(chp, "%s%s%s", html_Apply, html_Save, html_Disarm);
          break;
        case PAGE_HOME:
          // Information table
          chprintf(chp, "%s%s%s", html_tr_td, text_Time, html_e_td_td);
          tempTime = getTimeUnixSec(); printFrmTimestamp(chp, &tempTime);
          chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_Start, text_time, html_e_td_td);
          printFrmTimestamp(chp, &startTime);
          chprintf(chp, "%s%s%s%s", html_e_td_e_tr_tr_td, text_Up, text_time, html_e_td_td);
          tempTime -= startTime; printFrmUpTime(chp, &tempTime);
          chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_AC, text_power, html_e_td_td);
          printOkNok(chp, !(palReadPad(GPIOD, GPIOD_AC_OFF)));
          chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, text_Battery, html_e_td_td);
          printOkNok(chp, palReadPad(GPIOD, GPIOD_BAT_OK));
          chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_RTC, text_battery, html_e_td_td);
          printOkNok(chp, ((rtcVbat > ADC_VBAT_HIGH_VOLTAGE)?1:0));
          chprintf(chp, " (%.2f V)", rtcVbat);
          chprintf(chp, "%s%s", html_e_td_e_tr, html_e_table);
          chprintf(chp, "<h1>%s</h1>\r\n", text_Modem);
          chprintf(chp, "%s%s", html_table, html_tr_td);
          chprintf(chp, "%s%s%s", text_Type, html_e_td_td, gprsModemInfo);
          chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, text_On, html_e_td_td);
          printOkNok(chp, !palReadPad(GPIOD, GPIOD_GSM_STATUS));
          chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, text_Alive, html_e_td_td);
          printOkNok(chp, gprsIsAlive);
          chprintf(chp, "%s%sed%s", html_e_td_e_tr_tr_td, text_Register, html_e_td_td);
          switch(gprsReg){
            case 0 : chprintf(chp, "%s", html_i_OK); break;
            case 1 : chprintf(chp, "%s", html_i_home); break;
            case 2 : chprintf(chp, "%s", html_i_starting); break;
            case 3 : chprintf(chp, "%s", html_i_disabled); break;
            case 5 : chprintf(chp, "%s", html_i_globe); break;
            default : chprintf(chp, "%s", html_i_question);; break; // case 4
          }
          chprintf(chp, "%s%s %s%s%u%%", html_e_td_e_tr_tr_td, text_Signal, text_strength, html_e_td_td, gprsStrength);
          // +CPSI: GSM,Online,230-02,0x0726,4285,69 // remove +CPSI: &[7]
          chprintf(chp, "%s%s %s%s%s%", html_e_td_e_tr_tr_td, text_System, text_info, html_e_td_td, &gprsSystemInfo[7]);
          chprintf(chp, "%s%s", html_e_td_e_tr, html_e_table);
          // Buttons
          chprintf(chp, "%s%s", html_LoadDefault, html_Save);
          // TODO OHS add configuration download/upload
          //chprintf(chp, "<a href='config.bin' download>");
          break;
        case PAGE_SETTING:
          // Information table
          chprintf(chp, "%s%s / %s %s%s", html_tr_td, text_Arm, text_Authentication, text_time,
                   html_e_td_td);
          printIntInput(chp, 'C', conf.armDelay / 4, 3, 5, 60);
          chprintf(chp, " %s%s%s %s %s %s%s", durationSelect[0], html_e_td_e_tr_tr_td, text_Auto,
                   text_arm, text_zone, text_delay, html_e_td_td);
          printIntInput(chp, 'E', conf.autoArm, 3, 1, 240);
          chprintf(chp, " %s%s%s %s %s %s%s", durationSelect[1], html_e_td_e_tr_tr_td, text_Zone,
                   text_open, text_alarm, text_delay, html_e_td_td);
          printIntInput(chp, 'F', conf.openAlarm, 3, 1, 240);
          chprintf(chp, " %s%s%s %s%s", durationSelect[1], html_e_td_e_tr_tr_td, text_Admin,
                   text_user, html_e_td_td);
          printTextInputWMin(chp, 'u', conf.user, NAME_LENGTH, MIN_PASS_LNEGTH);
          chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_Admin, text_password,
                   html_e_td_td);
          printPassInput(chp, 'p', conf.password, NAME_LENGTH, MIN_PASS_LNEGTH); chprintf(chp, "%s", html_br);
          printPassInput(chp, 'P', conf.password, NAME_LENGTH, MIN_PASS_LNEGTH);
          chprintf(chp, "%s%s", html_e_td_e_tr, html_e_table);

          chprintf(chp, "<h1>%s</h1>\r\n%s", text_Radio, html_table);
          chprintf(chp, "%s%s%s", html_tr_td, text_Key, html_e_td_td);
          printTextInputWMin(chp, 'K', conf.radioKey, RADIO_KEY_SIZE - 1, RADIO_KEY_SIZE - 1);
          chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, text_Frequency, html_e_td_td);
          printTwoButton(chp, "i", GET_CONF_SYSTEM_FLAG_RADIO_FREQ(conf.systemFlags), 0, 0b00,
                                   "868 Mhz", "915 Mhz");
          chprintf(chp, "%s%s", html_e_td_e_tr, html_e_table);

          chprintf(chp, "<h1>%s</h1>\r\n%s", text_SMTP, html_table);
          chprintf(chp, "%s%s %s%s", html_tr_td, text_Server, text_address, html_e_td_td);
          printTextInput(chp, 'a', conf.SMTPAddress, URL_LENGTH);
          chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_Server, text_port, html_e_td_td);
          printIntInput(chp, 'b', conf.SMTPPort, 5, 0, 65535);
          chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_User, text_name, html_e_td_td);
          printTextInput(chp, 'c', conf.SMTPUser, EMAIL_LENGTH);
          chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_User, text_password, html_e_td_td);
          printPassInput(chp, 'd', conf.SMTPPassword, NAME_LENGTH, MIN_PASS_LNEGTH);
          // chprintf(chp, "%s", html_br); printPassInput(chp, 'D', conf.SMTPPassword, NAME_LENGTH, MIN_PASS_LNEGTH);
          chprintf(chp, "%s%s", html_e_td_e_tr, html_e_table);

          chprintf(chp, "<h1>%s</h1>\r\n%s", text_MQTT, html_table);
          chprintf(chp, "%s%s %s%s", html_tr_td, text_Server, text_address, html_e_td_td);
          printTextInput(chp, 'y', conf.mqtt.address, URL_LENGTH);
          chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_Server, text_port, html_e_td_td);
          printIntInput(chp, 'q', conf.mqtt.port, 5, 0, 65535);
          chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_User, text_name, html_e_td_td);
          printTextInput(chp, 't', conf.mqtt.user, NAME_LENGTH);
          chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_User, text_password, html_e_td_td);
          printPassInput(chp, 'r', conf.mqtt.password, NAME_LENGTH, MIN_PASS_LNEGTH);
          // chprintf(chp, "%s", html_br); printPassInput(chp, 'R', conf.mqtt.password, NAME_LENGTH, MIN_PASS_LNEGTH);
          chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, text_Connected, html_e_td_td);
          printOkNok(chp, !(GET_CONF_MQTT_CONNECT_ERROR(conf.mqtt.setting) | GET_CONF_MQTT_ADDRESS_ERROR(conf.mqtt.setting)));
          chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_Global, text_Subscribe, html_e_td_td);
          printOnOffButton(chp, "0", GET_CONF_MQTT_SUBSCRIBE(conf.mqtt.setting));
          chprintf(chp, "%s%s %s %s %s%s", html_e_td_e_tr_tr_td, text_Global, text_Home, text_Assistant, text_Discovery, html_e_td_td);
          printOnOffButton(chp, "1", GET_CONF_MQTT_HAD(conf.mqtt.setting));
          chprintf(chp, "%s%s", html_e_td_e_tr, html_e_table);
          chprintf(chp, "<h1>%s</h1>\r\n%s", text_NTP, html_table);
          chprintf(chp, "%s%s %s%s", html_tr_td, text_Server, text_address,
                   html_e_td_td);
          printTextInput(chp, 'f', conf.SNTPAddress, URL_LENGTH);

          chprintf(chp, "%s%s %s%s", html_tr_td, text_DS, text_start, html_e_td_td);
          chprintf(chp, "%sW%s", html_select, html_e_tag);
          for (uint8_t i = 0; i < ARRAY_SIZE(weekNumber); i++) {
            chprintf(chp, "%s%u", html_option, i);
            if (conf.timeDstWeekNum == i) { chprintf(chp, "%s", html_selected); }
            else                          { chprintf(chp, "%s", html_e_tag); }
            chprintf(chp, "%s%s", weekNumber[i], html_e_option);
          }
          chprintf(chp, "%s", html_e_select);
          chprintf(chp, "%sS%s", html_select, html_e_tag);
          for (uint8_t i = 0; i < ARRAY_SIZE(weekDay); i++) {
            chprintf(chp, "%s%u", html_option, i);
            if (conf.timeDstDow == i) { chprintf(chp, "%s", html_selected); }
            else                      { chprintf(chp, "%s", html_e_tag); }
            chprintf(chp, "%s%s", weekDay[i], html_e_option);
          }
          chprintf(chp, "%s %s ", html_e_select, text_of);
          chprintf(chp, "%sM%s", html_select, html_e_tag);
          for (uint8_t i = 0; i < ARRAY_SIZE(monthName); i++) {
            chprintf(chp, "%s%u", html_option, i);
            if (conf.timeDstMonth == i) { chprintf(chp, "%s", html_selected); }
            else                        { chprintf(chp, "%s", html_e_tag); }
            chprintf(chp, "%s%s", monthName[i], html_e_option);
          }
          chprintf(chp, "%s %s ", html_e_select, text_at);
          printIntInput(chp, 'h', conf.timeDstHour , 2, 0, 23);
          chprintf(chp, " %s", text_oclock);
          chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_DS, text_offset, html_e_td_td);
          printIntInput(chp, 'O', conf.timeDstOffset, 5, -1440, 1440);
          chprintf(chp, " %s%s", durationSelect[1], html_e_td_e_tr_tr_td);

          chprintf(chp, "%s %s%s", text_DS, text_end, html_e_td_td);
          chprintf(chp, "%sw%s", html_select, html_e_tag);
          for (uint8_t i = 0; i < ARRAY_SIZE(weekNumber); i++) {
            chprintf(chp, "%s%u", html_option, i);
            if (conf.timeStdWeekNum == i) { chprintf(chp, "%s", html_selected); }
            else                          { chprintf(chp, "%s", html_e_tag); }
            chprintf(chp, "%s%s", weekNumber[i], html_e_option);
          }
          chprintf(chp, "%s", html_e_select);
          chprintf(chp, "%ss%s", html_select, html_e_tag);
          for (uint8_t i = 0; i < ARRAY_SIZE(weekDay); i++) {
            chprintf(chp, "%s%u", html_option, i);
            if (conf.timeStdDow == i) { chprintf(chp, "%s", html_selected); }
            else                      { chprintf(chp, "%s", html_e_tag); }
            chprintf(chp, "%s%s", weekDay[i], html_e_option);
          }
          chprintf(chp, "%s %s ", html_e_select, text_of);
          chprintf(chp, "%sm%s", html_select, html_e_tag);
          for (uint8_t i = 0; i < ARRAY_SIZE(monthName); i++) {
            chprintf(chp, "%s%u", html_option, i);
            if (conf.timeStdMonth == i) { chprintf(chp, "%s", html_selected); }
            else                        { chprintf(chp, "%s", html_e_tag); }
            chprintf(chp, "%s%s", monthName[i], html_e_option);
          }
          chprintf(chp, "%s %s ", html_e_select, text_at);
          printIntInput(chp, 'h', conf.timeStdHour , 2, 0, 23);
          chprintf(chp, " %s", text_oclock);
          chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td,
                   text_Standard, text_offset, html_e_td_td);
          printIntInput(chp, 'o', conf.timeStdOffset, 5, -1440, 1440);
          chprintf(chp, " %s%s", durationSelect[1], html_e_td_e_tr_tr_td);
          chprintf(chp, "%s %s%s", text_Time, text_format, html_e_td_td);
          printTextInput(chp, 'g', conf.dateTimeFormat, NAME_LENGTH);
          chprintf(chp, "%s%s", html_e_td_e_tr, html_e_table);

          // JavaScript
          chprintf(chp, "%s%s%s", html_script, JSCredential, html_e_script);
          chprintf(chp, "%sPass.js'>%s", html_script_src, html_e_script);
          // Buttons
          chprintf(chp, "%s%s", html_ApplyValPass, html_Save);
          break;
        case PAGE_TCL:
          // Information table - TCL heap
          chprintf(chp, "%s%s %u %s", html_tr_th, text_Heap, UMM_MALLOC_CFG_HEAP_SIZE/1024, text_kB);
          chprintf(chp, "%s%s", html_e_th_th, text_Used);
          chprintf(chp, "%s%s", html_e_th_th, text_Free);
          chprintf(chp, "%s%s", html_e_th_th, text_Total);
          chprintf(chp, "%s%s%s", html_e_th_th, text_Metric, html_e_th_e_tr);
          chprintf(chp, "%s%s%s", html_tr_td, text_Entries, html_e_td_td);
          chprintf(chp, "%u%s", ummHeapInfo.usedEntries, html_e_td_td);
          chprintf(chp, "%u%s", ummHeapInfo.freeEntries, html_e_td_td);
          chprintf(chp, "%u%s", ummHeapInfo.totalEntries, html_e_td_td);
          chprintf(chp, "%s %u%%%s", text_Fragmentation, umm_fragmentation_metric(), html_e_td_e_tr_tr_td);
          chprintf(chp, "%s%s", text_Blocks, html_e_td_td);
          chprintf(chp, "%u%s", ummHeapInfo.usedBlocks, html_e_td_td);
          chprintf(chp, "%u%s", ummHeapInfo.freeBlocks, html_e_td_td);
          chprintf(chp, "%u%s", ummHeapInfo.totalBlocks, html_e_td_td);
          chprintf(chp, "%s %u%%", text_Used, umm_usage_metric());
          chprintf(chp, "%s%s %u %s", html_tr_th, text_Storage, (UBS_BLOCK_SIZE * UBS_BLOCK_COUNT)/1024, text_kB);
          chprintf(chp, "%s%s", html_e_th_th, text_Used);
          chprintf(chp, "%s%s", html_e_th_th, text_Free);
          chprintf(chp, "%s%s", html_e_th_th, text_Total);
          chprintf(chp, "%s%s%s", html_e_th_th, text_Metric, html_e_th_e_tr);
          chprintf(chp, "%s%s%s", html_tr_td, text_Blocks, html_e_td_td);
          chprintf(chp, "%u%s", UBS_BLOCK_COUNT - uBSFreeBlocks, html_e_td_td);
          chprintf(chp, "%u%s", uBSFreeBlocks, html_e_td_td);
          chprintf(chp, "%u%s", UBS_BLOCK_COUNT, html_e_td_td);
          chprintf(chp, "%s %u%%", text_Used, (uint8_t)((UBS_BLOCK_COUNT - uBSFreeBlocks) / (float)(UBS_BLOCK_COUNT / (float)100)));
          chprintf(chp, "%s%s", html_e_td_e_tr, html_e_table);

          chprintf(chp, "<i class='icon' style='font-size:20px;' title=\"");
          tcl_list_cmd(&tcl, &chp, "\r\n", 0b00111100);
          chprintf(chp, "\">&#xf121;</i>");
          chprintf(chp, "<i class='icon' style='font-size:20px;' title=\"");
          tcl_list_var(&tcl, &chp, "\r\n");
          chprintf(chp, "\">&#xf292;</i>");
          chprintf(chp, "%s%s", html_br, html_br);
          // Script area
          printTextArea(chp, 's', tclCmd, TCL_SCRIPT_LENGTH, 120, 20);
          chprintf(chp, "%s%s", html_br, html_br);
          // Select script
          chprintf(chp, "%s%s%s%s", html_table, html_tr_td, text_Script, html_e_td_td);

          chprintf(chp, "%sP%s", html_select_submit, html_e_tag);
          logAddress = 1;
          for (scriptp = scriptLL; scriptp != NULL; scriptp = scriptp->next) {
            chprintf(chp, "%s%u", html_option, logAddress);
            if (webScript == logAddress) { chprintf(chp, "%s", html_selected); }
            else                         { chprintf(chp, "%s", html_e_tag); }
            chprintf(chp, "%u. %s%s", logAddress, scriptp->name, html_e_option);
            logAddress++;
          }
          chprintf(chp, "%s%u", html_option, DUMMY_NO_VALUE);
          if (webScript == DUMMY_NO_VALUE) { chprintf(chp, "%s", html_selected); }
          else                             { chprintf(chp, "%s", html_e_tag); }
          chprintf(chp, "New script%s", html_e_option);
          chprintf(chp, "%s%s", html_e_select, html_e_td_e_tr_tr_td);

          chprintf(chp, "%s%s", text_Name, html_e_td_td);
          printTextInput(chp, 'n', scriptName, NAME_LENGTH);
          chprintf(chp, "%s%s", html_e_td_e_tr, html_e_table);
          // Buttons
          chprintf(chp, "%s%s%s%s", html_Run, html_Refresh, html_Save, html_Restart);
          // Output
          chprintf(chp, "%s<pre>Last output:\r\n%.*s</pre>", html_br, strlen(tclOutput), tclOutput);
          //for (uint16_t j = 0; j < strlen(tclOutput); j++) { DBG_HTTP("-%x", tclOutput[j]); }
          //DBG_HTTP("\r\n");
          break;
        case PAGE_TIMER:
          chprintf(chp, "%s#", html_tr_th);
          chprintf(chp, "%s%s", html_e_th_th, text_Name);
          chprintf(chp, "%s%s", html_e_th_th, text_On);
          chprintf(chp, "%s%s", html_e_th_th, text_Type);
          chprintf(chp, "%s%s", html_e_th_th, text_Start);
          chprintf(chp, "%s%s", html_e_th_th, text_Period);
          chprintf(chp, "%s%s %s", html_e_th_th, text_Run, text_time);
          chprintf(chp, "%s%s %s", html_e_th_th, text_Next, text_on);
          chprintf(chp, "%s%s %s", html_e_th_th, text_Next, text_off);
          chprintf(chp, "%s%s", html_e_th_th, text_Address);
          chprintf(chp, "%s%s", html_e_th_th, text_Script);
          chprintf(chp, "%s%s", html_e_th_th, text_On);
          chprintf(chp, "%s%s", html_e_th_th, text_Off);
          chprintf(chp, "%s%s%s\r\n", html_e_th_th, text_Status, html_e_th_e_tr);
          // Information table
          for (uint8_t i = 0; i < TIMER_SIZE; i++) {
            chprintf(chp, "%s%u.%s", html_tr_td, i + 1, html_e_td_td);
            chprintf(chp, "%s%s", conf.timer[i].name, html_e_td_td);
            printOkNok(chp, GET_CONF_TIMER_ENABLED(conf.timer[i].setting));
            chprintf(chp, "%s", html_e_td_td);
            if (GET_CONF_TIMER_TYPE(conf.timer[i].setting)) {
              chprintf(chp, "%s %s", text_Run, text_on);
              for (uint8_t j = 0; j < ARRAY_SIZE(weekDayShort); j++) {
                if ((conf.timer[i].setting >> (8 - j)) & 0b1) {
                  chprintf(chp, " %s", weekDayShort[j]);
                }
              }
            } else {
              chprintf(chp, "%s", text_Period);
            }
            chprintf(chp, "%s", html_e_td_td);
            chprintf(chp, "%02u:%02u%s", conf.timer[i].startTime / SECONDS_PER_MINUTE,
                     conf.timer[i].startTime % SECONDS_PER_MINUTE, html_e_td_td);
            if (!GET_CONF_TIMER_TYPE(conf.timer[i].setting)) {
              chprintf(chp, "%u %s", conf.timer[i].periodTime,
                       durationSelect[GET_CONF_TIMER_PERIOD_TYPE(conf.timer[i].setting)]);
            }
            chprintf(chp, "%s%u %s", html_e_td_td,  conf.timer[i].runTime,
                     durationSelect[GET_CONF_TIMER_RUN_TYPE(conf.timer[i].setting)]);
            chprintf(chp, "%s", html_e_td_td);
            tempTime = conf.timer[i].nextOn;
            if (GET_CONF_TIMER_ENABLED(conf.timer[i].setting)) printFrmTimestamp(chp, &tempTime);
            chprintf(chp, "%s", html_e_td_td);
            tempTime = conf.timer[i].nextOff;
            if (GET_CONF_TIMER_ENABLED(conf.timer[i].setting)) printFrmTimestamp(chp, &tempTime);
            chprintf(chp, "%s", html_e_td_td);
            printNodeAddress(chp, conf.timer[i].toAddress, 'I',  conf.timer[i].toFunction,
                             conf.timer[i].toNumber, true);
            chprintf(chp, "%s", html_e_td_td);
            if (conf.timer[i].evalScript[0]) chprintf(chp, "%s", conf.timer[i].evalScript);
            else chprintf(chp, "%s", NOT_SET);
            chprintf(chp, "%s", html_e_td_td);
            chprintf(chp, "%.2f%s", conf.timer[i].constantOn, html_e_td_td);
            chprintf(chp, "%.2f%s", conf.timer[i].constantOff, html_e_td_td);
            printOkNok(chp, GET_CONF_TIMER_TRIGGERED(conf.timer[i].setting));
            chprintf(chp, "%s", html_e_td_e_tr);
          }
          chprintf(chp, "%s%s", html_e_table, html_table);
          chprintf(chp, "%s%s%s", html_tr_td, text_Timer, html_e_td_td);
          chprintf(chp, "%sP%s", html_select_submit, html_e_tag);
          for (uint8_t i = 0; i < TIMER_SIZE; i++) {
            chprintf(chp, "%s%u", html_option, i);
            if (webTimer == i) { chprintf(chp, "%s", html_selected); }
            else               { chprintf(chp, "%s", html_e_tag); }
            chprintf(chp, "%u. %s%s", i + 1, conf.timer[i].name, html_e_option);
          }
          chprintf(chp, "%s%s", html_e_select, html_e_td_e_tr_tr_td);
          chprintf(chp, "%s%s", text_Name, html_e_td_td);
          printTextInput(chp, 'n', conf.timer[webTimer].name, NAME_LENGTH);
          chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_Timer, text_is, html_e_td_td);
          printOnOffButton(chp, "B", GET_CONF_TIMER_ENABLED(conf.timer[webTimer].setting));
          chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, text_Type, html_e_td_td);
          printTwoButton(chp, "C", GET_CONF_TIMER_TYPE(conf.timer[webTimer].setting), 1, 0b10,
                         text_Period, text_Calendar);
          chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, weekDay[0], html_e_td_td);
          printOnOffButton(chp, "J", GET_CONF_TIMER_MO(conf.timer[webTimer].setting));
          chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, weekDay[1], html_e_td_td);
          printOnOffButton(chp, "I", GET_CONF_TIMER_TU(conf.timer[webTimer].setting));
          chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, weekDay[2], html_e_td_td);
          printOnOffButton(chp, "H", GET_CONF_TIMER_WE(conf.timer[webTimer].setting));
          chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, weekDay[3], html_e_td_td);
          printOnOffButton(chp, "G", GET_CONF_TIMER_TH(conf.timer[webTimer].setting));
          chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, weekDay[4], html_e_td_td);
          printOnOffButton(chp, "F", GET_CONF_TIMER_FR(conf.timer[webTimer].setting));
          chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, weekDay[5], html_e_td_td);
          printOnOffButton(chp, "E", GET_CONF_TIMER_SA(conf.timer[webTimer].setting));
          chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, weekDay[6], html_e_td_td);
          printOnOffButton(chp, "D", GET_CONF_TIMER_SU(conf.timer[webTimer].setting));
          chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_Start, text_time, html_e_td_td);
          printTimeInput(chp, 't', conf.timer[webTimer].startTime / 60, conf.timer[webTimer].startTime % 60);
          chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_Period, text_duration, html_e_td_td);
          printIntInput(chp, 's', conf.timer[webTimer].periodTime, 3, 1, 255);
          printDurationSelect(chp, 'S', GET_CONF_TIMER_PERIOD_TYPE(conf.timer[webTimer].setting));
          chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_Run, text_duration, html_e_td_td);
          printIntInput(chp, 'r', conf.timer[webTimer].runTime, 3, 1, 255);
          printDurationSelect(chp, 'R', GET_CONF_TIMER_RUN_TYPE(conf.timer[webTimer].setting));
          chprintf(chp, "%s%s%s", html_tr_td, text_Address, html_e_td_td);
          chprintf(chp, "%sa%s", html_select, html_e_tag);
          for (uint8_t i = 0; i <= NODE_SIZE; i++) {
            if (i < NODE_SIZE) {
              if (node[i].type == 'I') {
                chprintf(chp, "%s%u", html_option, i);
                if ((node[i].address  == conf.timer[webTimer].toAddress) &&
                    (node[i].function == conf.timer[webTimer].toFunction) &&
                    (node[i].number   == conf.timer[webTimer].toNumber))
                     { chprintf(chp, "%s", html_selected); }
                else { chprintf(chp, "%s", html_e_tag); }
                printNodeAddress(chp, node[i].address, node[i].type, node[i].function, node[i].number, true);
                chprintf(chp, "%s", html_e_option);
              }
            } else {
              chprintf(chp, "%s%u", html_option, DUMMY_NO_VALUE);
              if ((conf.timer[webTimer].toAddress  == 0) &&
                  (conf.timer[webTimer].toFunction == ' ') &&
                  (conf.timer[webTimer].toNumber   == 0))
                   { chprintf(chp, "%s", html_selected); }
              else { chprintf(chp, "%s", html_e_tag); }
              chprintf(chp, "%s%s", NOT_SET, html_e_option);
            }
          }
          chprintf(chp, "%s", html_e_select);
          chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_Evaluate, text_script, html_e_td_td);
          chprintf(chp, "%sp%s", html_select, html_e_tag);
          logAddress = 1;
          for (scriptp = scriptLL; scriptp != NULL; scriptp = scriptp->next) {
            chprintf(chp, "%s%s", html_option, scriptp->name);
            if (strcmp(&conf.timer[webTimer].evalScript[0], scriptp->name) == 0) {
              chprintf(chp, "%s", html_selected);
              logAddress = 0;
            } else {
              chprintf(chp, "%s", html_e_tag);
            }
            chprintf(chp, "%s%s", scriptp->name, html_e_option);
          }
          chprintf(chp, "%s", html_option);
          if (logAddress) { chprintf(chp, "%s", html_selected); }
          else            { chprintf(chp, "%s", html_e_tag); }
          chprintf(chp, "%s%s%s", NOT_SET, html_e_option, html_e_select);
          chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_On, text_value, html_e_td_td);
          printFloatInput(chp, 'o', conf.timer[webTimer].constantOn);
          chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_Off, text_value, html_e_td_td);
          printFloatInput(chp, 'f', conf.timer[webTimer].constantOff);
          chprintf(chp, "%s%s", html_e_td_e_tr, html_e_table);
          // JavaScript
          chprintf(chp, "%s%s%s", html_script, JSTimer, html_e_script);
          // Buttons
          chprintf(chp, "%s%s", html_Apply, html_Save);
          break;
        case PAGE_TRIGGER:
          chprintf(chp, "%s#", html_tr_th);
          chprintf(chp, "%s%s", html_e_th_th, text_Name);
          chprintf(chp, "%s%s", html_e_th_th, text_On);
          chprintf(chp, "%s%s", html_e_th_th, text_Type);
          chprintf(chp, "%s%s", html_e_th_th, text_Condition);
          chprintf(chp, "%s%s", html_e_th_th, text_Value);
          chprintf(chp, "%s%s", html_e_th_th, text_Hysteresis);
          chprintf(chp, "%s%s", html_e_th_th, text_Script);
          chprintf(chp, "%s%s", html_e_th_th, text_Alert);
          chprintf(chp, "%s%s", html_e_th_th, text_Pass);
          chprintf(chp, "%s%s %s", html_e_th_th, text_Pass, text_Off);
          chprintf(chp, "%s%s", html_e_th_th, text_To);
          chprintf(chp, "%s%s", html_e_th_th, text_On);
          chprintf(chp, "%s%s", html_e_th_th, text_Off);
          chprintf(chp, "%s%s%s\r\n", html_e_th_th, text_Status, html_e_th_e_tr);
          // Information table
          for (uint8_t i = 0; i < TRIGGER_SIZE; i++) {
            chprintf(chp, "%s%u.%s", html_tr_td, i + 1, html_e_td_td);
            chprintf(chp, "%s%s", conf.trigger[i].name, html_e_td_td);
            printOkNok(chp, GET_CONF_TRIGGER_ENABLED(conf.trigger[i].setting));
            chprintf(chp, "%s", html_e_td_td);
            if (GET_CONF_TRIGGER_ENABLED(conf.trigger[i].setting)) {
              switch (conf.trigger[i].type) {
                case 'G': chprintf(chp, "%s - ", text_Group);
                  printGroup(chp, conf.trigger[i].number);
                  break;
                case 'Z': chprintf(chp, "%s - ", text_Zone);
                  printZone(chp, conf.trigger[i].number);
                  break;
                default: printNodeAddress(chp, conf.trigger[i].address, 'S', conf.trigger[i].function,
                                          conf.trigger[i].number, true);
                  break;
              }
            }
            chprintf(chp, "%s", html_e_td_td);
            if (GET_CONF_TRIGGER_ENABLED(conf.trigger[i].setting))
              chprintf(chp, "%s", triggerCondition[conf.trigger[i].condition]);
            chprintf(chp, "%s", html_e_td_td);
            if (GET_CONF_TRIGGER_ENABLED(conf.trigger[i].setting)) {
              switch (conf.trigger[i].type) {
                case 'G': chprintf(chp, "%s", groupState[(uint8_t)conf.trigger[i].value]);
                  break;
                case 'Z': chprintf(chp, "%s", zoneState[(uint8_t)conf.trigger[i].value]);
                  break;
                default: chprintf(chp, "%.2f", conf.trigger[i].value);
                  break;
              }
            }
            chprintf(chp, "%s", html_e_td_td);
            if (GET_CONF_TRIGGER_ENABLED(conf.trigger[i].setting)) {
              if (conf.trigger[i].type == 'S') chprintf(chp, "%.2f", conf.trigger[i].hysteresis);
            }
            chprintf(chp, "%s", html_e_td_td);
            if (GET_CONF_TRIGGER_ENABLED(conf.trigger[i].setting)) {
              if (conf.trigger[i].evalScript[0]) chprintf(chp, "%s", conf.trigger[i].evalScript);
              else chprintf(chp, "%s", NOT_SET);
            }
            chprintf(chp, "%s", html_e_td_td);
            if (GET_CONF_TRIGGER_ENABLED(conf.trigger[i].setting))
              printOkNok(chp, GET_CONF_TRIGGER_ALERT(conf.trigger[i].setting));
            chprintf(chp, "%s", html_e_td_td);

            if (GET_CONF_TRIGGER_ENABLED(conf.trigger[i].setting)) {
              if (GET_CONF_TRIGGER_PASS(conf.trigger[i].setting)) {
                if (GET_CONF_TRIGGER_PASS_VALUE(conf.trigger[i].setting)) chprintf(chp, "%s", text_value);
                else chprintf(chp, "%s", text_constant);
                // once
                if (GET_CONF_TRIGGER_PASS(conf.trigger[i].setting) > 1) {
                  chprintf(chp, " %s", triggerPassType[GET_CONF_TRIGGER_PASS(conf.trigger[i].setting)]);
                }
              }
            }
            chprintf(chp, "%s", html_e_td_td);
            // Pass off
            if (GET_CONF_TRIGGER_ENABLED(conf.trigger[i].setting)) {
              if (GET_CONF_TRIGGER_PASS(conf.trigger[i].setting)) {
                if (GET_CONF_TRIGGER_PASS_OFF(conf.trigger[i].setting) < 2) {
                  chprintf(chp, "%s", triggerPassOffType[GET_CONF_TRIGGER_PASS_OFF(conf.trigger[i].setting)]);
                } else {
                  chprintf(chp, "%s %u %s", text_after, conf.trigger[i].offTime,
                  durationSelect[GET_CONF_TRIGGER_OFF_PERIOD(conf.trigger[i].setting)]);
                }
              }
            }
            chprintf(chp, "%s", html_e_td_td);
            if (GET_CONF_TRIGGER_ENABLED(conf.trigger[i].setting)) {
              printNodeAddress(chp, conf.trigger[i].toAddress, 'I',  conf.trigger[i].toFunction,
                               conf.trigger[i].toNumber, true);
            }
            chprintf(chp, "%s", html_e_td_td);
            chprintf(chp, "%.2f%s", conf.trigger[i].constantOn, html_e_td_td);
            chprintf(chp, "%.2f%s", conf.trigger[i].constantOff, html_e_td_td);
            printOkNok(chp, GET_CONF_TRIGGER_TRIGGERED(conf.trigger[i].setting));
            chprintf(chp, "%s", html_e_td_e_tr);
          }
          chprintf(chp, "%s%s", html_e_table, html_table);
          // Trigger options
          chprintf(chp, "%s%s%s", html_tr_td, text_Trigger, html_e_td_td);
          chprintf(chp, "%sP%s", html_select_submit, html_e_tag);
          for (uint8_t i = 0; i < TRIGGER_SIZE; i++) {
            chprintf(chp, "%s%u", html_option, i);
            if (webTrigger == i) { chprintf(chp, "%s", html_selected); }
            else                 { chprintf(chp, "%s", html_e_tag); }
            chprintf(chp, "%u. %s%s", i + 1, conf.trigger[i].name, html_e_option);
          }
          chprintf(chp, "%s%s", html_e_select, html_e_td_e_tr_tr_td);
          chprintf(chp, "%s%s", text_Name, html_e_td_td);
          printTextInput(chp, 'n', conf.trigger[webTrigger].name, NAME_LENGTH);
          chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_Trigger, text_is, html_e_td_td);
          printOnOffButton(chp, "B", GET_CONF_TRIGGER_ENABLED(conf.trigger[webTrigger].setting));
          chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, text_Type, html_e_td_td);
          chprintf(chp, "%sy%sy' onchange='sd(this)%s", html_select, html_id_tag, html_e_tag);
          logAddress = 0; number = 0;
          for (uint8_t i = 0; i < ALARM_GROUPS; i++) {
            if (GET_CONF_GROUP_ENABLED(conf.group[i].setting)) {
              chprintf(chp, "%s%u", html_option, logAddress);
              if ((conf.trigger[webTrigger].type == 'G') && (conf.trigger[webTrigger].number == i)) {
                chprintf(chp, "%s", html_selected);
                number = 1;
              } else { chprintf(chp, "%s", html_e_tag); }
              chprintf(chp, "%s - ", text_Group);
              printGroup(chp, i);
              chprintf(chp, "%s", html_e_option);
            }
            logAddress++;
          }
          for (uint8_t i = 0; i < ALARM_ZONES; i++) {
            if (GET_CONF_ZONE_ENABLED(conf.zone[i])) {
              chprintf(chp, "%s%u", html_option, logAddress);
              if ((conf.trigger[webTrigger].type == 'Z') && (conf.trigger[webTrigger].number == i)) {
                chprintf(chp, "%s", html_selected);
                number = 1;
              } else { chprintf(chp, "%s", html_e_tag); }
              chprintf(chp, "%s - ", text_Zone);
              printZone(chp, i);
              chprintf(chp, "%s", html_e_option);
            }
            logAddress++;
          }
          for (uint8_t i = 0; i < NODE_SIZE; i++) {
            if (node[i].type == 'S') {
              chprintf(chp, "%s%u", html_option, logAddress);
              if ((conf.trigger[webTrigger].type == node[i].type) &&
                  (conf.trigger[webTrigger].number == node[i].number) &&
                  (conf.trigger[webTrigger].function == node[i].function) &&
                  (conf.trigger[webTrigger].address == node[i].address)) {
                chprintf(chp, "%s", html_selected);
                number = 1;
              } else { chprintf(chp, "%s", html_e_tag); }
              chprintf(chp, "%s - ", text_Sensor);
              printNodeAddress(chp, node[i].address, node[i].type, node[i].function, node[i].number, true);
              chprintf(chp, "%s", html_e_option);
            }
            logAddress++;
          }
          chprintf(chp, "%s%u", html_option, DUMMY_NO_VALUE);
          if (!number) chprintf(chp, "%s", html_selected);
          else         chprintf(chp, "%s", html_e_tag);
          if ((conf.trigger[webTrigger].type == 'S') && (!number)) chprintf(chp, "%s", text_not_found);
          else                                      chprintf(chp, "%s", NOT_SET);
          chprintf(chp, "%s%s", html_e_option, html_e_select);
          // Condition
          chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, text_Condition, html_e_td_td);
          chprintf(chp, "%sc%sc%s", html_select, html_id_tag, html_e_tag);
          for (uint8_t i = 0; i < ARRAY_SIZE(triggerCondition); i++) {
             chprintf(chp, "%s%u", html_option, i);
             if (conf.trigger[webTrigger].condition == i) { chprintf(chp, "%s", html_selected); }
             else                                         { chprintf(chp, "%s", html_e_tag); }
             chprintf(chp, "%s%s", triggerCondition[i], html_e_option);
           }
          chprintf(chp, "%s", html_e_select);
          // Value
          chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, text_Value, html_e_td_td);
          chprintf(chp, "%s1%s", html_div_id_1, html_div_id_2);
          chprintf(chp, "%sg%sg%s", html_select, html_id_tag, html_e_tag);
          for (uint8_t i = 0; i < ARRAY_SIZE(groupState); i++) {
            chprintf(chp, "%s%u", html_option, i);
            if ((uint8_t)conf.trigger[webTrigger].value == i) { chprintf(chp, "%s", html_selected); }
            else                                              { chprintf(chp, "%s", html_e_tag); }
            chprintf(chp, "%s%s", groupState[i], html_e_option);
          }
          chprintf(chp, "%s", html_e_select);
          chprintf(chp, "%s%s2%s", html_div_e, html_div_id_1, html_div_id_2);
          chprintf(chp, "%sz%sz%s", html_select, html_id_tag, html_e_tag);
          for (uint8_t i = 0; i < ARRAY_SIZE(zoneState); i++) {
            chprintf(chp, "%s%u", html_option, i);
            if ((uint8_t)conf.trigger[webTrigger].value == i) { chprintf(chp, "%s", html_selected); }
            else                                              { chprintf(chp, "%s", html_e_tag); }
            chprintf(chp, "%s%s", zoneState[i], html_e_option);
          }
          chprintf(chp, "%s", html_e_select);
          chprintf(chp, "%s%s3%s", html_div_e, html_div_id_1, html_div_id_2);
          printFloatInput(chp, 'v', conf.trigger[webTrigger].value);
          chprintf(chp, "%s", html_div_e);
          chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, text_Hysteresis, html_e_td_td);
          printFloatInput(chp, 'h', conf.trigger[webTrigger].hysteresis);

          chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_Evaluate, text_script, html_e_td_td);
          chprintf(chp, "%sp%s", html_select, html_e_tag);
          logAddress = 1;
          for (scriptp = scriptLL; scriptp != NULL; scriptp = scriptp->next) {
            chprintf(chp, "%s%s", html_option, scriptp->name);
            if (strcmp(&conf.trigger[webTrigger].evalScript[0], scriptp->name) == 0) {
              chprintf(chp, "%s", html_selected);
              logAddress = 0;
            } else {
              chprintf(chp, "%s", html_e_tag);
            }
            chprintf(chp, "%s%s", scriptp->name, html_e_option);
          }
          chprintf(chp, "%s", html_option);
          if (logAddress) { chprintf(chp, "%s", html_selected); }
          else            { chprintf(chp, "%s", html_e_tag); }
          chprintf(chp, "%s%s%s", NOT_SET, html_e_option, html_e_select);

          chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, text_Alert, html_e_td_td);
          printOnOffButton(chp, "H", GET_CONF_TRIGGER_ALERT(conf.trigger[webTrigger].setting));
          chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, text_Pass, html_e_td_td);
          printTwoButton(chp, "C", GET_CONF_TRIGGER_PASS_VALUE(conf.trigger[webTrigger].setting),
                         0, 0b00, text_constant, text_value);
          chprintf(chp, "%s", html_br);
          printThreeButton(chp, "s", GET_CONF_TRIGGER_PASS(conf.trigger[webTrigger].setting),
                           1, 0b110, triggerPassType[0], triggerPassType[1], triggerPassType[2], 0);
          chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_Pass, text_Off,html_e_td_td);
          printThreeButton(chp, "S", GET_CONF_TRIGGER_PASS_OFF(conf.trigger[webTrigger].setting),
                           2, 0b100, triggerPassOffType[0], triggerPassOffType[1], triggerPassOffType[2], 0);
          chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_Off, text_timer, html_e_td_td);
          printIntInput(chp, 't', conf.trigger[webTrigger].offTime, 3, 1, 255);
          printDurationSelect(chp, 'T', GET_CONF_TRIGGER_OFF_PERIOD(conf.trigger[webTrigger].setting));
          chprintf(chp, "%s%s%s", html_tr_td, text_Address, html_e_td_td);
          chprintf(chp, "%sa%sa%s", html_select, html_id_tag, html_e_tag);
          for (uint8_t i = 0; i <= NODE_SIZE; i++) {
            if (i < NODE_SIZE) {
              if (node[i].type == 'I') {
                chprintf(chp, "%s%u", html_option, i);
                if ((node[i].address  == conf.trigger[webTrigger].toAddress) &&
                    (node[i].function == conf.trigger[webTrigger].toFunction) &&
                    (node[i].number   == conf.trigger[webTrigger].toNumber))
                     { chprintf(chp, "%s", html_selected); }
                else { chprintf(chp, "%s", html_e_tag); }
                printNodeAddress(chp, node[i].address, node[i].type, node[i].function, node[i].number, true);
                chprintf(chp, "%s", html_e_option);
              }
            } else {
              chprintf(chp, "%s%u", html_option, DUMMY_NO_VALUE);
              if ((conf.trigger[webTrigger].toAddress  == 0) &&
                  (conf.trigger[webTrigger].toFunction == ' ') &&
                  (conf.trigger[webTrigger].toNumber   == 0))
                   { chprintf(chp, "%s", html_selected); }
              else { chprintf(chp, "%s", html_e_tag); }
              chprintf(chp, "%s%s", NOT_SET, html_e_option);
            }
          }
          chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_On, text_value, html_e_td_td);
          printFloatInput(chp, 'o', conf.trigger[webTrigger].constantOn);
          chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_Off, text_value, html_e_td_td);
          printFloatInput(chp, 'f', conf.trigger[webTrigger].constantOff);
          chprintf(chp, "%s%s", html_e_td_e_tr, html_e_table);
          // JavaScript
          chprintf(chp, "%s%s%s", html_script, JSTrigger, html_e_script);
          chprintf(chp, "%s%s%u", html_script, JSTriggerSel_1, ALARM_GROUPS);
          chprintf(chp, "%s%u%s%u", JSTriggerSel_2, ALARM_GROUPS, JSTriggerSel_3, ALARM_GROUPS + ALARM_ZONES);
          chprintf(chp, "%s%s", JSTriggerSel_4, html_e_script);
          // Buttons
          chprintf(chp, "%s%s", html_Apply, html_Save);
          break;
        case PAGE_LOGIN:
          // Information table
          chprintf(chp, "%s%s%s", html_tr_td, text_User, html_e_td_td);
          printPassInput(chp, 'u', "", NAME_LENGTH, MIN_PASS_LNEGTH);
          chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, text_Password, html_e_td_td);
          printPassInput(chp, 'p', "", NAME_LENGTH, MIN_PASS_LNEGTH);
          chprintf(chp, "%s%s", html_e_td_e_tr, html_e_table);
          // Buttons
          chprintf(chp, "%s", html_Submit);
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
    memset(postData, 0, HTTP_POST_DATA_SIZE); // Empty POST data buffer
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
  char *pTail = (*pPostData); // Used as tail that overwrite the *pPostData

  *pValueLen = 0;             // Set value length = 0
  memset(pName, 0, nameLen);  // Clear out name
  (*pValue) = NULL;           // Clear out value as NULL terminated

  while (**pPostData != 0){
    ch = **pPostData; (*pPostData)++;
    //DBG_HTTP("ch:%c, ", ch);
    switch (ch) {
      case '+': ch = ' ';
        *pTail = ch;
        //DBG_HTTP("pTail:%s\r\n", pTail);
        pTail++;
        break;
      case '%':  // handle URL encoded characters by converting back to original form
        ch1 = **pPostData; (*pPostData)++;
        ch2 = **pPostData; (*pPostData)++;
        if (ch1 == 0 || ch2 == 0) {
          *pTail = 0;
          pTail++;
          continue;
        }
        char hex[3] = { ch1, ch2, 0 };
        ch = strtoul(hex, NULL, 16);
        *pTail = ch;
        //DBG_HTTP("pTail:%s\r\n", pTail);
        pTail++;
        break;
      case '=': // that's end of name, switch to storing value
        //DBG_HTTP("pTail:%s\r\n", pTail);
        pTail++;
        nameLen = 0;
        (*pValue) = (*pPostData);
        continue; // do not store '='
        break;
      case '&': // that's end of pair, go away
        // null rest of *pPostData up to pTail
        while (pTail != (*pPostData)) {
          *pTail = 0; pTail++;
        }
        //DBG_HTTP("--*pValue:%s\r\n", *pValue);
        return true;
        break;
      default:
        *pTail = ch;
        //DBG_HTTP("pTail:%s\r\n", pTail);
        pTail++;
        break;
    }
    // check against 1 so we don't overwrite the final NULL
    if (nameLen > 1) {
      *pName++ = ch;
      --nameLen;
    } else {
      if (nameLen == 0) {
        (*pValueLen)++;
      }
    }
    //DBG_HTTP("nameLen:%u, valueLen:%u, value:%s\r\n", nameLen, *pValueLen, *pValue);
  }
  // null rest of *pPostData up to pTail
  while (pTail != (*pPostData)) {
    *pTail = 0; pTail++;
  }
  return false; // Null is end
}
/*
 * Receiving POST
 */
err_t httpd_post_receive_data(void *connection, struct pbuf *p) {

  DBG_HTTP("-PD-connection: %u\r\n", (uint32_t *)connection);
  if (currentConn == connection) {
    DBG_HTTP("p->payload: '%.*s'\r\n", p->len, p->payload);
    //DBG_HTTP("p->ref: %u\r\n", p->ref);
    //DBG_HTTP("p->next: %u\r\n", p->next);
    //DBG_HTTP("p->tot_len: %u\r\n", p->tot_len);

    //DBG_HTTP("strlen(postData): %u\r\n", strlen(postData));
    // Append payload to buffer, maximum size of postData and null
    strncat(postData, (const char *)p->payload, LWIP_MIN(p->tot_len, (sizeof(postData)-strlen(postData))));

    //pPostData = &postData[strlen(postData)];
    //pPostData = (char *)pbuf_get_contiguous(p, postData, sizeof(postData)-strlen(postData),
                //LWIP_MIN(p->tot_len,sizeof(postData)-strlen(postData)), 0);
    //DBG_HTTP("postData: %s\r\n", postData);
    //DBG_HTTP("pPostData: %s\r\n", pPostData);
    //--u8_t buf[32];
    //--u8_t ptr = (u8_t)pbuf_get_contiguous(p, buf, sizeof(buf), LWIP_MIN(option_len, sizeof(buf)), offset);

    //DBG_HTTP("strlen(postData): %u\r\n", strlen(postData));
    if (p != NULL) pbuf_free(p);
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
                  resp = sendCmd(15, NODE_CMD_REGISTRATION); // Broadcast to register
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
                  if (resp) pushToMqtt(typeSensor, webNode, functionName);
                break;
                case '0' ... '7': // Handle all single radio buttons for settings
                  if (valueP[0] == '0') node[webNode].setting &= ~(1 << (name[0]-48));
                  else                 node[webNode].setting |=  (1 << (name[0]-48));
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
                  conf.key[webKey].value = strtoul(valueP, NULL, 16); // as unsigned long int
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

#endif /* OHS_HTTPDHANDLER_H_ */
