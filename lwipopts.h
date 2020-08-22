/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
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
 * Author: Simon Goldschmidt
 *
 */
#ifndef LWIP_HDR_LWIPOPTS_H__
#define LWIP_HDR_LWIPOPTS_H__

/* rand() */
#include <stdlib.h>
#include <hal.h>

/* Fixed settings mandated by the ChibiOS integration.*/
#include "static_lwipopts.h"

/* OHS overrides */
#define MEMP_NUM_SYS_TIMEOUT    (LWIP_NUM_SYS_TIMEOUT_INTERNAL + 4) // +1 MDSN,
#define MEMP_NUM_UDP_PCB        7 // , +1 MDSN
#define MEMP_NUM_TCP_PCB        6
#define MEMP_NUM_TCP_PCB_LISTEN 10
#define MEMP_NUM_PBUF           20
#define MEMP_NUM_RAW_PCB        6
#define MEMP_NUM_TCP_SEG        12
#define PBUF_POOL_SIZE          16

/* Optional, application-specific settings.*/
#if !defined(TCPIP_MBOX_SIZE)
#define TCPIP_MBOX_SIZE                 MEMP_NUM_PBUF
#endif
#if !defined(TCPIP_THREAD_STACKSIZE)
#define TCPIP_THREAD_STACKSIZE          2048
#endif

/* Use ChibiOS specific priorities. */
#if !defined(TCPIP_THREAD_PRIO)
#define TCPIP_THREAD_PRIO               (LOWPRIO + 11)
#endif
#if !defined(LWIP_THREAD_PRIORITY)
#define LWIP_THREAD_PRIORITY            (LOWPRIO + 10)
#endif

// HTTPD
#define LWIP_HTTPD_CUSTOM_FILES         1
#define LWIP_HTTPD_DYNAMIC_HEADERS      1
#define LWIP_HTTPD_SUPPORT_POST         1
#define MEM_SIZE                        16000
// DNS
#define LWIP_RAND() ((uint32_t)rand())
#define LWIP_DNS 1
// DHCP
#define LWIP_DHCP 1
// IGMP
#define LWIP_IGMP 0
// MDNS
#define LWIP_MDNS_RESPONDER 0
#define LWIP_NUM_NETIF_CLIENT_DATA (LWIP_MDNS_RESPONDER) // +1 MDSN
// Rename thread name
#define TCPIP_THREAD_NAME               "tcpip"

// LWIP DEBUG
#define LWIP_DEBUG   LWIP_DBG_ON
//#define HTTPD_DEBUG  LWIP_DBG_ON
//#define ETHARP_DEBUG LWIP_DBG_ON
//#define NETIF_DEBUG  LWIP_DBG_ON
//#define ICMP_DEBUG LWIP_DBG_ON
//#define IP_DEBUG LWIP_DBG_ON
//#define DNS_DEBUG LWIP_DBG_ON
// SNTP
//#define SNTP_DEBUG LWIP_DBG_ON
// SMTP
//#define SMTP_DEBUG LWIP_DBG_ON
// MDNS
//#define IGMP_DEBUG LWIP_DBG_ON
//#define MDNS_DEBUG LWIP_DBG_ON

//SNTP
#define SNTP_SERVER_DNS 1
#define SNTP_UPDATE_DELAY 3600000 // SNTP update every 1 hour
//#define SNTP_SERVER_ADDRESS "195.113.144.201"

// Maximum segment size
#define TCP_MSS 1024

// Number of rx pbufs to enqueue to parse an incoming request (up to the first newline)
#define LWIP_HTTPD_REQ_QUEUELEN 7

#define LWIP_NETIF_EXT_STATUS_CALLBACK 1

// Checks
#if !LWIP_NETIF_EXT_STATUS_CALLBACK
#error "This tests needs LWIP_NETIF_EXT_STATUS_CALLBACK enabled"
#endif

#define LWIP_NETIF_HOSTNAME 1

//ChibiOS RTC drivers
/* old
#define SNTP_SET_SYSTEM_TIME(sec) rtcSetTimeUnixSec(&RTCD1, (sec))
*/

/* Test function to verify SNTP_SET_SYSTEM_TIME macro
void SetTimeUnixA(time_t ut){
  RTCDateTime _ts;
  struct tm* _pt;

  _pt = gmtime(&ut);
  rtcConvertStructTmToDateTime( _pt, 0, &_ts);
  rtcSetTime(&RTCD1, &_ts);
}
*/

/*
#define SNTP_SET_SYSTEM_TIME(sec) \
  do{time_t rawtime = (sec);\
     RTCDateTime _ts;\
     convertUnixSecondToRTCDateTime(&_ts, rawtime);\
     rtcSetTime(&RTCD1, &_ts);}while(0)
*/

/* SET new driver
 * DONE - Get rid of struct tm in SNTP_SET_SYSTEM_TIME, by using my own convert functions.
 */
#define SNTP_SET_SYSTEM_TIME(sec) \
  do{time_t rawtime = (sec);\
     RTCDateTime _ts;\
     struct tm* _pt;\
     _pt = gmtime(&rawtime);\
     rtcConvertStructTmToDateTime(_pt, 0, &_ts);\
     rtcSetTime(&RTCD1, &_ts);}while(0)


/* GET old RTC driver
#define SNTP_GET_SYSTEM_TIME(sec, us) \
    do{uint64_t time = rtcGetTimeUnixUsec(&RTCD1);\
       (sec) = time / 1000000;\
       (us) = time % 1000000;}while(0)
*/

/* GET new driver
#define SNTP_GET_SYSTEM_TIME(sec, us) \
    do{struct tm timestamp;\
       rtcGetTime(&RTCD1, &timespec);\
       rtcConvertDateTimeToStructTm(&timespec, &timestamp, NULL);\
       uint64_t time = mktime(&timestamp);\
       (sec) = time / 1000000;\
       (us) = time % 1000000;}while(0)
*/

#endif /* LWIP_HDR_LWIPOPTS_H__ */
