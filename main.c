/*
  OHS gateway code for HW v 2.0.x
  Adam Baron 2020

*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "ch.h"
#include "hal.h"

#include "rt_test_root.h"
#include "oslib_test_root.h"

// Added from ChibiOS
#include "shell.h"
#include "chprintf.h"
#include "usbcfg.h"

// Define debug console
BaseSequentialStream* console = (BaseSequentialStream*)&SD3;

// OHS includes
#include "ohs_conf.h"
#include "ohs_shell.h"
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
char gprsModemInfo[20]; // SIMCOM_SIM7600x-x
char gprsSmsText[120];

// LWIP
#include "lwipthread.h"
#include "lwip/apps/httpd.h"
#include "lwip/apps/sntp.h"
#include "lwip/apps/smtp.h"
#include "ohs_httpdhandler.h"

// uBS
//#include "uBS.h"

// Semaphores
binary_semaphore_t gprsSem;
binary_semaphore_t emailSem;

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

/*
 * This is a periodic thread that does absolutely nothing except flashing
 * a LED.
 */
static THD_WORKING_AREA(waThread1, 256);
static THD_FUNCTION(Thread1, arg) {
  chRegSetThreadName(arg);
  systime_t time;

  while (true) {
    time = serusbcfg.usbp->state == USB_ACTIVE ? 250 : 500;

    palSetPad(GPIOC, GPIOC_HEARTBEAT);
    chThdSleepMilliseconds(time);
    palClearPad(GPIOC, GPIOC_HEARTBEAT);
    chThdSleepMilliseconds(500);
  }
}


/*
 * helper function
 */
/*
static void GetTimeTm(struct tm *timp) {
  rtcGetTime(&RTCD1, &timespec);
  rtcConvertDateTimeToStructTm(&timespec, timp, NULL);
}
*/

msg_t resp;
struct tm *ptm;

/*
 * Application entry point.
 */
int main(void) {
  halInit();
  chSysInit();

  // Semaphores
  chBSemObjectInit(&gprsSem, false);
  chBSemObjectInit(&emailSem, false);

  sdStart(&SD3,  &ser_cfg); // Debug port
  chprintf(console, "\r\nOHS v.%u.%u start\r\n", OHS_MAJOR, OHS_MINOR);

  gprsInit(&SD6); // GPRS modem

  rs485Start(&RS485D2, &ser_mpc_cfg);
  chprintf(console, "RS485 timeout: %d(uS)/%d(tick)\r\n", RS485D2.oneByteTimeUS, RS485D2.oneByteTimeI);

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

  shellInit();

  // Creating the mailboxes.
  //chMBObjectInit(&alarmEvent_mb, alarmEvent_mb_buffer, ALARMEVENT_FIFO_SIZE);

  // Pools
  chPoolObjectInit(&alarmEvent_pool, sizeof(alarmEvent_t), NULL);
  chPoolObjectInit(&logger_pool, sizeof(logger_t), NULL);
  chPoolObjectInit(&registration_pool, sizeof(registration_t), NULL);
  chPoolObjectInit(&sensor_pool, sizeof(sensor_t), NULL);
  chPoolObjectInit(&alert_pool, sizeof(alert_t), NULL);
  //chPoolObjectInit(&node_pool, sizeof(node_t), NULL);
  //chPoolLoadArray(&alarmEvent_pool, alarmEvent_pool_queue, ALARMEVENT_FIFO_SIZE);
  for(uint8_t i = 0; i < ALARMEVENT_FIFO_SIZE; i++) { chPoolFree(&alarmEvent_pool, &alarmEvent_pool_queue[i]); }
  for(uint8_t i = 0; i < LOGGER_FIFO_SIZE; i++) { chPoolFree(&logger_pool, &logger_pool_queue[i]); }
  for(uint8_t i = 0; i < REG_FIFO_SIZE; i++) { chPoolFree(&registration_pool, &registration_pool_queue[i]); }
  for(uint8_t i = 0; i < SENSOR_FIFO_SIZE; i++) { chPoolFree(&sensor_pool, &sensor_pool_queue[i]); }
  for(uint8_t i = 0; i < ALERT_FIFO_SIZE; i++) { chPoolFree(&alert_pool, &alert_pool_queue[i]); }
  //for(uint8_t i = 0; i < NODE_SIZE; i++) { chPoolFree(&node_pool, &node_pool_queue[i]); }

  spiStart(&SPID1, &spi1cfg);  // SPI
  adcStart(&ADCD1, NULL);      // Activates the ADC1 driver

  // Create thread(s).
  chThdCreateStatic(waZoneThread, sizeof(waZoneThread), NORMALPRIO, ZoneThread, (void*)"zone");
  chThdCreateStatic(waAEThread1, sizeof(waAEThread1), NORMALPRIO, AEThread, (void*)"alarm 1");
  chThdCreateStatic(waAEThread2, sizeof(waAEThread2), NORMALPRIO, AEThread, (void*)"alarm 2");
  chThdCreateStatic(waAEThread3, sizeof(waAEThread3), NORMALPRIO, AEThread, (void*)"alarm 3");
  chThdCreateStatic(waLoggerThread, sizeof(waLoggerThread), NORMALPRIO, LoggerThread, (void*)"logger");
  chThdCreateStatic(waRS485Thread, sizeof(waRS485Thread), NORMALPRIO, RS485Thread, (void*)"RS485");
  chThdCreateStatic(waRegistrationThread, sizeof(waRegistrationThread), NORMALPRIO, RegistrationThread, (void*)"registration");
  chThdCreateStatic(waSensorThread, sizeof(waSensorThread), NORMALPRIO, SensorThread, (void*)"sensor");
  chThdCreateStatic(waModemThread, sizeof(waModemThread), NORMALPRIO, ModemThread, (void*)"modem");
  chThdCreateStatic(waAlertThread, sizeof(waAlertThread), NORMALPRIO, AlertThread, (void*)"alert");
  chThdCreateStatic(waServiceThread, sizeof(waServiceThread), NORMALPRIO, ServiceThread, (void*)"service");
  chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, (void*)"heartbeat");
  /*
  static THD_WORKING_AREA(waShell, 2048);
  chThdCreateStatic(waShell, sizeof(waShell), NORMALPRIO + 1, shellThread, (void *)&shell_cfg1);
  */

  //  shellInit();
  //chThdCreateStatic(waShell, sizeof(waShell), NORMALPRIO, shellThread, (void *)&shell_cfg1);

  // Ethernet
  macAddr[0] = LWIP_ETHADDR_0;
  macAddr[1] = LWIP_ETHADDR_1;
  macAddr[2] = LWIP_ETHADDR_2;
  macAddr[3] = STM32_UUID[0] & 0xff;
  macAddr[4] = (STM32_UUID[0] >> 8) & 0xff;
  macAddr[5] = (STM32_UUID[0] >> 16) & 0xff;
  struct lwipthread_opts lwip_opts =
  { &macAddr[0], 0, 0, 0, NET_ADDRESS_DHCP
    #if LWIP_NETIF_HOSTNAME
      , 0
    #endif
  };

  lwipInit(&lwip_opts);
  httpd_init();
  sntp_init();
  mdns_resp_init();

  // Read last group[] state
  readFromBkpRTC((uint8_t*)&group, sizeof(group), 0);
  // Read conf.
  readFromBkpSRAM((uint8_t*)&conf, sizeof(config_t), 0);
  chprintf(console, "Size of conf: %u, group: %u\r\n", sizeof(conf), sizeof(group));
  // Check if we have new major version update
  if (conf.versionMajor != OHS_MAJOR) {
    setConfDefault(); // Load OHS default conf.
    initRuntimeGroups(); // Initialize runtime variables
    writeToBkpSRAM((uint8_t*)&conf, sizeof(config_t), 0);
    writeToBkpRTC((uint8_t*)&group, sizeof(group), 0);
  }
  //setConfDefault(); // Load OHS default conf.
  // SMTP
  smtp_set_server_addr(conf.SMTPAddress);
  smtp_set_server_port(conf.SMTPPort);
  smtp_set_auth(conf.SMTPUser, conf.SMTPPassword);
  // SNTP
  sntp_setservername(0, conf.SNTPAddress);

  /*
  uint16_t uBSaddress;
  uBSInit();
  uBSGetFreeBlock(&uBSaddress);
  chprintf(console, "uBS, free space: %u, First block: %u\r\n", uBSGetFreeSpace(), uBSaddress);
  */

  // Start
  startTime = getTimeUnixSec();
  pushToLogText("Ss");
  // Initialize zones state
  initRuntimeZones();

  // Idle runner
  while (true) {
    /*
    if (SDU1.config->usbp->state == USB_ACTIVE) {
      thread_t *shelltp = chThdCreateFromHeap(NULL, SHELL_WA_SIZE,"shell", NORMALPRIO + 1, shellThread, (void *)&shell_cfg1);
      chThdWait(shelltp);               // Waiting termination.
    }
    */

    chThdSleepMilliseconds(10000);

    /*
    temptime = calculateDST(2019, 3, 0, 0, 2);
    chprintf(console, "DST s %d \r\n", temptime);
    ptm = gmtime(&temptime);
    chprintf(console, "DST s %s \r\n", asctime(ptm));

    temptime = calculateDST(2019, 10, 0, 0, 3);
    chprintf(console, "DST e %d \r\n", temptime);
    ptm = gmtime(&temptime);
    chprintf(console, "DST e %s \r\n", asctime(ptm));
    */

    // Dump BKP SRAM
    /*
    for(uint16_t i = 0; i < 0x100; i++) {
      data = *(baseAddress + i);
      chprintf(console, "%x ", data);
      if (((i+1)%0x10) == 0) {
        chprintf(console, "\r\n");
        chThdSleepMilliseconds(2);
      }
    }
    */
    // Dump BKP RTC
    /*
    for(uint16_t i = 0; i < 20; i++) {
      data32 = *(RTCBaseAddress + i);
      chprintf(console, "%x %x %x %x ", (data32 >> 24) &0xFF, (data32 >> 16) &0xFF,(data32 >> 8) &0xFF,(data32 >> 0) &0xFF);
      if (((i+1)%4) == 0) {
        chprintf(console, "\r\n");
        chThdSleepMilliseconds(2);
      }
    }
    */
    // BKP RTC read test
    /*
    chprintf(console, ">%s, %d<\r\n", myStr, sizeof(myStr));
    chprintf(console, "Read >%d\r\n", readFromBkpRTC((uint8_t*)&myStr, sizeof(myStr), 0));
    chprintf(console, ">%s<\r\n", myStr);
    */

    /*
    // Send RS485 registration request
    chprintf(console, "RS485: %d, %d, %d, %d\r\n", RS485D2.state, RS485D2.trcState, RS485D2.ibHead, RS485D2.ibExpLen);
    RS485Msg_t rs485Msg;
    rs485Msg.address = 1;
    //rs485Msg.ctrl = RS485_FLAG_DTA;
    rs485Msg.ctrl = RS485_FLAG_CMD;
    //rs485Msg.length = 10;
    rs485Msg.length = 1;
    for (uint8_t i = 0; i < rs485Msg.length; i++) { rs485Msg.data[i] = i; }
    resp = rs485SendMsgWithACK(&RS485D2, &rs485Msg, 3);
    chprintf(console, "Sent: %d\r\n", resp);
    */

  }
}
