/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = utilZlibStream.hpp

   Descriptive Name = Zlib compression stream

   When/how to use: this program may be used on binary and text-formatted
   versions of DPS component. This file contains code logic for log page
   operations

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          8/8/2016  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_ZLIB_STREAM_HPP_
#define UTIL_ZLIB_STREAM_HPP_

#include "utilCompressionStream.hpp"

extern "C" struct z_stream_s ;

namespace engine
{

   class utilZlibInStream: public utilCompressionInStream
   {
   public:
      utilZlibInStream() ;
      ~utilZlibInStream() ;

   public:
      INT32 init( utilInStream& upstream,
                  INT32 bufSize = UTIL_STREAM_DEFAULT_BUFFER_SIZE ) ;
      INT32 read( CHAR* buf, INT64 bufLen, INT64& readSize ) ;
      INT32 close() ;

   private:
      z_stream_s* _zstream ;
      CHAR*       _zbuf ;
      INT32       _zbufSize ;
      BOOLEAN     _inited ;
      BOOLEAN     _read ;
      BOOLEAN     _end ;
   } ;

   class utilZlibOutStream: public utilCompressionOutStream
   {
   public:
      utilZlibOutStream() ;
      ~utilZlibOutStream() ;

   public:
      INT32 init( utilOutStream& downstream, 
                  UTIL_COMPRESSION_LEVEL level = UTIL_COMP_BALANCE,
                  INT32 bufSize = UTIL_STREAM_DEFAULT_BUFFER_SIZE ) ;
      INT32 write( const CHAR* buf, INT64 bufLen ) ;
      INT32 flush() ;
      INT32 finish() ;
      INT32 close() ;

   private:
      z_stream_s* _zstream ;
      UTIL_COMPRESSION_LEVEL _level ;
      CHAR*       _zbuf ;
      INT32       _zbufSize ;
      BOOLEAN     _inited ;
      BOOLEAN     _finished ;
      BOOLEAN     _dirty ;
   } ;

   class utilZlibStreamCompressor: public SDBObject
   {
   public:
      utilZlibStreamCompressor() ;
      ~utilZlibStreamCompressor() ;

   public:
      INT32 init( INT32 bufSize = UTIL_STREAM_DEFAULT_BUFFER_SIZE ) ;
      INT32 compress( utilInStream& in, utilOutStream& out,
                      UTIL_COMPRESSION_LEVEL level = UTIL_COMP_BALANCE ) ;
      INT32 uncompress( utilInStream& in, utilOutStream& out ) ;

   private:
      utilStream  _stream ;
   } ;
}

#endif /* UTIL_ZLIB_STREAM_HPP_ */
