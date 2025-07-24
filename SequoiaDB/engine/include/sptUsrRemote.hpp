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

   Source File Name = sptUsrRemote.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          19/07/2016  WJM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SPT_USRREMOTE_HPP__
#define SPT_USRREMOTE_HPP__

#include "sptUsrRemoteAssit.hpp"
#include "sptApi.hpp"
#include "../bson/bsonobj.h"
#include "oss.hpp"

namespace engine
{
   class _sptUsrRemote : public SDBObject
   {
   JS_DECLARE_CLASS( _sptUsrRemote )

   public:
      _sptUsrRemote() ;

      ~_sptUsrRemote() ;

   public:
      INT32 construct( const _sptArguments &arg,
                       _sptReturnVal &rval,
                       bson::BSONObj &detail) ;

      INT32 destruct() ;

      INT32 toString( const _sptArguments &arg,
                      _sptReturnVal &rval,
                      bson::BSONObj &detail ) ;

      INT32 getInfo( const _sptArguments &arg,
                     _sptReturnVal &rval,
                     bson::BSONObj &detail ) ;

      INT32 close( const _sptArguments &arg,
                   _sptReturnVal &rval,
                   bson::BSONObj &detail ) ;

      INT32 runCommand( const _sptArguments &arg,
                        _sptReturnVal &rval,
                        bson::BSONObj &detail ) ;

      INT32 runCommand( const string &command,
                        const bson::BSONObj &optionObj,
                        const bson::BSONObj &matchObj,
                        const bson::BSONObj &valueObj,
                        bson::BSONObj &errDetail, bson::BSONObj &retObj,
                        BOOLEAN needRecv = TRUE ) ;
   private:
      INT32 _mergeArg( const bson::BSONObj& optionObj,
                       const bson::BSONObj& matchObj,
                       const bson::BSONObj& valueObj,
                       bson::BSONObjBuilder& builder ) ;

      INT32 _initBSONObj( BSONObj &recvObj, const CHAR* buf ) ;
   private:
      sptUsrRemoteAssit _assit ;
      string  _hostname ;
      string  _svcname ;

   } ;

}
#endif
