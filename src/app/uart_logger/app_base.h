#pragma once

#include <stdalign.h>
#include <stdint.h>
#include <sys/cdefs.h>
#if defined(CH32V003)
  #include "ch32fun.h"
  #include <stddef.h>
  #include <stdio.h>

  #include "c_scan.h"

  #define SYSCLK_FREQ FUNCONF_SYSTEM_CORE_CLOCK
  #define sprintf_    sprintf
  #if defined(FUNCONF_DEBUG)
    // #define printf_ printf
    // #define puts_ puts

    #define printf_dbg printf
    #define puts_dbg puts
  #else
    // #define printf_(...)
    // #define puts_(...)
    
    #define printf_dbg(...)
    #define puts_dbg(...)
  #endif
  #define printf_info printf
  #define puts_info puts
  #define printf_err printf
  #define puts_err puts
#else
  #include <ch32v00x.h>
#endif

#ifndef APP_CONF_RUNNING_LOG
  #define APP_CONF_RUNNING_LOG 1
#endif

#ifndef APP_CONF_RELOAD_CFG_ON_START
  #define APP_CONF_RELOAD_CFG_ON_START 1
#endif

#ifndef APP_CONF_RELOAD_TIME_ON_START
  #define APP_CONF_RELOAD_TIME_ON_START 0
#endif

#ifndef APP_CONF_CACHE_BUFF
  #define APP_CONF_CACHE_BUFF 1
#endif

#if APP_CONF_CACHE_BUFF
  // with write cache enabled, periodic sync is disabled by default
  #ifndef APP_CONF_PERIODIC_SYNC
    #define APP_CONF_PERIODIC_SYNC 0
  #endif
#endif

#ifndef APP_CONF_PERIODIC_SYNC
  #define APP_CONF_PERIODIC_SYNC 1
#endif


#define BIT(x)                     (1 << x)

#define GPIO_INPUT_ANALOG          0b0000
#define GPIO_INPUT_FLOAT           0b0100
#define GPIO_INPUT_PULLUPDWN       0b1000
#define GPIO_OUTPUT_50MHz_PP       0b0011
#define GPIO_OUTPUT_50MHz_OD       0b0111
#define GPIO_OUTPUT_50MHz_PERI_PP  0b1011
#define GPIO_OUTPUT_50MHz_PERII_OD 0b1111

#if 1
  #define APP_STATUS_LED1_ON()     GPIOC->BSHR = GPIO_BSHR_BS2
  #define APP_STATUS_LED1_OFF()    GPIOC->BSHR = GPIO_BSHR_BR2
  #define APP_STATUS_LED1_TOGGLE() *((GPIOC->OUTDR & GPIO_Pin_2) ? &GPIOC->BCR : &GPIOC->BSHR) = GPIO_Pin_2
#else
  #define APP_STATUS_LED1_ON()     GPIOC->BSHR = GPIO_BSHR_BR2
  #define APP_STATUS_LED1_OFF()    GPIOC->BSHR = GPIO_BSHR_BS2
  #define APP_STATUS_LED1_TOGGLE() *((GPIOC->OUTDR & GPIO_Pin_2) ? &GPIOC->BCR : &GPIOC->BSHR) = GPIO_Pin_2
#endif

#define APP_KEY_START_STOP    GPIO_Pin_3

#define APP_TIMER_TICK_PERIOD 50 /*milli-sec*/
#define APP_UART_RX_BUFF_SZ   100

#define UART_DMA_Rx                           DMA1_Channel5

#define MEM_DMA_Mover                         DMA1_Channel1
#define MEM_DMA_Start_Mover(src, count)       DMA1->INTFCR = DMA1_FLAG_GL1|DMA1_FLAG_TC1|DMA1_FLAG_HT1|DMA1_FLAG_TE1; \
                                              MEM_DMA_Mover->PADDR = (ptrdiff_t)src; \
                                              MEM_DMA_Mover->CNTR = (ptrdiff_t)count; \
                                              MEM_DMA_Mover->CFGR |= DMA_CFGR1_EN
#define MEM_DMA_Reset_Mover()                 MEM_DMA_Mover->CFGR &= ~DMA_CFGR1_EN
#define MEM_DMA_Wait_For_Mover()              while ((DMA1->INTFR & (DMA1_FLAG_GL1|DMA1_FLAG_TC1|DMA1_FLAG_HT1|DMA1_FLAG_TE1)) == 0) \
                                              ; \

// using uint32 instead of uint16 __aligned(4) saves 50bytes
typedef volatile struct {
  uint32_t uYear /* __attribute_aligned__(4) */;
  uint32_t uMonth /* __attribute_aligned__(4) */;
  uint32_t uDay /* __attribute_aligned__(4) */;
  uint32_t uHour /* __attribute_aligned__(4) */;
  uint32_t uMinute /* __attribute_aligned__(4) */;
  uint16_t uSecond;
  uint16_t uMilliSec;
  #if (APP_CONF_PERIODIC_SYNC == 1)
  uint16_t uPeriodicSync;
  #endif
} stCFGTime;

typedef struct {
  char cOutputFormatter /* __attribute_aligned__(4) */;
  uint32_t uSeqStart /* __attribute_aligned__(4) */;
  uint32_t uSeqStop /* __attribute_aligned__(4) */;
} stCFGLog;

typedef enum { APP_LOGGING_OFF = 0, APP_LOGGING_ON, APP_LOGGING_ERROR } eAPP_State;

typedef volatile struct {
  uint8_t u8OperationalState;
  uint8_t u8zKey;
} stHMI;

extern stHMI gHMI;
extern stCFGTime gTime;

#if defined(FUNCONF_DEBUG)
  // PD0 - Debug 1, UART Rx buffer processing time
  #define dbgGPIO_Init() GPIOD->CFGLR &= 0xFFFFFFF0;GPIOD->CFGLR |= (GPIO_OUTPUT_50MHz_PERI_PP << 4*0)
  #define dbgDBG1_HIGH() GPIOD->BSHR = GPIO_BSHR_BS0
  #define dbgDBG1_LOW()  GPIOD->BSHR = GPIO_BSHR_BR0
#else
  #define dbgGPIO_Init()
  #define dbgDBG1_HIGH()
  #define dbgDBG1_LOW()
#endif
