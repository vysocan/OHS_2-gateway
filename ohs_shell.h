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
  // TODO OHS Implement format of logger
  if (argc == 1) {
    /*
    if (strcmp(argv[0], "format") == 0) {
      goto FORMAT;
    } else {
    */
      FRAMReadPos = (atoi(argv[0]) - LOGGER_OUTPUT_LEN + 1) * FRAM_MSG_SIZE;
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

    chprintf(chp, "#%4u : ", (FRAMReadPos/FRAM_MSG_SIZE));
    printFrmTimestamp(chp, &timeConv.val);
    chprintf(chp, " : %s.", logText);
    chprintf(chp, " : Flags: ");
    for (uint8_t j = 0; j < ARRAY_SIZE(alertType); j++) {
      if ((((uint8_t)rxBuffer[FRAM_MSG_SIZE-1] >> j) & 0b1) == 1)
        chprintf(chp, "%s, ", alertType[(uint8_t)rxBuffer[FRAM_MSG_SIZE-1]].name);
    }
    chprintf(chp, "\r\n");

    FRAMReadPos += FRAM_MSG_SIZE;       // Advance for next read
  }
  spiReleaseBus(&SPID1);                // Ownership release.
  return;

  /*
FORMAT:

  char buffer[FRAM_MSG_SIZE + FRAM_HEADER_SIZE];
  // SPI
  spiAcquireBus(&SPID1);

  spiSelect(&SPID1);
  buffer[0] = CMD_25AA_WREN;
  spiSend(&SPID1, 1, buffer);
  spiUnselect(&SPID1);

  buffer[0] = CMD_25AA_WRITE;
  buffer[1] = 0;
  buffer[2] = 0;
  buffer[3] = 0;

  memset(&buffer[FRAM_HEADER_SIZE], 0xff, FRAM_MSG_SIZE);

  spiSelect(&SPID1);
  spiSend(&SPID1, FRAM_MSG_SIZE + FRAM_HEADER_SIZE, buffer);
  spiUnselect(&SPID1);
  spiReleaseBus(&SPID1);
*/
  return;

ERROR:
  shellUsage(chp, "log\r\n       log N - where N is log last entry point");
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
    unix_time = atol(argv[1]);
    if (unix_time > 0){
      convertUnixSecondToRTCDateTime(&timespec, unix_time);
      rtcSetTime(&RTCD1, &timespec);
      return;
    }
  }

  // Rest is error
  chprintf(chp, "Usage: date\r\n");
  chprintf(chp, "       date set N\r\n");
  chprintf(chp, "where N is time in seconds since Unix epoch\r\n");
  chprintf(chp, "You can get current N value from unix console by the command:\r\n");
  chprintf(chp, "%s", "date +\%s\r\n");
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
  (void)argv;

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
 * Applet to show threads
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
