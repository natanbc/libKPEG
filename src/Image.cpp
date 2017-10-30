#include <string>
#include <cmath>

#include "Image.hpp"
#include "Logger.hpp"

namespace kpeg
{
    Image::Image() :
     m_filename{""} ,
     m_pixelPtr{nullptr} ,
     m_JPEGversion{""} ,
     m_comment{""}
    {
        LOG(Logger::Level::INFO) << "Created new Image object" << std::endl;
    }
    
    void Image::createImageFromMCUs( const std::vector<MCU>& MCUVector )
    {
        LOG(Logger::Level::INFO) << "Creating Image from MCU vector..." << std::endl;
        
        int mcuNum = 0;
        
        int jpegWidth = m_width % 8 == 0 ? m_width : m_width + 8 - ( m_width % 8 );
        int jpegHeight = m_height % 8 == 0 ? m_height : m_height + 8 - ( m_height % 8 );
        
//         std::cout << "MCU VEc: " << MCUVector.size() << std::endl;
//         
//         for ( int i = 0; i < MCUVector.size(); ++i )
//         {
//             auto pb = MCUVector[i].getAllMatrices();
//             std::cout << "i: " << i + 1 << std::endl;
//             
//             for ( auto&& m : pb )
//             {
//                 for ( auto&& row : m )
//                 {
//                     for ( auto&& v : row )
//                         std::cout << v << " "; 
//                     std::cout << std::endl;
//                 }
//                 std::cout << std::endl;
//             }
//         }
        
        // Create a pixel pointer of size (Image width) x (Image height)
        m_pixelPtr = std::make_shared<std::vector<std::vector<Pixel>>>( jpegHeight, std::vector<Pixel>( jpegWidth, Pixel() ) );
        
        for ( int y = 0; y <= jpegHeight - 8; y += 8 )
        {
            for ( int x = 0; x <= jpegWidth - 8; x += 8 )
            {
                auto pixelBlock = MCUVector[mcuNum].getAllMatrices();
                //std::cout << "MCU#: " << mcuNum << std::endl;
                
                for ( int v = 0; v < 8; ++v )
                {
                    for ( int u = 0; u < 8; ++u )
                    {
                        (*m_pixelPtr)[y + v][x + u].comp[0] = pixelBlock[0][v][u]; // R
                        (*m_pixelPtr)[y + v][x + u].comp[1] = pixelBlock[1][v][u]; // G
                        (*m_pixelPtr)[y + v][x + u].comp[2] = pixelBlock[2][v][u]; // B
                    }
                }
            
                mcuNum++;
            }
        }
        
        if ( m_width != jpegWidth )
        {
            for ( auto&& row : *m_pixelPtr )
                for ( int c = 0; c < 8 - m_width % 8; ++c )
                    row.pop_back();
        }
        
        if ( m_height != jpegHeight )
        {
            for ( int c = 0; c < 8 - m_height % 8; ++c )
                m_pixelPtr->pop_back();
        }        

        LOG(Logger::Level::INFO) << "Finished created Image from MCU [OK]" << std::endl;
    }
    
    PixelPtr Image::getPixelPtr()
    {
        return m_pixelPtr;
    }

    const unsigned int Image::getWidth() const
    {
        return m_width;
    }

    const unsigned int Image::getHeight() const
    {
        return m_height;
    }

    const bool Image::dumpRawData( const std::string& filename )
    {
        if ( m_pixelPtr == nullptr )
        {
            LOG(Logger::Level::ERROR) << "Unable to create dump file \'" + filename + "\', Invalid pixel pointer" << std::endl;
            return false;
        }
        
        std::ofstream dumpFile( filename, std::ios::out );
        
        if ( !dumpFile.is_open() || !dumpFile.good() )
        {
            LOG(Logger::Level::ERROR) << "Unable to create dump file \'" + filename + "\'." << std::endl;
            return false;
        }
        
        dumpFile << "P6" << std::endl;
        dumpFile << "# PPM dump created using libKPEG: https://github.com/TheIllusionistMirage/libKPEG" << std::endl;
        dumpFile << m_width << " " << m_height << std::endl;
        dumpFile << 255 << std::endl;
        
        for ( auto&& row : *m_pixelPtr )
        {
            for ( auto&& pixel : row )
                dumpFile << (UInt8)pixel.comp[RGBComponents::RED]
                         << (UInt8)pixel.comp[RGBComponents::GREEN]
                         << (UInt8)pixel.comp[RGBComponents::BLUE];
        }
        
        LOG(Logger::Level::INFO) << "Raw image data dumped to file: \'" + filename + "\'." << std::endl;
        dumpFile.close();
        return true;
    }
    
    void Image::setImageFilename(const std::string& filename)
    {
        m_filename = filename;
    }

    void Image::setJPEGVersion(const std::string& version)
    {
        m_JPEGversion = version;
    }

    void Image::setComment(const std::string& comment)
    {
        m_comment = comment;
    }
    
    void Image::setDimensions( const std::size_t width, const std::size_t height )
    {
        m_width = width;
        m_height = height;
    }
    
    const std::string valueToBitString( const Int16 value )
    {
        if ( value == 0x0000 )
            return "";
        
        Int16 val = value;
        int bits = ( std::log2( std::abs( value ) ) ) + 1;
        //std::cout << "Bits: " << bits << ", ";
        std::string bitStr( bits, '0' );
        
        if ( val < 0 )
        {
            UInt16 delta = 0xFFFF >> (16 - bits);
            val += delta;
            val = std::abs( val );
        }
        
        int i = bitStr.size() - 1;
        while ( val > 0 )
        {
            bitStr[i--] = '0' + val % 2;
            val /= 2;
        }
        
        return bitStr;
    }
    
    const Int16 bitStringtoValue( const std::string& bitStr )
    {
        if ( bitStr == "" )
            return 0x0000;
        
        Int16 value = 0x0000;
        
        char sign = bitStr[0];
        int factor = sign == '0' ? -1 : 1;
            
        for ( auto i = 0; i < bitStr.size(); ++i )
        {
            if ( bitStr[i] == sign )
                value += Int16( std::pow( 2, bitStr.size() - 1 - i ) );
        }
        
        return factor * value;
    }
}
