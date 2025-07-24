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

   Source File Name = sptDBCursor.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          24/10/2017  WJM  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SPT_DB_CURSOR_HPP
#define SPT_DB_CURSOR_HPP
#include "client.hpp"
#include "sptApi.hpp"

namespace engine
{
   class _sptDBCursor : public SDBObject
   {
   JS_DECLARE_CLASS( _sptDBCursor )
   public:
      _sptDBCursor( sdbclient::_sdbCursor *pCursor = NULL ) ;
      virtual ~_sptDBCursor() ;
   public:
      INT32 construct( const _sptArguments &arg,
                       _sptReturnVal &rval,
                       bson::BSONObj &detail ) ;

      INT32 destruct() ;

      INT32 close( const _sptArguments &arg,
                   _sptReturnVal &rval,
                   bson::BSONObj &detail ) ;

      INT32 advance( const _sptArguments &arg,
                     _sptReturnVal &rval,
                     bson::BSONObj &detail ) ;

      INT32 next( const _sptArguments &arg,
                  _sptReturnVal &rval,
                  bson::BSONObj &detail ) ;

      INT32 current( const _sptArguments &arg,
                     _sptReturnVal &rval,
                     bson::BSONObj &detail ) ;

      INT32 resolve( const _sptArguments &arg, UINT32 opcode,
                     BOOLEAN &processed, string &callFunc, BOOLEAN &setIDProp,
                     _sptReturnVal &rval, BSONObj &detail ) ;

      sdbclient::_sdbCursor *getCursor()
      {
         return _cursor.pCursor ;
      }

      static INT32 cvtToBSON( const CHAR* key, const sptObject &value,
                              BOOLEAN isSpecialObj, BSONObjBuilder& builder,
                              string &errMsg ) ;
      static INT32 fmpToCursor( const sptObject &value,
                                sdbclient::_sdbCursor** pCursor,
                                string &errMsg ) ;
   private:
      sdbclient::sdbCursor _cursor ;
      BOOLEAN _hasRead ;
      BOOLEAN _finishRead ;
   };
   typedef _sptDBCursor sptDBCursor ;

   #define SPT_SET_CURSOR_TO_RETURNVAL( pCursor )\
      do\
      {\
         sptDBCursor *__sptCursor__ = SDB_OSS_NEW sptDBCursor( pCursor ) ;\
         if( NULL == __sptCursor__ )\
         {\
            rc = SDB_OOM ;\
            detail = BSON( SPT_ERR << "Failed to alloc memory for sptDBCursor" ) ;\
            goto error ;\
         }\
         rc = rval.setUsrObjectVal< sptDBCursor >( __sptCursor__ ) ;\
         if( SDB_OK != rc )\
         {\
            SAFE_OSS_DELETE( __sptCursor__ ) ;\
            pCursor = NULL ;\
            detail = BSON( SPT_ERR << "Failed to set return obj" ) ;\
            goto error ;\
         }\
      }while(0)

}
#endif
