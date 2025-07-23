/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = sptUsrFileContent.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/03/2017  wujiaming  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SPT_USRFILECONTENT_HPP_
#define SPT_USRFILECONTENT_HPP_

#include "sptApi.hpp"

namespace engine
{
   class _sptUsrFileContent : public SDBObject
   {
   JS_DECLARE_CLASS( _sptUsrFileContent )
   public:
      _sptUsrFileContent() ;

      ~_sptUsrFileContent() ;

      INT32 init( const CHAR* buf, UINT32 len ) ;

      INT32 construct( const _sptArguments &arg,
                       _sptReturnVal &rval,
                       bson::BSONObj &detail ) ;

      INT32 destruct() ;

      INT32 getLength( const _sptArguments &arg,
                       _sptReturnVal &rval,
                       bson::BSONObj &detail ) ;

      INT32 toBase64Code( const _sptArguments &arg,
                          _sptReturnVal &rval,
                          bson::BSONObj &detail ) ;

      INT32 clear( const _sptArguments &arg,
                   _sptReturnVal &rval,
                   bson::BSONObj &detail ) ;

      inline SINT64 getLength()
      {
         return _length ;
      }

      inline const CHAR* getBuf()
      {
         return _buf ;
      }

      inline void clear()
      {
         if( _buf )
         {
            SDB_OSS_FREE( _buf ) ;
            _buf = NULL ;
            _length = 0 ;
         }
      }

   private:
      SINT64 _length ;
      CHAR *_buf ;
   } ;

   typedef _sptUsrFileContent sptUsrFileContent ;
}
#endif
