/*
 * rs485.h
 *
 *  Created on: 4. 11. 2018
 *      Author: vysocan
 */

#ifndef RS485_H_
#define RS485_H_

#define RS485_MSG_SIZE    64
#define RS485_HEADER_SIZE 3
#define RS485_CRC_SIZE    2
#define RS485_PACKET_SIZE RS485_MSG_SIZE+RS485_HEADER_SIZE+RS485_CRC_SIZE

/*
 * Extra USARTs definitions here (missing from the ST header file).
 */
#define USART_CR2_STOP1_BITS    (0 << 12)   /**< @brief CR2 1 stop bit value.*/
#define USART_CR2_STOP0P5_BITS  (1 << 12)   /**< @brief CR2 0.5 stop bit value.*/
#define USART_CR2_STOP2_BITS    (2 << 12)   /**< @brief CR2 2 stop bit value.*/
#define USART_CR2_STOP1P5_BITS  (3 << 12)   /**< @brief CR2 1.5 stop bit value.*/

typedef struct {
  /**
   * @brief Bit rate.
   */
  uint32_t                  speed;
  stm32_gpio_t              port;
  stm32_gpio_t              pad;
} RS485Config;

#ifdef __cplusplus
extern "C" {
#endif
  void RS485_init(void);
  void RS485_start(RS485Driver *rs485p, const RS485Config *config);
  void RS485_stop(RS485Driver *rs485p);
#ifdef __cplusplus
}
#endif

#endif /* RS485_H_ */
