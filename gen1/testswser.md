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

![e-Meter system diagram](../emeter-system.drawio.png)

Looking at the ESP8266, I want to use the (hardware) UART that is mapped to the USB for development and debugging.
The ESP8266 does not have a second UART (there is a "half" TX only UART). 
Besides, the data coming from the e-Meter is inverted anyhow 
(see [logical levels](https://www.netbeheernederland.nl/_upload/Files/Slimme_meter_15_32ffe3cc38.pdf#page=6) SPACE "0" as > 4V, MARK "1" as < 1V)

So, my conclusion was that using a bit-banged serial port makes sense here.

And fortunately one exists: [EspSoftwareSerial](https://github.com/plerup/espsoftwareserial).
We do not even have to download it, it is available in the stock ESP8266 board package.

### Problem

The e-Meter sends stats every 10 seconds.
The stats are sent as an textual page, each line containing a key value pair.
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
| 2.7.0 |  x.0  |   0  |  37  |   37  |   0% |
| 2.4.0 |  1.0  |  50  |  16  |   66  |  76% |
| 2.3.0 |  1.0  |  50  |   1  |   51  |  98% |


## Appendix 

### ESP 2.7.0

```
Welcome to TestSwSer
ver: ESP8266 board 2_7_0
ver: SoftwareSerial 6.8.1
cpu: init (160MHz)
crc: init (test pass)
p1 : init
p1 : error: crc mismatch #D8F9=='EAA4'
p1 : error: telegram has CRC error, discarding 755 bytes
p1 : error: crc mismatch #98A9=='FEA1'
p1 : error: telegram has CRC error, discarding 768 bytes
p1 : error: crc mismatch #4D2B=='CDB2'
p1 : error: telegram has CRC error, discarding 752 bytes
p1 : error: crc mismatch #5A6E=='479A'
p1 : error: telegram has CRC error, discarding 751 bytes
p1 : error: timeout waiting for EOT, discarding 756 bytes
p1 : error: crc mismatch #8D9F=='D1C7'
p1 : error: telegram has CRC error, discarding 749 bytes
p1 : error: crc mismatch #2BD6=='6FFF'
p1 : error: telegram has CRC error, discarding 765 bytes
p1 : error: crc mismatch #E575=='0C8C'
p1 : error: telegram has CRC error, discarding 752 bytes
p1 : error: crc mismatch #3DC1=='7E37'
p1 : error: telegram has CRC error, discarding 764 bytes
p1 : error: crc mismatch #C544=='78C8'
p1 : error: telegram has CRC error, discarding 761 bytes
p1 : error: crc mismatch #2BA1=='D7C5'
p1 : error: telegram has CRC error, discarding 752 bytes
p1 : error: crc mismatch #8EC0=='FB71'
p1 : error: telegram has CRC error, discarding 754 bytes
p1 : error: crc mismatch #9C15=='35F8'
p1 : error: telegram has CRC error, discarding 759 bytes
p1 : error: crc mismatch #2A76=='34BA'
p1 : error: telegram has CRC error, discarding 762 bytes
p1 : error: crc mismatch #D679=='43D8'
p1 : error: telegram has CRC error, discarding 753 bytes
p1 : error: crc mismatch #ACE1=='C2DC'
p1 : error: telegram has CRC error, discarding 762 bytes
p1 : error: crc mismatch #78CE=='ACC8'
p1 : error: telegram has CRC error, discarding 745 bytes
p1 : error: timeout waiting for EOT, discarding 763 bytes
p1 : error: crc mismatch #F7FA=='2A9E'
p1 : error: telegram has CRC error, discarding 767 bytes
p1 : error: crc mismatch #D3EE=='4883'
p1 : error: telegram has CRC error, discarding 760 bytes
p1 : error: timeout waiting for EOT, discarding 754 bytes
p1 : error: crc mismatch #730B=='EE45'
p1 : error: telegram has CRC error, discarding 761 bytes
p1 : error: crc mismatch #AE91=='CC6F'
p1 : error: telegram has CRC error, discarding 753 bytes
p1 : error: timeout waiting for EOT, discarding 760 bytes
p1 : error: timeout waiting for EOT, discarding 742 bytes
p1 : error: crc mismatch #0ACB=='13CF'
p1 : error: telegram has CRC error, discarding 756 bytes
p1 : error: timeout waiting for EOT, discarding 754 bytes
p1 : error: crc mismatch #61B2=='CAD0'
p1 : error: telegram has CRC error, discarding 771 bytes
p1 : error: timeout waiting for EOT, discarding 745 bytes
p1 : error: timeout waiting for EOT, discarding 756 bytes
p1 : error: crc mismatch #258E=='460A'
p1 : error: telegram has CRC error, discarding 755 bytes
p1 : error: timeout waiting for EOT, discarding 751 bytes
p1 : error: crc mismatch #EB72=='12F6'
p1 : error: telegram has CRC error, discarding 755 bytes
p1 : error: crc mismatch #9FE2=='74D0'
p1 : error: telegram has CRC error, discarding 747 bytes
p1 : error: crc mismatch #1A7E=='14AE'
p1 : error: telegram has CRC error, discarding 769 bytes
p1 : error: crc mismatch #9645=='B9E3'
p1 : error: telegram has CRC error, discarding 765 bytes
p1 : error: crc mismatch #85B5=='EA23'
p1 : error: telegram has CRC error, discarding 753 bytes
```

### ESP 2.6.0

Several compile errors (build also complains about git?).

### ESP 2.5.0

When installing, the downloaded BSP was correct.

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