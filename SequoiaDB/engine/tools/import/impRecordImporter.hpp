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

   Source File Name = impRecordImporter.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/7/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef IMP_RECORD_IMPORTER_HPP_
#define IMP_RECORD_IMPORTER_HPP_

#include "msg.h"
#include "core.hpp"
#include "oss.hpp"
#include "../client/client.h"
#include "impCommon.hpp"
#include <string>

using namespace std;

namespace import
{
   class RecordImporter : public SDBObject
   {
   public:
      RecordImporter( const string& hostname,
                      const string& svcname,
                      const string& user,
                      const string& password,
                      const string& csname,
                      const string& clname,
                      BOOLEAN useSSL = FALSE,
                      BOOLEAN enableTransaction = FALSE,
                      BOOLEAN allowKeyDuplication = TRUE,
                      BOOLEAN replaceKeyDuplication = FALSE,
                      BOOLEAN allowIDKeyDuplication = FALSE,
                      BOOLEAN replaceIDKeyDuplication = FALSE,
                      BOOLEAN mustHasIDField = TRUE,
                      INT32 batchSize = 1000 ) ;

      ~RecordImporter() ;

      INT32 connect() ;

      void disconnect() ;

      INT32 import( PageInfo* pageInfo ) ;

   private:
      INT32 _initInsertMsg() ;

      INT32 _bulkInsert( PageInfo* pageInfo, SINT32 flag ) ;

      INT32 _send( const CHAR *pMsg, INT32 len, BOOLEAN isHeader = TRUE ) ;

      INT32 _recv() ;

      INT32 _extract() ;

      INT32 _setSessionAttr() ;

      INT32 _getLastResultObj( bson *result ) ;

   private:
      INT32 _insertBufferSize ;
      INT32 _recvBufferSize ;
      INT32 _resultBufferSize ;

      BOOLEAN  _useSSL ;
      BOOLEAN  _enableTransaction ;
      BOOLEAN  _allowKeyDuplication ;
      BOOLEAN  _replaceKeyDuplication ;
      BOOLEAN  _allowIDKeyDuplication ;
      BOOLEAN  _replaceIDKeyDuplication ;
      BOOLEAN  _endianConvert ;
      BOOLEAN  _mustHasIDField ;

      // db handle
      sdbConnectionHandle  _connection ;
      sdbCSHandle          _collectionSpace ;
      sdbCollectionHandle  _collection ;

      // Message header
      MsgOpInsert*  _insertMsg ;
      CHAR*         _insertBuffer ;
      CHAR*         _recvBuffer ;
      CHAR*         _resultBuffer ;

      string   _hostname ;
      string   _svcname ;
      string   _user ;
      string   _password ;
      string   _csname ;
      string   _clname ;

      INT32    _batchSize ;

      _sdbMsgConvertor* _msgConvertor ;
      INT16             _peerProtocolVersion ;

   } ;
}

#endif /* IMP_RECORD_IMPORTER_HPP_ */
