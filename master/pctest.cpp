#include <iostream>

#include <boost/date_time/posix_time/posix_time.hpp>

#include <assert.h>
#include <fcntl.h>
#include <termios.h>

#include "xbeeapi.hpp"

// Build with: g++ master/pctest.cpp -DLINUX -I ./include -o pctest && ./pctest

class PCSerialPort
{
public:
    PCSerialPort( int fd ) : m_fd( fd )
    {
    }
    
    void writeByte( uint8_t v )
    {
        assert( ::write( m_fd, &v, 1 ) == 1 );
    }
    
    uint8_t readByte()
    {
        char r;
        while( read( m_fd, &r, 1 ) != 1 );
        return r;
    }
    
    
private:
    int m_fd;
};



namespace
{
    const uint8_t cmdModemStatus    = 0x8a;
    const uint8_t cmdAT             = 0x08;
    const uint8_t cmdTX             = 0x10;
    const uint8_t txStatus          = 0x8b;
    const uint8_t rxPacket          = 0x90;
}

void test( const char* portName )
{
    //int fd = open( portName, O_RDWR | O_NOCTTY | O_NDELAY );
    int fd = open( portName, O_RDWR | O_NOCTTY );
    assert( fd != -1 );
    
    termios options;
    
    tcgetattr(fd, &options);
    // 19200 8 N 1
    cfsetispeed(&options, B19200);
    cfsetospeed(&options, B19200);
    
    options.c_cflag |= HUPCL;

    assert( tcsetattr(fd, TCSANOW, &options) == 0 );
 
    PCSerialPort s( fd );
    XBeeComms<PCSerialPort> xbeeComms( s );
    
    while (true)
    {   
        Packet resp( xbeeComms.readPacket() );
        
        try
        {
            if ( resp.m_message[0] == rxPacket )
            {
                uint64_t fullAddress = 0;
                for ( int i = 1; i < 9; ++i )
                {
                    fullAddress = (fullAddress << 8LL) | static_cast<uint64_t>(resp.m_message[i]);
                }

                // Packet acknowledged
                assert( resp.m_message[11] == 0x01 );
                
                uint8_t tempMSB = resp.m_message[12];
                uint8_t tempLSB = resp.m_message[13];
                uint8_t tempCheck = resp.m_message[14];
                assert( tempMSB ^ tempLSB == tempCheck );
                
                uint16_t TReading = (static_cast<uint16_t>(tempMSB)<<8) | static_cast<uint16_t>(tempLSB);
                uint16_t TReadingRaw = TReading;
                int SignBit = TReading & 0x8000;  // test most sig bit
                if (SignBit) // negative
                {
                    TReading = (TReading ^ 0xffff) + 1; // 2's comp
                }

                double Tc_100 = TReading * 0.0625;
                
                uint16_t voltageRaw = static_cast<uint16_t>( resp.m_message[15] )<<8 | resp.m_message[16];
                uint8_t voltageChecksum = resp.m_message[17];
                double voltage = static_cast<double>(voltageRaw) * (1.2/1023.0);
                
                int movement = resp.m_message[18];
                
                std::cout << boost::posix_time::second_clock::local_time() << ", " << std::hex << " 0x" << fullAddress << ", " << Tc_100 << ", " << voltage << ", " << std::dec << movement << std::endl;
            }
            else
            {
                std::cerr << "Unhandled message: 0x" << std::hex << (int) resp.m_message[0] << std::endl;
            }
        }
        catch ( std::exception& e )
        {
            std::cerr << "  Exception caught: " << e.what() << std::endl;
        }
        //resp.dump();
    }
}

int main( int argc, char *argv[] )
{
    std::cout << "Starting monitoring" << std::endl << std::endl;
    test( "/dev/ttyUSB0" );
}
