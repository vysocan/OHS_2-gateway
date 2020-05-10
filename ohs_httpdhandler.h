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

#if !LWIP_HTTPD_CUSTOM_FILES
#error This needs LWIP_HTTPD_CUSTOM_FILES
#endif
#if !LWIP_HTTPD_DYNAMIC_HEADERS
#error This needs LWIP_HTTPD_DYNAMIC_HEADERS
#endif
#if !LWIP_HTTPD_SUPPORT_POST
#error This needs LWIP_HTTPD_SUPPORT_POST
#endif

#ifndef HTTP_DEBUG
#define HTTP_DEBUG 0
#endif

#if HTTP_DEBUG
#define DBG_HTTP(...) {chprintf((BaseSequentialStream*)&SD3, __VA_ARGS__);}
#else
#define DBG_HTTP(...)
#endif

static void *current_connection;
static void *valid_connection;
static char current_uri[LWIP_HTTPD_MAX_REQUEST_URI_LEN];

const char text_i_OK[]              = "<i class='fa fa-check'></i>";
const char text_i_ALARM[]           = "<i class='fa fa-bell'></i>";
const char text_i_disabled[]        = "<i class='fa fa-ban'></i>";
const char text_i_starting[]        = "<i class='fa fa-spinner fa-pulse'></i>";
const char text_i_home[]            = "<i class='fa fa-home'></i>";
const char text_i_question[]        = "<i class='fa fa-question'></i>";
const char text_i_zone[]            = "<i class='fa fa-square-o'></i>";
const char text_i_qlobe[]           = "<i class='fa fa-globe'></i>";
const char text_i_auth[]            = "<i class='fa fa-lock'></i>";
const char text_i_contact[]         = "<i class='fa fa-address-card-o'></i>";
const char text_i_key[]             = "<i class='fa fa-key'></i>";
const char text_i_sens[]            = "<i class='fa fa-share-alt'></i>";
const char html_tr_td[]             = "<tr><td>";
const char html_e_td_td[]           = "</td><td>";
const char html_e_td_e_tr[]         = "</td></tr>";
const char html_e_td_e_tr_tr_td[]   = "</td></tr><tr><td>";
const char html_tr_th[]             = "<tr><th>";
const char html_e_th_th[]           = "</th><th>";
const char html_e_th_e_tr[]         = "</th></tr>";
const char html_select_submit[]     = "<select onchange='this.form.submit()' name='";
const char html_e_tag[]             = "'>";
const char html_e_select[]          = "</select>";
const char html_option[]            = "<option value='";
const char html_e_option[]          = "</option>";
const char html_selected[]          = "' selected>";
const char html_m_tag[]             = "' value='";
const char html_id_tag[]            = "' id='";
const char html_t_tag_1[]           = "<input type='text' maxlength='";
const char html_i_tag_1[]           = "<input type='time";
const char html_i_tag_2[]           = "' min='00:00' max='23:59' required>";
const char html_n_tag_1[]           = "<input type='number' style='width:";
const char html_n_tag_2[]           = "em' min='";
const char html_n_tag_3[]           = "' max='";
const char html_p_tag_1[]           = "<input type='password' maxlength='";
const char html_s_tag_2[]           = "' size='";
const char html_s_tag_3[]           = "' name='";
const char html_radio_s[]           = "<div class='rm'>";
const char html_radio_sl[]          = "<div class='rml'>";
const char html_radio_sb[]          = "<div class='rmb'>";
const char html_div_e[]             = "</div>";
const char html_select[]            = "<select name='";
const char html_Apply[]             = "<input type='submit' name='A' value='Apply'/>";
const char html_ApplyValPass[]      = "<input type='submit' name='A' value='Apply' onclick='return pv()'/>";
const char html_Save[]              = "<input type='submit' name='e' value='Save'/>";
const char html_Reregister[]        = "<input type='submit' name='R' value='Call registration'/>";
const char html_Now[]               = "<input type='submit' name='N' value='Now'/>";
const char html_FR[]                = "<input type='submit' name='R' value='<<'/>";
const char html_R[]                 = "<input type='submit' name='r' value='<'/>";
const char html_FF[]                = "<input type='submit' name='F' value='>>'/>";
const char html_F[]                 = "<input type='submit' name='f' value='>'/>";
const char html_Run[]               = "<input type='submit' name='R' value='Run'/>";
const char html_Refresh[]           = "<input type='submit' name='F' value='Refresh'/>";
const char html_Restart[]           = "<input type='submit' name='S' value='Restart'/>";
const char html_textarea_1[]        = "<textarea name='";
const char html_textarea_2[]        = "' id='";
const char html_textarea_3[]        = "' rows='";
const char html_textarea_4[]        = "' cols='";
const char html_textarea_5[]        = "' maxlength='";
const char html_textarea_e[]        = "</textarea>";
const char html_e_table[]           = "</table>";
const char html_table[]             = "<table>";
const char html_form_1[]            = "<form action='";
const char html_form_2[]            = "' method='post'>";
const char html_br[]                = "<br>";
// Radio buttons
const char html_cbPart1a[]          = "<div class='rc'><input type='radio' name='";
const char html_cbPart1b[]          = "' id='";
const char html_cbPart2[]           = "' value='";
const char html_cbPart3[]           = "'";
const char html_cbChecked[]         = "' checked ";
const char html_cbPart4a[]          = "><label for='";
const char html_cbPart4b[]          = "'>";
const char html_cbPart5[]           = "</label></div>";
const char html_cbJSen[]            = " onclick=\"en";
const char html_cbJSdis[]           = " onclick=\"dis";
const char html_cbJSend[]           = "()\"";
// JavaScript related
const char html_script[]            = "<script>";
const char html_e_script[]          = "</script>";
const char html_script_src[]        = "<script type='text/javascript' src='/js/";
const char JSen1[]                  = "en1()";
const char JSdis1[]                 = "dis1()";
const char JSContact[]              = "var e1=document.querySelectorAll(\"#g\");"
                                      "var d1=document.querySelectorAll(\"#xx\");";
const char JSCredential[]           = "var tc=document.querySelectorAll(\"#p,#d\");";
const char JSZone[]                 = "var e1=document.querySelectorAll(\"#xx\");"
                                      "var d1=document.querySelectorAll(\"#a1,#a0\");";
const char JSTimer[]                = "var e1=document.querySelectorAll(\"#s,#S\");"
                                      "var d1=document.querySelectorAll(\"#D0,#D1,#E0,#E1,#F0,#F1,#G0,#G1,#H0,#H1,#I0,#I1,#J0,#J1\");";

// old
const char JSTrigger[]              = "var x=document.querySelectorAll(\"#XX\");"
                                      "var y=document.querySelectorAll(\"#F0,#F1,#C0,#C1,#m0,#m1,#m2,#r,#l0,#l1,#l2,#l3,#t,#c,#f\");";


#define HTML_PAGE_SIZE  (MEM_SIZE - 1600) // Change in lwip opt.h MEM_SIZE

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
  {"/tcl.html",     "Scripts"}
};

static char postData[1024] __attribute__((section(".ram4")));

void printOkNok(BaseSequentialStream *chp, const int8_t value) {
  if (value == 1) chprintf(chp, "%s", text_i_OK);
  else            chprintf(chp, "%s", text_i_disabled);
}

void printRadioButton(BaseSequentialStream *chp, const char *name, const uint8_t value,
                 const char *label, bool selected, const uint8_t enableJS) {
  chprintf(chp, "%s%s%s", html_cbPart1a, name, html_cbPart1b);
  chprintf(chp, "%s%u%s%u", name, value, html_cbPart2, value);
  selected ? chprintf(chp, "%s", html_cbChecked) : chprintf(chp, "%s", html_cbPart3);
  if (enableJS) {
    if (value) {
      chprintf(chp, "%s%u%s", html_cbJSen, enableJS, html_cbJSend);
    } else {
      chprintf(chp, "%s%u%s", html_cbJSdis, enableJS, html_cbJSend);
    }
  }
  chprintf(chp, "%s%s%u", html_cbPart4a, name, value);
  chprintf(chp, "%s%s%s", html_cbPart4b, label, html_cbPart5);
}

#define GET_BUTTON_STATE(x,y) (x==y)
void printFourButton(BaseSequentialStream *chp, const char *name, const uint8_t state, const uint8_t enableJS,
                     const char *text1, const char *text2, const char *text3, const char *text4, const uint8_t size) {
  if (size) chprintf(chp, "%s", html_radio_sb);
  else      chprintf(chp, "%s", html_radio_sl);
  printRadioButton(chp, name, 0, text1, GET_BUTTON_STATE(state, 0), enableJS);
  printRadioButton(chp, name, 1, text2, GET_BUTTON_STATE(state, 1), enableJS);
  printRadioButton(chp, name, 2, text3, GET_BUTTON_STATE(state, 2), enableJS);
  printRadioButton(chp, name, 3, text4, GET_BUTTON_STATE(state, 3), enableJS);
  chprintf(chp, "%s", html_div_e);
}

void printTwoButton(BaseSequentialStream *chp, const char *name, const uint8_t state, const uint8_t enableJS,
                     const char *text1, const char *text2) {
  chprintf(chp, "%s", html_radio_sl);
  printRadioButton(chp, name, 0, text1, GET_BUTTON_STATE(state, 0), enableJS);
  printRadioButton(chp, name, 1, text2, GET_BUTTON_STATE(state, 1), enableJS);
  chprintf(chp, "%s", html_div_e);
}

void  printOnOffButton(BaseSequentialStream *chp, const char *name, const uint8_t state, const uint8_t enableJS) {
  chprintf(chp, "%s", html_radio_s);
  printRadioButton(chp, name, 1, text_On, state, enableJS);
  printRadioButton(chp, name, 0, text_Off, !state, enableJS);
  chprintf(chp, "%s", html_div_e);
}

void selectGroup(BaseSequentialStream *chp, uint8_t selected, char name) {
  chprintf(chp, "%s%c%s%c%s", html_select, name, html_id_tag, name, html_e_tag);
  for (uint8_t i = 0; i < ALARM_GROUPS; i++) {
    chprintf(chp, "%s%u", html_option, i);
    if (selected == i) { chprintf(chp, "%s", html_selected); }
    else               { chprintf(chp, "%s", html_e_tag); }
    chprintf(chp, "%u. %s - ", i + 1, conf.groupName[i]);
    GET_CONF_GROUP_ENABLED(conf.group[i]) ? chprintf(chp, "%s", text_On) : chprintf(chp, "%s", text_Off);
    chprintf(chp, "%s", html_e_option);
  }
  chprintf(chp, "%s15", html_option);
  if (selected == 15) { chprintf(chp, "%s", html_selected); }
  else                { chprintf(chp, "%s", html_e_tag); }
  chprintf(chp, "%s%s", NOT_SET, html_e_option);
  chprintf(chp, "%s", html_e_select);
}

void printNodeValue(BaseSequentialStream *chp, const uint8_t index) {
  if (node[index].type != 'K') {
    switch(node[index].function){
      case 'T': chprintf(chp, "%.2f °C", node[index].value); break;
      case 'H':
      case 'X': chprintf(chp, "%.2f %%", node[index].value); break;
      case 'P': chprintf(chp, "%.2f mBar", node[index].value); break;
      case 'V':
      case 'B': chprintf(chp, "%.2f V", node[index].value); break;
      case 'G': chprintf(chp, "%.2f ppm", node[index].value); break;
      default: chprintf(chp, "%.2f", node[index].value); break;
    }
  }
}

void printTextInput(BaseSequentialStream *chp, const char name, const char *value, const uint8_t size){
  chprintf(chp, "%s%u%s%u%s", html_t_tag_1, size, html_s_tag_2, size, html_s_tag_3);
  chprintf(chp, "%c%s%s", name, html_m_tag, value);
  chprintf(chp, "%s%c%s", html_id_tag, name, html_e_tag);
}

void printPassInput(BaseSequentialStream *chp, const char name, const char *value, const uint8_t size){
  chprintf(chp, "%s%u%s%u%s", html_p_tag_1, size, html_s_tag_2, size, html_s_tag_3);
  chprintf(chp, "%c%s%s", name, html_m_tag, value);
  chprintf(chp, "%s%c%s", html_id_tag, name, html_e_tag);
}

void printIntInput(BaseSequentialStream *chp, const char name, const int16_t value,
                   const uint8_t size, const uint16_t min, const uint16_t max){
  chprintf(chp, "%s%u", html_n_tag_1, size + 2);
  chprintf(chp, "%s%u%s%u%s", html_n_tag_2, min, html_n_tag_3, max, html_s_tag_3);
  chprintf(chp, "%c%s%d", name, html_m_tag, value);
  chprintf(chp, "%s%c%s", html_id_tag, name, html_e_tag);
}

void printFloatInput(BaseSequentialStream *chp, const char name, const float value){
  chprintf(chp, "%s6em%s", html_n_tag_1, html_s_tag_3);
  chprintf(chp, "%c%s%.02f", name, html_m_tag, value);
  chprintf(chp, "%s%c' step='0.01'>", html_id_tag, name);
}

void printTimeInput(BaseSequentialStream *chp, const char name, const uint8_t hour,
                    const uint8_t minute){
  chprintf(chp, "%s%s%c", html_i_tag_1, html_s_tag_3, name);
  chprintf(chp, "%s%c%s", html_id_tag, name, html_m_tag);
  chprintf(chp, "%02u:%02u%s", hour, minute, html_i_tag_2);
}

void printTextArea(BaseSequentialStream *chp, const char name, const char *value,
                   const uint16_t maxSize, const uint8_t cols, const uint8_t rows){
  chprintf(chp, "%s%c%s%c%s%u", html_textarea_1, name, html_textarea_2, name, html_textarea_3, rows);
  chprintf(chp, "%s%u%s%u%s", html_textarea_4, cols, html_textarea_5, maxSize, html_e_tag);
  chprintf(chp, "%s%s", value, html_textarea_e);
}

void printDurationSelect(BaseSequentialStream *chp, const char name, const uint8_t value){
  chprintf(chp, "%s%c%s%c%s", html_select, name, html_id_tag, name, html_e_tag);
   for (uint8_t i = 0; i < ARRAY_SIZE(durationSelect); i++) {
     chprintf(chp, "%s%u", html_option, i);
     if (value == i) { chprintf(chp, "%s", html_selected); }
     else            { chprintf(chp, "%s", html_e_tag); }
     chprintf(chp, "%s%s", durationSelect[i], html_e_option);
   }
   chprintf(chp, "%s", html_e_select);
}

void genfiles_ex_init(void) {
  /* nothing to do here yet */
}

static uint8_t webNode = 0, webContact = 0, webKey = 0, webZone = 0, webGroup = 0,
    webTimer = 0, webScript = DUMMY_NO_VALUE;
char scriptName[NAME_LENGTH];
static uint16_t webLog = 0;

int fs_open_custom(struct fs_file *file, const char *name){
  char temp[3] = "";
  uint16_t logAddress;
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
          GET_CONF_CONTACT_IS_GLOBAL(conf.contact[webContact]) ? chprintf(chp, JSen1) : chprintf(chp, JSdis1);
          break;
        case PAGE_ZONE:
          GET_CONF_ZONE_TYPE(conf.zone[webZone]) ? chprintf(chp, JSen1) : chprintf(chp, JSdis1);
          break;
        case PAGE_TIMER:
          GET_CONF_TIMER_TYPE(conf.timer[webTimer].setting) ? chprintf(chp, JSen1) : chprintf(chp, JSdis1);
          break;
      }
      chprintf(chp, "\"><div class='wrp'><div class='sb'>\r\n");
      chprintf(chp, "<div class='tt'>OHS 2.0 - %u.%u</div>\r\n", OHS_MAJOR, OHS_MINOR);
      chprintf(chp, "<ul class='nav'>\r\n");
      for (uint8_t i = 0; i < ARRAY_SIZE(webPage); ++i) {
        if (htmlPage == i) chprintf(chp, "<li><a class='active' href='%s'>%s</a></li>\r\n", webPage[i].link, webPage[i].name);
        else chprintf(chp, "<li><a href='%s'>%s</a></li>\r\n", webPage[i].link, webPage[i].name);
      }
      chprintf(chp, "</ul></div><div class='mb'><h1>%s</h1>\r\n", webPage[htmlPage].name);
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
              chprintf(chp, "%s%u.%s%s - ", html_tr_td, i + 1, html_e_td_td, node[i].name);
              printNodeAddress(chp, node[i].address, node[i].type, node[i].function, node[i].number);
              chprintf(chp, "%s", html_e_td_td);
              printOkNok(chp, GET_NODE_ENABLED(node[i].setting));
              chprintf(chp, "%s", html_e_td_td);
              printOkNok(chp, GET_NODE_MQTT_PUB(node[i].setting));
              chprintf(chp, "%s", html_e_td_td);
              printFrmTimestamp(chp, &node[i].last_OK);
              chprintf(chp, "%s", html_e_td_td);
              // queued
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
          printNodeAddress(chp, node[webNode].address, node[webNode].type, node[webNode].function, node[webNode].number);
          chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, text_Type, html_e_td_td);
          printNodeType(chp, node[webNode].type);
          chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, text_Function, html_e_td_td);
          printNodeFunction(chp, node[webNode].function);
          chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_Node, text_is, html_e_td_td);
          printOnOffButton(chp, "0", GET_NODE_ENABLED(node[webNode].setting), 0);
          chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_MQTT, text_publish, html_e_td_td);
          printOnOffButton(chp, "7", GET_NODE_MQTT_PUB(node[webNode].setting), 0);
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
            chprintf(chp, "%s%s", conf.contactName[i], html_e_td_td);
            printOkNok(chp, GET_CONF_CONTACT_ENABLED(conf.contact[i]));
            chprintf(chp, "%s%s", html_e_td_td, conf.contactPhone[i]);
            chprintf(chp, "%s%s", html_e_td_td, conf.contactEmail[i]);
            chprintf(chp, "%s", html_e_td_td);
            printOkNok(chp, GET_CONF_CONTACT_IS_GLOBAL(conf.contact[i]));
            chprintf(chp, "%s", html_e_td_td);
            logAddress = 0; // Just temp. var.
            for (uint8_t j = 0; j < KEYS_SIZE; j++) {
              if (conf.keyContact[j] == i) {
                logAddress++;
              }
            }
            chprintf(chp, "%u%s", logAddress, html_e_td_td);
            if (!GET_CONF_CONTACT_IS_GLOBAL(conf.contact[i])) printGroup(chp, GET_CONF_CONTACT_GROUP(conf.contact[i]));
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
            chprintf(chp, "%u. %s%s", i + 1, conf.contactName[i], html_e_option);
          }
          chprintf(chp, "%s%s", html_e_select, html_e_td_e_tr_tr_td);
          chprintf(chp, "%s%s", text_Name, html_e_td_td);
          printTextInput(chp, 'n', conf.contactName[webContact], NAME_LENGTH);
          chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_User, text_is, html_e_td_td);
          printOnOffButton(chp, "0", GET_CONF_CONTACT_ENABLED(conf.contact[webContact]), 0);
          chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, text_Number, html_e_td_td);
          printTextInput(chp, 'p', conf.contactPhone[webContact], PHONE_LENGTH);
          chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, text_Email, html_e_td_td);
          printTextInput(chp, 'm', conf.contactEmail[webContact], EMAIL_LENGTH);
          chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, text_Global, html_e_td_td);
          printOnOffButton(chp, "5", GET_CONF_CONTACT_IS_GLOBAL(conf.contact[webContact]), 1);
          chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, text_Group, html_e_td_td);
          selectGroup(chp, GET_CONF_CONTACT_GROUP(conf.contact[webContact]), 'g');
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
            if (conf.keyContact[i] == DUMMY_NO_VALUE) chprintf(chp, "%s", NOT_SET);
            else chprintf(chp, "%s", conf.contactName[conf.keyContact[i]]);
            chprintf(chp, "%s", html_e_td_td);
            printOkNok(chp, GET_CONF_KEY_ENABLED(conf.keySetting[i]));
            chprintf(chp, "%s", html_e_td_td);
            uint32Conv.val = conf.keyValue[i];
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
              if (conf.keyContact[webKey] == i) { chprintf(chp, "%s", html_selected); }
              else                              { chprintf(chp, "%s", html_e_tag); }
              chprintf(chp, "%u. %s%s", i + 1, conf.contactName[i], html_e_option);
            } else {
              chprintf(chp, "%s%u", html_option, DUMMY_NO_VALUE);
              if (conf.keyContact[webKey] == DUMMY_NO_VALUE) { chprintf(chp, "%s", html_selected); }
              else                                           { chprintf(chp, "%s", html_e_tag); }
              chprintf(chp, "%s%s", NOT_SET, html_e_option);
            }
          }
          chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_Key, text_is, html_e_td_td);
          printOnOffButton(chp, "0", GET_CONF_KEY_ENABLED(conf.keySetting[webKey]), 0);
          chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, text_Hash, html_e_td_td);
          chprintf(chp, "%s%u%s%u", html_t_tag_1, KEY_LENGTH * 2, html_s_tag_2, KEY_LENGTH * 2);
          chprintf(chp, "%sk%s", html_s_tag_3, html_m_tag);
          uint32Conv.val = conf.keyValue[webKey];
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
          chprintf(chp, "%s%s", html_e_th_th, text_Status);
          chprintf(chp, "%s%s%s\r\n", html_e_th_th, text_Group, html_e_th_e_tr);
          // Information table
          for (uint8_t i = 0; i < ALARM_ZONES; i++) {
            if (GET_CONF_ZONE_IS_PRESENT(conf.zone[i])) {
              chprintf(chp, "%s%u.%s", html_tr_td, i + 1, html_e_td_td);
              chprintf(chp, "%s%s", conf.zoneName[i], html_e_td_td);
              printOkNok(chp, GET_CONF_ZONE_ENABLED(conf.zone[i]));
              chprintf(chp, "%s", html_e_td_td);
              GET_CONF_ZONE_BALANCED(conf.zone[i]) ? chprintf(chp, "%s ", text_balanced) : chprintf(chp, "un%s ", text_balanced);
              GET_CONF_ZONE_IS_REMOTE(conf.zone[i]) ? chprintf(chp, "%s ", text_remote) : chprintf(chp, "%s ", text_local);
              if (GET_CONF_ZONE_IS_BATTERY(conf.zone[i])) chprintf(chp, "%s ", text_battery);
              GET_CONF_ZONE_TYPE(conf.zone[i]) ? chprintf(chp, "%s", text_analog) : chprintf(chp, "%s", text_digital);
              chprintf(chp, "%s", html_e_td_td);
              printOkNok(chp, GET_CONF_ZONE_ARM_HOME(conf.zone[i]));
              chprintf(chp, "%s", html_e_td_td);
              printOkNok(chp, GET_CONF_ZONE_OPEN_ALARM(conf.zone[i]));
              chprintf(chp, "%s", html_e_td_td);
              printOkNok(chp, GET_CONF_ZONE_PIR_AS_TMP(conf.zone[i]));
              chprintf(chp, "%s", html_e_td_td);
              chprintf(chp, "%u %s%s", GET_CONF_ZONE_AUTH_TIME(conf.zone[i])*conf.armDelay, durationSelect[0], html_e_td_td);
              if (GET_CONF_ZONE_ENABLED(conf.zone[i])) printFrmTimestamp(chp, &zone[i].lastPIR);
              chprintf(chp, "%s", html_e_td_td);
              if (GET_CONF_ZONE_ENABLED(conf.zone[i])) printFrmTimestamp(chp, &zone[i].lastOK);
              chprintf(chp, "%s", html_e_td_td);
              if (GET_CONF_ZONE_ENABLED(conf.zone[i])) {
                switch(zone[i].lastEvent){
                  case 'O': chprintf(chp, "%s", text_i_OK); break;
                  case 'P': chprintf(chp, "%s", text_i_ALARM); break;
                  case 'N': chprintf(chp, "%s", text_i_starting); break;
                  default: chprintf(chp, "%s", text_tamper); break;
                }
              } else { chprintf(chp, "%s", text_i_disabled); }
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
          printOnOffButton(chp, "0", GET_CONF_ZONE_ENABLED(conf.zone[webZone]), 0);
          chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_Arm, text_home, html_e_td_td);
          printOnOffButton(chp, "7", GET_CONF_ZONE_ARM_HOME(conf.zone[webZone]), 0);
          chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_Open, text_alarm, html_e_td_td);
          printOnOffButton(chp, "8", GET_CONF_ZONE_OPEN_ALARM(conf.zone[webZone]), 0);
          chprintf(chp, "%s%s %s %s%s", html_e_td_e_tr_tr_td,  text_Alarm, text_as, text_tamper, html_e_td_td);
          printOnOffButton(chp, "9", GET_CONF_ZONE_PIR_AS_TMP(conf.zone[webZone]), 0);
          chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, text_Balanced, html_e_td_td);
          printOnOffButton(chp, "a", GET_CONF_ZONE_BALANCED(conf.zone[webZone]), 0);
          chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_Authentication, text_delay, html_e_td_td);
          printFourButton(chp, "d", GET_CONF_ZONE_AUTH_TIME(conf.zone[webZone]), false,
                          text_0x, text_1x, text_2x, text_3x, 0);
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
              printOnOffButton(chp, temp, (conf.alert[j] >> i) & 0b1, 0);
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
            txBuffer[1] = (logAddress >> 8) & 0xFF;
            txBuffer[2] = logAddress & 0xFF;

            spiSelect(&SPID1);                  // Slave Select assertion.
            spiSend(&SPID1, 3, txBuffer);       // Send read command
            spiReceive(&SPID1, FRAM_MSG_SIZE, rxBuffer);
            spiUnselect(&SPID1);                // Slave Select de-assertion.

            memcpy(&timeConv.ch[0], &rxBuffer[0], sizeof(timeConv.ch)); // Prepare timestamp
            decodeLog(&rxBuffer[4], logText, 1);

            printFrmTimestamp(chp, &timeConv.val);
            chprintf(chp, "%s%s.", html_e_td_td, logText);
            chprintf(chp, "%s", html_e_td_td);
            for (uint8_t j = 0; j < ARRAY_SIZE(alertType); j++) {
              if ((((uint8_t)rxBuffer[FRAM_MSG_SIZE-1] >> j) & 0b1) == 1) chprintf(chp, "%s ", alertType[j].name);
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
          chprintf(chp, "%s%s %s", html_e_th_th, text_Alarm, text_trigger);
          chprintf(chp, "%s%s", html_e_th_th, text_Armed);
          chprintf(chp, "%s%s%s\r\n", html_e_th_th, text_Status, html_e_th_e_tr);
          // Information table
          for (uint8_t i = 0; i < ALARM_GROUPS; i++) {
            chprintf(chp, "%s%u.%s", html_tr_td, i + 1, html_e_td_td);
            chprintf(chp, "%s%s", conf.groupName[i], html_e_td_td);
            printOkNok(chp, GET_CONF_GROUP_ENABLED(conf.group[i]));
            chprintf(chp, "%s", html_e_td_td);
            printOkNok(chp, GET_CONF_GROUP_AUTO_ARM(conf.group[i]));
            chprintf(chp, "%s", html_e_td_td);
            printGroup(chp, GET_CONF_GROUP_ARM_CHAIN(conf.group[i]));
            chprintf(chp, "%s", html_e_td_td);
            printGroup(chp, GET_CONF_GROUP_DISARM_CHAIN(conf.group[i]));
            chprintf(chp, "%s", html_e_td_td);
            logAddress = 0; // Just temp. var.
            for (uint8_t j = 0; j < ALARM_ZONES; j++) {
              if (GET_CONF_ZONE_GROUP(conf.zone[j]) == i) {
                if (logAddress > 0) chprintf(chp, "%s", html_br);
                chprintf(chp, "%u. %s", j+1, conf.zoneName[j]);
                logAddress++;
              }
            }
            logAddress = 0; // Just temp. var.
            chprintf(chp, "%s", html_e_td_td);
            for (uint8_t j = 0; j < NODE_SIZE; j++) {
              if ((GET_NODE_GROUP(node[j].setting) == i) && (node[j].type == 'K')) {
                if (logAddress > 0) chprintf(chp, "%s", html_br);
                chprintf(chp, "%u. %s", j+1, node[j].name);
                logAddress++;
              }
            }
            logAddress = 0; // Just temp. var.
            chprintf(chp, "%s", html_e_td_td);
            for (uint8_t j = 0; j < NODE_SIZE; j++) {
              if ((GET_NODE_GROUP(node[j].setting) == i) && (node[j].type == 'S')) {
                if (logAddress > 0) chprintf(chp, "%s", html_br);
                chprintf(chp, "%u. %s", j+1, node[j].name);
                logAddress++;
              }
            }
            logAddress = 0; // Just temp. var.
            chprintf(chp, "%s", html_e_td_td);
            for (uint8_t j = 0; j < CONTACTS_SIZE; j++) {
              if ((GET_CONF_CONTACT_GROUP(conf.contact[j]) == i) ||
                  (GET_CONF_CONTACT_IS_GLOBAL(conf.contact[j]))){
                if (logAddress > 0) chprintf(chp, "%s", html_br);
                chprintf(chp, "%s", conf.contactName[j]);
                logAddress++;
              }
            }
            chprintf(chp, "%s", html_e_td_td);
            chprintf(chp, "%s", html_e_td_td);
            if (GET_GROUP_ARMED(group[i].setting)) {
              if GET_GROUP_ARMED_HOME(group[i].setting) { chprintf(chp, "%s", text_i_home); }
              else                                      { chprintf(chp, "%s", text_i_OK); }
            } else {
              if (group[i].armDelay > 0) { chprintf(chp, "%s", text_i_starting); }
              else                       { chprintf(chp, "%s", text_i_disabled); }
            }
            chprintf(chp, "%s", html_e_td_td);
            if (GET_GROUP_ALARM(group[i].setting) == 0) {
              if (GET_GROUP_WAIT_AUTH(group[i].setting)) { chprintf(chp, "%s", text_i_starting); }
              else                                       { chprintf(chp, "%s", text_i_OK); }
            } else { chprintf(chp, "%s", text_i_ALARM); }
            chprintf(chp, "%s", html_e_td_e_tr);
          }
          chprintf(chp, "%s%s", html_e_table, html_table);
          chprintf(chp, "%s%s%s", html_tr_td, text_Group, html_e_td_td);
          chprintf(chp, "%sP%s", html_select_submit, html_e_tag);
          for (uint8_t i = 0; i < ALARM_GROUPS; i++) {
            chprintf(chp, "%s%u", html_option, i);
            if (webGroup == i) { chprintf(chp, "%s", html_selected); }
            else               { chprintf(chp, "%s", html_e_tag); }
            chprintf(chp, "%u. %s%s", i + 1, conf.groupName[i], html_e_option);
          }
          chprintf(chp, "%s%s", html_e_select, html_e_td_e_tr_tr_td);
          chprintf(chp, "%s%s", text_Name, html_e_td_td);
          printTextInput(chp, 'n', conf.groupName[webGroup], NAME_LENGTH);
          chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_Group, text_is, html_e_td_td);
          printOnOffButton(chp, "0", GET_CONF_GROUP_ENABLED(conf.group[webGroup]), 0);
          chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_Auto, text_arm, html_e_td_td);
          printOnOffButton(chp, "5", GET_CONF_GROUP_AUTO_ARM(conf.group[webGroup]), 0);
          chprintf(chp, "%s%s %ss %s 1%s", html_e_td_e_tr_tr_td,  text_Alarm, text_trigger, text_relay, html_e_td_td);
          printOnOffButton(chp, "4", GET_CONF_GROUP_PIR1(conf.group[webGroup]), 0);
          chprintf(chp, "%s%s %ss %s 1%s", html_e_td_e_tr_tr_td,  text_Alarm, text_trigger, text_relay, html_e_td_td);
          printOnOffButton(chp, "3", GET_CONF_GROUP_PIR2(conf.group[webGroup]), 0);
          chprintf(chp, "%s%s %ss %s 2%s", html_e_td_e_tr_tr_td,  text_Tamper, text_trigger, text_relay, html_e_td_td);
          printOnOffButton(chp, "2", GET_CONF_GROUP_TAMPER1(conf.group[webGroup]), 0);
          chprintf(chp, "%s%s %ss %s 2%s", html_e_td_e_tr_tr_td,  text_Tamper, text_trigger, text_relay, html_e_td_td);
          printOnOffButton(chp, "1", GET_CONF_GROUP_TAMPER2(conf.group[webGroup]), 0);
          chprintf(chp, "%s%s %s %s%s", html_e_td_e_tr_tr_td, text_Arm, text_group, text_chain, html_e_td_td);
          selectGroup(chp, GET_CONF_GROUP_ARM_CHAIN(conf.group[webGroup]), 'a');
          chprintf(chp, "%s%s %s %s%s", html_e_td_e_tr_tr_td, text_Disarm, text_group, text_chain, html_e_td_td);
          selectGroup(chp, GET_CONF_GROUP_DISARM_CHAIN(conf.group[webGroup]), 'd');
          chprintf(chp, "%s%s", html_e_td_e_tr, html_e_table);
          // Buttons
          chprintf(chp, "%s%s", html_Apply, html_Save);
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
          chprintf(chp, "%s%s %s%", html_e_td_e_tr_tr_td, text_RTC, text_battery);
          chprintf(chp, "%s%.2f V", html_e_td_td, rtcVbat);
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
            case 0 : chprintf(chp, "%s", text_i_OK); break;
            case 1 : chprintf(chp, "%s", text_i_home); break;
            case 2 : chprintf(chp, "%s", text_i_starting); break;
            case 3 : chprintf(chp, "%s %s", text_registration, text_denied); break;
            case 5 : chprintf(chp, "%s", text_roaming); break;
            default : chprintf(chp, "%s", text_i_question);; break; // case 4
          }
          chprintf(chp, "%s%s %s%s%u%%", html_e_td_e_tr_tr_td, text_Signal, text_strength, html_e_td_td, gprsStrength);
          chprintf(chp, "%s%s", html_e_td_e_tr, html_e_table);
          // Buttons
          chprintf(chp, "%s%s", html_Apply, html_Save);
          break;
        case PAGE_SETTING:
          // Information table
          chprintf(chp, "%s%s / %s %s%s", html_tr_td, text_Arm, text_Authentication, text_time,
                   html_e_td_td);
          printIntInput(chp, 'D', conf.armDelay / 4, 3, 5, 50);
          chprintf(chp, " %s%s%s %s %s %s%s", durationSelect[0], html_e_td_e_tr_tr_td, text_Auto,
                   text_arm, text_zone, text_delay, html_e_td_td);
          printIntInput(chp, 'E', conf.autoArm, 3, 1, 240);
          chprintf(chp, " %s%s%s %s %s %s%s", durationSelect[1], html_e_td_e_tr_tr_td, text_Zone,
                   text_open, text_alarm, text_delay, html_e_td_td);
          printIntInput(chp, 'F', conf.openAlarm, 3, 1, 240);
          chprintf(chp, " %s%s%s %s%s", durationSelect[1], html_e_td_e_tr_tr_td, text_Admin,
                   text_user, html_e_td_td);
          printTextInput(chp, 'u', conf.user, NAME_LENGTH);
          chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_Admin, text_password,
                   html_e_td_td);
          printPassInput(chp, 'p', conf.password, NAME_LENGTH); chprintf(chp, "%s", html_br);
          printPassInput(chp, 'P', conf.password, NAME_LENGTH);
          chprintf(chp, "%s%s", html_e_td_e_tr, html_e_table);

          chprintf(chp, "<h1>%s</h1>\r\n%s", text_SMTP, html_table);
          chprintf(chp, "%s%s %s%s", html_tr_td, text_Server, text_address, html_e_td_td);
          printTextInput(chp, 'a', conf.SMTPAddress, URL_LENGTH);
          chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_Server, text_port, html_e_td_td);
          printIntInput(chp, 'b', conf.SMTPPort, 5, 0, 65535);
          chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_User, text_name, html_e_td_td);
          printTextInput(chp, 'c', conf.SMTPUser, EMAIL_LENGTH);
          chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_User, text_password, html_e_td_td);
          printPassInput(chp, 'd', conf.SMTPPassword, NAME_LENGTH); chprintf(chp, "%s", html_br);
          printPassInput(chp, 'D', conf.SMTPPassword, NAME_LENGTH);
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
          chprintf(chp, "%sH%s", html_select, html_e_tag);
          for (uint8_t i = 0; i < 24; i++) {
            chprintf(chp, "%s%u", html_option, i);
            if (conf.timeDstHour == i) { chprintf(chp, "%s", html_selected); }
            else                       { chprintf(chp, "%s", html_e_tag); }
            chprintf(chp, "%02u%s", i, html_e_option);
          }
          chprintf(chp, "%s %s", html_e_select, text_oclock);
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
          chprintf(chp, "%sh%s", html_select, html_e_tag);
          for (uint8_t i = 0; i < 24; i++) {
            chprintf(chp, "%s%u", html_option, i);
            if (conf.timeStdHour == i) { chprintf(chp, "%s", html_selected); }
            else                       { chprintf(chp, "%s", html_e_tag); }
            chprintf(chp, "%02u%s", i, html_e_option);
          }
          chprintf(chp, "%s %s", html_e_select, text_oclock);
          chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_Standard, text_offset, html_e_td_td);
          printIntInput(chp, 'o', conf.timeStdOffset, 5, -1440, 1440);
          chprintf(chp, " %s%s", durationSelect[1], html_e_td_e_tr_tr_td);
          chprintf(chp, " %s %s%s", text_Time, text_format, html_e_td_td);
          printTextInput(chp, 'g', conf.dateTimeFormat, NAME_LENGTH);
          chprintf(chp, "%s%s%s", html_e_td_e_tr, html_e_table);

          // JavaScript
          chprintf(chp, "%s%s%s", html_script, JSCredential, html_e_script);
          chprintf(chp, "%sPass.js'>%s", html_script_src, html_e_script);
          // Buttons
          chprintf(chp, "%s%s", html_ApplyValPass, html_Save);
          break;
        case PAGE_TCL:
          // Information table - TCL heap
          chprintf(chp, "%s%s %u%s", html_tr_th, text_Heap, UMM_MALLOC_CFG_HEAP_SIZE/1024, text_kB);
          chprintf(chp, "%s%s", html_e_th_th, text_Used);
          chprintf(chp, "%s%s", html_e_th_th, text_Free);
          chprintf(chp, "%s%s", html_e_th_th, text_Total);
          chprintf(chp, "%s%s%s\r\n", html_e_th_th, text_Metric, html_e_th_e_tr);
          chprintf(chp, "%s%s%s", html_tr_td, text_Entries, html_e_td_td);
          chprintf(chp, "%u%s", ummHeapInfo.usedEntries, html_e_td_td);
          chprintf(chp, "%u%s", ummHeapInfo.freeEntries, html_e_td_td);
          chprintf(chp, "%u%s", ummHeapInfo.totalEntries, html_e_td_td);
          chprintf(chp, "%s %u%%%s", text_Used, umm_usage_metric(), html_e_td_e_tr_tr_td);
          chprintf(chp, "%s%s", text_Blocks, html_e_td_td);
          chprintf(chp, "%u%s", ummHeapInfo.usedBlocks, html_e_td_td);
          chprintf(chp, "%u%s", ummHeapInfo.freeBlocks, html_e_td_td);
          chprintf(chp, "%u%s", ummHeapInfo.totalBlocks, html_e_td_td);
          chprintf(chp, "%s %u%%", text_Fragmentation, umm_fragmentation_metric());
          chprintf(chp, "%s%s", html_e_td_e_tr, html_e_table);

          chprintf(chp, "<i class='fas fa-code' title=\"");
          tcl_list_cmd(&tcl, &chp);
          chprintf(chp, "\"></i>");
          chprintf(chp, "<i class='fas fa-hashtag' title=\"");
          tcl_list_var(&tcl, &chp);
          chprintf(chp, "\"></i>");
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
          chprintf(chp, "%s<pre>%s</pre>", html_br, &tclOutput[0]);
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
            // TODO OHS print node address, but first find node index
            //if (conf.timer[i].toAddress > 0) chprintf(chp, "%s - ", node[conf.timer[i].toAddress].name);
            printNodeAddress(chp, conf.timer[i].toAddress, 'I',  conf.timer[i].toFunction,
                             conf.timer[i].toNumber);
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
          printOnOffButton(chp, "B", GET_CONF_TIMER_ENABLED(conf.timer[webTimer].setting), 0);
          chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, text_Type, html_e_td_td);
          printTwoButton(chp, "C", GET_CONF_TIMER_TYPE(conf.timer[webTimer].setting), true,
                         text_Period, text_Calendar);
          chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, weekDay[0], html_e_td_td);
          printOnOffButton(chp, "J", GET_CONF_TIMER_MO(conf.timer[webTimer].setting), 0);
          chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, weekDay[1], html_e_td_td);
          printOnOffButton(chp, "I", GET_CONF_TIMER_TU(conf.timer[webTimer].setting), 0);
          chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, weekDay[2], html_e_td_td);
          printOnOffButton(chp, "H", GET_CONF_TIMER_WE(conf.timer[webTimer].setting), 0);
          chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, weekDay[3], html_e_td_td);
          printOnOffButton(chp, "G", GET_CONF_TIMER_TH(conf.timer[webTimer].setting), 0);
          chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, weekDay[4], html_e_td_td);
          printOnOffButton(chp, "F", GET_CONF_TIMER_FR(conf.timer[webTimer].setting), 0);
          chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, weekDay[5], html_e_td_td);
          printOnOffButton(chp, "E", GET_CONF_TIMER_SA(conf.timer[webTimer].setting), 0);
          chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, weekDay[6], html_e_td_td);
          printOnOffButton(chp, "D", GET_CONF_TIMER_SU(conf.timer[webTimer].setting), 0);
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
                chprintf(chp, "%s - ", node[i].name);
                printNodeAddress(chp, node[i].address, node[i].type, node[i].function, node[i].number);
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
          chprintf(chp, "%s%s", NOT_SET, html_e_option);
          /*
          for (uint8_t i = 0; i <= SCRIPT_SIZE; i++) {
            if (i < SCRIPT_SIZE) {
              chprintf(chp, "%s%u", html_option, i);
              if (conf.timer[webTimer].evalScript == i) { chprintf(chp, "%s", html_selected); }
              else                                      { chprintf(chp, "%s", html_e_tag); }
              chprintf(chp, "%u. %s%s", i + 1, conf.scriptName[i], html_e_option);
            } else {
              chprintf(chp, "%s%u", html_option, DUMMY_NO_VALUE);
              if (conf.timer[webTimer].evalScript == DUMMY_NO_VALUE) { chprintf(chp, "%s", html_selected); }
              else                                      { chprintf(chp, "%s", html_e_tag); }
              chprintf(chp, "%s%s", NOT_SET, html_e_option);
            }
          }
          */
          chprintf(chp, "%s", html_e_select);
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
        default:
          break;
      }

      // Custom end
      chprintf(chp, "</form></div></div></body></html>\r\n");

      if (file->pextension != NULL) {
        file->data = (const char *)file->pextension;
        file->len = ms.eos;
        file->index = file->len;
        // allow persisteng connections
        file->flags = FS_FILE_FLAGS_HEADER_PERSISTENT;
        return 1;
      }
    }
  }
  return 0;
}

void fs_close_custom(struct fs_file *file)
{
  if (file && file->pextension) {
    mem_free(file->pextension);
    file->pextension = NULL;
  }
}

err_t httpd_post_begin(void *connection, const char *uri, const char *http_request,
                 u16_t http_request_len, int content_len, char *response_uri,
                 u16_t response_uri_len, u8_t *post_auto_wnd) {
  LWIP_UNUSED_ARG(connection);
  LWIP_UNUSED_ARG(http_request);
  LWIP_UNUSED_ARG(http_request_len);
  LWIP_UNUSED_ARG(content_len);
  LWIP_UNUSED_ARG(post_auto_wnd);

  //DBG_HTTP("-PB-connection: %u\r\n", (uint32_t *)connection);
  //DBG_HTTP("-PB-uri: %s\r\n", (char *)uri);
  //DBG_HTTP("-PB-http_request: %s\r\n", (char *)http_request);
  //DBG_HTTP("-PB-http_request_len: %u\r\n", http_request_len);
  //DBG_HTTP("-PB-content_len: %u\r\n", content_len);
  //DBG_HTTP("-PB-response_uri: %s\r\n", (char *)response_uri);
  //DBG_HTTP("-PB-response_uri_len: %u\r\n", response_uri_len);
  //DBG_HTTP("-PB-post_auto_wnd: %u\r\n", *post_auto_wnd);

  if (current_connection != connection) {
    current_connection = connection;
    chsnprintf(response_uri, response_uri_len, uri);
    chsnprintf(current_uri, response_uri_len, uri);
    memset(postData, 0 , sizeof(postData)); // Empty POST data buffer
    return ERR_OK;
  }
  return ERR_VAL;
}

// TODO OHS Rewrite Post function pass pointer to value instead of copy
bool readPostParam(char **pPostData, char *name, uint8_t nameLen, char *value, uint16_t valueLen){
  uint8_t ch, ch1, ch2;

  // clear out name and value so they'll be NULL terminated
  memset(name, 0, nameLen);
  memset(value, 0, valueLen);

  while (**pPostData != 0){
    ch = **pPostData; (*pPostData)++;
    switch (ch) {
      case '+': ch = ' ';
        break;
      case '=': // that's end of name, switch to storing value
        nameLen = 0;
        continue; // do not store '='
        break;
      case '&': // that's end of pair, go away
        return true;
        break;
      case '%':  // handle URL encoded characters by converting back to original form
        ch1 = **pPostData; (*pPostData)++;
        ch2 = **pPostData; (*pPostData)++;
        if (ch1 == 0 || ch2 == 0) return false;
        char hex[3] = { ch1, ch2, 0 };
        ch = strtoul(hex, NULL, 16);
        break;
    }

    // check against 1 so we don't overwrite the final NULL
    if (nameLen > 1) {
      *name++ = ch;
      --nameLen;
    } else {
      if ((valueLen > 1) && (nameLen == 0))  {
        *value++ = ch;
        --valueLen;
      }
    }
  }
  return false; // Null is end
}

err_t httpd_post_receive_data(void *connection, struct pbuf *p) {

  //DBG_HTTP("-PD-connection: %u\r\n", (uint32_t *)connection);
  if (current_connection == connection) {
    //DBG_HTTP("p->payload: %s\r\n", p->payload);
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
    pbuf_free(p);
    return ERR_OK;
  }
  pbuf_free(p);
  return ERR_VAL;
}

void httpd_post_finished(void *connection, char *response_uri, u16_t response_uri_len) {
  LWIP_UNUSED_ARG(connection);
  LWIP_UNUSED_ARG(response_uri_len);

  uint16_t number;
  int8_t resp;
  char name[3];
  uint8_t message[REGISTRATION_SIZE];
  char value[255];
  bool repeat;
  char *ptr, *pEnd;;

  //DBG_HTTP("-PE-connection: %u\r\n", (uint32_t *)connection);
  DBG_HTTP("-PE-postData: %s\r\n", postData);

  if (current_connection == connection) {
    ptr = &postData[0];

    for (uint8_t htmlPage = 0; htmlPage < ARRAY_SIZE(webPage); ++htmlPage) {
      if (!strcmp(current_uri, webPage[htmlPage].link)) {
        switch (htmlPage) {
          case PAGE_NODE:
            do{
              repeat = readPostParam(&ptr, &name[0], sizeof(name), &value[0], sizeof(value));
              DBG_HTTP("Parse: %s=%s<\r\n", name, value);
              switch(name[0]){
                case 'P': // select
                  number = strtol(value, NULL, 10);
                  if (number != webNode) { webNode = number; repeat = 0; }
                break;
                case 'R': // Reregistration
                  resp = sendCmd(15, NODE_CMD_REGISTRATION); // call all to register
                break;
                case 'A': // Apply
                  message[0] = 'R';
                  message[1] = (uint8_t)node[webNode].type;
                  message[2] = (uint8_t)node[webNode].function;
                  message[3] = node[webNode].number;
                  message[4] = (uint8_t)((node[webNode].setting >> 8) & 0b11111111);;
                  message[5] = (uint8_t)(node[webNode].setting & 0b11111111);
                  memcpy(&message[6], node[webNode].name, NAME_LENGTH);
                  resp = sendData(node[webNode].address, message, REGISTRATION_SIZE);
                  /*
                  if (sendData(node[webNode].address, message, REGISTRATION_SIZE) != 1) {
                    // look queue slot
                    _found = DUMMY_NO_VALUE;
                    if (node[webNode].queue != DUMMY_NO_VALUE) {
                      _found = node[webNode].queue; // Replace last message in queue
                    } else {
                      // Look for empty queue slot
                      for (uint8_t i = 0; i < NODE_QUEUE; i++) {
                        if(node_queue[i].expire == 0) { _found = i; break; }
                      }
                    }
                    if (_found != DUMMY_NO_VALUE) {
                      // Put message into queue
                      node_queue[_found].address  = node[webNode].address;
                      node_queue[_found].index    = webNode;
                      node_queue[_found].expire   = timestamp.get() + SECS_PER_HOUR; // Message expires in 1 hour
                      node_queue[_found].length   = REG_LEN;
                      memcpy(node_queue[_found].msg, message, REG_LEN);
                      node[webNode].queue         = _found; // Pointer to message queue
                    } else {
                      pushToLog("FM"); // Message queue is full
                    }
                  }
                  */
                break;
                case 'n': // name
                  strncpy(node[webNode].name, value, NAME_LENGTH);
                break;
                case '0' ... '7': // Handle all single radio buttons for settings
                  if (value[0] == '0') node[webNode].setting &= ~(1 << (name[0]-48));
                  else                 node[webNode].setting |=  (1 << (name[0]-48));
                break;
                case 'g': // group
                  number = strtol(value, NULL, 10);
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
              repeat = readPostParam(&ptr, &name[0], sizeof(name), &value[0], sizeof(value));
              DBG_HTTP("Parse: %s = %s<\r\n", name, value);
              switch(name[0]){
                case 'P': // select
                  number = strtol(value, NULL, 10);
                  if (number != webContact) { webContact = number; repeat = 0; }
                break;
                case 'n': // name
                  strncpy(conf.contactName[webContact], value, NAME_LENGTH);
                break;
                case 'p': // phone number
                  strncpy(conf.contactPhone[webContact], value, PHONE_LENGTH);
                break;
                case 'm': // email
                  strncpy(conf.contactEmail[webContact], value, EMAIL_LENGTH);
                break;
                case '0' ... '7': // Handle all single radio buttons for settings
                  if (value[0] == '0') conf.contact[webContact] &= ~(1 << (name[0]-48));
                  else                 conf.contact[webContact] |=  (1 << (name[0]-48));
                break;
                case 'g': // group
                  number = strtol(value, NULL, 10);
                  SET_CONF_CONTACT_GROUP(conf.contact[webContact], number);
                break;
                case 'e': // save
                  writeToBkpSRAM((uint8_t*)&conf, sizeof(config_t), 0);
                break;
              }
            } while (repeat);
            break;
          case PAGE_KEY:
            do{
              repeat = readPostParam(&ptr, &name[0], sizeof(name), &value[0], sizeof(value));
              DBG_HTTP("Parse: %s = %s<\r\n", name, value);
              switch(name[0]){
                case 'P': // select
                  number = strtol(value, NULL, 10);
                  if (number != webKey) { webKey = number; repeat = 0; }
                break;
                case 'c': // Contact ID
                  conf.keyContact[webKey] = strtol(value, NULL, 10);
                break;
                case 'k': // key
                  conf.keyValue[webKey] = strtoul(value, NULL, 16); // as unsigned long int
                break;
                case '0' ... '7': // Handle all single radio buttons for settings
                  if (value[0] == '0') conf.keySetting[webKey] &= ~(1 << (name[0]-48));
                  else                 conf.keySetting[webKey] |=  (1 << (name[0]-48));
                break;
                //case 'g': // group
                  //number = strtol(value, NULL, 10);
                  //SET_CONF_KEY_GROUP(conf.keySetting[webKey], number);
                //break;
                case 'e': // save
                  writeToBkpSRAM((uint8_t*)&conf, sizeof(config_t), 0);
                break;
              }
            } while (repeat);
            break;
          case PAGE_ZONE:
            do{
              repeat = readPostParam(&ptr, &name[0], sizeof(name), &value[0], sizeof(value));
              DBG_HTTP("Parse: %s = %s<\r\n", name, value);
              switch(name[0]){
                case 'P': // select
                  number = strtol(value, NULL, 10);
                  if (number != webZone) { webZone = number; repeat = 0; }
                break;
                case 'A': // Apply, for remote zone send packet.
                  //
                break;
                case 'n': // name
                  strncpy(conf.zoneName[webZone], value, NAME_LENGTH);
                break;
                case 'd': // delay
                  SET_CONF_ZONE_AUTH_TIME(conf.zone[webKey], (value[0] - 48));
                break;
                case '0' ... '9': // Handle all single radio buttons for settings
                  if (value[0] == '0') conf.zone[webZone] &= ~(1 << (name[0]-48));
                  else                 conf.zone[webZone] |=  (1 << (name[0]-48));
                break;
                case 'a': // Handle all single radio buttons for settings 10 ->
                  if (value[0] == '0') conf.zone[webZone] &= ~(1 << (name[0]-87)); // a(97) - 10
                  else                 conf.zone[webZone] |=  (1 << (name[0]-87));
                break;
                case 'g': // group
                  number = strtol(value, NULL, 10);
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
              repeat = readPostParam(&ptr, &name[0], sizeof(name), &value[0], sizeof(value));
              DBG_HTTP("Parse: %s = %s<\r\n", name, value);
              switch(name[0]){
                case '0' ... ('0' + ARRAY_SIZE(alertType)): // Handle all radio buttons in groups 0 .. #, A .. #
                  if (value[0] == '0') conf.alert[name[0]-48] &= ~(1 << (name[1]-65));
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
              repeat = readPostParam(&ptr, &name[0], sizeof(name), &value[0], sizeof(value));
              DBG_HTTP("Parse: %s = %s<\r\n", name, value);
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
              repeat = readPostParam(&ptr, &name[0], sizeof(name), &value[0], sizeof(value));
              DBG_HTTP("Parse: %s = %s<\r\n", name, value);
              switch(name[0]){
                case 'P': // select
                  number = strtol(value, NULL, 10);
                  if (number != webGroup) { webGroup = number; repeat = 0; }
                break;
                case 'A': // Apply
                break;
                case 'n': // name
                  strncpy(conf.groupName[webGroup], value, NAME_LENGTH);
                break;
                case '0' ... '7': // Handle all single radio buttons for settings
                  if (value[0] == '0') conf.group[webGroup] &= ~(1 << (name[0]-48));
                  else                 conf.group[webGroup] |=  (1 << (name[0]-48));
                break;
                case 'a': // arm chain
                  number = strtol(value, NULL, 10);
                  SET_CONF_GROUP_ARM_CHAIN(conf.group[webGroup], number);
                break;
                case 'd': // disarm chain
                  number = strtol(value, NULL, 10);
                  SET_CONF_GROUP_DISARM_CHAIN(conf.group[webGroup], number);
                break;
                case 'e': // save
                  writeToBkpSRAM((uint8_t*)&conf, sizeof(config_t), 0);
                break;
              }
            } while (repeat);
            break;
          case PAGE_SETTING:
            do{
              repeat = readPostParam(&ptr, &name[0], sizeof(name), &value[0], sizeof(value));
              DBG_HTTP("Parse: %s = %s<\r\n", name, value);
              switch(name[0]){
                case 'A': // Apply
                  // SMTP
                  smtp_set_server_addr(conf.SMTPAddress);
                  smtp_set_server_port(conf.SMTPPort);
                  smtp_set_auth(conf.SMTPUser, conf.SMTPPassword);
                  // SNTP
                  sntp_setservername(0, conf.SNTPAddress);
                break;
                case 'D':
                  conf.armDelay = strtol(value, NULL, 10) * 4;
                break;
                case 'E':
                  conf.autoArm = strtol(value, NULL, 10);
                break;
                case 'F':
                  conf.openAlarm = strtol(value, NULL, 10);
                break;
                case 'u': // user
                  strncpy(conf.user, value, NAME_LENGTH);
                break;
                case 'p': // password
                  strncpy(conf.password, value, NAME_LENGTH);
                break;
                case 'a': // SMTP server
                  strncpy(conf.SMTPAddress, value, URL_LENGTH);
                break;
                case 'b': // SMTP port
                  conf.SMTPPort = strtol(value, NULL, 10);
                break;
                case 'c': // SMTP user
                  strncpy(conf.SMTPUser, value, EMAIL_LENGTH);
                break;
                case 'd': // SMTP password
                  strncpy(conf.SMTPPassword, value, NAME_LENGTH);
                break;
                case 'f': // NTP server
                  strncpy(conf.SNTPAddress, value, URL_LENGTH);
                break;
                case 'w':
                  conf.timeStdWeekNum = strtol(value, NULL, 10);
                break;
                case 's':
                  conf.timeStdDow = strtol(value, NULL, 10);
                break;
                case 'm':
                  conf.timeStdMonth = strtol(value, NULL, 10);
                break;
                case 'h':
                  conf.timeStdHour = strtol(value, NULL, 10);
                break;
                case 'o':
                  conf.timeStdOffset = strtol(value, NULL, 10);
                break;
                case 'W':
                  conf.timeDstWeekNum = strtol(value, NULL, 10);
                break;
                case 'S':
                  conf.timeDstDow = strtol(value, NULL, 10);
                break;
                case 'M':
                  conf.timeDstMonth = strtol(value, NULL, 10);
                break;
                case 'H':
                  conf.timeDstHour = strtol(value, NULL, 10);
                break;
                case 'O':
                  conf.timeDstOffset = strtol(value, NULL, 10);
                break;
                case 'g': // time format
                  strncpy(conf.dateTimeFormat, value, NAME_LENGTH);
                break;

                case 'e': // save
                  writeToBkpSRAM((uint8_t*)&conf, sizeof(config_t), 0);
                break;
              }
            } while (repeat);
            break;
          case PAGE_HOME:
            do{
              repeat = readPostParam(&ptr, &name[0], sizeof(name), &value[0], sizeof(value));
              DBG_HTTP("Parse: %s = %s<\r\n", name, value);
              switch(name[0]){
                case 'A': // Apply
                break;
              }
            } while (repeat);
            break;
          case PAGE_TCL:
            do{
              repeat = readPostParam(&ptr, &name[0], sizeof(name), &value[0], sizeof(value));
              DBG_HTTP("Parse: %s = %s<\r\n", name, value);
              scriptEvent_t *outMsg;
              switch(name[0]){
                case 'P': // select
                  number = strtol(value, NULL, 10);
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
                  strncpy(scriptName, value, NAME_LENGTH);
                  break;
                case 'R': // Run
                  outMsg = chPoolAlloc(&script_pool);
                  if (outMsg != NULL) {
                    outMsg->callback = NULL;
                    outMsg->result = NULL;
                    outMsg->flags = 1;
                    outMsg->index = &tclCmd[0];
                    msg_t msg = chMBPostTimeout(&script_mb, (msg_t)outMsg, TIME_IMMEDIATE);
                    if (msg != MSG_OK) {
                      //chprintf(console, "MB full %d\r\n", temp);
                    }
                  } else {
                    chprintf(console, "CB full %d \r\n", outMsg);
                  }
                break;
                case 's': // script
                  strncpy(tclCmd, value, TCL_SCRIPT_LENGTH);
                break;
                case 'e': // save
                  // TODO OHS Add malloc, realloc checks for return NULL pointer.
                  if (webScript == DUMMY_NO_VALUE) {
                    // For new script append linked list
                    scriptp = umm_malloc(sizeof(struct scriptLL_t));
                    scriptp->name = umm_malloc(NAME_LENGTH + 1);
                    strncpy(scriptp->name, &scriptName[0], NAME_LENGTH);
                    number = strlen(tclCmd);
                    scriptp->cmd = umm_malloc(number + 1);
                    memset(scriptp->cmd + number, 0, 1);
                    strncpy(scriptp->cmd, &tclCmd[0], number);
                    scriptp->next = scriptLL;
                    scriptLL = scriptp;
                    // uBS
                    uBSWrite(&scriptName[0], NAME_LENGTH, &tclCmd[0], strlen(tclCmd));
                    // new script is added to top of linked list, no need to do pointer check
                    webScript = 1;
                  } else {
                    // For old script replace values
                    number = 1;
                    // Find pointer to script
                    for (scriptp = scriptLL; scriptp != NULL; scriptp = scriptp->next) {
                      if (number == webScript) break;
                      number++;
                    }
                    if (scriptp != NULL) {
                      // Do we need to rename it
                      if (strcmp(scriptp->name, &scriptName[0]) != 0) {
                        DBG_HTTP("Rename: %x, %x\r\n", scriptp, scriptp->name);
                        strncpy(scriptp->name, &scriptName[0], NAME_LENGTH);
                        uBSRename(scriptp->name, &scriptName[0], NAME_LENGTH);
                      }
                      number = strlen(tclCmd);
                      //scriptp->cmd = umm_realloc(scriptp->cmd, number + 1);
                      umm_free(scriptp->cmd);
                      scriptp->cmd = umm_malloc(number + 1);
                      strncpy(scriptp->cmd, &tclCmd[0], number);
                      memset(scriptp->cmd + number, 0, 1);
                      uBSWrite(&scriptName[0], NAME_LENGTH, &tclCmd[0], strlen(tclCmd));
                    }
                  }
                break;
              }
            } while (repeat);
            break;
          case PAGE_TIMER:
            do{
              repeat = readPostParam(&ptr, &name[0], sizeof(name), &value[0], sizeof(value));
              DBG_HTTP("Parse: %s = %s<\r\n", name, value);
              switch(name[0]){
                case 'P': // select
                  number = strtol(value, NULL, 10);
                  if (number != webTimer) { webTimer = number; repeat = 0; }
                break;
                case 'A': // Apply
                  setTimer(webTimer, true);
                break;
                case 'n': // name
                  strncpy(conf.timer[webTimer].name, value, NAME_LENGTH);
                break;
                case 's': // period
                  conf.timer[webTimer].periodTime = strtol(value, NULL, 10);
                break;
                case 'S': // period
                  number = strtol(value, NULL, 10);
                  SET_CONF_TIMER_PERIOD_TYPE(conf.timer[webTimer].setting, number);
                break;
                case 'r': // run
                  conf.timer[webTimer].runTime = strtol(value, NULL, 10);
                break;
                case 'R': // run
                  number = strtol(value, NULL, 10);
                  SET_CONF_TIMER_RUN_TYPE(conf.timer[webTimer].setting, number);
                break;
                case 'a': // node aaddress
                  number = strtol(value, NULL, 10);
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
                  conf.timer[webTimer].constantOn = strtof(value, NULL);
                break;
                case 'f':
                  conf.timer[webTimer].constantOff = strtof(value, NULL);
                break;
                case 't': // time
                  conf.timer[webTimer].startTime = strtol(value, &pEnd, 10) * MINUTES_PER_HOUR ;
                  conf.timer[webTimer].startTime += strtol(++pEnd, NULL, 10);
                break;
                case 'p': // script
                  strncpy(conf.timer[webTimer].evalScript, value, NAME_LENGTH);
                break;
                case 'B' ... 'J': // Handle all single radio buttons for settings B(66)=0
                  if (value[0] == '0') conf.timer[webTimer].setting &= ~(1 << (name[0]-66));
                  else                 conf.timer[webTimer].setting |=  (1 << (name[0]-66));
                break;
                case 'e': // save
                  writeToBkpSRAM((uint8_t*)&conf, sizeof(config_t), 0);
                break;
              }
            } while (repeat);
            break;
          default:
            break;
        }
      }
    }

    chsnprintf(response_uri, response_uri_len, current_uri);
    DBG_HTTP("-PE-response_uri: %s\r\n", response_uri);
    current_connection = NULL;
  }
}

#endif /* OHS_HTTPDHANDLER_H_ */
