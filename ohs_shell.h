/*
 * ohs_shell.h
 *
 *  Created on: 19. 10. 2018
 *      Author: vysocan
 */

#ifndef OHS_SHELL_H_
#define OHS_SHELL_H_

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
      FRAMReadPos = (strtoul(argv[0], NULL, 0) - LOGGER_OUTPUT_LEN) * FRAM_MSG_SIZE;
    }
  }
  if (argc == 0) { FRAMReadPos = FRAMWritePos - (FRAM_MSG_SIZE * LOGGER_OUTPUT_LEN); }

  spiAcquireBus(&SPID1);                // Acquire ownership of the bus.
  for(uint16_t i = 0; i < LOGGER_OUTPUT_LEN; i++) {
    txBuffer[0] = CMD_25AA_READ;
    txBuffer[1] = 0;
    txBuffer[2] = (FRAMReadPos >> 8) & 0xFF;
    txBuffer[3] = FRAMReadPos & 0xFF;

    spiSelect(&SPID1);                  // Slave Select assertion.
    spiSend(&SPID1, FRAM_HEADER_SIZE, txBuffer); // Send read command
    spiReceive(&SPID1, FRAM_MSG_SIZE, rxBuffer);
    spiUnselect(&SPID1);                // Slave Select de-assertion.

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
    chprintf(chp, "\r\n");

    FRAMReadPos += FRAM_MSG_SIZE;       // Advance for next read
  }
  spiReleaseBus(&SPID1);                // Ownership release.
  // Show usage if no aguments
  if (argc == 0) {
    chprintf(chp, "\r\n");
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
  chprintf(chp, "Log formated.\r\n");
  return;

ERROR:
  chprintf(chp, "Usage: log - show this, current log entry.\r\n");
  chprintf(chp, "       log N - where N is log last entry number(1 - 4096).\r\n");
  chprintf(chp, "       log format - erase all log entries.\r\n");
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
      chprintf(chp, "Incorrect time in RTC cell.\r\n");
    }
    else{
      ptm = gmtime(&unix_time);
      strftime(dateTime, 30, conf.dateTimeFormat, ptm); // Format date time as needed
      chprintf(chp, "Current: %d ", unix_time);
      chprintf(chp, "%s, ", durationSelect[0]);
      chprintf(chp, "%s\r\n", dateTime);
    }
    return;
  }

  if ((argc == 2) && (strcmp(argv[0], "set") == 0)){
    unix_time = strtol(argv[1], NULL, 0);
    // 315532800 = 1.1.1980 00:00:00
    if (unix_time > 315532800){
      convertUnixSecondToRTCDateTime(&timespec, unix_time);
      rtcSetTime(&RTCD1, &timespec);
      chprintf(chp, "Date set.\r\n");
      return;
    }
  }

  // Rest is error
  chprintf(chp, "Usage: date\r\n");
  chprintf(chp, "       date set N - where N is time in seconds since Unix epoch, and greater then 1.1.1980(315532801).\r\n");
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
      chprintf(chp, "\r\nDebug ON.\r\n");
      return;
    } else if (strcmp(argv[0], "off") == 0) {
      // Reroute all console back to SD3
      console = (BaseSequentialStream*)&SD3;
      chprintf(chp, "\r\nDebug OFF.\r\n");
      return;
    }
  }
  // Rest is error
  shellUsage(chp, "debug on|off");
}
/*
 * Applet to uBS
 */
static void cmd_ubs(BaseSequentialStream *chp, int argc, char *argv[]) {

  if (argc == 1) {
    switch (argv[0][0]) {
      case 's':
        chprintf(chp, "uBS      total    free    used\r\n");
        chprintf(chp, "------------------------------\r\n");
        chprintf(chp, "blocks : %5u   %5u   %5u\r\n",
                 UBS_BLOCK_COUNT, uBSFreeBlocks, UBS_BLOCK_COUNT - uBSFreeBlocks);
        chprintf(chp, "bytes  : %5u   %5u   %5u\r\n",
                 UBS_SPACE_MAX, uBSFreeSpace, UBS_SPACE_MAX - uBSFreeSpace);
        break;
      case 'f':
        uBSFormat();
        uBSInit();
        chprintf(chp, "uBS formated and re-initialized.\r\n");
        break;
      default: goto ERROR;
        break;
    }
  } else goto ERROR;

  return;

  ERROR:
    shellUsage(chp, "ubs status|format");
}
/*
 * Applet to show netif ip address and MAC
 */
static void cmd_net(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  //chprintf(chp, "Network:\r\n-------------------------------\r\n");
  chprintf(chp, "IP address : %u.%u.%u.%u\r\n",
           netInfo.ip & 0xff, (netInfo.ip >> 8) & 0xff,
           (netInfo.ip >> 16) & 0xff, (netInfo.ip >> 24) & 0xff);
  chprintf(chp, "Netmask    : %u.%u.%u.%u\r\n",
           netInfo.mask & 0xff, (netInfo.mask >> 8) & 0xff,
           (netInfo.mask >> 16) & 0xff, (netInfo.mask >> 24) & 0xff);
  chprintf(chp, "Gateway    : %u.%u.%u.%u\r\n",
           netInfo.gw & 0xff, (netInfo.gw >> 8) & 0xff,
           (netInfo.gw >> 16) & 0xff, (netInfo.gw >> 24) & 0xff);
  chprintf(chp, "Flags      : %u\r\n", netInfo.status);
  chprintf(chp, "MAC        : %02x:%02x:%02x:%02x:%02x:%02x\r\n",
           macAddr[0], macAddr[1], macAddr[2],
           macAddr[3], macAddr[4], macAddr[5]);
}
/*
 * Applet to allow DFU upgrade
 */
static void cmd_boot(BaseSequentialStream *chp, int argc, char *argv[]) {

  if (argc == 1) {
    if (strcmp(argv[0], "dfu") == 0) {
      chprintf(chp, "Reboot DfuSe.\r\n");
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
 * Applet to show threads with used information
 */
/*
static void cmd_threads(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)argv;
  if (argc > 0) {
    shellUsage(chp, "threads");
    return;
  }

  static const char *states[] = {CH_STATE_NAMES};
  thread_t *tp;
  size_t n = 0;
  size_t sz;
  uint32_t used_pct;

  chprintf(chp, "\r\n");
  chprintf(chp, "   begin      end   size   used    %% prio     state         name\r\n");
  chprintf(chp, "--------------------------------------------------------------\r\n");

  tp = chRegFirstThread();
  do {
     n = 0;
#if (CH_DBG_ENABLE_STACK_CHECK == TRUE) || (CH_CFG_USE_DYNAMIC == TRUE)
    uint32_t stklimit = (uint32_t)tp->wabase;
#else
    uint32_t stklimit = 0U;
#endif

    uint8_t *begin = (uint8_t *)stklimit;
    //uint8_t *begin = (uint32_t)tp->ctx.sp;
    uint8_t *end = (uint8_t *)tp;
    sz = end - begin;

    while(begin < end) {
      if(*begin++ == CH_DBG_STACK_FILL_VALUE) ++n;
    }

    used_pct = (n * 100) / sz;

    chprintf(chp, "%08lx %08lx %6u %6u %3u%% %4lu %9s %12s\r\n",
             stklimit, (uint32_t)tp, sz, n, used_pct, (uint32_t)tp->prio, states[tp->state], tp->name == NULL ? "" : tp->name);

    tp = chRegNextThread(tp);
  } while (tp != NULL);

  chprintf(chp, "\r\n");
}
*/
/*
 * Shell commands
 */
static const ShellCommand commands[] = {
  {"date",  cmd_date},
  {"log",  cmd_log},
  //{"mythreads",  cmd_threads},
  {"debug",  cmd_debug},
  {"ubs",  cmd_ubs},
  {"network",  cmd_net},
  {"boot",  cmd_boot},
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
