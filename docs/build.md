# To Build...

## RISCV gcc toolchain

Download and extract either one of those based on your Ubuntu version and ensure that `bin` folder of the extracted path is in your environment variable's `PATH` list

[riscv64-uclibc-ng-ubuntu-22.04-gcc.tar.xz](https://github.com/riscv-collab/riscv-gnu-toolchain/releases/download/2026.02.13/riscv64-uclibc-ng-ubuntu-22.04-gcc.tar.xz)
[riscv64-uclibc-ng-ubuntu-24.04-gcc.tar.xz](https://github.com/riscv-collab/riscv-gnu-toolchain/releases/download/2026.02.13/riscv64-uclibc-ng-ubuntu-24.04-gcc.tar.xz)

## Compilation

Walk through the [Makefile]() and [app_base.h]() to understand the available various pre-compilation options.
by the way the default setting is the optimized version that is suitable for most of the use case(s)

### To compile without any debug print

> [!NOTE]
> Debug print uses semi-hosting mechanism to send the log that are invoked via printf(...) calls, so you need debugger connected to view the logs

`make -C src/app/uart_logger DEBUG=0`

### To compile with debug print

`make -C src/app/uart_logger`

### To generate intellisense support file

`make -C src/app/uart_logger clangd`

### To clean

`make -C src/app/uart_logger clangd_clean clean`

## [Pre-built binaries](https://github.com/123swk123/uart_data_logger/releases)

