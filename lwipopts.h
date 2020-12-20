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
 * err_enum_t:
 * ERR_OK           No error, everything OK.
 * ERR_MEM           Out of memory error.
 * ERR_BUF           Buffer error.
 * ERR_TIMEOUT       Timeout.
 * ERR_RTE           Routing problem.
 * ERR_INPROGRESS    Operation in progress
 * ERR_VAL           Illegal value.
 * ERR_WOULDBLOCK    Operation would block.
 * ERR_USE           Address in use.
 * ERR_ALREADY       Already connecting.
 * ERR_ISCONN        Conn already established.
 * ERR_CONN          Not connected.
 * ERR_IF            Low-level netif error
 * ERR_ABRT          Connection aborted.
 * ERR_RST           Connection reset.
 * ERR_CLSD          Connection closed.
 * ERR_ARG           Illegal argument.
 *
 */
#ifndef LWIP_HDR_LWIPOPTS_H__
#define LWIP_HDR_LWIPOPTS_H__

/* rand() */
#include <stdlib.h>
#include <hal.h>

/* Fixed settings mandated by the ChibiOS integration.*/
#include "static_lwipopts.h"
#include "date_time.h"

#define LWIP_PRINT(...)  {chprintf((BaseSequentialStream*) &SD3, __VA_ARGS__); chprintf((BaseSequentialStream*) &SD3, "\r\n");}

// TODO OHS #define LWIP_RAM_HEAP_POINTER (void *) SOME_ADDRESS Move LWIP to CCM?

/* OHS overrides */
#define MEMP_NUM_SYS_TIMEOUT    (LWIP_NUM_SYS_TIMEOUT_INTERNAL + 6) // +1 MDSN,
#define MEMP_NUM_UDP_PCB        7 // , +1 MDSN
#define MEMP_NUM_TCP_PCB        7
#define MEMP_NUM_TCP_PCB_LISTEN 10
#define MEMP_NUM_PBUF           20
#define MEMP_NUM_RAW_PCB        6
#define MEMP_NUM_TCP_SEG        15
//#define TCP_SND_QUEUELEN        ((6 * (TCP_SND_BUF) + (TCP_MSS - 1))/(TCP_MSS))
//#define MEMP_NUM_TCP_SEG        TCP_SND_QUEUELEN
#define PBUF_POOL_SIZE          16

/* Optional, application-specific settings.*/
#if !defined(TCPIP_MBOX_SIZE)
#define TCPIP_MBOX_SIZE                 MEMP_NUM_PBUF
#endif
#if !defined(TCPIP_THREAD_STACKSIZE)
#define TCPIP_THREAD_STACKSIZE          1024 * 2
#endif

/* Use ChibiOS specific priorities. */
#if !defined(TCPIP_THREAD_PRIO)
#define TCPIP_THREAD_PRIO               (LOWPRIO + 11)
#endif
#if !defined(LWIP_THREAD_PRIORITY)
#define LWIP_THREAD_PRIORITY            (LOWPRIO + 10)
#endif

// Statistics options
#define LWIP_STATS              0
#if LWIP_STATS
#define LWIP_STATS_DISPLAY      1
#define LINK_STATS              1
#define IP_STATS                1
#define ICMP_STATS              1
#define IGMP_STATS              1
#define IPFRAG_STATS            1
#define UDP_STATS               1
#define TCP_STATS               1
#define MEM_STATS               1
#define MEMP_STATS              1
#define PBUF_STATS              1
#define SYS_STATS               1
#endif /* LWIP_STATS */

// HTTPD
#define LWIP_HTTPD_CUSTOM_FILES         1
#define LWIP_HTTPD_DYNAMIC_HEADERS      1
#define LWIP_HTTPD_SUPPORT_POST         1
#define MEM_SIZE                        20000
// DNS
#define LWIP_RAND() ((uint32_t)rand())
#define LWIP_DNS 1
#define DNS_TABLE_SIZE                  4   /** DNS maximum number of entries to maintain locally. */
#define DNS_MAX_NAME_LENGTH             256 /** DNS maximum host name length supported in the name table. */
#define DNS_SERVER_ADDRESS(ipaddr)      (((ipaddr)->addr) = 134744072)
#define DNS_MAX_SERVERS                 2   /** The maximum of DNS servers */
#define DNS_DOES_NAME_CHECK             1   /** DNS do a name checking between the query and the response. */
// DHCP
#define LWIP_DHCP 1
// IGMP
#define LWIP_IGMP 0
// MDNS
// IGMP multicast in hal_mac_lld.c
#define LWIP_MDNS_RESPONDER 0
#define LWIP_NUM_NETIF_CLIENT_DATA (LWIP_MDNS_RESPONDER) // +1 MDSN
// Rename thread name
#define TCPIP_THREAD_NAME               "tcpip"

/**
 * DEFAULT_TCP_RECVMBOX_SIZE: The mailbox size for the incoming packets on a
 * NETCONN_TCP. The queue size value itself is platform-dependent, but is passed
 * to sys_mbox_new() when the recvmbox is created.
 */
#ifndef DEFAULT_TCP_RECVMBOX_SIZE
#define DEFAULT_TCP_RECVMBOX_SIZE       40
#endif

/**
 * DEFAULT_ACCEPTMBOX_SIZE: The mailbox size for the incoming connections.
 * The queue size value itself is platform-dependent, but is passed to
 * sys_mbox_new() when the acceptmbox is created.
 */
#ifndef DEFAULT_ACCEPTMBOX_SIZE
#define DEFAULT_ACCEPTMBOX_SIZE         4
#endif

// LWIP DEBUG
//#define LWIP_DEBUG   LWIP_DBG_ON
//#define HTTPD_DEBUG  LWIP_DBG_ON
//#define ETHARP_DEBUG LWIP_DBG_ON
//#define NETIF_DEBUG  LWIP_DBG_ON
//#define ICMP_DEBUG LWIP_DBG_ON
//#define IP_DEBUG LWIP_DBG_ON
//#define DNS_DEBUG LWIP_DBG_ON
//#define SOCKETS_DEBUG LWIP_DBG_ON
// SNTP
//#define SNTP_DEBUG LWIP_DBG_ON
// SMTP
//#define SMTP_DEBUG LWIP_DBG_ON
// MDNS
//#define IGMP_DEBUG LWIP_DBG_ON
//#define MDNS_DEBUG LWIP_DBG_ON
// MQTT
//#define MQTT_DEBUG LWIP_DBG_ON

// SNTP
#define SNTP_SERVER_DNS         1
#define SNTP_STARTUP_DELAY      1       // enable
#define SNTP_STARTUP_DELAY_FUNC (20000) // SNTP first query after 20 seconds
#define SNTP_UPDATE_DELAY       3600000 // SNTP update every 30 minutes

// Maximum segment size
#define TCP_MSS 1024

// Number of rx pbufs to enqueue to parse an incoming request (up to the first newline)
#define LWIP_HTTPD_REQ_QUEUELEN 7

#define LWIP_NETIF_EXT_STATUS_CALLBACK 1

// Checks
#if !LWIP_NETIF_EXT_STATUS_CALLBACK
#error "We need LWIP_NETIF_EXT_STATUS_CALLBACK enabled"
#endif

#define LWIP_NETIF_HOSTNAME 1

/*
 * new SNTP_SET_SYSTEM_TIME function
 *
 *      chprintf((BaseSequentialStream *)&SD3, "SNTP: %d\n\r", timestamp);\
 */
#define SNTP_SET_SYSTEM_TIME(sec) \
  do{time_t timestamp = (sec);\
     RTCDateTime _ts;\
     convertUnixSecondToRTCDateTime(&_ts, timestamp);\
     rtcSetTime(&RTCD1, &_ts);\
   }while(0)

#endif /* LWIP_HDR_LWIPOPTS_H__ */
