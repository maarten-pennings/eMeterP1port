# e-Meter P1 port

ESP8266 Arduino sketch to read Dutch e-Meters and upload to ThingSpeak.


## Introduction

In the Netherlands, new electricity meters (e-Meters) are being rolled out. These meters are referred to
as [smart](https://nl.wikipedia.org/wiki/Slimme_meter). A smart meter is a digital (gas- or) electricity meter
that can be remotely read, by the power company, using e.g. power line communication. 

The Dutch government has [standardized (v5)](https://www.netbeheernederland.nl/_upload/Files/Slimme_meter_15_a727fce1f1.pdf) smart meters. 
I do have a v4.2 meter, and that older [spec](https://www.netbeheernederland.nl/_upload/Files/Slimme_meter_15_32ffe3cc38.pdf) differs on details.

One interesting aspect is that Dutch smart meters are required to have a so-called P1 port. Basically, this is a (transmit-only)
serial port, that spits out the meter readings regularly (every 10 seconds in v4.2, every second in V5.0).

This project uses an ESP8266 to receive the readings and to upload them to a webserver; 
this could be a Google docs spreadsheet, but I use the cloud service from [ThingSpeak](https://thingspeak.com/).

![e-Meter system diagram](emeter-system.drawio.png)


## Generation 0

The P1 port standard requires the data line to be inverted. 
So all "normal" serial ports can't handle the P1 port.

If you have an FTDI cable you are in luck: it has hardware support to invert the data back.

See [generation 0](gen0) for my first experiments.


## Generation 1

If you want to use a bigger device like a Raspberry Pi, you can go for the FTDI cable.
I wanted a lighter device: an ESP8266.
I have written a [generation 1 firmware](gen1) that has been operational from April 2017 to may 2021. 
Then I had a power failure, which causes a telegram line to become so long that my firmware crashes on every telegram.

I do **not** recommend using the generation 1 firmware because it uses the SoftwareSerial library 
to invert the RX pin; this bit-bang approach is unreliable (many CRC errors). 
There is a better way: the ESP8266 has, like the FTDI cable, hardware support to invert the RX pin.
This approach is taken in generation 2.


## Generation 2

I went a whole year without logging, mainly because the SoftwareSerial gave [too many errors](gen1/testswser)
when using it with the later ESP8266 Arduino releases.

Early 2022 I discovered that the ESP8266 also has hardware support to re-invert the data line.
Then I developed [generation 2](gen2).

(end)

   
