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

   Source File Name = pmdStatusHistoryLogger.cpp

   Descriptive Name = pmd status history logger

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== ======== ==============================================
          05/30/2024  XJH  Initial Draft

   Last Changed =

*******************************************************************************/

#include "pmdStatusHistoryLogger.hpp"
#include "pmdTrace.hpp"
#include "pmd.hpp"
#include "ossProc.hpp"
#include "utilStr.hpp"
#include "pmdDef.hpp"
#include "pmdEnv.hpp"

using namespace std ;

namespace engine
{
   /*
      _pmdStatusLog implement
   */
   _pmdStatusLog::_pmdStatusLog()
   :_pid( OSS_INVALID_PID ), _primary( 0 )
   {
      _status[0] = 0 ;
   }

   _pmdStatusLog::_pmdStatusLog( OSSPID pid, ossTimestamp time,
                                 UINT32 primary, const CHAR *pStatus )
   :_pid( pid ), _time( time ), _primary( primary )
   {
      if ( pStatus )
      {
         ossStrncpy( _status, pStatus, sizeof( _status ) - 1 ) ;
         _status[ sizeof( _status ) - 1 ] = 0 ;
      }
      else
      {
         _status[0] = 0 ;
      }
   }

   BOOLEAN pmdStr2StatusLog( const string& str, pmdStatusLog& log )
   {
      BOOLEAN isOk = TRUE ;

      vector<string> fields = utilStrSplit( str, "," ) ;
      if( PMD_STATUS_LOG_FIELD_NUM != fields.size() )
      {
         isOk = FALSE ;
      }
      else
      {
         ossSscanf( fields.at(0).c_str(), "%d", &log._pid ) ;
         ossStringToTimestamp( fields.at(1).c_str(), log._time ) ;
         ossSscanf( fields.at(2).c_str(), "%d", &log._primary ) ;
         ossStrncpy( log._status, fields.at(3).c_str(), sizeof(log._status) - 1 ) ;
         log._status[ sizeof( log._status ) - 1 ] = 0 ;
      }

      return isOk ;
   }

   /*
      _pmdStatusHistoryLogger implement
   */
   _pmdStatusHistoryLogger::_pmdStatusHistoryLogger ()
   : _initOk( FALSE )
   {
      ossMemset( _fileName, 0, OSS_MAX_PATHSIZE + 1 ) ;
   }

   _pmdStatusHistoryLogger::~_pmdStatusHistoryLogger ()
   {
      _file.close() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDSTATUSHSTLOG_INIT, "_pmdStatusHistoryLogger::init" )
   INT32 _pmdStatusHistoryLogger::init()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__PMDSTATUSHSTLOG_INIT ) ;

      ossScopedLock _lock( &_loggerLock, EXCLUSIVE ) ;

      /// build path
      rc = utilBuildFullPath( pmdGetOptionCB()->getDbPath(),
                              PMD_STATUSHST_FILE_NAME,
                              OSS_MAX_PATHSIZE, _fileName ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to build full file path , "
                    "rc: %d", rc ) ;

      // 1. open file
      rc = _openFile() ;
      if ( rc )
      {
         goto error ;
      }

      // 2. load logs
      rc = _clearEarlyLogs() ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      rc = _loadLogs() ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      _initOk = TRUE ;

   done :
      _file.close() ;
      PD_TRACE_EXITRC( SDB__PMDSTATUSHSTLOG_INIT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   INT32 _pmdStatusHistoryLogger::_openFile()
   {
      INT32 rc = SDB_OK ;
      BOOLEAN isExist = FALSE ;
      INT32 mode = OSS_READWRITE | OSS_CREATE ;
      INT32 permission = OSS_DEFAULTFILE ;
      BOOLEAN isCreated = FALSE ;

      if ( _file.isOpened() )
      {
         goto done ;
      }

      // 1. open file
      rc = ossFile::exists( _fileName, isExist ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to check existence of file[%s], "
                    "rc: %d", _fileName, rc ) ;

      rc = _file.open( _fileName, mode, permission ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to open file[%s], "
                    "rc: %d", _fileName, rc ) ;
      isCreated = ( !isExist ) ? TRUE : FALSE ;

      if ( !isExist )
      {
         CHAR *header = PMD_STATUS_LOG_HEADER ;
         rc = _file.writeN( header, PMD_STATUS_LOG_HEADER_SIZE ) ;
         PD_RC_CHECK ( rc, PDERROR, "Failed to write header into file[%s], "
                    "rc: %d", _fileName, rc ) ;
      }

   done:
      return rc ;
   error:
      _file.close() ;
      if ( isCreated )
      {
         _file.deleteFileIfExists( _fileName ) ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDSTATUSHSTLOG__LOADLOG, "_pmdStatusHistoryLogger::_loadLogs" )
   INT32 _pmdStatusHistoryLogger::_loadLogs()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__PMDSTATUSHSTLOG__LOADLOG ) ;

      INT64 fileSize = 0 ;
      INT64 readSize = 0 ;
      CHAR* readBuf = NULL ;
      vector<string> records ;

      try
      {
         // if sdb.id too large, it is abnormal
         rc = _file.getFileSize( fileSize ) ;
         PD_RC_CHECK ( rc, PDERROR, "Failed to get size of file[%s], rc: %d",
                       _file.getPath().c_str(), rc ) ;

         if ( fileSize > PMD_STATUS_FILESIZE_LIMIT )
         {
            rc = SDB_SYS ;
            PD_RC_CHECK ( rc, PDERROR, "File size is too large[%s], rc: %d",
                          _file.getPath().c_str(), rc ) ;
         }

         // read all string
         readBuf = ( CHAR* )SDB_OSS_MALLOC( fileSize + 1 ) ;
         if ( !readBuf )
         {
            rc = SDB_OOM ;
            goto error ;
         }
         ossMemset( readBuf, 0, fileSize + 1 ) ;

         rc = _file.seek( 0, OSS_SEEK_SET ) ;
         PD_RC_CHECK ( rc, PDERROR, "Failed to seek file[%s], rc: %d",
                       _file.getPath().c_str(), rc ) ;
         rc = _file.readN( readBuf, fileSize, readSize ) ;
         PD_RC_CHECK ( rc, PDERROR, "Failed to read file[%s], rc: %d",
                       _file.getPath().c_str(), rc ) ;

         records = utilStrSplit( readBuf, OSS_NEWLINE ) ;
         if ( records.empty() )
         {
            rc = SDB_SYS ;
            goto error ;
         }
         records.erase( records.begin() ) ; // erase header

         // format log
         for ( vector<string>::iterator i = records.begin();
               i != records.end();
               ++i )
         {
            pmdStatusLog log ;
            if ( pmdStr2StatusLog( *i, log ) )
            {
               _buffer.push_back( log ) ;
            }
         }
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         goto error ;
      }

   done :
      if ( readBuf )
      {
         SDB_OSS_FREE( readBuf ) ;
         readBuf = NULL ;
      }
      PD_TRACE_EXITRC( SDB__PMDSTATUSHSTLOG__LOADLOG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDSTATUSHSTLOG__CLREARLY, "_pmdStatusHistoryLogger::_clearEarlyLogs" )
   INT32 _pmdStatusHistoryLogger::_clearEarlyLogs()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__PMDSTATUSHSTLOG__CLREARLY ) ;

      INT64 fileSize = 0 ;
      INT64 readSize = 0 ;
      CHAR* readBuf = NULL ;
      vector<string> splited ;
      INT64 popSize = 0 ;
      string writeBuf ;

      try
      {
         rc = _file.getFileSize( fileSize ) ;
         PD_RC_CHECK ( rc, PDERROR, "Failed to get size of file[%s], rc: %d",
                       _file.getPath().c_str(), rc ) ;

         /// if file isn't large enough, do nothing
         if ( fileSize < PMD_STATUS_LOGSIZE_MAX )
         {
            goto done ;
         }

         /// if file is too large, just truncate it
         if ( fileSize > PMD_STATUS_FILESIZE_LIMIT )
         {
            rc = _file.truncate( PMD_STATUS_LOG_HEADER_SIZE );
            PD_RC_CHECK ( rc, PDERROR, "Failed to truncate file[%s], rc: %d",
                          _file.getPath().c_str(), rc ) ;
            goto done ;
         }

         /// pop half logs
         readBuf = ( CHAR* )SDB_OSS_MALLOC( fileSize + 1 ) ;
         if ( !readBuf )
         {
            rc = SDB_OOM ;
            goto error ;
         }
         ossMemset( readBuf, 0, fileSize + 1 ) ;
         rc = _file.readN( readBuf, fileSize, readSize ) ;
         PD_RC_CHECK ( rc, PDERROR, "Failed to read file[%s], rc: %d",
                       _file.getPath().c_str(), rc ) ;

         splited = utilStrSplit( readBuf, OSS_NEWLINE ) ;
         if ( splited.empty() )
         {
            rc = SDB_SYS ;
            goto error ;
         }
         popSize = splited.size() / 2 ;
         splited.erase( splited.begin(), splited.begin() + 1 + popSize ) ;

         for ( vector<string>::const_iterator i = splited.begin();
               i != splited.end();
               ++i )
         {
            writeBuf += *i ;
            writeBuf += OSS_NEWLINE ;
         }

         rc = _file.truncate( PMD_STATUS_LOG_HEADER_SIZE );
         PD_RC_CHECK ( rc, PDERROR, "Failed to truncate file[%s], rc: %d",
                       _file.getPath().c_str(), rc ) ;

         rc = _file.seek( 0, OSS_SEEK_END ) ;
         PD_RC_CHECK ( rc, PDERROR, "Failed to seek file[%s], rc: %d",
                       _file.getPath().c_str(), rc ) ;
         rc = _file.writeN( writeBuf.c_str(), ossStrlen( writeBuf.c_str() ) ) ;
         PD_RC_CHECK ( rc, PDERROR, "Failed to write status info into file[%s], "
                       "rc: %d", _file.getPath().c_str(), rc ) ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC( SDB__PMDSTATUSHSTLOG__CLREARLY, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   INT32 _pmdStatusHistoryLogger::_checkAndClearLogs()
   {
      INT32 rc = SDB_OK ;
      UINT32 popSize = 0 ;
      string writeBuf ;

      if ( _buffer.size() <= PMD_STATUS_LOGITEM_MAX )
      {
         goto done ;
      }

      popSize = _buffer.size() / 2 ;

      try
      {
         _buffer.erase( _buffer.begin(), _buffer.begin() + popSize ) ;

         /// flush to file
         for ( PMD_STATUS_LOG_LIST::iterator i = _buffer.begin() ; i != _buffer.end() ; ++i )
         {
            pmdStatusLog &item = *i ;
            writeBuf += item.toString() ;
         }

         /// truncate
         rc = _file.truncate( PMD_STATUS_LOG_HEADER_SIZE );
         PD_RC_CHECK ( rc, PDERROR, "Failed to tuncate file[%s], rc: %d",
                       _file.getPath().c_str(), rc ) ;

         rc = _file.seek( 0, OSS_SEEK_END ) ;
         PD_RC_CHECK ( rc, PDERROR, "Failed to seek file[%s], rc: %d",
                       _file.getPath().c_str(), rc ) ;

         rc = _file.writeN( writeBuf.c_str(), ossStrlen( writeBuf.c_str() ) ) ;
         PD_RC_CHECK ( rc, PDERROR, "Failed to write status info into file[%s], "
                       "rc: %d", _file.getPath().c_str(), rc ) ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _pmdStatusHistoryLogger::log()
   {
      INT32 rc = SDB_OK ;
      ossTimestamp curTime ;
      ossGetCurrentTime( curTime ) ;
      string strLog ;

      ossScopedLock _lock( &_loggerLock, EXCLUSIVE ) ;

      if( FALSE == _initOk )
      {
         rc = SDB_SYS ;
         goto error ;
      }

      try
      {
         pmdStatusLog log( ossGetCurrentProcessID(), curTime,
                           pmdIsPrimary() ? 1 : 0,
                           pmdGetKRCB()->getDBStatusDesp() ) ;

         if ( log._pid == _lastLog._pid &&
              log._primary == _lastLog._primary &&
              0 == ossStrcmp( log._status, _lastLog._status ) )
         {
            /// not change, ignore
            goto done ;
         }

         strLog = log.toString() ;

         /// 1.open file
         rc = _openFile() ;
         if ( rc )
         {
            goto error ;
         }

         /// 2.write to file
         rc = _file.seek( 0, OSS_SEEK_END ) ;
         PD_RC_CHECK ( rc, PDERROR, "Failed to write seek file[%s], "
                       "rc: %d", _file.getPath().c_str(), rc ) ;

         rc = _file.writeN( strLog.c_str(), strLog.length() ) ;
         PD_RC_CHECK ( rc, PDERROR, "Failed to write status info into file[%s], "
                       "rc: %d", _file.getPath().c_str(), rc ) ;

         _lastLog = log ;

         /// 3. push to buffer
         _buffer.push_back( log ) ;

         /// 4. check and clear by size
         _checkAndClearLogs() ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         goto error ;
      }

   done:
      _file.close() ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDSTATUSHSTLOG_GETLATEST, "_pmdStatusHistoryLogger::getLatestLogs" )
   INT32 _pmdStatusHistoryLogger::getLatestLogs( UINT32 num, PMD_STATUS_LOG_LIST &vecLogs )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__PMDSTATUSHSTLOG_GETLATEST ) ;

      ossScopedLock _lock( &_loggerLock, SHARED ) ;

      if( FALSE == _initOk )
      {
         rc = SDB_SYS ;
         goto error ;
      }

      try
      {
         for( PMD_STATUS_LOG_LIST::reverse_iterator it = _buffer.rbegin();
              it != _buffer.rend() && vecLogs.size() < num ;
              it++ )
         {
            vecLogs.push_back(*it) ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to get status logs, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC( SDB__PMDSTATUSHSTLOG_GETLATEST, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDSTATUSHSTLOG_CLRALL, "_pmdStatusHistoryLogger::clearAll" )
   INT32 _pmdStatusHistoryLogger::clearAll()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__PMDSTATUSHSTLOG_CLRALL ) ;

      ossScopedLock _lock( &_loggerLock, EXCLUSIVE ) ;

      if( FALSE == _initOk )
      {
         rc = SDB_SYS ;
         goto error ;
      }

      _buffer.clear() ;
      _file.close() ;

      rc = _file.deleteFileIfExists( _fileName ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to delete file[%s], rc: %d",
                    _file.getPath().c_str(), rc ) ;

   done :
      PD_TRACE_EXITRC( SDB__PMDSTATUSHSTLOG_CLRALL, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   pmdStatusHistoryLogger* pmdGetStatusHstLogger ()
   {
      static pmdStatusHistoryLogger _statusLogger ;
      return &_statusLogger ;
   }

}


