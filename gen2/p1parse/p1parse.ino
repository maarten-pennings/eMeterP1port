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


// TODO replace id and CRC
#define TELEGRAM_EXAMPLE_1 \
  "oops1\r\n" \
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
  "!A62C\r\n" \
  "oops2\r\n" 


// From http://domoticx.com/p1-poort-slimme-meter-hardware/
#define TELEGRAM_EXAMPLE_2 \
  "/XMX5LGBBFG10\r\n" \
  "\r\n" \
  "1-3:0.2.8(42)\r\n" \
  "0-0:1.0.0(170108161107W)\r\n" \
  "0-0:96.1.1(4530303331303033303031363939353135)\r\n" \
  "1-0:1.8.1(002074.842*kWh)\r\n" \
  "1-0:1.8.2(000881.383*kWh)\r\n" \
  "1-0:2.8.1(000010.981*kWh)\r\n" \
  "1-0:2.8.2(000028.031*kWh)\r\n" \
  "0-0:96.14.0(0001)\r\n" \
  "1-0:1.7.0(00.494*kW)\r\n" \
  "1-0:2.7.0(00.000*kW)\r\n" \
  "0-0:96.7.21(00004)\r\n" \
  "0-0:96.7.9(00003)\r\n" \
  "1-0:99.97.0(3)(0-0:96.7.19)(160315184219W)(0000000310*s)(160207164837W)(0000000981*s)(151118085623W)(0000502496*s)\r\n" \
  "1-0:32.32.0(00000)\r\n" \
  "1-0:32.36.0(00000)\r\n" \
  "0-0:96.13.1()\r\n" \
  "0-0:96.13.0()\r\n" \
  "1-0:31.7.0(003*A)\r\n" \
  "1-0:21.7.0(00.494*kW)\r\n" \
  "1-0:22.7.0(00.000*kW)\r\n" \
  "0-1:24.1.0(003)\r\n" \
  "0-1:96.1.0(4730303139333430323231313938343135)\r\n" \
  "0-1:24.2.1(170108160000W)(01234.000*m3)\r\n" \
  "!63A1\r\n"


// A filter describes an obis object that we are interested in.
//  name is a short name
//  description is description from standard, see https://www.netbeheernederland.nl/_upload/Files/Slimme_meter_15_a727fce1f1.pdf
//  obis is referenec num from standard
//  open_delim is the character just in front of the value (right most)
//  close_delim is the character just after the value (right most)
//  fieldname is a character that we use in defining the template upload string


class Telegram_Filter {
  public:
    Telegram_Filter(const char * name, const char *description, const char *obis, char open_delim, char close_delim, char fieldname): 
      name(name), description(description), obis(obis), open_delim(open_delim), close_delim(close_delim), fieldname(fieldname) {};
    const char* name;
    const char* description;
    const char* obis;
    char        open_delim;
    char        close_delim;
    char        fieldname; 
};


Telegram_Filter telegram_filters[] = {
  Telegram_Filter( "In-Day-kWh"   , "Meter Reading electricity delivered to client (Tariff 1)", "1-0:1.8.1"  , '(', '*', 'L' ),
  Telegram_Filter( "In-Night-kWh" , "Meter Reading electricity delivered to client (Tariff 2)", "1-0:1.8.2"  , '(', '*', 'H' ),

  Telegram_Filter( "Out-Day-kWh"  , "Meter Reading electricity delivered by client (Tariff 1)", "1-0:2.8.1"  , '(', '*', 'l' ),
  Telegram_Filter( "Out-Night-kWh", "Meter Reading electricity delivered by client (Tariff 2)", "1-0:2.8.2"  , '(', '*', 'h' ),

  Telegram_Filter( "Night1-Day2"  , "Tariff indicator electricity"                            , "0-0:96.14.0", '(', ')', 'I' ),

  Telegram_Filter( "In-W"         , "Actual electricity power delivered (+P)"                 , "1-0:1.7.0"  , '(', '*', 'P' ),
  Telegram_Filter( "Out-W"        , "Actual electricity power received (-P)"                  , "1-0:2.7.0"  , '(', '*', 'P' ),

  Telegram_Filter( "Fails-short"  , "Number of power failures in any phase"                   , "0-0:96.7.21", '(', ')', 'F' ),
  Telegram_Filter( "Fails-long"   , "Number of long power failures in any phase"              , "0-0:96.7.9" , '(', ')', 'f' ),

//  Telegram_Filter( "In-L1-W"      , "Instantaneous power L1 (+P)"                             , "1-0:21.7.0" , '(', '*', 'A' ),
//  Telegram_Filter( "Out-L1-W"     , "Instantaneous power L1 (-P)"                             , "1-0:22.7.0" , '(', '*', 'a' ),
//  Telegram_Filter( "In-L2-W"      , "Instantaneous power L2 (+P)"                             , "1-0:41.7.0" , '(', '*', 'B' ),
//  Telegram_Filter( "Out-L2-W"     , "Instantaneous power L2 (-P)"                             , "1-0:42.7.0" , '(', '*', 'b' ),
//  Telegram_Filter( "In-L3-W"      , "Instantaneous power L3 (+P)"                             , "1-0:61.7.0" , '(', '*', 'C' ),
//  Telegram_Filter( "Out-L3-W"     , "Instantaneous power L3 (-P)"                             , "1-0:62.7.0" , '(', '*', 'c' ),

  Telegram_Filter( "Gas-m3"       , "Last 5-minute value gas delivered to client"             , "0-1:24.2.1" , '(', '*', 'G' ),
};


#define TELEGRAM_NUMFILTERS      ( sizeof(telegram_filters) / sizeof(Telegram_Filter) )



#define TELEGRAM_VALUE_MAXCHAR   16

class Telegram_Object {
  public:
    bool present;
    char value[TELEGRAM_VALUE_MAXCHAR];
};

Telegram_Object telegram_objects[TELEGRAM_NUMFILTERS];



enum TelegramState {
  idle,
  head,
  body,
  csum
};

#define TELEGRAM_MAXWAIT_MS 10000
#define TELEGRAM_MAXLINELEN 128

class Telegram {
  public :
    void clr_stat();
    int  get_stat_fail();
    int  get_stat_good();
    void set_idle();
    void add(int ch);
  private:
    void update_crc(int len);
    bool header_ok();
    bool bodyln_ok();
    bool csumln_ok();
  private:
    TelegramState _state;
    uint32_t      _time;
    char          _data[TELEGRAM_MAXLINELEN];
    int           _len;
    unsigned int  _crc;
    int           _numfail;
    int           _numgood;
};



void Telegram::clr_stat() {
  _numfail = 0;
  _numgood = 0;
}


int Telegram::get_stat_fail() {
  return _numfail;
}


int Telegram::get_stat_good() {
  return _numgood;
}


void Telegram::update_crc(int len) {
  for( int pos=0; pos<len; pos++ ) {
    _crc ^= _data[pos];
    for(int i=8; i!=0; i--) {
      int bit = _crc & 0x0001;
      _crc >>= 1;
      if( bit ) _crc ^= 0xA001; // If the LSB is set xor with polynome x16+x15+x2+1
    }
  }
}


// Forces telegram parse to the idel state
void Telegram::set_idle() {
  _state = idle;
  _time = millis(); // in idle: time of last error; otherwise time of first char in telegram
  _len = 0; // in idle: number of discarded bytes (before header); otherwise num of chars in current line
}


bool Telegram::header_ok() {
  // "/KFM5KAIFA-METER<CR><LF><CR><LF>"
  bool ok = _len>8 && _data[0]=='/' && _data[4]=='5' && _data[_len-4]=='\r' && _data[_len-3]=='\n' && _data[_len-2]=='\r' && _data[_len-1]=='\n';
  if( !ok ) {
    _data[_len-4] = '\0';
    Serial.printf("tele: ERROR header line corrupt '%s'\n",_data);
    return false;
  }

  update_crc(_len);
  //_data[_len-4]='\0';
  //Serial.printf("tele: header received '%s'\n",_data);

  for( int i=0; i<TELEGRAM_NUMFILTERS; i++ ) {
    telegram_objects[i].present = false; // flag all objects as not found
  }
  
  return true;
}


bool Telegram::bodyln_ok() {
  // "1-0:1.8.1(012345.678*kWh)<CR><LF>"
  bool ok = _len>2 && _data[_len-2]=='\r' && _data[_len-1]=='\n';
  if( !ok ) {
    _data[_len-2] = '\0';
    Serial.printf("tele: ERROR body line corrupt '%s'\n",_data);
    return false;
  }

  update_crc(_len);
  _data[_len-2]='\0';
  //Serial.printf("tele: body line received '%s'\n",_data);

  for( int i=0; i<TELEGRAM_NUMFILTERS; i++ ) {
    if( strncmp(_data, telegram_filters[i].obis, strlen(telegram_filters[i].obis))==0 ) {
      char * pos_open_delim  = strrchr( _data, telegram_filters[i].open_delim  );
      if( pos_open_delim==0 ) {
        Serial.printf("tele: ERROR body line '%s' could not find open delim '%c'\n",_data,telegram_filters[i].open_delim);
        return false;
      }
      char * pos_close_delim = strrchr( _data, telegram_filters[i].close_delim );
      if( pos_open_delim==0 ) {
        Serial.printf("tele: ERROR body line '%s' could not find close delim '%c'\n",_data,telegram_filters[i].close_delim);
        return false;
      }

      int len = pos_close_delim - pos_open_delim - 1;
      if( len<1 || len>=TELEGRAM_VALUE_MAXCHAR ) {
        Serial.printf("tele: ERROR body line '%s' datwaidth too large (%d)\n",_data,len);
        return false;
      }
      
      telegram_objects[i].present = true;
      memcpy( telegram_objects[i].value, pos_open_delim+1, len );
      telegram_objects[i].value[len] - '0';

      //Serial.printf("tele: %s %s\n",telegram_filters[i].name, telegram_objects[i].value);
    }
  }
  return true;
}


bool Telegram::csumln_ok() {
  // "!70CE<CR><LF>"
  bool ok = _len==7 && _data[0]=='!' && isxdigit(_data[1]) && isxdigit(_data[2]) && isxdigit(_data[3]) && isxdigit(_data[4]) && _data[5]=='\r' && _data[6]=='\n';
  if( !ok ) {
    _data[_len-2] = '\0';
    Serial.printf("tele: ERROR csum line corrupt '%s'\n",_data);
    return false;
  }

  update_crc(1); // include '!' in crc
  int c1 = toupper(_data[1]);
  int d1 = (c1>='A') ? (c1-'A'+10) : (c1-'0');
  int c2 = toupper(_data[2]);
  int d2 = (c2>='A') ? (c2-'A'+10) : (c2-'0');
  int c3 = toupper(_data[3]);
  int d3 = (c3>='A') ? (c3-'A'+10) : (c3-'0');
  int c4 = toupper(_data[4]);
  int d4 = (c4>='A') ? (c4-'A'+10) : (c4-'0');
  int csum = ((d1*16+d2)*16+d3)*16+d4;

  if( csum!=_crc ) {
    Serial.printf("tele: ERROR crc mismatch (actual %04X, telegram says %04X)\n",_crc,csum);
    return false;
  }

  // Check if all obkects have values
  for( int i=0; i<TELEGRAM_NUMFILTERS; i++ ) {
    if( ! telegram_objects[i].present ) {
      Serial.printf("tele: missing field '%s' with code '%s'\n",telegram_filters[i].name,telegram_filters[i].obis);
      return false;
    }
  }

  //_data[_len-2]='\0';
  //Serial.printf("tele: csum line received '%s'\n",_data);
  
  return true;  
}


void Telegram::add(int ch) {
  // if( ch=='\n') Serial.printf("\\n\n[%d,%d]",_state,_len);
  // else if( ch=='\r') Serial.printf("\\r",ch);
  // else if( ch!=-1) Serial.printf("%c",ch);

  // too long no data?
  if( ch<0 ) {
    if( millis() - _time > TELEGRAM_MAXWAIT_MS ) {
      if( _len>0 ) Serial.printf("tele: ... %d bytes of telegram data discarded, too long no data\n",_len);
      else Serial.printf("tele: ERROR no telegram data received\n");
      _numfail++;
      set_idle();
    }
    return;
  }

  // Waiting for header
  if( _state==idle ) {
    if( ch=='/' ) {
      // Start of header. Init data collection, reset time to start of telegram
      if( _len>0 ) { Serial.printf("tele: ... %d bytes of telegram data discarded, now header\n",_len); _numfail++; }
      _len= 0;
      _data[_len++]= ch;
      _time= millis();
      _state= head;
      _crc= 0x0000; // initial value
    } else {
      // bytes come in without header
      if( _len==0 ) Serial.printf("tele: ERROR data without header ...\n");
      _len++;
    }
    return;
  }

  // Does character fit?
  if( _len==TELEGRAM_MAXLINELEN ) {
    Serial.printf("tele: ERROR line too long (%d)\n",_len);
    _numfail++;
    set_idle();
    return;
  }
  _data[_len++]= ch;
 
  switch( _state ) {
  case head:
    // Get e.g. "/KFM5KAIFA-METER<CR><LF><CR><LF>"
    if( _len>3 && _data[_len-3]=='\n' && _data[_len-1]=='\n' ) { // include whiteline
      // Header is complete (including whiteline)
      if( header_ok() ) {
        _state= body;
        _len= 0;
      } else {
        _numfail++;
        set_idle();
      }
    }  
    break;
    
  case body:
    // Get e.g. "1-0:1.8.1(012345.678*kWh)<CR><LF>"
    if( ch=='!' ) {
      _state = csum;
    } else if( ch=='\n' ) {
      // Bodyline is complete
      if( bodyln_ok() ) {
        _len= 0;
      } else {
        _numfail++;
       set_idle();
      }
    }
    break;
    
  case csum:
    // get e.g. "!70CE<CR><LF>"
    if( ch=='\n' ) {
      // CRC is complete
      if( csumln_ok() ) _numgood++; else _numfail++;
      set_idle();
    }
    break;
  
  } // switch
}


Telegram telegram;

void setup() {
  Serial.begin(115200, SERIAL_8N1, SERIAL_FULL);
  do delay(250); while( !Serial );
  Serial.printf("\n\n\nWelcome to p1read\n");

  Serial.flush();
  USC0(UART0) = USC0(UART0) | BIT(UCRXI);
  Serial.printf("seri: RX inverted\n");

  Serial.printf("\n");
  telegram.set_idle();
}


#if 1
const char *s = TELEGRAM_EXAMPLE_1 TELEGRAM_EXAMPLE_2 ;
int ix = -25;

void loop() {
  if( ix<0 ) {
    delay(1000);
    telegram.add(-1);
    ix++;
  }
  if( ix<strlen(s) ) {
    int ch= s[ix++]; // Serial.read();
    if( ch!=-1 ) telegram.add(ch);
  }
  if( telegram.get_stat_good()>0 ) {
    Serial.printf("tele: fail %d good %d\n",telegram.get_stat_fail(),telegram.get_stat_good() );
    for( int i=0; i<TELEGRAM_NUMFILTERS; i++ ) {
      Serial.printf("  %s %s\n",telegram_filters[i].name, telegram_objects[i].value);
    }
    telegram.clr_stat();
  }
  delay(1);
}
#else
void loop() {
  telegram.add(Serial.read());

  if( telegram.get_stat_good()>0 ) {
    Serial.printf("tele: fail %d good %d\n",telegram.get_stat_fail(),telegram.get_stat_good() );
    for( int i=0; i<TELEGRAM_NUMFILTERS; i++ ) {
      Serial.printf("  %-15s %s\n",telegram_filters[i].name, telegram_objects[i].value);
    }
    telegram.clr_stat();
  }

}
#endif

/*

- check timeout in all states

*/
