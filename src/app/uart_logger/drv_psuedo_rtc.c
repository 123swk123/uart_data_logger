#include "ff.h"
#include "app_base.h"
#include "ch32fun.h"
#include "ch32v003hw.h"
#include <stdint.h>

#define MAP_DAYS2MONTH {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}

static uint8_t key_exclusion_period = 0;

stCFGTime gTime;

INTERRUPT_DECORATOR void TIM2_IRQHandler(void);
void TIM2_IRQHandler(void) {
  static const uint8_t days2month[] = MAP_DAYS2MONTH;
//   printf_("IRQ TIM2:%04X\n", TIM2->INTFR);
  TIM2->INTFR = 0;

  if(key_exclusion_period == 0) {
    uint8_t tmp = GPIOC->INDR & APP_KEY_START_STOP;
    if (tmp != gHMI.u8zKey)
      gHMI.u8zKey = tmp;
    else {
      if (!tmp) {
        // switch open => high
        // switch close => low
        gHMI.u8OperationalState = (gHMI.u8OperationalState == APP_LOGGING_OFF) ? APP_LOGGING_ON : APP_LOGGING_OFF;
        key_exclusion_period = 100;  // set 10 seconds exclusion period
      }
    }
  } else {
    key_exclusion_period--;
  }

  printf_("%u %u\n", key_exclusion_period, gHMI.u8OperationalState);

  if (gHMI.u8OperationalState == APP_LOGGING_OFF)
    APP_STATUS_LED1_OFF();
  else if (gHMI.u8OperationalState == APP_LOGGING_ON)
    APP_STATUS_LED1_ON();
  else {
    // Error state
    APP_STATUS_LED1_TOGGLE();
  }

  gTime.uMilliSec += APP_TIMER_TICK_PERIOD;
  if (gTime.uMilliSec == 1000) {
    gTime.uMilliSec = 0;
    if (++gTime.uSecond == 60) {
      gTime.uSecond = 0;
      if (++gTime.uMinute == 60) {
        gTime.uMinute = 0;
        if (++gTime.uHour == 24) {
          gTime.uHour = 0;
          if (++gTime.uDay == days2month[gTime.uMonth]) {
            gTime.uDay = 1;
            if (++gTime.uMonth == 12) {
              gTime.uMonth = 1;
              gTime.uYear++; // Year is un-checked for
            }
          }
        }
      }
    }
  }
}

DWORD get_fattime(void) {
  // time_t t;
  // struct tm *stm;

  // t = time(0);
  // stm = localtime(&t);

  // return ((DWORD)(FF_NORTC_YEAR - 1980) << 25 | (DWORD)FF_NORTC_MON << 21 | (DWORD)FF_NORTC_MDAY << 16);
  return (DWORD)(gTime.uYear - 1980) << 25 | (DWORD)gTime.uMonth << 21 | (DWORD)gTime.uDay << 16 |
         (DWORD)gTime.uHour << 11 | (DWORD)gTime.uMinute << 5 | (DWORD)gTime.uSecond >> 1;
}
