/*
 * hard_fault.h
 *
 *  Created on: Feb 28, 2026
 *      Author: vysocan
 */

#ifndef HARD_FAULT_H_
#define HARD_FAULT_H_

/*
 * Custom HardFault handler to capture faulting registers.
 * Inspect fault_info[] in the debugger after a crash.
 * [0]=R0, [1]=R1, [2]=R2, [3]=R3, [4]=R12, [5]=LR, [6]=PC, [7]=xPSR,
 * [8]=CFSR, [9]=HFSR, [10]=BFAR, [11]=MMFAR
 */
volatile uint32_t fault_info[12];

void __attribute__((used)) HardFault_Handler_C(uint32_t *hardfault_args) {
  fault_info[0]  = hardfault_args[0];  // R0
  fault_info[1]  = hardfault_args[1];  // R1
  fault_info[2]  = hardfault_args[2];  // R2
  fault_info[3]  = hardfault_args[3];  // R3
  fault_info[4]  = hardfault_args[4];  // R12
  fault_info[5]  = hardfault_args[5];  // LR
  fault_info[6]  = hardfault_args[6];  // PC  <-- faulting instruction
  fault_info[7]  = hardfault_args[7];  // xPSR
  fault_info[8]  = *(volatile uint32_t *)0xE000ED28;  // CFSR
  fault_info[9]  = *(volatile uint32_t *)0xE000ED2C;  // HFSR
  fault_info[10] = *(volatile uint32_t *)0xE000ED38;  // BFAR
  fault_info[11] = *(volatile uint32_t *)0xE000ED34;  // MMFAR
  __asm volatile("bkpt #0");  // Break into debugger
  while(1);
}

void __attribute__((naked)) HardFault_Handler(void) {
  __asm volatile(
    "tst lr, #4          \n"  // Check EXC_RETURN bit 2
    "ite eq              \n"
    "mrseq r0, msp       \n"  // If 0, fault used MSP
    "mrsne r0, psp       \n"  // If 1, fault used PSP
    "b HardFault_Handler_C \n"
    ::: "r0"
  );
}

#endif /* HARD_FAULT_H_ */
