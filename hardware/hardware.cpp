#include "WProgram.h"

#include "../include/xbeeapi.hpp"
#include "onewiretemperature.hpp"

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
    int ledPin = 13;    // LED connected to digital pin 13
    int temperaturePin = 12;
    
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
            delay(450);
            digitalWrite(ledPin, LOW);
            delay(0);            
        }
        else
        {
            digitalWrite(ledPin, HIGH);
            delay(50);
            digitalWrite(ledPin, LOW);
            delay(450);
        }
        delay(300);
        value >>= 1;
    }
    delay(500);
}

void assert( uint8_t errCode, bool predicate )
{
    if ( !predicate )
    {
        signal(errCode);
        while(true);
    }
}

void run()                     
{
    // initialize the digital pin as an output:
    Serial.begin(19200);
    pinMode(ledPin, OUTPUT);
    
    ArduinoSerialPort s;
    XBeeComms<ArduinoSerialPort> xbee( s );
    
    uint8_t frameId = 1;
    
    // Set Zigbee PAN
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
    
    // Check association status
    {
        Packet p;
        
        p.push_back( cmdAT );
        p.push_back( frameId );
        p.push_back( 'A' );
        p.push_back( 'I' );
        xbee.write(p);
        
        Packet resp( xbee.readPacket() );
        assert( 0x4, resp.m_message[0] == 0x88 );
        assert( 0x5, resp.m_message[1] == frameId );
        assert( 0x6, resp.m_message[4] == STATUS_OK );
        
        // Association status. 0x0: coord started or endpoint joined PAN.
        assert( 0x7, resp.m_message[5] == 0x0 );
        
        frameId++;
    }
    
    OneWireTemperature tempSensor( temperaturePin );
    bool light = true;
    while(true)
    {
        if ( light ) digitalWrite(ledPin, HIGH);
        else digitalWrite(ledPin, LOW);
        
        
        // Read the temperature sensor
        uint16_t temp = tempSensor.readTemp();
        
        // Interrogate the XBee module for supply voltage information
        uint16_t voltage = 0;
        {
            Packet p;
            p.push_back( cmdAT );
            p.push_back( frameId );
            p.push_back( '%' );
            p.push_back( 'V' );
            xbee.write(p);
            
            Packet resp( xbee.readPacket() );
            assert( 0x8, resp.m_message[0] == 0x88 );
            assert( 0x9, resp.m_message[1] == frameId );
            assert( 0xa, resp.m_message[4] == STATUS_OK );
            
            voltage = static_cast<uint16_t>( resp.m_message[5] )<<8 | resp.m_message[6];
        }
        
        light = !light;
        
        // Send a packet to the coordinator (64 bit address of 0)
        {
            Packet p;
            p.push_back( cmdTX );
            
            p.push_back( frameId );
            
            // 64-bit dest address
            for ( size_t i = 0; i < 8; ++i ) p.push_back(0);
            
            // 16-bit address (?)
            p.push_back(0);
            p.push_back(0);
            
            // Max hops is max avail
            p.push_back(0);
            
            // Unicast packet
            p.push_back(0);
            
            p.push_back( temp>>8 );
            p.push_back( temp&255 );
            
            xbee.write(p);
            
            Packet resp( xbee.readPacket() );
            
            // Check for TX success
            assert( 0xb, resp.m_message[5] == 0x0 );
        
            frameId++;
        }
        
        delay(10000);
    }
}

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

