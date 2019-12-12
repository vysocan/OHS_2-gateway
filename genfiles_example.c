/**
 * @file
 * HTTPD custom file system example for runtime generated files
 *
 * This file demonstrates how to add support for generated files to httpd.
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
#include "genfiles_example.h"

#include "lwip/apps/fs.h"
#include "lwip/def.h"
#include "lwip/mem.h"

#include <stdio.h>
#include <string.h>

#include "memstreams.h"

/** define LWIP_HTTPD_EXAMPLE_GENERATEDFILES to 1 to enable this file system */
#ifndef LWIP_HTTPD_EXAMPLE_GENERATEDFILES
#define LWIP_HTTPD_EXAMPLE_GENERATEDFILES 1
#endif

#if LWIP_HTTPD_EXAMPLE_GENERATEDFILES

#if !LWIP_HTTPD_CUSTOM_FILES
#error This needs LWIP_HTTPD_CUSTOM_FILES
#endif
#if !LWIP_HTTPD_DYNAMIC_HEADERS
#error This needs LWIP_HTTPD_DYNAMIC_HEADERS
#endif

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


void
genfiles_ex_init(void)
{
  /* nothing to do here yet */
}

int fs_open_custom(struct fs_file *file, const char *name){
  for (int i = 0; i < HTML_PAGES; ++i) {
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
      chprintf(chp, "<link rel='stylesheet' href='/css/OHS.css'>\r\n");
      chprintf(chp, "</head><body onload=\"\"><div class='wrp'><div class='sb'>\r\n");
      chprintf(chp, "<div class='tt'>OHS 2.0.%u.%u</div>\r\n", 0, 1);
      chprintf(chp, "<ul class='nav'>\r\n");
      for (int j = 0; j < HTML_PAGES; ++j) {
        if (i == j) chprintf(chp, "<li><a class='active' href='%s'>%s</a></li>\r\n", webMenuLink[j], webMenuName[j]);
        else chprintf(chp, "<li><a href='%s'>%s</a></li>\r\n", webMenuLink[j], webMenuName[j]);
      }
      chprintf(chp, "</ul></div>\r\n");
      chprintf(chp, "<div class='mb'><h1>%s</h1>\r\n", webMenuName[i]);

      chprintf(chp, "<table><tr><th>#</th><th>Address</th><th>On</th><th>MQTT</th><th>Last Message</th><th>Queued</th>\r\n");
      chprintf(chp, "<th>Function</th><th>Type</th><th>Value</th><th>Group</th></tr>\r\n");

      //<tr><td>1.</td><td>Domek - Vstupni dvere - W:1:0</td><td><i class='fa fa-check'></i></td><td><i class='fa fa-ban'></i></td>
      //<td>2d, 01:36:52</td><td>Authentication</td><td>iButton</td><td></td><td>1 - Domek</td><td><i class='fa fa-ban'></i></td></tr>

      for (uint8_t i = 0; i < 10; i++) {
        chprintf(chp, "<tr><td>%u.</td><td>%s\r\n", i);

        /*
        server.printP(html_tr_td);
        server << i+1; server.print('.'); server.printP(html_e_td_td);
        printNodeAddress(server, i);
        server.printP(html_e_td_td);
        (node[i].setting & B1) ? server.printP(text_i_OK) : server.printP(text_i_disabled); server.printP(html_e_td_td);
        ((node[i].setting >> 7) & B1) ? server.printP(text_i_OK) : server.printP(text_i_disabled); server.printP(html_e_td_td);
        time_temp.set(timestamp.get() - node[i].last_OK);
        server.print((char*)time_temp.formatedUpTime()); server.printP(html_e_td_td);
        printNodeFunction(server, node[i].function);
        server.printP(html_e_td_td);
        printNodeType(server, node[i].type);
        if ((node[i].type == 'B') && ((node[i].setting >> 5) & B1)) { server.printP(text_spdashsp); server.printP(text_low); }
        server.printP(html_e_td_td);
        //  Do not show value for authentication units
        if (node[i].function != 'K') {
          dtostrf(node[i].value, 6, 2, value); server << value;
          switch(node[i].type){
            case 'T': server.printP(text_degC); break;
            case 'H':
            case 'X': server.printP(text_percent); break;
            case 'P': server.printP(text_mBar); break;
            case 'V':
            case 'B': server.printP(text_Volt); break;
            case 'G': server.printP(text_ppm); break;
            default: break;
          }
        }
        server.printP(html_e_td_td);
        server << ((node[i].setting >> 1) & B1111) + 1; server.printP(text_spdashsp); server << conf.group_name[((node[i].setting >> 1) & B1111)];
        server.printP(html_e_td_td);
        (node[i].queue != 255) ? server.printP(text_i_OK) : server.printP(text_i_disabled);
        server.printP(html_e_td_e_tr);
        */
      }




      chprintf(chp, "\r\n");

      chprintf(chp, "</table><input type='submit' name='l' value='Load last'/><input type='submit' name='r' value='Reset to default'/>\r\n");
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

#if LWIP_HTTPD_FS_ASYNC_READ
u8_t
fs_canread_custom(struct fs_file *file)
{
  LWIP_UNUSED_ARG(file);
  /* This example does not use delayed/async reading */
  return 1;
}

u8_t
fs_wait_read_custom(struct fs_file *file, fs_wait_cb callback_fn, void *callback_arg)
{
  LWIP_ASSERT("not implemented in this example configuration", 0);
  LWIP_UNUSED_ARG(file);
  LWIP_UNUSED_ARG(callback_fn);
  LWIP_UNUSED_ARG(callback_arg);
  /* Return
     - 1 if ready to read (at least one byte)
     - 0 if reading should be delayed (call 'tcpip_callback(callback_fn, callback_arg)' when ready) */
  return 1;
}

int
fs_read_async_custom(struct fs_file *file, char *buffer, int count, fs_wait_cb callback_fn, void *callback_arg)
{
  LWIP_ASSERT("not implemented in this example configuration", 0);
  LWIP_UNUSED_ARG(file);
  LWIP_UNUSED_ARG(buffer);
  LWIP_UNUSED_ARG(count);
  LWIP_UNUSED_ARG(callback_fn);
  LWIP_UNUSED_ARG(callback_arg);
  /* Return
     - FS_READ_EOF if all bytes have been read
     - FS_READ_DELAYED if reading is delayed (call 'tcpip_callback(callback_fn, callback_arg)' when done) */
  /* all bytes read already */
  return FS_READ_EOF;
}

#else /* LWIP_HTTPD_FS_ASYNC_READ */
int
fs_read_custom(struct fs_file *file, char *buffer, int count)
{
  LWIP_ASSERT("not implemented in this example configuration", 0);
  LWIP_UNUSED_ARG(file);
  LWIP_UNUSED_ARG(buffer);
  LWIP_UNUSED_ARG(count);
  /* Return
     - FS_READ_EOF if all bytes have been read
     - FS_READ_DELAYED if reading is delayed (call 'tcpip_callback(callback_fn, callback_arg)' when done) */
  /* all bytes read already */
  return FS_READ_EOF;
}

#endif /* LWIP_HTTPD_FS_ASYNC_READ */

#endif /* LWIP_HTTPD_EXAMPLE_GENERATEDFILES */
