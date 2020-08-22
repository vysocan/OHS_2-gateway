/*
 * ohs_peripherial.h
 *
 *  Created on: 23. 2. 2020
 *      Author: adam
 *
 * Peripheral configurations
 */
/*
 * SPI macros for 168Mhz
 * Peripherial Clock /4 = 42MHz for SPI2 SPI3
 * Peripherial Clock /2 = 84MHz for SPI1 SPI4 SPI5 SPI6
 *                                                             SPI1/4/5/6  SPI2/3   */
#define SPI_BaudRatePrescaler_2         ((uint16_t)0x0000) //  42 MHz      21 MHZ
#define SPI_BaudRatePrescaler_4         ((uint16_t)0x0008) //  21 MHz      10.5 MHz
#define SPI_BaudRatePrescaler_8         ((uint16_t)0x0010) //  10.5 MHz    5.25 MHz
#define SPI_BaudRatePrescaler_16        ((uint16_t)0x0018) //  5.25 MHz    2.626 MHz
#define SPI_BaudRatePrescaler_32        ((uint16_t)0x0020) //  2.626 MHz   1.3125 MHz
#define SPI_BaudRatePrescaler_64        ((uint16_t)0x0028) //  1.3125 MHz  656.25 KHz
#define SPI_BaudRatePrescaler_128       ((uint16_t)0x0030) //  656.25 KHz  328.125 KHz
#define SPI_BaudRatePrescaler_256       ((uint16_t)0x0038) //  328.125 KHz 164.06 KHz

#ifndef OHS_PERIPHERAL_H_
#define OHS_PERIPHERAL_H_
/*
 * FM25V05-G Cypress FRAM
 * Maximum speed SPI configuration (40MHz, CPHA=0, CPOL=0, MSb first).
 */
const SPIConfig spi1cfg = {
  false,
  NULL,
  GPIOD, // CS PORT
  GPIOD_SPI1_CS, // CS PIN
  SPI_BaudRatePrescaler_4,
  0
};
/*
 * RFM69HW SPI setting
 */
const SPIConfig spi3cfg = {
  false,
  NULL,
  GPIOD, // CS PORT
  GPIOD_SPI3_CS, // CS PIN
  SPI_BaudRatePrescaler_32,
  0
};
/*
 * Console default setting
 */
static SerialConfig serialCfg = {
  115200,
  0,
  0,
  0,
  NULL, NULL, NULL, NULL
};
/*
 * RS485 default setting
 */
static RS485Config rs485cfg = {
  19200,          // speed
  0,              // address
  GPIOD,          // port
  GPIOD_USART2_DE // pad
};
/*
 * RFM69 configuration
 */
rfm69Config_t rfm69cfg = {
  &SPID3,
  &spi3cfg,
  LINE_RADIO_IRQ,
  RF69_868MHZ,
  1,
  100
};
/*
 * ADC related
 */
#define ADC_GRP1_NUM_CHANNELS 11
#define ADC_GRP1_BUF_DEPTH    1
#define ADC_SCALING_VBAT      (0.0032f) // 3.3V/4095 * (2 for F407) or * (4 fpr F437)
static adcsample_t adcSamples[ADC_GRP1_NUM_CHANNELS * ADC_GRP1_BUF_DEPTH];
/*
 * ADC callback
 */
static void adcerrorcallback(ADCDriver *adcp, adcerror_t err) {
  (void)adcp;
  (void)err;
}
/*
 * ADC conversion group.
 * Mode:        Linear buffer, 1 sample(s) of 11 channel(s), SW triggered.
 * Channels:    IN_0,3,4,5,6,8,9,10,12,13 and VBAT.
 */
static const ADCConversionGroup adcgrpcfg1 = {
  FALSE,
  ADC_GRP1_NUM_CHANNELS,
  NULL,
  adcerrorcallback,
  0,                        /* CR1 */
  ADC_CR2_SWSTART,          /* CR2 */
  ADC_SMPR1_SMP_AN10(ADC_SAMPLE_15) |
  ADC_SMPR1_SMP_AN12(ADC_SAMPLE_15) |
  ADC_SMPR1_SMP_AN13(ADC_SAMPLE_15) |
  ADC_SMPR1_SMP_VBAT(ADC_SAMPLE_15), /* Sample times for channels 10-18 */
  ADC_SMPR2_SMP_AN0(ADC_SAMPLE_15) |
  ADC_SMPR2_SMP_AN3(ADC_SAMPLE_15) |
  ADC_SMPR2_SMP_AN4(ADC_SAMPLE_15) |
  ADC_SMPR2_SMP_AN5(ADC_SAMPLE_15) |
  ADC_SMPR2_SMP_AN6(ADC_SAMPLE_15) |
  ADC_SMPR2_SMP_AN8(ADC_SAMPLE_15) |
  ADC_SMPR2_SMP_AN9(ADC_SAMPLE_15) , /* Sample times for channels 0-9 */
  0, /* SQR1 Conversion sequence 13-16*/
  0, /* HRT */
  0, /* LTR */
  ADC_SQR2_SQ7_N(ADC_CHANNEL_IN10)  |
  ADC_SQR2_SQ8_N(ADC_CHANNEL_IN12) |
  ADC_SQR2_SQ9_N(ADC_CHANNEL_IN8) |
  ADC_SQR2_SQ10_N(ADC_CHANNEL_IN9) |
  ADC_SQR2_SQ11_N(ADC_CHANNEL_VBAT), /* SQR2 Conversion sequence 7-12 */
  ADC_SQR3_SQ6_N(ADC_CHANNEL_IN13) |
  ADC_SQR3_SQ5_N(ADC_CHANNEL_IN0) |
  ADC_SQR3_SQ4_N(ADC_CHANNEL_IN3) |
  ADC_SQR3_SQ3_N(ADC_CHANNEL_IN4) |
  ADC_SQR3_SQ2_N(ADC_CHANNEL_IN5) |
  ADC_SQR3_SQ1_N(ADC_CHANNEL_IN6) /* SQR3 Conversion sequence 1-6 */
};

#endif /* OHS_PERIPHERAL_H_ */
