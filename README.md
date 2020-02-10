MeterP1port
ESP8266 Arduino sketch to read Dutch e-Meters and upload to ThingSpeak.

Disclaimer: I put the Arduino sketck on GitHub nearly three years after I developed it. 
As a consequence, this README.md is written later and may not be completely accurate.

## Introduction
In the Netherlands, new electricity meters (e-meters) are being rolled out. These meters are referred to
as [smart](https://nl.wikipedia.org/wiki/Slimme_meter). A smart meter is a digital (gas- or) electricy meter
that can be remotely read. 

The Dutch goverment has [standardized](https://www.netbeheernederland.nl/_upload/Files/Slimme_meter_15_91e8f3e526.pdf) smart meters. 
One interesting aspect is that Dutch smart meters are required to have a so-called P1 port. Basically, this is a (transmit-only)
serial port, that spits out the meter readings every 10 seconds.

This project uses an ESP8266 to receive the readings, and uploads them to the cloud ([ThingSpeak](https://thingspeak.com/))

## Wiring

As mentioned in the introduction, the P1 port is basically a serial port.

### Considerations
However, in practice there are some considerations.

 - The P1 port is write-only (from the perspective of the meter).
 - The P1 port does seem to deliver sufficient power (5V, 250mA), nevertheless, I run my NodeMCU from a separate USB power supply.
 - The P1 port has a "data request", this must be pulled high, for the e-meter to produce data
 - The P1 port has a "data out" which uses UART encoding: 115200/*/N/1
 - However, as the specification states 
   _the “Data” line must be designed as an Open Collector output, the “Data” line must be logically inverted_. 
   In other words, the signal is inverted.

### Wire to PC (USB)
I started this project with an [FTDI cable](https://nl.farnell.com/ftdi/ttl-232r-3v3/cable-usb-to-ttl-level-serial/dp/1329311). 
On the [FTDI website](https://www.ftdichip.com/Support/Utilities.htm)
you can download FT_Prog. This windows program allows configuring an FTDI cable: "5.5 FT232R Hardware_Specific"
explains "Additional features available on the FT232R device allow RS232 signals to be inverted". 
That's what we need for the data pin.

You can also buy a [dedicated cable](https://www.aliexpress.com/i/32945225256.html)

![USB cable](usb.png)

### Wire to ESP8266
For the real project, we will use a software UART on the ESP8266, so no "active" is needed.

The connector to mate with the e-meter is an RJ (telephone) jack. 
The official standard prescribes an **RJ12**	plug, which is a 6 pole 6 connector (6P6C) plug.
Since we will not use the outer two pins (power), we can also take a **RJ14** (6P4C) plug.

This is the wireing:

![Wiring](connection.png)

## Parsing
Once the wiring is established, we need to parse the data.
The official [spec](https://www.netbeheernederland.nl/_upload/Files/Slimme_meter_15_a727fce1f1.pdf) is helpful here.

