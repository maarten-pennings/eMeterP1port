// emp1g2.ino - Dutch smart meter reader - parses telegrams and publishes them on ThingSpeak
#define APP_NAME    "eMP1g2"
#define APP_VERSION "1.0"
// 20220606  1.0  Maarten Pennings  Generation 2 of my e-Meter P1 port reader


#include <Nvm.h>
#include <Cfg.h>
#include "tele.h"


// === Wiring ===================================================================================

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


// === LED ==========================================================================================


 // Note on my robotdyn board the led on pin two was "worn"; I replaced it with a red one
#define LED_BLUEPIN     2 // on robotdyn board: blue LED near D1 and D2 pins (eg top right side)
//#define LED_BLUEPIN    16 // on robotdyn board: blue LED near SD1 and SD2 pins (eg top left side)

bool _led_ison;

void led_show(void) {
  if( _led_ison ) digitalWrite(LED_BLUEPIN, LOW); else digitalWrite(LED_BLUEPIN, HIGH);
}

void led_on(void) {
  _led_ison= true;
  led_show();
}

void led_off(void) {
  _led_ison= false;
  led_show();
}

void led_toggle(void) {
  _led_ison= ! _led_ison;
  led_show();
}

void led_init(void) { 
  pinMode(LED_BLUEPIN, OUTPUT); 
  led_off();
  Serial.printf("led : init\n");
}


// === CFG ============================================================================================
// Persistent configuration

// The configurable fields used by this application
NvmField CfgEmp1Fields[] = {
  {"Access point"    , ""                                                  ,  0, "The eMP1 publishes data over internet. Supply credentials for a WiFi access point it should connect to. " },
  {"ssid"            , "MySSID"                                            , 32, "The ssid of the wifi network this device should connect to." },
  {"password"        , "MyPassword"                                        , 32, "The password of the wifi network this device should connect to. "},
  
  {"Server 1 (post)" , ""                                                  ,  0, "The eMP1 may publish data using the 'POST' protocol. Supply the server, URL and the post body (in two chunks), or leave blank. " },
  {"postserver"      , "api.thingspeak.com"                                , 32, "The name of the server to which measurements are send via POST (empty for none)."},
  {"posturl"         , "/update"                                           , 32, "The URL for the POST server."},
  {"postbody1"       , "field1=%L&field2=%H&field3=%l&field4=%h&field5=%P&", 64, "Body part 1 HELP: %L=Cons-Night1-kWh, %H=Cons-Day2-kWh, %l=Prod-Night1-kWh, %h=Prod-Day2-kWh, %I=Night1-Day2, %P=Cons-kW, %p=Prod-kW, %F=Fails-short-#, %f=Fails-long-#."},
  {"postbody2"       , "field6=%p&field7=%F&field8=%E&key=MyWriteKeyXXXXXX", 64, "Body part 2 HELP: %A=Cons-L1-kW, %a=Prod-L1-kW, %B=Cons-L2-kW, %b=Prod-L2-kW, %C=Cons-L3-kW, %c=Prod-L3-kW, %G=Cons-Gas-m3, %%=%, add . to skip dot (%.P)."},
  {"postperiod"      , "60000"                                             ,  8, "The number of milliseconds between post's. "},

  {"Server 2 (get)"  , ""                                                  ,  0, "The eMP1 may publish data using the 'GET' protocol. Supply the server and URL, or leave blank. " },
  {"getserver"       , "nwebmsg.fritz.box"  /* "192.168.179.74" */         , 32, "The name of the server to which measurements are send via GET (empty for none)."},
  {"geturl"          , "/?msg=%.P&mode=right"                               , 32, "The URL for the GET server [HELP: use % as in postbody]."},
  {"getperiod"       , "1000"                                              ,  8, "The number of milliseconds between get's. "},

  {0                 , 0                                                   ,  0, 0},  
};
Cfg cfg( APP_NAME, CfgEmp1Fields, CFG_SERIALLVL_USR, LED_BLUEPIN);


// === UART ============================================================================================
// Magic trick: the ESP8266 support RX invertion


void uart_init() {
  // Invert RX (Dutch smart meter needs that)
  USC0(UART0) = USC0(UART0) | BIT(UCRXI);
  Serial.flush();
  delay(500); Serial.read(); // There is often one zero remaining, Serial.flush() does not help
  
  Serial.printf("uart: init\n");
}


// === Wifi =================================================================================================


void wifi_init() {
  WiFi.hostname( APP_NAME );  
  WiFi.mode(WIFI_STA);
  Serial.printf("wifi: %s ..",cfg.getval("ssid"));
  WiFi.begin(cfg.getval("ssid"), cfg.getval("password") );
  while( WiFi.status()!=WL_CONNECTED ) {
    led_on();
    delay(250);
    led_off();
    delay(250);
    Serial.printf(".");
  }
  Serial.printf(" %s\n",WiFi.localIP().toString().c_str());
}



// === http =====================================================================================


int http_strncpy(char *buf, int size, const char * nums, bool skipdot) {
  const char *r=nums; // read pointer
  char *w=buf;  // write pointer
  // r points to a numeric string; strip leading 0s
  while( *r=='0' && ( isdigit(*(r+1)) || (skipdot && *(r+1)=='.') ) ) r++; // must terminate e.g. on "0\0"
  while( *r!='\0' && size>1 ) {
    if( *r!='.' || !skipdot) { *w = *r; w++; size--; }
    r++;
  }
  *w='\0'; w++; size--;
  return w-buf-1;
}


// This is like sprintf; it prints `fmt` to `buf`, replacing "%x" thingies with values from the last telegram
// The letters to be used after % are the ones registered as key in tele_fields[]
int http_subst(char *buf, int size, const char * fmt ) {
  const char *r=fmt; // read pointer
  char *w=buf; // write pointer
  while( *r!='\0' && size>1 ) {
    if( *r=='%' ) {
      // Skip escape character (%)
      r++;
      // Is there a * modifier
      bool skipdot = *r=='.';
      if( skipdot ) r++;
      // Get the key
      char key = *r++;
      // Lookup value associatied with the key (if any)
      const char * value = NULL;
      for( int i=0; i<TELE_NUMFIELDS; i++ ) {
        if( tele_field_key(i) == key ) { value=tele_field_value(i); break; }
      }
      //Serial.printf("star=%d key=%c val='%s'\n",star,key, value);
      // Was the key found
      if( value==NULL ) {
        // key was not found, insert it
        if( size>1 ) { *w='%'; w++; size--; }
        if( size>1 && skipdot ) { *w='.'; w++; size--; }
        if( size>1 && !skipdot && key!='%' ) { *w=key; w++; size--; }
      } else {
        // Found, insert value
        int len=http_strncpy(w,size,value,skipdot); 
        if( len>size-1) len=size-1;
        w+=len; size-=len;
      }
    } else {
      // Plain character; copy
      *w=*r; w++; size--; r++; 
    }
  }
  // Add terminating zero
  *w='\0'; w++; size--;
  // Return bytes written
  return w-buf-1;
}


#define HTTP_BUF_SIZE 256
char http_buf[HTTP_BUF_SIZE];


// curl -d "field1=101&field2=202&key=1234567890" -X POST http://api.thingspeak.com/update


// Send a POST request
void http_post() {
  char * srv = cfg.getval("postserver");
  if( *srv=='\0' ) { Serial.printf("emp1: post: no server\n"); return; }
  
  WiFiClient client;
  if( client.connect(srv,80) ) {
    // Create body with the fields
    int len1= http_subst( http_buf, HTTP_BUF_SIZE, cfg.getval("postbody1") );
    int len2= http_subst( http_buf+len1, HTTP_BUF_SIZE-len1, cfg.getval("postbody2") );
    int len=len1+len2;
    if( len+1==HTTP_BUF_SIZE ) Serial.printf("emp1: post: url truncated\r\n");
    // Construct API request body
    client.print("POST "); client.print(cfg.getval("posturl")); client.print(" HTTP/1.1\r\n");
    client.print("Host: "); client.print(srv); client.print("\r\n");
    client.print("Connection: close\r\n");
    client.print("Content-Type: application/x-www-form-urlencoded\r\n");
    client.print("Content-Length: "); client.print(len); client.print("\r\n");
    client.print("\r\n");
    client.print(http_buf); client.print("\r\n");
    client.print("\r\n");
    Serial.printf("emp1: post: %s\n", srv);
  } else {
    Serial.printf("emp1: post: cannot connect to %s\n", srv);
  }
  delay(1);
  client.stop();
}


// Send a GET request, returns true iff successful
void http_get() {
  char * srv = cfg.getval("getserver");
  if( *srv=='\0' ) { Serial.printf("emp1: get : no server\n"); return; }
  
  WiFiClient client;
  if( client.connect(srv,80) ) {
    // Create body with the fields
    int len= http_subst(http_buf,HTTP_BUF_SIZE,cfg.getval("geturl"));
    if( len+1==HTTP_BUF_SIZE ) Serial.printf("emp1: get : url truncated\r\n");
    // Construct API request body
    client.print("GET "); client.print(http_buf); client.print(" HTTP/1.1\r\n");
    client.print("Host: "); client.print(cfg.getval("getserver")); client.print("\r\n");
    client.print("Connection: close\r\n");
    client.print("\r\n");
    Serial.printf("emp1: get : %s\n", srv);
  } else {
    Serial.printf("emp1: get : cannot connect to %s\n", srv);
  }
  delay(1);
  client.stop();
}


// === APP ============================================================================================


// Use real serial port or spoof prerecorded data
#if 1
  #define SERIAL_READ() Serial.read()
#else 
  // An @ in the string causes a wait of 1000ms
  //const char *s = "noise" TELE_EXAMPLE_1 "@@@@@@@@@@@@much\r\nmore noise@@@@@@@@@@@@" TELE_EXAMPLE_2 "@" TELE_EXAMPLE_3 TELE_EXAMPLE_2 "@@" TELE_EXAMPLE_3;
  const char *s = "@@@@@" TELE_EXAMPLE_1 "@@@@@" TELE_EXAMPLE_2 "@@@@@" TELE_EXAMPLE_3 
                  "@@@@@" TELE_EXAMPLE_1 "@@@@@" TELE_EXAMPLE_2 "@@@@@" TELE_EXAMPLE_3 
                  "@@@@@" TELE_EXAMPLE_1 "@@@@@" TELE_EXAMPLE_2 "@@@@@" TELE_EXAMPLE_3 
                  "@@@@@" TELE_EXAMPLE_1 "@@@@@" TELE_EXAMPLE_2 "@@@@@" TELE_EXAMPLE_3 
                  "@@@@@" TELE_EXAMPLE_1 "@@@@@" TELE_EXAMPLE_2 "@@@@@" TELE_EXAMPLE_3;
  int SERIAL_READ() {
    if( *s=='\0' ) { delay(1000); /* end of s, no inc */ return -1; }
    else if( *s=='@' ) { delay(1000); s++; return -1; }
    else { delay(1); return *s++; }
  }
#endif


uint32_t app_last_post;
uint32_t app_last_get;

uint32_t cfg_postperiod;
uint32_t cfg_getperiod;

void setup() {
  // Bring up serial
  Serial.begin(115200, SERIAL_8N1, SERIAL_FULL);
  do delay(250); while( !Serial );

  // Print app name and versions
  Serial.printf("\n\n\nWelcome to e-meter P1 port generation 2\n");
  Serial.printf("  App: " APP_NAME " " APP_VERSION "\n");
  Serial.printf("  BSP: " ARDUINO_ESP8266_RELEASE "\n");
  Serial.printf("  CFG: " CFG_VERSION "\n");
  Serial.printf("  NVM: " NVM_VERSION "\n");
  Serial.printf("  compiler: " __VERSION__ "\n" );
  Serial.printf("  arduino : %d\n",ARDUINO );
  Serial.printf("  compiled: " __DATE__ ", " __TIME__ "\n" );
  Serial.printf("\n");

  // On boot: check if config button is pressed
  cfg.check(); 
  if( cfg.cfgmode() ) { cfg.setup(); return; }
  Serial.printf("\n");

  // Get/show config params for post
  Serial.printf("cfg : post http://%s%s\n",cfg.getval("postserver"), cfg.getval("posturl"));
  Serial.printf("cfg : post %s%s\n",cfg.getval("postbody1"), cfg.getval("postbody2"));
  cfg_postperiod = String(cfg.getval("postperiod")).toInt();
  if( cfg_postperiod<1000 ) cfg_postperiod = 1000;
  Serial.printf("cfg : post %dms\n",cfg_postperiod);
  // Get/show config params for get
  Serial.printf("cfg : get  http://%s%s\n",cfg.getval("getserver"), cfg.getval("geturl"));
  cfg_getperiod  = String(cfg.getval("getperiod")).toInt();
  if( cfg_getperiod<1000 ) cfg_getperiod = 1000;
  Serial.printf("cfg : get  %dms\n", cfg_getperiod);
  Serial.printf("\n");

  // Init all modules
  led_init();
  uart_init();
  wifi_init();
  tele_init();

  // Start parsing
  Serial.printf("\n");
  app_last_post = millis() - cfg_postperiod;
  app_last_get = millis() - cfg_getperiod;
}


#define SEC(ms) (((ms)+500)/1000)


void loop() {
  uint32_t now = millis();
  
  // if in config mode, do config loop (when config completes, it restarts the device)
  if( cfg.cfgmode() ) { cfg.loop(); return; }

  // If in normal app mode, get and dispatch telegrams
  Tele_Result res = tele_parser_add(SERIAL_READ());
  if( res==TELE_RESULT_AVAILABLE ) {
    // Serial.printf("emp1: available\n");
    led_on();
    for( int i=0; i<TELE_NUMFIELDS; i++ ) Serial.printf("  %-15s %s\n",tele_field_name(i), tele_field_value(i));
    if( now-app_last_post > cfg_postperiod ) {
      http_post();
      app_last_post = now;
    } else {
      Serial.printf("emp1: post: wait %us\n", SEC(cfg_postperiod-(now-app_last_post)) );
    }
    if( now-app_last_get > cfg_getperiod ) {
      http_get();
      app_last_get = now;
    } else {
      Serial.printf("emp1: get : wait %us\n", SEC(cfg_getperiod-(now-app_last_get)) );
    }
    led_off();
  }
}
