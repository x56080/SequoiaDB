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

   Source File Name = pmdStartupHistoryLogger.hpp

   Descriptive Name = pmd start-up history logger

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== ======== ==============================================
          01/01/2018  Ting YU  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef PMD_STARTUPHISTORYLOGGER_HPP_
#define PMD_STARTUPHISTORYLOGGER_HPP_

#include <vector>
#include "ossFile.hpp"
#include "ossUtil.hpp"
#include "ossTypes.hpp"
#include "ossMemPool.hpp"
#include "ossLatch.hpp"
#include "pmdStartup.hpp"

#define PMD_STARTUPHST_FILE_NAME ".SEQUOIADB_STARTUP_HISTORY"

namespace engine
{
   /*
      _pmdStartupHistoryLogger define
      start-up history logger
   */
   #define PMD_STARTUP_LOG_LEN_MAX     ( 64 )
   #define PMD_STARTUP_LOG_HEADER      "pid, startTime, startType, version"OSS_NEWLINE
   #define PMD_STARTUP_LOG_HEADER_SIZE ( sizeof( PMD_STARTUP_LOG_HEADER ) - 1 )
   #define PMD_STARTUP_LOG_FIELD_NUM   ( 4 )
   #define PMD_STARTUP_FILESIZE_LIMIT  ( 2000 * 1024 )
   #define PMD_STARTUP_LOGSIZE_MAX     ( 400 * 1024 )

   struct _pmdStartupLog
   {
      OSSPID            _pid ;
      ossTimestamp      _time ;
      SDB_START_TYPE    _type ;
      CHAR              _dbVersion[32] ;

      _pmdStartupLog() ;

      _pmdStartupLog( OSSPID pid, ossTimestamp time, SDB_START_TYPE type ) ;

      string toString()
      {
         CHAR strTime[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { 0 } ;
         CHAR strLog[ PMD_STARTUP_LOG_LEN_MAX ] = { 0 } ;

         ossTimestampToString( _time, strTime ) ;

         ossSnprintf( strLog, sizeof( strLog ) - 1, "%d,%s,%s,%s"OSS_NEWLINE,
                      _pid,
                      strTime,
                      pmdGetStartTypeStr( _type ),
                      _dbVersion ) ;
         return strLog ;
      }
   } ;
   typedef _pmdStartupLog pmdStartupLog ;

   BOOLEAN pmdStr2StartupLog( const string& str, pmdStartupLog& log ) ;

   typedef ossPoolVector< pmdStartupLog > PMD_STARTUP_LOG_LIST ;

   /*
      _pmdStartupHistoryLogger define
    */
   class _pmdStartupHistoryLogger : public SDBObject
   {
      public:
         _pmdStartupHistoryLogger () ;
         ~_pmdStartupHistoryLogger () ;

         INT32 init() ;
         INT32 clearAll() ;
         INT32 getLatestLogs( UINT32 num, PMD_STARTUP_LOG_LIST &vecLogs );

      protected:
         INT32 _clearEarlyLogs() ;
         INT32 _loadLogs() ;
         INT32 _log() ;

      private:
         ossSpinSLatch _loggerLock ;
         ossFile _file ;
         PMD_STARTUP_LOG_LIST _buffer ;
         BOOLEAN _initOk ;
         CHAR _fileName[ OSS_MAX_PATHSIZE + 1 ] ;
   };

   typedef _pmdStartupHistoryLogger pmdStartupHistoryLogger ;

   pmdStartupHistoryLogger* pmdGetStartupHstLogger () ;

}

#endif //PMD_STARTUPHISTORYLOGGER_HPP_

