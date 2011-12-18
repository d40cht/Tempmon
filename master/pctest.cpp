#include <iostream>

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
    
    uint8_t frameId = 0x1;
    
    const uint8_t STATUS_OK = 0;
    const uint8_t STATUS_ERROR = 1;
    const uint8_t STATUS_INVALID_COMMAND = 2;
    const uint8_t STATUS_INVALID_PARAMETER = 3;
    
    {
        std::cout << "Check firmware version" << std::endl;
        Packet p;    
        
        p.push_back( cmdAT );
        p.push_back( frameId );
        p.push_back( 'V' );
        p.push_back( 'R' );

        xbeeComms.write( p );
        
        Packet pin( xbeeComms.readPacket() );
        assert( pin.m_message[0] == 0x88 );
        assert( pin.m_message[1] == frameId );
        assert( pin.m_message[4] == STATUS_OK );
        
        uint16_t versionNumber = static_cast<uint16_t>(pin.m_message[5])<<8 | pin.m_message[6];
        
        assert( versionNumber == 0x1347 ); // End point
        //assert( versionNumber == 0x1147 ); // Coordinator
        
        frameId++;
    }
    
    {
        std::cout << "Set PAN" << std::endl;
        Packet p;
        
        p.push_back( cmdAT );
        p.push_back( frameId );
        p.push_back( 'I' );
        p.push_back( 'D' );
        p.push_back( 0x27 );
        p.push_back( 0x27 );
        xbeeComms.write(p);
        
        Packet resp( xbeeComms.readPacket() );
        assert( resp.m_message[0] == 0x88 );
        assert( resp.m_message[1] == frameId );
        assert( resp.m_message[4] == STATUS_OK );
        
        frameId++;
    }
    
    // Set the coordinator to open (Node join time of infinity)
    /*{
        Packet p;
        
        p.push_back( cmdAT );
        p.push_back( frameId );
        p.push_back( 'N' );
        p.push_back( 'J' );
        p.push_back( 0xff );
        s.write(p);
        
        Packet resp( s.readPacket() );
        assert( resp.m_message[0] = 0x88 );
        assert( resp.m_message[1] == frameId );
        assert( resp.m_message[4] == STATUS_OK );
        
        frameId++;
    }*/
    
    {
        std::cout << "Check association status" << std::endl;
        Packet p;
        
        p.push_back( cmdAT );
        p.push_back( frameId );
        p.push_back( 'A' );
        p.push_back( 'I' );
        xbeeComms.write(p);
        
        Packet resp( xbeeComms.readPacket() );
        assert( resp.m_message[0] == 0x88 );
        assert( resp.m_message[1] == frameId );
        assert( resp.m_message[4] == STATUS_OK );
        
        // Association status. 0x0: coord started or endpoint joined PAN.
        assert( resp.m_message[5] == 0x0 );
        //resp.dump();
        
        frameId++;
    }
    
    {
        std::cout << "Check operating PAN" << std::endl;
        Packet p;
        
        p.push_back( cmdAT );
        p.push_back( frameId );
        p.push_back( 'O' );
        p.push_back( 'P' );
        xbeeComms.write(p);
        
        Packet resp( xbeeComms.readPacket() );
        assert( resp.m_message[0] == 0x88 );
        assert( resp.m_message[1] == frameId );
        assert( resp.m_message[4] == STATUS_OK );
        
        resp.dump();
        
        frameId++;
    }
    
    // Sleep mode - set to cyclic. Then will sleep for e.g. 20 seconds before checking for serial data.
    // If none, will sleep again. Will remain awake idle for ST time, then sleep for SP.
    // To set up: ST 0x0010 (idle for 16ms before sleep), SP 0x03e8 (10 seconds sleep peroid) SM4 (cyclic sleep enabled)/
    
    // Then get the AVR to poll the serial with AT until it gets a response
    
    {
        std::cout << "Send a packet to the coordinator (64 bit address of 0)" << std::endl;
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
        
        // The data
        p.push_back( 0x1 );
        p.push_back( 0x2 );
        p.push_back( 0x3 );
        p.push_back( 0x4 );
        p.push_back( 0x5 );
        
        xbeeComms.write(p);
        
        Packet resp( xbeeComms.readPacket() );
        resp.dump();

        // Check for TX success
        assert( resp.m_message[5] == 0x0 );
        
        frameId++;
    }
}

int main( int argc, char *argv[] )
{
    std::cout << "Hello" << std::endl;
    
    test( "/dev/ttyUSB0" );
    
    std::cout << "Goodbye" << std::endl;
}
