# Generation 0

Getting e-Meter P1 port data with an FTDI cable.


## Introduction

Does my meter spit out data, and can I get my hands on it?

The P1 port is a standard UART, but in practice it has some peculiarities.

 - The P1 port is write-only (from the perspective of the meter): only TX is wired, there is no RX.
 - The P1 port does deliver power (5V, 250mA).
   Nevertheless, **my board resets while booting when powered from the meter**.
   After several trials I decided to run my NodeMCU from a separate USB power supply.
 - The P1 port has a "data request" (RTS), this must be pulled high, for the e-meter to produce data.
 - The P1 port has a "data out" (TX), which uses UART encoding: 115200/8/N/1.
 - However, as the specification states 
   _the "Data" line must be designed as an Open Collector output, the "Data" line must be logically inverted_. 
   In other words, the e-Meter TX signal is inverted (TXN).


## Wiring

For a proof of concept, I started with an [FTDI cable](https://nl.farnell.com/ftdi/ttl-232r-3v3/cable-usb-to-ttl-level-serial/dp/1329311). 
On the [FTDI website](https://www.ftdichip.com/Support/Utilities.htm)
you can download **FT_Prog**. This windows program allows configuring an FTDI cable.
The crucial aspect is that we can invert the RX input. 
Section "5.5 FT232R Hardware_Specific" explains "Additional features available on the FT232R device allow RS232 signals to be inverted". 

You can also buy a [dedicated cable](https://www.aliexpress.com/i/32945225256.html).

![USB cable](usb.png)

We need a common ground, we tab the TXN port, and we hook the RTS line to VCC to make 
the e-Meter emit data over TXN.


## Result

This is what my meter spits out. I have anonymized the equipment identifiers (and adapted the CRC for that).

Here is an annotated telegram.

```text
/KFM5KAIFA-METER                               // / X X X 5 Identification CR LF
                                               // CR LF
1-3:0.2.8(42)                                  // Version information for P1 output // 4.2
0-0:1.0.0(220605132822S)                       // [20]22-06-05 13:28:22 S[ummertime]
0-0:96.1.1(456d795f73657269616c5f6e756d626572) // Equipment identifier // 'Emy_serial_number' // ''.join(chr(int(s[i:i+2],16)) for i in range(0,len(s),2))
1-0:1.8.1(019232.216*kWh)                      // Meter Reading electricity delivered **to** client (Tariff 1)
1-0:1.8.2(016881.373*kWh)                      // Meter Reading electricity delivered **to** client (Tariff 2)
1-0:2.8.1(000000.000*kWh)                      // Meter Reading electricity delivered **by** client (Tariff 1) 
1-0:2.8.2(000000.000*kWh)                      // Meter Reading electricity delivered **by** client (Tariff 2)
0-0:96.14.0(0001)                              // Tariff indicator electricity
1-0:1.7.0(00.393*kW)                           // Actual electricity power delivered (+P)
1-0:2.7.0(00.000*kW)                           // Actual electricity power received (-P)
0-0:96.7.21(00020)                             // Number of power failures in any phase
0-0:96.7.9(00008)                              // Number of long power failures in any phase
1-0:99.97.0(3)(0-0:96.7.19)(211209190618W)(0000003557*s)(210416081947S)(0000004676*s)(000101000011W)(2147483647*s) // Power Failure Event Log (long power failures)
1-0:32.32.0(00000)                             // Number of voltage sags in phase L1
1-0:52.32.0(00000)                             // Number of voltage sags in phase L2 
1-0:72.32.0(00000)                             // Number of voltage sags in phase L3 
1-0:32.36.0(00000)                             // Number of voltage swells in phase L1
1-0:52.36.0(00000)                             // Number of voltage swells in phase L2
1-0:72.36.0(00000)                             // Number of voltage swells in phase L3
0-0:96.13.1()                                  // UNKNOWN
0-0:96.13.0()                                  // Text message max 1024 characters
                                               // MISSING 1-0:32.7.0 // Instantaneous voltage L1 in V resolution
                                               // MISSING 1-0:52.7.0 // Instantaneous voltage L2 in V resolution
                                               // MISSING 1-0:72.7.0 // Instantaneous voltage L3 in V resolution
1-0:31.7.0(000*A)                              // Instantaneous current L1 in A resolution
1-0:51.7.0(001*A)                              // Instantaneous current L2 in A resolution
1-0:71.7.0(000*A)                              // Instantaneous current L3 in A resolution
1-0:21.7.0(00.001*kW)                          // Instantaneous power L1 (+P) in W resolution
1-0:22.7.0(00.000*kW)                          // Instantaneous power L1 (-P) in W resolution
1-0:41.7.0(00.205*kW)                          // Instantaneous power L2 (+P) in W resolution
1-0:42.7.0(00.000*kW)                          // Instantaneous power L2 (-P) in W resolution
1-0:61.7.0(00.187*kW)                          // Instantaneous power L3 (+P) in W resolution
1-0:62.7.0(00.000*kW)                          // Instantaneous power L3 (-P) in W resolution
0-1:24.1.0(003)                                // Device-Type
0-1:96.1.0(476d795f73657269616c5f6e756d626572) // Equipment identifier (Gas) 
0-1:24.2.1(220605130000S)(16051.302*m3)        // Last 5-minute Meter reading and capture time // [20]22-06-05 13:00:00S[ummertime]
                                               // MISSING 0-n:24.1.0 // Device-Type 
                                               // MISSING 0-n:96.1.0 // Equipment identifier  
                                               // MISSING 0-n:24.2.1 + 0-n:24.2.1 // Last 5-minute Meter reading and capture time (e.g. slave E meter)
!70CE                                          // ! CRC CR LF
```

And here one without annotations.
Note that every line ends with CR LF.

```text
/KFM5KAIFA-METER

1-3:0.2.8(42)
0-0:1.0.0(220605132831S)
0-0:96.1.1(456d795f73657269616c5f6e756d626572)
1-0:1.8.1(019232.217*kWh)
1-0:1.8.2(016881.373*kWh)
1-0:2.8.1(000000.000*kWh)
1-0:2.8.2(000000.000*kWh)
0-0:96.14.0(0001)
1-0:1.7.0(00.394*kW)
1-0:2.7.0(00.000*kW)
0-0:96.7.21(00020)
0-0:96.7.9(00008)
1-0:99.97.0(3)(0-0:96.7.19)(211209190618W)(0000003557*s)(210416081947S)(0000004676*s)(000101000011W)(2147483647*s)
1-0:32.32.0(00000)
1-0:52.32.0(00000)
1-0:72.32.0(00000)
1-0:32.36.0(00000)
1-0:52.36.0(00000)
1-0:72.36.0(00000)
0-0:96.13.1()
0-0:96.13.0()
1-0:31.7.0(000*A)
1-0:51.7.0(001*A)
1-0:71.7.0(000*A)
1-0:21.7.0(00.001*kW)
1-0:22.7.0(00.000*kW)
1-0:41.7.0(00.206*kW)
1-0:42.7.0(00.000*kW)
1-0:61.7.0(00.187*kW)
1-0:62.7.0(00.000*kW)
0-1:24.1.0(003)
0-1:96.1.0(476d795f73657269616c5f6e756d626572)
0-1:24.2.1(220605130000S)(16051.302*m3)
!8746
```


(end)
