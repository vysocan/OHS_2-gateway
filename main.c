/*
 * OHS gateway code for HW v 2.0.x
 * Adam Baron (c) 2020-2024
 *
 *
 */
//TODO OHS move arm/disarm group to some kind of fifo, it take a lot of resources calling nodes, mqtt ...
// Optimize stack and overflow
#define PORT_INT_REQUIRED_STACK 128
// Remove input queue for GPRS to save RAM
#define STM32_SERIAL_USART6_IN_BUF_SIZE 0

// Standard libs
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
// ChibiOS
#include "ch.h"
#include "hal.h"
// Added from ChibiOS
#include "shell.h"
#include "chprintf.h"
#include "usbcfg.h"

// Define debug console
BaseSequentialStream* console = (BaseSequentialStream*)&SD3;

// Semaphores
binary_semaphore_t gprsSem;
binary_semaphore_t emailSem;
// TCL callback semaphores
binary_semaphore_t cbTimerSem;
binary_semaphore_t cbTriggerSem;
// MQTT semaphore
binary_semaphore_t mqttSem;

// RFM69
#include "rfm69.h"
// UMM
#include "umm_malloc.h"
#include "umm_malloc_cfg.h"
#define UMM_MALLOC_CFG_HEAP_SIZE (1024*48)
char ohsUmmHeap[UMM_MALLOC_CFG_HEAP_SIZE] __attribute__((section(".ram4")));
// TCL
#define TCL_SCRIPT_LENGTH        (512)
#define TCL_OUTPUT_LENGTH        (1024*2)
char tclOutput[TCL_OUTPUT_LENGTH] __attribute__((section(".ram4")));
char tclCmd[TCL_SCRIPT_LENGTH] __attribute__((section(".ram4")));
#include "tcl.h"
struct tcl tcl;
// uBS
#include "uBS.h"

#define LOG_TEXT_LENGTH 80
char logText[LOG_TEXT_LENGTH] __attribute__((section(".ram4"))); // To decode log text

// MQTT HAD
#define MQTT_PAYLOAD_LENGTH 512 // same as MQTT_OUTPUT_RINGBUF_SIZE in lwipopts.h
char mqttPayload[MQTT_PAYLOAD_LENGTH] __attribute__((section(".ram4"))); // To decode log text
char mqttHadUid[12];

// OHS includes
#include "ohs_text_const.h"
#include "ohs_conf.h"
#include "date_time.h"
#include "ohs_peripheral.h"
#include "ohs_functions.h"

// GPRS
#include "gprs.h"
typedef enum {
  gprs_NotInitialized,
  gprs_OK,
  gprs_ForceReset,
  gprs_Failed
} gprsStatus_t;
volatile gprsStatus_t gsmStatus = gprs_NotInitialized;
volatile int8_t gprsIsAlive = 0;
volatile int8_t gprsSetSMS = 0;
volatile int8_t gprsReg = 2;
volatile int8_t gprsStrength = 0;
char gprsModemInfo[20] __attribute__((section(".ram4"))); // SIMCOM_SIM7600x-x
char gprsSystemInfo[80] __attribute__((section(".ram4")));
char gprsSmsText[128] __attribute__((section(".ram4")));

// LWIP
#include "lwipthread.h"
#include "lwip/opt.h"
#include "lwip/arch.h"
#include "lwip/api.h"
#include "lwip/stats.h"
#include "lwip/tcpip.h"
#include "lwip/apps/httpd.h"
#include "lwip/apps/sntp.h"
#include "lwip/apps/smtp.h"
#if LWIP_MDNS_RESPONDER
#define MDNS_HOSTANME "ohs"
#include "lwip/apps/mdns.h"
#endif
// HTTPD OHS handler
#include "ohs_httpdhandler.h"
// MQTT
#include "lwip/apps/mqtt_priv.h" // Needed for conf.mqtt
#include "lwip/apps/mqtt.h"
#include "ohs_mqtt_functions.h"
// mDNS
#include "ohs_mdns_functions.h"

// Shell functions
#include "ohs_shell.h"

// Thread handling
#include "ohs_th_zone.h"
#include "ohs_th_alarm.h"
#include "ohs_th_logger.h"
#include "ohs_th_rs485.h"
#include "ohs_th_registration.h"
#include "ohs_th_sensor.h"
#include "ohs_th_modem.h"
#include "ohs_th_alert.h"
#include "ohs_th_service.h"
#include "ohs_th_radio.h"
#include "ohs_th_trigger.h"
#include "ohs_th_tcl.h"
#include "ohs_th_mqtt.h"
#include "ohs_th_heartbeat.h"
#define SHELL_WA_SIZE THD_WORKING_AREA_SIZE(2*1024)
/*
 * Application entry point.
 */
int main(void) {
  halInit();
  chSysInit();

  // Semaphores
  chBSemObjectInit(&gprsSem, false);
  chBSemObjectInit(&emailSem, false);
  chBSemObjectInit(&cbTimerSem, false);
  chBSemObjectInit(&cbTriggerSem, false);
  chBSemObjectInit(&mqttSem, false);
  // Debug port
  sdStart(&SD3,  &serialCfg);
  chprintf(console, "\r\nOHS v.%u.%u.%u start\r\n", OHS_MAJOR, OHS_MINOR, OHS_MOD);
  // GPRS modem
  gprsInit(&SD6);
  // Init nodes
  initRuntimeNodes();
  // RS485
  rs485Start(&RS485D2, &rs485cfg);
  // chprintf(console, "RS485 timeout: %d(uS)/%d(tick)\r\n", RS485D2.oneByteTimeUS, RS485D2.oneByteTimeI);

  // Ethernet
  macAddr[0] = LWIP_ETHADDR_0; macAddr[1] = LWIP_ETHADDR_1; macAddr[2] = LWIP_ETHADDR_2;
  macAddr[3] = STM32_UUID[0] & 0xff;
  macAddr[4] = (STM32_UUID[0] >> 8) & 0xff;
  macAddr[5] = (STM32_UUID[0] >> 16) & 0xff;
  struct lwipthread_opts lwip_opts =
  { &macAddr[0], 0, 0, 0, NET_ADDRESS_DHCP
    #if LWIP_NETIF_HOSTNAME
      , OHS_NAME
    #endif
    ,NULL, NULL
  };
  // lwip init
  lwipInit(&lwip_opts);

  // Initializes a serial-over-USB CDC driver.
  sduObjectInit(&SDU1);
  sduStart(&SDU1, &serusbcfg);
  /*
   * Activates the USB driver and then the USB bus pull-up on D+.
   * Note, a delay is inserted in order to not have to disconnect the cable
   * after a reset.
   */
  usbDisconnectBus(serusbcfg.usbp);
  chThdSleepMilliseconds(1500);
  usbStart(serusbcfg.usbp, &usbcfg);
  usbConnectBus(serusbcfg.usbp);

  // Initialize .ram4
  memset(&tclCmd[0], 0, TCL_SCRIPT_LENGTH);
  memset(&tclOutput[0], 0, TCL_OUTPUT_LENGTH);
  memset(&gprsModemInfo[0], 0, sizeof(gprsModemInfo));
  memset(&gprsSmsText[0], 0, sizeof(gprsSmsText));
  memset(&gprsSystemInfo[0], 0, sizeof(gprsSystemInfo));
  memset(&logText[0], 0, LOG_TEXT_LENGTH);
  memset(&mqttPayload[0], 0, MQTT_PAYLOAD_LENGTH);
  memset(&httpAlertMsg[0], 0 , HTTP_ALERT_MSG_SIZE);

  shellInit();

  // Pools
  chPoolObjectInit(&alarmEvent_pool, sizeof(alarmEvent_t), NULL);
  chPoolObjectInit(&logger_pool, sizeof(loggerEvent_t), NULL);
  chPoolObjectInit(&registration_pool, sizeof(registrationEvent_t), NULL);
  chPoolObjectInit(&sensor_pool, sizeof(sensorEvent_t), NULL);
  chPoolObjectInit(&alert_pool, sizeof(alertEvent_t), NULL);
  chPoolObjectInit(&script_pool, sizeof(scriptEvent_t), NULL);
  chPoolObjectInit(&trigger_pool, sizeof(triggerEvent_t), NULL);
  chPoolObjectInit(&mqtt_pool, sizeof(mqttEvent_t), NULL);
  // Prepare pools
  chPoolLoadArray(&alarmEvent_pool, (void *)alarmEvent_pool_queue, ALARM_EVENT_FIFO_SIZE);
  chPoolLoadArray(&logger_pool, (void *)logger_pool_queue, LOGGER_FIFO_SIZE);
  chPoolLoadArray(&registration_pool, (void *)registration_pool_queue, REG_FIFO_SIZE);
  chPoolLoadArray(&sensor_pool, (void *)sensor_pool_queue, SENSOR_FIFO_SIZE);
  chPoolLoadArray(&alert_pool, (void *)alert_pool_queue, ALERT_FIFO_SIZE);
  chPoolLoadArray(&script_pool, (void *)script_pool_queue, SCRIPT_FIFO_SIZE);
  chPoolLoadArray(&trigger_pool, (void *)trigger_pool_queue, TRIGGER_FIFO_SIZE);
  chPoolLoadArray(&mqtt_pool, (void *)mqtt_pool_queue, MQTT_FIFO_SIZE);

  // SPI
  spiStart(&SPID1, &spi1cfg);
  // RFM69
  if (GET_CONF_SYSTEM_FLAG_RADIO_FREQ(conf.systemFlags)) {
    rfm69cfg.freqBand = RF69_915MHZ;
  }
  rfm69Start(&rfm69cfg);
  rfm69SetHighPower(true); // long range version

  // ADC1 driver
  adcStart(&ADCD1, NULL);

  // UMM heap for TCL
  umm_init(&ohsUmmHeap[0], UMM_MALLOC_CFG_HEAP_SIZE);
  // uBS for FRAM
  uBSInit();

  // Create thread(s).
  chThdCreateStatic(waZoneThread, sizeof(waZoneThread), NORMALPRIO, ZoneThread, (void*)"zone");
  chThdCreateStatic(waAEThread1, sizeof(waAEThread1), NORMALPRIO + 1, AEThread, (void*)"alarm 1");
  chThdCreateStatic(waAEThread2, sizeof(waAEThread2), NORMALPRIO + 1, AEThread, (void*)"alarm 2");
  chThdCreateStatic(waAEThread3, sizeof(waAEThread3), NORMALPRIO + 1, AEThread, (void*)"alarm 3");
  chThdCreateStatic(waLoggerThread, sizeof(waLoggerThread), NORMALPRIO, LoggerThread, (void*)"logger");
  chThdCreateStatic(waRS485Thread, sizeof(waRS485Thread), NORMALPRIO, RS485Thread, (void*)"rs485");
  chThdCreateStatic(waRegistrationThread, sizeof(waRegistrationThread), NORMALPRIO - 1, RegistrationThread, (void*)"reg");
  chThdCreateStatic(waSensorThread, sizeof(waSensorThread), NORMALPRIO - 1, SensorThread, (void*)"sensor");
  chThdCreateStatic(waModemThread, sizeof(waModemThread), NORMALPRIO, ModemThread, (void*)"modem");
  chThdCreateStatic(waAlertThread, sizeof(waAlertThread), NORMALPRIO, AlertThread, (void*)"alert");
  chThdCreateStatic(waServiceThread, sizeof(waServiceThread), NORMALPRIO, ServiceThread, (void*)"service");
  chThdCreateStatic(waRadioThread, sizeof(waRadioThread), NORMALPRIO, RadioThread, (void*)"radio");
  chThdCreateStatic(waTriggerThread, sizeof(waTriggerThread), NORMALPRIO - 1, TriggerThread, (void*)"trigger");
  chThdCreateStatic(waTclThread, sizeof(waTclThread), LOWPRIO + 1, tclThread, (void*)"tcl");
  chThdCreateStatic(waMqttThread, sizeof(waMqttThread), NORMALPRIO - 2, MqttThread, (void*)"mqtt");
  //chThdCreateStatic(waShell, sizeof(waShell), NORMALPRIO, shellThread, (void *)&shell_cfg);
  chThdCreateStatic(waHeartBeatThread, sizeof(waHeartBeatThread), LOWPRIO, HeartBeatThread, (void*)"h-beat");

  // LWIP
  stats_init();
  LOCK_TCPIP_CORE();
  httpd_init();
  sntp_init();
  UNLOCK_TCPIP_CORE();
#if LWIP_MDNS_RESPONDER
  ETH->MACFFR |= ETH_MACFFR_PAM; // Allow multicast
  mdns_resp_register_name_result_cb(mdnsStatusReport);
  LOCK_TCPIP_CORE();
  mdns_resp_init();
  mdns_resp_add_netif(netif_default, MDNS_HOSTANME);
  mdns_resp_add_service(netif_default, MDNS_HOSTANME, "_http", DNSSD_PROTO_TCP, 80, mdnsSrvTxt, NULL);
  UNLOCK_TCPIP_CORE();
#endif

  // Read last groups state
  readFromBkpRTC((uint8_t*)&group, sizeof(group), 0);
  // Read conf struct
  readFromBkpSRAM((uint8_t*)&conf, sizeof(config_t), 0);
  chprintf(console, "Size of conf: %u, group: %u\r\n", sizeof(conf), sizeof(group));

  // Check if we have 1.5.0 -> 1.5.x version update
  if ((conf.versionMajor == 1) && (conf.versionMinor == 5)) {
    // Set new version conf struct changes

  } else if (OHS_MINOR != conf.versionMinor) {
    // Unknown version change, clear all
    setConfDefault();
    // Save the changes
    conf.versionMajor = OHS_MAJOR;
    conf.versionMinor = OHS_MINOR;
    writeToBkpSRAM((uint8_t*)&conf, sizeof(config_t), 0);
    // Init and save runtime variables
    initRuntimeGroups(); // Initialize runtime variables
    writeToBkpRTC((uint8_t*)&group, sizeof(group), 0);
  }

  // RFM69 key
  if (conf.radioKey[0] != 0) rfm69Encrypt(conf.radioKey);

  // LWIP
  LOCK_TCPIP_CORE();
  // SMTP
  smtp_set_server_addr(conf.SMTPAddress);
  smtp_set_server_port(conf.SMTPPort);
  smtp_set_auth(conf.SMTPUser, conf.SMTPPassword);
  // SNTP
  sntp_setservername(0, conf.SNTPAddress);
  UNLOCK_TCPIP_CORE();
  // MQTT
  CLEAR_CONF_MQTT_ADDRESS_ERROR(conf.mqtt.setting); // Force resolve address on start
  // Make MQTT Home Assistant Discovery uid
  chsnprintf(mqttHadUid, sizeof(mqttHadUid), "%s-%02x%02x%02x",
             OHS_NAME, macAddr[3], macAddr[4], macAddr[5]);
  // Set HTTPD connection ID to be "unique", to disallow Id=NULL as valid
  authorizedConn.id = STM32_UUID[0] + rand();

  // Start
  startTime = getTimeUnixSec();
  pushToLogText("Ss");
  // Initialize zones state
  initRuntimeZones();
  // Initialize timers
  for (uint8_t i = 0; i < TIMER_SIZE; i++) {
    if (GET_CONF_TIMER_ENABLED(conf.timer[i].setting)) setTimer(i, true);
  }

  // Idle runner
  while (true) {
    chThdSleepMilliseconds(100);

    // USB shell
    if (SDU1.config->usbp->state == USB_ACTIVE) {
      thread_t *shelltp = chThdCreateFromHeap(NULL, SHELL_WA_SIZE,
                                              "shell", NORMALPRIO + 1, shellThread, (void *)&shell_cfg);
      chThdWait(shelltp); // Waiting termination.
    }
  }
}
