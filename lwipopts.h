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
 * No error, everything OK. ERR_OK         = 0,
 * Out of memory error.     ERR_MEM        = -1,
 * Buffer error.            ERR_BUF        = -2,
 * Timeout.                 ERR_TIMEOUT    = -3,
 * Routing problem.         ERR_RTE        = -4,
 * Operation in progress    ERR_INPROGRESS = -5,
 * Illegal value.           ERR_VAL        = -6,
 * Operation would block.   ERR_WOULDBLOCK = -7,
 * Address in use.          ERR_USE        = -8,
 * Already connecting.      ERR_ALREADY    = -9,
 * Conn already established.ERR_ISCONN     = -10,
 * Not connected.           ERR_CONN       = -11,
 * Low-level netif error    ERR_IF         = -12,
 * Connection aborted.      ERR_ABRT       = -13,
 * Connection reset.        ERR_RST        = -14,
 * Connection closed.       ERR_CLSD       = -15,
 * Illegal argument.        ERR_ARG        = -16
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

#define LWIP_PRINT(...)  {chprintf((BaseSequentialStream*) &SD3, __VA_ARGS__);}// chprintf((BaseSequentialStream*) &SD3, "\r");}

// TODO OHS #define LWIP_RAM_HEAP_POINTER (void *) SOME_ADDRESS Move LWIP to CCM?

// Use UMM as heap for lwip
#define MEM_LIBC_MALLOC 0
#if MEM_LIBC_MALLOC
#include "umm_malloc.h"
#include "umm_malloc_cfg.h"
#define mem_clib_free(p)     umm_free(p)
#define mem_clib_malloc(s)   umm_malloc(s)
#define mem_clib_calloc(c,s) umm_calloc(c,s)
#endif

/* OHS overrides */
#define MEM_SIZE                 1024 * 32
#define MEMP_NUM_SYS_TIMEOUT     (LWIP_NUM_SYS_TIMEOUT_INTERNAL + 6) // +1 MDSN,
#define MEMP_NUM_UDP_PCB         5 // , +1 MDSN
#define MEMP_NUM_TCP_PCB         30
#define MEMP_NUM_TCP_PCB_LISTEN  5
#define MEMP_NUM_PBUF            10
#define MEMP_NUM_RAW_PCB         6
#define MEMP_NUM_TCP_SEG         15
//#define MEMP_NUM_TCPIP_MSG_INPKT 20 // +2 MQTT
//#define TCP_SND_QUEUELEN        ((6 * (TCP_SND_BUF) + (TCP_MSS - 1))/(TCP_MSS))
//#define MEMP_NUM_TCP_SEG        TCP_SND_QUEUELEN
#define PBUF_POOL_SIZE           10


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
#define LINK_STATS              0
#define IP_STATS                0
#define ICMP_STATS              0
#define IGMP_STATS              0
#define IPFRAG_STATS            0
#define UDP_STATS               0
#define TCP_STATS               0
#define MEM_STATS               1
#define MEMP_STATS              1
#define PBUF_STATS              1
#define SYS_STATS               1
#endif /* LWIP_STATS */

// HTTPD
#define LWIP_HTTPD_CUSTOM_FILES         1
#define LWIP_HTTPD_FILE_EXTENSION       1
#define LWIP_HTTPD_DYNAMIC_HEADERS      1
#define LWIP_HTTPD_SUPPORT_POST         1
#define LWIP_HTTPD_SUPPORT_COOKIES      1
#define LWIP_HTTPD_SUPPORT_FS_OPEN_AUTH 1
#define HTTPD_ADDITIONAL_CONTENT_TYPES  {"wolf2", HTTP_CONTENT_TYPE("font/woff2")} // Adding our icon-font type to HTTPD
#define HTTPD_SERVER_AGENT "lwIP/" LWIP_VERSION_STRING // Make the HTTPD name a bit smaller
// DNS
#define LWIP_RAND() ((uint32_t)rand())
#define LWIP_DNS 1
#define DNS_TABLE_SIZE                  4   /** DNS maximum number of entries to maintain locally. */
#define DNS_MAX_NAME_LENGTH             256 /** DNS maximum host name length supported in the name table. */
#define DNS_SERVER_ADDRESS(ipaddr)      (((ipaddr)->addr) = 134744072) // Google's 8.8.8.8
#define DNS_MAX_SERVERS                 2   /** The maximum of DNS servers */
#define DNS_DOES_NAME_CHECK             1   /** DNS do a name checking between the query and the response. */
// DHCP
#define LWIP_DHCP 1
// MDNS
// IGMP multicast in hal_mac_lld.c
#define LWIP_MDNS_RESPONDER 1
#define LWIP_NUM_NETIF_CLIENT_DATA (LWIP_MDNS_RESPONDER) // +1 MDSN
#if LWIP_MDNS_RESPONDER
// IGMP
#define LWIP_IGMP 1
#else
// IGMP
#define LWIP_IGMP 0
#endif
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
//#define LWIP_DEBUG LWIP_DBG_ON
//#define MEMP_DEBUG LWIP_DBG_ON
//#define TIMERS_DEBUG LWIP_DBG_ON
//#define MEM_DEBUG LWIP_DBG_ON
//#define SYS_DEBUG LWIP_DBG_ON
//#define ETHARP_DEBUG LWIP_DBG_ON
//#define NETIF_DEBUG  LWIP_DBG_ON
//#define ICMP_DEBUG LWIP_DBG_ON
//#define IP_DEBUG LWIP_DBG_ON
//#define DNS_DEBUG LWIP_DBG_ON
//#define SOCKETS_DEBUG LWIP_DBG_ON
// HTTPD
//#define HTTPD_DEBUG  LWIP_DBG_ON
// SNTP
//#define SNTP_DEBUG LWIP_DBG_ON
// SMTP
//#define SMTP_DEBUG LWIP_DBG_ON
// MDNS
//#define IGMP_DEBUG LWIP_DBG_ON
//#define MDNS_DEBUG LWIP_DBG_ON
// MQTT
//#define MQTT_DEBUG LWIP_DBG_ON
// HTTP Client
//#define HTTPC_DEBUG LWIP_DBG_ON

// SNTP
#define SNTP_SERVER_DNS         1
#define SNTP_STARTUP_DELAY      1       // enable
#define SNTP_STARTUP_DELAY_FUNC (20000) // SNTP first query after # milliseconds
#define SNTP_UPDATE_DELAY       3600000 // SNTP update every # milliseconds

// Maximum segment size
#define TCP_MSS 1460

// Number of rx pbufs to enqueue to parse an incoming request (up to the first newline)
#define LWIP_HTTPD_REQ_QUEUELEN 7

#define LWIP_NETIF_EXT_STATUS_CALLBACK 1

// Checks
#if !LWIP_NETIF_EXT_STATUS_CALLBACK
#error "We need LWIP_NETIF_EXT_STATUS_CALLBACK enabled"
#endif

#define LWIP_NETIF_HOSTNAME 1

// MQTT extra settings
#define MQTT_OUTPUT_RINGBUF_SIZE 512

// MBED TLS
/*
#define LWIP_ALTCP              1
#define LWIP_ALTCP_TLS          1
#define LWIP_ALTCP_TLS_MBEDTLS  1
#define MBEDTLS_PLATFORM_MEMORY 1
#define ALTCP_MBEDTLS_RNG_FN    mbedtls_entropy_func
//#define MEMP_MEM_MALLOC         1
*/

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

/*
 * LWIP core locking
 */
#if LWIP_TCPIP_CORE_LOCKING
void lwip_assert_core_locked(void);
#define LWIP_ASSERT_CORE_LOCKED() lwip_assert_core_locked();
#endif

#endif /* LWIP_HDR_LWIPOPTS_H__ */
