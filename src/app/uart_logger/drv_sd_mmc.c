#include "drv_systick.h"

#include "ff.h"     /* Basic definitions of FatFs */
#include "diskio.h" /* Declarations FatFs MAI */

/* SPI SPE mask */
#define CTLR1_SPE_Set   ((uint16_t)0x0040)
#define CTLR1_SPE_Reset ((uint16_t)0xFFBF)

/* SPI SSOE mask */
#define CTLR2_SSOE_Set   ((uint16_t)0x0004)
#define CTLR2_SSOE_Reset ((uint16_t)0xFFFB)

/* SPI_NSS_internal_software_management */
#define SPI_NSSInternalSoft_Set   ((uint16_t)0x0100)
#define SPI_NSSInternalSoft_Reset ((uint16_t)0xFEFF)

// clang-format off
#define SPI_DMA_Rx                  DMA1_Channel2
#define SPI_DMA_Tx                  DMA1_Channel3
#define SPI_DMA_FLAGS_Rx            DMA1_FLAG_GL2
#define SPI_DMA_FLAGS_Tx            DMA1_FLAG_GL3
#define SPI_DMA_FLAGS_Tx_Rx         (DMA1_FLAG_GL3 | DMA1_FLAG_TC3 | DMA1_FLAG_HT3 | DMA1_FLAG_TE3 | \
                                    DMA1_FLAG_GL2 | DMA1_FLAG_TC2 | DMA1_FLAG_HT2 | DMA1_FLAG_TE2)

#define SPI_DMA_Start_Tx_Rx(count)  SPI_DMA_Tx->CNTR = SPI_DMA_Rx->CNTR = count; \
                                    SPI_DMA_Tx->CFGR |= DMA_CFGR1_EN; \
                                    SPI_DMA_Rx->CFGR |= DMA_CFGR1_EN

#define SPI_DMA_Start_Tx(count)     DMA1->INTFCR = SPI_DMA_FLAGS_Tx_Rx; \
                                    SPI_DMA_Tx->CNTR = count; \
                                    SPI_DMA_Tx->CFGR |= DMA_CFGR1_EN

#define SPI_DMA_Reset_Tx_Rx()       SPI_DMA_Tx->CFGR &= ~DMA_CFGR1_EN;SPI_DMA_Rx->CFGR &= ~DMA_CFGR1_EN

#define SPI_DMA_Reset_Tx()          SPI_DMA_Tx->CFGR &= ~DMA_CFGR1_EN

#define SPI_DMA_Wait_For_Rx()       while ((DMA1->INTFR & SPI_DMA_FLAGS_Rx) == 0) \
                                    ; \
                                    while (SPI1->STATR & SPI_I2S_FLAG_BSY) \
                                    ; \
                                    SPI_DMA_Reset_Tx_Rx()

// PC4|CS 	: GPIO, Push-Pull
// PC5|SCK 	: SPI Peri, Push-Pull
// PC6|MOSI : SPI Peri, Push-Pull
// PC7|MISO : SPI Peri, Input, pull-up
#define SPI_GPIO_DEFAULT            (GPIO_INPUT_PULLUPDWN << 4 * 7) | (GPIO_OUTPUT_50MHz_PERI_PP << 4 * 6) | \
                                    (GPIO_OUTPUT_50MHz_PERI_PP << 4 * 5) | (GPIO_OUTPUT_50MHz_PP << 4 * 4) | \
                                    (GPIO_INPUT_PULLUPDWN << 4 * 3) | (GPIO_OUTPUT_50MHz_PP << 4 * 2) | \
                                    (GPIO_INPUT_PULLUPDWN << 4 * 1) | (GPIO_OUTPUT_50MHz_PERI_PP << 4 * 0)
// Keeps MOSI line high as a GPIO pull-up
#define SPI_GPIO_MOSI_HIGH          (GPIO_INPUT_PULLUPDWN << 4 * 7) | (GPIO_OUTPUT_50MHz_PP << 4 * 6) | \
                                    (GPIO_OUTPUT_50MHz_PERI_PP << 4 * 5) | (GPIO_OUTPUT_50MHz_PP << 4 * 4) | \
                                    (GPIO_INPUT_PULLUPDWN << 4 * 3) | (GPIO_OUTPUT_50MHz_PP << 4 * 2) | \
                                    (GPIO_INPUT_PULLUPDWN << 4 * 1) | (GPIO_OUTPUT_50MHz_PERI_PP << 4 * 0)
// clang-format on

#define CS_HIGH()              GPIOC->BSHR = GPIO_BSHR_BS4
#define CS_LOW()               GPIOC->BSHR = GPIO_BSHR_BR4

#define drvSysTck_Delay_Sync() while (SysTick->SR == 0)

/* MMC/SD command */
#define CMD0   (0)         /* GO_IDLE_STATE */
#define CMD1   (1)         /* SEND_OP_COND (MMC) */
#define ACMD41 (0x80 + 41) /* SEND_OP_COND (SDC) */
#define CMD8   (8)         /* SEND_IF_COND */
#define CMD9   (9)         /* SEND_CSD */
#define CMD10  (10)        /* SEND_CID */
#define CMD12  (12)        /* STOP_TRANSMISSION */
#define ACMD13 (0x80 + 13) /* SD_STATUS (SDC) */
#define CMD16  (16)        /* SET_BLOCKLEN */
#define CMD17  (17)        /* READ_SINGLE_BLOCK */
#define CMD18  (18)        /* READ_MULTIPLE_BLOCK */
#define CMD23  (23)        /* SET_BLOCK_COUNT (MMC) */
#define ACMD23 (0x80 + 23) /* SET_WR_BLK_ERASE_COUNT (SDC) */
#define CMD24  (24)        /* WRITE_BLOCK */
#define CMD25  (25)        /* WRITE_MULTIPLE_BLOCK */
#define CMD32  (32)        /* ERASE_ER_BLK_START */
#define CMD33  (33)        /* ERASE_ER_BLK_END */
#define CMD38  (38)        /* ERASE */
#define CMD55  (55)        /* APP_CMD */
#define CMD58  (58)        /* READ_OCR */

static volatile DSTATUS Stat = STA_NOINIT; /* Physical drive status */
static BYTE CardType;                      /* Card type flags */

static uint8_t gu8BuffSPITx[10];

// static uint8_t gu8BuffSPIRx[10];'
#define gu8BuffSPIRx gu8BuffSPITx

#ifdef FUNCONF_DEBUG
static void _dump_buffer(uint8_t *buff,
                         uint32_t len_alg16) {
  while (len_alg16--) {
    printf_("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n", buff[0], buff[1],
            buff[2], buff[3], buff[4], buff[5], buff[6], buff[7], buff[8], buff[9], buff[10], buff[11], buff[12],
            buff[13], buff[14], buff[15]);
    buff += 16;
  }
}
#else
#define _dump_buffer(x, y) 
#endif

static inline void _spi_init(void) {
  // PC0|CS 	: GPIO, Push-Pull
  // PC5|SCK 	: SPI Peri, Push-Pull
  // PC6|MOSI : SPI Peri, Push-Pull
  // PC7|MISO : SPI Peri, Input, pull-up
  // GPIOC->CFGLR = 0x8BB88883;
  GPIOC->CFGLR = SPI_GPIO_DEFAULT;
  GPIOC->OUTDR = 0xFF;
  // GPIO_PinRemapConfig (GPIO_Remap_SPI1, ENABLE);

  // SPI1->CTLR1 |= SPI_NSSInternalSoft_Set;
  SPI1->CTLR1 = SPI_Direction_2Lines_FullDuplex | SPI_Mode_Master | SPI_NSS_Soft | SPI_NSSInternalSoft_Set |
                SPI_DataSize_8b | SPI_FirstBit_MSB | SPI_CPOL_Low | SPI_CPHA_1Edge | SPI_BaudRatePrescaler_16;
  SPI1->CTLR2 = SPI_I2S_DMAReq_Tx | SPI_I2S_DMAReq_Rx;

  SPI_DMA_Rx->PADDR = SPI_DMA_Tx->PADDR = (ptrdiff_t)&SPI1->DATAR;
  SPI_DMA_Tx->MADDR = (ptrdiff_t)gu8BuffSPITx;
  SPI_DMA_Rx->MADDR = (ptrdiff_t)gu8BuffSPIRx;
  SPI_DMA_Rx->CFGR = DMA_DIR_PeripheralSRC | DMA_PeripheralInc_Disable | DMA_MemoryInc_Enable |
                     DMA_PeripheralDataSize_Byte | DMA_MemoryDataSize_Byte | DMA_Mode_Normal | DMA_Priority_Medium |
                     DMA_M2M_Disable;
  SPI_DMA_Tx->CFGR = DMA_DIR_PeripheralDST | DMA_PeripheralInc_Disable | DMA_MemoryInc_Enable |
                     DMA_PeripheralDataSize_Byte | DMA_MemoryDataSize_Byte | DMA_Mode_Normal | DMA_Priority_Medium |
                     DMA_M2M_Disable;

  SPI1->CTLR1 |= CTLR1_SPE_Set;
}

/*-----------------------------------------------------------------------*/
/* Wait for card ready                                                   */
/*-----------------------------------------------------------------------*/
static int wait_ready(        /* 1:Ready, 0:Timeout */
                      UINT wt /* Timeout [ms] */
) {
  // BYTE d;

  // SysTick->CTLR /*&= ~(1<<0)*/=0; //stop system counter
  // SysTick->SR = 0;                //clear CNTIF
  // SysTick->CNT = 0;
  // SysTick->CMP=(wt*1000)*(SYSCLK_FREQ/8000000);/*1ms*/
  // SysTick->CTLR /*|= (1<<0)*/ += 1;  //start system counter
  drvSystick_Setms(wt);

  do {
    // DMA1->INTFCR = SPI_DMA_FLAGS_Tx_Rx;
    GPIOC->CFGLR = SPI_GPIO_MOSI_HIGH;
    SPI_DMA_Reset_Tx_Rx();
    SPI_DMA_Start_Tx_Rx(1);
    /* This loop takes a time. Insert rot_rdq() here for multitask envilonment. */
    SPI_DMA_Wait_For_Rx();
  } while (gu8BuffSPIRx[0] != 0xFF && drvSystick_Running()); /* Wait for card goes ready or timeout */

  return (gu8BuffSPIRx[0] == 0xFF) ? 1 : 0;
}

/*-----------------------------------------------------------------------*/
/* Deselect card and release SPI                                         */
/*-----------------------------------------------------------------------*/

static void _deselect(void) {
  // SPI_DMA_Tx->MADDR = (ptrdiff_t)gu8BuffSPITx;
  // SPI_DMA_Rx->MADDR = (ptrdiff_t)gu8BuffSPIRx;
  CS_HIGH(); /* Set CS# high */
  GPIOC->CFGLR = SPI_GPIO_MOSI_HIGH;
  // gu8BuffSPITx[0] = 0xFF;
  // DMA1->INTFCR = SPI_DMA_FLAGS_Tx_Rx;
  SPI_DMA_Start_Tx_Rx(1); /* Dummy clock (force DO hi-z for multiple slave SPI) */
  SPI_DMA_Wait_For_Rx();
}

/*-----------------------------------------------------------------------*/
/* Select card and wait for ready                                        */
/*-----------------------------------------------------------------------*/

static int _select(void) /* 1:OK, 0:Timeout */
{
  // SPI_DMA_Tx->MADDR = (ptrdiff_t)gu8BuffSPITx;
  CS_LOW(); /* Set CS# low */
  GPIOC->CFGLR = SPI_GPIO_MOSI_HIGH;
  // gu8BuffSPITx[0] = 0xFF;
  // DMA1->INTFCR = SPI_DMA_FLAGS_Tx_Rx;
  SPI_DMA_Start_Tx_Rx(1); /* Dummy clock (force DO enabled) */
  SPI_DMA_Wait_For_Rx();

  if (wait_ready(500))
    return 1; /* Wait for card ready */

  _deselect();
  return 0; /* Timeout */
}

/*-----------------------------------------------------------------------*/
/* Send a command packet to the MMC                                      */
/*-----------------------------------------------------------------------*/
static BYTE send_cmd(          /* Return value: R1 resp (bit7==1:Failed to send) */
                     BYTE cmd, /* Command index */
                     DWORD arg /* Argument */
) {
  BYTE n, res;

  if (cmd & 0x80) { /* Send a CMD55 prior to ACMD<n> */
    cmd &= 0x7F;
    res = send_cmd(CMD55, 0);
    if (res > 1)
      return res;
  }

  /* Select the card and wait for ready except to stop multiple block read */
  if (cmd != CMD12) {
    _deselect();
    if (!_select())
      return 0xFF;
  } else {
    // SPI_DMA_Tx->MADDR = (ptrdiff_t)gu8BuffSPITx;
    // SPI_DMA_Rx->MADDR = (ptrdiff_t)gu8BuffSPIRx;
  }

  GPIOC->CFGLR = SPI_GPIO_DEFAULT;

  /* Send command packet */
  gu8BuffSPITx[0] = (0x40 | cmd);        /* Start + command index */
  gu8BuffSPITx[1] = ((BYTE)(arg >> 24)); /* Argument[31..24] */
  gu8BuffSPITx[2] = ((BYTE)(arg >> 16)); /* Argument[23..16] */
  gu8BuffSPITx[3] = ((BYTE)(arg >> 8));  /* Argument[15..8] */
  gu8BuffSPITx[4] = ((BYTE)arg);         /* Argument[7..0] */
  // *(uint32_t*)(&gu8BuffSPITx[1]) = arg;
  gu8BuffSPITx[5] = 0x01; /* Dummy CRC + Stop */
  if (cmd == CMD0)
    gu8BuffSPITx[5] = 0x95; /* Valid CRC for CMD0(0) */
  if (cmd == CMD8)
    gu8BuffSPITx[5] = 0x87; /* Valid CRC for CMD8(0x1AA) */
  SPI_DMA_Start_Tx_Rx(6);
  SPI_DMA_Wait_For_Rx();

  GPIOC->CFGLR = SPI_GPIO_MOSI_HIGH;
  /* Receive command resp */
  if (cmd == CMD12) { /*gu8BuffSPITx[0]=0xFF;*/
    SPI_DMA_Start_Tx_Rx(1);
  }; /* Diacard following one byte when CMD12 */
  n = 10; /* Wait for response (10 bytes max) */
  do {
    // res = xchg_spi(0xFF);
    SPI_DMA_Start_Tx_Rx(1);
    SPI_DMA_Wait_For_Rx();
    res = gu8BuffSPIRx[0];
  } while ((res & 0x80) && --n);
  // gu8BuffSPITx[0]=gu8BuffSPITx[1]=gu8BuffSPITx[2]=gu8BuffSPITx[3]=gu8BuffSPITx[4]=0xFF;
  // gu8BuffSPITx[5]=gu8BuffSPITx[6]=gu8BuffSPITx[7]=gu8BuffSPITx[8]=gu8BuffSPITx[9]=0xFF;
  // SPI_DMA_Start_Tx_Rx(10);
  // SPI_DMA_Wait_For_Rx();
  // for(n = 0; n < 10; n++)
  // {
  //     res = gu8BuffSPIRx[n];
  //     if(!(res & 0x80)) break;
  // }

  printf_("send_cmd %d: %02x\n", cmd, res);
  return res; /* Return received response */
}

/*-----------------------------------------------------------------------*/
/* Receive a data packet from the MMC                                    */
/*-----------------------------------------------------------------------*/
static int rcvr_datablock(            /* 1:OK, 0:Error */
                          BYTE *buff, /* Data buffer */
                          UINT btr    /* Data block length (byte) */
) {
  // previously `send_cmd` or `_dselect` should have selected gu8BuffSPITx/Rx as our working DMA buffer
  GPIOC->CFGLR = SPI_GPIO_MOSI_HIGH;
  drvSystick_Setms(200);
  do { /* Wait for DataStart token in timeout of 200ms */
    // DMA1->INTFCR = SPI_DMA_FLAGS_Tx_Rx;
    SPI_DMA_Start_Tx_Rx(1);
    SPI_DMA_Wait_For_Rx();
    /* This loop will take a time. Insert rot_rdq() here for multitask envilonment. */
  } while ((gu8BuffSPIRx[0] == 0xFF) && drvSystick_Running());
  if (gu8BuffSPIRx[0] != 0xFE) {
    printf_("rcvr_datablock:%02x\n", gu8BuffSPIRx[0]);
    return 0;
  } /* Function fails if invalid DataStart token or timeout */

  SPI_DMA_Tx->MADDR = (ptrdiff_t)buff;
  SPI_DMA_Rx->MADDR = (ptrdiff_t)buff;
  // rcvr_spi_multi(buff, btr);		/* Store trailing data to the buffer */
  SPI_DMA_Start_Tx_Rx(btr);
  SPI_DMA_Wait_For_Rx();

  SPI_DMA_Tx->MADDR = (ptrdiff_t)gu8BuffSPITx;
  SPI_DMA_Rx->MADDR = (ptrdiff_t)gu8BuffSPIRx;
  SPI_DMA_Start_Tx_Rx(2);
  //_dump_buffer(buff, 512/16);
  SPI_DMA_Wait_For_Rx();
  // xchg_spi(0xFF); xchg_spi(0xFF);			/* Discard CRC */

  return 1; /* Function succeeded */
}

/*-----------------------------------------------------------------------*/
/* Send a data packet to the MMC                                         */
/*-----------------------------------------------------------------------*/
static int xmit_datablock(                  /* 1:OK, 0:Failed */
                          const BYTE *buff, /* Ponter to 512 byte data to be sent */
                          BYTE token        /* Token */
) {
  BYTE resp;

  if (!wait_ready(500))
    return 0; /* Wait for card ready */

  // previously `wait_ready` should have selected gu8BuffSPITx/Rx as our working DMA buffer
  GPIOC->CFGLR = SPI_GPIO_DEFAULT;
  gu8BuffSPITx[0] = token;
  SPI_DMA_Start_Tx_Rx(1);
  SPI_DMA_Wait_For_Rx();
  // xchg_spi(token);					/* Send token */
  if (token != 0xFD) { /* Send data if token is other than StopTran */
    SPI_DMA_Tx->MADDR = (ptrdiff_t)buff;
    SPI_DMA_Rx->MADDR = (ptrdiff_t)buff;
    SPI_DMA_Start_Tx_Rx(512);
    SPI_DMA_Wait_For_Rx();
    // xmit_spi_multi(buff, 512);		/* Data */

    GPIOC->CFGLR = SPI_GPIO_MOSI_HIGH;
    SPI_DMA_Tx->MADDR = (ptrdiff_t)gu8BuffSPITx;
    SPI_DMA_Rx->MADDR = (ptrdiff_t)gu8BuffSPIRx;
    SPI_DMA_Start_Tx_Rx(3);
    SPI_DMA_Wait_For_Rx();
    // xchg_spi(0xFF); xchg_spi(0xFF);	/* Dummy CRC */

    resp = gu8BuffSPIRx[2];
    // resp = xchg_spi(0xFF);				/* Receive data resp */
    if ((resp & 0x1F) != 0x05)
      return 0; /* Function fails if the data packet was not accepted */
  }
  return 1;
}

/*-----------------------------------------------------------------------*/
/* Initialize a Drive                                                    */
/*-----------------------------------------------------------------------*/
DSTATUS disk_initialize(BYTE pdrv /* Physical drive nmuber to identify the drive */
) {
  BYTE n, cmd, ty; //, ocr[4];

  if (pdrv)
    return STA_NOINIT;

  _spi_init();

  // SPI_DMA_Tx->MADDR = (ptrdiff_t)gu8BuffSPITx;
  // SPI_DMA_Rx->MADDR = (ptrdiff_t)gu8BuffSPIRx;

  /* Send 80 dummy clocks */
  // gu8BuffSPITx[0]=gu8BuffSPITx[1]=gu8BuffSPITx[2]=gu8BuffSPITx[3]=gu8BuffSPITx[4]=0xFF;
  // gu8BuffSPITx[5]=gu8BuffSPITx[6]=gu8BuffSPITx[7]=gu8BuffSPITx[8]=gu8BuffSPITx[9]=0xFF;
  GPIOC->CFGLR = SPI_GPIO_MOSI_HIGH;
  SPI_DMA_Start_Tx_Rx(10);
  SPI_DMA_Wait_For_Rx();

  ty = 0;
  if (send_cmd(CMD0, 0) == 1) {       /* Put the card SPI/Idle state */
    drvSystick_Setms(1000);           /* Initialization timeout = 1 sec */
    if (send_cmd(CMD8, 0x1AA) == 1) { /* SDv2? */
                                      // gu8BuffSPITx[0]=gu8BuffSPITx[1]=gu8BuffSPITx[2]=gu8BuffSPITx[3]=0xFF;
      GPIOC->CFGLR = SPI_GPIO_MOSI_HIGH;
      SPI_DMA_Start_Tx_Rx(4);
      SPI_DMA_Wait_For_Rx();
      // for (n = 0; n < 4; n++) ocr[n] = xchg_spi(0xFF);	/* Get 32 bit return value of R7 resp */

      if (gu8BuffSPIRx[2] == 0x01 && gu8BuffSPIRx[3] == 0xAA) { /* Is the card supports vcc of 2.7-3.6V? */
        while (drvSystick_Running() && send_cmd(ACMD41, 1UL << 30))
          ;                                                    /* Wait for end of initialization with ACMD41(HCS) */
        if (drvSystick_Running() && send_cmd(CMD58, 0) == 0) { /* Check CCS bit in the OCR */
          // gu8BuffSPITx[0]=gu8BuffSPITx[1]=gu8BuffSPITx[2]=gu8BuffSPITx[3]=0xFF;
          GPIOC->CFGLR = SPI_GPIO_MOSI_HIGH;
          SPI_DMA_Start_Tx_Rx(4);
          SPI_DMA_Wait_For_Rx();
          // for (n = 0; n < 4; n++) ocr[n] = xchg_spi(0xFF);
          ty = (gu8BuffSPIRx[0] & 0x40) ? CT_SDC2 | CT_BLOCK : CT_SDC2; /* Card id SDv2 */
        }
      }
    } else {                          /* Not SDv2 card */
      if (send_cmd(ACMD41, 0) <= 1) { /* SDv1 or MMC? */
        ty = CT_SDC1;
        cmd = ACMD41; /* SDv1 (ACMD41(0)) */
      } else {
        ty = CT_MMC3;
        cmd = CMD1; /* MMCv3 (CMD1(0)) */
      }
      while (drvSystick_Running() && send_cmd(cmd, 0))
        ;                                                     /* Wait for end of initialization */
      if (drvSystick_TimedOut() || send_cmd(CMD16, 512) != 0) /* Set block length: 512 */
        ty = 0;
    }
  }
  CardType = ty; /* Card type */
  _deselect();

  if (ty) { /* OK */
    // FCLK_FAST();			/* Set fast clock */
    Stat &= ~STA_NOINIT; /* Clear STA_NOINIT flag */
  } else {               /* Failed */
    Stat = STA_NOINIT;
  }

  uint8_t csd[16];
  if (send_cmd(CMD9, 0) == 0 && rcvr_datablock(csd, 16)) { /* READ_CSD */
    // _dump_buffer (csd, 1);
    /*Read TRANS_SPEED <= CSD[103:96] a.k.a csd[3]*/
    // csd[3][2:0]                   csd[3][6:3]
    // +-------+-------------------+  +-------+------------+
    // | Code  | Transfer Rate Unit|  | Code  | Multiplier |
    // +-------+-------------------+  +-------+------------+
    // | 0x0   | 100 kbit/s        |  | 0x00  | Reserved   |
    // | 0x1   | 1 Mbit/s          |  | 0x08  | 1.0        |
    // | 0x2   | 10 Mbit/s         |  | 0x10  | 1.2        |
    // | 0x3   | 100 Mbit/s        |  | 0x18  | 1.3        |
    // | 0x4   | Reserved          |  | 0x20  | 1.5        |
    // | 0x5   | Reserved          |  | 0x28  | 2.0        |
    // | 0x6   | Reserved          |  | 0x30  | 2.5        |
    // | 0x7   | Reserved          |  | 0x38  | 3.0        |
    // +-------+-------------------+  | 0x40  | 3.5        |
    //                                | 0x48  | 4.5        |
    //                                | 0x50  | 4.0        |
    //                                | 0x58  | 5.0        |
    //                                | 0x60  | 5.5        |
    //                                | 0x68  | 6.0        |
    //                                | 0x70  | 7.0        |
    //                                | 0x78  | 8.0        |
    //                                +-------+------------+
    //
    // +--------+--------+-------+--------+--------+--------+--------+
    // | HSRXEN | Scaler | MHz   |  Hex   |  Hex   |  bps   |  bps   |
    // |        |        |       |  Min   |  Max   |  Min   |  Max   |
    // +--------+--------+-------+--------+--------+--------+--------+
    // |   1    |   2    |  36   |  0x28  |  0x28  | 40Mb   | 40Mb   |
    // |   0    |   2    |  24   |  0x25  |  0x27  | 25Mb   | 35Mb   |
    // |   0    |   4    |  12   |  0x21  |  0x24  | 12Mb   | 20Mb   |
    // |   0    |   8    |   6   |  0x1C  |  0x20  |  6Mb   | 10Mb   |
    // |   0    |  16    |   3   |  0x16  |  0x1B  |  3Mb   | 5.5Mb  |
    // |   0    |  32    |  1.5  |  0x13  |  0x15  | 1.5Mb  | 2.5Mb  |
    // |   0    |  64    | 0.75  |  0x0E  |  0x12  | 800kb  | 1.3Mb  |
    // |   0    | 128    |0.375  |  0x08  |  0x0D  | 400kb  | 700kb  |
    // |   0    | 256    |0.1875 |  0x00  |  0x07  | 100kb  | 350kb  |
    // +--------+--------+-------+--------+--------+--------+--------+
    n = (csd[15 - 12] >> 3) | ((csd[15 - 12] << 4) & 0x70);
    printf_("TRANS_SPEED: %x->%x\n", csd[15 - 12], n);
    SPI1->CTLR1 &= ~(0b111 << 3);
    // using `if` instead of `if-else-if-else` saves ROM by 140 bytes at an expense of little more execution time.
    if (n >= 0x28)
      /*SPI1->CTLR1 |= SPI_BaudRatePrescaler_2;*/ SPI1->HSCR = 1;
    if ((n >= 0x25) && (n <= 0x27))
      SPI1->CTLR1 |=
          SPI_BaudRatePrescaler_4; // TODO: should be pre-scaler 2, but some reason data write corrupts @ 25MHz
    if ((n >= 0x21) && (n <= 0x24))
      SPI1->CTLR1 |= SPI_BaudRatePrescaler_4;
    if ((n >= 0x1C) && (n <= 0x20))
      SPI1->CTLR1 |= SPI_BaudRatePrescaler_8;
    if ((n >= 0x16) && (n <= 0x1B))
      SPI1->CTLR1 |= SPI_BaudRatePrescaler_16;
    if ((n >= 0x13) && (n <= 0x15))
      SPI1->CTLR1 |= SPI_BaudRatePrescaler_32;
    if ((n >= 0x0E) && (n <= 0x12))
      SPI1->CTLR1 |= SPI_BaudRatePrescaler_64;
    if ((n >= 0x08) && (n <= 0x0D))
      SPI1->CTLR1 |= SPI_BaudRatePrescaler_128;
    if (n <= 0x07)
      SPI1->CTLR1 |= SPI_BaudRatePrescaler_256;
    // SPI1->CTLR1 &= ~(0b111<<3);SPI1->CTLR1 |= n;
  }

  printf_("disk_init: %u\n", Stat);
  return Stat;
}

/*-----------------------------------------------------------------------*/
/* Get disk status                                                       */
/*-----------------------------------------------------------------------*/
DSTATUS disk_status(BYTE drv /* Physical drive number (0) */
) {
  if (drv)
    return STA_NOINIT; /* Supports only drive 0 */

  return Stat; /* Return disk status */
}

/*-----------------------------------------------------------------------*/
/* Read sector(s)                                                        */
/*-----------------------------------------------------------------------*/
DRESULT disk_read(BYTE drv,     /* Physical drive number (0) */
                  BYTE *buff,   /* Pointer to the data buffer to store read data */
                  LBA_t sector, /* Start sector number (LBA) */
                  UINT count    /* Number of sectors to read (1..128) */
) {
  DWORD sect = (DWORD)sector;

  if (drv || !count)
    return RES_PARERR; /* Check parameter */
  if (Stat & STA_NOINIT)
    return RES_NOTRDY; /* Check if drive is ready */

  if (!(CardType & CT_BLOCK))
    sect *= 512; /* LBA ot BA conversion (byte addressing cards) */

  printf_("rd sect:%x\n", sect);

  if (count == 1) { /* Single sector read */
    // uint8_t n = 3;
    // while (n--) {
    //     if (send_cmd (CMD17, sect) == 0) {
    //         if (rcvr_datablock (buff, 512))
    //             count = 0;
    //         break;
    //     }
    // }
    if ((send_cmd(CMD17, sect) == 0) /* READ_SINGLE_BLOCK */
        && rcvr_datablock(buff, 512)) {
      count = 0;
    }
  } else {                            /* Multiple sector read */
    if (send_cmd(CMD18, sect) == 0) { /* READ_MULTIPLE_BLOCK */
      do {
        if (!rcvr_datablock(buff, 512))
          break;
        buff += 512;
      } while (--count);
      send_cmd(CMD12, 0); /* STOP_TRANSMISSION */
    }
  }
  _deselect();

  return count ? RES_ERROR : RES_OK; /* Return result */
}

/*-----------------------------------------------------------------------*/
/* Write sector(s)                                                       */
/*-----------------------------------------------------------------------*/
DRESULT disk_write(BYTE drv,         /* Physical drive number (0) */
                   const BYTE *buff, /* Ponter to the data to write */
                   LBA_t sector,     /* Start sector number (LBA) */
                   UINT count        /* Number of sectors to write (1..128) */
) {
  DWORD sect = (DWORD)sector;

  if (drv || !count)
    return RES_PARERR; /* Check parameter */
  if (Stat & STA_NOINIT)
    return RES_NOTRDY; /* Check drive status */
  if (Stat & STA_PROTECT)
    return RES_WRPRT; /* Check write protect */

  if (!(CardType & CT_BLOCK))
    sect *= 512; /* LBA ==> BA conversion (byte addressing cards) */

  if (count == 1) {                  /* Single sector write */
    if ((send_cmd(CMD24, sect) == 0) /* WRITE_BLOCK */
        && xmit_datablock(buff, 0xFE)) {
      count = 0;
    }
  } else { /* Multiple sector write */
    if (CardType & CT_SDC)
      send_cmd(ACMD23, count);        /* Predefine number of sectors */
    if (send_cmd(CMD25, sect) == 0) { /* WRITE_MULTIPLE_BLOCK */
      do {
        if (!xmit_datablock(buff, 0xFC))
          break;
        buff += 512;
      } while (--count);
      if (!xmit_datablock(0, 0xFD))
        count = 1; /* STOP_TRAN token */
    }
  }
  _deselect();

  return count ? RES_ERROR : RES_OK; /* Return result */
}

/*-----------------------------------------------------------------------*/
/* Miscellaneous drive controls other than data read/write               */
/*-----------------------------------------------------------------------*/
DRESULT disk_ioctl(BYTE drv,  /* Physical drive number (0) */
                   BYTE cmd,  /* Control command code */
                   void *buff /* Pointer to the conrtol data */
) {
#if 0
    DRESULT res;
    BYTE n, csd[16];
    DWORD st, ed, csize;
    LBA_t *dp;


    if (drv)
        return RES_PARERR; /* Check parameter */
    if (Stat & STA_NOINIT)
        return RES_NOTRDY; /* Check if drive is ready */

    res = RES_ERROR;

    switch (cmd) {
    case CTRL_SYNC: /* Wait for end of internal write process of the drive */
        if (_select())
            res = RES_OK;
        break;

    case GET_SECTOR_COUNT:            /* Get drive capacity in unit of sector (DWORD) */
        if ((send_cmd (CMD9, 0) == 0) && rcvr_datablock (csd, 16)) {
            if ((csd[0] >> 6) == 1) { /* SDC CSD ver 2 */
                csize = csd[9] + ((WORD)csd[8] << 8) + ((DWORD)(csd[7] & 63) << 16) + 1;
                *(LBA_t *)buff = csize << 10;
            } else { /* SDC CSD ver 1 or MMC */
                n = (csd[5] & 15) + ((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) + 2;
                csize = (csd[8] >> 6) + ((WORD)csd[7] << 2) + ((WORD)(csd[6] & 3) << 10) + 1;
                *(LBA_t *)buff = csize << (n - 9);
            }
            res = RES_OK;
        }
        break;

    case GET_BLOCK_SIZE:                     /* Get erase block size in unit of sector (DWORD) */
        if (CardType & CT_SDC2) {            /* SDC ver 2+ */
            if (send_cmd (ACMD13, 0) == 0) { /* Read SD status */
                GPIOC->CFGLR = SPI_GPIO_MOSI_HIGH;
                SPI_DMA_Start_Tx_Rx (1);
                SPI_DMA_Wait_For_Rx();          // xchg_spi(0xFF);
                if (rcvr_datablock (csd, 16)) { /* Read partial block */
                    SPI_DMA_Start_Tx_Rx (10);
                    SPI_DMA_Wait_For_Rx();
                    SPI_DMA_Start_Tx_Rx (10);
                    SPI_DMA_Wait_For_Rx();
                    SPI_DMA_Start_Tx_Rx (10);
                    SPI_DMA_Wait_For_Rx();
                    SPI_DMA_Start_Tx_Rx (10);
                    SPI_DMA_Wait_For_Rx();
                    SPI_DMA_Start_Tx_Rx (8);
                    SPI_DMA_Wait_For_Rx();
                    // for (n = 64 - 16; n; n--) xchg_spi(0xFF);	/* Purge trailing data */
                    *(DWORD *)buff = 16UL << (csd[10] >> 4);
                    res = RES_OK;
                }
            }
        } else {                                                         /* SDC ver 1 or MMC */
            if ((send_cmd (CMD9, 0) == 0) && rcvr_datablock (csd, 16)) { /* Read CSD */
                if (CardType & CT_SDC1) {                                /* SDC ver 1.XX */
                    *(DWORD *)buff = (((csd[10] & 63) << 1) + ((WORD)(csd[11] & 128) >> 7) + 1) << ((csd[13] >> 6) - 1);
                } else {                                                 /* MMC */
                    *(DWORD *)buff = ((WORD)((csd[10] & 124) >> 2) + 1) * (((csd[11] & 3) << 3) + ((csd[11] & 224) >> 5) + 1);
                }
                res = RES_OK;
            }
        }
        break;

    case CTRL_TRIM: /* Erase a block of sectors (used when _USE_ERASE == 1) */
        if (!(CardType & CT_SDC))
            break;  /* Check if the card is SDC */
        if (disk_ioctl (drv, MMC_GET_CSD, csd))
            break;  /* Get CSD */
        if (!(csd[10] & 0x40))
            break;  /* Check if ERASE_BLK_EN = 1 */
        dp = buff;
        st = (DWORD)dp[0];
        ed = (DWORD)dp[1]; /* Load sector block */
        if (!(CardType & CT_BLOCK)) {
            st *= 512;
            ed *= 512;
        }
        if (send_cmd (CMD32, st) == 0 && send_cmd (CMD33, ed) == 0 && send_cmd (CMD38, 0) == 0 && wait_ready (30000)) { /* Erase sector block */
            res = RES_OK;                                                                                               /* FatFs does not check result of this command */
        }
        break;

        /* Following commands are never used by FatFs module */

    case MMC_GET_TYPE: /* Get MMC/SDC type (BYTE) */
        *(BYTE *)buff = CardType;
        res = RES_OK;
        break;

    case MMC_GET_CSD:                                                       /* Read CSD (16 bytes) */
        if (send_cmd (CMD9, 0) == 0 && rcvr_datablock ((BYTE *)buff, 16)) { /* READ_CSD */
            res = RES_OK;
        }
        break;

    case MMC_GET_CID:                                                        /* Read CID (16 bytes) */
        if (send_cmd (CMD10, 0) == 0 && rcvr_datablock ((BYTE *)buff, 16)) { /* READ_CID */
            res = RES_OK;
        }
        break;

    case MMC_GET_OCR:                   /* Read OCR (4 bytes) */
        if (send_cmd (CMD58, 0) == 0) { /* READ_OCR */
            SPI_DMA_Tx->MADDR = (ptrdiff_t)buff;SPI_DMA_Rx->MADDR = (ptrdiff_t)buff;
            GPIOC->CFGLR = SPI_GPIO_MOSI_HIGH;
            SPI_DMA_Start_Tx_Rx (4);
            SPI_DMA_Wait_For_Rx();
            // for (n = 0; n < 4; n++) *(((BYTE*)buff) + n) = xchg_spi(0xFF);
            SPI_DMA_Tx->MADDR = (ptrdiff_t)gu8BuffSPITx;SPI_DMA_Rx->MADDR = (ptrdiff_t)gu8BuffSPIRx;
            res = RES_OK;
        }
        break;

    case MMC_GET_SDSTAT:                 /* Read SD status (64 bytes) */
        if (send_cmd (ACMD13, 0) == 0) { /* SD_STATUS */
            gu8BuffSPITx[0] = 0xFF;
            SPI_DMA_Start_Tx_Rx (1);
            SPI_DMA_Wait_For_Rx();  // xchg_spi(0xFF);
            if (rcvr_datablock ((BYTE *)buff, 64))
                res = RES_OK;
        }
        break;

    default:
        res = RES_PARERR;
    }

    _deselect();

    printf_("disk_ioctl[%d]:%d\n", cmd, res);
    return res;
#else
  printf_("disk_ioctl[%d]\n", cmd);
  if ((cmd == CTRL_SYNC) && _select()) {
    return RES_OK;
  }
  return RES_ERROR;
#endif
}
