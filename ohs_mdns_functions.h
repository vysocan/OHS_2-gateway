/*
 * ohs_mdns_functions.h
 *
 *  Created on: Oct 11, 2024
 *      Author: vysocan
 */

#ifndef OHS_MDNS_FUNCTIONS_H_
#define OHS_MDNS_FUNCTIONS_H_

/*
 * MDNS responder callback Service Status
 */
#if LWIP_MDNS_RESPONDER
static void mdnsSrvTxt(struct mdns_service *service, void *txt_userdata) {
  err_t resp;
  LWIP_UNUSED_ARG(txt_userdata);

  resp = mdns_resp_add_service_txtitem(service, "path=/", 6);
  if (resp != ERR_OK) {
    chprintf(console, "MDNS: Add service txt status %d.\r\n", resp);
  }
}
#endif
/*
 * MDNS responder callback Status Report
 */
#if LWIP_MDNS_RESPONDER
static void mdnsStatusReport(struct netif* netif, u8_t result, s8_t service) {
  (void)netif;
  (void)result;
  (void)service;

  //chprintf(console,"mdns status[netif %d][service %d]: %d\r\n", netif->num, service, result);
}
#endif

#endif /* OHS_MDNS_FUNCTIONS_H_ */
