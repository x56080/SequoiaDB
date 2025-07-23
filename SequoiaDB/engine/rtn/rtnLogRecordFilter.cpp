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

   Source File Name = rtnLogRecordFilter.cpp

   Descriptive Name = Log Record Filter

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/01/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#include "rtnLogRecordFilter.hpp"
#include "dms.hpp"
#include "dpsDef.hpp"
#include "dpsLogRecordDef.hpp"
#include "dpsOp2Record.hpp"
#include "dpsUtil.hpp"
#include "ossErr.h"
#include "ossMemPool.hpp"
#include "ossUtil.h"
#include "ossUtil.hpp"
#include "pmd.hpp"
#include "pdTrace.hpp"
#include "pmdRestSession.hpp"
#include "rtnAlterTask.hpp"
#include "rtnTrace.hpp"
#include "utilUniqueID.hpp"
#include <exception>

using namespace std ;

namespace engine
{

   /*
      _rtnLogRecordFilter implement
    */
   _rtnLogRecordFilter::_rtnLogRecordFilter( utilChangeStreamWatchInfo &watchInfo )
   : _watchInfo( watchInfo )
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNLOGRECFILTER_FILTERINFO, "_rtnLogRecordFilter::filterInfo" )
   INT32 _rtnLogRecordFilter::filterInfo( const utilChangeStreamLogInfo &logInfo,
                                          BOOLEAN &isMatched,
                                          BOOLEAN &needFilterRecord,
                                          BOOLEAN &needCheckError,
                                          BOOLEAN &needFilterType )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNLOGRECFILTER_FILTERINFO ) ;

      isMatched = FALSE ;
      needFilterRecord = FALSE ;
      needCheckError = FALSE ;
      needFilterType = FALSE ;

      if ( UTIL_WATCH_ALL == _watchInfo.getWatchLevel() )
      {
         // watch all
         isMatched = TRUE ;
      }
      else
      {
         utilCSUniqueID csUniqueID = logInfo.getCSUID() ;
         utilCLUniqueID clUniqueID = logInfo.getCLUID() ;

         if ( UTIL_UNIQUEID_NULL != clUniqueID )
         {
            // watch collection by unqiue ID
            isMatched = _watchInfo.isWatchingCL( clUniqueID, needFilterRecord ) ;
         }
         else if ( UTIL_UNIQUEID_NULL != csUniqueID )
         {
            // watch collection space by unqiue ID
            isMatched = _watchInfo.isWatchingCS( csUniqueID, needFilterRecord ) ;
         }
         else
         {
            // it is a system collection or collection space
            isMatched = _watchInfo.isWatchingCL( clUniqueID, needFilterRecord ) ||
                        _watchInfo.isWatchingCS( csUniqueID, needFilterRecord ) ;

            if ( !isMatched )
            {
               // special case for logs without collection info
               // - invalidate catalog
               // - transaction commit
               // - transaction rollback
               if ( LOG_TYPE_INVALIDATE_CATA == logInfo.getLogType() ||
                    LOG_TYPE_TS_COMMIT == logInfo.getLogType() ||
                    LOG_TYPE_TS_ROLLBACK == logInfo.getLogType() )
               {
                  isMatched = TRUE ;
                  needFilterRecord = TRUE ;
               }
            }
         }

         // check error if log type in error log type
         needCheckError = isMatched &&
                          _watchInfo.isErrorLogType( logInfo.getLogType() ) ;
      }

      if ( isMatched )
      {
         // if is matched, by not in watching log type, need check error
         // e.g. for drop collection, etc.
         if ( !( _watchInfo.isWatchingLogType( logInfo.getLogType() ) ) )
         {
            if ( needCheckError )
            {
               needFilterType = TRUE ;
            }
            else
            {
               isMatched = FALSE ;
            }
         }
      }

      PD_TRACE_EXITRC( SDB__RTNLOGRECFILTER_FILTERINFO, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNLOGRECFILTER_FILTERRECORD, "_rtnLogRecordFilter::filterRecord" )
   INT32 _rtnLogRecordFilter::filterRecord( const dpsLogRecord &record,
                                            BOOLEAN &isMatched )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNLOGRECFILTER_FILTERRECORD ) ;

      DPS_LOG_TYPE logType = (DPS_LOG_TYPE)( record.head()._type ) ;

      isMatched = FALSE ;

      switch ( logType )
      {
      case LOG_TYPE_DATA_INSERT :
      case LOG_TYPE_DATA_UPDATE :
      case LOG_TYPE_DATA_DELETE :
      case LOG_TYPE_DATA_POP :
      case LOG_TYPE_CL_CRT :
      case LOG_TYPE_CL_DELETE :
      case LOG_TYPE_IX_CRT :
      case LOG_TYPE_IX_DELETE :
      case LOG_TYPE_CL_TRUNC :
      case LOG_TYPE_LOB_WRITE :
      case LOG_TYPE_LOB_UPDATE :
      case LOG_TYPE_LOB_REMOVE :
      case LOG_TYPE_LOB_TRUNCATE :
      {
         const CHAR *clName = NULL ;

         // get full name as collection name
         dpsLogRecord::iterator iter = record.find( DPS_LOG_PUBLIC_FULLNAME ) ;
         PD_CHECK( iter.valid(), SDB_SYS, error, PDERROR, "Failed to "
                   "filter log record, [%s] log record without fullname",
                   dpsGetOPName( logType ) ) ;
         clName = iter.value() ;

         // test if watching
         isMatched = _watchInfo.isWatchingCL( clName ) ;

         break ;
      }
      case LOG_TYPE_INVALIDATE_CATA :
      {
         const CHAR *fullName = NULL ;

         // get full name
         dpsLogRecord::iterator iter = record.find( DPS_LOG_PUBLIC_FULLNAME ) ;
         PD_CHECK( iter.valid(), SDB_SYS, error, PDERROR, "Failed to "
                   "filter log record, [%s] log record without fullname",
                   dpsGetOPName( logType ) ) ;
         fullName = iter.value() ;

         // check full name
         if ( NULL == ossStrchr( fullName, '.' ) )
         {
            if ( 0 == ossStrcmp( fullName, "SYS" ) )
            {
               // it is database level
               isMatched = TRUE ;
            }
            else
            {
               // it is collection space level
               isMatched = _watchInfo.isWatchingCS( fullName ) ;
            }
         }
         else
         {
            // it is collection level
            isMatched = _watchInfo.isWatchingCL( fullName ) ;
         }
         break ;
      }
      case LOG_TYPE_ALTER :
      {
         const CHAR *fullName = NULL ;
         RTN_ALTER_OBJECT_TYPE objectType = RTN_ALTER_INVALID_OBJECT ;

         // get full name
         dpsLogRecord::iterator iter = record.find( DPS_LOG_PUBLIC_FULLNAME ) ;
         PD_CHECK( iter.valid(), SDB_SYS, error, PDERROR, "Failed to "
                   "filter log record, [%s] log record without full name",
                   dpsGetOPName( logType ) ) ;
         fullName = iter.value() ;

         // get object type
         iter = record.find( DPS_LOG_ALTER_OBJECT_TYPE ) ;
         PD_CHECK( iter.valid(), SDB_SYS, error, PDERROR, "Failed to "
                   "filter log record, [%s] log record without object type",
                   dpsGetOPName( logType ) ) ;
         objectType = (RTN_ALTER_OBJECT_TYPE)( *( (INT32 *)( iter.value() ) ) ) ;

         // test if watching
         if ( RTN_ALTER_COLLECTION_SPACE == objectType )
         {
            isMatched = _watchInfo.isWatchingCS( fullName ) ;
         }
         else if ( RTN_ALTER_COLLECTION == objectType )
         {
            isMatched = _watchInfo.isWatchingCL( fullName ) ;
         }
         break ;
      }
      case LOG_TYPE_CS_CRT :
      {
         const CHAR *csName = NULL ;

         // get collectino space name
         dpsLogRecord::iterator iter = record.find( DPS_LOG_CSCRT_CSNAME ) ;
         PD_CHECK( iter.valid(), SDB_SYS, error, PDERROR, "Failed to "
                   "filter log record, [%s] log record without "
                   "collection space name", dpsGetOPName( logType ) ) ;
         csName = iter.value() ;

         // test if watching
         isMatched = _watchInfo.isWatchingCS( csName ) ;

         break ;
      }
      case LOG_TYPE_CS_DELETE :
      {
         const CHAR *csName = NULL ;

         // get collectino space name
         dpsLogRecord::iterator iter = record.find( DPS_LOG_CSDEL_CSNAME ) ;
         PD_CHECK( iter.valid(), SDB_SYS, error, PDERROR, "Failed to "
                   "filter log record, [%s] log record without "
                   "collection space name", dpsGetOPName( record.head()._type ) ) ;
         csName = iter.value() ;

         // test if watching
         isMatched = _watchInfo.isWatchingCS( csName ) ;

         break ;
      }
      case LOG_TYPE_CS_RENAME :
      {
         const CHAR *csName = NULL ;

         // get collection space name
         dpsLogRecord::iterator iter = record.find( DPS_LOG_CSRENAME_CSNAME ) ;
         PD_CHECK( iter.valid(), SDB_SYS, error, PDERROR, "Failed to "
                   "filter log record, [%s] log record without "
                   "collection space name", dpsGetOPName( record.head()._type ) ) ;
         csName = iter.value() ;

         // test if watching
         isMatched = _watchInfo.isWatchingCS( csName ) ;

         break ;
      }
      case LOG_TYPE_CL_RENAME :
      {
         const CHAR *csName = NULL ;
         const CHAR *clName = NULL ;
         CHAR fullName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] = { 0 } ;

         // get collection space name
         dpsLogRecord::iterator iter = record.find( DPS_LOG_CLRENAME_CSNAME ) ;
         PD_CHECK( iter.valid(), SDB_SYS, error, PDERROR, "Failed to "
                   "filter log record, [%s] log record without "
                   "collection space name", dpsGetOPName( record.head()._type ) ) ;
         csName = iter.value() ;

         // get collection name
         iter = record.find( DPS_LOG_CLRENAME_CLOLDNAME ) ;
         PD_CHECK( iter.valid(), SDB_SYS, error, PDERROR, "Failed to "
                   "filter log record, [%s] log record without "
                   "collection name", dpsGetOPName( record.head()._type ) ) ;
         clName = iter.value() ;

         // merge full name
         ossSnprintf( fullName, sizeof( fullName ), "%s.%s", csName, clName ) ;

         // test if watching
         isMatched = _watchInfo.isWatchingCL( fullName ) ;

         break ;
      }
      case LOG_TYPE_ADDUNIQUEID :
      {
         const CHAR *csName = NULL ;

         // get collection space name
         dpsLogRecord::iterator iter = record.find( DPS_LOG_ADDUNIQUEID_CSNAME ) ;
         PD_CHECK( iter.valid(), SDB_SYS, error, PDERROR, "Failed to "
                   "filter log record, [%s] log record without "
                   "collection space name", dpsGetOPName( record.head()._type ) ) ;
         csName = iter.value() ;

         // test if watching
         isMatched = _watchInfo.isWatchingCS( csName ) ;

         break ;
      }
      case LOG_TYPE_RETURN :
      {
         dpsLogRecord::iterator iter = record.find( DPS_LOG_RETURN_OPTIONS ) ;
         PD_CHECK( iter.valid(), SDB_SYS, error, PDERROR, "Failed to "
                   "filter log record, [%s] log record without "
                   "return options", dpsGetOPName( record.head()._type ) ) ;

         // test if watching
         isMatched = ( 0 != *( (INT32 *)( iter.value() ) ) ) ;

         break ;
      }
      case LOG_TYPE_TS_COMMIT :
      case LOG_TYPE_TS_ROLLBACK :
      {
         if ( UTIL_WATCH_ALL == _watchInfo.getWatchLevel() )
         {
            // just let it matched
            isMatched = TRUE ;
         }
         else
         {
            DPS_TRANS_ID transID = DPS_INVALID_TRANS_ID ;
            rc = dpsGetTransIDFromRecord( record, TRUE, transID ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get transaction ID "
                         "from [%s] log record, rc: %d",
                         dpsGetOPName( record.head()._type ), rc ) ;
            transID = DPS_TRANS_GET_ID( transID ) ;
            isMatched = _transSet.count( transID ) ;
         }
         break ;
      }
      case LOG_TYPE_SEC_KEY_CRT :
      case LOG_TYPE_DUMMY :
      {
         break ;
      }
      default:
      {
         PD_LOG( PDERROR, "Failed to filter log record, invalid log type [%d]",
                 record.head()._type ) ;
         SDB_ASSERT( FALSE, "invalid log type" ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNLOGRECFILTER_FILTERRECORD, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNLOGRECFILTER_ADDTRANSID, "_rtnLogRecordFilter::addTransID" )
   INT32 _rtnLogRecordFilter::addTransID( const DPS_TRANS_ID &transID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNLOGRECFILTER_ADDTRANSID ) ;

      try
      {
         DPS_TRANS_ID origTransID = DPS_TRANS_GET_ID( transID ) ;
         _transSet.insert( origTransID ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to save transaction ID, "
                 "occurred exception [%s]", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNLOGRECFILTER_ADDTRANSID, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNLOGRECFILTER_REMOVETRANSID, "_rtnLogRecordFilter::removeTransID" )
   INT32 _rtnLogRecordFilter::removeTransID( const DPS_TRANS_ID &transID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNLOGRECFILTER_REMOVETRANSID ) ;

      DPS_TRANS_ID origTransID = DPS_TRANS_GET_ID( transID ) ;
      _transSet.erase( origTransID ) ;

      PD_TRACE_EXITRC( SDB__RTNLOGRECFILTER_REMOVETRANSID, rc ) ;

      return rc ;
   }

}
