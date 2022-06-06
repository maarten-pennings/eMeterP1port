// p1parse.ino - Dutch smart meter reader - parses telegrams

// step 1. connect ESP8266 NodeMCU via USB to laptop
// step 2. flash this sketch to the ESP using that serial-over-USB link
// step 3. *after* that connect smart meter as shown in [Wiring]
// step 4. see p1 telegrams coming in on serial port on PC

// Wiring
//   |  NodeMCU  |
//   |GND      D7|           +-+----
//   |3V3      D8|           |6|GND
//   |EN       RX|-----<-----|5|TXD  P1-port
//   |RSt      TX|           |4|NC    smart
//   |GND     GND|-----=-----|3|GND   meter
//   |VIN     3V3|----->-----|2|RTS
//   +----USB----+           |1|5V
//                           +-+----

// Notes
// - this script uses Serial (UART0) to send printf data to laptop
// - it also uses Serial (UART0) to receive data from p1 port 
// - so laptop can not send data to ESP
// - the ESP uart support inverting the RX pin


#include "tele.h"


void uart_init() {
  // Invert RX (Dutch smart meter needs that)
  USC0(UART0) = USC0(UART0) | BIT(UCRXI);
  Serial.flush();
  
  Serial.printf("uart: init\n");
}

// === APP ============================================================================================


// use real serial port or spoof prerecorded data
#if 0
  #define SERIAL_READ() Serial.read()
#else 
  // An @ in the string causes a wait of 1000ms
  const char *s = "noise" TELE_EXAMPLE_1 "@@@@@@@@@@@@much\r\nmore noise@@@@@@@@@@@@" TELE_EXAMPLE_2 TELE_EXAMPLE_3;
  int SERIAL_READ() {
    if( *s=='\0' ) { delay(1000); /* end of s, no inc */ return -1; }
    else if( *s=='@' ) { delay(1000); s++; return -1; }
    else { delay(1); return *s++; }
  }
#endif


int app_fail;

void setup() {
  Serial.begin(115200, SERIAL_8N1, SERIAL_FULL);
  do delay(250); while( !Serial );
  Serial.printf("\n\n\nWelcome to p1read\n");

  uart_init();
  tele_init();

  Serial.printf("\n");
  app_fail = 0;
}


void loop() {
  Tele_Result res = tele_parser_add(SERIAL_READ());
  if( res==TELE_RESULT_ERROR ) app_fail++;
  if( res==TELE_RESULT_AVAILABLE ) {
    Serial.printf("tele: available\n  %-15s %d\n","Num-Fail",app_fail );
    for( int i=0; i<TELE_NUMFIELDS; i++ ) {
      Serial.printf("  %-15s %s\n",tele_field_name(i), tele_field_value(i));
    }
    app_fail = 0;
  }
}
