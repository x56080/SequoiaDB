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

   Source File Name = utilFileStream.hpp

   Descriptive Name = File I/O stream

   When/how to use: this program may be used on binary and text-formatted
   versions of DPS component. This file contains code logic for log page
   operations

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          8/6/2016  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_FILE_STREAM_HPP_
#define UTIL_FILE_STREAM_HPP_

#include "utilStream.hpp"
#include "ossFile.hpp"
#include <string>

namespace engine
{
   class utilFileStreamBase: public SDBObject
   {
   private:
      // disallow copy and assign
      utilFileStreamBase( const utilFileStreamBase& ) ;
      void operator=( const utilFileStreamBase& ) ;

   protected:
      utilFileStreamBase() ;
      
   public:
      virtual ~utilFileStreamBase() ;
      INT32 init( ossFile* file, BOOLEAN fileManaged ) ;
      OSS_INLINE ossFile* file() const { return _file ; }

   protected:
      // in stream only
      INT32 _read( CHAR* buf, INT64 bufLen, INT64& readSize ) ;

      // out stream only
      INT32 _write( const CHAR* buf, INT64 bufLen ) ;
      INT32 _flush() ;

      INT32 _close() ;

   protected:
      ossFile*    _file ;
      BOOLEAN     _fileManaged ;
      BOOLEAN     _inited ;
   } ;
   
   class utilFileInStream: public utilFileStreamBase, public utilInStream
   {
   public:
      utilFileInStream() ;
      ~utilFileInStream() ;

   public:
      INT32 read( CHAR* buf, INT64 bufLen, INT64& readSize ) ;
      INT32 close() ;
   } ;

   class utilFileOutStream: public utilFileStreamBase, public utilOutStream
   {
   public:
      utilFileOutStream() ;
      ~utilFileOutStream() ;

   public:
      INT32 write( const CHAR* buf, INT64 bufLen ) ;
      INT32 flush() ;
      INT32 close() ;
   } ;
}

#endif /* UTIL_FILE_STREAM_HPP_ */
