#include "onewiretemperature.hpp"


uint16_t OneWireTemperature::readTemp()
{
    reset(m_pin);
    writeByte(m_pin, 0xcc); // skip rom (broadcast to all devices)
    writeByte(m_pin, 0x44); // perform temperature conversion, strong pullup for one sec
    
    delay(100);
  
    reset(m_pin);
    writeByte(m_pin, 0xcc);
    writeByte(m_pin, 0xbe); // read scratchpad (first two registers are the temperature)
  
    uint8_t LowByte = readByte(m_pin);
    uint8_t HighByte = readByte(m_pin);
    uint16_t TReading = (static_cast<uint16_t>(HighByte) << 8) | static_cast<uint16_t>(LowByte);
    
    return TReading;
}

void OneWireTemperature::reset(int Pin) // reset.  Should improve to act as a presence pulse
{
     digitalWrite(Pin, LOW);
     pinMode(Pin, OUTPUT); // bring low for 500 us
     delayMicroseconds(500);
     pinMode(Pin, INPUT);
     delayMicroseconds(500);
}

void OneWireTemperature::writeByte(int Pin, byte d) // output byte d (least sig bit first).
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

byte OneWireTemperature::readByte(int Pin) // read byte, least sig byte first
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
    return d;
}

