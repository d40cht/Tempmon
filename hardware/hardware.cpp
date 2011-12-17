#include "WProgram.h"
#include "../include/xbeeapi.hpp"


#if 1

class ArduinoSerialPort
{
public:
    void writeByte( uint8_t v )
    {
        Serial.write( v );
    }
    
    uint8_t readByte()
    {
        while ( !Serial.available() ) delay(10);
        return Serial.read();
    }
};

namespace
{
    int ledPin =  13;    // LED connected to digital pin 13
    
    const uint8_t cmdModemStatus    = 0x8a;
    const uint8_t cmdAT             = 0x08;
    const uint8_t cmdTX             = 0x10;
    const uint8_t txStatus          = 0x8b;
    const uint8_t rxPacket          = 0x90;
    
    const uint8_t STATUS_OK = 0;
    const uint8_t STATUS_ERROR = 1;
    const uint8_t STATUS_INVALID_COMMAND = 2;
    const uint8_t STATUS_INVALID_PARAMETER = 3;
}

void signal( uint8_t value )
{
    for ( size_t i = 0; i < 8; ++i )
    {
        if ( value & 1 )
        {
            digitalWrite(ledPin, HIGH);
            delay(300);
            digitalWrite(ledPin, LOW);
            delay(200);            
        }
        else
        {
            digitalWrite(ledPin, HIGH);
            delay(100);
            digitalWrite(ledPin, LOW);
            delay(400);
        }
        value >>= 1;
    }
}

void assert( uint8_t errCode, bool predicate )
{
    if ( !predicate ) signal(errCode);
    while(true);
}

void run()                     
{
    // initialize the digital pin as an output:
    Serial.begin(19200);
    pinMode(ledPin, OUTPUT);
    
    ArduinoSerialPort s;
    XBeeComms<ArduinoSerialPort> xbee( s );
    
    assert( 0x4f, false );
    
    uint8_t frameId = 1;
    {
        Packet p;
        
        p.push_back( cmdAT );
        p.push_back( frameId );
        p.push_back( 'I' );
        p.push_back( 'D' );
        p.push_back( 0x27 );
        p.push_back( 0x27 );
        xbee.write(p);
        
        Packet resp( xbee.readPacket() );
        assert( 0x1, resp.m_message[0] == 0x88 );
        assert( 0x2, resp.m_message[1] == frameId );
        assert( 0x3, resp.m_message[4] == STATUS_OK );
        
        frameId++;
    }
    
    while(true)
    {
        

        digitalWrite(ledPin, HIGH);   // set the LED on
        delay(100);                  // wait for a second
        digitalWrite(ledPin, LOW);    // set the LED off
        delay(100);                  // wait for a second
    }
}

#else

#define TEMP_PIN  12 //See Note 1, sheepdogguides..ar3ne1tt.htm

void OneWireReset(int Pin);//See Note 2
void OneWireOutByte(int Pin, byte d);
byte OneWireInByte(int Pin);

void setup() {
    digitalWrite(TEMP_PIN, LOW);
    pinMode(TEMP_PIN, INPUT);      // sets the digital pin as input (logic 1)
    Serial.begin(19200);

    delay(100);
    Serial.print("temperature measurement:\n");
}

void loop(){

  {
      int sensorValue = digitalRead(11);
      //digitalWrite(13, sensorValue);
      Serial.println( sensorValue );
      delay(1000);
  }

  {
    int HighByte, LowByte, TReading, SignBit, Tc_100, Whole, Fract;
  
    OneWireReset(TEMP_PIN);
    OneWireOutByte(TEMP_PIN, 0xcc);
    OneWireOutByte(TEMP_PIN, 0x44); // perform temperature conversion, strong pullup for one sec
  
    OneWireReset(TEMP_PIN);
    OneWireOutByte(TEMP_PIN, 0xcc);
    OneWireOutByte(TEMP_PIN, 0xbe);
  
    LowByte = OneWireInByte(TEMP_PIN);
    HighByte = OneWireInByte(TEMP_PIN);
    TReading = (HighByte << 8) + LowByte;
    SignBit = TReading & 0x8000;  // test most sig bit
    if (SignBit) // negative
    {
      TReading = (TReading ^ 0xffff) + 1; // 2's comp
    }
    //Tc_100 = (6 * TReading) + TReading / 4;    // multiply by (100 * 0.0625) or 6.25
    Tc_100 = TReading*50;
  
    Whole = Tc_100 / 100;  // separate off the whole and fractional portions
    Fract = Tc_100 % 100;
  
    if (SignBit) // If its negative
    {
       Serial.print("-");
    }
    Serial.print(Whole);
    Serial.print(".");
    if (Fract < 10)
    {
       Serial.print("0");
    }
  
    Serial.print(Fract);
  
        Serial.print("\n");
    delay(1000);      // 5 second delay.  Adjust as necessary
  }
}

void OneWireReset(int Pin) // reset.  Should improve to act as a presence pulse
{
     digitalWrite(Pin, LOW);
     pinMode(Pin, OUTPUT); // bring low for 500 us
     delayMicroseconds(500);
     pinMode(Pin, INPUT);
     delayMicroseconds(500);
}

void OneWireOutByte(int Pin, byte d) // output byte d (least sig bit first).
{
   byte n;

   for(n=8; n!=0; n--)
   {
      if ((d & 0x01) == 1)  // test least sig bit
      {
         digitalWrite(Pin, LOW);
         pinMode(Pin, OUTPUT);
         delayMicroseconds(5);
         pinMode(Pin, INPUT);
         delayMicroseconds(60);
      }
      else
      {
         digitalWrite(Pin, LOW);
         pinMode(Pin, OUTPUT);
         delayMicroseconds(60);
         pinMode(Pin, INPUT);
      }

      d=d>>1; // now the next bit is in the least sig bit position.
   }

}

byte OneWireInByte(int Pin) // read byte, least sig byte first
{
    byte d, n, b;

    for (n=0; n<8; n++)
    {
        digitalWrite(Pin, LOW);
        pinMode(Pin, OUTPUT);
        delayMicroseconds(5);
        pinMode(Pin, INPUT);
        delayMicroseconds(5);
        b = digitalRead(Pin);
        delayMicroseconds(50);
        d = (d >> 1) | (b<<7); // shift d to right and insert b in most sig bit position
    }
    return(d);
}

void run()
{
    setup();

    for ( int i = 0; i < 16; ++i )
    {
        digitalWrite( 13, 1 );
        delay(50);
        digitalWrite(13, 0);
        delay(50);
    }
    while ( true )
    {
        loop();
    }
}

#endif

int main(void)
{
    init();
    run();
    
    return 0;
}

void * operator new(size_t size)
{
  return malloc(size);
}

void operator delete(void * ptr)
{
  free(ptr);
} 

extern "C" void   atexit(void) {} 
extern "C" void __cxa_pure_virtual(void) {}

