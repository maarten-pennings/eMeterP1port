# e-Meter P1 port

ESP8266 Arduino sketch to read Dutch e-Meters and upload to ThingSpeak.

There is a [generation 1](gen1) using SoftwareSerial, but I suggest you take [generation 2](gen2) which uses
a hardware feature to invert back the TX line.


## Introduction

In the Netherlands, new electricity meters (e-meters) are being rolled out. These meters are referred to
as [smart](https://nl.wikipedia.org/wiki/Slimme_meter). A smart meter is a digital (gas- or) electricity meter
that can be remotely read, by the power company, using e.g. power line communication. 

The Dutch government has [standardized](https://www.netbeheernederland.nl/_upload/Files/Slimme_meter_15_a727fce1f1.pdf) smart meters. 
I do have a v4.2 meter, and that older [spec](https://www.netbeheernederland.nl/_upload/Files/Slimme_meter_15_32ffe3cc38.pdf) differs on details.

One interesting aspect is that Dutch smart meters are required to have a so-called P1 port. Basically, this is a (transmit-only)
serial port, that spits out the meter readings regularly (every 10 seconds in v4.2, every second in V5.0).

This project uses an ESP8266 to receive the readings, and to upload them to a webserver; 
this could be a google docs spreadsheet, but I use the cloud service from [ThingSpeak](https://thingspeak.com/).

![e-Meter system diagram](emeter-system.drawio.png)


## Generation 0

The P1 port standard requires the data line to be inverted. 

If you have an FTDI cable you are in luck: it has hardware support to invert the data back.

See [generation 0](gen0) for my first experiments.


## Generation 1

If you want to use a bigger device like a Raspberry Pi, you can go for the FTDI cable.
I wanted a lighter device: an ESP8266.
I have written a [generation 1 firmware](gen1) that has been operational from April 2017 to may 2021. 
Then I had a power failure, which caused a telegram line to become so long that my firmware crashed.

I do not recommend using the generation 1 firmware because it uses the SoftwareSerial library to invert back the
TXD line. There is a better way: the ESP8266 has, like the FTDI cable, hardware support to invert the data back.


## Generation 2

I went a whole year without logging, mainly because the SoftwareSerial gave [too many errors](gen1/testswser)
when using it with the later ESP8266 Arduino releases.

Early 2022 I discovered that the ESP8266 also has hardware support to re-invert the data line.

Then I developed [generation 2](gen2).

(end)

   
