/*
 * ohs_adc.h
 *
 *  Created on: 7. 9. 2018
 *      Author: Adam
 */

#ifndef OHS_ADC_H_
#define OHS_ADC_H_

static void adcerrorcallback(ADCDriver *adcp, adcerror_t err) {
  (void)adcp;
  (void)err;
}

/*
 * ADC conversion group.
 * Mode:        Linear buffer, 1 sample(s) of 10 channel(s), SW triggered.
 * Channels:    IN_0,3,4,5,6,8,9,10,12,13.
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
  ADC_SMPR1_SMP_AN13(ADC_SAMPLE_15)
  , /* Sample times for channels 10-18 */
  ADC_SMPR2_SMP_AN0(ADC_SAMPLE_15) |
  ADC_SMPR2_SMP_AN3(ADC_SAMPLE_15) |
  ADC_SMPR2_SMP_AN4(ADC_SAMPLE_15) |
  ADC_SMPR2_SMP_AN5(ADC_SAMPLE_15) |
  ADC_SMPR2_SMP_AN6(ADC_SAMPLE_15) |
  ADC_SMPR2_SMP_AN8(ADC_SAMPLE_15) |
  ADC_SMPR2_SMP_AN9(ADC_SAMPLE_15)
  , /* Sample times for channels 0-9 */
  0, /* SQR1 Conversion sequence 13-16*/
  ADC_SQR2_SQ7_N(ADC_CHANNEL_IN10)  |
  ADC_SQR2_SQ8_N(ADC_CHANNEL_IN12) |
  ADC_SQR2_SQ9_N(ADC_CHANNEL_IN8) |
  ADC_SQR2_SQ10_N(ADC_CHANNEL_IN9)
  , /* SQR2 Conversion sequence 7-12 */
  ADC_SQR3_SQ6_N(ADC_CHANNEL_IN13) |
  ADC_SQR3_SQ5_N(ADC_CHANNEL_IN0) |
  ADC_SQR3_SQ4_N(ADC_CHANNEL_IN3) |
  ADC_SQR3_SQ3_N(ADC_CHANNEL_IN4) |
  ADC_SQR3_SQ2_N(ADC_CHANNEL_IN5) |
  ADC_SQR3_SQ1_N(ADC_CHANNEL_IN6)
  /* SQR3 Conversion sequence 1-6 */
};


#endif /* OHS_ADC_H_ */
