/*
 * date_time.c
 *
 *  Created on: 13. 10. 2020
 *      Author: adam
 */
#include "date_time.h"
#include "chprintf.h"

// SNTP timestamp to compare
//volatile uint32_t RTCTimestamp = 0;
//volatile int32_t  RTCDeviation = 0;

/*
 * Days in a month
 */
static const uint8_t TM_RTC_Months[2][12] = {
    {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},   /* Not leap year */
    {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}    /* Leap year */
};
/*
 * Convert RTCDateTime to seconds NOT using tm (ISO C `broken-down time' structure.)
 */
uint32_t convertRTCDateTimeToUnixSecond(RTCDateTime *dateTime) {
  uint32_t days = 0, seconds = 0;
  uint16_t i;
  uint16_t year = (uint16_t) (dateTime->year + 1980);

  // Year is below offset year
  if (year < RTC_OFFSET_YEAR) return 0;
  // Days in previus years
  for (i = RTC_OFFSET_YEAR; i < year; i++) {
    days += RTC_DAYS_IN_YEAR(i);
  }
  // Days in current year
  for (i = 1; i < dateTime->month; i++) {
    days += TM_RTC_Months[RTC_LEAP_YEAR(year)][i - 1];
  }
  // Day starts with 1
  days += dateTime->day - 1;
  seconds = days * SECONDS_PER_DAY;
  seconds += dateTime->millisecond / 1000;

  return seconds;
}
/*
 * Get Daylight Saving Time for particular rule
 *
 * @parm year - 1900
 * @parm month - 1=Jan, 2=Feb, ... 12=Dec
 * @parm week - 1=First, 2=Second, 3=Third, 4=Fourth, or 0=Last week of the month
 * @parm dow, Day of week - 0=Sun, 1=Mon, ... 6=Sat
 * @parm hour - 0 - 23
 * @retval seconds
 */
uint32_t calculateDST(uint16_t year, uint8_t month, uint8_t week, uint8_t dow, uint8_t hour){
  RTCDateTime dstDateTime;
  time_t rawtime;
  uint8_t _week = week; // Local copy

  if (_week == 0) {      //Last week = 0
    if (month++ > 12) {  //for "Last", go to the next month
      month = 1;
      year++;
    }
    _week = 1;            //and treat as first week of next month, subtract 7 days later
  }
  // First day of the month, or first day of next month for "Last" rules
  dstDateTime.year = year;
  dstDateTime.month = month ;
  dstDateTime.day = 1;
  dstDateTime.millisecond = hour * SECONDS_PER_HOUR * 1000;

  // Do DST
  rawtime = convertRTCDateTimeToUnixSecond(&dstDateTime);
  //chprintf(console, "DST: %d\r\n", rawtime);

  // Weekday function by Michael Keith and Tom Craver
  year += 1980;
  uint8_t weekday = (dstDateTime.day += month < 3 ?
      year-- : year - 2, 23 * month / 9 + dstDateTime.day + 4 + year/4 - year/100 + year/400)%7;
  //chprintf(console, "DST: %d\r\n", weekday);

  rawtime += ((7 * (_week - 1) + (dow - weekday + 7) % 7) * SECONDS_PER_DAY);
  //chprintf(console, "DST: %d\r\n", rawtime);

  //back up a week if this is a "Last" rule
  if (week == 0) {
    rawtime = rawtime - (7 * SECONDS_PER_DAY);
  }

  return rawtime;
}
/*
 * Convert seconds to RTCDateTime NOT using tm (ISO C `broken-down time' structure.)
 */
void convertUnixSecondToRTCDateTime(RTCDateTime* dateTime, uint32_t unixSeconds) {
  uint16_t year;

  // Get milliseconds
  dateTime->millisecond = (unixSeconds % SECONDS_PER_DAY) * 1000;
  unixSeconds /= SECONDS_PER_DAY;
  // Get week day, Monday is day one
  dateTime->dayofweek = (unixSeconds + 3) % 7 + 1;
  // Get year
  year = 1970;
  while (true) {
    if (RTC_LEAP_YEAR(year)) {
      if (unixSeconds >= 366) {
        unixSeconds -= 366;
      } else {
        break;
      }
    } else if (unixSeconds >= 365) {
      unixSeconds -= 365;
    } else {
      break;
    }
    year++;
  }
  // Get year in xx format
  dateTime->year = year - 1980;
  // Get month
  for (dateTime->month = 0; dateTime->month < 12; dateTime->month++) {
    if (RTC_LEAP_YEAR(year)) {
      if (unixSeconds >= (uint32_t)TM_RTC_Months[1][dateTime->month]) {
        unixSeconds -= TM_RTC_Months[1][dateTime->month];
      } else {
        break;
      }
    } else if (unixSeconds >= (uint32_t)TM_RTC_Months[0][dateTime->month]) {
      unixSeconds -= TM_RTC_Months[0][dateTime->month];
    } else {
      break;
    }
  }
  // Month starts with 1
  dateTime->month++;
  // Get day, day starts with 1
  dateTime->day = unixSeconds + 1;
}
#define RECALPF_TIMEOUT                 ((uint32_t) 0x00001000)
#define RTC_SmoothCalibPeriod_32sec     ((uint8_t)0b00) /*!<  if RTCCLK = 32768 Hz, Smooth calibation
                                                             period is 32s,  else 2exp20 RTCCLK seconds */
#define RTC_SmoothCalibPeriod_16sec     ((uint8_t)0b01) /*!<  if RTCCLK = 32768 Hz, Smooth calibation
                                                             period is 16s, else 2exp19 RTCCLK seconds */
#define RTC_SmoothCalibPeriod_8sec      ((uint8_t)0b10) /*!<  if RTCCLK = 32768 Hz, Smooth calibation
                                                             period is 8s, else 2exp18 RTCCLK seconds */
#define RTC_SmoothCalibPlusPulses_Set   ((uint8_t)0b1) /*!<  The number of RTCCLK pulses added
                                                                during a X -second window = Y - CALM[8:0].
                                                                 with Y = 512, 256, 128 when X = 32, 16, 8 */
#define RTC_SmoothCalibPlusPulses_Reset ((uint8_t)0b0) /*!<  The number of RTCCLK pulses subbstited
                                                                 during a 32-second window =   CALM[8:0]. */
#define IS_RTC_SMOOTH_CALIB_PERIOD(PERIOD) (((PERIOD) == RTC_SmoothCalibPeriod_32sec) || \
                                            ((PERIOD) == RTC_SmoothCalibPeriod_16sec) || \
                                            ((PERIOD) == RTC_SmoothCalibPeriod_8sec))
#define IS_RTC_SMOOTH_CALIB_PLUS(PLUS)     (((PLUS) == RTC_SmoothCalibPlusPulses_Set) || \
                                            ((PLUS) == RTC_SmoothCalibPlusPulses_Reset))
#define IS_RTC_SMOOTH_CALIB_MINUS(VALUE)   ((VALUE) <= 0x000001FF)
#define RTC_CALP (15U)
#define RTC_CALW (13U)
/*
 * @brief  Configures the Smooth Calibration Settings.
 * @param  RTC_SmoothCalibPeriod : Select the Smooth Calibration Period.
 *   This parameter can be can be one of the following values:
 *     @arg RTC_SmoothCalibPeriod_32sec : The smooth calibration periode is 32s.
 *     @arg RTC_SmoothCalibPeriod_16sec : The smooth calibration periode is 16s.
 *     @arg RTC_SmoothCalibPeriod_8sec  : The smooth calibartion periode is 8s.
 * @param  RTC_SmoothCalibPlusPulses : Select to Set or reset the CALP bit.
 *   This parameter can be one of the following values:
 *     @arg RTC_SmoothCalibPlusPulses_Set  : Add one RTCCLK puls every 2**11 pulses.
 *     @arg RTC_SmoothCalibPlusPulses_Reset: No RTCCLK pulses are added.
 * @param  RTC_SmouthCalibMinusPulsesValue: Select the value of CALM[8:0] bits.
 *   This parameter can be one any value from 0 to 0x000001FF.
 * @retval An ErrorStatus enumeration value:
 *          - SUCCESS: RTC Calib registers are configured
 *          - ERROR: RTC Calib registers are not configured
 */
uint8_t RTC_SmoothCalibConfig(uint8_t RTC_SmoothCalibPeriod,
                              uint8_t RTC_SmoothCalibPlusPulses,
                              uint16_t RTC_SmouthCalibMinusPulsesValue) {
  uint8_t status = 0;
  uint32_t recalpfcount = 0;

  /* Check the parameters */
  chDbgCheck(IS_RTC_SMOOTH_CALIB_PERIOD(RTC_SmoothCalibPeriod));
  chDbgCheck(IS_RTC_SMOOTH_CALIB_PLUS(RTC_SmoothCalibPlusPulses));
  chDbgCheck(IS_RTC_SMOOTH_CALIB_MINUS(RTC_SmouthCalibMinusPulsesValue));

  /* Disable the write protection for RTC registers */
  RTC->WPR = 0xCA;
  RTC->WPR = 0x53;

  /* check if a calibration is pending*/
  if ((RTC->ISR & RTC_ISR_RECALPF) != RESET) {
    /* wait until the Calibration is completed*/
    while (((RTC->ISR & RTC_ISR_RECALPF) != RESET) && (recalpfcount != RECALPF_TIMEOUT)) {
      recalpfcount++;
    }
  }

  /* check if the calibration pending is completed or if there is no calibration operation at all*/
  if ((RTC->ISR & RTC_ISR_RECALPF) == RESET) {
    /* Configure the Smooth calibration settings */
    RTC->CALR = (uint32_t)((RTC_SmoothCalibPeriod << RTC_CALW)
              | (uint32_t)(RTC_SmoothCalibPlusPulses << RTC_CALP)
              | (uint32_t)RTC_SmouthCalibMinusPulsesValue);
    status = 1;
  } else {
    status = 0;
  }

  chprintf((BaseSequentialStream *)&SD3, "RTC->CALR: %d\n\r", RTC->CALR);

  /* Enable the write protection for RTC registers */
  RTC->WPR = 0xFF;

  return status;
}
