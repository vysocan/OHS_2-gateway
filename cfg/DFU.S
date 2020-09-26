/**
 * @file rusEfiStartup.S
 * rusEfi startup assembly
 *
 * We have ChibiOS '_crt0_entry' jumping here and then we jump back.
 * There is probably no stack at this point to invoke a method.
 *
 * https://community.st.com/s/question/0D50X00009XkezoSAB/stm32f42xx3xx-usb-device-dfu-within-a-program
 * http://www.chibios.com/forum/viewtopic.php?f=16&p=35916&sid=7f5d3b0c2e8d74d2144a1f9625dafacf#p35916
 * https://github.com/rusefi/ChibiOS/commit/1290cfd8491a6e9a5ab4faa2af8f1debcca94e81
 * https://github.com/rusefi/rusefi/commit/2c99bf035df0986921a0e0b0a5bf98675ef3e8cb
 */

// todo: I have no idea which lines are doing what
// todo: one of these is resolving 'Error: lo register required -- `ldr SP,[R0,#0]'' :)
    .syntax unified
    .cpu    cortex-m3
    .thumb
    .align  2
    .thumb_func
// end of 'I have no idea'

    .global rusEfiAsboluteStartupMethod
rusEfiAsboluteStartupMethod:
// [rusefi][DFU][start]
// Clive Two.Zero is the God of ST community forum
// Device specific, if in doubt RTFM
LDR R0, =0x2002FFF0 // End of SRAM for your CPU
LDR R1, =0xDEADBEEF // magic value
LDR R2, [R0, #0]
STR R0, [R0, #0]    // Invalidate
CMP R2, R1
BEQ UseDFU
    // DFU bootloader not needed, jump back to normal ChibiOS startup
    bl  rusEfiAsboluteStartupMethodReturnPoint
UseDFU:
    // AN2606 Application note
    // STM32 microcontroller system memory boot mode
LDR R0, =0x40023844 // RCC_APB2ENR
LDR R1, =0x00004000 // ENABLE SYSCFG CLOCK
STR R1, [R0, #0]
LDR R0, =0x40013800 // SYSCFG_MEMRMP
LDR R1, =0x00000001 // MAP ROM AT ZERO
STR R1, [R0, #0]
LDR R0, =0x1FFF0000 // ROM BASE
LDR SP,[R0, #0]     // SP @ +0
LDR R0,[R0, #4]     // PC @ +4
BX  R0              // this jumps to DFU bootloader
    // I believe we are never executing this line
// [rusefi][DFU][end]
