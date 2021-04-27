# Software Serial

This page logs my experiments getting the [EspSoftwareSerial](https://github.com/plerup/espsoftwareserial).
library working on my ESP8266.

## Introduction

### Setup

I have a smart electricity meter, following the 
[Dutch standards](https://www.netbeheernederland.nl/_upload/Files/Slimme_meter_15_32ffe3cc38.pdf).

Such a meter has a serial port, transmitting meter stats (every 10 seconds).
The meter has no RX, only TX, and the TX has its bits flipped.

I'm using an ESP8266 as host; it can upload the received e-meter stats to some cloud service using its WiFi stack.

![e-Meter system diagram](emeter-system.drawio.png)

Looking at the ESP8266, I want to use the (hardware) UART that is mapped to the USB for development and debugging.
The ESP8266 does not have a second UART (there is a "half" TX only UART). 
Besides, the data coming from the e-Meter is inverted anyhow 
(see [logical levels](https://www.netbeheernederland.nl/_upload/Files/Slimme_meter_15_32ffe3cc38.pdf#page=6) SPACE "0" as > 4V, MARK "1" as < 1V)

So, my conclusion was that using a bit-banged serial port makes sense here.

And fortunately one exists: [EspSoftwareSerial](https://github.com/plerup/espsoftwareserial).
We do not even have to download it, it is available in the stock ESP8266 board package.

### Problem

The e-Meter sends stats every 10 seconds.
The stats are send as an textual page, each line containing a key value pair.
An example of a line in the page is `1-0:1.8.1(025707.312*kWh)`.
The `1-0:1.8.1` is the key, and `025707.312*kWh` the value.
The key denotes the "Meter Reading electricity delivered to client (low tariff) in 0,001 kWh" 
as specified by the [standard](https://www.netbeheernederland.nl/_upload/Files/Slimme_meter_15_32ffe3cc38.pdf#page=17).

The [standard](https://www.netbeheernederland.nl/_upload/Files/Slimme_meter_15_32ffe3cc38.pdf#page=7)
also specifies the whole page layout:

```
/AAA5AAAAA

...line...
...line...
...
...line...
!XXXX
```

A notable aspect is that the `XXXX` is a CRC16 of all bytes from (including) `/` up to (including `!`).

My problem is that the SoftwareSerial implementation looses so many bits that the CRC16 very often does not match.

It seems so bad that the current (April 2021) version of ESP8266 board package has a failure of 100%.


## Experiment

|  ESP  | SwSer | pass | fail | count | rate |
|:-----:|:-----:|:----:|:----:|:-----:|:----:|
| 2.4.0 |  1.0  |  50  |  16  |   66  |  76% |
| 2.3.0 |  1.0  |  50  |   1  |   51  |  98% |


## Appendix 

### ESP 2.4.0

```
Welcome to TestSwSer
ver: ESP8266 board 2_4_0
ver: SoftwareSerial 1.0
cpu: init (160MHz)
crc: init (test pass)
p1 : init
p1 : telegram 1 received (864 bytes)
p1 : telegram 2 received (864 bytes)
p1 : telegram 3 received (864 bytes)
p1 : telegram 4 received (864 bytes)
p1 : telegram 5 received (864 bytes)
p1 : telegram 6 received (864 bytes)
p1 : telegram 7 received (864 bytes)
p1 : telegram 8 received (864 bytes)
p1 : error: crc mismatch #539F=='F114'
p1 : error: telegram has CRC error, discarding 862 bytes
p1 : telegram 9 received (864 bytes)
p1 : telegram 10 received (864 bytes)
p1 : telegram 11 received (864 bytes)
p1 : error: crc mismatch #37DA=='C0B9'
p1 : error: telegram has CRC error, discarding 863 bytes
p1 : telegram 12 received (864 bytes)
p1 : telegram 13 received (864 bytes)
p1 : error: crc mismatch #8500=='AA86'
p1 : error: telegram has CRC error, discarding 864 bytes
p1 : telegram 14 received (864 bytes)
p1 : telegram 15 received (864 bytes)
p1 : error: crc mismatch #9658=='5618'
p1 : error: telegram has CRC error, discarding 862 bytes
p1 : telegram 16 received (864 bytes)
p1 : telegram 17 received (864 bytes)
p1 : error: crc mismatch #185B=='6032'
p1 : error: telegram has CRC error, discarding 863 bytes
p1 : telegram 18 received (864 bytes)
p1 : telegram 19 received (864 bytes)
p1 : telegram 20 received (864 bytes)
p1 : telegram 21 received (864 bytes)
p1 : telegram 22 received (864 bytes)
p1 : telegram 23 received (864 bytes)
p1 : telegram 24 received (864 bytes)
p1 : telegram 25 received (864 bytes)
p1 : telegram 26 received (864 bytes)
p1 : error: crc mismatch #6AB4=='1419'
p1 : error: telegram has CRC error, discarding 864 bytes
p1 : telegram 27 received (864 bytes)
p1 : telegram 28 received (864 bytes)
p1 : error: crc mismatch #00C0=='06F7'
p1 : error: telegram has CRC error, discarding 863 bytes
p1 : telegram 29 received (864 bytes)
p1 : telegram 30 received (864 bytes)
p1 : telegram 31 received (864 bytes)
p1 : telegram 32 received (864 bytes)
p1 : error: crc mismatch #E87A=='055E'
p1 : error: telegram has CRC error, discarding 862 bytes
p1 : telegram 33 received (864 bytes)
p1 : telegram 34 received (864 bytes)
p1 : telegram 35 received (864 bytes)
p1 : telegram 36 received (864 bytes)
p1 : error: crc mismatch #CC54=='550D'
p1 : error: telegram has CRC error, discarding 863 bytes
p1 : error: crc mismatch #F3E1=='04A3'
p1 : error: telegram has CRC error, discarding 863 bytes
p1 : telegram 37 received (864 bytes)
p1 : telegram 38 received (864 bytes)
p1 : error: crc mismatch #6A03=='F415'
p1 : error: telegram has CRC error, discarding 864 bytes
p1 : telegram 39 received (864 bytes)
p1 : telegram 40 received (864 bytes)
p1 : telegram 41 received (864 bytes)
p1 : telegram 42 received (864 bytes)
p1 : error: crc mismatch #1E95=='08CE'
p1 : error: telegram has CRC error, discarding 864 bytes
p1 : telegram 43 received (864 bytes)
p1 : telegram 44 received (864 bytes)
p1 : error: crc mismatch #F720=='544B'
p1 : error: telegram has CRC error, discarding 863 bytes
p1 : telegram 45 received (864 bytes)
p1 : telegram 46 received (864 bytes)
p1 : error: crc mismatch #E3EB=='2BD4'
p1 : error: telegram has CRC error, discarding 863 bytes
p1 : error: crc mismatch #45D4=='49AF'
p1 : error: telegram has CRC error, discarding 863 bytes
p1 : telegram 47 received (864 bytes)
p1 : error: crc mismatch #F194=='41FA'
p1 : error: telegram has CRC error, discarding 862 bytes
p1 : telegram 48 received (864 bytes)
p1 : telegram 49 received (864 bytes)
p1 : telegram 50 received (864 bytes)
app: 66 intervals (of 10s): 50 pass, 16 fail
```


### ESP 2.3.0

```
Welcome to TestSwSer
ver: ESP8266 board 2_3_0
ver: SoftwareSerial 1.0
cpu: init (160MHz)
crc: init (test pass)
p1 : init
p1 : telegram 1 received (864 bytes)
p1 : telegram 2 received (864 bytes)
p1 : telegram 3 received (864 bytes)
p1 : telegram 4 received (864 bytes)
p1 : telegram 5 received (864 bytes)
p1 : telegram 6 received (864 bytes)
p1 : error: crc mismatch #F9AF=='66A6'
p1 : error: telegram has CRC error, discarding 863 bytes
p1 : telegram 7 received (864 bytes)
p1 : telegram 8 received (864 bytes)
p1 : telegram 9 received (864 bytes)
p1 : telegram 10 received (864 bytes)
p1 : telegram 11 received (864 bytes)
p1 : telegram 12 received (864 bytes)
p1 : telegram 13 received (864 bytes)
p1 : telegram 14 received (864 bytes)
p1 : telegram 15 received (864 bytes)
p1 : telegram 16 received (864 bytes)
p1 : telegram 17 received (864 bytes)
p1 : telegram 18 received (864 bytes)
p1 : telegram 19 received (864 bytes)
p1 : telegram 20 received (864 bytes)
p1 : telegram 21 received (864 bytes)
p1 : telegram 22 received (864 bytes)
p1 : telegram 23 received (864 bytes)
p1 : telegram 24 received (864 bytes)
p1 : telegram 25 received (864 bytes)
p1 : telegram 26 received (864 bytes)
p1 : telegram 27 received (864 bytes)
p1 : telegram 28 received (864 bytes)
p1 : telegram 29 received (864 bytes)
p1 : telegram 30 received (864 bytes)
p1 : telegram 31 received (864 bytes)
p1 : telegram 32 received (864 bytes)
p1 : telegram 33 received (864 bytes)
p1 : telegram 34 received (864 bytes)
p1 : telegram 35 received (864 bytes)
p1 : telegram 36 received (864 bytes)
p1 : telegram 37 received (864 bytes)
p1 : telegram 38 received (864 bytes)
p1 : telegram 39 received (864 bytes)
p1 : telegram 40 received (864 bytes)
p1 : telegram 41 received (864 bytes)
p1 : telegram 42 received (864 bytes)
p1 : telegram 43 received (864 bytes)
p1 : telegram 44 received (864 bytes)
p1 : telegram 45 received (864 bytes)
p1 : telegram 46 received (864 bytes)
p1 : telegram 47 received (864 bytes)
p1 : telegram 48 received (864 bytes)
p1 : telegram 49 received (864 bytes)
p1 : telegram 50 received (864 bytes)
app: 51 intervals (of 10s): 50 pass, 1 fail
```

(end of doc)