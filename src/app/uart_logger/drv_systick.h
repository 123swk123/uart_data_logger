#pragma once

#include "app_base.h"

static inline void drvSystick_Setms (uint32_t delay_ms) {
    SysTick->CTLR /*&= ~(1<<0)*/ = 0;                           // stop system counter
    SysTick->SR = 0;                                            // clear CNTIF
    SysTick->CNT = 0;
    SysTick->CMP = (delay_ms * 1000) * (SYSCLK_FREQ / 8000000); /*X ms*/
    SysTick->CTLR /*|= (1<<0)*/ += 1;                           // start system counter
}

static inline void drvSystick_WaitForTimeOut (void) {
    while (SysTick->SR == 0);
}

static inline uint8_t drvSystick_TimedOut (void) {
    return SysTick->SR;
}

static inline uint8_t drvSystick_Running (void) {
    return (SysTick->SR == 0);
}

static inline void drvSystick_ClearAndStopTimeOut (void) {
    SysTick->SR = 0;
    SysTick->CTLR /*&= ~(1<<0)*/ = 0;  // stop system counter
}
