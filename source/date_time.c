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
