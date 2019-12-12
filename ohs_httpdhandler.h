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
const char html_tr_td[]             = "<tr><td>";
const char html_e_td_td[]           = "</td><td>";
const char html_e_td_e_tr[]         = "</td></tr>";
const char html_e_td_e_tr_tr_td[]   = "</td></tr><tr><td>";
const char html_select_submit[]     = "<select onchange='this.form.submit()' name='";
const char html_e_tag[]             = "'>";
const char html_e_select[]          = "</select>";
const char html_option[]            = "<option value='";
const char html_e_option[]          = "</option>";
const char html_selected[]          = "' selected>";
const char html_m_tag[]             = "' value='";
const char html_id_tag[]            = "' id='";
const char html_s_tag[]             = "<input type='text' name='";
const char html_s_tag_s[]           = "<input type='text' maxlength='3' size='3' name='";
const char html_radio_s[]           = "<div class='rm'>";
const char html_radio_sl[]          = "<div class='rml'>";
const char html_radio_sb[]          = "<div class='rmb'>";
const char html_div_e[]             = "</div>";
const char html_select[]            = "<select name='";

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


#define HTML_SIZE  4096 // Change also in lwip opt.h MEM_SIZE
#define HTML_PAGES 2
const char webMenuLink[HTML_PAGES][12]  = {
   "/index.html",
   "/test.html"
};
const char webMenuName[HTML_PAGES][12]  = {
   "Global",
   "Test"
};

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

void printOnOffButton(BaseSequentialStream *chp, const char *name, const uint8_t state, const bool enableJS) {
  chprintf(chp, "%s", html_radio_s);
  printRadioButton(chp, name, 1, text_On, state, enableJS);
  printRadioButton(chp, name, 0, text_Off, !state, enableJS);
  chprintf(chp, "%s", html_div_e);
}

void printGroup(BaseSequentialStream *chp, const uint8_t value) {
  if (value < ALARM_GROUPS) {
    chprintf(chp, "%u - %s", value + 1, conf.groupName[value]);
  } else chprintf(chp, "%s", NOT_SET);
}

void selectGroup(BaseSequentialStream *chp, uint8_t selected, char name) {
  chprintf(chp, "%s%c%s", html_select, name, html_e_tag);
  for (uint8_t i = 0; i < ALARM_GROUPS; i++) {
    chprintf(chp, "%s%u", html_option, i);
    if (selected == i) { chprintf(chp, "%s", html_selected); }
    else               { chprintf(chp, "%s", html_e_tag); }
    chprintf(chp, "%u - %s - ", i + 1, conf.groupName[i]);
    GET_CONF_GROUP_ENABLED(conf.group[i]) ? chprintf(chp, "%s", text_On) : chprintf(chp, "%s", text_Off);
    chprintf(chp, "%s", html_e_option);
  }
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

void printTextInput(BaseSequentialStream *chp, const char name, const char *value, const uint8_t type){
  if (type) chprintf(chp, "%s", html_s_tag);
  else      chprintf(chp, "%s", html_s_tag_s);
  chprintf(chp, "%c%s%s", name, html_m_tag, value);
  chprintf(chp, "%s%c%s", html_id_tag, name, html_e_tag);
}

void genfiles_ex_init(void) {
  /* nothing to do here yet */
}

uint8_t webSens = 0;
int fs_open_custom(struct fs_file *file, const char *name){
  for (uint8_t i = 0; i < HTML_PAGES; ++i) {
    if (!strcmp(name, webMenuLink[i])) {
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
      chprintf(chp, "<div class='tt'>OHS 2.0-%u.%u</div>\r\n", OHS_MAJOR, OHS_MINOR);
      chprintf(chp, "<ul class='nav'>\r\n");
      for (int j = 0; j < HTML_PAGES; ++j) {
        if (i == j) chprintf(chp, "<li><a class='active' href='%s'>%s</a></li>\r\n", webMenuLink[j], webMenuName[j]);
        else chprintf(chp, "<li><a href='%s'>%s</a></li>\r\n", webMenuLink[j], webMenuName[j]);
      }
      chprintf(chp, "</ul></div><div class='mb'><h1>%s</h1>\r\n", webMenuName[i]);

      // Custom start
      chprintf(chp, "<table><tr><th>#</th><th>Address</th><th>On</th><th>MQTT</th><th>Last Message</th><th>Queued</th>\r\n");
      chprintf(chp, "<th>Type</th><th>Function</th><th>Value</th><th>Group</th></tr>\r\n");

      for (uint8_t j = 0; j < NODE_SIZE; j++) {
        if (node[j].address != 0) {
          chprintf(chp, "%s%u.%s%s - ", html_tr_td, j + 1, html_e_td_td, node[j].name);
          printNodeAddress(chp, node[j].address, node[j].number);
          chprintf(chp, "%s", html_e_td_td);
          printOkNok(chp, GET_NODE_ENABLED(node[j].setting));
          chprintf(chp, "%s", html_e_td_td);
          printOkNok(chp, GET_NODE_MQTT_PUB(node[j].setting));
          chprintf(chp, "%s", html_e_td_td);
          printFrmTimestamp(chp, &node[j].last_OK);
          chprintf(chp, "%s", html_e_td_td);
          // queued
          chprintf(chp, "%s", html_e_td_td);
          printNodeType(chp, node[j].type);
          chprintf(chp, "%s", html_e_td_td);
          printNodeFunction(chp, node[j].function);
          chprintf(chp, "%s", html_e_td_td);
          printNodeValue(chp, j);
          chprintf(chp, "%s", html_e_td_td);
          printGroup(chp, GET_NODE_GROUP(node[j].setting));
          chprintf(chp, "%s", html_e_td_e_tr);
        }
      }
      chprintf(chp, "</table>");

      chprintf(chp, "<form action='%s' method='post'>\r\n", webMenuLink[i]);

      chprintf(chp, "<table>");
      // Form
      chprintf(chp, "%s%s%s", html_tr_td, text_Node, html_e_td_td);
      chprintf(chp, "%sP%s", html_select_submit, html_e_tag);
      for (uint8_t j = 0; j < NODE_SIZE; j++) {
        if (node[j].address != 0) {
          chprintf(chp, "%s%u", html_option, j);
          if (webSens == j) { chprintf(chp, "%s", html_selected); }
          else              { chprintf(chp, "%s", html_e_tag); }
          chprintf(chp, "%u - %s%s", j + 1, node[j].name, html_e_option);
        }
      }
      chprintf(chp, "%s%s", html_e_select, html_e_td_e_tr);
      chprintf(chp, "%s%s%s", html_tr_td, text_Address, html_e_td_td);
      printNodeAddress(chp, node[webSens].address, node[webSens].number);
      chprintf(chp, "%s", html_e_td_e_tr);
      chprintf(chp, "%s%s%s", html_tr_td, text_Name, html_e_td_td);
      printTextInput(chp, 'n', node[webSens].name, 1);
      chprintf(chp, "%s", html_e_td_e_tr);
      chprintf(chp, "%s%s %s%s", html_tr_td, text_Node, text_is, html_e_td_td);
      printOnOffButton(chp, "0", GET_NODE_ENABLED(node[webSens].setting), false);
      chprintf(chp, "%s", html_e_td_e_tr);

      chprintf(chp, "%s%s %s%s", html_tr_td, text_Group, text_is, html_e_td_td);
      selectGroup(chp, GET_NODE_GROUP(node[webSens].setting), 'g');
      chprintf(chp, "%s", html_e_td_e_tr);

      chprintf(chp, "</table></form>\r\n");
      /*
      <input type='text' name='n' value='Vstupni dvere' id='n'></td></tr>
      <tr><td>Address</td><td>Domek - Vstupni dvere - W:1:0</td></tr>
      <tr><td>Node is</td><td><div class='rm'>
      <div class='rc'><input type='radio' name='0' id='01' value='1' checked ><label for='01'>On</label></div>
      <div class='rc'><input type='radio' name='0' id='00' value='0'><label for='00'>Off</label></div>
      </div></td></tr>
      <tr><td>Type</td><td>Authentication:iButton</td></tr>
      <tr><td>MQTT publish</td><td>
      <div class='rm'><div class='rc'><input type='radio' name='7' id='71' value='1'><label for='71'>On</label></div>
      <div class='rc'><input type='radio' name='7' id='70' value='0' checked ><label for='70'>Off</label></div>
      </div></td></tr>
      <tr><td>Group</td><td>
      <select name='g'><option value='0' selected>1 - Domek - On</option>
      <option value='1'>2 - Garaz - On</option>
      </select></td></tr></table>

      <input type='submit' name='A' value='Apply'/>
      <input type='submit' name='e' value='Save all'/>
      <input type='submit' name='R' value='Reregister'/>
      */

      chprintf(chp, "</table><input type='submit' name='l' value='Load last'/><input type='submit' name='r' value='Reset to default'/>\r\n");

      // Custom end
      chprintf(chp, "</div></div></body></html>\r\n");


      if (file->pextension != NULL) {
        /* instead of doing memcpy, you would generate e.g. a JSON here */
        //memcpy(file->pextension, generated_html, sizeof(generated_html));
        file->data = (const char *)file->pextension;
        file->len = ms.eos;
        file->index = file->len;
        /* allow persisteng connections */
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
  return ERR_OK;
}

bool readPostParam(struct pbuf *p, uint16_t *pos, char *name, uint8_t nameLen, char *value, uint8_t valueLen){
  // assume name is at current place in stream
  uint8_t ch;

  // clear out name and value so they'll be NUL terminated
  memset(name, 0, nameLen);
  memset(value, 0, valueLen);

  // decrement length so we don't write into NUL terminator
  //--nameLen;
  //--valueLen;
  //chprintf((BaseSequentialStream*)&SD3, "pos: %u, len: %u\r\n", *pos, p->len);

  while (*pos < p->len){
    ch = pbuf_get_at(p, (*pos)++);
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
      uint8_t ch1 = pbuf_get_at(p, (*pos)++);
      uint8_t ch2 = pbuf_get_at(p, (*pos)++);
      if (ch1 == 0 || ch2 == 0)
        return false;
      char hex[3] = { ch1, ch2, 0 };
      ch = strtoul(hex, NULL, 16);
    }

    // check against 1 so we don't overwrite the final NUL
    if (nameLen > 1) {
      *name++ = ch;
      --nameLen;
    } else {
      if ((valueLen > 1) && (nameLen == 0))  {
        *value++ = ch;
        --valueLen;
      }
    }
    //chprintf((BaseSequentialStream*)&SD3, "pos: %c-%u, %u, %u\r\n", ch, *pos, nameLen, valueLen);
  }
  // if we get here, we hit the end-of-file, so POST is over and there are no more parameters
  return false;
}

err_t httpd_post_receive_data(void *connection, struct pbuf *p) {
  LWIP_UNUSED_ARG(connection);

  uint16_t pos = 0;
  uint8_t number;
  char name[17];
  char value[17];
  bool repeat;

  chprintf((BaseSequentialStream*)&SD3, "POST: %s\r\n", (char *)p->payload);
  chprintf((BaseSequentialStream*)&SD3, "current_uri: %s\r\n", current_uri);
  for (uint8_t i = 0; i < HTML_PAGES; ++i) {
    if (!strcmp(current_uri, webMenuLink[i])) {
      do{
        repeat = readPostParam(p, &pos, name, 3, value, 17);
        chprintf((BaseSequentialStream*)&SD3, "Parse: %s=%s<\r\n", name, value);
        switch(name[0]){
          case 'P':
            number = strtol(value, NULL, 10);
            if (number != webSens) { webSens = number; repeat = 0; }
          break;
          case 'n': // name
            strncpy (node[webSens].name, value, NAME_LENGTH);
          break;
          case '0' ... '7': // Handle all single radio buttons for settings
             if (value[0] == '0') node[webSens].setting &= ~(1 << (name[0]-48));
             else                 node[webSens].setting |=  (1 << (name[0]-48));
          break;
          case 'g': // group
            number = strtol(value, NULL, 10);
            SET_NODE_GROUP(node[webSens].setting, number);
            chprintf((BaseSequentialStream*)&SD3, "Group: %u=%u<\r\n", number,  GET_NODE_GROUP(node[webSens].setting));
          break;
          case 'e': break; // save
        }

      } while (repeat);
    }
  }

  return ERR_OK;
}

void httpd_post_finished(void *connection, char *response_uri, u16_t response_uri_len) {
  LWIP_UNUSED_ARG(connection);
  LWIP_UNUSED_ARG(response_uri_len);

  strcpy(response_uri, current_uri);
  response_uri_len = strlen(response_uri);
}

#endif /* OHS_HTTPDHANDLER_H_ */
