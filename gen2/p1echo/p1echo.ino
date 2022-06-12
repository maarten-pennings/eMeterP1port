// p1echo.ino - Dutch smart meter reader - echos telegrams
// step 1. connect ESP8266 NodeMCU via USB to laptop
// step 2. flash this sketch to the ESP using that serial-over-USB link
// step 3. *after* that connect smart meter as shown in [Wiring]
// step 4. see p1 telegrams coming in on serial port on PC

// Wiring
//   |  NodeMCU  |
//   |GND      D7|           +-+----
//   |3V3      D8|           |6|GND
//   |EN       RX|-----<-----|5|TXN  P1-port
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

void setup() {
  Serial.begin(115200, SERIAL_8N1, SERIAL_FULL);
  do delay(250); while( !Serial );
  Serial.printf("\n\n\nWelcome to p1read\n");

  Serial.flush();
  USC0(UART0) = USC0(UART0) | BIT(UCRXI);
  Serial.println("seri : RX inverted");
}

void loop() {
  int ch= Serial.read();
  if( ch!=-1 ) Serial.printf("%c",ch);
}
