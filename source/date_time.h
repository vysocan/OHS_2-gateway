/*
 * date_time.h
 *
 *  Created on: 13. 10. 2020
 *      Author: adam
 */

#ifndef SOURCE_DATE_TIME_H__
#define SOURCE_DATE_TIME_H__

#include <stdint.h>
#include "ch.h"
#include "hal.h"

// Time related
#define SECONDS_PER_DAY     86400U
#define SECONDS_PER_HOUR    3600U
#define SECONDS_PER_MINUTE  60U
#define MINUTES_PER_HOUR    60U
#define RTC_LEAP_YEAR(year) ((((year) % 4 == 0) && ((year) % 100 != 0)) || ((year) % 400 == 0))
#define RTC_DAYS_IN_YEAR(x) RTC_LEAP_YEAR(x) ? 366 : 365
#define RTC_OFFSET_YEAR     1970

//extern volatile uint32_t RTCTimestamp;
extern volatile int32_t  RTCDeviation;

uint32_t calculateDST(uint16_t year, uint8_t month, uint8_t week, uint8_t dow, uint8_t hour);
uint32_t convertRTCDateTimeToUnixSecond(RTCDateTime *dateTime);
void convertUnixSecondToRTCDateTime(RTCDateTime* dateTime, uint32_t unixSeconds);

#endif /* SOURCE_DATE_TIME_H__ */
