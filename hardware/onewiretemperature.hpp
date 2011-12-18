#include "WProgram.h"

class OneWireTemperature
{
public:
    OneWireTemperature( uint8_t pin ) : m_pin(pin)
    {
        digitalWrite(m_pin, LOW);
        pinMode(m_pin, INPUT);      // sets the digital pin as input (logic 1)
    }
    
    uint16_t readTemp();
    
private:
    void reset(int Pin);//See Note 2
    void writeByte(int Pin, byte d);
    byte readByte(int Pin);

private:
    uint8_t m_pin;
};

