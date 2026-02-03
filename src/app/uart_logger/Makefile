
PREFIX=riscv32-unknown-linux-uclibc

all : build

TARGET:=uart_logger
TARGET_MCU:=CH32V003

ADDITIONAL_C_FILES += c_scan.c drv_sd_mmc.c ff.c drv_psuedo_rtc.c

DEBUG:=1

# by default log file name is simple running number
ifeq ($(APP_CONF_RUNNING_LOG), 0)
# 	change the log file naming to DDhhmm.log instead of <running no>.log
#	for running number log file name, you need to have a file called `head`
	EXTRA_CFLAGS+=-DAPP_CONF_RUNNING_LOG=0
endif

# by default periodic sync is enabled
ifeq ($(APP_CONF_PERIODIC_SYNC), 0)
	EXTRA_CFLAGS+=-DAPP_CONF_PERIODIC_SYNC=0
endif

include ../../ch32fun/ch32fun.mk

flash : cv_flash
clean : cv_clean
