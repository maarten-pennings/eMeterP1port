// tele.h - Interface to Dutch smart meter reader - parsing telegrams
#ifndef _TELE_H_
#define _TELE_H_


// The number of fields registered (in tele.cpp) for extraction by the parser
#define TELE_NUMFIELDS 16


// The add() function will return the abstract state of the parser
enum Tele_Result {
  TELE_RESULT_ERROR,      // a partial telegram is received but there was an error (timeout, syntax error, crc error, field missing).
  TELE_RESULT_COLLECTING, // telegram data is still being collected, no errors have been found yet, but the telegram is also not yet complete.
  TELE_RESULT_AVAILABLE,  // a complete telegram is received, its CRC matches, and all fields (as specified in tele_fields[]) are found.
};


// Initialize this module
void         tele_init();


// Start and feed the parser
Tele_Result  tele_parser_add(int ch);


// Once the parser's add() returns TELE_RESULT_AVAILABLE, the field_value is available.
// Note 0 <= ix < TELE_NUMFIELDS
const char   tele_field_key(int ix);
const char * tele_field_name(int ix);
const char * tele_field_description(int ix);
const char * tele_field_value(int ix);



// Example telegrams (meter ids are anonymized, CRC is adapted for that


#define TELE_EXAMPLE_1 \
  "/KFM5KAIFA-METER\r\n" \
  "\r\n" \
  "1-3:0.2.8(42)\r\n" \
  "0-0:1.0.0(220605191342S)\r\n" \
  "0-0:96.1.1(456d795f73657269616c5f6e756d626572)\r\n" \
  "1-0:1.8.1(019235.878*kWh)\r\n" \
  "1-0:1.8.2(016881.373*kWh)\r\n" \
  "1-0:2.8.1(000000.000*kWh)\r\n" \
  "1-0:2.8.2(000000.000*kWh)\r\n" \
  "0-0:96.14.0(0001)\r\n" \
  "1-0:1.7.0(00.586*kW)\r\n" \
  "1-0:2.7.0(00.000*kW)\r\n" \
  "0-0:96.7.21(00020)\r\n" \
  "0-0:96.7.9(00008)\r\n" \
  "1-0:99.97.0(3)(0-0:96.7.19)(211209190618W)(0000003557*s)(210416081947S)(0000004676*s)(000101000011W)(2147483647*s)\r\n" \
  "1-0:32.32.0(00000)\r\n" \
  "1-0:52.32.0(00000)\r\n" \
  "1-0:72.32.0(00000)\r\n" \
  "1-0:32.36.0(00000)\r\n" \
  "1-0:52.36.0(00000)\r\n" \
  "1-0:72.36.0(00000)\r\n" \
  "0-0:96.13.1()\r\n" \
  "0-0:96.13.0()\r\n" \
  "1-0:31.7.0(000*A)\r\n" \
  "1-0:51.7.0(001*A)\r\n" \
  "1-0:71.7.0(001*A)\r\n" \
  "1-0:21.7.0(00.004*kW)\r\n" \
  "1-0:22.7.0(00.000*kW)\r\n" \
  "1-0:41.7.0(00.269*kW)\r\n" \
  "1-0:42.7.0(00.000*kW)\r\n" \
  "1-0:61.7.0(00.309*kW)\r\n" \
  "1-0:62.7.0(00.000*kW)\r\n" \
  "0-1:24.1.0(003)\r\n" \
  "0-1:96.1.0(476d795f73657269616c5f6e756d626572)\r\n" \
  "0-1:24.2.1(220605190000S)(16051.816*m3)\r\n" \
  "!78CA\r\n"

#define TELE_EXAMPLE_2 \
  "/KFM5KAIFA-METER\r\n" \
  "\r\n" \
  "1-3:0.2.8(42)\r\n" \
  "0-0:1.0.0(220605191351S)\r\n" \
  "0-0:96.1.1(4530303033303030303034343234323134)\r\n" \
  "1-0:1.8.1(019235.879*kWh)\r\n" \
  "1-0:1.8.2(016881.373*kWh)\r\n" \
  "1-0:2.8.1(000000.000*kWh)\r\n" \
  "1-0:2.8.2(000000.000*kWh)\r\n" \
  "0-0:96.14.0(0001)\r\n" \
  "1-0:1.7.0(00.590*kW)\r\n" \
  "1-0:2.7.0(00.000*kW)\r\n" \
  "0-0:96.7.21(00020)\r\n" \
  "0-0:96.7.9(00008)\r\n" \
  "1-0:99.97.0(3)(0-0:96.7.19)(211209190618W)(0000003557*s)(210416081947S)(0000004676*s)(000101000011W)(2147483647*s)\r\n" \
  "1-0:32.32.0(00000)\r\n" \
  "1-0:52.32.0(00000)\r\n" \
  "1-0:72.32.0(00000)\r\n" \
  "1-0:32.36.0(00000)\r\n" \
  "1-0:52.36.0(00000)\r\n" \
  "1-0:72.36.0(00000)\r\n" \
  "0-0:96.13.1()\r\n" \
  "0-0:96.13.0()\r\n" \
  "1-0:31.7.0(000*A)\r\n" \
  "1-0:51.7.0(001*A)\r\n" \
  "1-0:71.7.0(001*A)\r\n" \
  "1-0:21.7.0(00.004*kW)\r\n" \
  "1-0:22.7.0(00.000*kW)\r\n" \
  "1-0:41.7.0(00.273*kW)\r\n" \
  "1-0:42.7.0(00.000*kW)\r\n" \
  "1-0:61.7.0(00.313*kW)\r\n" \
  "1-0:62.7.0(00.000*kW)\r\n" \
  "0-1:24.1.0(003)\r\n" \
  "0-1:96.1.0(4730303032333430313934303336323135)\r\n" \
  "0-1:24.2.1(220605190000S)(16051.816*m3)\r\n" \
  "!A5F9\r\n"

#define TELE_EXAMPLE_3 \
  "/KFM5KAIFA-METER\r\n" \
  "\r\n" \
  "1-3:0.2.8(42)\r\n" \
  "0-0:1.0.0(220605191411S)\r\n" \
  "0-0:96.1.1(456d795f73657269616c5f6e756d626572)\r\n" \
  "1-0:1.8.1(019235.883*kWh)\r\n" \
  "1-0:1.8.2(016881.373*kWh)\r\n" \
  "1-0:2.8.1(000000.000*kWh)\r\n" \
  "1-0:2.8.2(000000.000*kWh)\r\n" \
  "0-0:96.14.0(0001)\r\n" \
  "1-0:1.7.0(00.584*kW)\r\n" \
  "1-0:2.7.0(00.000*kW)\r\n" \
  "0-0:96.7.21(00020)\r\n" \
  "0-0:96.7.9(00008)\r\n" \
  "1-0:99.97.0(3)(0-0:96.7.19)(211209190618W)(0000003557*s)(210416081947S)(0000004676*s)(000101000011W)(2147483647*s)\r\n" \
  "1-0:32.32.0(00000)\r\n" \
  "1-0:52.32.0(00000)\r\n" \
  "1-0:72.32.0(00000)\r\n" \
  "1-0:32.36.0(00000)\r\n" \
  "1-0:52.36.0(00000)\r\n" \
  "1-0:72.36.0(00000)\r\n" \
  "0-0:96.13.1()\r\n" \
  "0-0:96.13.0()\r\n" \
  "1-0:31.7.0(000*A)\r\n" \
  "1-0:51.7.0(001*A)\r\n" \
  "1-0:71.7.0(001*A)\r\n" \
  "1-0:21.7.0(00.004*kW)\r\n" \
  "1-0:22.7.0(00.000*kW)\r\n" \
  "1-0:41.7.0(00.268*kW)\r\n" \
  "1-0:42.7.0(00.000*kW)\r\n" \
  "1-0:61.7.0(00.315*kW)\r\n" \
  "1-0:62.7.0(00.000*kW)\r\n" \
  "0-1:24.1.0(003)\r\n" \
  "0-1:96.1.0(476d795f73657269616c5f6e756d626572)\r\n" \
  "0-1:24.2.1(220605190000S)(16051.816*m3)\r\n" \
  "!A62C\r\n"


#endif
