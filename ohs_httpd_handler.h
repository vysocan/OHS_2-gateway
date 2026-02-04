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

#define HTTP_POST_DATA_SIZE 4097 // 4KB + 1B for null terminator
#define HTTP_ALERT_MSG_SIZE 80
#define HTTP_SET_COOKIE_SIZE 48

typedef enum {
  ALERT_INFO,
  ALERT_WARN,
  ALERT_ERROR
} AlertLevel;

typedef struct {
  char       msg[HTTP_ALERT_MSG_SIZE];
  AlertLevel type;
} HttpAlert;

static void *currentConn;
char current_uri[LWIP_HTTPD_MAX_REQUEST_URI_LEN] __attribute__((section(".ram4")));
char postData[HTTP_POST_DATA_SIZE] __attribute__((section(".ram4")));
HttpAlert httpAlert __attribute__((section(".ram4")));
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
    webGroup = 0, webTimer = 0, webScript = DUMMY_NO_VALUE, webTrigger = 0,
    webEnroll = 1;
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
/*
 * LWIP open custom file
 */
bool getPostData(char **pPostData, char *pName, uint8_t nameLen, char **pValue, uint16_t *pValueLen);

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
      chprintf(chp, "%sEnDis.js'>%s", HTML_script_src, HTML_e_script);
      chprintf(chp, "</head>\r\n<body onload=\"");
      // JavaScript enable/disable on body load
      switch (htmlPage) {
        case PAGE_USER:
          GET_CONF_CONTACT_IS_GLOBAL(conf.contact[webContact].setting) ? chprintf(chp, JS_en1) : chprintf(chp, JS_dis1);
          break;
        case PAGE_ZONE:
          GET_CONF_ZONE_TYPE(conf.zone[webZone]) ? chprintf(chp, JS_en1) : chprintf(chp, JS_dis1);
          break;
        case PAGE_TIMER:
          GET_CONF_TIMER_TYPE(conf.timer[webTimer].setting) ? chprintf(chp, JS_en1) : chprintf(chp, JS_dis1);
          break;
        case PAGE_TRIGGER:
          GET_CONF_TRIGGER_PASS(conf.trigger[webTrigger].setting) ? chprintf(chp, JS_en1) : chprintf(chp, JS_dis1);
          GET_CONF_TRIGGER_PASS_OFF(conf.trigger[webTrigger].setting) ? chprintf(chp, JS_en2) : chprintf(chp, JS_dis2);
          chprintf(chp, "sd(document.getElementById('y'));"); // Type select
          break;
        case PAGE_TCL:
        case PAGE_LOGIN:
        case PAGE_HOME:
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
      if (strlen(httpAlert.msg)) {
        const char *alertClass = "alrt";
        const char *alertTitle = "Error!";
        switch (httpAlert.type) {
            case ALERT_INFO: alertClass = "alrt info"; alertTitle = "Info"; break;
            case ALERT_WARN: alertClass = "alrt warn"; alertTitle = "Warning"; break;
            case ALERT_ERROR: default: break;
        }

        chprintf(chp, "<div class='%s' id='at'><span class='cbtn' onclick=\"this.parentElement.style.display='none';\">&times;</span>", alertClass);
        chprintf(chp, "<b>%s</b><br><br>", alertTitle);
        chprintf(chp, "%s.%s", httpAlert.msg, HTML_div_e);
        memset(&httpAlert, 0 , sizeof(HttpAlert)); // Empty alert message
      }
      // Header
      chprintf(chp, "<h1>%s</h1>\r\n", webPage[htmlPage].name);
      chprintf(chp, "%s%s%s%s", HTML_form_1, webPage[htmlPage].link, HTML_form_2, HTML_table);
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

  //DBG_HTTP("-PB-connection: %u\r\n", (uint32_t *)connection);
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
        } else {
           // Buffer overflow prevention
           pbuf_free(p);
           return ERR_MEM;
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

  char *postDataP = &postData[0];

  DBG_HTTP("-PE-connection: %u\r\n", (uint32_t *)connection);

  if (currentConn == connection) {
    for (uint8_t htmlPage = 0; htmlPage < ARRAY_SIZE(webPage); ++htmlPage) {
      if (!strcmp(current_uri, webPage[htmlPage].link)) {
        switch (htmlPage) {
          case PAGE_NODE:
            httpd_post_custom_node(&postDataP);
            break;
          case PAGE_USER:
            httpd_post_custom_user(&postDataP);
            break;
          case PAGE_KEY:
            httpd_post_custom_key(&postDataP);
            break;
          case PAGE_ZONE:
            httpd_post_custom_zone(&postDataP);
            break;
          case PAGE_ALERT:
            httpd_post_custom_alert(&postDataP);
            break;
          case PAGE_LOG:
            httpd_post_custom_log(&postDataP);
            break;
          case PAGE_GROUP:
            httpd_post_custom_group(&postDataP);
            break;
          case PAGE_SETTING:
            httpd_post_custom_setting(&postDataP);
            break;
          case PAGE_HOME:
            httpd_post_custom_home(&postDataP);
            break;
          case PAGE_TCL:
            httpd_post_custom_tcl(&postDataP);
            break;
          case PAGE_TIMER:
            httpd_post_custom_timer(&postDataP);
            break;
          case PAGE_TRIGGER:
            httpd_post_custom_trigger(&postDataP);
            break;
          case PAGE_LOGIN:
            httpd_post_custom_login(&postDataP, connection);
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
