// testswser.ino -- Test the Sofwtare Serial library with a Dutch P1 port from an electricity meter
#include <core_version.h>   // ARDUINO_ESP8266_RELEASE
#include <ctype.h>          // isxdigit()
#include <SoftwareSerial.h>


// CPU =================================================================


extern "C" {
  #include "user_interface.h"
}

// Switched CPU to 160MHz
void cpu_init() {
  system_update_cpu_freq(SYS_CPU_160MHZ);
  Serial.printf("cpu: init (%dMHz)\n", system_get_cpu_freq() );
}


// CRC =================================================================


// Computes and returns the CRC16 of the bytes in buf[0..len).
unsigned int crc_compute(const void *buf, int len) {
  unsigned int crc= 0;
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

// An example telegram
const char * crc_testdata = "" \
  "/KFM5KAIFA-METER\r\n" \
  "\r\n" \
  "1-3:0.2.8(42)\r\n" \
  "0-0:1.0.0(210418151702S)\r\n" \
  "0-0:96.1.1(4530303131323233333434353536363737)\r\n" \
  "1-0:1.8.1(025707.312*kWh)\r\n" \
  "1-0:1.8.2(023687.404*kWh)\r\n" \
  "1-0:2.8.1(000000.000*kWh)\r\n" \
  "1-0:2.8.2(000000.000*kWh)\r\n" \
  "0-0:96.14.0(0001)\r\n" \
  "1-0:1.7.0(01.830*kW)\r\n" \
  "1-0:2.7.0(00.000*kW)\r\n" \
  "0-0:96.7.21(00019)\r\n" \
  "0-0:96.7.9(00007)\r\n" \
  "1-0:99.97.0(2)(0-0:96.7.19)(210416081947S)(0000004676*s)(000101000011W)(2147483647*s)\r\n" \
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
  "1-0:71.7.0(002*A)\r\n" \
  "1-0:21.7.0(00.001*kW)\r\n" \
  "1-0:22.7.0(00.000*kW)\r\n" \
  "1-0:41.7.0(00.328*kW)\r\n" \
  "1-0:42.7.0(00.000*kW)\r\n" \
  "1-0:61.7.0(00.501*kW)\r\n" \
  "1-0:62.7.0(00.000*kW)\r\n" \
  "0-1:24.1.0(003)\r\n" \
  "0-1:96.1.0(4730303131323233333434353536363737)\r\n" \
  "0-1:24.2.1(210418150000S)(23770.161*m3)\r\n" \
  "!BC26\r\n";

// Quick and dirty test (returns 1 iff passed), using crc_testdata, by spoofing p1_buf
int crc_test() {
  int eotpos = 857;
  extern int p1_len;
  extern char p1_buf[]; 

  p1_len= eotpos+7;
  memcpy(p1_buf,crc_testdata,p1_len);
  return p1_crc_ok(eotpos);
}

// No real initialisation needed, only executes a CRC test
void crc_init() {
  int ok = crc_test();
  Serial.printf("crc: init (test %s)\n", ok?"pass":"FAIL" );
}


// P1 ==================================================================


// I patched SoftwareSerialVersion to publish its version number as EspSoftwareSerialVersion
#ifndef EspSoftwareSerialVersion
// I patched "C:\Users\maarten\AppData\Local\Arduino15\packages\esp8266\hardware\esp8266\2.7.4\libraries\SoftwareSerial\src\SoftwareSerial.h" to have a line like:   
//   #define EspSoftwareSerialVersion "6.8.5" // version copied from "..\library.properties"
#define EspSoftwareSerialVersion "unknown"
#warning Unknown version for EspSoftwareSerialVersion, needs a patch
#endif

#define  P1_BUF_SIZE 1500
char     p1_buf[P1_BUF_SIZE+6]; // +6 allows "xxxx\r\n" after "!" - makes the test in p1_find_eot() address allocated bytes
int      p1_len;
uint32_t p1_time; // If p1_len>0 time of first char. If p1_len==0 time of last warning.
uint32_t p1_count_ok; // number of received telegrams (with matching CRC)
uint32_t p1_count_err; // number of corrupt telegrams

//SoftwareSerial p1_serial; // ESP8266 board support: 2.6.0(?)
SoftwareSerial p1_serial(D6, -1, true); // ESP8266 board support: 2.3.0, 2.4.0, 2.7.0

void p1_init() {
  //p1_serial.begin(115200,SWSERIAL_8N1,D6,D6,true,256); // ESP8266 board support: 2.6.0
  //p1_serial.begin(115200,D6,-1,SWSERIAL_8N1,true,256); // ESP8266 board support: 2.6.0(?)
  p1_serial.begin(115200); // ESP8266 board support: 2.3.0, 2.4.0, 2.7.0
  
  p1_len= 0;
  p1_count_ok= 0;
  p1_count_err= 0;
  p1_time= millis(); // p1_len==0: time of last warning
  Serial.print("p1 : init\n");
}

// Find Begin-Of-Telegram (the '/' character), in p1_buf[0..len).
// Returns position of the BOT in p1_buf (zero based) or -1 if not found.
int p1_find_bot(int len) {
  int i=0;
  while( i<len && p1_buf[i]!='/' ) i++;
  bool found= p1_buf[i]=='/';
  if( found ) return i; else return -1;
}

// Find End-Of-Telegram (the '!XXXX\r\n'); to save time, searches in p1_buf[p1_len..p1_len+len) only.
// Returns position of the EOT (of the '!') in p1_buf (zero based) or -1 if not found.
int p1_find_eot(int len) {
  int i= min(0,p1_len-6); // We actually look 6 back, in case the EOT is read in two chuncks
  int i_end= p1_len+len; 
  while( i<i_end && p1_buf[i]!='!' ) i++; // i_end could be P1_BUF_SIZE, so i could be P1_BUF_SIZE-1. p1_buf has 6 extra bytes
  bool found= p1_buf[i]=='!' && isxdigit(p1_buf[i+1]) && isxdigit(p1_buf[i+2]) && isxdigit(p1_buf[i+3]) && isxdigit(p1_buf[i+4]) && p1_buf[i+5]=='\r' && p1_buf[i+6]=='\n';
  if( found ) return i; else return -1;
}

// Computes CRC16 of p1_buf[0..len). 
// requirements: p1_buf[0] must be the BOT, p1_buf[len] must be the EOT, and p1[len+1..len+4] must store the transmitted CRC value.
// Returns 1 if the computed CRC matches the transmitted one and 0 otherwise.
int p1_crc_ok(int len) {
  // Check BOT and EOT
  if( p1_buf[0]  !='/' ) Serial.printf("p1 : should not happen: missing BOT\n");
  if( p1_buf[len]!='!' ) Serial.printf("p1 : should not happen: missing EOT\n");
  // Compute CRC
  unsigned int crc_computed= crc_compute(p1_buf, len+1); // Compute CRC from  ['/'..'!'] inclusive
  // Get transmitted CRC
  char crc_transmitted[5];
  strncpy(crc_transmitted, &p1_buf[len+1], 4); // four hex digits, starting after '!'
  crc_transmitted[4]='\0';
  // Compare
  int crc_ok= strtoul( crc_transmitted,NULL,16)==crc_computed;  
  if( ! crc_ok ) Serial.printf("p1 : error: crc mismatch #%04X=='%s'\n",crc_computed,crc_transmitted);
  // Return
  return crc_ok;
}

// Called when a correct and complete telegram is received.
// p1_buf[0..p1_len) contains a complete telegram, and p1_buf[0]=='/',  p1_buf[p1_len]=='!', and CRC matches.
void p1_handle_ok() {
  p1_count_ok++;
  Serial.printf("p1 : telegram %u received (%d bytes)\n",p1_count_ok,p1_len);
}

#define P1_ERR_NOBOT    1 // bytes received without BOT
#define P1_ERR_TIMEOUT  2 // timeout (>8sec) waiting for EOT
#define P1_ERR_OVERFLOW 3 // telegram overflow; too many bytes received in p1_buf without an EOT
#define P1_ERR_CRCERR   4 // BOT and EOT received, but CRC error

// Called when a corrupt telegram received.
void p1_handle_err(int err, int discarded) {
  p1_count_err++;
  switch( err ) {
    case P1_ERR_NOBOT :
      Serial.printf("p1 : error: bytes without BOT, discarding %d bytes\n",discarded);
      break;
    case P1_ERR_TIMEOUT :
      Serial.printf("p1 : error: timeout waiting for EOT, discarding %d bytes\n",discarded);
      break;
    case P1_ERR_OVERFLOW :
      Serial.printf("p1 : error: telegram overflow without EOT, discarding %d bytes\n",discarded);
      break;
    case P1_ERR_CRCERR :
      Serial.printf("p1 : error: telegram has CRC error, discarding %d bytes\n",discarded);
      break;
    default  :
      Serial.printf("p1 : should not happen: unknown error tag (%d), discarding %d bytes\n",err, discarded);
      break;
  }
}

// Must be periodically called; it reads the software serial, and calls p1_handle_xxx when a telegram is received
void p1_read() {
  uint32_t now = millis();
  // How many bytes to read?
  int num = 64; 
  if( num>P1_BUF_SIZE-p1_len ) num= P1_BUF_SIZE-p1_len;
  // Read the bytes from the P1 port
  int len= p1_serial.readBytes( &p1_buf[p1_len], num );
  // Statemachine
  if( p1_len==0 && len==0 ) {
    // state: idle and no bytes received
    if( now-p1_time >10000 ) {
      // "the data has to be sent by the P1 port to the P1 device every ten seconds"
      Serial.printf("p1 : waiting for data\n");
      p1_time = now; // p1_len==0: time of last warning
    }
  } else if( p1_len==0 && len>0 ) {
    // state is idle, and some bytes received
    int botpos= p1_find_bot(len);
    if( botpos==-1 ) {
      p1_handle_err(P1_ERR_NOBOT,len); 
      p1_len= 0;
      p1_time= now; // p1_len==0: time of last warning
    } else {
      if( botpos!=0 ) p1_handle_err(P1_ERR_NOBOT, botpos-1);
      // Shift [botpos..len) to [0..len-botpos) 
      memmove(&p1_buf[0],&p1_buf[botpos],len-botpos);
      p1_time= now;  // p1_len>0: time of first char
      p1_len= len-botpos;
      // We assume that num<<size-of-telegram; this means that the first read chunck cannot have EOT
    }
  } else if( p1_len>=0 && len==0 ) {
    // state: receiving, and no bytes received
    if( now-p1_time > 8000 ) {
      //  must complete a data transfer to the P1 device within eight seconds
      p1_handle_err(P1_ERR_TIMEOUT,p1_len); 
      p1_len= 0;
      p1_time= now; // p1_len==0: time of last warning
    }
  } else if( p1_len>=0 && len>0 ) {
    // state: receiving, and new bytes received
    int eotpos= p1_find_eot(len);
    p1_len+= len; // commit received bytes
    if( eotpos==-1 ) {
      // state is receiving, and no EOT found yet
      if( p1_len==P1_BUF_SIZE ) {
        p1_handle_err(P1_ERR_OVERFLOW,p1_len);
        p1_len= 0;
        p1_time= now; // p1_len==0: time of last warning  
      }
    } else {
      // state: receiving, and EOT found
      if( ! p1_crc_ok(eotpos) ) {
        p1_handle_err(P1_ERR_CRCERR,p1_len); 
      } else {
        p1_handle_ok();
      }
      if( eotpos+7!=p1_len ) p1_handle_err(P1_ERR_NOBOT, p1_len-(eotpos+7)); // Assumption: there is no BOT of a next telegram
      p1_len= 0; 
      p1_time= now; // p1_len==0: time of last warning
    } 
  }
}


// APP =================================================================


uint32_t started;

void setup() {
  delay(2000);
  Serial.begin(115200);
  Serial.printf("\n\nWelcome to TestSwSer\n");
  Serial.printf("ver: ESP8266 board " ARDUINO_ESP8266_RELEASE "\n");
  Serial.printf("ver: SoftwareSerial " EspSoftwareSerialVersion "\n");
  cpu_init();
  crc_init();
  p1_init();  
  started = millis();
}

void loop() {
  p1_read();
  if( p1_count_ok==50 ) {
    uint32_t span= millis()-started;
    Serial.printf("app: %u intervals (of 10s): %u pass, %u fail\n", (span+5000)/10000, p1_count_ok, p1_count_err);
    p1_count_ok= 0;
    p1_count_err= 0;
    started = millis();
  }
}
