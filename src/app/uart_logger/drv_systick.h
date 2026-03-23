#pragma once

#include "app_base.h"

// static inline void drvSystick_Setms (uint32_t delay_ms) {
//     SysTick->CTLR /*&= ~(1<<0)*/ = 0;                           // stop system counter
//     SysTick->SR = 0;                                            // clear CNTIF
//     SysTick->CNT = 0;
//     SysTick->CMP = delay_ms * (1000 * (SYSCLK_FREQ / 8000000)); /*X ms*/
//     printf_info("sta:%d %d\n", SysTick->CNT, SysTick->CMP);
//     SysTick->CTLR |= (1<<0);//+= 1;                           // start system counter
// }

// static inline void drvSystick_WaitForTimeOut (void) {
//     while (SysTick->SR == 0);
//     SysTick->SR = 0;
// }

// static inline uint8_t drvSystick_TimedOut (void) {
//     return SysTick->SR;
// }

// static inline uint8_t drvSystick_Running (void) {
//     return (SysTick->SR == 0);
// }

// static inline void drvSystick_ClearAndStopTimeOut (void) {
//     printf_info("stp:%X %X %d %d\n", SysTick->CTLR, SysTick->SR, SysTick->CNT, SysTick->CMP);
//     // SysTick->SR = 0;
//     // SysTick->CTLR /*&= ~(1<<0)*/ = 0;  // stop system counter
// }

static inline void drvSystick_Restart (void) {
    // printf_dbg("rsta:%X %X %u %u\n", SysTick->CTLR, SysTick->SR, SysTick->CNT, SysTick->CMP);
    SysTick->CNT = 0;
    SysTick->CTLR |= (1<<0);
}

static inline const uint32_t drvSystick_Offset_Setms (uint32_t delay_ms) {
    const uint32_t rtn = SysTick->CNT + (delay_ms * (1000 * (SYSCLK_FREQ / 8000000)));
    // printf_dbg("sta1:%d %d\n", SysTick->CNT, rtn);
    return rtn;
}

static inline const uint32_t drvSystick_Offset_Setus (uint32_t delay_us) {
    const uint32_t rtn = SysTick->CNT + (delay_us * (1 * (SYSCLK_FREQ / 8000000)));
    return rtn;
}

static inline uint8_t drvSystick_Offset_Running (const uint32_t offset) {
    return (SysTick->CNT < offset);
}

static inline uint8_t drvSystick_Offset_TimedOut (const uint32_t offset) {
    return (SysTick->CNT >= offset);
}
