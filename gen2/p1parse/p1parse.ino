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



// === FIELD ====================================================================================
// A field describes an obis object that we are interested in.


// Once a field in the telegram is parsed and passes all checks, 
// its value is made available through Tele_Field.value[].
// That is a string with storage size TELE_VALUE_SIZE.
#define TELE_VALUE_SIZE   16


// The following class represents one field, we make one instance per obis object that we are interested in.
// All field properties are compile time constants, except `value`,
// which becomes available after parsing a telegram successfully.
//  key         ultra short name (1 character) used in printf-like format strings
//  name        is the name (5-15 chars)
//  description is description from standard, see https://www.netbeheernederland.nl/_upload/Files/Slimme_meter_15_a727fce1f1.pdf
//  obis        is obis code, like "1-0:1.8.1", from the standard
//  open_delim  is the character just in front of the value (right most)
//  close_delim is the character just after the value (right most)
//  value       is filled with the actual parsed value (once parsing is successful) 
class Tele_Field {
  public:
    Tele_Field(char key, const char * name, const char *description, const char *obis, char open_delim, char close_delim): 
      key(key), name(name), description(description), obis(obis), open_delim(open_delim), close_delim(close_delim) {};
    const char         key;
    const char * const name;
    const char * const description;
    const char * const obis;
    const char         open_delim;
    const char         close_delim;
          char         value[TELE_VALUE_SIZE];
};


// These are the fields that I'm interested in, feel free to modify
Tele_Field tele_fields[] = {
  Tele_Field( 'L', "Cons-Day-kWh"   , "Meter Reading electricity delivered to client (Tariff 1)", "1-0:1.8.1"  , '(', '*' ),
  Tele_Field( 'H', "Cons-Night-kWh" , "Meter Reading electricity delivered to client (Tariff 2)", "1-0:1.8.2"  , '(', '*' ),
                     
  Tele_Field( 'l', "Prod-Day-kWh"   , "Meter Reading electricity delivered by client (Tariff 1)", "1-0:2.8.1"  , '(', '*' ),
  Tele_Field( 'h', "Prod-Night-kWh" , "Meter Reading electricity delivered by client (Tariff 2)", "1-0:2.8.2"  , '(', '*' ),
                     
  Tele_Field( 'I', "Night1-Day2"    , "Tariff indicator electricity"                            , "0-0:96.14.0", '(', ')' ),
                     
  Tele_Field( 'P', "Cons-kW"        , "Actual electricity power delivered (+P)"                 , "1-0:1.7.0"  , '(', '*' ),
  Tele_Field( 'P', "Prod-kW"        , "Actual electricity power received (-P)"                  , "1-0:2.7.0"  , '(', '*' ),
                     
  Tele_Field( 'F', "Fails-short-#"  , "Number of power failures in any phase"                   , "0-0:96.7.21", '(', ')' ),
  Tele_Field( 'f', "Fails-long-#"   , "Number of long power failures in any phase"              , "0-0:96.7.9" , '(', ')' ),
                     
  Tele_Field( 'A', "Cons-L1-kW"     , "Instantaneous power L1 (+P)"                             , "1-0:21.7.0" , '(', '*' ),
  Tele_Field( 'a', "Prod-L1-kW"     , "Instantaneous power L1 (-P)"                             , "1-0:22.7.0" , '(', '*' ),
  Tele_Field( 'B', "Cons-L2-kW"     , "Instantaneous power L2 (+P)"                             , "1-0:41.7.0" , '(', '*' ),
  Tele_Field( 'b', "Prod-L2-kW"     , "Instantaneous power L2 (-P)"                             , "1-0:42.7.0" , '(', '*' ),
  Tele_Field( 'C', "Cons-L3-kW"     , "Instantaneous power L3 (+P)"                             , "1-0:61.7.0" , '(', '*' ),
  Tele_Field( 'c', "Prod-L3-kW"     , "Instantaneous power L3 (-P)"                             , "1-0:62.7.0" , '(', '*' ),
                     
  Tele_Field( 'G', "Cons-Gas-m3"    , "Last 5-minute value gas delivered to client"             , "0-1:24.2.1" , '(', '*' ),
};


// The total number of fields the parser should look for.
#define TELE_NUMFIELDS      ( sizeof(tele_fields) / sizeof(Tele_Field) )



// === PARSER ====================================================================================================
// The parse is fed one character at a time (or -1 if there is no new character - this is needed to manage time-outs)
// Once a complete telegram is received and approved (eg CRC), the results are published via tele_fields[].


#define TELE_MAXWAIT_MS 10000  // telegram is repeated this many ms
#define TELE_LINE_SIZE   2100  // telegram can have 1024 char message, each char encoded as HexHex


// The internal states of the parser
enum Tele_State {
  TELE_STATE_IDLE, // waiting for the first character (of the header)
  TELE_STATE_HEAD, // collecting header characters (until the whiteline)
  TELE_STATE_BODY, // collecting all obis objects, if the match a field definition the value is copied there
  TELE_STATE_CSUM  // collecting the CRC
};


// The add() function will return the abstract state of the parse
enum Tele_Result {
  TELE_RESULT_ERROR,      // a partial telegram is received but there was an error (timeout, syntax error, crc error, field missing).
  TELE_RESULT_COLLECTING, // telegram data is still being collected, no errors have been found yet, but the telegram is also not yet complete.
  TELE_RESULT_AVAILABLE,  // a complete telegram is received, its CRC matches, and all fields (as specified in tele_fields[]) are found.
};


// The parser
class Tele_Parser {
  public :
    void          begin();
    Tele_Result   add(int ch);
  private:
    void          set_state_idle();
    void          set_state_head();
    void          set_state_body();
    void          set_state_csum();
  private:
    void          update_crc(int len);
    bool          header_ok();
    bool          bodyln_ok();
    bool          csumln_ok();
  private:
    Tele_State    _state;
    uint32_t      _time;
    char          _data[TELE_LINE_SIZE];
    int           _len;
    unsigned int  _crc;
};


// Sets the parse to an intial state
void Tele_Parser::begin() {
  set_state_idle();
}


// Updates the `_crc` with the bytes in `_data`, op to `len`.
void Tele_Parser::update_crc(int len) {
  for( int pos=0; pos<len; pos++ ) {
    _crc ^= _data[pos];
    for(int i=8; i!=0; i--) {
      int bit = _crc & 0x0001;
      _crc >>= 1;
      if( bit ) _crc ^= 0xA001; // If the LSB is set xor with polynome x16+x15+x2+1
    }
  }
}


// Forces telegram parse to the idle state
void Tele_Parser::set_state_idle() {
  _state = TELE_STATE_IDLE;
  _time = millis(); // in idle: time of last error
  _len = 0; // in idle: number of discarded bytes (before header)
}


// Forces telegram parse to the head state
void Tele_Parser::set_state_head() {
  _state= TELE_STATE_HEAD;
  _time= millis(); // time of first char in telegram
  _len= 0; // num of chars in current line
  _crc= 0x0000; // initial value for checksum
}


// Forces telegram parse to the body state
void Tele_Parser::set_state_body() {
  _state = TELE_STATE_BODY;
  // _time= millis(); // do not reset time of first char in telegram
  _len= 0; // num of chars in current line
}


// Forces telegram parse to the check sum state
void Tele_Parser::set_state_csum() {
  _state = TELE_STATE_CSUM;
  // _time= millis(); // do not reset time of first char in telegram
  // _len = 0; // do not reset, was already collecting as body line
}


// Returns true iff the `_data[0.._len)` is a valid header.
// Prints and error if not.
// Updates CRC with header bytes, clears field values
bool Tele_Parser::header_ok() {
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

  // Flag all fields as not found
  for( int i=0; i<TELE_NUMFIELDS; i++ ) {
    tele_fields[i].value[0] = '\0'; 
  }
  
  return true;
}


// Returns true iff the `_data[0.._len)` is a body line (obis object).
// Prints and error if not.
// Updates CRC with received bytes, sets field value if it matches one of the fields
bool Tele_Parser::bodyln_ok() {
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

  for( int i=0; i<TELE_NUMFIELDS; i++ ) {
    if( strncmp(_data, tele_fields[i].obis, strlen(tele_fields[i].obis))==0 ) {
      char * pos_open_delim  = strrchr( _data, tele_fields[i].open_delim  );
      if( pos_open_delim==0 ) {
        Serial.printf("tele: ERROR body line '%s' could not find open delim '%c'\n",_data,tele_fields[i].open_delim);
        return false;
      }
      char * pos_close_delim = strrchr( _data, tele_fields[i].close_delim );
      if( pos_open_delim==0 ) {
        Serial.printf("tele: ERROR body line '%s' could not find close delim '%c'\n",_data,tele_fields[i].close_delim);
        return false;
      }

      int len = pos_close_delim - pos_open_delim - 1;
      if( len<=0 || len>=TELE_VALUE_SIZE ) { 
        // 0 len not allowed (and empty value means not found)
        // TELE_VALUE_SIZE no allowed (we need to append the terminating zero)
        Serial.printf("tele: ERROR body line '%s' data width mismatch (%d)\n",_data,len);
        return false;
      }
      
      memcpy( tele_fields[i].value, pos_open_delim+1, len );
      tele_fields[i].value[len] = '\0';

      //Serial.printf("tele: %s %s\n",tele_fields[i].name, tele_fields[i].value);
    }
  }
  return true;
}


// Returns true iff the `_data[0.._len)` is a valid crc line, the CRC matches that of all collected bytes, and all field are found.
// Prints and error if not.
bool Tele_Parser::csumln_ok() {
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

  // Check if all objects have values
  for( int i=0; i<TELE_NUMFIELDS; i++ ) {
    if( tele_fields[i].value[0] == '\0' ) {
      Serial.printf("tele: missing field '%s' with code '%s'\n",tele_fields[i].name,tele_fields[i].obis);
      return false;
    }
  }

  //_data[_len-2]='\0';
  //Serial.printf("tele: csum line received '%s'\n",_data);
  
  return true;  
}


// Feed the next character from the emeter into the parser. Feed -1 if no char received (this checks timeouts).
// The parse will keep state of where it is.
// It returns TELE_RESULT_AVAILABLE when a complete telegram is received, its CRC matches, and all fields are found.
// It returns TELE_RESULT_ERROR when a partial telegram is received but there was an error (timeout, syntax error, crc error, field missing).
// It returns TELE_RESULT_COLLECTING when telegram data is being collected, no errors have been found, but it is also not complete yet.
Tele_Result Tele_Parser::add(int ch) {
  Tele_Result res = TELE_RESULT_COLLECTING;
  // if( ch=='\n') Serial.printf("\\n\n[%d,%d]",_state,_len+1); else if( ch=='\r') Serial.printf("\\r",ch); else if( ch==-1) {} else if( ch>'\x20' && ch<'\x7f') Serial.printf("%c",ch); else Serial.printf("\\x%x",(uint8_t)ch);

  // Too long no data?
  if( ch<0 ) {
    if( millis() - _time > TELE_MAXWAIT_MS ) {
      if( _len>0 ) Serial.printf("tele: ... timeout (%d bytes discarded)\n",_len); else Serial.printf("tele: ERROR timeout\n");
      set_state_idle();
      res = TELE_RESULT_ERROR;
    }
    return res;
  }

  // Waiting for header
  if( _state==TELE_STATE_IDLE ) {
    if( ch=='/' ) {
      // Start of header. Init data collection, reset time to start of telegram
      if( _len>0 ) { Serial.printf("tele: ... found header (%d bytes discarded)\n",_len); res=TELE_RESULT_ERROR; }
      set_state_head();
      _data[_len++]= ch;
    } else {
      // bytes come in without header
      if( _len==0 ) Serial.printf("tele: ERROR data without header ...\n");
      _len++;
    }
    return res;
  }

  // Does character fit in line buffer?
  if( _len==TELE_LINE_SIZE ) {
    Serial.printf("tele: ERROR line too long (%d)\n",_len);
    res= TELE_RESULT_ERROR; 
    set_state_idle();
    return res;
  }
  _data[_len++]= ch;

  // Switch state if a special char comes in
  switch( _state ) {
  case TELE_STATE_HEAD:
    // Get e.g. "/KFM5KAIFA-METER<CR><LF><CR><LF>"
    if( _len>3 && _data[_len-3]=='\n' && _data[_len-1]=='\n' ) { // include whiteline
      // Header is complete (including whiteline)
      if( header_ok() ) {
        set_state_body();
      } else {
        res= TELE_RESULT_ERROR; 
        set_state_idle();
      }
    }  
    break;
    
  case TELE_STATE_BODY:
    // Get e.g. "1-0:1.8.1(012345.678*kWh)<CR><LF>"
    if( ch=='!' ) {
      set_state_csum();
    } else if( ch=='\n' ) {
      // Body line is complete
      if( bodyln_ok() ) {
        _len= 0;
      } else {
       res= TELE_RESULT_ERROR; 
       set_state_idle();
      }
    }
    break;
    
  case TELE_STATE_CSUM:
    // get e.g. "!70CE<CR><LF>"
    if( ch=='\n' ) {
      // CRC is complete
      if( csumln_ok() ) res=TELE_RESULT_AVAILABLE; else res=TELE_RESULT_ERROR; 
      set_state_idle();
    }
    break;
  
  } // switch
  return res;
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


Tele_Parser parser;
int app_fail;

void setup() {
  Serial.begin(115200, SERIAL_8N1, SERIAL_FULL);
  do delay(250); while( !Serial );
  Serial.printf("\n\n\nWelcome to p1read\n");

  USC0(UART0) = USC0(UART0) | BIT(UCRXI);
  Serial.flush();
  Serial.printf("seri: RX inverted\n");

  Serial.printf("\n");
  app_fail = 0;
  parser.begin();
}


void loop() {
  Tele_Result res = parser.add(SERIAL_READ());
  if( res==TELE_RESULT_ERROR ) app_fail++;
  if( res==TELE_RESULT_AVAILABLE ) {
    Serial.printf("tele: available\n  %-15s %d\n","Num-Fail",app_fail );
    for( int i=0; i<TELE_NUMFIELDS; i++ ) {
      Serial.printf("  %-15s %s\n",tele_fields[i].name, tele_fields[i].value);
    }
    app_fail = 0;
  }
}
