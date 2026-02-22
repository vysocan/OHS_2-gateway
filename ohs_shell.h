/*
 * ohs_shell.h
 *
 *  Created on: 19. 10. 2018
 *      Author: vysocan
 */

#ifndef OHS_SHELL_H_
#define OHS_SHELL_H_

// Show showpin command in shell interface
#ifndef SHELL_SHOW_PIN
#define SHELL_SHOW_PIN 0
#endif

/*
 * Console applet to show log entries
 */
static void cmd_log(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)argv;

  if (argc > 1)  { goto ERROR; }
  if (argc == 1) {
    if (strcmp(argv[0], "format") == 0) {
      goto FORMAT;
    } else {
      safeStrtoul(argv[0], (uint32_t *)&FRAMReadPos, 0);
      FRAMReadPos = (FRAMReadPos - LOGGER_OUTPUT_LEN) * FRAM_MSG_SIZE;
    }
  }
  if (argc == 0) {
    FRAMReadPos = FRAMWritePos - (FRAM_MSG_SIZE * LOGGER_OUTPUT_LEN);
    if (FRAMReadPos >= UINT16_MAX) {
      FRAMReadPos = 0;
    }
  }

  spiAcquireBus(&SPID1);                // Acquire ownership of the bus.
  for(uint16_t i = 0; i < LOGGER_OUTPUT_LEN; i++) {
    getLogEntry(FRAMReadPos);
    memcpy(&timeConv.ch[0], &rxBuffer[0], sizeof(timeConv.ch)); // Prepare timestamp
    decodeLog(&rxBuffer[4], logText, true);

    chprintf(chp, "#%4u : ", 1 + (FRAMReadPos/FRAM_MSG_SIZE));
    printFrmTimestamp(chp, &timeConv.val);
    chprintf(chp, " : %s.", logText);
    chprintf(chp, " : Flags: ");
    for (uint8_t j = 0; j < ARRAY_SIZE(alertType); j++) {
      if ((((uint8_t)rxBuffer[FRAM_MSG_SIZE-1] >> j) & 0b1) == 1)
        chprintf(chp, "%s ", alertType[j].name);
    }
    chprintf(chp, SHELL_NEWLINE_STR);

    FRAMReadPos += FRAM_MSG_SIZE;       // Advance for next read
  }
  spiReleaseBus(&SPID1);                // Ownership release.
  // Show usage if no aguments
  if (argc == 0) {
    chprintf(chp, SHELL_NEWLINE_STR);
    goto ERROR;
  }
  return;

FORMAT:
  // Set 0xFF as initial/empty state, same as new FRAM
  memset(&txBuffer[FRAM_HEADER_SIZE], 0xFF, FRAM_MSG_SIZE);

  // SPI
  spiAcquireBus(&SPID1);
  for(uint32_t i = 0; i < UINT16_MAX; i += FRAM_MSG_SIZE) {
    spiSelect(&SPID1);
    txBuffer[0] = CMD_25AA_WREN;
    spiSend(&SPID1, 1, txBuffer);
    spiUnselect(&SPID1);
    // Set start address
    txBuffer[0] = CMD_25AA_WRITE;
    txBuffer[1] = 0;
    txBuffer[2] = (i >> 8) & 0xFF;
    txBuffer[3] = i & 0xFF;
    // Write
    spiSelect(&SPID1);
    spiSend(&SPID1, FRAM_HEADER_SIZE + FRAM_MSG_SIZE, txBuffer);
    spiUnselect(&SPID1);
  }
  spiReleaseBus(&SPID1);

  FRAMReadPos = FRAMWritePos = 0;
  chprintf(chp, "Log formated." SHELL_NEWLINE_STR);
  return;

ERROR:
  chprintf(chp, "Usage: log - show this, current log entry." SHELL_NEWLINE_STR);
  chprintf(chp, "       log N - where N is log last entry number(1 - 4096)." SHELL_NEWLINE_STR);
  chprintf(chp, "       log format - erase all log entries." SHELL_NEWLINE_STR);
  return;
}
/*
 * Console applet for date set and get
 */
static void cmd_date(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)argv;
  struct tm *ptm;
  char   dateTime[30];
  time_t unix_time;

  if (argc == 0) {
    unix_time = getTimeUnixSec();

    if (unix_time == -1){
      chprintf(chp, "Incorrect time in RTC cell." SHELL_NEWLINE_STR);
    }
    else{
      ptm = gmtime(&unix_time);
      strftime(dateTime, 30, conf.dateTimeFormat, ptm); // Format date time as needed
      chprintf(chp, "Current: %d ", unix_time);
      chprintf(chp, "%s, ", durationSelect[0]);
      chprintf(chp, "%s" SHELL_NEWLINE_STR, dateTime);
    }
    return;
  }

  if ((argc == 2) && (strcmp(argv[0], "set") == 0)){
    unix_time = strtol(argv[1], NULL, 0);
    // 315532800 = 1.1.1980 00:00:00
    if (unix_time > 315532800){
      convertUnixSecondToRTCDateTime(&timespec, unix_time);
      rtcSetTime(&RTCD1, &timespec);
      chprintf(chp, "Date set." SHELL_NEWLINE_STR);
      return;
    }
  }

  // Rest is error
  chprintf(chp, "Usage: date" SHELL_NEWLINE_STR);
  chprintf(chp, "       date set N - where N is time in seconds since Unix epoch, and greater then 1.1.1980(315532801)." SHELL_NEWLINE_STR);
  return;
}
/*
 * Applet to route console to USB
 */
static void cmd_debug(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)argv;

  if (argc == 1) {
    if (strcmp(argv[0], "on") == 0) {
      // Reroute all console to USB
      console = (BaseSequentialStream*)&SDU1;
      chprintf(chp, SHELL_NEWLINE_STR "Debug ON." SHELL_NEWLINE_STR);
      return;
    } else if (strcmp(argv[0], "off") == 0) {
      // Reroute all console back to SD3
      console = (BaseSequentialStream*)&SD3;
      chprintf(chp, SHELL_NEWLINE_STR "Debug OFF." SHELL_NEWLINE_STR);
      return;
    }
  }
  // Rest is error
  shellUsage(chp, "debug on|off");
}
/*
 * Helper to show FRAM content
 */
static void cmd_ubs_show_fram(BaseSequentialStream *chp, uint16_t block) {
  uint32_t addressStart = UBS_ADDRESS_START + (block * UBS_BLOCK_SIZE);
  uint8_t ubsRxBuffer[16], ubsTxBuffer[4];
  
  chprintf(chp, "Block %u (Addr: 0x%06X):" SHELL_NEWLINE_STR, block, addressStart);
  chprintf(chp, "      0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F" SHELL_NEWLINE_STR);

  for (uint16_t i = 0; i < UBS_BLOCK_SIZE; i += 16) {
    uint32_t currentAddr = addressStart + i;

    ubsTxBuffer[0] = CMD_25AA_READ;
    ubsTxBuffer[1] = (currentAddr >> 16) & 0xFF;
    ubsTxBuffer[2] = (currentAddr >> 8) & 0xFF;
    ubsTxBuffer[3] = (currentAddr) & 0xFF;
    spiAcquireBus(&SPID1);
    spiSelect(&SPID1);
    spiSend(&SPID1, 4, ubsTxBuffer);
    spiReceive(&SPID1, 16, ubsRxBuffer);
    spiUnselect(&SPID1);
    spiReleaseBus(&SPID1);

    chprintf(chp, "%04X: ", i);
    
    // Print Hex
    for (uint8_t j = 0; j < 16; j++) {
      chprintf(chp, "%02X ", ubsRxBuffer[j]);
    }
    
    // Print ASCII
    for (uint8_t j = 0; j < 16; j++) {
      if (ubsRxBuffer[j] >= 32 && ubsRxBuffer[j] < 127) {
        chprintf(chp, "%c", ubsRxBuffer[j]);
      } else {
        chprintf(chp, ".");
      }
    }
    chprintf(chp, SHELL_NEWLINE_STR);
  }
}

/*
 * Applet to uBS
 * Usage: ubs show - show uBS status
 *        ubs show N - show Nth block
 *        ubs format - format uBS
 */
static void cmd_ubs(BaseSequentialStream *chp, int argc, char *argv[]) {

  if ((argc == 2) && (argv[0][0] == 's')) {
    uint32_t block;
    safeStrtoul(argv[1], &block, 0);

    if (block < UBS_BLOCK_COUNT) {
      cmd_ubs_show_fram(chp, (uint16_t)block);
    } else {
      chprintf(chp, "Error: Block number out of range (0 - %d)." SHELL_NEWLINE_STR, UBS_BLOCK_COUNT - 1);
    }
    return;
  }

  if (argc == 1) {
    switch (argv[0][0]) {
      case 's':
        if (argv[0][1] != 't') {
          goto ERROR;
        }
        // Show uBS status
        chprintf(chp, "uBS      total    free    used" SHELL_NEWLINE_STR);
        chprintf(chp, "------------------------------" SHELL_NEWLINE_STR);
        chprintf(chp, "blocks : %5u   %5u   %5u" SHELL_NEWLINE_STR,
                 UBS_BLOCK_COUNT, uBSFreeBlocks, UBS_BLOCK_COUNT - uBSFreeBlocks);
        chprintf(chp, "bytes  : %5u   %5u   %5u" SHELL_NEWLINE_STR,
                 UBS_SPACE_MAX, uBSFreeSpace, UBS_SPACE_MAX - uBSFreeSpace);
        break;
      case 'f':
        uBSFormat();
        uBSInit();
        chprintf(chp, "uBS formated and re-initialized." SHELL_NEWLINE_STR);
        break;
      default: goto ERROR;
        break;
    }
  } else goto ERROR;

  return;

  ERROR:
    shellUsage(chp, "ubs status|format|show <block>");
}
/*
 * Applet to show netif ip address and MAC
 */
static void cmd_net(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  chprintf(chp, "Hostname   : " OHS_NAME SHELL_NEWLINE_STR);
#if LWIP_MDNS_RESPONDER
  chprintf(chp, "mDNS       : http://" MDNS_HOSTANME ".local" SHELL_NEWLINE_STR);
#endif
  chprintf(chp, "IP address : %s" SHELL_NEWLINE_STR, ip4addr_ntoa((ip4_addr_t *)&netInfo.ip));
  chprintf(chp, "Netmask    : %s" SHELL_NEWLINE_STR, ip4addr_ntoa((ip4_addr_t *)&netInfo.mask));
  chprintf(chp, "Gateway    : %s" SHELL_NEWLINE_STR, ip4addr_ntoa((ip4_addr_t *)&netInfo.gw));
  chprintf(chp, "Flags      : 0x%04x" SHELL_NEWLINE_STR, netInfo.status); // As defined in lwip's netif.h
  chprintf(chp, "MAC        : %02x:%02x:%02x:%02x:%02x:%02x" SHELL_NEWLINE_STR,
           macAddr[0], macAddr[1], macAddr[2],
           macAddr[3], macAddr[4], macAddr[5]);
  chprintf(chp, "MQTT uid   : %s" SHELL_NEWLINE_STR, mqttHadUid);
}
/*
 * Applet to allow DFU upgrade
 */
static void cmd_boot(BaseSequentialStream *chp, int argc, char *argv[]) {

  if (argc == 1) {
    if (strcmp(argv[0], "dfu") == 0) {
      chprintf(chp, "Reboot DfuSe." SHELL_NEWLINE_STR);
      // Leave DFU breadcrumb which assmebly startup code would check
      *((unsigned long *)0x2002FFF0) = 0xDEADBEEF; // End of RAM F437
      //*((unsigned long *)0x2001FFF0) = 0xDEADBEEF; // End of RAM F407
      // Reset PHY
      palClearPad(GPIOE, GPIOE_ETH_RESET);
      chThdSleepMilliseconds(10);
      // And now reboot
      NVIC_SystemReset();
      return;
    }
  }

  // Rest is error
  shellUsage(chp, "boot dfu - reboot to DfuSe firmware upgrade mode.");
}
/*
 * Applet to reset admin
 */
static void cmd_reset(BaseSequentialStream *chp, int argc, char *argv[]) {

  if (argc == 1) {
    if (strcmp(argv[0], "pass") == 0) {
      strcpy(conf.user, "admin");
      strcpy(conf.password, "pass");
      chprintf(chp, "Admin user and pasword is reset." SHELL_NEWLINE_STR);
      return;
    }
  }

  // Rest is error
  shellUsage(chp, "reset pass - reset admin user to default");
}
/*
 * Applet to show threads with used information
 */
static void cmd_thread(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)argv;
  if (argc > 0) {
    shellUsage(chp, "threads");
    return;
  }

  static const char *states[] = {CH_STATE_NAMES};
  thread_t *tp;
  size_t counter = 0;
  size_t size;
  uint32_t used;

  chprintf(chp, "stklimit    stack  address   size   used    %% prio     state         name" SHELL_NEWLINE_STR SHELL_NEWLINE_STR);

  tp = chRegFirstThread();
  do {
     counter = 0;
#if (CH_DBG_ENABLE_STACK_CHECK == TRUE) || (CH_CFG_USE_DYNAMIC == TRUE)
    uint32_t stklimit = (uint32_t)tp->wabase;
#else
    uint32_t stklimit = 0U;
#endif

    uint8_t *begin = (uint8_t *)stklimit;
    uint8_t *end = (uint8_t *)tp->ctx.sp;

    size = end - begin;

    while(begin < end) {
      if(*begin++ != CH_DBG_STACK_FILL_VALUE) ++counter;
    }

    used = (counter * 100) / size;

    chprintf(chp, "%08lx %08lx %08lx %6u %6u %3u%% %4lu %9s %12s" SHELL_NEWLINE_STR,
             stklimit, (uint32_t)tp->ctx.sp, (uint32_t)tp,
             size, counter, used, (uint32_t)tp->realprio, states[tp->state],
             tp->name == NULL ? "" : tp->name);

    tp = chRegNextThread(tp);
  } while (tp != NULL);

  chprintf(chp, SHELL_NEWLINE_STR);
}
/*
 * Show MCU pin configuration and state
 */
#if SHELL_SHOW_PIN
void * gpio_ports[] = {GPIOA, GPIOB, GPIOC, GPIOD, GPIOE, GPIOF, GPIOG, GPIOH, GPIOI, GPIOJ};

void cmd_showpin (BaseSequentialStream *chp, int argc, char **argv) // Debug: pint hw pin mode.
{
  char *p;
  stm32_gpio_t *gpp;
  int bn, pn;
  static char *modes[] = {"In", "Out", "AF", "AIn"};
  static char *otypes[] = {"PP", "OD"};
  static char *ospeeds[] = {"Low", "Med", "Low", "Hi"};
  static char *pupd[] = {"float", "p-up", "p-dn", "res"};

  if (argc < 1) {
    shellUsage(chp, "showpin (A0 b1 ...) - enter pin(s).");
    return;
  }

  for(uint8_t i=0; i<argc; i++) {
    p = argv[i];
    if (*p == 'p') p++;
    if (*p == 'P') p++;

    pn = (*p - 'A') & 0xf; // cheap toLower
    bn = atoi (p+1);
    gpp = gpio_ports[pn];

    chprintf (chp, "P%c%d: ", 'A'+pn, bn);
    chprintf (chp, "Mode: %s", modes[(gpp->MODER >> (2*bn))&3]);
    if (((gpp->MODER >> (2*bn))&3) == 2) {
      if (bn<8) chprintf (chp, "%d, ", (gpp->AFRL >> (4* bn   ) ) &0xf);
      if (bn>7) chprintf (chp, "%d, ", (gpp->AFRH >> (4*(bn-8)) ) &0xf);
    } else {
      chprintf (chp, ", ", modes[(gpp->MODER >> (2*bn))&3]);
    }
    chprintf (chp, "Type: %s, ", otypes[(gpp->OTYPER >> bn)&1]);
    chprintf (chp, "Speed: %s, ", ospeeds[(gpp->OSPEEDR >> (2*bn))&3]);
    chprintf (chp, "Resistor: %s, ", pupd[(gpp->PUPDR >> (2*bn))&3]);
    chprintf (chp, "IDR: %d, ", (gpp->IDR >> (1*bn))&1);
    chprintf (chp, "ODR: %d.", (gpp->ODR >> (1*bn))&1);
    chprintf (chp, SHELL_NEWLINE_STR);
  }
}
#endif
/*
 * Applet to send commands to modem
 */
static void cmd_modem(BaseSequentialStream *chp, int argc, char *argv[]) {
  int8_t resp;
  uint8_t response[80];

  if (argc == 2) {
    if (strcmp(argv[0], "cmd") == 0) {
      if (chBSemWaitTimeout(&gprsSem, TIME_S2I(1)) == MSG_OK) {
        resp = gprsSendCmd(argv[1]);
        chBSemSignal(&gprsSem);
        chprintf(chp, "Result: %d" SHELL_NEWLINE_STR, resp);
      } else {
        chprintf(chp, "Modem busy." SHELL_NEWLINE_STR);
      }
      return;
    } else if (strcmp(argv[0], "cmdWR") == 0) {
      if (chBSemWaitTimeout(&gprsSem, TIME_S2I(1)) == MSG_OK) {
        resp = gprsSendCmdWR(argv[1], response, sizeof(response));
        chBSemSignal(&gprsSem);
        chprintf(chp, "Result: %d, Response: %s" SHELL_NEWLINE_STR, resp, response);
      } else {
        chprintf(chp, "Modem busy." SHELL_NEWLINE_STR);
      }
      return;
    }
  }

  // Usage
  shellUsage(chp, "modem cmd <AT command> | cmdWR <AT command>");
}
/*
 * Shell commands
 */
static const ShellCommand commands[] = {
  {"boot",  cmd_boot},
  {"date",  cmd_date},
  {"debug",  cmd_debug},
  {"log",  cmd_log},
  {"modem",  cmd_modem},
  {"network",  cmd_net},
  {"reset",  cmd_reset},
#if SHELL_SHOW_PIN
  {"showpin", cmd_showpin},
#endif
  {"thread",  cmd_thread},
  {"ubs",  cmd_ubs},
  {NULL, NULL}
};
/*
 * ShellConfig
 */
static const ShellConfig shell_cfg = {
  (BaseSequentialStream *)&SDU1,
  commands
};

#endif /* OHS_SHELL_H_ */
