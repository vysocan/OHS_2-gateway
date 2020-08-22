/*
 * ohs_th_heartbeat.h
 *
 *  Created on: 18. 4. 2020
 *      Author: adam
 */

#ifndef OHS_TH_HEARTBEAT_H_
#define OHS_TH_HEARTBEAT_H_

/*
 * This is a periodic thread that does absolutely nothing except flashing
 * a LED.
 */
static THD_WORKING_AREA(waHeartBeatThread, 64);
static THD_FUNCTION(HeartBeatThread, arg) {
  chRegSetThreadName(arg);
  systime_t time;

  while (true) {
    time = serusbcfg.usbp->state == USB_ACTIVE ? 100 : 500;

    palSetPad(GPIOC, GPIOC_HEARTBEAT);
    chThdSleepMilliseconds(time);
    palClearPad(GPIOC, GPIOC_HEARTBEAT);
    chThdSleepMilliseconds(500);
  }
}

#endif /* OHS_TH_HEARTBEAT_H_ */
