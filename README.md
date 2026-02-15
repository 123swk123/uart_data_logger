# UART data logger to SD card

> [!NOTE]
> *Work in progress...*

This is a simple UART data logger, logger saves the data to FAT(12/16/32) formatted SD card.

## Schematic

![Schematic](docs/schematic.png)

## Preparations

- Any SDHC card should work, but I have tested them upto 8GB
- Format the card as FAT12/16/32
- copy the [config.txt](src/card_config/config.txt) and [head](src/card_config/head) to root of the SD card

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
| Brief pulsing |  Actively logging the received UART data |

### config.txt

first 2 lines are comment and they are skipped during the configuration process

3rd line describes the serial port configuration.
> `s<no>:baud rate,data bits<8,9>,parity<0,1,2>,stop bits<1,2>,output formatter<c,d,x,X>,seq start char,seq end char`
```
s<no>:    Serial port number, starting from 1
range:    1 to Maximum no.of UART HW supported by the controller, in case of CH32V003 its just 1.
Example:  s1
```
```
baud rate:  UART logger baud rate
range:      1200 ~ 300000 (check CH32v003 reference manual to calculate the percentage error due to divider round off)
Example:    115200
```
```
data bits:  UART logger data bits per frame
range:      8, 9
Example:    8
```
```
parity:   UART data partiy enable/disable
range:    0 - No parity (disabled)
          1 - odd parity
          2 - even parity
Example:  0
```
```
stop bits:  No.of stop bits per frame
range:      1, 2
Example:    1
```
```
output formatter: logger output encode format
range:            c - store the UART data in a transparent 8-bit ASCII raw
                  u - store the UART data as unsigned decimal value
                  x - store the UART data as hexadecimal value
                  X - store the UART data as hexadecimal value (all caps)
Example:          x
```
```
seq start char: a 9-bit unsigned integer. Upon reciving this character logger notes the event time stamp at start of the row/line
                and the logger continues to log the sub-sequent received data in the same row/line as per the output format
                Note: This seq start character will not be logged.
                      time stamp format: hour.minute.seconds.milli second (resolution: 100milli)
range:          0 ~ 511
Example:        13 (\r)
```
```
seq stop char:  a 9-bit unsigned integer. Upon reciving this character logger closes the current row/line with \r\n and
                creates a new row/line, then the logger waits for seq start character.
                Note: seq stop character will not be logged; Seq start and stop char can be the same.
range:          0 ~ 511
Example:        10 (\n)
```

4th line describes the time offset information.
> `t:year,month,day,hour,minute`
```
t:  start of time offset definition

year:     unsigned 16bit integer
range:    > 1980
Example:  2026

month:    unsigned 16bit integer
range:    1 ~ 12
Example:  2

day:      unsigned 16bit integer
range:    1 ~ 31
Example:  10

hour:     unsigned 16bit integer
range:    0 ~ 23
Example:  22

minute:   unsigned 16bit integer
range:    0 ~ 59
Example:  0
```

### Reasons for logger error

1. SD card not present
2. SD card format not recognized, so unable to mount
3. missing [config.txt](src/card_config/config.txt) in the root of SD card
4. missing [head](src/card_config/head) in the root of SD card
5. Invalid / Un-recognizable contents in [config.txt](src/card_config/config.txt) or [head](src/card_config/head)

## How to build

TODO

## Tested features

> [!WARNING]
> Not all aspects of the functionality was tested (This was a side project)

- SDHC card upto 8GB
- Tested baud
  - 115200
  - 750000
  - 921600
- Data bits
  - 8
  - 9
- Parity
  - None
- Stop bits
  - 2
- Logging format
  - `c` - ASCII characters
  - `u` - unsigned decimal
  - `x`/`X` - Hexadecimal
- Seq start/Seq stop
  - `\r`
  - `0xFF`
  - `0x1CC`

### Sample log output
[Test log results](docs/log_output.md)

### Thanks to various open source code base helped realizing the project

- Framework [ch32fun](https://github.com/cnlohr/ch32fun)
- SD Card driver [modified sample driver from ChaN](https://elm-chan.org/docs/mmc/mmc_e.html)
- FAT(12/16/32) library [ChaN's FatFs](https://elm-chan.org/fsw/ff/)
- [Aleksej Muratov's sscanf](https://github.com/MuratovAS/mini-scanf) modified for this project to further reduce the ROM size
