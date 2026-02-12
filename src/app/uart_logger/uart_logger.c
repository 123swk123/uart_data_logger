#include "app_base.h"

#include "ch32fun.h"
#include "ch32v003hw.h"
#include "ff.h"
#include <stdint.h>
#include <stdio.h>

stHMI gHMI;
stCFGLog gCfgLog;
FATFS gFsFAT;
FIL ghFile;
uint16_t gu16buffRx[APP_UART_RX_BUFF_SZ];

#if (APP_CONF_RUNNING_LOG)
static char gbuffCache[FF_MIN_SS];
static uint32_t gidxCacheWr = 0;
#endif

int _write(int fd,
           const char *buf,
           int size) {
  int i = 0;
  int writeSize = size;

  volatile union {
    uint32_t u32;
    struct {
      uint8_t b3;
      uint8_t b4;
      uint8_t b5;
      uint8_t b6;
    };
  } *data1;
  volatile union {
    uint32_t u32;
    struct {
      uint8_t sz;
      uint8_t b0;
      uint8_t b1;
      uint8_t b2;
    };
  } *data0 = (void *)&size;
  do {
    /**
     * data0  data1 8 bytes
     * data0 The lowest byte storage length, the maximum is 7
     *
     */
    while (*(DMDATA0) != 0u)
      ;

    if (writeSize > 7) {
      *(DMDATA1) = (*(buf + i + 3)) | (*(buf + i + 4) << 8) | (*(buf + i + 5) << 16) | (*(buf + i + 6) << 24);
      // data1.b3 = *(buf+i+3);
      // data1.b4 = *(buf+i+4);
      // data1.b5 = *(buf+i+5);
      // data1.b6 = *(buf+i+6);
      // *(DMDATA1) = data1.u32;

      *(DMDATA0) = (7u) | (*(buf + i) << 8) | (*(buf + i + 1) << 16) | (*(buf + i + 2) << 24);
      // data0->b2 = *(buf+i+2);
      // data0->b1 = *(buf+i+1);
      // data0->b0 = *(buf+i+0);
      // data0->sz = writeSize;
      // *(DMDATA0) = data0->u32;

      i += 7;
      writeSize -= 7;
    } else {
      *(DMDATA1) = (*(buf + i + 3)) | (*(buf + i + 4) << 8) | (*(buf + i + 5) << 16) | (*(buf + i + 6) << 24);
      // data1.b3 = *(buf+i+3);
      // data1.b4 = *(buf+i+4);
      // data1.b5 = *(buf+i+5);
      // data1.b6 = *(buf+i+6);
      // *(DMDATA1) = data1.u32;

      *(DMDATA0) = (writeSize) | (*(buf + i) << 8) | (*(buf + i + 1) << 16) | (*(buf + i + 2) << 24);
      // data0->b2 = *(buf+i+2);
      // data0->b1 = *(buf+i+1);
      // data0->b0 = *(buf+i+0);
      // data0->sz = writeSize;
      // *(DMDATA0) = data0->u32;

      writeSize = 0;
    }
  } while (writeSize);
  return size;
}

static char *_f_gets(char *buff,
                     FIL *f) {
  UINT nBytesAccessed;
  char c = '\0';

  if (buff == NULL) {
    // skip this line
    while (c != '\n') {
      f_read(&ghFile, &c, 1, &nBytesAccessed);
      if (nBytesAccessed == 0) {
        return buff;
      }
    }
    return buff;
  } else {
    while (c != '\n') {
      f_read(&ghFile, &c, 1, &nBytesAccessed);
      if (nBytesAccessed == 0) {
        return NULL;
      }
      if (c == '\n')
        *buff++ = '\0';
      else if (c != '\r')
        *buff++ = c;
    }
    return buff;
  }
}

static inline FRESULT doConfig(void) {
  FRESULT rslt = f_open(&ghFile, "config.txt", FA_READ);
  int tmp;
  if (rslt == FR_OK) {
    #if (APP_CONF_CACHE_BUFF == 1)
    #define buff gbuffCache
    #else
    char buff[40];
    #endif
    char *pBuff;
    struct {
      uint32_t uBaud;
      uint32_t uBits;
      uint32_t uParity;
      uint32_t uStop;
    } cfgUART;

// skip the first 2 comment line
#if 0
        UINT nBytesAccessed;
        tmp = 0;
        while(tmp < 2) {
            f_read(&ghFile, buff, 1, &nBytesAccessed);
            if(nBytesAccessed == 0) {rslt = FR_INVALID_PARAMETER;goto _doConfig_Exit;}
            if(buff[0] == '\n')
                tmp++;
        }
#else
    _f_gets(NULL, &ghFile);
    _f_gets(NULL, &ghFile);
#endif

    // read uart1 config
    // pBuff = f_gets(buff, sizeof(buff)-1, &ghFile);
    pBuff = _f_gets(buff, &ghFile);
    if (pBuff == 0) {
      rslt = FR_INVALID_PARAMETER;
      goto _doConfig_Exit;
    }
    puts_(buff);
    // skip first 3 chars 's1:'
    tmp = c_sscanf(&buff[3], "%u%u%u%u%*c%c%u%u", &cfgUART.uBaud, &cfgUART.uBits, &cfgUART.uParity, &cfgUART.uStop,
                   &gCfgLog.cOutputFormatter, &gCfgLog.uSeqStart, &gCfgLog.uSeqStop);
    printf_("s1:%u,%u,%u,%u,%u,%u,%c|\n", cfgUART.uBaud, cfgUART.uBits, cfgUART.uParity, cfgUART.uStop, gCfgLog.uSeqStart,
            gCfgLog.uSeqStop, gCfgLog.cOutputFormatter);
    if (tmp != 8) {
      rslt = FR_INVALID_PARAMETER;
      goto _doConfig_Exit;
    }

#define CTLR1_UE_Set   ((uint16_t)0x2000) /* USART Enable Mask */
#define CTLR1_UE_Reset ((uint16_t)0xDFFF) /* USART Disable Mask */
                                          // re-initialize the UART with user-given parameters

#if 1
    // disable USART1
    USART1->CTLR1 = 0;
    USART1->STATR = 0;
    // Rx Enabled
    tmp = USART_CTLR1_RE;
    if (cfgUART.uBits == 9) {
      tmp |= USART_CTLR1_M;
    }
    if (cfgUART.uParity == 1) // Odd-parity
      tmp |= USART_CTLR1_PCE | USART_CTLR1_PS;
    if (cfgUART.uParity == 2) // Even-parity
      tmp |= USART_CTLR1_PCE;
    USART1->CTLR1 = tmp;

    // for CTLR2
    // Stop bits 1 or 2
    USART1->CTLR2 = (cfgUART.uStop == 1) ? (0b00 << 12) : (0b10 << 12);

    // DMAR:En
    USART1->CTLR3 = USART_CTLR3_DMAR;

    UART_DMA_Rx->PADDR = (ptrdiff_t)&USART1->DATAR;
    UART_DMA_Rx->MADDR = (ptrdiff_t)gu16buffRx;
    UART_DMA_Rx->CNTR = APP_UART_RX_BUFF_SZ;
    UART_DMA_Rx->CFGR = DMA_DIR_PeripheralSRC | DMA_PeripheralInc_Disable | DMA_MemoryInc_Enable |
                        DMA_PeripheralDataSize_HalfWord | DMA_MemoryDataSize_HalfWord | DMA_Mode_Circular |
                        DMA_Priority_Medium | DMA_M2M_Disable;

  #if 0
        uint32_t integerdivider = ((25 * SYSCLK_FREQ) / (4 * cfgUART.uBaud));
        uint32_t tmpreg = (integerdivider / 100) << 4;
        uint32_t fractionaldivider = integerdivider - (100 * (tmpreg >> 4));
        tmpreg |= ((((fractionaldivider * 16) + 50) / 100)) & ((uint8_t)0x0F);
        USART1->BRR = tmpreg;
  #elif 0
    uint32_t integerdivider = ((SYSCLK_FREQ >> 4) * 100) / cfgUART.uBaud;
    USART1->BRR = integerdivider / 100; //(SYSCLK_FREQ>>4)/cfgUART.uBaud;
    integerdivider -= USART1->BRR * 100;
    USART1->BRR = (USART1->BRR << 4) | (integerdivider / 6);
  #else
    // div=(Fsys/16)*128/baud               => ((Fsys >> 4) << 7)/baud
    // fra=(div-((div/128)*128))/(128/16)   => (div - (div & 0xFFFF_FF80)) >> 3
    // baud=int(384e3);div=(fsys>>4<<7)//baud;reg=((div & 0xFFFF_FF80)>>3)|((div &
    // 0x7f)>>3);divisor=(reg>>4)+((reg&0x0f)/16);act_baud=fsys//16//divisor;print(f'{div/128} -> {reg} =>
    // {divisor}:{act_baud}bps:{((baud-act_baud)*100)/baud:.04}%')
    USART1->BRR = ((SYSCLK_FREQ >> 4) << 7) / cfgUART.uBaud;
    USART1->BRR = ((USART1->BRR & 0xFFFFFF80) >> 3) | ((USART1->BRR & 0x7F) >> 3);
  #endif

    USART1->GPR = 0;

    USART1->CTLR1 |= USART_CTLR1_UE;
    printf_("%04X, %04X, %04X, %04X\n", USART1->CTLR1, USART1->CTLR2, USART1->CTLR3, USART1->BRR);
#endif

    // read configured time
    // pBuff = f_gets(buff, sizeof(buff)-1, &ghFile);
    #if (APP_CONF_RELOAD_TIME_ON_START == 0)
    if(gTime.uYear > 1980)
    #endif
    {
      pBuff = _f_gets(buff, &ghFile);
      if (pBuff == 0) {
        rslt = FR_INVALID_PARAMETER;
        goto _doConfig_Exit;
      }
      puts_(buff);
      tmp = c_sscanf(&buff[2], "%u%u%u%u%u", &gTime.uYear, &gTime.uMonth, &gTime.uDay, &gTime.uHour, &gTime.uMinute);
      printf_("t:%d,%d,%d,%d,%d\n", gTime.uYear, gTime.uMonth, gTime.uDay, gTime.uHour, gTime.uMinute);
      gTime.uSecond = gTime.uMilliSec = 0;
      if (tmp != 5) {
        rslt = FR_INVALID_PARAMETER;
        goto _doConfig_Exit;
      }
    }
  }
_doConfig_Exit:
  f_close(&ghFile);
  return rslt;
  #if (APP_CONF_CACHE_BUFF == 1)
    #undef buff
  #endif
}

#if (APP_CONF_RUNNING_LOG == 0)
static inline FRESULT doCreateNewLog(void) {
  char buff[12+1];
  // DDhhmmss.log <-- short file name
  char *pBuff = buff;

  pBuff += mini_itoa(gTime.uDay, 10, 0, 1, pBuff);
  // *pBuff++ = '_';
  pBuff += mini_itoa(gTime.uHour, 10, 0, 1, pBuff);
  // *pBuff++ = '_';
  pBuff += mini_itoa(gTime.uMinute, 10, 0, 1, pBuff);
  // *pBuff++ = '_';
  pBuff += mini_itoa(gTime.uSecond, 10, 0, 1, pBuff);

  *pBuff++ = '.';
  *pBuff++ = 'l';
  *pBuff++ = 'o';
  *pBuff++ = 'g';
  *pBuff++ = '\0';

  puts_(buff);
  FRESULT rslt = f_open(&ghFile, buff, FA_CREATE_NEW | FA_WRITE);
  if (rslt == FR_OK) {
    return rslt;
  }

  f_close(&ghFile);
  return rslt;
}
#else
static inline FRESULT doCreateNewLog(void) {
  FRESULT rslt = f_open(&ghFile, "head", FA_READ | FA_WRITE);
  if (rslt == FR_OK) {
    char buff[16];
    UINT nBytesAccessed;
    int tmp;
#define nFileIdx nBytesAccessed

    if (f_size(&ghFile) >= sizeof(buff)) {
      rslt = FR_INVALID_PARAMETER;
      goto _doCreateNewLog_Exit;
    }

    rslt = f_read(&ghFile, buff, sizeof(buff), &nBytesAccessed);
    if (rslt != FR_OK) {
      goto _doCreateNewLog_Exit;
    }
    rslt = f_rewind(&ghFile);
    if (rslt != FR_OK) {
      goto _doCreateNewLog_Exit;
    }
    buff[nBytesAccessed] = '\0';
    tmp = c_sscanf(buff, "%u", &nFileIdx);
    if (tmp != 1) {
      rslt = FR_INVALID_PARAMETER;
      goto _doCreateNewLog_Exit;
    }
    // tmp = sprintf_(buff, "%d", ++nFileIdx);
    tmp = mini_itoa(++nFileIdx, 10, 0, 1, buff);
    char *pBuff = &buff[tmp];
    rslt = f_write(&ghFile, buff, tmp, &nBytesAccessed);
    if (rslt != FR_OK) {
      goto _doCreateNewLog_Exit;
    }
    f_close(&ghFile);

    *pBuff++ = '.';
    *pBuff++ = 'l';
    *pBuff++ = 'o';
    *pBuff++ = 'g';
    *pBuff++ = '\0';
    puts_(buff);
    rslt = f_open(&ghFile, buff, FA_CREATE_NEW | FA_WRITE);
    if (rslt != FR_OK) {
      goto _doCreateNewLog_Exit;
    }
    return rslt;

#undef nFileIdx
  }
_doCreateNewLog_Exit:
  f_close(&ghFile);
  return rslt;
}
#endif

#if (APP_CONF_CACHE_BUFF) 
static inline int doWriteLog(uint16_t val) {
  UINT nBytesAccessed;

  #define cacheWrite(x) gbuffCache[gidxCacheWr++]=x
  #define cacheWritePtr &gbuffCache[gidxCacheWr]
  #define cacheWriterInc() gidxCacheWr++

  if (val == gCfgLog.uSeqStart) {
    if (val == gCfgLog.uSeqStop) {
      cacheWrite('\r');
      cacheWrite('\n');
    }
    cacheWrite('[');
    gidxCacheWr += mini_itoa(gTime.uHour, 10, 0, 1, cacheWritePtr);
    cacheWrite(':');
    gidxCacheWr += mini_itoa(gTime.uMinute, 10, 0, 1, cacheWritePtr);
    cacheWrite(':');
    gidxCacheWr += mini_itoa(gTime.uSecond, 10, 0, 1, cacheWritePtr);
    cacheWrite('.');
    gidxCacheWr += mini_itoa(gTime.uMilliSec, 10, 0, 1, cacheWritePtr);
    cacheWrite(']');
  } else if (val == gCfgLog.uSeqStop) {
    cacheWrite('\r');
    cacheWrite('\n');
  } else {
    switch (gCfgLog.cOutputFormatter) {
    case 'u':
      gidxCacheWr += mini_itoa(val, 10, 0, 1, cacheWritePtr);
      cacheWrite(' ');
      break;
    case 'x':
    case 'X':
      gidxCacheWr += mini_itoa(val, 16, (gCfgLog.cOutputFormatter=='X'), 1, cacheWritePtr);
      cacheWrite(' ');
      break;
    case 'c':
    default:
      cacheWrite(val & 0xFF);
    }
  }

_write_buff:
  if(gidxCacheWr >= (FF_MIN_SS-20)) {
    f_write(&ghFile, gbuffCache, gidxCacheWr, &nBytesAccessed);
    if (nBytesAccessed != gidxCacheWr) {
      gHMI.u8OperationalState = APP_LOGGING_ERROR;
    }
    gidxCacheWr = 0;
    return nBytesAccessed;
  } else {
    return 0;
  }
}
#else
static inline int doWriteLog(uint16_t val) {
  char buff[20];
  UINT nBytesAccessed;
  int bytes2write = 0;

  if (val == gCfgLog.uSeqStart) {
    if (val == gCfgLog.uSeqStop) {
      buff[0] = '\r';
      buff[1] = '\n';
      buff[2] = '[';
      bytes2write = 3;
    } else {
      buff[0] = '[';
      bytes2write = 1;
    }
    bytes2write += mini_itoa(gTime.uHour, 10, 0, 1, &buff[bytes2write]);
    buff[bytes2write++] = ':';
    bytes2write += mini_itoa(gTime.uMinute, 10, 0, 1, &buff[bytes2write]);
    buff[bytes2write++] = ':';
    bytes2write += mini_itoa(gTime.uSecond, 10, 0, 1, &buff[bytes2write]);
    buff[bytes2write++] = '.';
    bytes2write += mini_itoa(gTime.uMilliSec, 10, 0, 1, &buff[bytes2write]);
    buff[bytes2write++] = ']';
  } else if (val == gCfgLog.uSeqStop) {
    buff[0] = '\r';
    buff[1] = '\n';
    bytes2write = 2;
  } else {
    switch (gCfgLog.cOutputFormatter) {
    case 'u':
      bytes2write = mini_itoa(val, 10, 0, 1, buff);
      break;
    case 'x':
      bytes2write = mini_itoa(val, 16, 0, 1, buff);
      break;
    case 'X':
      bytes2write = mini_itoa(val, 16, 1, 1, buff);
      break;
    case 'c':
    default:
      buff[0] = val & 0xFF;
      bytes2write = 1;
    }
  }

_write_buff:
  f_write(&ghFile, &buff, bytes2write, &nBytesAccessed);
  if (nBytesAccessed != bytes2write) {
    gHMI.u8OperationalState = APP_LOGGING_ERROR;
  }

  return bytes2write;
}
#endif

int main(void) {
  SystemInit();

  RCC->APB2PCENR |=
      RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD | RCC_APB2Periph_SPI1 | RCC_APB2Periph_USART1;
  RCC->APB1PCENR |= RCC_APB1Periph_TIM2;
  RCC->AHBPCENR |= RCC_AHBPeriph_DMA1;

  // dbgGPIO_Init();

  AFIO->PCFR1 = AFIO_PCFR1_USART1_REMAP_1 | AFIO_PCFR1_USART1_REMAP | AFIO_PCFR1_SPI1_REMAP;

  // Timer 2 initialization
  // OC1: OCMode Freeze, Compare Output Disabled, OCR pre-load Disabled
  // TIM2->CH1CVR = 500-1;
  // TIM2->CTLR2 <- No need
  // TIM2->CHCTLR1 <- reset value is good
  // TIM2->CCER <- reset value is good]

  // Timer: Internal Clock, Count Up Mode, CLK_DIV1, Auto-reload Enabled
  TIM2->PSC = 48000 - 1;               // 1khz
  TIM2->ATRLR = APP_TIMER_TICK_PERIOD; // 100 milli-sec
  TIM2->CTLR1 = TIM_ARPE | TIM_CEN | TIM_URS;
  // TIM2->SWEVGR = TIM_EventSource_Trigger;//TIM_PSCReloadMode_Immediate;
  // TIM2->SMCFGR <- reset value is good
  TIM2->DMAINTENR = TIM_IT_Update;

  gCfgLog.cOutputFormatter = gTime.uYear = 0;
  gHMI.u8OperationalState = APP_LOGGING_OFF;
  gHMI.u8zKey = GPIOC->INDR & APP_KEY_START_STOP;

  __enable_irq();
  NVIC_EnableIRQ(TIM2_IRQn);

  while (1) {
    if (gHMI.u8OperationalState == APP_LOGGING_ON) {
      FRESULT tmp = f_mount(&gFsFAT, "", 0); // lazy mount, saves some 100bytes of ROM
      printf_("mount: %d\n", tmp);
      if (tmp == FR_OK) {
        #if (APP_CONF_RELOAD_CFG_ON_START == 0)
        if (gCfgLog.cOutputFormatter == 0)
        #endif
        {
          tmp = doConfig();
        }

        if (tmp == FR_OK) {
          tmp = doCreateNewLog();
          if (tmp == FR_OK) {
            UART_DMA_Rx->CFGR |= DMA_CFGR1_EN;
            // uint16_t* tail = &gu16buffRx[0];
            uint32_t tail = 0;

            // do the first header write
            doWriteLog(gCfgLog.uSeqStart);

            while (gHMI.u8OperationalState == APP_LOGGING_ON) {
              // while(USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == RESET);
              // while((USART1->STATR & USART_FLAG_RXNE) == 0);
              // uint16_t tmp = USART1->DATAR;//USART_ReceiveData(USART1);

              #if 0
              uint16_t* head = &gu16buffRx[APP_UART_RX_BUFF_SZ] - UART_DMA_Rx->CNTR;
              // dbgDBG1_HIGH();
              while (tail != head) {
                APP_STATUS_LED1_OFF();
                tmp = doWriteLog(*tail++);
                if (tail >= &gu16buffRx[APP_UART_RX_BUFF_SZ]) {
                  tail = &gu16buffRx[0];
                }
              }
              // dbgDBG1_LOW();
              #else
              uint32_t head = (APP_UART_RX_BUFF_SZ - UART_DMA_Rx->CNTR);
              // dbgDBG1_HIGH();
              while (tail != head) {
                // printf_("%u<->%u\n", tail, head);
                APP_STATUS_LED1_OFF();
                tmp = doWriteLog(gu16buffRx[tail++]);
                tail = tail % APP_UART_RX_BUFF_SZ;
              }
              // dbgDBG1_LOW();
              #endif

              #if (APP_CONF_PERIODIC_SYNC)
              if (!(gTime.uMilliSec % (5 * APP_TIMER_TICK_PERIOD))) {
                // @ every 1 sec if we have written anything
                if ((int)tmp > 0) {
                  f_sync(&ghFile);
                }
              }
              #endif
            }

            #if (APP_CONF_CACHE_BUFF)
            if (gidxCacheWr) {
              #define nBytesAccessed tail
              f_write(&ghFile, gbuffCache, gidxCacheWr, &nBytesAccessed);
              #undef nBytesAccessed
            }
            #endif

            f_close(&ghFile);
          } else {
            printf_("doCreateNewLog:%d\n", tmp);
            gHMI.u8OperationalState = APP_LOGGING_ERROR;
          }
        } else {
          printf_("doConfig:%d\n", tmp);
          gHMI.u8OperationalState = APP_LOGGING_ERROR;
        }

        f_unmount("");
        printf_("Safe removal\n");
      } else {
        gHMI.u8OperationalState = APP_LOGGING_ERROR;
      }
    } else {
      // Turn off UART Rx DMA
      UART_DMA_Rx->CFGR &= ~DMA_CFGR1_EN;
    }
  }
}
