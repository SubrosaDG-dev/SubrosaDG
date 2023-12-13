//          __        ____ ____
// ___  ___/  |_ __ _/_   /_   |
// \  \/ /\   __\  |  \   ||   |
//  \   /  |  | |  |  /   ||   |
//   \_/   |__| |____/|___||___|
//
//  License: BSD License ; see LICENSE
//

// AUTOMATICALLY GENERATED SINGLE HEADER FILE.

//          __        ____ ____
// ___  ___/  |_ __ _/_   /_   |
// \  \/ /\   __\  |  \   ||   |
//  \   /  |  | |  |  /   ||   |
//   \_/   |__| |____/|___||___|
//
//  License: BSD License ; see LICENSE
//

#ifndef VTU11_ALIAS_HPP
#define VTU11_ALIAS_HPP

#include <string>
#include <map>
#include <utility>
#include <vector>

namespace vtu11
{

using StringStringMap = std::map<std::string, std::string>;

enum class DataSetType : int
{
    PointData = 0, CellData = 1
};

using DataSetInfo = std::tuple<std::string, DataSetType, size_t>;
using DataSetData = std::vector<double>;

using VtkCellType = std::int8_t;
using VtkIndexType = std::int64_t;

using HeaderType = size_t;
using Byte = unsigned char;

} // namespace vtu11

#ifndef VTU11_ASCII_FLOATING_POINT_FORMAT
    #define VTU11_ASCII_FLOATING_POINT_FORMAT "%.6g"
#endif

// To dynamically select std::filesystem where available, you could use:
#if defined(__cplusplus) && __cplusplus >= 201703L
    #if __has_include(<filesystem>) // has_include is C++17
        #include <filesystem>
        namespace vtu11fs = std::filesystem;
    #elif __has_include(<experimental/filesystem>)
        #include <experimental/filesystem>
        namespace vtu11fs = std::experimental::filesystem;
    #else
        #include "inc/filesystem.hpp"
        namespace vtu11fs = ghc::filesystem;
    #endif
#else
    namespace vtu11fs = ghc::filesystem;
#endif

#endif // VTU11_ALIAS_HPP
//          __        ____ ____
// ___  ___/  |_ __ _/_   /_   |
// \  \/ /\   __\  |  \   ||   |
//  \   /  |  | |  |  /   ||   |
//   \_/   |__| |____/|___||___|
//
//  License: BSD License ; see LICENSE
//

#ifndef VTU11_WRITER_HPP
#define VTU11_WRITER_HPP


namespace vtu11
{

struct AsciiWriter
{
  template<typename T>
  void writeData( std::ostream& output,
                  const std::vector<T>& data );

  void writeAppended( std::ostream& output );

  void addHeaderAttributes( StringStringMap& attributes );
  void addDataAttributes( StringStringMap& attributes );

  StringStringMap appendedAttributes( );
};

struct Base64BinaryWriter
{
  template<typename T>
  void writeData( std::ostream& output,
                  const std::vector<T>& data );

  void writeAppended( std::ostream& output );

  void addHeaderAttributes( StringStringMap& attributes );
  void addDataAttributes( StringStringMap& attributes );

  StringStringMap appendedAttributes( );
};

struct Base64BinaryAppendedWriter
{
  template<typename T>
  void writeData( std::ostream& output,
                  const std::vector<T>& data );

  void writeAppended( std::ostream& output );

  void addHeaderAttributes( StringStringMap& attributes );
  void addDataAttributes( StringStringMap& attributes );

  StringStringMap appendedAttributes( );

  size_t offset = 0;

  std::vector<std::pair<const char*, HeaderType>> appendedData;
};

struct RawBinaryAppendedWriter
{
  template<typename T>
  void writeData( std::ostream& output,
                  const std::vector<T>& data );

  void writeAppended( std::ostream& output );

  void addHeaderAttributes( StringStringMap& attributes );
  void addDataAttributes( StringStringMap& attributes );

  StringStringMap appendedAttributes( );

  size_t offset = 0;

  std::vector<std::pair<const char*, HeaderType>> appendedData;
};

} // namespace vtu11


#endif // VTU11_WRITER_HPP
//          __        ____ ____
// ___  ___/  |_ __ _/_   /_   |
// \  \/ /\   __\  |  \   ||   |
//  \   /  |  | |  |  /   ||   |
//   \_/   |__| |____/|___||___|
//
//  License: BSD License ; see LICENSE
//

#ifndef VTU11_UTILITIES_HPP
#define VTU11_UTILITIES_HPP


#include <functional>
#include <limits>
#include <type_traits>

namespace vtu11
{

#define VTU11_THROW( message ) throw std::runtime_error( message )
#define VTU11_CHECK( expr, message ) if( !( expr ) ) VTU11_THROW ( message )

std::string endianness( );

template<typename Iterator>
std::string base64Encode( Iterator begin, Iterator end );

size_t encodedNumberOfBytes( size_t rawNumberOfBytes );

class ScopedXmlTag final
{
public:
    ScopedXmlTag( std::ostream& output,
                  const std::string& name,
                  const StringStringMap& attributes );

    ~ScopedXmlTag( );

private:
    std::function<void( )> closeTag;
};

void writeEmptyTag( std::ostream& output,
                    const std::string& name,
                    const StringStringMap& attributes );

template<typename T> inline
std::string appendSizeInBits( const char* str )
{
    return str + std::to_string( sizeof( T ) * 8 );
}

// SFINAE if signed integer
template<typename T> inline
typename std::enable_if<std::numeric_limits<T>::is_integer &&
                        std::numeric_limits<T>::is_signed, std::string>::type
    dataTypeString( )
{
    return appendSizeInBits<T>( "Int" );
}

// SFINAE if unsigned signed integer
template<typename T> inline
typename std::enable_if<std::numeric_limits<T>::is_integer &&
                       !std::numeric_limits<T>::is_signed, std::string>::type
    dataTypeString( )
{
    return appendSizeInBits<T>( "UInt" );
}

// SFINAE if double or float
template<typename T> inline
typename std::enable_if<std::is_same<T, double>::value ||
                        std::is_same<T, float>::value, std::string>::type
    dataTypeString( )
{
    return appendSizeInBits<T>( "Float" );
}

} // namespace vtu11


#endif // VTU11_UTILITIES_HPP
//          __        ____ ____
// ___  ___/  |_ __ _/_   /_   |
// \  \/ /\   __\  |  \   ||   |
//  \   /  |  | |  |  /   ||   |
//   \_/   |__| |____/|___||___|
//
//  License: BSD License ; see LICENSE
//

#ifndef VTU11_ZLIBWRITER_HPP
#define VTU11_ZLIBWRITER_HPP


#ifdef VTU11_ENABLE_ZLIB

namespace vtu11
{

struct CompressedRawBinaryAppendedWriter
{
  template<typename T>
  void writeData( std::ostream& output,
                  const std::vector<T>& data );

  void writeAppended( std::ostream& output );

  void addHeaderAttributes( StringStringMap& attributes );
  void addDataAttributes( StringStringMap& attributes );

  StringStringMap appendedAttributes( );

  size_t offset = 0;

  std::vector<std::vector<std::vector<std::uint8_t>>> appendedData;
  std::vector<std::vector<HeaderType>> headers;
};

} // namespace vtu11


#endif // VTU11_ENABLE_ZLIB

#endif // VTU11_ZLIBWRITER_HPP
//          __        ____ ____
// ___  ___/  |_ __ _/_   /_   |
// \  \/ /\   __\  |  \   ||   |
//  \   /  |  | |  |  /   ||   |
//   \_/   |__| |____/|___||___|
//
//  License: BSD License ; see LICENSE
//

#ifndef VTU11_VTU11_HPP
#define VTU11_VTU11_HPP


namespace vtu11
{

struct Vtu11UnstructuredMesh
{
  const std::vector<double>& points_;
  const std::vector<VtkIndexType>& connectivity_;
  const std::vector<VtkIndexType>& offsets_;
  const std::vector<VtkCellType>& types_;

  const std::vector<double>& points( ){ return points_; }
  const std::vector<VtkIndexType>& connectivity( ){ return connectivity_; }
  const std::vector<VtkIndexType>& offsets( ){ return offsets_; }
  const std::vector<VtkCellType>& types( ){ return types_; }

  size_t numberOfPoints( ){ return points_.size( ) / 3; }
  size_t numberOfCells( ){ return types_.size( ); }
};

/*! Write modes (not case sensitive):
 *
 *  - Ascii
 *  - Base64Inline
 *  - Base64Appended
 *  - RawBinary
 *  - RawBinaryCompressed
 *
 *  Comments:
 *  - RawCompressedBinary needs zlib to be present. If VTU11_ENABLE_ZLIB
 *    is not defined, the uncompressed version is used instead.
 *  - Compressing data takes more time than writing more data uncompressed
 *  - Ascii produces surprisingly small files, is nice to debug, but
 *    is rather slow to read in Paraview. Archiving ascii .vtu files using
 *    a standard zip tool (for example) produces decently small file sizes.
 *  - Writing raw binary data breakes the xml standard. To still produce
 *    valid xml files you can use base64 encoding, at the cost of having
 *    about 30% times larger files.
 *  - Both raw binary modes use appended format
 */

//! Writes single file
template<typename MeshGenerator>
void writeVtu( const std::string& filename,
               MeshGenerator& mesh,
               const std::vector<DataSetInfo>& dataSetInfo,
               const std::vector<DataSetData>& dataSetData,
               const std::string& writeMode = "ascii" );

//! Creates path/baseName.pvtu and path/baseName directory
void writePVtu( const std::string& path,
                const std::string& baseName,
                const std::vector<std::string>& filenames,
                const std::vector<DataSetInfo>& dataSetInfo,
                size_t numberOfFiles );

//! Forwards path/baseName.vtu to the writeVtu function
template<typename MeshGenerator>
void writePartition( const std::string& path,
                     const std::string& baseName,
                     const std::string& filename,
                     MeshGenerator& mesh,
                     const std::vector<DataSetInfo>& dataSetInfo,
                     const std::vector<DataSetData>& dataSetData,
                     const std::string& writeMode = "ascii" );

} // namespace vtu11


#endif // VTU11_VTU11_HPP
//          __        ____ ____
// ___  ___/  |_ __ _/_   /_   |
// \  \/ /\   __\  |  \   ||   |
//  \   /  |  | |  |  /   ||   |
//   \_/   |__| |____/|___||___|
//
//  License: BSD License ; see LICENSE
//

#ifndef VTU11_UTILITIES_IMPL_HPP
#define VTU11_UTILITIES_IMPL_HPP

#include <array>

namespace vtu11
{
namespace detail
{

inline void writeTag( std::ostream& output,
                      const std::string& name,
                      const StringStringMap& attributes,
                      const std::string& tagEnd )
{
    output << "<" << name;

    for( const auto& attribute : attributes )
    {
        output << " " << attribute.first << "=\"" << attribute.second << "\"";
    }

    output << tagEnd << "\n";
}

} // namespace detail

inline ScopedXmlTag::ScopedXmlTag( std::ostream& output,
                                   const std::string& name,
                                   const StringStringMap& attributes ) :
   closeTag( [ &output, name ]( ){ output << "</" << name << ">\n"; } )
{
    detail::writeTag( output, name, attributes, ">" );
}

inline ScopedXmlTag::~ScopedXmlTag( )
{
  closeTag( );
}

inline void writeEmptyTag( std::ostream& output,
                           const std::string& name,
                           const StringStringMap& attributes )
{
  detail::writeTag( output, name, attributes, "/>" );
}

inline std::string endianness( )
{
   int i = 0x0001;

   if( *reinterpret_cast<char*>( &i ) != 0 )
   {
     return "LittleEndian";
   }
   else
   {
     return "BigEndian";
   }
}

constexpr char base64Map[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

template<typename Iterator>
inline std::string base64Encode( Iterator begin, Iterator end )
{
    constexpr size_t size = sizeof( decltype( *begin ) );

    size_t length = static_cast<size_t>( std::distance( begin, end ) );
    size_t rawBytes = length * size;
    size_t encodedBytes = ( rawBytes / 3 + 1 ) * 4;

    std::string result;

    result.reserve( encodedBytes );

    auto it = begin;
    size_t byteIndex = 0;

    auto next = [&]( )
    {
        char byte = *( reinterpret_cast<const char*>( &( *it ) ) + byteIndex++ );

        if( byteIndex == size )
        {
            it++;
            byteIndex = 0;
        }

        return byte;
    };

    auto encodeTriplet = [&]( std::array<char, 3> bytes, size_t padding )
    {
        char tmp[5] = { base64Map[(   bytes[0] & 0xfc ) >> 2],
                        base64Map[( ( bytes[0] & 0x03 ) << 4 ) + ( ( bytes[1] & 0xf0 ) >> 4 )],
                        base64Map[( ( bytes[1] & 0x0f ) << 2 ) + ( ( bytes[2] & 0xc0 ) >> 6 )],
                        base64Map[bytes[2] & 0x3f],
                        '\0' };

        std::fill( tmp + 4 - padding, tmp + 4, '=' );

        result += tmp;
    };

    // in steps of 3
    for( size_t i = 0; i < rawBytes / 3; ++i )
    {
        encodeTriplet( { next( ), next( ), next( ) }, 0 );
    }

    // cleanup
    if( it != end )
    {
        std::array<char, 3> bytes { '\0', '\0', '\0' };

        size_t remainder = static_cast<size_t>( std::distance( it, end ) ) * size - static_cast<size_t>( byteIndex );

        for( size_t i = 0; i < remainder; ++i )
        {
            bytes[i] = next( );
        }

        encodeTriplet( bytes, 3 - remainder );
    }

    return result;
}

// http://www.cplusplus.com/forum/beginner/51572/
inline size_t encodedNumberOfBytes( size_t rawNumberOfBytes )
{
    if( rawNumberOfBytes != 0 )
    {
        return ( ( rawNumberOfBytes - 1 ) / 3 + 1 ) * 4;
    }
    else
    {
        return 0;
    }
}

} // namespace vtu11

#endif // VTU11_UTILITIES_IMPL_HPP
//          __        ____ ____
// ___  ___/  |_ __ _/_   /_   |
// \  \/ /\   __\  |  \   ||   |
//  \   /  |  | |  |  /   ||   |
//   \_/   |__| |____/|___||___|
//
//  License: BSD License ; see LICENSE
//

#ifndef VTU11_WRITER_IMPL_HPP
#define VTU11_WRITER_IMPL_HPP


#include <fstream>

namespace vtu11
{
namespace detail
{

template<typename T> inline
void writeNumber( char (&)[64], T )
{
    VTU11_THROW( "Invalid data type." );
}

#define VTU11_WRITE_NUMBER_SPECIALIZATION( string, type )     \
template<> inline                                             \
void writeNumber<type>( char (&buffer)[64], type value )      \
{                                                             \
    std::snprintf( buffer, sizeof( buffer ), string, value ); \
}

VTU11_WRITE_NUMBER_SPECIALIZATION( VTU11_ASCII_FLOATING_POINT_FORMAT, double )
VTU11_WRITE_NUMBER_SPECIALIZATION( "%lld", long long int )
VTU11_WRITE_NUMBER_SPECIALIZATION( "%ld" , long int )
VTU11_WRITE_NUMBER_SPECIALIZATION( "%d"  , int )
VTU11_WRITE_NUMBER_SPECIALIZATION( "%hd" , short )
VTU11_WRITE_NUMBER_SPECIALIZATION( "%hhd", char )
VTU11_WRITE_NUMBER_SPECIALIZATION( "%llu", unsigned long long int )
VTU11_WRITE_NUMBER_SPECIALIZATION( "%ld" , unsigned long int )
VTU11_WRITE_NUMBER_SPECIALIZATION( "%d"  , unsigned int )
VTU11_WRITE_NUMBER_SPECIALIZATION( "%hd" , unsigned short )
VTU11_WRITE_NUMBER_SPECIALIZATION( "%hhd", unsigned char )

} // namespace detail

template<typename T>
inline void AsciiWriter::writeData( std::ostream& output,
                                    const std::vector<T>& data )
{
    char buffer[64];

    for( auto value : data )
    {
        detail::writeNumber( buffer, value );

        output << buffer << " ";
    }

    output << "\n";
}

template<>
inline void AsciiWriter::writeData( std::ostream& output,
                                    const std::vector<std::int8_t>& data )
{
  for( auto value : data )
  {
    // will otherwise interpret uint8 as char and output nonsense instead
  	// changed the datatype from unsigned to int
      output << static_cast<int>( value ) << " ";
  }

  output << "\n";
}

inline void AsciiWriter::writeAppended( std::ostream& )
{

}

inline void AsciiWriter::addHeaderAttributes( StringStringMap& )
{
}

inline void AsciiWriter::addDataAttributes( StringStringMap& attributes )
{
  attributes["format"] = "ascii";
}

inline StringStringMap AsciiWriter::appendedAttributes( )
{
  return { };
}

// ----------------------------------------------------------------

template<typename T>
inline void Base64BinaryWriter::writeData( std::ostream& output,
                                           const std::vector<T>& data )
{
  HeaderType numberOfBytes = data.size( ) * sizeof( T );

  output << base64Encode( &numberOfBytes, &numberOfBytes + 1 );
  output << base64Encode( data.begin( ), data.end( ) );

  output << "\n";
}

inline void Base64BinaryWriter::writeAppended( std::ostream& )
{

}

inline void Base64BinaryWriter::addHeaderAttributes( StringStringMap& attributes )
{
  attributes["header_type"] = dataTypeString<HeaderType>( );
}

inline void Base64BinaryWriter::addDataAttributes( StringStringMap& attributes )
{
  attributes["format"] = "binary";
}

inline StringStringMap Base64BinaryWriter::appendedAttributes( )
{
  return { };
}

// ----------------------------------------------------------------

template<typename T>
inline void Base64BinaryAppendedWriter::writeData( std::ostream&,
                                                   const std::vector<T>& data )
{
  HeaderType rawBytes = data.size( ) * sizeof( T );

  appendedData.emplace_back( reinterpret_cast<const char*>( &data[0] ), rawBytes );

  offset += encodedNumberOfBytes( rawBytes + sizeof( HeaderType ) );
}

inline void Base64BinaryAppendedWriter::writeAppended( std::ostream& output )
{
  for( auto dataSet : appendedData )
  {
    // looks like header and data has to be encoded at once
    std::vector<char> data( dataSet.second + sizeof( HeaderType ) );

    *reinterpret_cast<HeaderType*>( &data[0] ) = dataSet.second;

    std::copy( dataSet.first, dataSet.first + dataSet.second, &data[ sizeof( HeaderType ) ] );

    output << base64Encode( data.begin( ), data.end( ) );
  }

  output << "\n";
}

inline void Base64BinaryAppendedWriter::addHeaderAttributes( StringStringMap& attributes )
{
  attributes["header_type"] = dataTypeString<HeaderType>( );
}

inline void Base64BinaryAppendedWriter::addDataAttributes( StringStringMap& attributes )
{
  attributes["format"] = "appended";
  attributes["offset"] = std::to_string( offset );
}

inline StringStringMap Base64BinaryAppendedWriter::appendedAttributes( )
{
  return { { "encoding", "base64" } };
}

// ----------------------------------------------------------------

template<typename T>
inline void RawBinaryAppendedWriter::writeData( std::ostream&,
                                                const std::vector<T>& data )
{
  HeaderType rawBytes = data.size( ) * sizeof( T );

  appendedData.emplace_back( reinterpret_cast<const char*>( &data[0] ), rawBytes );

  offset += sizeof( HeaderType ) + rawBytes;
}

inline void RawBinaryAppendedWriter::writeAppended( std::ostream& output )
{
  for( auto dataSet : appendedData )
  {
    const char* headerBegin = reinterpret_cast<const char*>( &dataSet.second );

    for( const char* ptr = headerBegin; ptr < headerBegin + sizeof( HeaderType ); ++ptr )
    {
      output << *ptr;
    }

    for( const char* ptr = dataSet.first; ptr < dataSet.first + dataSet.second; ++ptr )
    {
      output << *ptr;
    }
  }

  output << "\n";
}

inline void RawBinaryAppendedWriter::addHeaderAttributes( StringStringMap& attributes )
{
  attributes["header_type"] = dataTypeString<HeaderType>( );
}

inline void RawBinaryAppendedWriter::addDataAttributes( StringStringMap& attributes )
{
  attributes["format"] = "appended";
  attributes["offset"] = std::to_string( offset );
}

inline StringStringMap RawBinaryAppendedWriter::appendedAttributes( )
{
  return { { "encoding", "raw" } };
}

} // namespace vtu11

#endif // VTU11_WRITER_IMPL_HPP
//          __        ____ ____
// ___  ___/  |_ __ _/_   /_   |
// \  \/ /\   __\  |  \   ||   |
//  \   /  |  | |  |  /   ||   |
//   \_/   |__| |____/|___||___|
//
//  License: BSD License ; see LICENSE
//

#ifndef VTU11_ZLIBWRITER_IMPL_HPP
#define VTU11_ZLIBWRITER_IMPL_HPP

#ifdef VTU11_ENABLE_ZLIB

#include "zlib.h"

namespace vtu11
{
namespace detail
{

template<typename T>
std::vector<HeaderType> zlibCompressData( const std::vector<T>& data,
                                          std::vector<std::vector<Byte>>& targetBlocks,
                                          size_t blockSize = 32768 ) // 2^15
{
  using IntType = uLong;

  // Somewhere in vtu11/inc/filesystem.hpp, with MSVC, max is defined as macro. This pre-
  // vents us from using std::numeric_limits<T>::max( ), so we turn off these checks.
  #ifndef max
  if( data.size( ) > std::numeric_limits<IntType>::max( ) ||
      blockSize > std::numeric_limits<IntType>::max( ) )
  {
      throw std::runtime_error( "Size too large for uLong zlib type." );
  }
  #endif

  std::vector<HeaderType> header( 3, 0 );

  if( data.empty( ) )
  {
    return header;
  }

  auto blocksize = static_cast<IntType>( blockSize );

  auto compressedBuffersize = compressBound( blocksize );

  Byte* buffer = new Byte[compressedBuffersize];
  Byte* currentByte = const_cast<Byte*>( reinterpret_cast<const Byte*>( &data[0] ) );

  IntType numberOfBytes = static_cast<IntType>( data.size( ) ) * sizeof( T );
  IntType numberOfBlocks = ( numberOfBytes - 1 ) / blocksize + 1;

  auto compressBlock = [&]( IntType numberOfBytesInBlock )
  {
      IntType compressedLength = compressedBuffersize;

    int errorCode = compress( buffer, &compressedLength, currentByte, numberOfBytesInBlock );

    if( errorCode != Z_OK )
    {
      delete[] buffer;

      throw std::runtime_error( "Error in zlib compression (code " + std::to_string( errorCode ) + ")." );
    }

    targetBlocks.emplace_back( buffer, buffer + compressedLength );
    header.push_back( compressedLength );

    currentByte += numberOfBytesInBlock;
  };

  for( IntType iBlock = 0; iBlock < numberOfBlocks - 1; ++iBlock )
  {
    compressBlock( static_cast<IntType>( blocksize ) );
  }

  IntType remainder = numberOfBytes - ( numberOfBlocks - 1 ) * blocksize;

  compressBlock( remainder );

  delete[] buffer;

  header[0] = header.size( ) - 3;
  header[1] = blocksize;
  header[2] = remainder;

  return header;
}

} // detail

template<typename T>
inline void CompressedRawBinaryAppendedWriter::writeData( std::ostream&,
                                                          const std::vector<T>& data )
{
  std::vector<std::vector<Byte>> compressedBlocks;

  auto header = detail::zlibCompressData( data, compressedBlocks );

  offset += sizeof( HeaderType ) * header.size( );

  for( const auto& compressedBlock : compressedBlocks )
  {
    offset += compressedBlock.size( );
  }

  appendedData.push_back( std::move( compressedBlocks ) );
  headers.push_back( std::move( header ) );
}

inline void CompressedRawBinaryAppendedWriter::writeAppended( std::ostream& output )
{
  for( size_t iDataSet = 0; iDataSet < appendedData.size( ); ++iDataSet )
  {
    const char* headerBegin = reinterpret_cast<const char*>( &headers[iDataSet][0] );
    size_t numberOfHeaderBytes = headers[iDataSet].size( ) * sizeof( HeaderType );

    for( const char* ptr = headerBegin; ptr < headerBegin + numberOfHeaderBytes; ++ptr )
    {
      output << *ptr;
    }

    for( const auto& compressedBlock : appendedData[iDataSet] )
    {
      for( auto ptr = compressedBlock.begin( ); ptr < compressedBlock.end( ); ++ptr )
      {
        output << *ptr;
      }
    } // for compressedBLock
  } // for iDataSet

  output << "\n";
}

inline void CompressedRawBinaryAppendedWriter::addHeaderAttributes( StringStringMap& attributes )
{
  attributes["header_type"] = dataTypeString<HeaderType>( );
  attributes["compressor"] = "vtkZLibDataCompressor";
}

inline void CompressedRawBinaryAppendedWriter::addDataAttributes( StringStringMap& attributes )
{
  attributes["format"] = "appended";
  attributes["offset"] = std::to_string( offset );
}

inline StringStringMap CompressedRawBinaryAppendedWriter::appendedAttributes( )
{
  return { { "encoding", "raw" } };
}

} // namespace vtu11

#endif // VTU11_ENABLE_ZLIB
#endif // VTU11_ZLIBWRITER_IMPL_HPP

//          __        ____ ____
// ___  ___/  |_ __ _/_   /_   |
// \  \/ /\   __\  |  \   ||   |
//  \   /  |  | |  |  /   ||   |
//   \_/   |__| |____/|___||___|
//
//  License: BSD License ; see LICENSE
//

#ifndef VTU11_VTU11_IMPL_HPP
#define VTU11_VTU11_IMPL_HPP


#include <limits>

namespace vtu11
{
namespace detail
{

template<typename DataType, typename Writer> inline
StringStringMap writeDataSetHeader( Writer&& writer,
                                    const std::string& name,
                                    size_t ncomponents )
{
    StringStringMap attributes = { { "type", dataTypeString<DataType>( ) } };

    if( name != "" )
    {
        attributes["Name"] = name;
    }

    if( ncomponents > 1 )
    {
        attributes["NumberOfComponents"] = std::to_string( ncomponents );
    }

    writer.addDataAttributes( attributes );

    return attributes;
}

template<typename Writer, typename DataType> inline
void writeDataSet( Writer& writer,
                   std::ostream& output,
                   const std::string& name,
                   size_t ncomponents,
                   const std::vector<DataType>& data )
{
    auto attributes = writeDataSetHeader<DataType>( writer, name, ncomponents );

    if( attributes["format"] != "appended" )
    {
        ScopedXmlTag dataArrayTag( output, "DataArray", attributes );

        writer.writeData( output, data );
    }
    else
    {
        writeEmptyTag( output, "DataArray", attributes );

        writer.writeData( output, data );
    }
}

template<typename Writer> inline
void writeDataSets( const std::vector<DataSetInfo>& dataSetInfo,
                    const std::vector<DataSetData>& dataSetData,
                    std::ostream& output, Writer& writer, DataSetType type )
{
    for( size_t iDataset = 0; iDataset < dataSetInfo.size( ); ++iDataset )
    {
        const auto& metadata = dataSetInfo[iDataset];

        if( std::get<1>( metadata ) == type )
        {
            detail::writeDataSet( writer, output, std::get<0>( metadata ),
                std::get<2>( metadata ), dataSetData[iDataset] );
        }
    }
}

template<typename Writer> inline
void writeDataSetPVtuHeaders( const std::vector<DataSetInfo>& dataSetInfo,
                              std::ostream& output, Writer& writer, DataSetType type )
{
    for( size_t iDataset = 0; iDataset < dataSetInfo.size( ); ++iDataset )
    {
        const auto& metadata = dataSetInfo[iDataset];

        if( std::get<1>( metadata ) == type )
        {
            auto attributes = detail::writeDataSetHeader<double>( writer,
               std::get<0>( metadata ), std::get<2>( metadata ) );

            writeEmptyTag( output, "PDataArray", attributes );
        }
    }
}

template<typename Writer, typename Content> inline
void writeVTUFile( const std::string& filename,
                   const char* type,
                   Writer&& writer,
                   Content&& writeContent )
{
    std::ofstream output( filename, std::ios::binary );

    VTU11_CHECK( output.is_open( ), "Failed to open file \"" + filename + "\"" );

    // Set buffer size to 32K
    std::vector<char> buffer( 32 * 1024 );

    output.rdbuf( )->pubsetbuf( buffer.data( ), static_cast<std::streamsize>( buffer.size( ) ) );

    output << "<?xml version=\"1.0\"?>\n";

    StringStringMap headerAttributes { { "byte_order",  endianness( ) },
                                       { "type"      ,  type          },
                                       { "version"   ,  "0.1"         } };

    writer.addHeaderAttributes( headerAttributes );

    {
        ScopedXmlTag vtkFileTag( output, "VTKFile", headerAttributes );

        writeContent( output );

    } // VTKFile

    output.close( );
}

template<typename MeshGenerator, typename Writer> inline
void writeVtu( const std::string& filename,
               MeshGenerator& mesh,
               const std::vector<DataSetInfo>& dataSetInfo,
               const std::vector<DataSetData>& dataSetData,
               Writer&& writer )
{
    detail::writeVTUFile( filename, "UnstructuredGrid", writer, [&]( std::ostream& output )
    {
        {
            ScopedXmlTag unstructuredGridFileTag( output, "UnstructuredGrid", { } );
            {
                ScopedXmlTag pieceTag( output, "Piece",
                {
                    { "NumberOfPoints", std::to_string( mesh.numberOfPoints( ) ) },
                    { "NumberOfCells" , std::to_string( mesh.numberOfCells( )  ) }

                } );

                {
                    ScopedXmlTag pointDataTag( output, "PointData", { } );

                    detail::writeDataSets( dataSetInfo, dataSetData,
                        output, writer, DataSetType::PointData );

                } // PointData

                {
                    ScopedXmlTag cellDataTag( output, "CellData", { } );

                    detail::writeDataSets( dataSetInfo, dataSetData,
                        output, writer, DataSetType::CellData );

                } // CellData

                {
                    ScopedXmlTag pointsTag( output, "Points", { } );

                    detail::writeDataSet( writer, output, "", 3, mesh.points( ) );

                } // Points

                {
                    ScopedXmlTag pointsTag( output, "Cells", { } );

                    detail::writeDataSet( writer, output, "connectivity", 1, mesh.connectivity( ) );
                    detail::writeDataSet( writer, output, "offsets", 1, mesh.offsets( ) );
                    detail::writeDataSet( writer, output, "types", 1, mesh.types( ) );

                } // Cells

            } // Piece
        } // UnstructuredGrid

        auto appendedAttributes = writer.appendedAttributes( );

        if( !appendedAttributes.empty( ) )
        {
            ScopedXmlTag appendedDataTag( output, "AppendedData", appendedAttributes );

            output << "_";

            writer.writeAppended( output );

        } // AppendedData

    } ); // writeVTUFile

} // writeVtu

} // namespace detail

template<typename MeshGenerator> inline
void writeVtu( const std::string& filename,
               MeshGenerator& mesh,
               const std::vector<DataSetInfo>& dataSetInfo,
               const std::vector<DataSetData>& dataSetData,
               const std::string& writeMode )
{
    auto mode = writeMode;

    std::transform( mode.begin( ), mode.end ( ), mode.begin( ), []( unsigned char c )
                    { return static_cast<unsigned char>( std::tolower( c ) ); } );

    if( mode == "ascii" )
    {
        detail::writeVtu( filename, mesh, dataSetInfo, dataSetData, AsciiWriter { } );
    }
    else if( mode == "base64inline" )
    {
        detail::writeVtu( filename, mesh, dataSetInfo, dataSetData, Base64BinaryWriter { } );
    }
    else if( mode == "base64appended" )
    {
        detail::writeVtu( filename, mesh, dataSetInfo, dataSetData, Base64BinaryAppendedWriter { } );
    }
    else if( mode == "rawbinary" )
    {
        detail::writeVtu( filename, mesh, dataSetInfo, dataSetData, RawBinaryAppendedWriter { } );
    }
    else if( mode == "rawbinarycompressed" )
    {
        #ifdef VTU11_ENABLE_ZLIB
            detail::writeVtu( filename, mesh, dataSetInfo, dataSetData, CompressedRawBinaryAppendedWriter { } );
        #else
            detail::writeVtu( filename, mesh, dataSetInfo, dataSetData, RawBinaryAppendedWriter { } );
        #endif
    }
    else
    {
        VTU11_THROW( "Invalid write mode: \"" + writeMode + "\"." );
    }

} // writeVtu

namespace detail
{

struct PVtuDummyWriter
{
    void addHeaderAttributes( StringStringMap& ) { }
    void addDataAttributes( StringStringMap& ) { }
};

} // detail

inline void writePVtu( const std::string& path,
                       const std::string& baseName,
                       const std::vector<std::string>& filenames,
                       const std::vector<DataSetInfo>& dataSetInfo,
                       const size_t numberOfFiles)
{
    auto directory = vtu11fs::path { path } / baseName;
    auto pvtufile = vtu11fs::path { path } / ( baseName + ".pvtu" );

    // create directory for vtu files if not existing
    if( !vtu11fs::exists( directory ) )
    {
        vtu11fs::create_directories( directory );
    }

    detail::PVtuDummyWriter writer;

    detail::writeVTUFile( pvtufile.string( ), "PUnstructuredGrid", writer,
                          [&]( std::ostream& output )
    {
        std::string ghostLevel = "0"; // Hardcoded to be 0

        ScopedXmlTag pUnstructuredGridFileTag( output,
            "PUnstructuredGrid", { { "GhostLevel", ghostLevel } } );

        {
            ScopedXmlTag pPointDataTag( output, "PPointData", { } );

            detail::writeDataSetPVtuHeaders( dataSetInfo, output, writer, DataSetType::PointData );

        } // PPointData

        {
            ScopedXmlTag pCellDataTag( output, "PCellData", { } );

            detail::writeDataSetPVtuHeaders( dataSetInfo, output, writer, DataSetType::CellData );

        } // PCellData

        {
            ScopedXmlTag pPointsTag( output, "PPoints", { } );
            StringStringMap attributes = { { "type", dataTypeString<double>( ) }, { "NumberOfComponents", "3" } };

            writer.addDataAttributes( attributes );

            writeEmptyTag( output, "PDataArray", attributes );

        } // PPoints

        for( size_t nFiles = 0; nFiles < numberOfFiles; ++nFiles )
        {
            std::string pieceName = baseName + "/" + filenames[nFiles] + ".vtu";

            writeEmptyTag( output, "Piece", { { "Source", pieceName } } );

        } // Pieces

    } ); // writeVTUFile

} // writePVtu

template<typename MeshGenerator> inline
void writePartition( const std::string& path,
                     const std::string& baseName,
                     const std::string& filename,
                     MeshGenerator& mesh,
                     const std::vector<DataSetInfo>& dataSetInfo,
                     const std::vector<DataSetData>& dataSetData,
                     const std::string& writeMode )
{
    auto vtuname = filename + ".vtu";

    auto fullname = vtu11fs::path { path } /
                    vtu11fs::path { baseName } /
                    vtu11fs::path { vtuname };

    writeVtu( fullname.string( ), mesh, dataSetInfo, dataSetData, writeMode );

} // writePartition

} // namespace vtu11

#endif // VTU11_VTU11_IMPL_HPP
