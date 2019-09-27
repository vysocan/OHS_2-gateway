/*
 * ohs_shell.h
 *
 *  Created on: 19. 10. 2018
 *      Author: vysocan
 */

#ifndef OHS_SHELL_H_
#define OHS_SHELL_H_

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
  chprintf(chp, "     begin        end   size   used    %% prio     state         name\r\n");
  chprintf(chp, "--------------------------------------------------------------------\r\n");

  tp = chRegFirstThread();
  do {
     n = 0;
#if (CH_DBG_ENABLE_STACK_CHECK == TRUE) || (CH_CFG_USE_DYNAMIC == TRUE)
    uint32_t stklimit = (uint32_t)tp->wabase;
#else
    uint32_t stklimit = 0U;
#endif

    uint8_t *begin = (uint8_t *)stklimit;
    uint8_t *end = (uint8_t *)tp;
    sz = end - begin;

    while(begin < end)
       if(*begin++ == CH_DBG_STACK_FILL_VALUE) ++n;

    used_pct = (n * 100) / sz;

    chprintf(chp, "0x%08lx 0x%08lx %6u %6u %3u%% %4lu %9s %12s\r\n",
             stklimit, (uint32_t)tp, sz, n, used_pct, (uint32_t)tp->prio, states[tp->state], tp->name == NULL ? "" : tp->name);

    tp = chRegNextThread(tp);
  } while (tp != NULL);

  chprintf(chp, "\r\n");
}

const char text_System[]            = "System";
const char text_started[]           = "started";
const char text_Undefined[]         = "Undefined";
const char text_removed[]           = "removed";
const char text_disabled[]          = "disabled";
const char text_address[]           = "address";
const char text_registration[]      = "registration";
const char text_error[]             = "error";
const char text_registered[]        = "registered";
const char text_is[]                = "is";
const char text_Authentication[]    = "Authentication";
const char text_Sensor[]            = "Sensor";
const char text_Input[]             = "Output"; // :) Input on node is Output to GW
const char text_iButton[]           = "iButton";
const char text_Temperature[]       = "Temperature";
const char text_Humidity[]          = "Humidity";
const char text_Pressure[]          = "Pressure";
const char text_Voltage[]           = "Voltage";
const char text_Battery[]           = "Battery";
const char text_Digital[]           = "Digital";
const char text_Analog[]            = "Analog";
const char text_Float[]             = "Float";
const char text_TX_Power[]          = "TX_Power";
const char text_Gas[]               = "Gas";
const char text_not[]               = "not";
const char text_strength[]          = "strength";
const char text_unknown[]           = "unknown";
const char text_roaming[]           = "roaming";
const char text_searching[]         = "searching";
const char text_network[]           = "network";
const char text_denied[]            = "denied";
const char text_cosp[]              = ", ";
const char text_Modem[]             = "Modem";
const char text_On[]                = "On";
const char text_Off[]               = "Off";
const char text_power[]             = "power";


void printNodeFunction(char *text, char function) {
  switch(function){
    case 'K': strcat(text, text_Authentication); break;
    case 'S': strcat(text, text_Sensor); break;
    case 'I': strcat(text, text_Input); break;
    default: strcat(text, text_Undefined); break;
  }
}

void printNodeType(char *text, char type) {
  switch(type){
    case 'i': strcat(text, text_iButton); break;
    case 'T': strcat(text, text_Temperature); break;
    case 'H': strcat(text, text_Humidity); break;
    case 'P': strcat(text, text_Pressure); break;
    case 'V': strcat(text, text_Voltage); break;
    case 'B': strcat(text, text_Battery); break;
    case 'D': strcat(text, text_Digital); break;
    case 'A': strcat(text, text_Analog); break;
    case 'F': strcat(text, text_Float); break;
    case 'X': strcat(text, text_TX_Power); break;
    case 'G': strcat(text, text_Gas); break;
    default : strcat(text, text_Undefined); break;
  }
}


static void decodeLog(char *in, char *out){
  out[0] = 0;
  switch(in[0]){
    case 'S': // System
      strcat(out, text_System); strcat(out, " ");
      switch(in[1]){
        case 'S': strcat(out, text_started); break;   // boot
        default:  strcat(out, text_Undefined); break; // unknown
      }
    break;
    case 'N': // remote nodes
      printNodeFunction(out, in[4]); strcat(out, ":");
      printNodeType(out, in[5]);
      strcat(out, " "); strcat(out, text_address); strcat(out, " ");
      //if ((uint8_t)in[2] < RADIO_UNIT_OFFSET) { strcat(out, "W:"); strcat(out, (uint8_t)in[2]); }
      //else                                    { strcat(out, "R:"); strcat(out, (uint8_t)in[2]-RADIO_UNIT_OFFSET); }
      //strcat(out, ":"); strcat(out, (uint8_t)in[3]); strcat(out, " ");
      if (in[1] != 'E') {strcat(out, text_is); strcat(out, " ");}
      switch(in[1]){
        case 'F' : strcat(out,text_disabled); break;
        case 'R' : strcat(out,text_registered); break;
        case 'r' : strcat(out,text_removed); break;
        default : strcat(out, text_registration); strcat(out, " "); strcat(out, text_error); break;
      }
    break;
    case 'M': // Modem
      strcat(out, text_Modem); strcat(out, " ");
      if((in[1]) >= '0' && (in[1]) <= '5') { strcat(out, text_network); strcat(out, " "); }
      switch(in[1]){
        case '0' : strcat(out, text_not); strcat(out, " "); strcat(out, text_registered); break;
        case '1' : strcat(out, text_registered); break;
        case '2' : strcat(out, text_searching); break;
        case '3' : strcat(out, text_registration); strcat(out, " "); strcat(out, text_denied); break;
        // case 4 : strcat(out, text_unk); break;
        case '5' : strcat(out, text_roaming); break;
        case 'O' : strcat(out, text_power); strcat(out, " "); strcat(out, text_On); break;
        case 'F' : strcat(out, text_power); strcat(out, " "); strcat(out, text_Off); break;
        default : strcat(out, text_unknown); break;
      }
      if((in[1]) >= '0' && (in[1]) <= '5') {
        strcat(out, text_cosp); strcat(out, text_strength); strcat(out, " ");
        // server << (uint8_t)value[7] << '%';
      }
    break;

    default: strcat(out, text_Undefined);
    //for(uint16_t ii = 0; ii < LOGGER_MSG_LENGTH; ii++) {
    //  chprintf(chp, "%x %c-", rxBuffer[ii+4], rxBuffer[ii+4]);
    //}
    break; // unknown

    strcat(out, "."); // End
  }

}

/*
 * Console applet to show log entries
 */
#define LOGGER_OUTPUT_LEN 15
static void cmd_log(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)argv;

  char   rxBuffer[FRAM_MSG_SIZE];
  char   txBuffer[3];
  struct tm *ptm;
  char   dateTime[30];

  if (argc > 1)  { goto ERROR; }
  if (argc == 1) { FRAMReadPos = (atoi(argv[0]) - LOGGER_OUTPUT_LEN + 1) * FRAM_MSG_SIZE; }
  if (argc == 0) { FRAMReadPos = FRAMWritePos - (FRAM_MSG_SIZE * LOGGER_OUTPUT_LEN); }

  for(uint16_t i = 0; i < LOGGER_OUTPUT_LEN; i++) {
    txBuffer[0] = CMD_25AA_READ;
    txBuffer[1] = (FRAMReadPos >> 8) & 0xFF;
    txBuffer[2] = FRAMReadPos & 0xFF;

    spiAcquireBus(&SPID1);              // Acquire ownership of the bus.
    spiSelect(&SPID1);                  // Slave Select assertion.
    spiSend(&SPID1, 3, txBuffer);       // Send read command
    spiReceive(&SPID1, FRAM_MSG_SIZE, rxBuffer);
    spiUnselect(&SPID1);                // Slave Select de-assertion.
    spiReleaseBus(&SPID1);              // Ownership release.

    memcpy(&timeConv.ch[0], &rxBuffer[0], sizeof(timeConv.ch));
    ptm = gmtime(&timeConv.val);
    strftime(dateTime, 30, conf.dateTimeFormat, ptm); // Format date time as needed

    decodeLog(&rxBuffer[4], logText);

    chprintf(chp, "#%d\t%s Text: %s", (FRAMReadPos/FRAM_MSG_SIZE), dateTime, logText);
    chprintf(chp, " Flags: %x\r\n", rxBuffer[FRAM_MSG_SIZE-1]);
    chThdSleepMilliseconds(2);

    FRAMReadPos+=FRAM_MSG_SIZE; // Advance for next read
  }
  return;

ERROR:
  chprintf(chp, "Usage: log\r\n");
  chprintf(chp, "       log N - where N is log entry  start point\r\n");
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
    unix_time = GetTimeUnixSec();

    if (unix_time == -1){
      chprintf(chp, "Incorrect time in RTC cell.\r\n");
    }
    else{
      ptm = gmtime(&unix_time);
      strftime(dateTime, 30, conf.dateTimeFormat, ptm); // Format date time as needed
      chprintf(chp, "Current: %d\t", unix_time);
      chprintf(chp, "%s\r\n", dateTime);
    }
    return;
  }

  if ((argc == 2) && (strcmp(argv[0], "set") == 0)){
    unix_time = atol(argv[1]);
    if (unix_time > 0){
      SetTimeUnixSec(unix_time);
      return;
    }
    else{
      goto ERROR;
    }
  }
  else{
    goto ERROR;
  }

ERROR:
  chprintf(chp, "Usage: date\r\n");
  chprintf(chp, "       date set N\r\n");
  chprintf(chp, "where N is time in seconds sins Unix epoch\r\n");
  chprintf(chp, "you can get current N value from unix console by the command:\r\n");
  chprintf(chp, "%s", "date +\%s\r\n");
  return;
}

// Routing console to USB
static void cmd_debug(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)argv;

  if (argc != 1) {
    chprintf(chp, "Usage: debug on/off\r\n");
    return;
  }
  // Reroute all console to USB
  if ((argc == 1) && (strcmp(argv[0], "on") == 0)){
    console = (BaseSequentialStream*)&SDU1;
  }

  // Reroute all console back to SD3
  if ((argc == 1) && (strcmp(argv[0], "off") == 0)){
    console = (BaseSequentialStream*)&SD3;
    chprintf(chp, "\r\nstopped\r\n");
  }
}

/*
 *
 */
static const ShellCommand commands[] = {
  {"date",  cmd_date},
  {"log",  cmd_log},
  {"threads",  cmd_threads},
  {"debug",  cmd_debug},
  {NULL, NULL}
};

/*
 *
 */
/*static const ShellConfig shell_cfg1 = {
  (BaseSequentialStream  *)&SD3,
  commands
};*/

static const ShellConfig shell_cfg1 = {
  (BaseSequentialStream *)&SDU1,
  commands
};

/*
 * working area for shell thread
 */
#define SHELL_WA_SIZE   THD_WORKING_AREA_SIZE(2048)
//static THD_WORKING_AREA(waShell, 2048);


#endif /* OHS_SHELL_H_ */
