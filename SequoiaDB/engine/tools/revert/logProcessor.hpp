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

   Source File Name = logProcessor.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/12/2023  Yang Qincheng  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef REVERT_LOG_PROCESSOR_HPP_
#define REVERT_LOG_PROCESSOR_HPP_

#include "revertCommon.hpp"
#include "revertOptions.hpp"
#include "lobMetaMgr.hpp"
#include "ossTypes.hpp"
#include "ossAtomic.hpp"
#include "../bson/bson.hpp"
#include "dpsDef.hpp"
#include "dpsLogFile.hpp"
#include "client.hpp"
#include <string>

using namespace sdbclient ;
using namespace std ;
using namespace engine ;

namespace sdbrevert
{
   class logProcessor : public SDBObject
   {
      public:
         logProcessor() ;
         ~logProcessor() ;
         INT32 init( revertOptions &options,
                     logFileMgr &logFileMgr,
                     lobMetaMgr &lobMetaMgr,
                     lobLockMap &lobLockMap,
                     ossAtomic32 &globalRc,
                     globalInfoMgr &globalInfoMgr ) ;
         INT32 run() ;

      private:
         INT32 _connect() ;
         INT32 _getCL() ;
         INT32 _revertLogFile( const string &filePath ) ;
         INT32 _revertArchivelogFile( const string &filePath ) ;
         INT32 _revertReplicalogFile( const string &filePath ) ;
         INT32 _getLogFileSizeAndNum( const string &filePath, UINT32& fileSize, UINT32& fileNum ) ;
         INT32 _revertFileWithLSN( dpsLogFile &logFile,
                                   DPS_LSN_OFFSET startLSN,
                                   DPS_LSN_OFFSET endLSN ) ;
         INT32 _ensureBufSize( UINT32 size ) ;
         INT32 _revertLogRecord( const CHAR* log, DPS_LSN_OFFSET lsn ) ;
         INT32 _revertDocDelete( const CHAR* log, DPS_LSN_OFFSET lsn ) ;
         INT32 _revertLobDelete( const CHAR* log, DPS_LSN_OFFSET lsn ) ;
         INT32 _writeDocs() ;
         INT32 _writeLobPieces( const bson::OID &oid,
                                UINT32 sequence,
                                const CHAR *data,
                                UINT32 offset,
                                UINT32 len,
                                DPS_LSN_OFFSET lsn,
                                UINT32 lobPageSize ) ;
         bson::BSONObj _formatDocData( const bson::BSONObj &obj, DPS_LSN_OFFSET lsn ) ;
         INT32 _fillLobHole( sdbLob &lob, UINT32 offset, UINT32 len ) ;
         INT32 _uncompresLogFile( const string &filePath, string &newFilePath, BOOLEAN &create ) ;

      private:
         sdb*                  _conn ;
         sdbCollection*        _outputCL ;
         revertOptions*        _options ;
         BOOLEAN               _connected ;
         logFileMgr*           _logFileMgr ;
         lobMetaMgr*           _lobMetaMgr ;
         lobLockMap*           _lobLockMap ;
         globalInfoMgr*        _globalInfoMgr ;
         resultInfo            _localInfo ;
         vector<bson::BSONObj> _docBuf ;
         CHAR*                 _buf ;
         UINT32                _bufSize ;
         string                _logFilePath ;
         CHAR*                 _lobHoleBuf ;
         ossAtomic32*          _interrupted ;
   } ;
}

#endif /* REVERT_LOG_PROCESSOR_HPP_ */