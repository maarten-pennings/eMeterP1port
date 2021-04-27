// e-meter P1 port --> webserver (thingspeak, webmsg)
// 2021 apr 17  v8  Maarten Pennings  After power failure no longer worked; improved CFG screen, added versions
// 2020 feb 10  v7  Maarten Pennings  Added to https://github.com/maarten-pennings/eMeterP1port
// 2018 nov 12  v6  Maarten Pennings  Better help strings for Cfg
// 2017 may 07  v5  Maarten Pennings  Added Cfg module
// 2017 may 06  v4  Maarten Pennings  Added get method to second server
// 2017 may 02  v3  Maarten Pennings  post via char*, not String
// 2017 apr 30  v2  Maarten Pennings  Add extra state checking
// 2017 apr 02  v1  Maarten Pennings  Created
#include <ESP8266WiFi.h>
#include "SoftwareSerial.h"
#include <string.h>
#include <stdlib.h>  
#include <Cfg.h>
extern "C" {
#include "user_interface.h"
}

#define APP_VERSION "v8"

#ifndef EspSoftwareSerialVersion
// I patched "C:\Users\maarten\AppData\Local\Arduino15\packages\esp8266\hardware\esp8266\2.7.4\libraries\SoftwareSerial\src\SoftwareSerial.h"
// To have a line like "#define EspSoftwareSerialVersion "v6.8.5" version found in "..\library.properties"
#define EspSoftwareSerialVersion "unknown"
#warning Unknown version for EspSoftwareSerialVersion
#endif


// Cfg ======================================================================================================


// The configurable fields used by this application
NvmField CfgEmp1Fields[] = {
  {"Access point"    , ""                                                  ,  0, "The eMP1 publishes data over internet. Supply credentials for a WiFi access point it should connect to. " },
  {"ssid"            , "MySSID"                                            , 32, "The ssid of the wifi network this device should connect to." },
  {"password"        , "MyPassword"                                        , 32, "The password of the wifi network this device should connect to. "},
  
  {"Server 1 (post)" , ""                                                  ,  0, "The eMP1 may publish data using the 'POST' protocol. Supply the server, URL and the post body (in two chunks), or leave blank. " },
  {"postserver"      , "api.thingspeak.com"                                , 32, "The name of the server to which measurements are send via POST (empty for none)."},
  {"posturl"         , "/update"                                           , 32, "The URL for the POST server."},
  {"postbody1"       , "field1=%L&field2=%H&field3=%P&field4=%I&field5=%D&", 64, "Body part 1 [HELP: %L=E1-kWh, %H=E2-kWh, %P=Pow-kW, %I=lo:1/hi:2, %D=E-time]."},
  {"postbody2"       , "field6=%G&field7=%T&field8=%E&key=MyWriteKeyXXXXXX", 64, "Body part 2 [HELP: %G=G-m3, %T=G-time, %E=Number-comm-errors, %p=Pow-W]. "},

  {"Server 2 (get)"  , ""                                                  ,  0, "The eMP1 may publish data using the 'GET' protocol. Supply the server and URL, or leave blank. " },
  {"getserver"       , "nwebmsg.fritz.box"  /* "192.168.179.74" */         , 32, "The name of the server to which measurements are send via GET (empty for none)."},
  {"geturl"          , "/?msg=%p&mode=right"                               , 32, "The URL for the GET server.[HELP: use % as in postbody]"},

  {0                 , 0                                                   ,  0, 0},  
};
Cfg cfg("eMP1", CfgEmp1Fields, CFG_SERIALLVL_USR);


// Process P1 data ==========================================================================================


#define P1_MAXLINELENGTH 120 // longest normal line I have seen is 86 (1-0:99.97.0 Power Failure Event Log) 
#define P1_MAXFIELDSIZE   16 // longest field in a line


// Variables of this type contain a P1 telegram _line_ transmitted by the emeter (i.e. text)
typedef struct {
  char          data[P1_MAXLINELENGTH+2]; // reserve 2 so that we can append \n\0
  int           len; // length of data data[len-1]=='\n' and data[len]==0;
} p1_line_t; 


// Flags for p1_parsed_t.flags
#define FLAGS_FOUND_START                0x01
#define FLAGS_FOUND_DATATIMESTAMP        0x02
#define FLAGS_FOUND_ELECDELIVTARIF1      0x04
#define FLAGS_FOUND_ELECDELIVTARIF2      0x08
#define FLAGS_FOUND_TARIFINDIACTOR       0x10
#define FLAGS_FOUND_ELECPOWERDELIV       0x20
#define FLAGS_FOUND_GASDATETIMESTAMP     0x40
#define FLAGS_FOUND_GASREADING           0x80
#define FLAGS_FOUND_ALL                  0xFF


// Variables of this type contain a high-level P1 telegram (extracted "relevant" fields, just the numbers)
typedef struct {
  unsigned int flags;                    // records which parts have been found
  unsigned int crc;                      // records crc of all lines from start-of-telegram till now
  unsigned int commerrors;               // number of telegrams with errors; reset when error-free one is found
  char DateTimeStamp[P1_MAXFIELDSIZE];   // field 5 of the ThingSpeak record
  char ElecDelivTarif1[P1_MAXFIELDSIZE]; // field 1 
  char ElecDelivTarif2[P1_MAXFIELDSIZE]; // field 2
  char TarifIndicator[P1_MAXFIELDSIZE];  // field 4
  char ElecPowerDeliv[P1_MAXFIELDSIZE];  // field 3
  char GasDateTimeStamp[P1_MAXFIELDSIZE];// field 7
  char GasReading[P1_MAXFIELDSIZE];      // field 6
  char Temporary[P1_MAXFIELDSIZE];       // scratch pad for subst()
} p1_parsed_t;


// Normalizes (a string representation of) a number; e.g. "000.123" -> "0.123"
void p1_normalize(char * sval) {
  char * p1= sval;
  char * p2= sval;
  // Strip leading 0s (unless next char is decimal dot)
  while( *p2=='0' && *(p2+1)!='.' ) p2++;
  // Copy all digits and decimal point
  while( *p2!='\0' ) {
    if( ('0'<=*p2 && *p2<='9') || '.'==*p2 ) { *p1=*p2; p1++; }
    p2++;
  }
  // Add terminating zero
  *p1='\0';
}


// If telegram line 't' starts starts with 'tag', copy the 1st value to 'out' ('out' must have size MAXFIELDSIZE).
// Returns true on success, false on fail.
// Example "1-0:1.7.0(01.193*kW)"  -->  "1.193"
bool p1_decode_field1(p1_line_t * t, const char * tag, const char * description, char * out) {
  if( strstr(t->data,tag)==t->data ) {
    char * start_of_val = strchr(t->data,'(');
    char * end_of_val = strchr(t->data,')');
    int len= end_of_val-start_of_val-1;
    if( start_of_val!=NULL && end_of_val!=NULL && len>0 && len<P1_MAXFIELDSIZE) {
      strncpy(out, start_of_val+1, len); out[len]='\0';
      p1_normalize(out);
      Serial.printf("  %s: %s\n",description,out);
      return true;
    }
  }
  return false;
}


// If telegram line 't'  starts starts with 'tag', copy the 2nd value to 'out' ('out' must have size MAXFIELDSIZE)
// Returns true on success, false on fail.
// Example "0-1:24.2.1(101209110000W)(12785.123*m3)" --> "12785.123"
bool p1_decode_field2(p1_line_t * t, const char * tag, const char * description, char * out) {
  if( strstr(t->data,tag)==t->data ) {
    char * start_of_val = strrchr(t->data,'(');
    char * end_of_val = strrchr(t->data,')');
    int len= end_of_val-start_of_val-1;
    if( start_of_val!=NULL && end_of_val!=NULL && len>0 && len<P1_MAXFIELDSIZE) {
      strncpy(out, start_of_val+1, len); out[len]='\0';
      p1_normalize(out);
      Serial.printf("  %s: %s\n",description,out);
      return true;
    }
  }
  return false;
}


static unsigned int CRC16(unsigned int crc, void *buf, int len) {
  for (int pos = 0; pos < len; pos++)   {
    unsigned char val = ((unsigned char*)buf)[pos];
    crc ^= (unsigned int)val;         // XOR byte into least sig. byte of crc

    for (int i = 8; i != 0; i--) {    // Loop over each bit
      if ((crc & 0x0001) != 0) {      // If the LSB is set
        crc >>= 1;                    // Shift right and XOR 0xA001
        crc ^= 0xA001;
      } else {                        // Else LSB is not set
        crc >>= 1;                    // Just shift right
      }
    }
  }

  return crc;
}


// Return values of p1_decode_stream()
#define PARSE_IDLE                                      1
#define PARSE_START_WHILE_IDLE_INFO                     2
#define PARSE_DATA_WHILE_IDLE_ERROR                     3
#define PARSE_END_WHILE_IDLE_ERROR                      4
#define PARSE_START_WHILE_PARSING_ERROR                 5
#define PARSE_DATA_WHILE_PARSING_INFO                   6
#define PARSE_END_WHILE_PARSING_DATA_MISSING_ERROR      7
#define PARSE_END_WHILE_PARSING_CRC_MISMATCH_ERROR      8
#define PARSE_END_WHILE_PARSING_COMPLETE_INFO           9


// State machine that receives telegrams line by line in 't', parses it into 'p'.
// Returns the state (PARSE_XXX) of the state machine.
int p1_decode_stream(p1_line_t * t, p1_parsed_t * p) {
  int result;
  bool crc_ok= false;
  if( t->data[0]=='/' ) { // Start of telegram found
    Serial.printf("\n");
    if( (p->flags&FLAGS_FOUND_START)==FLAGS_FOUND_START ) result=PARSE_START_WHILE_PARSING_ERROR; else result=PARSE_START_WHILE_IDLE_INFO;
    Serial.printf("Start\n");
    p->crc=CRC16(0x0000,t->data, t->len); // Reset CRC
    p->flags = FLAGS_FOUND_START; // Clear all other flags, set 'found start'
  } else if( t->data[0]=='!' ) { // End of telegram found
    p->crc= CRC16(p->crc,t->data, 1);
    char crc_in_telegram[5];
    strncpy(crc_in_telegram, t->data+1, 4); crc_in_telegram[4]='\0';
    crc_ok= strtoul( crc_in_telegram,NULL,16)==p->crc;
    if( !crc_ok ) p->commerrors++;
    Serial.printf("  %%E: number of erroneous p1 telegrams       : %d\n",p->commerrors);
    Serial.printf("End %s\n", crc_ok?"OK":"CRCERROR" );
    if( (p->flags&FLAGS_FOUND_START)!=FLAGS_FOUND_START ) result=PARSE_END_WHILE_IDLE_ERROR; 
    else if( (p->flags&FLAGS_FOUND_ALL)!=FLAGS_FOUND_ALL ) result=PARSE_END_WHILE_PARSING_DATA_MISSING_ERROR; 
    else if( !crc_ok ) result=PARSE_END_WHILE_PARSING_CRC_MISMATCH_ERROR;
    else result=PARSE_END_WHILE_PARSING_COMPLETE_INFO;
  } else { // Data of telegram found
    p->crc= CRC16(p->crc, t->data, t->len);
    if( (p->flags&FLAGS_FOUND_START)==FLAGS_FOUND_START ) result=PARSE_DATA_WHILE_PARSING_INFO; else result=PARSE_DATA_WHILE_IDLE_ERROR;
    // Pick up the wanted fields
    bool ok;
    ok=p1_decode_field1(t,"0-0:1.0.0"  ,"%D: Date-time stamp (YYMMDDhhmmss)         ",p->DateTimeStamp);    if( ok ) p->flags |= FLAGS_FOUND_DATATIMESTAMP;
    ok=p1_decode_field1(t,"1-0:1.8.1"  ,"%L: Electricity to client, tarif 1 (kWh)   ",p->ElecDelivTarif1);  if( ok ) p->flags |= FLAGS_FOUND_ELECDELIVTARIF1;
    ok=p1_decode_field1(t,"1-0:1.8.2"  ,"%H: Electricity to client, tarif 2 (kWh)   ",p->ElecDelivTarif2);  if( ok ) p->flags |= FLAGS_FOUND_ELECDELIVTARIF2;
    ok=p1_decode_field1(t,"0-0:96.14.0","%I: Tarif indicator (1=lo, 2=hi)           ",p->TarifIndicator);   if( ok ) p->flags |= FLAGS_FOUND_TARIFINDIACTOR;
    ok=p1_decode_field1(t,"1-0:1.7.0"  ,"%P: Electricity power delivered (kW) [%p=W]",p->ElecPowerDeliv);   if( ok ) p->flags |= FLAGS_FOUND_ELECPOWERDELIV;
    ok=p1_decode_field1(t,"0-1:24.2.1" ,"%T: Last gas meter reading (YYMMDDhhmmss)  ",p->GasDateTimeStamp); if( ok ) p->flags |= FLAGS_FOUND_GASDATETIMESTAMP;
    ok=p1_decode_field2(t,"0-1:24.2.1" ,"%G: Last gas meter reading (m3)            ",p->GasReading);       if( ok ) p->flags |= FLAGS_FOUND_GASREADING;
  }
  return result;
}


// e-meter P1 port: D6 for rx, no tx, inverted signals
// Constructor: SoftwareSerial(int8_t rxPin, int8_t txPin = -1, bool invert = false);
#define P1_RXPIN D6
SoftwareSerial p1_serial(P1_RXPIN, -1, true); // See begin(), it re-sets these values
p1_line_t      p1_line;


// Read data from the emeter (if available) and feed to state machine)
// Returns the state (PARSE_XXX) of the state machine.
int p1_read_emeter(p1_parsed_t * p) {
  int result = PARSE_IDLE;
  p1_line_t *t = &p1_line;
  while( p1_serial.available() ) {
    t->len= p1_serial.readBytesUntil('\n', t->data, P1_MAXLINELENGTH); // t->data does not have the \n
    if( t->len==P1_MAXLINELENGTH ) Serial.printf("  WARNING: line length overflow\n");
    // t->data has 2 more free locations after P1_MAXLINELENGTH
    t->data[t->len++]= '\n';
    t->data[t->len]= 0;
    // Serial.print(t->data);
    result = p1_decode_stream(t,p);
    if( result==PARSE_END_WHILE_PARSING_COMPLETE_INFO ) break;
    delay(1);
  } 
  return result;
}


// LED ==========================================================================================

 
// GPIO16 == D4 == BLUE led (LED on == D4 low)
#define LED_BLUEPIN    2  

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
}


// Wifi =================================================================================================


void wifi_init() {
  // Enable WiFi
  WiFi.mode(WIFI_STA);
  Serial.printf("Connecting to %s ",cfg.getval("ssid"));
  WiFi.begin(cfg.getval("ssid"), cfg.getval("password") );
  while( WiFi.status()!=WL_CONNECTED ) {
    led_toggle();
    delay(250);
    Serial.printf(".");
  }
  Serial.printf(" connected\n");
  Serial.printf("IP is %s\n",WiFi.localIP().toString().c_str());
  led_off();
}


// POST server==========================================================================================


int subst(char *buf, int size, char * fmt, p1_parsed_t * p ) {
  double temporary;
  int result=0;
  char *r=fmt;
  char *w=buf;
  while( *r!='\0' && size>1 ) {
    if( *r=='%' ) {
      // Escape character (%); inspect next
      char *val=0;
      r++;
      switch( *r ) {
        case 'D' : val= p->DateTimeStamp; break;
        case 'L' : val= p->ElecDelivTarif1; break;
        case 'H' : val= p->ElecDelivTarif2; break;
        case 'I' : val= p->TarifIndicator; break;
        case 'P' : val= p->ElecPowerDeliv; break;
        case 'T' : val= p->GasDateTimeStamp; break;
        case 'G' : val= p->GasReading; break;
        case 'E' : val= p->Temporary; sprintf(p->Temporary,"%d",p->commerrors); break;
        case 'p' : val= p->Temporary; temporary=atof(p->ElecPowerDeliv); sprintf(p->Temporary,"%d",(int)(temporary*1000)); break;
        case '%' : r++; /* no-break */
        case '\0': /* no-break; */
        default  : *w='%'; w++; size--; result++; break;
      }
      if( val!=0 ) {
        int len=snprintf(w,size,"%s",val); 
        if( len>size-1) len=size-1;
        w+=len; size-=len; r++; result+=len;
      }
    } else {
      // Plain character; copy
      *w=*r; w++; size--; r++; result++; 
    }
  }
  *w='\0'; w++; size--;
  return result;
}


#define BUF_SIZE 256
char buf[BUF_SIZE];


void post_write(p1_parsed_t * p) {
  char * srv = cfg.getval("postserver");
  if( *srv=='\0' ) { Serial.printf("  No 'post' server\n"); return; }
  
  WiFiClient client;
  if( client.connect(srv,80) ) {
    // Create body with the fields
    int len1=subst(buf,BUF_SIZE,cfg.getval("postbody1"),p);
    int len2=subst(buf+len1,BUF_SIZE-len1,cfg.getval("postbody2"),p);
    int len=len1+len2;
    if( len+1==BUF_SIZE ) Serial.printf("  WARNING: body truncated\n");
    //Serial.printf("  '%s'\n",buf);
    // Construct API request body
    client.print("POST "); client.print(cfg.getval("posturl")); client.print(" HTTP/1.1\r\n");
    client.print("Host: "); client.print(srv); client.print("\r\n");
    client.print("Connection: close\r\n");
    client.print("Content-Type: application/x-www-form-urlencoded\r\n");
    client.print("Content-Length: "); client.print(len); client.print("\r\n");
    client.print("\r\n");
    client.print(buf); client.print("\r\n");
    client.print("\r\n");
    Serial.printf("  Send to %s\n",srv);  
  } else {
    Serial.printf("  Cannot connect to %s\n", srv);  
  }
  delay(1);
  client.stop();
}



// GET server =========================================================================================


void get_write(p1_parsed_t * p) {
  char * srv = cfg.getval("getserver");
  if( *srv=='\0' ) { Serial.printf("  No 'get' server\n"); return; }

  WiFiClient client;
  if( client.connect(srv,80) ) {
    // Create body with the fields
    int len=subst(buf,BUF_SIZE,cfg.getval("geturl"),p);
    //Serial.printf("  '%s'\n",buf);
    if( len+1==BUF_SIZE ) Serial.printf("  WARNING: url truncated\r\n");
    // Construct API request body
    client.print("GET "); client.print(buf); client.print(" HTTP/1.1\r\n");
    client.print("Host: "); client.print(cfg.getval("getserver")); client.print("\r\n");
    client.print("Connection: close\r\n");
    client.print("\r\n");
    Serial.printf("  Send to %s\n", srv);  
  } else {
    Serial.printf("  Cannot connect to %s\n", srv);  
  }
  delay(1);
  client.stop();
}


// main ============================================================================================================


p1_parsed_t   parsed;
void setup() {
  // Set host name 
  WiFi.hostname("eMP1");  
  
  // Debug prints
  delay(1000);
  Serial.begin(115200);
  Serial.printf("\n\nWelcome to e-meter P1 port\n");
  Serial.printf("  App: " APP_VERSION "\n");
  Serial.printf("  CFG: " CFG_VERSION "\n");
  Serial.printf("  NVM: " NVM_VERSION "\n");
  Serial.printf("  EspSoftwareSerial: " EspSoftwareSerialVersion "\n");

  // Speed
  system_update_cpu_freq(SYS_CPU_160MHZ);
  Serial.printf("Running at %dMHz\n\n", system_get_cpu_freq() );
  
  // On boot: check if config button is pressed
  cfg.check(); 
  if( cfg.cfgmode() ) { cfg.setup(); return; }

  // Led
  led_init();

  // Wifi
  wifi_init();
  
  // P1 port
  // void begin(uint32_t baud, SoftwareSerialConfig config, int8_t rxPin, int8_t txPin, bool invert, int bufCapacity = 64, int isrBufCapacity = 0);
  p1_serial.begin(115200, SWSERIAL_8N1, P1_RXPIN, -1, true, 1024 ); // isBufCapacity is derived
  //p1_serial.begin(115200);
  parsed.flags = 0; // init statemachine
  parsed.commerrors = 0; // no p1 read errors yet
}


unsigned long lastsend = 0;
unsigned long lastparsed = 0;
void loop() {
  // if in config mode, do config loop (when config completes, it restarts the device)
  if( cfg.cfgmode() ) { cfg.loop(); return; }

  unsigned long now = millis();
  int result= p1_read_emeter(&parsed);
  if( result!=PARSE_IDLE ) lastparsed= millis();
  switch( result ) {
  case PARSE_IDLE : 
    if( now-lastparsed>10*1000 ) {
      Serial.printf("\nTelegram expected - 10 seconds without one\n");
      lastparsed = now;
    }
    break;
  case PARSE_START_WHILE_IDLE_INFO : 
    Serial.printf("  Telegram being received - start found\n");
    break;
  case PARSE_DATA_WHILE_IDLE_ERROR : 
    Serial.printf("  Telegram being received - ERROR data without start\n");
    break;
  case PARSE_END_WHILE_IDLE_ERROR : 
    Serial.printf("  Telegram being received - ERROR end without start\n");
    break;
  case PARSE_START_WHILE_PARSING_ERROR : 
    Serial.printf("  Telegram being received - ERROR start after start\n");
    break;
  case PARSE_DATA_WHILE_PARSING_INFO : 
    // Serial.printf("  Telegram being received - data field received\n");
    break;
  case PARSE_END_WHILE_PARSING_DATA_MISSING_ERROR : 
    Serial.printf("  Telegram complete - data missing (previous: %lu ms)\n", now-lastsend );
    for(int i=0; i<10; i++) { led_toggle(); delay(50); } led_off(); 
    Serial.printf("Done\n");
    break;
  case PARSE_END_WHILE_PARSING_CRC_MISMATCH_ERROR : 
    Serial.printf("  Telegram complete - crc mismatch (previous: %lu ms)\n", now-lastsend );
    for(int i=0; i<10; i++) { led_toggle(); delay(50); } led_off();
    Serial.printf("Done\n");
    break;
  case PARSE_END_WHILE_PARSING_COMPLETE_INFO : 
    if( now-lastsend>60*1000 || lastsend==0 ) {
      led_on();
      Serial.printf("  Telegram complete - prepare to send (previous: %lu ms)\n", now-lastsend );
      post_write(&parsed);
      lastsend= now;
      parsed.commerrors=0;
      led_off();
      delay(200);
    } else {
      Serial.printf("  Telegram complete - too early (previous: %lu ms)\n", now-lastsend );
    }
    led_on();
    get_write(&parsed);
    led_off();
    Serial.printf("Done\n");
    break;
  default :
    Serial.printf("  Telegram status error - status %d\n",result);
    break;
  } 
}



/*
Example from http://domoticx.com/p1-poort-slimme-meter-hardware/

/ISk5\2MT382-1 000
1-3:0.2.8(40)
0-0:1.0.0(101209113020W)
0-0:96.1.1(4B384547303034303436333935353037)
1-0:1.8.1(123456.789*kWh)
1-0:1.8.2(123456.789*kWh)
1-0:2.8.1(123456.789*kWh)
1-0:2.8.2(123456.789*kWh)
0-0:96.14.0(0002)
1-0:1.7.0(01.193*kW)
1-0:2.7.0(00.000*kW)
0-0:17.0.0(016.1*kW)
0-0:96.3.10(1)
0-0:96.7.21(00004)
0-0:96.7.9(00002)
1-0:99:97.0(2)(0:96.7.1 9)(101208152415W)(0000000240*s)(101208151004W)(00000000301*s)
1-0:32.32.0(00002)
1-0:52.32.0(00001)
1-0:72:32.0(00000)
1-0:32.36.0(00000)
1-0:52.36.0(00003)
1-0:72.36.0(00000)
0-0:96.13.1(3031203631203831)
0-0:96.13.0(303132333435363738393A3B3C3D3E3F303132333435363738393A3B3C3D3E3F303132333435363738393A3B
3C3D3E3F303132333435363738393A3B3C3D3E3F303132333435363738393A3B3C3D3E3F)
0-1:24.1.0(03)
0-1:96.1.0(3232323241424344313233343536373839)
0-1:24.2.1(101209110000W)(12785.123*m3)
0-1:24.4.0(1)
!522B

*/
