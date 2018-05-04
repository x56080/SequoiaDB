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

   Source File Name = utilStream.hpp

   Descriptive Name = Stream API

   When/how to use: this program may be used on binary and text-formatted
   versions of DPS component. This file contains code logic for log page
   operations

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          8/5/2016  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_STREAM_HPP_
#define UTIL_STREAM_HPP_

#include "ossTypes.hpp"
#include "oss.hpp"

namespace engine
{
   /* interface */
   class utilInStream: public SDBObject
   {
   private:
      // disallow copy and assign
      utilInStream( const utilInStream& ) ;
      void operator=( const utilInStream& ) ;
   protected:
      utilInStream() {}
   public:
      virtual ~utilInStream() {}
      virtual INT32 read( CHAR* buf, INT64 bufLen, INT64& readSize ) = 0 ;
      virtual INT32 close() = 0 ;
   } ;

   /* interface */
   class utilOutStream: public SDBObject
   {
   private:
      // disallow copy and assign
      utilOutStream( const utilOutStream& ) ;
      void operator=( const utilOutStream& ) ;
   protected:
      utilOutStream() {}
   public:
      virtual ~utilOutStream() {}
      virtual INT32 write( const CHAR* buf, INT64 bufLen ) = 0 ;
      virtual INT32 flush() = 0 ;
      virtual INT32 close() = 0 ;
   } ;

   #define UTIL_STREAM_DEFAULT_BUFFER_SIZE (64 * 1024)

   class utilStreamInterrupt
   {
   public:
      utilStreamInterrupt() {}
      virtual ~utilStreamInterrupt() {}
      virtual BOOLEAN isInterrupted() = 0 ;
   } ;

   class utilStream: public SDBObject
   {
   public:
      utilStream() ;
      ~utilStream() ;

   public:
      INT32 init( INT32 bufSize = UTIL_STREAM_DEFAULT_BUFFER_SIZE ) ;
      INT32 copy( utilInStream& in, utilOutStream& out,
                  INT64* streamSize = NULL, utilStreamInterrupt* si = NULL ) ;
      INT32 copy( utilInStream& in, utilOutStream&out, INT64 size,
                  INT64* streamSize = NULL, utilStreamInterrupt* si = NULL ) ;

   private:
      CHAR* _buf ;
      INT32 _bufSize ;
   } ;
}

#endif /* UTIL_STREAM_HPP_ */
