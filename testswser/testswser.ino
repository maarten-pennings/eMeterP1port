// testswser.ino -- Test the Sofwtare Serial library with a Dutch P1 port from an electricity meter
#include <core_version.h>   // ARDUINO_ESP8266_RELEASE
#include <ctype.h>          // isxdigit()
#include <SoftwareSerial.h>


// CPU =================================================================

extern "C" {
  #include "user_interface.h"
}
void cpu_init() {
  system_update_cpu_freq(SYS_CPU_160MHZ);
  Serial.printf("cpu: init (%dMHz)\n", system_get_cpu_freq() );
}


// CRC =================================================================

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

// Quick and dirty test  
void crc_test() {
  int bang_pos = 857;
  extern int p1_len;
  extern char p1_buf[]; 

  p1_len= bang_pos+7;
  memcpy(p1_buf,crc_testdata,p1_len);
  Serial.printf("crc: ");
  p1_bufcomplete(bang_pos);
}

void crc_init() {
  crc_test();
  Serial.printf("crc: init\n" );
}


// P1 ==================================================================


#ifndef EspSoftwareSerialVersion
// I patched "C:\Users\maarten\AppData\Local\Arduino15\packages\esp8266\hardware\esp8266\2.7.4\libraries\SoftwareSerial\src\SoftwareSerial.h" to have a line like:   
//   #define EspSoftwareSerialVersion "v6.8.5" // version copied from "..\library.properties"
#define EspSoftwareSerialVersion "unknown"
#warning Unknown version for EspSoftwareSerialVersion
#endif

#define  P1_BUF_SIZE 1024
char     p1_buf[P1_BUF_SIZE+7]; // +6 allows "xxxx\r\n\0" after "!" - see p1_bufadd()
int      p1_len;
uint32_t p1_time; // If p1_len>0 time of first char. If p1_len==0 time of last warning.

//SoftwareSerial p1_serial;
SoftwareSerial p1_serial(D6, -1, true); 

void p1_init() {
  //p1_serial.begin(115200,SWSERIAL_8N1,D6,D6,true,256); // isBufCapacity is derived
  //p1_serial.enableIntTx(false);
  p1_serial.begin(115200); // ESP8266 must run at @160 MHz
  Serial.print("p1 : init\n");
}

void p1_begin() {
  p1_len= 0;
  Serial.print("p1 : listening\n");
  p1_time= millis(); // p1_len==0: time of last warning
}

void p1_bufcomplete(int bang_pos) {
  if( p1_buf[bang_pos]!='!' ) Serial.printf("p1 : error in bang_pos\n");
  unsigned int crc_computed= crc_compute(p1_buf, bang_pos+1); // Compute CRC from  ['/'..'!'] inclusive
  char crc_in_telegram[5];
  strncpy(crc_in_telegram, &p1_buf[bang_pos+1], 4); 
  crc_in_telegram[4]='\0';
  int crc_ok= strtoul( crc_in_telegram,NULL,16)==crc_computed;  
  Serial.printf("p1 : telegram completed (%d bytes); crc #%04X=='%s' %s\n",p1_len,crc_computed,crc_in_telegram,crc_ok?"match":"error");
}

void p1_bufadd(uint32_t now, int len) {
  int i= p1_len;
  p1_len+= len;
  while( i<p1_len && p1_buf[i]!='!' ) i++;
  int endmarker= p1_buf[i]=='!' && isxdigit(p1_buf[i+1]) && isxdigit(p1_buf[i+2]) && isxdigit(p1_buf[i+3]) && isxdigit(p1_buf[i+4]) && p1_buf[i+5]=='\r' && p1_buf[i+6]=='\n';
  if( endmarker ) {
    // state is receiving, and endmarker found
    p1_bufcomplete(i);
    p1_len= 0; // In theory there could be chars for the next telegram
    p1_time= now; // p1_len==0: time of last warning
  } else {
    // state is receiving, and no endmarker found
    if( p1_len==P1_BUF_SIZE ) {
      Serial.printf("p1 : telegram overflow without EOT, discarding %d bytes\n",p1_len);
      p1_len= 0;
      p1_time= now; // p1_len==0: time of last warning  
    } else {
      // wait for more
    }
  }
}

void p1_read() {
  uint32_t now = millis();
  int num = 64; 
  if( num>P1_BUF_SIZE-p1_len ) num= P1_BUF_SIZE-p1_len;
  int len= p1_serial.readBytes( &p1_buf[p1_len], num );
  if( p1_len==0 ) {
    // state: idle, looking for the start of a telegram
    if( len==0 ) {
      // state is idle, and no bytes received
      if( now-p1_time >10000 ) {
        // "the data has to be sent by the P1 port to the P1 device every ten seconds"
        Serial.printf("p1 : waiting for data\n");
        p1_time = now; // p1_len==0: time of last warning
      }
    } else {
      // state is idle, and something received
      int i=0;
      while( i<len && p1_buf[i]!='/' ) i++;
      if( i==len ) {
        Serial.printf("p1 : bytes without SOT, discarding %d bytes\n",len);
        p1_time= now; // p1_len==0: time of last warning
      } else {
        memmove(&p1_buf[0],&p1_buf[i],len-i);
        p1_time= now;  // p1_len>0: time of first char
        p1_bufadd(now,len-i);
      }
    }
  } else {
    // state: receiving
    if( len==0 ) {
      // state is receiving, and no bytes received
      if( now-p1_time > 8000 ) {
        //  must complete a data transfer to the P1 device within eight seconds
        Serial.printf("p1 : timeout waiting for EOT, discarding %d bytes\n",p1_len);
        p1_len= 0;
        p1_time= now; // p1_len==0: time of last warning
      }
    } else {
      // state is receiving, and new bytes received
      p1_bufadd(now,len);
    }
  }
}


// APP =================================================================

void setup() {
  delay(2000);
  Serial.begin(115200);
  Serial.printf("\n\nWelcome to TestSwSer\n");
  Serial.printf("ver: ESP8266 board " ARDUINO_ESP8266_RELEASE "\n");
  Serial.printf("ver: SoftwareSerial " EspSoftwareSerialVersion "\n");
  cpu_init();
  crc_init();
  p1_init();  
  p1_begin();
}

void loop() {
  p1_read();
}
