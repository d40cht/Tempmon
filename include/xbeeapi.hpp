#pragma once

#ifdef LINUX
/*typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long long uint64_t;*/
#endif

class Packet;

#ifdef LINUX
#   define ASSERT(a) assert(a)
#else
#   define ASSERT(a)
#endif

template<typename T>
class XBeeComms
{
public:
    XBeeComms( T& serial ) : m_serial( serial )
    {
    }
    
    void write( const Packet& p );
    Packet readPacket();

private:
    T&  m_serial;
};

class ByteBuf
{
    static const size_t capacity=48;
public:
    ByteBuf() : m_count(0)
    {
    }
    
    void push_back( uint8_t val )
    {
        ASSERT( m_count < capacity );
        m_buf[m_count++] = val;
    }
    
    size_t size() const { return m_count; }
    
    uint8_t operator[]( size_t index ) const
    {
        ASSERT( index < m_count );
        return m_buf[index];
    }
    
    uint8_t front() const
    {
        return this->operator[](0);
    }
    
    void pop_front()
    {
        // Inefficient. But who cares.
        for ( size_t i = 1; i < m_count; ++i )
        {
            m_buf[i-1] = m_buf[i];
        }
        m_count--;
    }
    
private:
    uint8_t m_buf[capacity];
    size_t  m_count;
};

class Packet
{
public:
    void push_back( uint8_t v )
    {
        m_message.push_back( v );
    }
    
#ifdef LINUX
    void dump()
    {
        for ( size_t i = 0; i < m_message.size(); ++i )
        {
            std::cout << std::hex;
            std::cout << i << " : " << m_message[i] << ", 0x" << (int) m_message[i] << std::endl;
        }
        std::cout << std::endl;
    }
#endif
    
public:
    ByteBuf m_message;
};

template<typename T>
void XBeeComms<T>::write( const Packet& p )
{
    uint16_t length = p.m_message.size();
    m_serial.writeByte( 0x7e );
    m_serial.writeByte( (length>>8) & 255 );
    m_serial.writeByte( length & 255 );
    uint8_t cs = 0;
    
    for ( size_t i = 0; i < p.m_message.size(); ++i )
    {
        uint8_t el = p.m_message[i];
        cs += el;
        m_serial.writeByte( el );
    }
    m_serial.writeByte( 0xff - cs );   
}

template<typename T>
Packet XBeeComms<T>::readPacket()
{
    ByteBuf input;
    while (true)
    {
        input.push_back( m_serial.readByte() );
        size_t len = input.size();
        
        if ( len > 0 )
        {
            if ( input.front() != 0x7e )
            {
                input.pop_front();
            }
            else if ( input.size() > 3 )
            {
                size_t packetLength = static_cast<uint16_t>(input[1]<<8) | input[2];
                
                // The packet is long enough, let's validate it.
                if ( input.size() >= 4 + packetLength )
                {
                    uint8_t acc = 0;
                    for ( size_t i = 0; i < packetLength+1; ++i )
                    {
                        acc += input[i+3];
                    }
                    if ( acc == 0xff )
                    {
                        Packet resp;
                        
                        for ( size_t i = 3; i < input.size()-1; ++i )
                        {
                            resp.push_back( input[i] );
                        }
                        
                        return resp;
                    }
                }
            }
        }
    }
}

