/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = pmdStatusHistoryLogger.hpp

   Descriptive Name = pmd status history logger

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== ======== ==============================================
          05/30/2024  XJH  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef PMD_STATUSHISTORYLOGGER_HPP_
#define PMD_STATUSHISTORYLOGGER_HPP_

#include <vector>
#include "ossFile.hpp"
#include "ossUtil.hpp"
#include "ossTypes.hpp"
#include "ossMemPool.hpp"
#include "ossLatch.hpp"

#define PMD_STATUSHST_FILE_NAME  ".SEQUOIADB_STATUS_HISTORY"

namespace engine
{
   /*
      _pmdStatusHistoryLogger define
      status history logger
   */
   #define PMD_STATUS_LOG_LEN_MAX      ( 64 )
   #define PMD_STATUS_LOG_HEADER       "pid, time, isPrimary, status"OSS_NEWLINE
   #define PMD_STATUS_LOG_HEADER_SIZE  ( sizeof( PMD_STATUS_LOG_HEADER ) - 1 )
   #define PMD_STATUS_LOG_FIELD_NUM    ( 4 )
   #define PMD_STATUS_FILESIZE_LIMIT   ( 2000 * 1024 )
   #define PMD_STATUS_LOGSIZE_MAX      ( 400 * 1024 )
   #define PMD_STATUS_LOGITEM_MAX      ( 5000 )

   struct _pmdStatusLog
   {
      OSSPID            _pid ;
      ossTimestamp      _time ;
      UINT32            _primary ;
      CHAR              _status[32] ;

      _pmdStatusLog() ;

      _pmdStatusLog( OSSPID pid, ossTimestamp time, UINT32 primary, const CHAR *pStatus ) ;

      string toString()
      {
         CHAR strTime[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { 0 } ;
         CHAR strLog[ PMD_STATUS_LOG_LEN_MAX ] = { 0 } ;

         ossTimestampToString( _time, strTime ) ;

         ossSnprintf( strLog, sizeof( strLog ) - 1, "%d,%s,%d,%s"OSS_NEWLINE,
                      _pid,
                      strTime,
                      _primary,
                      _status ) ;
         return strLog ;
      }
   } ;
   typedef _pmdStatusLog pmdStatusLog ;

   BOOLEAN pmdStr2StatusLog( const string& str, pmdStatusLog& log ) ;

   typedef ossPoolVector< pmdStatusLog >  PMD_STATUS_LOG_LIST ;

   /*
      _pmdStatusHistoryLogger define
    */
   class _pmdStatusHistoryLogger : public SDBObject
   {
      public:
         _pmdStatusHistoryLogger () ;
         ~_pmdStatusHistoryLogger () ;

         INT32 init() ;
         INT32 clearAll() ;
         INT32 getLatestLogs( UINT32 num, PMD_STATUS_LOG_LIST &vecLogs );

         INT32 log() ;

      protected:
         INT32 _clearEarlyLogs() ;
         INT32 _loadLogs() ;

         INT32 _openFile() ;
         INT32 _checkAndClearLogs() ;

      private:
         ossSpinSLatch _loggerLock ;
         ossFile _file ;
         PMD_STATUS_LOG_LIST _buffer ;
         BOOLEAN _initOk ;
         CHAR _fileName[ OSS_MAX_PATHSIZE + 1 ] ;
         pmdStatusLog _lastLog ;
   };

   typedef _pmdStatusHistoryLogger pmdStatusHistoryLogger ;

   pmdStatusHistoryLogger* pmdGetStatusHstLogger () ;

}

#endif //PMD_STATUSHISTORYLOGGER_HPP_

