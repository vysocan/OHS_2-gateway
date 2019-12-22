/*
 * ohs_httpdhandler.h
 *
 *  Created on: 6. 12. 2019
 *      Author: vysocan
 */

#ifndef OHS_HTTPDHANDLER_H_
#define OHS_HTTPDHANDLER_H_

#include "lwip/opt.h"
#include "genfiles_example.h"

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

static void *current_connection;
static void *valid_connection;
static char current_uri[LWIP_HTTPD_MAX_REQUEST_URI_LEN];

/* This is the page we send. It's not generated, as you see.
 * Generating custom things instead of memcpy is left to your imagination :-)
 */
const char generated_html[] =
"<html>\n"
"<head><title>lwIP - A Lightweight TCP/IP Stack</title></head>\n"
" <body bgcolor=\"white\" text=\"black\">\n"
"  <table width=\"100%\">\n"
"   <tr valign=\"top\">\n"
"    <td width=\"80\">\n"
"     <a href=\"http://www.sics.se/\"><img src=\"/img/OHS.gif\"\n"
"      border=\"0\" alt=\"SICS logo\" title=\"SICS logo\"></a>\n"
"    </td>\n"
"    <td width=\"500\">\n"
"     <h1>lwIP - A Lightweight TCP/IP Stack</h1>\n"
"     <h2>Generated page</h2>\n"
"     <p>This page might be generated in-memory at runtime</p>\n"
"    </td>\n"
"    <td>\n"
"    &nbsp;\n"
"    </td>\n"
"   </tr>\n"
"  </table>\n"
"<form name=\"login\" action=\"generated.html\" method=\"post\""
"<div><label>Username</label><input type=\"text\" name=\"user\">"
"<label>Password</label><input type=\"text\" name=\"pass\">"
"<button type=\"submit\">Login</button>"
"</div></form>"
" </body>\n"
"</html>";

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
const char html_s_tag_1[]           = "<input type='text' maxlength='";
const char html_s_tag_2[]           = "' size='";
const char html_s_tag_3[]           = "' name='";
const char html_radio_s[]           = "<div class='rm'>";
const char html_radio_sl[]          = "<div class='rml'>";
const char html_radio_sb[]          = "<div class='rmb'>";
const char html_div_e[]             = "</div>";
const char html_select[]            = "<select name='";
const char html_Apply[]             = "<input type='submit' name='A' value='Apply'/>";
const char html_Save[]              = "<input type='submit' name='e' value='Save all'/>";
const char html_Reregister[]        = "<input type='submit' name='R' value='Call registration'/>";
const char html_e_table[]           = "</table>";
const char html_table[]             = "<table>";
const char html_e_form[]            = "</form>";
const char html_form_1[]            = "<form action='";
const char html_form_2[]            = "' method='post'>";

const char html_cbPart1a[]          = "<div class='rc'><input type='radio' name='";
const char html_cbPart1b[]          = "' id='";
const char html_cbPart2[]           = "' value='";
const char html_cbPart3[]           = "'";
const char html_cbChecked[]         = "' checked ";
const char html_cbPart4a[]          = "><label for='";
const char html_cbPart4b[]          = "'>";
const char html_cbPart5[]           = "</label></div>";
const char html_cbJSen[]            = " onclick=\"en()\"";
const char html_cbJSdis[]           = " onclick=\"dis()\"";

#define HTML_SIZE  12000 // Change also in lwip opt.h MEM_SIZE
#define HTML_PAGES 4
const char webMenuLink[HTML_PAGES][13]  = {
   "/index.html",
   "/test.html",
   "/contact.html",
   "/zone.html"
};
const char webMenuName[HTML_PAGES][10]  = {
   "Global",
   "Test",
   "Contact",
   "Zone"
};

static char postData[128];
static char *pPostData;

void printOkNok(BaseSequentialStream *chp, const uint8_t value) {
  value ? chprintf(chp, "%s", text_i_OK) : chprintf(chp, "%s", text_i_disabled);
}

void printRadioButton(BaseSequentialStream *chp, const char *name, const uint8_t val,
                 const char *label, bool selected, bool enableJS) {
  chprintf(chp, "%s%s%s", html_cbPart1a, name, html_cbPart1b);
  chprintf(chp, "%s%u%s%u", name, val, html_cbPart2, val);
  if (selected) chprintf(chp, "%s", html_cbChecked);
  else          chprintf(chp, "%s", html_cbPart3);
  if (enableJS) {
    if (val) chprintf(chp, "%s", html_cbJSen);
    else     chprintf(chp, "%s", html_cbJSdis);
  }
  chprintf(chp, "%s%s%u", html_cbPart4a, name, val);
  chprintf(chp, "%s%s%s", html_cbPart4b, label, html_cbPart5);
}

#define GET_BUTTON_STATE(x,y) (x==y)
void printFourButton(BaseSequentialStream *chp, const char *name, const uint8_t state, const bool enableJS) {
  chprintf(chp, "%s", html_radio_sl);
  printRadioButton(chp, name, 0, text_1, GET_BUTTON_STATE(state, 0), enableJS);
  printRadioButton(chp, name, 1, text_2, GET_BUTTON_STATE(state, 1), enableJS);
  printRadioButton(chp, name, 2, text_3, GET_BUTTON_STATE(state, 2), enableJS);
  printRadioButton(chp, name, 3, text_4, GET_BUTTON_STATE(state, 3), enableJS);
  chprintf(chp, "%s", html_div_e);
}

void  printOnOffButton(BaseSequentialStream *chp, const char *name, const uint8_t state, const bool enableJS) {
  chprintf(chp, "%s", html_radio_s);
  printRadioButton(chp, name, 1, text_On, state, enableJS);
  printRadioButton(chp, name, 0, text_Off, !state, enableJS);
  chprintf(chp, "%s", html_div_e);
}

void printGroup(BaseSequentialStream *chp, const uint8_t value) {
  if (value < ALARM_GROUPS) {
    chprintf(chp, "%u. %s", value + 1, conf.groupName[value]);
  } else chprintf(chp, "%s", NOT_SET);
}

void selectGroup(BaseSequentialStream *chp, uint8_t selected, char name) {
  chprintf(chp, "%s%c%s", html_select, name, html_e_tag);
  for (uint8_t i = 0; i < ALARM_GROUPS; i++) {
    chprintf(chp, "%s%u", html_option, i);
    if (selected == i) { chprintf(chp, "%s", html_selected); }
    else               { chprintf(chp, "%s", html_e_tag); }
    chprintf(chp, "%u. %s - ", i + 1, conf.groupName[i]);
    GET_CONF_GROUP_ENABLED(conf.group[i]) ? chprintf(chp, "%s", text_On) : chprintf(chp, "%s", text_Off);
    chprintf(chp, "%s", html_e_option);
  }
  chprintf(chp, "%s", html_e_select);
}

void printNodeValue(BaseSequentialStream *chp, const uint8_t index) {
  if (node[index].type != 'K') {
    switch(node[index].function){
      case 'T': chprintf(chp, "%.2f �C", node[index].value); break;
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
  chprintf(chp, "%s%u%s%u%s", html_s_tag_1, size, html_s_tag_2, size, html_s_tag_3);
  chprintf(chp, "%c%s%s", name, html_m_tag, value);
  chprintf(chp, "%s%c%s", html_id_tag, name, html_e_tag);
}

void printKey(BaseSequentialStream *chp, const char *value, const uint8_t size){
  for (uint8_t i = 0; i < size; ++i) {
    chprintf(chp, "%02x", value[i]);
  }
}


void genfiles_ex_init(void) {
  /* nothing to do here yet */
}

static uint8_t webNode = 0, webContact = 0, webKey = 0, webZone = 0;
int fs_open_custom(struct fs_file *file, const char *name){
  for (uint8_t htmlPage = 0; htmlPage < HTML_PAGES; ++htmlPage) {
    if (!strcmp(name, webMenuLink[htmlPage])) {
      /* initialize fs_file correctly */
      memset(file, 0, sizeof(struct fs_file));
      file->pextension = mem_malloc(HTML_SIZE);

      MemoryStream ms;
      BaseSequentialStream *chp;
      // Memory stream object to be used as a string writer, reserving one byte for the final zero.
      msObjectInit(&ms, (uint8_t *)file->pextension, HTML_SIZE-1, 0);
      // Performing the print operation using the common code.
      chp = (BaseSequentialStream *)(void *)&ms;

      chprintf(chp, "<!DOCTYPE html><html lang='en'><head><meta charset='utf-8'><title>Open home security</title>\r\n");
      chprintf(chp, "<link rel='stylesheet' href='/css/OHS.css'></head>\r\n");
      chprintf(chp, "<body onload=\"\"><div class='wrp'><div class='sb'>\r\n");
      chprintf(chp, "<div class='tt'>OHS 2.0 - %u.%u</div>\r\n", OHS_MAJOR, OHS_MINOR);
      chprintf(chp, "<ul class='nav'>\r\n");
      for (uint8_t i = 0; i < HTML_PAGES; ++i) {
        if (htmlPage == i) chprintf(chp, "<li><a class='active' href='%s'>%s</a></li>\r\n", webMenuLink[i], webMenuName[i]);
        else chprintf(chp, "<li><a href='%s'>%s</a></li>\r\n", webMenuLink[i], webMenuName[i]);
      }
      chprintf(chp, "</ul></div><div class='mb'><h1>%s</h1>\r\n", webMenuName[htmlPage]);

      // Custom start
      switch (htmlPage) {
        case 0:
          chprintf(chp, "%s%s#", html_table, html_tr_th);
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
              printNodeAddress(chp, node[i].address, node[i].number);
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
          // Form
          chprintf(chp, "%s%s%s", html_form_1, webMenuLink[htmlPage], html_form_2);
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
          printNodeAddress(chp, node[webNode].address, node[webNode].number);
          chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, text_Type, html_e_td_td);
          printNodeType(chp, node[webNode].type);
          chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, text_Function, html_e_td_td);
          printNodeFunction(chp, node[webNode].function);
          chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_Node, text_is, html_e_td_td);
          printOnOffButton(chp, "0", GET_NODE_ENABLED(node[webNode].setting), false);
          chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_MQTT, text_publish, html_e_td_td);
          printOnOffButton(chp, "7", GET_NODE_MQTT_PUB(node[webNode].setting), false);
          chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, text_Group, html_e_td_td);
          selectGroup(chp, GET_NODE_GROUP(node[webNode].setting), 'g');
          chprintf(chp, "%s%s", html_e_td_e_tr, html_e_table);
          // Buttons
          chprintf(chp, "%s%s%s%s", html_Apply, html_Save, html_Reregister, html_e_form);
          break;
        case 1:
          chprintf(chp, "%s%s#", html_table, html_tr_th);
          chprintf(chp, "%s%s", html_e_th_th, text_Name);
          chprintf(chp, "%s%s", html_e_th_th, text_On);
          chprintf(chp, "%s%s", html_e_th_th, text_Number);
          chprintf(chp, "%s%s", html_e_th_th, text_Email);
          chprintf(chp, "%s%s", html_e_th_th, text_Global);
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
            if (!GET_CONF_CONTACT_IS_GLOBAL(conf.contact[i])) printGroup(chp, GET_CONF_CONTACT_GROUP(conf.contact[i]));
            else chprintf(chp, "%s", text_all);
            chprintf(chp, "%s", html_e_td_e_tr);
          }
          chprintf(chp, "%s", html_e_table);
          // Form
          chprintf(chp, "%s%s%s", html_form_1, webMenuLink[htmlPage], html_form_2);
          chprintf(chp, "%s", html_table);
          chprintf(chp, "%s%s%s", html_tr_td, text_Contact, html_e_td_td);
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
          chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_Contact, text_is, html_e_td_td);
          printOnOffButton(chp, "0", GET_CONF_CONTACT_ENABLED(conf.contact[webContact]), false);
          chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, text_Number, html_e_td_td);
          printTextInput(chp, 'p', conf.contactPhone[webContact], PHONE_LENGTH);
          chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, text_Email, html_e_td_td);
          printTextInput(chp, 'm', conf.contactEmail[webContact], EMAIL_LENGTH);
          chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, text_Global, html_e_td_td);
          printOnOffButton(chp, "5", GET_CONF_CONTACT_IS_GLOBAL(conf.contact[webContact]), false);
          chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, text_Group, html_e_td_td);
          selectGroup(chp, GET_CONF_CONTACT_GROUP(conf.contact[webContact]), 'g');
          chprintf(chp, "%s%s", html_e_td_e_tr, html_e_table);
          // Buttons
          chprintf(chp, "%s%s%s", html_Apply, html_Save, html_e_form);
          break;
        case 2:
          chprintf(chp, "%s%s#", html_table, html_tr_th);
          chprintf(chp, "%s%s", html_e_th_th, text_Name);
          chprintf(chp, "%s%s", html_e_th_th, text_On);
          chprintf(chp, "%s%s", html_e_th_th, text_Value);
          chprintf(chp, "%s%s", html_e_th_th, text_Global);
          chprintf(chp, "%s%s%s\r\n", html_e_th_th, text_Group, html_e_th_e_tr);
          // Information table
          for (uint8_t i = 0; i < KEYS_SIZE; i++) {
            chprintf(chp, "%s%u.%s", html_tr_td, i + 1, html_e_td_td);
            if (conf.keyContact[i] == 255) chprintf(chp, "%s", NOT_SET);
            else chprintf(chp, "%s", conf.contactName[conf.keyContact[i]]);
            chprintf(chp, "%s", html_e_td_td);
            printOkNok(chp, GET_CONF_KEY_ENABLED(conf.keySetting[i]));
            chprintf(chp, "%s", html_e_td_td);
            printKey(chp, conf.keyValue[i], KEY_LENGTH);
            chprintf(chp, "%s", html_e_td_td);
            printOkNok(chp, GET_CONF_KEY_IS_GLOBAL(conf.keySetting[i]));
            chprintf(chp, "%s", html_e_td_td);
            if (!GET_CONF_KEY_IS_GLOBAL(conf.keySetting[i])) printGroup(chp, GET_CONF_KEY_GROUP(conf.keySetting[i]));
            else chprintf(chp, "%s", text_all);
            chprintf(chp, "%s", html_e_td_e_tr);
          }
          chprintf(chp, "%s", html_e_table);
          // Form
          chprintf(chp, "%s%s%s", html_form_1, webMenuLink[htmlPage], html_form_2);
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
          chprintf(chp, "%s%s", text_Contact, html_e_td_td);
          chprintf(chp, "%sc%s", html_select, html_e_tag);
          for (uint8_t i = 0; i < CONTACTS_SIZE; i++) {
            chprintf(chp, "%s%u", html_option, i);
            if (conf.keyContact[webKey] == i) { chprintf(chp, "%s", html_selected); }
            else                              { chprintf(chp, "%s", html_e_tag); }
            chprintf(chp, "%u. %s%s", i + 1, conf.contactName[i], html_e_option);
          }
          chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_Key, text_is, html_e_td_td);
          printOnOffButton(chp, "0", GET_CONF_KEY_ENABLED(conf.keySetting[webKey]), false);
          chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, text_Value, html_e_td_td);
          chprintf(chp, "%s%u%s%u", html_s_tag_1, KEY_LENGTH * 2, html_s_tag_2, KEY_LENGTH * 2);
          chprintf(chp, "%sk%s", html_s_tag_3, html_m_tag);
          printKey(chp, conf.keyValue[webKey], KEY_LENGTH);
          chprintf(chp, "%s%k%s", html_id_tag, html_e_tag);
          chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, text_Global, html_e_td_td);
          printOnOffButton(chp, "5", GET_CONF_KEY_IS_GLOBAL(conf.keySetting[webKey]), false);
          chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, text_Group, html_e_td_td);
          selectGroup(chp, GET_CONF_KEY_GROUP(conf.keySetting[webKey]), 'g');
          chprintf(chp, "%s%s", html_e_td_e_tr, html_e_table);
          // Buttons
          chprintf(chp, "%s%s%s", html_Apply, html_Save, html_e_form);
          break;
        case 3:
          chprintf(chp, "%s%s#", html_table, html_tr_th);
          chprintf(chp, "%s%s", html_e_th_th, text_Name);
          chprintf(chp, "%s%s", html_e_th_th, text_On);
          chprintf(chp, "%s%s", html_e_th_th, text_Type);
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
              GET_CONF_ZONE_IS_REMOTE(conf.zone[i]) ? chprintf(chp, "%s ", text_remote) : chprintf(chp, "%s ", text_local);
              if (GET_CONF_ZONE_IS_BATTERY(conf.zone[i])) chprintf(chp, "%s ", text_battery);
              GET_CONF_ZONE_TYPE(conf.zone[i]) ? chprintf(chp, "%s", text_analog) : chprintf(chp, "%s", text_digital);
              chprintf(chp, "%s", html_e_td_td);
              printOkNok(chp, GET_CONF_ZONE_STILL_OPEN(conf.zone[i]));
              chprintf(chp, "%s", html_e_td_td);
              printOkNok(chp, GET_CONF_ZONE_PIR_AS_TMP(conf.zone[i]));
              chprintf(chp, "%s", html_e_td_td);
              chprintf(chp, "%u %s%s", GET_CONF_ZONE_AUTH_TIME(conf.zone[i])*conf.alarmTime, text_seconds, html_e_td_td);
              printFrmTimestamp(chp, &zone[i].lastPIR);
              chprintf(chp, "%s", html_e_td_td);
              printFrmTimestamp(chp, &zone[i].lastOK);
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
          // Form
          chprintf(chp, "%s%s%s", html_form_1, webMenuLink[htmlPage], html_form_2);
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
          printOnOffButton(chp, "0", GET_CONF_ZONE_ENABLED(conf.zone[webZone]), false);
          chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_Open, text_alarm, html_e_td_td);
          printOnOffButton(chp, "8", GET_CONF_ZONE_STILL_OPEN(conf.zone[webZone]), false);
          chprintf(chp, "%s%s %s %s%s", html_e_td_e_tr_tr_td,  text_Alarm, text_as, text_tamper, html_e_td_td);
          printOnOffButton(chp, "9", GET_CONF_ZONE_PIR_AS_TMP(conf.zone[webZone]), false);
          chprintf(chp, "%s%s %s%s", html_e_td_e_tr_tr_td, text_Authentication, text_delay, html_e_td_td);
          printFourButton(chp, "d", GET_CONF_ZONE_AUTH_TIME(conf.zone[webZone]), false);
          chprintf(chp, "%s%s%s", html_e_td_e_tr_tr_td, text_Group, html_e_td_td);
          selectGroup(chp, GET_CONF_ZONE_GROUP(conf.zone[webZone]), 'g');
          chprintf(chp, "%s%s", html_e_td_e_tr, html_e_table);
          // Buttons
          chprintf(chp, "%s%s%s", html_Apply, html_Save, html_e_form);
          break;
        default:
          break;
      }

      // Custom end
      chprintf(chp, "</div></div></body></html>\r\n");


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

  /* this example only provides one file */
  if (!strcmp(name, "/generated.html")) {
    /* initialize fs_file correctly */
    memset(file, 0, sizeof(struct fs_file));
    file->pextension = mem_malloc(sizeof(generated_html));
    if (file->pextension != NULL) {
      /* instead of doing memcpy, you would generate e.g. a JSON here */
      memcpy(file->pextension, generated_html, sizeof(generated_html));
      file->data = (const char *)file->pextension;
      file->len = sizeof(generated_html) - 1; /* don't send the trailing 0 */
      file->index = file->len;
      /* allow persisteng connections */
      file->flags = FS_FILE_FLAGS_HEADER_PERSISTENT;
      return 1;
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
  LWIP_UNUSED_ARG(response_uri_len);

  //chprintf((BaseSequentialStream*)&SD3, "connection: %X\r\n", (uint32_t *)connection);
  //chprintf((BaseSequentialStream*)&SD3, "response_uri: %s\r\n", (char *)response_uri);
  //chprintf((BaseSequentialStream*)&SD3, "uri: %s\r\n", (char *)uri);
  //chprintf((BaseSequentialStream*)&SD3, "http_request: %s\r\n", (char *)http_request);

  strcpy(current_uri, uri);
  strcpy(response_uri, uri);
  response_uri_len = strlen(response_uri);
  memset(postData, 0 , sizeof(postData)); // Empty POST data buffer

  return ERR_OK;
}

bool readPostParam(char *name, uint8_t nameLen, char *value, uint8_t valueLen){
  uint8_t ch;

  // clear out name and value so they'll be NUL terminated
  memset(name, 0, nameLen);
  memset(value, 0, valueLen);

  while (*pPostData != 0){
    ch = *pPostData; pPostData++;
    if (ch == '+') {ch = ' ';}
    else if (ch == '=') {
      // that's end of name, so switch to storing in value
      nameLen = 0;
      continue;
    }
    else if (ch == '&') {
      // that's end of pair, go away
      return true;
    }
    else if (ch == '%') {
      // handle URL encoded characters by converting back to original form
      uint8_t ch1 = *pPostData; pPostData++;
      uint8_t ch2 = *pPostData; pPostData++;
      if (ch1 == 0 || ch2 == 0) return false;
      char hex[3] = { ch1, ch2, 0 };
      ch = strtoul(hex, NULL, 16);
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
  LWIP_UNUSED_ARG(connection);

  strncat(postData, (const char *)p->payload, sizeof(postData)-strlen(postData)-1); // Maximum size of postData and null
  chprintf((BaseSequentialStream*)&SD3, "POST: %s\r\n", p->payload);
  chprintf((BaseSequentialStream*)&SD3, "postData: %s\r\n", postData);

  return ERR_OK;
}

void httpd_post_finished(void *connection, char *response_uri, u16_t response_uri_len) {
  LWIP_UNUSED_ARG(connection);
  LWIP_UNUSED_ARG(response_uri_len);

  uint8_t number;
  int8_t resp;
  char name[2];
  uint8_t message[REGISTRATION_SIZE];
  char value[EMAIL_LENGTH + 1];
  bool repeat;

  chprintf((BaseSequentialStream*)&SD3, "postData: %s\r\n", postData);
  chprintf((BaseSequentialStream*)&SD3, "current_uri: %s\r\n", current_uri);

  pPostData = &postData[0];

  for (uint8_t htmlPage = 0; htmlPage < HTML_PAGES; ++htmlPage) {
    if (!strcmp(current_uri, webMenuLink[htmlPage])) {
      switch (htmlPage) {
        case 0:
          do{
            repeat = readPostParam(name, sizeof(name), value, sizeof(value));
            chprintf((BaseSequentialStream*)&SD3, "Parse: %s = %s<\r\n", name, value);
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
                if (!sendData(node[webNode].address, message, REGISTRATION_SIZE)) {
                  // look queue slot
                  _found = 255;
                  if (node[webNode].queue != 255) {
                    _found = node[webNode].queue; // Replace last message in queue
                  } else {
                    // Look for empty queue slot
                    for (uint8_t i = 0; i < NODE_QUEUE; i++) {
                      if(node_queue[i].expire == 0) { _found = i; break; }
                    }
                  }
                  if (_found != 255) {
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
                strncpy (node[webNode].name, value, NAME_LENGTH);
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
        case 1:
          do{
            repeat = readPostParam(name, sizeof(name), value, sizeof(value));
            chprintf((BaseSequentialStream*)&SD3, "Parse: %s = %s<\r\n", name, value);
            switch(name[0]){
              case 'P': // select
                number = strtol(value, NULL, 10);
                if (number != webContact) { webContact = number; repeat = 0; }
              break;
              case 'n': // name
                strncpy (conf.contactName[webContact], value, NAME_LENGTH);
              break;
              case 'p': // phone number
                strncpy (conf.contactPhone[webContact], value, PHONE_LENGTH);
              break;
              case 'm': // email
                strncpy (conf.contactEmail[webContact], value, EMAIL_LENGTH);
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
        case 2:
          do{
            repeat = readPostParam(name, sizeof(name), value, sizeof(value));
            chprintf((BaseSequentialStream*)&SD3, "Parse: %s = %s<\r\n", name, value);
            switch(name[0]){
              case 'P': // select
                number = strtol(value, NULL, 10);
                if (number != webKey) { webKey = number; repeat = 0; }
              break;
              case 'c': // Contact ID
                conf.keyContact[webKey] = strtol(value, NULL, 10);
              break;
              case 'k': // key
                for (uint8_t j = 0; j < KEY_LENGTH; j++) {
                  if (value[j*2] > '9') value[j*2] = value[j*2] - 'A' + 10;
                  else value[j*2] = value[j*2] - '0';
                  if (value[j*2+1] > '9') value[j*2+1] = value[j*2+1] - 'A' + 10;
                  else value[j*2+1] = value[j*2+1] - '0';
                  conf.keyValue[webKey][j] = 16*value[j*2] + value[j*2+1];
                }
              break;
              case '0' ... '7': // Handle all single radio buttons for settings
                if (value[0] == '0') conf.keySetting[webKey] &= ~(1 << (name[0]-48));
                else                 conf.keySetting[webKey] |=  (1 << (name[0]-48));
              break;
              case 'g': // group
                number = strtol(value, NULL, 10);
                SET_CONF_KEY_GROUP(conf.keySetting[webKey], number);
              break;
              case 'e': // save
                writeToBkpSRAM((uint8_t*)&conf, sizeof(config_t), 0);
              break;
            }
          } while (repeat);
          break;
        case 3:
          do{
            repeat = readPostParam(name, sizeof(name), value, sizeof(value));
            chprintf((BaseSequentialStream*)&SD3, "Parse: %s = %s<\r\n", name, value);
            switch(name[0]){
              case 'P': // select
                number = strtol(value, NULL, 10);
                if (number != webZone) { webZone = number; repeat = 0; }
              break;
              case 'A': // Apply
              break;
              case 'n': // name
                strncpy (conf.zoneName[webKey], value, NAME_LENGTH);
              break;
              case 'd': // delay
                SET_CONF_ZONE_AUTH_TIME(conf.zone[webKey], (value[0] - 48));
              break;
              case '0' ... '9': // Handle all single radio buttons for settings
                if (value[0] == '0') conf.zone[webZone] &= ~(1 << (name[0]-48));
                else                 conf.zone[webZone] |=  (1 << (name[0]-48));
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
        default:
          break;
      }

    }
  }


  strcpy(response_uri, current_uri);
  response_uri_len = strlen(response_uri);
}

#endif /* OHS_HTTPDHANDLER_H_ */
