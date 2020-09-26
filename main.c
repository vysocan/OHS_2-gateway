/*
 * OHS gateway code for HW v 2.0.x
 * Adam Baron 2020
 *
 *
 */
// Optimize stack and overflow
#define PORT_INT_REQUIRED_STACK 128
// Remove input queue for GPRS to save RAM
#define STM32_SERIAL_USART6_IN_BUF_SIZE 0
//#define assert_param(expr) ((void)0) // TODO OHS remove?


#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

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

// Semaphores
binary_semaphore_t gprsSem;
binary_semaphore_t emailSem;
// TCL callback semaphores
binary_semaphore_t cbTimerSem;
binary_semaphore_t cbTriggerSem;

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

// OHS includes
#include "ohs_conf.h"
#include "ohs_functions.h"
#include "ohs_peripheral.h"

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
#include "lwip/opt.h" //
#include "lwip/arch.h"//
#include "lwip/api.h" //
#include "lwip/stats.h"
#include "lwip/apps/httpd.h"
#include "lwip/apps/sntp.h"
#include "lwip/apps/smtp.h"
#include "lwip/apps/mdns.h"
#include "ohs_httpdhandler.h"

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
#include "ohs_th_heartbeat.h"
#define OHS_HTTPS 1
#if OHS_HTTPS
#include "ohs_th_https.h"
#endif

#if OHS_HTTPS
#include "crypto.h"
/* Private typedef -----------------------------------------------------------*/
typedef enum {FAILED = 0, PASSED = !FAILED} TestStatus;
/* Private define ------------------------------------------------------------*/
#define PLAINTEXT_LENGTH 64
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
const uint8_t Plaintext[PLAINTEXT_LENGTH] =
  {
    0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
    0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a,
    0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c,
    0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51,
    0x30, 0xc8, 0x1c, 0x46, 0xa3, 0x5c, 0xe4, 0x11,
    0xe5, 0xfb, 0xc1, 0x19, 0x1a, 0x0a, 0x52, 0xef,
    0xf6, 0x9f, 0x24, 0x45, 0xdf, 0x4f, 0x9b, 0x17,
    0xad, 0x2b, 0x41, 0x7b, 0xe6, 0x6c, 0x37, 0x10
  };

/* Key to be used for AES encryption/decryption */
uint8_t Key[CRL_AES128_KEY] =
  {
    0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
    0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
  };

/* Initialization Vector, used only in non-ECB modes */
uint8_t IV[CRL_AES_BLOCK] =
  {
    0xf0 , 0xf1 , 0xf2 , 0xf3 , 0xf4 , 0xf5 , 0xf6 , 0xf7,
    0xf8 , 0xf9 , 0xfa , 0xfb , 0xfc , 0xfd , 0xfe , 0xff
  };


/* Buffer to store the output data */
uint8_t OutputMessage[PLAINTEXT_LENGTH];

/* Size of the output data */
uint32_t OutputMessageLength = 0;

const uint8_t Expected_Ciphertext[PLAINTEXT_LENGTH] =
  {
    0x87, 0x4d, 0x61, 0x91, 0xb6, 0x20, 0xe3, 0x26,
    0x1b, 0xef, 0x68, 0x64, 0x99, 0x0d, 0xb6, 0xce,
    0x98, 0x06, 0xf6, 0x6b, 0x79, 0x70, 0xfd, 0xff,
    0x86, 0x17, 0x18, 0x7b, 0xb9, 0xff, 0xfd, 0xff,
    0x5a, 0xe4, 0xdf, 0x3e, 0xdb, 0xd5, 0xd3, 0x5e,
    0x5b, 0x4f, 0x09, 0x02, 0x0d, 0xb0, 0x3e, 0xab,
    0x1e, 0x03, 0x1d, 0xda, 0x2f, 0xbe, 0x03, 0xd1,
    0x79, 0x21, 0x70, 0xa0, 0xf3, 0x00, 0x9c, 0xee
  };

/* Private function prototypes -----------------------------------------------*/
int32_t STM32_AES_CTR_Encrypt(uint8_t*  InputMessage,
                        uint32_t  InputMessageLength,
                        uint8_t  *AES128_Key,
                        uint8_t  *InitializationVector,
                        uint32_t  IvLength,
                        uint8_t  *OutputMessage,
                        uint32_t *OutputMessageLength);

int32_t STM32_AES_CTR_Decrypt(uint8_t*  InputMessage,
                        uint32_t  InputMessageLength,
                        uint8_t  *AES128_Key,
                        uint8_t  *InitializationVector,
                        uint32_t  IvLength,
                        uint8_t  *OutputMessage,
                        uint32_t *OutputMessageLength);

TestStatus Buffercmp(const uint8_t* pBuffer,
                     uint8_t* pBuffer1,
                     uint16_t BufferLength);
/* Private functions ---------------------------------------------------------*/
#endif

#define SHELL_WA_SIZE THD_WORKING_AREA_SIZE(2*2048)

#if LWIP_MDNS_RESPONDER
static void srv_txt(struct mdns_service *service, void *txt_userdata){
  err_t res;
  LWIP_UNUSED_ARG(txt_userdata);

  res = mdns_resp_add_service_txtitem(service, "path=/", 6);
  chprintf(console, "mdns add service txt status %d.\r\n", res);
}
#endif

#if LWIP_MDNS_RESPONDER
static void mdns_example_report(struct netif* netif, u8_t result, s8_t service){
  chprintf(console,"mdns status[netif %d][service %d]: %d\r\n", netif->num, service, result);
}
#endif



/*
 * Application entry point.
 */
int main(void) {
  halInit();
  chSysInit();

  /*
   * Initialize RNG
   */
  rccEnableAHB2(RCC_AHB2ENR_RNGEN, 0);
  RNG->CR |= RNG_CR_IE;
  RNG->CR |= RNG_CR_RNGEN;

  // Semaphores
  chBSemObjectInit(&gprsSem, false);
  chBSemObjectInit(&emailSem, false);
  chBSemObjectInit(&cbTimerSem, false);
  chBSemObjectInit(&cbTriggerSem, false);
  // Debug port
  sdStart(&SD3,  &serialCfg);
  chprintf(console, "\r\nOHS v.%u.%u start\r\n", OHS_MAJOR, OHS_MINOR);
  // GPRS modem
  gprsInit(&SD6);
  // Init nodes
  initRuntimeNodes();
  // RS485
  rs485Start(&RS485D2, &rs485cfg);
  chprintf(console, "RS485 timeout: %d(uS)/%d(tick)\r\n", RS485D2.oneByteTimeUS, RS485D2.oneByteTimeI);

  // Ethernet
  macAddr[0] = LWIP_ETHADDR_0; macAddr[1] = LWIP_ETHADDR_1; macAddr[2] = LWIP_ETHADDR_2;
  macAddr[3] = STM32_UUID[0] & 0xff;
  macAddr[4] = (STM32_UUID[0] >> 8) & 0xff;
  macAddr[5] = (STM32_UUID[0] >> 16) & 0xff;
  struct lwipthread_opts lwip_opts =
  { &macAddr[0], 0, 0, 0, NET_ADDRESS_DHCP
    #if LWIP_NETIF_HOSTNAME
      ,"OHS"
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
  memset(&alertMsg[0], 0 , HTTP_ALERT_MSG_SIZE); // Empty alert message

  shellInit();

  // Creating the mailboxes.
  //chMBObjectInit(&alarmEvent_mb, alarmEvent_mb_buffer, ALARMEVENT_FIFO_SIZE);

  // Pools
  chPoolObjectInit(&alarmEvent_pool, sizeof(alarmEvent_t), NULL);
  chPoolObjectInit(&logger_pool, sizeof(loggerEvent_t), NULL);
  chPoolObjectInit(&registration_pool, sizeof(registrationEvent_t), NULL);
  chPoolObjectInit(&sensor_pool, sizeof(sensorEvent_t), NULL);
  chPoolObjectInit(&alert_pool, sizeof(alertEvent_t), NULL);
  chPoolObjectInit(&script_pool, sizeof(scriptEvent_t), NULL);
  chPoolObjectInit(&trigger_pool, sizeof(triggerEvent_t), NULL);
  //chPoolLoadArray(&alarmEvent_pool, alarmEvent_pool_queue, ALARMEVENT_FIFO_SIZE);
  for(uint8_t i = 0; i < ALARMEVENT_FIFO_SIZE; i++) { chPoolFree(&alarmEvent_pool, &alarmEvent_pool_queue[i]); }
  for(uint8_t i = 0; i < LOGGER_FIFO_SIZE; i++) { chPoolFree(&logger_pool, &logger_pool_queue[i]); }
  for(uint8_t i = 0; i < REG_FIFO_SIZE; i++) { chPoolFree(&registration_pool, &registration_pool_queue[i]); }
  for(uint8_t i = 0; i < SENSOR_FIFO_SIZE; i++) { chPoolFree(&sensor_pool, &sensor_pool_queue[i]); }
  for(uint8_t i = 0; i < ALERT_FIFO_SIZE; i++) { chPoolFree(&alert_pool, &alert_pool_queue[i]); }
  for(uint8_t i = 0; i < SCRIPT_FIFO_SIZE; i++) { chPoolFree(&script_pool, &script_pool_queue[i]); }
  for(uint8_t i = 0; i < TRIGGER_FIFO_SIZE; i++) { chPoolFree(&trigger_pool, &trigger_pool_queue[i]); }

  // SPI
  spiStart(&SPID1, &spi1cfg);
  // RFM69
  rfm69Start(&rfm69cfg);
  rfm69SetHighPower(true);     // long range version

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
  chThdCreateStatic(waRS485Thread, sizeof(waRS485Thread), NORMALPRIO, RS485Thread, (void*)"RS485");
  chThdCreateStatic(waRegistrationThread, sizeof(waRegistrationThread), NORMALPRIO - 1, RegistrationThread, (void*)"registration");
  chThdCreateStatic(waSensorThread, sizeof(waSensorThread), NORMALPRIO - 1, SensorThread, (void*)"sensor");
  chThdCreateStatic(waModemThread, sizeof(waModemThread), NORMALPRIO, ModemThread, (void*)"modem");
  chThdCreateStatic(waAlertThread, sizeof(waAlertThread), NORMALPRIO, AlertThread, (void*)"alert");
  chThdCreateStatic(waServiceThread, sizeof(waServiceThread), NORMALPRIO, ServiceThread, (void*)"service");
  chThdCreateStatic(waRadioThread, sizeof(waRadioThread), NORMALPRIO, RadioThread, (void*)"radio");
  chThdCreateStatic(waTriggerThread, sizeof(waTriggerThread), NORMALPRIO - 1, TriggerThread, (void*)"trigger");
  chThdCreateStatic(waTclThread, sizeof(waTclThread), LOWPRIO + 1, tclThread, (void*)"tcl");
  chThdCreateStatic(waHeartBeatThread, sizeof(waHeartBeatThread), LOWPRIO, HeartBeatThread, (void*)"heartbeat");
  //static THD_WORKING_AREA(waShell, 2048);
  //chThdCreateStatic(waShell, sizeof(waShell), NORMALPRIO, shellThread, (void *)&shell_cfg);

  stats_init();
  //ETH->MACFFR |= ETH_MACFFR_PAM;
  httpd_init();
  sntp_init();
  // TODO OHS implement IGMP and MDNS
#if LWIP_MDNS_RESPONDER
  chThdSleepMilliseconds(100);
  mdns_resp_register_name_result_cb(mdns_example_report);
  mdns_resp_init();
  mdns_resp_add_netif(netif_default, "ohs");
  //chprintf(console, "netif_default %x\r\n", netif_default);
  mdns_resp_add_service(netif_default, "ohs", "_http", DNSSD_PROTO_TCP, 80, srv_txt, NULL);
  //mdns_resp_announce(netif_default);
  chThdSleepMilliseconds(100);
#endif

#if OHS_HTTPS
  chThdCreateStatic(wa_https_server, sizeof(wa_https_server), NORMALPRIO + 1, https_server, NULL);
#endif


  // Read last groups state
  readFromBkpRTC((uint8_t*)&group, sizeof(group), 0);
  // Read conf struct
  readFromBkpSRAM((uint8_t*)&conf, sizeof(config_t), 0);
  chprintf(console, "Size of conf: %u, group: %u\r\n", sizeof(conf), sizeof(group));

  // Check if we have new major version update
  if (conf.versionMajor != OHS_MAJOR) {
    setConfDefault();    // Load OHS default conf.
    initRuntimeGroups(); // Initialize runtime variables
    writeToBkpSRAM((uint8_t*)&conf, sizeof(config_t), 0);
    writeToBkpRTC((uint8_t*)&group, sizeof(group), 0);
  }
  //setConfDefault(); // Load OHS default conf.

  // TODO OHS Allow driver to set frequency
  // RFM69 key
  if (conf.radioKey[0] != 0) rfm69Encrypt(conf.radioKey);
  // SMTP
  smtp_set_server_addr(conf.SMTPAddress);
  smtp_set_server_port(conf.SMTPPort);
  smtp_set_auth(conf.SMTPUser, conf.SMTPPassword);
  // SNTP
  sntp_setservername(0, conf.SNTPAddress);

  // Start
  startTime = getTimeUnixSec();
  pushToLogText("Ss");
  // Initialize zones state
  initRuntimeZones();
  // Initialize timers
  for (uint8_t i = 0; i < TIMER_SIZE; i++) {
    if (GET_CONF_TIMER_ENABLED(conf.timer[i].setting)) setTimer(i, true);
  }

  size_t n, total, largest;
  // Idle runner
  while (true) {
    chThdSleepMilliseconds(100);

    // USB shell
    if (SDU1.config->usbp->state == USB_ACTIVE) {
      thread_t *shelltp = chThdCreateFromHeap(NULL, SHELL_WA_SIZE,
                                              "shell", NORMALPRIO + 1, shellThread, (void *)&shell_cfg);
      chThdWait(shelltp); // Waiting termination.
    }

    /*
    n = chHeapStatus(NULL, &total, &largest);
    chprintf(console, "core free memory : %u bytes" SHELL_NEWLINE_STR, chCoreGetStatusX());
    chprintf(console, "heap fragments   : %u" SHELL_NEWLINE_STR, n);
    chprintf(console, "heap free total  : %u bytes" SHELL_NEWLINE_STR, total);
    chprintf(console, "heap free largest: %u bytes" SHELL_NEWLINE_STR, largest);
    */
    //umm_info(&ohsUmmHeap[0], true);


    #if OHS_HTTPS
    /*!< At this stage the microcontroller clock setting is already configured,
          this is done through SystemInit() function which is called from startup
          file before to branch to application main.
          To reconfigure the default setting of SystemInit() function, refer to
          system_stm32f10x.c, system_stm32l1xx.c, system_stm32f0xx.c,
          system_stm32f2xx.c, system_stm32f30x.c, system_stm32f37x.c, or
          system_stm32f4xx.c file depending on device.
        */
     int32_t status = AES_SUCCESS;

     /* DeInitialize STM32 Cryptographic Library */
     Crypto_DeInit();

     /* Encrypt DATA with AES in CTR mode */
     status = STM32_AES_CTR_Encrypt( (uint8_t *) Plaintext, PLAINTEXT_LENGTH, Key, IV, sizeof(IV), OutputMessage,
                               &OutputMessageLength);
     if (status == AES_SUCCESS)
     {
       if (Buffercmp(Expected_Ciphertext, OutputMessage, PLAINTEXT_LENGTH) == PASSED)
       {
         /* add application traintment in case of AES CTR encrption is passed */
         chprintf(console, "AES CTR encrption is passed\r\n");
       }
       else
       {
         /* add application traintment in case of AES CTR encrption is failed */
         chprintf(console, "AES CTR encrption is failed\r\n");
       }
     }
     else
     {
       /* Add application traintment in case of encryption/decryption not success possible values
          *  of status:
          * AES_ERR_BAD_CONTEXT, AES_ERR_BAD_PARAMETER, AES_ERR_BAD_OPERATION
          */
     }
     status = STM32_AES_CTR_Decrypt( (uint8_t *) Expected_Ciphertext, PLAINTEXT_LENGTH, Key, IV, sizeof(IV), OutputMessage,
                               &OutputMessageLength);
     if (status == AES_SUCCESS)
     {
       if (Buffercmp(Plaintext, OutputMessage, PLAINTEXT_LENGTH) == PASSED)
       {
         /* add application traintment in case of AES CTR encrption is passed */
         chprintf(console, "AES CTR decrption is passed\r\n");
       }
       else
       {
         /* add application traintment in case of AES CTR encrption is failed */
         chprintf(console, "AES CTR decrption is failed\r\n");
       }
     }
     else
     {
       /* Add application traintment in case of encryption/decryption not success possible values
          *  of status:
          * AES_ERR_BAD_CONTEXT, AES_ERR_BAD_PARAMETER, AES_ERR_BAD_OPERATION
          */
     }

     chThdSleepMilliseconds(10000);

    #endif
  }
}

#if OHS_HTTPS
/**
  * @brief  AES CTR Encryption example.
  * @param  InputMessage: pointer to input message to be encrypted.
  * @param  InputMessageLength: input data message length in byte.
  * @param  AES128_Key: pointer to the AES key to be used in the operation
  * @param  InitializationVector: pointer to the Initialization Vector (IV)
  * @param  IvLength: IV length in bytes.
  * @param  OutputMessage: pointer to output parameter that will handle the encrypted message
  * @param  OutputMessageLength: pointer to encrypted message length.
  * @retval error status: can be AES_SUCCESS if success or one of
  *         AES_ERR_BAD_CONTEXT, AES_ERR_BAD_PARAMETER, AES_ERR_BAD_OPERATION
  *         if error occured.
  */
int32_t STM32_AES_CTR_Encrypt(uint8_t* InputMessage,
                        uint32_t InputMessageLength,
                        uint8_t  *AES128_Key,
                        uint8_t  *InitializationVector,
                        uint32_t  IvLength,
                        uint8_t  *OutputMessage,
                        uint32_t *OutputMessageLength)
{
  AESCTRctx_stt AESctx;

  uint32_t error_status = AES_SUCCESS;

  int32_t outputLength = 0;

  /* Set flag field to default value */
  AESctx.mFlags = E_SK_DEFAULT;

  /* Set key size to 16 (corresponding to AES-128) */
  AESctx.mKeySize = 16;

  /* Set iv size field to IvLength*/
  AESctx.mIvSize = IvLength;

  /* Initialize the operation, by passing the key.
   * Third parameter is NULL because CTR doesn't use any IV */
  error_status = AES_CTR_Encrypt_Init(&AESctx, AES128_Key, InitializationVector );

  /* check for initialization errors */
  if (error_status == AES_SUCCESS)
  {
    /* Encrypt Data */
    error_status = AES_CTR_Encrypt_Append(&AESctx,
                                          InputMessage,
                                          InputMessageLength,
                                          OutputMessage,
                                          &outputLength);

    if (error_status == AES_SUCCESS)
    {
      /* Write the number of data written*/
      *OutputMessageLength = outputLength;
      /* Do the Finalization */
      error_status = AES_CTR_Encrypt_Finish(&AESctx, OutputMessage + *OutputMessageLength, &outputLength);
      /* Add data written to the information to be returned */
      *OutputMessageLength += outputLength;
    }
  }

  return error_status;
}


/**
  * @brief  AES CTR Decryption example.
  * @param  InputMessage: pointer to input message to be decrypted.
  * @param  InputMessageLength: input data message length in byte.
  * @param  AES128_Key: pointer to the AES key to be used in the operation
  * @param  InitializationVector: pointer to the Initialization Vector (IV)
  * @param  IvLength: IV length in bytes.
  * @param  OutputMessage: pointer to output parameter that will handle the decrypted message
  * @param  OutputMessageLength: pointer to decrypted message length.
  * @retval error status: can be AES_SUCCESS if success or one of
  *         AES_ERR_BAD_CONTEXT, AES_ERR_BAD_PARAMETER, AES_ERR_BAD_OPERATION
  *         if error occured.
  */
int32_t STM32_AES_CTR_Decrypt(uint8_t* InputMessage,
                        uint32_t InputMessageLength,
                        uint8_t  *AES128_Key,
                        uint8_t  *InitializationVector,
                        uint32_t  IvLength,
                        uint8_t  *OutputMessage,
                        uint32_t *OutputMessageLength)
{
  AESCTRctx_stt AESctx;

  uint32_t error_status = AES_SUCCESS;

  int32_t outputLength = 0;

  /* Set flag field to default value */
  AESctx.mFlags = E_SK_DEFAULT;

  /* Set key size to 16 (corresponding to AES-128) */
  AESctx.mKeySize = 16;

  /* Set iv size field to IvLength*/
  AESctx.mIvSize = IvLength;

  /* Initialize the operation, by passing the key.
   * Third parameter is NULL because CTR doesn't use any IV */
  error_status = AES_CTR_Decrypt_Init(&AESctx, AES128_Key, InitializationVector );

  /* check for initialization errors */
  if (error_status == AES_SUCCESS)
  {
    /* Decrypt Data */
    error_status = AES_CTR_Decrypt_Append(&AESctx,
                                          InputMessage,
                                          InputMessageLength,
                                          OutputMessage,
                                          &outputLength);

    if (error_status == AES_SUCCESS)
    {
      /* Write the number of data written*/
      *OutputMessageLength = outputLength;
      /* Do the Finalization */
      error_status = AES_CTR_Decrypt_Finish(&AESctx, OutputMessage + *OutputMessageLength, &outputLength);
      /* Add data written to the information to be returned */
      *OutputMessageLength += outputLength;
    }
  }

  return error_status;
}

/**
  * @brief  Compares two buffers.
  * @param  pBuffer, pBuffer1: buffers to be compared.
  * @param  BufferLength: buffer's length
  * @retval PASSED: pBuffer identical to pBuffer1
  *         FAILED: pBuffer differs from pBuffer1
  */
TestStatus Buffercmp(const uint8_t* pBuffer, uint8_t* pBuffer1, uint16_t BufferLength)
{
  while (BufferLength--)
  {
    if (*pBuffer != *pBuffer1)
    {
      return FAILED;
    }

    pBuffer++;
    pBuffer1++;
  }

  return PASSED;
}
#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *   where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {}
}
#endif
#endif
