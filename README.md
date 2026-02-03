# UART data logger to SD card

*Work in progress...*

This is a simple UART data logger, logger saves the data to FAT(12/16/32) formatted SD card.

## Schematic

![Schematic](docs/schematic.png)

## How to use the logger

- After power-on the status LED will be OFF
- start/stop button is a simple toggle switch
    - press once to start logging
    - press again to stop logging
- Always remove the card when status LED is OFF.

| Status LED | Remarks |
|--|--|
| OFF | Logger stopped, free to remove the SD card after powering down |
| ON | Logger running, do not remove the card |
| Constant blinking | Logger error |
| Brief pulsing | received UART data and actively logging |

### Reasons for logger error

1. SD card not present
2. SD card format not recognized, so unable to mount
3. missing [config.txt](src/card_config/config.txt) in the root of SD card
4. missing [head](src/card_config/head) in the root of SD card
5. Invalid / Un-recognizable contents in [config.txt](src/card_config/config.txt) or [head](src/card_config/head)

## How to build

TODO

### Thanks to various open source code base helped to realize the project

- Framework [ch32fun](https://github.com/cnlohr/ch32fun)
- SD Card driver [modified sample driver from ChaN](https://elm-chan.org/docs/mmc/mmc_e.html)
- FAT(12/16/32) library [ChaN's FatFs](https://elm-chan.org/fsw/ff/)
- [Aleksej Muratov's sscanf](https://github.com/MuratovAS/mini-scanf) modified for this project to further reduce the ROM size

