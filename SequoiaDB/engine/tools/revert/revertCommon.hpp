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

   Source File Name = revertCommon.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/12/2023  Yang Qincheng  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef REVERT_COMMON_HPP_
#define REVERT_COMMON_HPP_

#include "ossTypes.hpp"
#include "ossLatch.hpp"
#include "ossAtomic.hpp"
#include "dpsDef.hpp"
#include "dpsLogFileMgr.hpp"
#include "dpsArchiveFile.hpp"
#include "../bson/bson.hpp"
#include <vector>
#include <string>

using namespace std ;

namespace sdbrevert
{
   #define SDB_REVERT_LOG                   "sdbrevert.log"
   #define SDB_REVERT_ARCHIVELOG_PREFIX     DPS_ARCHIVE_FILE_PREFIX
   #define SDB_REVERT_REPLICALOG_PREFIX     DPS_LOG_FILE_PREFIX
   #define SDB_REVERT_ARCHIVELOG_M_SUFFIX   ".m"
   #define SDB_REVERT_TMP_ARCHIVELOG_SUFFIX ".tmp"
   #define SDB_REVERT_REPLICALOG_META       DPS_METAFILE_NAME

   #define SDB_REVERT_LOB_LOCK_MAX_NUM      100

   // delete doc format
   #define SDB_REVERT_DOC_DELETE_ENTRY      "entry"
   #define SDB_REVERT_DOC_DELETE_OPTYPE     "optype"
   #define SDB_REVERT_DOC_DELETE_LABEL      "label"
   #define SDB_REVERT_DOC_DELETE_LSN        "lsn"
   #define SDB_REVERT_DOC_DELETE_SOURCE     "source"

   #define SDB_REVERT_MATCH_FIELD_OID       "_id"

   // op type
   #define SDB_REVERT_OPTYPE_DOC_DELETE     "DOC_DELETE"

   #define SDB_REVERT_PARAM_DELIMITER       ','

   #define SDB_REVERT_MAX_TIME              "2037-12-31-23:59:59"

   class logFileMgr : public SDBObject
   {
      public:
         void push( const string &filePath ) ;
         const string pop() ;
         BOOLEAN empty() ;

      private:
         vector<string>   _logFileList ;
         ossSpinSLatch    _mutex ;
   } ;

   class resultInfo : public SDBObject
   {
      public:
         resultInfo() ;
         ~resultInfo() ;
         void fileNumInc() ;
         void append( const resultInfo &info ) ;
         void setStartLSN( const DPS_LSN_OFFSET &lsn ) ;
         void setEndLSN( const DPS_LSN_OFFSET &lsn ) ;
         void parsedLogInc() ;
         void parsedDocInc() ;
         void parsedLobPiecesInc() ;
         void revertedDocInc() ;
         void revertedLobPiecesInc() ;
         void appendRevertedDoc( INT32 value ) ;
         void appendFailedLogFile( const string &logFileName ) ;
         void reset() ;
         const string toString() const ;

      private:
         INT32                _fileNum ;
         DPS_LSN_OFFSET       _startLSN ;
         DPS_LSN_OFFSET       _endLSN ;
         INT32                _parsedLogs ;
         INT32                _parsedDocs ;
         INT32                _parsedLobPieces ;
         INT32                _revertedDocs ;
         INT32                _revertedLobPieces ;
         vector<string>       _failedLogFiles ;
   } ;

   class globalInfoMgr : public SDBObject
   {
      public:
         globalInfoMgr() ;
         ~globalInfoMgr() ;
         void appendResultInfo( const resultInfo &info ) ;
         void appendFailedLogFile( const string &logFileName ) ;
         void printResultInfo() ;
         void setGlobalRc( INT32 rc ) ;
         INT32 getGlobalRc() ;
         void runNumInc() ;
         void runNumDec() ;
         INT32 getRunNum() ;

      private:
         resultInfo       _resultInfo ;
         ossSpinXLatch    _mutex ;
         ossAtomic32      _gloablRc ;
         ossAtomic32      _runNum ;
   } ;


   BOOLEAN isArchivelog( const string &filePath ) ;

   BOOLEAN isArchivelogM( const string &filePath ) ;

   BOOLEAN isReplicalog( const string &filePath ) ;

   BOOLEAN isReplicalogMeta( const string &filePath ) ;

   INT32 listAllLogFile( const vector<string> &pathList, logFileMgr &logFileMgr ) ;

   vector<string> splitString( const string& str, char delimiter ) ;

   BOOLEAN match( const bson::BSONObj &orgObj, const bson::BSONObj &matcherObj ) ;

   string joinPath( const string &path, const string &fileName ) ;
}

#endif /* REVERT_COMMON_HPP_ */