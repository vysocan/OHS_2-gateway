/**
 * @file
 * HTTPD example for simple POST
 */
 
 /*
 * Copyright (c) 2017 Simon Goldschmidt
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 * 
 * Author: Simon Goldschmidt <goldsimon@gmx.de>
 *
 */

#include "lwip/opt.h"

#include "lwip/apps/httpd.h"
#include "lwip/def.h"
#include "lwip/mem.h"

#include <stdio.h>
#include <string.h>

/** define LWIP_HTTPD_EXAMPLE_GENERATEDFILES to 1 to enable this file system */
#ifndef LWIP_HTTPD_EXAMPLE_SIMPLEPOST
#define LWIP_HTTPD_EXAMPLE_SIMPLEPOST 1
#endif

#if LWIP_HTTPD_EXAMPLE_SIMPLEPOST

#if !LWIP_HTTPD_SUPPORT_POST
#error This needs LWIP_HTTPD_SUPPORT_POST
#endif

#define USER_PASS_BUFSIZE 16

static void *current_connection;
static void *valid_connection;
static char last_user[USER_PASS_BUFSIZE];
static char current_uri[LWIP_HTTPD_MAX_REQUEST_URI_LEN];

err_t httpd_post_begin(void *connection, const char *uri, const char *http_request,
                 u16_t http_request_len, int content_len, char *response_uri,
                 u16_t response_uri_len, u8_t *post_auto_wnd) {
  LWIP_UNUSED_ARG(connection);
  LWIP_UNUSED_ARG(http_request);
  LWIP_UNUSED_ARG(http_request_len);
  LWIP_UNUSED_ARG(content_len);
  LWIP_UNUSED_ARG(post_auto_wnd);

  chprintf((BaseSequentialStream*)&SD3, "connection: %X\r\n", (uint32_t *)connection);
  chprintf((BaseSequentialStream*)&SD3, "response_uri: %s\r\n", (char *)response_uri);
  chprintf((BaseSequentialStream*)&SD3, "uri: %s\r\n", (char *)uri);
  //chprintf((BaseSequentialStream*)&SD3, "http_request: %s\r\n", (char *)http_request);

  strcpy(current_uri, uri);

  strcpy(response_uri, uri);
  response_uri_len = strlen(response_uri);
  return ERR_OK;

  /*
  if (!memcmp(uri, "/login.cgi", 11)) {
    if (current_connection != connection) {
      current_connection = connection;
      valid_connection = NULL;
      // default page is "login failed"
      snprintf(response_uri, response_uri_len, "/loginfail.html");
      // e.g. for large uploads to slow flash over a fast connection, you should
      //   manually update the rx window. That way, a sender can only send a full
      //   tcp window at a time. If this is required, set 'post_aut_wnd' to 0.
      //  We do not need to throttle upload speed here, so:
      *post_auto_wnd = 1;
      return ERR_OK;
    }
  }
  return ERR_VAL;
  */
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
  uint16_t pos = 0;
  char name[17];
  char value[17];
  bool repeat;

  chprintf((BaseSequentialStream*)&SD3, "POST: %s\r\n", (char *)p->payload);
  chprintf((BaseSequentialStream*)&SD3, "current_uri: %s\r\n", current_uri);
  do{
    repeat = readPostParam(p, &pos, name, 3, value, 17);
    chprintf((BaseSequentialStream*)&SD3, "Parse: %s=%s<\r\n", name, value);
  } while (repeat);


  return ERR_OK;
  /*
  if (current_connection == connection) {
    u16_t token_user = pbuf_memfind(p, "user=", 5, 0);
    u16_t token_pass = pbuf_memfind(p, "pass=", 5, 0);
    if ((token_user != 0xFFFF) && (token_pass != 0xFFFF)) {
      u16_t value_user = token_user + 5;
      u16_t value_pass = token_pass + 5;
      u16_t len_user = 0;
      u16_t len_pass = 0;
      u16_t tmp;
      // find user len
      tmp = pbuf_memfind(p, "&", 1, value_user);
      if (tmp != 0xFFFF) {
        len_user = tmp - value_user;
      } else {
        len_user = p->tot_len - value_user;
      }
      // find pass len
      tmp = pbuf_memfind(p, "&", 1, value_pass);
      if (tmp != 0xFFFF) {
        len_pass = tmp - value_pass;
      } else {
        len_pass = p->tot_len - value_pass;
      }
      if ((len_user > 0) && (len_user < USER_PASS_BUFSIZE) &&
          (len_pass > 0) && (len_pass < USER_PASS_BUFSIZE)) {
        // provide contiguous storage if p is a chained pbuf
        char buf_user[USER_PASS_BUFSIZE];
        char buf_pass[USER_PASS_BUFSIZE];
        char *user = "";//(char *)pbuf_get_contiguous(p, buf_user, sizeof(buf_user), len_user, value_user);
        char *pass = "";//(char *)pbuf_get_contiguous(p, buf_pass, sizeof(buf_pass), len_pass, value_pass);
        if (user && pass) {
          user[len_user] = 0;
          pass[len_pass] = 0;
          if (!strcmp(user, "lwip") && !strcmp(pass, "post")) {
            // user and password are correct, create a "session"
            valid_connection = connection;
            memcpy(last_user, user, sizeof(last_user));
          }
        }
      }
    }
    // not returning ERR_OK aborts the connection, so return ERR_OK unless the
    //   conenction is unknown
    return ERR_OK;
  }
  return ERR_VAL;
  */
}

void httpd_post_finished(void *connection, char *response_uri, u16_t response_uri_len) {

  strcpy(response_uri, current_uri);
  response_uri_len = strlen(response_uri);

  /*
  // default page is "login failed"
  snprintf(response_uri, response_uri_len, "/loginfail.html");
  if (current_connection == connection) {
    if (valid_connection == connection) {
      // login succeeded
      snprintf(response_uri, response_uri_len, "/session.html");
    }
    current_connection = NULL;
    valid_connection = NULL;
  }
  */
}

#endif /* LWIP_HTTPD_EXAMPLE_SIMPLEPOST*/
