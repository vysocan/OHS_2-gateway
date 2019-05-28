/*
 * rs485.c
 *
 *  Created on: 4. 11. 2018
 *      Author: vysocan
 */

#include "rs485.h"

static uint8_t rs485_in[RS485_PACKET_SIZE];
static uint8_t rs485_out[RS485_PACKET_SIZE];

/**
 * @brief   USART initialization.
 * @details This function must be invoked with interrupts disabled.
 *
 * @param[in] rs485p       pointer to a @p RS485Driver object
 * @param[in] config    the architecture-dependent serial driver configuration
 */
static void usart_init(RS485Driver *rs485p, const SerialConfig *config) {
  USART_TypeDef *u = rs485p->usart;

  /* Baud rate setting.*/
#if STM32_HAS_USART6
  if ((rs485p->usart == USART1) || (rs485p->usart == USART6))
#else
  if (rs485p->usart == USART1)
#endif
    u->BRR = STM32_PCLK2 / config->speed;
  else
    u->BRR = STM32_PCLK1 / config->speed;

  /* Note that some bits are enforced.*/
  u->CR2 = config->cr2 | USART_CR2_LBDIE;
  u->CR3 = config->cr3 | USART_CR3_EIE;
  u->CR1 = (config->cr1 | USART_CR1_UE | USART_CR1_PEIE |
                         USART_CR1_RXNEIE | USART_CR1_TE |
                         USART_CR1_RE) & ~USART_CR1_TCIE;
  u->SR = 0;
  (void)u->SR;  /* SR reset step 1.*/
  (void)u->DR;  /* SR reset step 2.*/

  /* Deciding mask to be applied on the data register on receive, this is
     required in order to mask out the parity bit.*/
  if ((config->cr1 & (USART_CR1_M | USART_CR1_PCE)) == USART_CR1_PCE) {
    rs485p->rxmask = 0x7F;
  }
  else {
    rs485p->rxmask = 0xFF;
  }
}

/**
 * @brief   USART de-initialization.
 * @details This function must be invoked with interrupts disabled.
 *
 * @param[in] u         pointer to an USART I/O block
 */
static void usart_deinit(USART_TypeDef *u) {

  u->CR1 = 0;
  u->CR2 = 0;
  u->CR3 = 0;
}


/**
 * @brief   Error handling routine.
 *
 * @param[in] rs485p       pointer to a @p RS485Driver object
 * @param[in] sr        USART SR register value
 */
static void set_error(RS485Driver *rs485p, uint16_t sr) {
  eventflags_t sts = 0;

  if (sr & USART_SR_ORE)
    sts |= SD_OVERRUN_ERROR;
  if (sr & USART_SR_PE)
    sts |= SD_PARITY_ERROR;
  if (sr & USART_SR_FE)
    sts |= SD_FRAMING_ERROR;
  if (sr & USART_SR_NE)
    sts |= SD_NOISE_ERROR;
  chnAddFlagsI(rs485p, sts);
}

/**
 * @brief   Common IRQ handler.
 *
 * @param[in] rs485p       communication channel associated to the USART
 */
static void serve_interrupt(RS485Driver *rs485p) {
  USART_TypeDef *u = rs485p->usart;
  uint16_t cr1 = u->CR1;
  uint16_t sr = u->SR;

  /* Special case, LIN break detection.*/
  if (sr & USART_SR_LBD) {
    osalSysLockFromISR();
    chnAddFlagsI(rs485p, SD_BREAK_DETECTED);
    u->SR = ~USART_SR_LBD;
    osalSysUnlockFromISR();
  }

  /* Data available.*/
  osalSysLockFromISR();
  while (sr & (USART_SR_RXNE | USART_SR_ORE | USART_SR_NE | USART_SR_FE |
               USART_SR_PE)) {
    uint8_t b;

    /* Error condition detection.*/
    if (sr & (USART_SR_ORE | USART_SR_NE | USART_SR_FE  | USART_SR_PE))
      set_error(rs485p, sr);
    b = (uint8_t)u->DR & rs485p->rxmask;
    if (sr & USART_SR_RXNE)
    {
      if (rs485p->config->rxchar_cb != NULL)
      {
          osalSysUnlockFromISR();                     // Do this for compatibility with V2 driver
          rs485p->config->rxchar_cb(rs485p, b);             // Receive character callback
          osalSysLockFromISR();
      }
      else
      {
        sdIncomingDataI(rs485p, b);
      }
      sr = u->SR;
    }
  }
  osalSysUnlockFromISR();


  /* Transmission buffer empty.*/
  if ((cr1 & USART_CR1_TXEIE) && (sr & USART_SR_TXE)) {
    msg_t b;
    osalSysLockFromISR();
    b = oqGetI(&rs485p->oqueue);
    if (b < MSG_OK) {
      chnAddFlagsI(rs485p, CHN_OUTPUT_EMPTY);
      u->CR1 = (cr1 & ~USART_CR1_TXEIE) | USART_CR1_TCIE;
      if (rs485p->config->txend1_cb != NULL)
        rs485p->config->txend1_cb(rs485p);            // Signal that Tx buffer finished with
    }
    else
      u->DR = b;
    osalSysUnlockFromISR();
  }

  /* Physical transmission end.*/
  if (sr & USART_SR_TC) {
    u->CR1 = cr1 & ~USART_CR1_TCIE;
    u->SR = ~USART_SR_TC;
    if (rs485p->config->txend2_cb != NULL)
    {
      rs485p->config->txend2_cb(rs485p);      // Signal that whole transmit message gone
    }
    else
    {
      osalSysLockFromISR();
      if (oqIsEmptyI(&rs485p->oqueue))
        chnAddFlagsI(rs485p, CHN_TRANSMISSION_END);
      osalSysUnlockFromISR();
    }
  }
}

OSAL_IRQ_HANDLER(STM32_USART2_HANDLER) {

  OSAL_IRQ_PROLOGUE();

  serve_interrupt(&SD2);

  OSAL_IRQ_EPILOGUE();
}
