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

   Source File Name = clsReplayer.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains Runtime code for insert
   request.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "clsReplayer.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "rtn.hpp"
#include "dpsOp2Record.hpp"
#include "clsReplBucket.hpp"
#include "pdTrace.hpp"
#include "clsTrace.hpp"
#include "rtnLob.hpp"
#include "rtnAlter.hpp"
#include "utilCompressor.hpp"
#include "mthModifier.hpp"

using namespace bson ;

namespace engine
{

   INT32 startIndexJob ( RTN_JOB_TYPE type,
                         const dpsLogRecordHeader *recordHeader,
                         _dpsLogWrapper *dpsCB,
                         BOOLEAN isRollBack ) ;

   #define CLS_PARALLA_CHECK_LSNNUM_MIN_SPAN          ( 10000 )

   /*
      _clsCLParallaInfo implement
   */
   _clsCLParallaInfo::_clsCLParallaInfo()
   {
      _lastInsertLSN = DPS_INVALID_LSN_OFFSET ;
      _lastUpdateLSN = DPS_INVALID_LSN_OFFSET ;
      _lastDelLSN = DPS_INVALID_LSN_OFFSET ;

      _lastParallaType = CLS_PARALLA_NULL ;

      _clearID() ;
   }

   _clsCLParallaInfo::~_clsCLParallaInfo()
   {
   }

   void _clsCLParallaInfo::_clearID()
   {
      _lastInsertID = 0 ;
      _lastDelID = 0 ;
      _lastUpdateID = 0 ;
      _idValue = 1 ;
   }

   INT32 _clsCLParallaInfo::waitLastLSN( _clsBucket *pBucket )
   {
      INT32 rc = SDB_OK ;
      DPS_LSN completeLSN = pBucket->completeLSN() ;
      DPS_LSN_OFFSET maxLSN = DPS_INVALID_LSN_OFFSET ;

      maxLSN = _max( _max( _lastInsertLSN, _lastUpdateLSN ),
                     _lastDelLSN ) ;

      while ( completeLSN.compareOffset( maxLSN ) <= 0 )
      {
         if ( CLS_BUCKET_NORMAL != pBucket->getStatus() ||
              pBucket->bucketSize() == 0 )
         {
            rc = pBucket->waitEmptyWithCheck() ;
            break ;
         }
         pBucket->waitSubmit( OSS_ONE_SEC ) ;
         completeLSN = pBucket->completeLSN() ;
      }

      return rc ;
   }

   BOOLEAN _clsCLParallaInfo::checkParalla( UINT16 type,
                                            DPS_LSN_OFFSET curLSN,
                                            _clsBucket *pBucket ) const
   {
      DPS_LSN completeLSN ;
      DPS_LSN_OFFSET otherLSN = DPS_INVALID_LSN_OFFSET ;
      UINT64 maxID = 0 ;
      BOOLEAN canRecParalla = FALSE ;

      switch ( (INT32)type )
      {
         case LOG_TYPE_DATA_INSERT :
            otherLSN = _max( _lastDelLSN, _lastUpdateLSN ) ;
            maxID = _lastDelID >= _lastUpdateID ? _lastDelID : _lastUpdateID ;

            if ( DPS_INVALID_LSN_OFFSET == otherLSN || 0 == maxID ||
                 ( CLS_PARALLA_REC == _lastParallaType &&
                   _lastInsertID + 1 == _idValue ) )
            {
               canRecParalla = TRUE ;
            }
            else
            {
               completeLSN = pBucket->completeLSN() ;
               if ( completeLSN.compareOffset( otherLSN ) > 0 &&
                    _idValue - maxID >= CLS_PARALLA_CHECK_LSNNUM_MIN_SPAN )
               {
                  canRecParalla = TRUE ;
               }
            }
            break ;
         case LOG_TYPE_DATA_UPDATE :
            break ;
         case LOG_TYPE_DATA_DELETE :
            otherLSN = _max( _lastInsertLSN, _lastUpdateLSN ) ;
            maxID = _lastInsertID >= _lastUpdateID ?
                    _lastInsertID : _lastUpdateID ;
            if ( DPS_INVALID_LSN_OFFSET == otherLSN || 0 == maxID ||
                 ( CLS_PARALLA_REC == _lastParallaType &&
                   _lastDelLSN + 1 == _idValue ) )
            {
               canRecParalla = TRUE ;
            }
            else
            {
               completeLSN = pBucket->completeLSN() ;
               if ( completeLSN.compareOffset( otherLSN ) > 0 &&
                    _idValue - maxID >= CLS_PARALLA_CHECK_LSNNUM_MIN_SPAN )
               {
                  canRecParalla = TRUE ;
               }
            }
            break ;
         default :
            break ;
      }

      return canRecParalla ;
   }

   void _clsCLParallaInfo::updateParalla( CLS_PARALLA_TYPE parallaType,
                                          UINT16 type,
                                          DPS_LSN_OFFSET curLSN )
   {
      _lastParallaType = parallaType ;

      if ( DPS_INVALID_LSN_OFFSET != _lastInsertLSN &&
           _lastInsertLSN > curLSN )
      {
         _lastInsertLSN = curLSN ;
      }
      if ( DPS_INVALID_LSN_OFFSET != _lastUpdateLSN &&
           _lastUpdateLSN > curLSN )
      {
         _lastUpdateLSN = curLSN ;
      }
      if ( DPS_INVALID_LSN_OFFSET != _lastDelLSN &&
           _lastDelLSN > curLSN )
      {
         _lastDelLSN = curLSN ;
      }

      switch ( (INT32)type )
      {
         case LOG_TYPE_DATA_INSERT :
            _lastInsertLSN = curLSN ;
            _lastInsertID = _idValue++ ;
            break ;
         case LOG_TYPE_DATA_UPDATE :
            _lastUpdateLSN = curLSN ;
            _lastUpdateID = _idValue++ ;
            break ;
         case LOG_TYPE_DATA_DELETE :
            _lastDelLSN = curLSN ;
            _lastDelID = _idValue++ ;
            break ;
         default :
            break ;
      }
   }

   CLS_PARALLA_TYPE _clsCLParallaInfo::getLastParallaType() const
   {
      return _lastParallaType ;
   }

   BOOLEAN _clsCLParallaInfo::isParallaTypeSwitch( CLS_PARALLA_TYPE type ) const
   {
      if ( CLS_PARALLA_NULL == _lastParallaType )
      {
         return FALSE ;
      }
      return _lastParallaType != type ? TRUE : FALSE ;
   }

   /*
      _clsReplayer implement
   */
   _clsReplayer::_clsReplayer( BOOLEAN useDps, BOOLEAN isReplSync )
   {
      _dmsCB = sdbGetDMSCB() ;
      _dpsCB = NULL ;
      if ( useDps )
      {
         _dpsCB = sdbGetDPSCB() ;
      }
      _monDBCB = pmdGetKRCB()->getMonDBCB () ;

      _isReplSync = isReplSync ;
   }

   _clsReplayer::~_clsReplayer()
   {

   }

   void _clsReplayer::enableDPS ()
   {
      _dpsCB = sdbGetDPSCB() ;
   }

   void _clsReplayer::disableDPS ()
   {
      _dpsCB = NULL ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSREP_REPLYBUCKET, "_clsReplayer::replayByBucket" )
   INT32 _clsReplayer::replayByBucket( dpsLogRecordHeader *recordHeader,
                                       pmdEDUCB *eduCB, clsBucket *pBucket )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSREP_REPLYBUCKET ) ;
      const CHAR *fullname = NULL ;
      BSONObj obj ;
      BSONElement idEle ;
      const bson::OID *oidPtr = NULL ;
      UINT32 sequence = 0 ;
      BOOLEAN clParalla = FALSE ;
      BOOLEAN recParalla = FALSE ;
      BOOLEAN doSameOID = FALSE ;
      clsCLParallaInfo *pInfo = NULL ;
      UINT32 bucketID = ~0 ;

      SDB_ASSERT( recordHeader && pBucket, "Invalid param" ) ;
      try
      {
      switch( recordHeader->_type )
      {
         case LOG_TYPE_DATA_INSERT :
            clParalla = TRUE ;
            doSameOID = TRUE ;
            rc = dpsRecord2Insert( (CHAR *)recordHeader, &fullname, obj ) ;
            break ;
         case LOG_TYPE_DATA_DELETE :
            clParalla = TRUE ;
            doSameOID = TRUE ;
            rc = dpsRecord2Delete( (CHAR *)recordHeader, &fullname, obj ) ;
            break ;
         case LOG_TYPE_DATA_UPDATE :
         {
            BSONObj match ;     //old match
            BSONObj oldObj ;
            BSONObj newMatch ;
            BSONObj modifier ;   //new change obj
            UINT32 logWriteMod = DMS_LOG_WRITE_MOD_INCREMENT ;
            clParalla = TRUE ;
            rc = dpsRecord2Update( (CHAR *)recordHeader, &fullname,
                                   match, oldObj, newMatch, modifier, NULL,
                                   NULL, NULL, &logWriteMod ) ;
            if ( SDB_OK == rc &&
                 0 == match.woCompare( newMatch, BSONObj(), false ) )
            {
               obj = match ;
               doSameOID = TRUE ;
            }
            break ;
         }
         case LOG_TYPE_LOB_WRITE :
         {
            recParalla = TRUE ;
            const CHAR *fullName = NULL ;
            const bson::OID *oid = NULL ;
            UINT32 offset = 0 ;
            UINT32 len = 0 ;
            UINT32 hash = 0 ;
            const CHAR *data = NULL ;
            DMS_LOB_PAGEID page = DMS_LOB_INVALID_PAGEID ;
            rc = dpsRecord2LobW( (CHAR *)recordHeader,
                                  &fullName, &oid,
                                  sequence, offset,
                                  len, hash, &data, page ) ;
            oidPtr = oid ;
            break ;
         }
         case LOG_TYPE_LOB_UPDATE :
         {
            recParalla = TRUE ;
            const CHAR *fullName = NULL ;
            const bson::OID *oid = NULL ;
            UINT32 offset = 0 ;
            UINT32 len = 0 ;
            UINT32 hash = 0 ;
            const CHAR *data = NULL ;
            UINT32 oldLen = 0 ;
            const CHAR *oldData = NULL ;
            DMS_LOB_PAGEID page = DMS_LOB_INVALID_PAGEID ;
            rc = dpsRecord2LobU( (CHAR *)recordHeader,
                                  &fullName, &oid,
                                  sequence, offset,
                                  len, hash, &data,
                                  oldLen, &oldData, page ) ;
            oidPtr = oid ;
            break ;
         }
         case LOG_TYPE_LOB_REMOVE :
         {
            recParalla = TRUE ;
            const CHAR *fullName = NULL ;
            const bson::OID *oid = NULL ;
            UINT32 offset = 0 ;
            UINT32 len = 0 ;
            UINT32 hash = 0 ;
            const CHAR *data = NULL ;
            DMS_LOB_PAGEID page = DMS_LOB_INVALID_PAGEID ;
            rc = dpsRecord2LobRm( (CHAR *)recordHeader,
                                  &fullName, &oid,
                                  sequence, offset,
                                  len, hash, &data, page ) ;
            oidPtr = oid ;
            break ;
         }
         case LOG_TYPE_DUMMY :
            bucketID = 0 ;
            break ;
         default :
            break ;
      }

      PD_RC_CHECK( rc, PDERROR, "Parse dps log[type: %d, lsn: %lld, len: %d]"
                   "falied, rc: %d", recordHeader->_type,
                   recordHeader->_lsn, recordHeader->_length, rc ) ;

      /*
         When collection paralla(clParalla) is true, but recParalla is false,
         we should update clParalla to recParalla in bellow case:
            1. unique index number <= 1 and no text index
         or 2. only the simple operator as insert/delete
      */
      if ( clParalla )
      {
         dmsStorageUnit *su = NULL ;
         const CHAR *pShortName = NULL ;
         dmsStorageUnitID suID = DMS_INVALID_SUID ;
         CLS_PARALLA_TYPE parallaType = CLS_PARALLA_CL ;
         INT32 rcTmp = SDB_OK ;

         rc = rtnResolveCollectionNameAndLock( fullname, _dmsCB, &su,
                                               &pShortName, suID ) ;
         if ( SDB_OK == rc )
         {
            // Currently parallel replaying on capped collection is forbidden,
            // because we need to be sure the records are exactly the same with
            // the ones on primary node, including their positions.
            if ( DMS_STORAGE_CAPPED != su->type() )
            {
               dmsMBContext *mbContext = NULL ;
               rc = su->data()->getMBContext( &mbContext, pShortName, SHARED ) ;
               if ( SDB_OK == rc )
               {
                  pInfo = _getOrCreateInfo( mbContext->mb()->_clUniqueID ) ;
                  if ( !pInfo )
                  {
                     rcTmp = SDB_OOM ;
                     /// can't goto error
                  }
                  // For collection who has text indices, parallel replay should
                  // also be forbidden. Otherwise, the records in the capped
                  // collection will not be exactly the same.
                  if ( !doSameOID || 0 != mbContext->mbStat()->_textIdxNum )
                  {
                     /// can't upgrade to recParalla
                  }
                  else if ( mbContext->mbStat()->_uniqueIdxNum <= 1 )
                  {
                     recParalla = TRUE ;
                     parallaType = CLS_PARALLA_REC ;
                  }
                  else if ( pInfo && pInfo->checkParalla( recordHeader->_type,
                                                          recordHeader->_lsn,
                                                          pBucket ) )
                  {
                     recParalla = TRUE ;
                     parallaType = CLS_PARALLA_REC ;
                  }
                  su->data()->releaseMBContext( mbContext ) ;
               }
            }
            _dmsCB->suUnlock( suID, SHARED ) ;

            /// when get or create clParallaInfo failed
            if ( rcTmp )
            {
               rc = rcTmp ;
               goto error ;
            }
            else if ( pInfo )
            {
               if ( pInfo->isParallaTypeSwitch( parallaType ) &&
                    SDB_OK != ( rc = pInfo->waitLastLSN( pBucket ) ) )
               {
                  INT32 bucketStatus = (INT32)pBucket->getStatus() ;
                  PD_LOG( PDWARNING, "Wait repl bucket empty failed, its "
                          "status[%s(%d)] is error",
                          clsGetReplBucketStatusDesp( bucketStatus ),
                          bucketStatus ) ;
                  goto error ;
               }

               /// update paralla info
               pInfo->updateParalla( parallaType, recordHeader->_type,
                                     recordHeader->_lsn ) ;
            } // else if ( pInfo )
         } // if ( SDB_OK == rc )
      } // if ( clParalla )

      if ( recParalla )
      {
         idEle = obj.getField( DMS_ID_KEY_NAME ) ;
         if ( !idEle.eoo() )
         {
            bucketID = pBucket->calcIndex( idEle.value(),
                                           idEle.valuesize() ) ;
         }
         else if ( NULL != oidPtr )
         {
            CHAR tmpData[ sizeof( *oidPtr ) + sizeof( sequence ) ] = { 0 } ;
            ossMemcpy( tmpData, ( const CHAR * )( oidPtr->getData()),
                       sizeof( *oidPtr ) ) ;
            ossMemcpy( &tmpData[ sizeof( *oidPtr ) ], ( const CHAR* )&sequence,
                       sizeof( sequence ) ) ;
            bucketID = pBucket->calcIndex( tmpData, sizeof( tmpData ) ) ;
         }
      }
      else if ( clParalla )
      {
         bucketID = pBucket->calcIndex( fullname, ossStrlen( fullname ) ) ;
      }

      if ( (UINT32)~0 != bucketID )
      {
         rc = pBucket->pushData( bucketID, (CHAR *)recordHeader,
                                 recordHeader->_length ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to push log to bucket, rc: %d",
                      rc ) ;
      }
      else
      {
         // wait bucket all complete and check status
         rc = pBucket->waitEmptyWithCheck() ;
         if ( rc )
         {
            INT32 bucketStatus = (INT32)pBucket->getStatus() ;
            PD_LOG( PDWARNING, "Wait repl bucket empty failed, its "
                    "status[%s(%d)] is error",
                    clsGetReplBucketStatusDesp( bucketStatus ),
                    bucketStatus ) ;
            goto error ;
         }

         // judge lsn valid
         if ( !pBucket->_expectLSN.invalid() &&
              0 != pBucket->_expectLSN.compareOffset( recordHeader->_lsn ) )
         {
            PD_LOG( PDWARNING, "Expect lsn[%lld], real complete lsn[%lld]",
                    recordHeader->_lsn, pBucket->_expectLSN.offset ) ;
            rc = SDB_CLS_REPLAY_LOG_FAILED ;
            goto error ;
         }

         _clearParallaInfo() ;

         rc = replay( recordHeader, eduCB ) ;
         // re-calc complete lsn
         if ( SDB_OK == rc && !pBucket->_expectLSN.invalid() )
         {
            pBucket->_expectLSN.offset += recordHeader->_length ;
            pBucket->_expectLSN.version = recordHeader->_version ;
         }
      }
      }
      catch (std::exception &e)
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "received unexcepted error when parsing inner bson "
                 "object dps repl log, error:%s", e.what() ) ;
         goto error ;
      }

   done:
      if ( SDB_OK != rc )
      {
         dpsLogRecord record ;
         CHAR tmpBuff[4096] = {0} ;
         INT32 rcTmp = record.load( (const CHAR*)recordHeader ) ;
         if ( SDB_OK == rcTmp )
         {
            record.dump( tmpBuff, sizeof(tmpBuff)-1, DPS_DMP_OPT_FORMATTED ) ;
         }
         PD_LOG( PDERROR, "sync bucket: replay log [type:%d, lsn:%lld, "
                 "data: %s] failed, rc: %d", recordHeader->_type,
                 recordHeader->_lsn, tmpBuff, rc ) ;
      }
      PD_TRACE_EXITRC ( SDB__CLSREP_REPLYBUCKET, rc );
      return rc ;
   error:
      goto done ;
   }

   // for all UPDATE/DELETE, make sure we use hint = {"":"$id"}, so that we can
   // bypass expensive optimizer to improve performance
   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSREP_REPLAY, "_clsReplayer::replay" )
   INT32 _clsReplayer::replay( dpsLogRecordHeader *recordHeader,
                               pmdEDUCB *eduCB, BOOLEAN incCount )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSREP_REPLAY );
      SDB_ASSERT( NULL != recordHeader, "head should not be NULL" ) ;

      if ( !_dpsCB )
      {
         eduCB->insertLsn( recordHeader->_lsn ) ;
      }

      try
      {
      switch ( recordHeader->_type )
      {
         case LOG_TYPE_DATA_INSERT :
         {
            const CHAR *fullname = NULL ;
            BSONObj obj ;
            rc = dpsRecord2Insert( (CHAR *)recordHeader,
                                   &fullname,
                                   obj ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
            rc = rtnReplayInsert( fullname, obj, 0, eduCB, _dmsCB, _dpsCB, 1 ) ;
            if ( SDB_OK == rc && incCount )
            {
               _monDBCB->monOperationCountInc ( MON_INSERT_REPL ) ;
            }
            else if ( SDB_IXM_DUP_KEY == rc )
            {
               PD_LOG( PDINFO, "Record[%s] already exist when insert",
                       obj.toString().c_str() ) ;
               rc = SDB_OK ;
            }
            break ;
         }
         case LOG_TYPE_DATA_UPDATE :
         {
            BSONObj match ;     //old match
            BSONObj oldObj ;
            BSONObj newMatch ;
            BSONObj modifier ;   //new change obj
            const CHAR *fullname = NULL ;
            INT64 updateNum = 0 ;
            UINT32 logWriteMod = DMS_LOG_WRITE_MOD_INCREMENT ;
            BSONObj hint = BSON(""<<IXM_ID_KEY_NAME);
            rc = dpsRecord2Update( (CHAR *)recordHeader,
                                   &fullname,
                                   match,
                                   oldObj,
                                   newMatch,
                                   modifier, NULL, NULL, NULL, &logWriteMod ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
            /// possibly get a empty modifier
            if ( !modifier.isEmpty() )
            {
               rc = rtnUpdate( fullname, match, modifier,
                               hint, 0, eduCB, _dmsCB, _dpsCB, 1,
                               &updateNum, NULL, NULL, logWriteMod ) ;
            }
            if ( SDB_OK == rc )
            {
               if ( updateNum > 0 )
               {
                  if ( incCount )
                  {
                     _monDBCB->monOperationCountInc ( MON_UPDATE_REPL ) ;
                  }
               }
               else if ( _isReplSync )
               {
                  SDB_ASSERT( updateNum > 0, "Updated number must > 0" ) ;
               }
            }
            break ;
         }
         case LOG_TYPE_DATA_DELETE :
         {
            const CHAR *fullname = NULL ;
            BSONObj obj ;
            INT64 deleteNum = 0 ;
            rc = dpsRecord2Delete( (CHAR *)recordHeader,
                                   &fullname,
                                   obj ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
            else
            {
               BSONElement idEle = obj.getField( DMS_ID_KEY_NAME ) ;
               if ( idEle.eoo() )
               {
                  PD_LOG( PDWARNING, "replay: failed to parse "
                          "oid from bson:[%s]",obj.toString().c_str() ) ;
                  rc = SDB_INVALIDARG ;
                  goto error ;
               }
               {
                  BSONObjBuilder selectorBuilder ;
                  selectorBuilder.append( idEle ) ;
                  BSONObj selector = selectorBuilder.obj() ;
                  BSONObj hint = BSON(""<<IXM_ID_KEY_NAME) ;
                  rc = rtnDelete( fullname, selector, hint, 0, eduCB, _dmsCB,
                                  _dpsCB, 1, &deleteNum ) ;
               }
            }
            if ( SDB_OK == rc )
            {
               if ( deleteNum > 0 )
               {
                  if ( incCount )
                  {
                     _monDBCB->monOperationCountInc ( MON_DELETE_REPL ) ;
                  }
               }
               else if ( _isReplSync )
               {
                  SDB_ASSERT( deleteNum > 0, "Deleted number must > 0" ) ;
               }
            }
            break ;
         }
         case LOG_TYPE_DATA_POP:
         {
            const CHAR *fullName = NULL ;
            INT64 logicalID = 0 ;
            INT8 direction = 1 ;
            rc = dpsRecord2Pop( (CHAR *)recordHeader, &fullName,
                                logicalID, direction );
            if ( rc )
            {
               goto error ;
            }

            rc = rtnPopCommand( fullName, logicalID, eduCB, _dmsCB,
                                _dpsCB, 1, direction ) ;
            if ( SDB_INVALIDARG == rc )
            {
               PD_LOG( PDERROR, "Logical id[%lld] is invalid when pop from "
                       "collection[%s]", logicalID, fullName ) ;
               rc = SDB_OK ;
            }
            else if ( SDB_DMS_CS_NOTEXIST == rc )
            {
               PD_LOG( PDERROR, "Collection space not exist when pop from "
                       "collection[%s]", fullName ) ;
               rc = SDB_OK ;
            }
            else if ( SDB_DMS_NOTEXIST == rc )
            {
               PD_LOG( PDERROR, "Collection[%s] not exist when pop",
                       fullName ) ;
               rc = SDB_OK ;
            }
            break ;
         }
         case LOG_TYPE_CS_CRT :
         {
            const CHAR *cs = NULL ;
            utilCSUniqueID csUniqueID = UTIL_UNIQUEID_NULL ;
            INT32 pageSize = 0 ;
            INT32 lobPageSize = 0 ;
            INT32 type = 0 ;
            rc = dpsRecord2CSCrt( (CHAR *)recordHeader, &cs, csUniqueID,
                                  pageSize, lobPageSize, type ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }

            if ( type < DMS_STORAGE_NORMAL || type >= DMS_STORAGE_DUMMY )
            {
               PD_LOG( PDERROR, "Invalid cs type[%c] in replica log", type ) ;
               rc = SDB_SYS ;
               goto error ;
            }

            rc = rtnCreateCollectionSpaceCommand( cs, eduCB, _dmsCB, _dpsCB,
                                                  csUniqueID,
                                                  pageSize, lobPageSize,
                                                  (DMS_STORAGE_TYPE)type ) ;
            if ( SDB_DMS_CS_EXIST == rc )
            {
               PD_LOG( PDWARNING, "Collection space[%s] already exist when "
                       "create", cs ) ;

               rc = _dmsCB->changeUniqueID( cs, csUniqueID, BSONObj(),
                                            eduCB, _dpsCB, FALSE, FALSE ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR,
                          "Fail to change cs[%s] unique id, rc: %d", cs, rc ) ;
               }
            }
            break ;
         }
         case LOG_TYPE_CS_DELETE :
         {
            const CHAR *cs = NULL ;
            rc = dpsRecord2CSDel( (CHAR *)recordHeader,
                                  &cs ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
            while ( TRUE )
            {
               rc = rtnDropCollectionSpaceCommand( cs, eduCB, _dmsCB, _dpsCB,
                                                   TRUE ) ;
               if ( SDB_LOCK_FAILED == rc )
               {
                  ossSleep ( 100 ) ;
                  continue ;
               }
               break ;
            }
            if ( SDB_DMS_CS_NOTEXIST == rc )
            {
               PD_LOG( PDWARNING, "Collection space[%s] not exist when "
                       "drop", cs ) ;
               rc = SDB_OK ;
            }
            break ;
         }
         case LOG_TYPE_CL_CRT :
         {
            const CHAR *cl = NULL ;
            utilCLUniqueID clUniqueID = UTIL_UNIQUEID_NULL ;
            UINT32 attribute = 0 ;
            UINT8 compType = UTIL_COMPRESSOR_INVALID ;
            BSONObj extOptions ;
            string cs ;
            utilCSUniqueID csUniqID = UTIL_UNIQUEID_NULL ;

            rc = dpsRecord2CLCrt( (CHAR *)recordHeader, &cl, clUniqueID,
                                  attribute, compType, extOptions ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }

            cs = dmsGetCSNameFromFullName( cl ) ;
            csUniqID = utilGetCSUniqueID( clUniqueID ) ;

            rc = rtnCreateCollectionCommand( cl, attribute, eduCB, _dmsCB,
                                             _dpsCB, clUniqueID,
                                             (UTIL_COMPRESSOR_TYPE)compType,
                                             0, TRUE,
                                             ( extOptions.isEmpty() ?
                                               NULL : &extOptions ) ) ;
            if ( SDB_DMS_EXIST == rc )
            {
               PD_LOG( PDWARNING, "Collection [%s] already exist when "
                       "create", cl ) ;

               BSONArrayBuilder clArr ;
               clArr << BSON( FIELD_NAME_NAME <<
                              dmsGetCLShortNameFromFullName( cl ).c_str() <<
                              FIELD_NAME_UNIQUEID <<
                              (INT64)clUniqueID ) ;
               rc = _dmsCB->changeUniqueID( cs.c_str(), csUniqID, clArr.arr(),
                                            eduCB, _dpsCB, FALSE, FALSE ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Fail to change cl[%s] unique id, rc: %d",
                          cl, rc ) ;
                  goto error ;
               }

            }
            break ;
         }
         case LOG_TYPE_CL_DELETE :
         {
            const CHAR *cl = NULL ;
            rc = dpsRecord2CLDel( (CHAR *)recordHeader,
                                   &cl ) ;
            rc = rtnDropCollectionCommand( cl, eduCB, _dmsCB, _dpsCB ) ;
            if ( SDB_DMS_NOTEXIST == rc )
            {
               PD_LOG( PDWARNING, "Collection [%s] not exist when drop", cl ) ;
               rc = SDB_OK ;
            }
            break ;
         }
         case LOG_TYPE_IX_CRT :
         {
            /// rebuild the index can be very time-consuming.
            /// we create a sub thread to handle it.
            startIndexJob ( RTN_JOB_CREATE_INDEX, recordHeader,
                            _dpsCB, FALSE ) ;
            break ;
         }
         case LOG_TYPE_IX_DELETE :
         {
            startIndexJob ( RTN_JOB_DROP_INDEX, recordHeader,
                            _dpsCB, FALSE ) ;
            break ;
         }
         case LOG_TYPE_CL_RENAME :
         {
            const CHAR *cs = NULL ;
            const CHAR *oldCl = NULL ;
            const CHAR *newCl = NULL ;
            rc = dpsRecord2CLRename( (CHAR *)recordHeader,
                                      &cs, &oldCl, &newCl ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
            rc = rtnRenameCollectionCommand( cs, oldCl, newCl,
                                             eduCB, _dmsCB, _dpsCB ) ;
            if ( SDB_DMS_NOTEXIST == rc )
            {
               INT32 rcTmp = SDB_OK ;
               CHAR newCLFullName [ DMS_COLLECTION_FULL_NAME_SZ + 1 ] = { 0 } ;
               ossSnprintf( newCLFullName, DMS_COLLECTION_FULL_NAME_SZ,
                            "%s.%s", cs, newCl ) ;
               rcTmp = rtnTestCollectionCommand( newCLFullName, _dmsCB ) ;
               if ( SDB_OK == rcTmp )
               {
                  /// When old cl doesn't exist, but new cl has already exist,
                  /// we should ignore error
                  rc = SDB_OK ;
                  PD_LOG( PDWARNING, "Rename cl[%s.%s] to [%s.%s], old"
                          " cl not exist, new cl exist, ignore error.",
                          cs, oldCl, cs, newCl ) ;
               }
               else
               {
                  PD_LOG( PDERROR, "Failed to rename cl[%s.%s] to [%s.%s], "
                          "rc: %d, test new cl rc: %d",
                          cs, oldCl, cs, newCl, rc, rcTmp ) ;
               }
            }
            else if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to rename cl[%s.%s] to [%s.%s], rc: %d",
                       cs, oldCl, cs, newCl, rc ) ;
               goto error ;
            }
            break ;
         }
         case LOG_TYPE_CS_RENAME :
         {
            const CHAR *oldName = NULL ;
            const CHAR *newName = NULL ;
            rc = dpsRecord2CSRename( (const CHAR *)recordHeader,
                                     &oldName,
                                     &newName ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
#ifdef _WINDOWS
            while ( TRUE )
            {
               rc = rtnRenameCollectionSpaceCommand( oldName, newName,
                                                     eduCB, _dmsCB, _dpsCB ) ;
               if ( SDB_LOCK_FAILED == rc )
               {
                  ossSleep ( 100 ) ;
                  continue ;
               }
               break ;
            }
#else
            rc = rtnRenameCollectionSpaceCommand( oldName, newName,
                                                  eduCB, _dmsCB, _dpsCB ) ;
#endif
            if ( SDB_DMS_CS_NOTEXIST == rc )
            {
               INT32 rcTmp = SDB_OK ;
               rcTmp = rtnTestCollectionSpaceCommand( newName, _dmsCB ) ;
               if ( SDB_OK == rcTmp )
               {
                  /// When old cs doesn't exist, but new cs has already exist,
                  /// we should ignore error
                  rc = SDB_OK ;
                  PD_LOG( PDWARNING, "Replay log: rename cs[%s] to [%s], old"
                          " cs not exist, new cs exist, ignore error.",
                          oldName, newName ) ;
               }
               else
               {
                  PD_LOG( PDERROR, "Failed to rename cs[%s] to [%s], rc: %d, "
                          "test new cs rc: %d", oldName, newName, rc, rcTmp ) ;
               }
            }
            else if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to rename cs[%s] to [%s], rc: %d",
                       oldName, newName, rc ) ;
               goto error ;
            }
            break ;
         }
         case LOG_TYPE_CL_TRUNC :
         {
            const CHAR *clname = NULL ;
            rc = dpsRecord2CLTrunc( (const CHAR *)recordHeader, &clname ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
            rc = rtnTruncCollectionCommand( clname, eduCB, _dmsCB, _dpsCB ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to truncate collection[%s], rc: %d",
                       clname, rc ) ;
               goto error ;
            }
            break ;
         }
         case LOG_TYPE_INVALIDATE_CATA :
         {
            UINT8 type = 0 ;
            const CHAR *csName = NULL ;
            const CHAR *clFullName = NULL ;
            const CHAR *ixName = NULL ;

            rc = dpsRecord2InvalidCata( (CHAR *)recordHeader,
                                        type,
                                        &clFullName,
                                        &ixName ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }

            // Check if only contains name of collection space
            if ( NULL != clFullName &&
                 NULL == ossStrchr( clFullName, '.' ) )
            {
               csName = clFullName ;
               clFullName = NULL ;
            }

            if ( OSS_BIT_TEST( type, DPS_LOG_INVALIDCATA_TYPE_STAT ) )
            {
               rtnAnalyzeParam param ;
               param._mode = SDB_ANALYZE_MODE_CLEAR ;
               param._needCheck = FALSE ;

               if ( csName && 0 == ossStrcmp( csName, "SYS" ) )
               {
                  // Reload all statistics
                  csName = NULL ;
               }

               rtnAnalyze( csName, clFullName, ixName, param,
                           eduCB, _dmsCB, NULL, NULL ) ;
            }

            if ( OSS_BIT_TEST( type, DPS_LOG_INVALIDCATA_TYPE_CATA ) )
            {
               catAgent *pCatAgent = NULL ;

               /// when sdbrestore, the shardCB is NULL
               if ( sdbGetShardCB() &&
                    NULL != ( pCatAgent = sdbGetShardCB()->getCataAgent() ) )
               {
                  pCatAgent->lock_w() ;
                  if ( NULL != clFullName )
                  {
                     pCatAgent->clear( clFullName ) ;
                  }
                  else if ( NULL != csName )
                  {
                     pCatAgent->clearBySpaceName( csName, NULL, NULL ) ;
                  }
                  else
                  {
                     PD_LOG( PDERROR, "Failed to find fullname in record" ) ;
                     pCatAgent->release_w() ;
                     rc = SDB_SYS ;
                     goto error ;
                  }
                  pCatAgent->release_w() ;
               }
            }

            if ( OSS_BIT_TEST( type, DPS_LOG_INVALIDCATA_TYPE_PLAN ) )
            {
               SDB_RTNCB * rtnCB = sdbGetRTNCB() ;

               if ( NULL != clFullName )
               {
                  rtnCB->getAPM()->invalidateCLPlans( clFullName ) ;
               }
               else if ( NULL != csName )
               {
                  rtnCB->getAPM()->invalidateSUPlans( csName ) ;
               }
               else
               {
                  rtnCB->getAPM()->invalidateAllPlans() ;
               }
            }

            rc = SDB_OK ;
            break ;
         }
         case LOG_TYPE_LOB_WRITE :
         {
            const CHAR *fullName = NULL ;
            const bson::OID *oid = NULL ;
            UINT32 sequence = 0 ;
            UINT32 offset = 0 ;
            UINT32 len = 0 ;
            UINT32 hash = 0 ;
            const CHAR *data = NULL ;
            DMS_LOB_PAGEID page = DMS_LOB_INVALID_PAGEID ;
            rc = dpsRecord2LobW( (CHAR *)recordHeader,
                                  &fullName, &oid,
                                  sequence, offset,
                                  len, hash, &data, page ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }

            rc = rtnWriteLob( fullName, *oid, sequence,
                              offset, len, data, eduCB,
                              1, _dpsCB ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to write lob:%d", rc ) ;
               if ( SDB_LOB_SEQUENCE_EXISTS == rc )
               {
                  rc = SDB_OK ;
               }
               else
               {
                  goto error ;
               }
            }
            break ;
         }
         case LOG_TYPE_LOB_REMOVE :
         {
            const CHAR *fullName = NULL ;
            const bson::OID *oid = NULL ;
            UINT32 sequence = 0 ;
            UINT32 offset = 0 ;
            UINT32 len = 0 ;
            UINT32 hash = 0 ;
            const CHAR *data = NULL ;
            DMS_LOB_PAGEID page = DMS_LOB_INVALID_PAGEID ;
            rc = dpsRecord2LobRm( (CHAR *)recordHeader,
                                  &fullName, &oid,
                                  sequence, offset,
                                  len, hash, &data, page ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }

            rc = rtnRemoveLobPiece( fullName, *oid,
                                    sequence, eduCB,
                                    1, _dpsCB ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to remove lob:%d", rc ) ;
               if ( SDB_LOB_SEQUENCE_NOT_EXIST == rc )
               {
                  rc = SDB_OK ;
               }
               else
               {
                  goto error ;
               }
            }
            break ;
         }
         case LOG_TYPE_LOB_UPDATE :
         {
            const CHAR *fullName = NULL ;
            const bson::OID *oid = NULL ;
            UINT32 sequence = 0 ;
            UINT32 offset = 0 ;
            UINT32 len = 0 ;
            UINT32 hash = 0 ;
            const CHAR *data = NULL ;
            DMS_LOB_PAGEID page = DMS_LOB_INVALID_PAGEID ;
            UINT32 oldLen = 0 ;
            const CHAR *oldData = NULL ;
            rc = dpsRecord2LobU( (CHAR *)recordHeader,
                                  &fullName, &oid,
                                  sequence, offset,
                                  len, hash, &data,
                                  oldLen, &oldData, page ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }

            rc = rtnUpdateLob( fullName, *oid, sequence,
                               offset, len, data, eduCB,
                               1, _dpsCB ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to update lob:%d", rc ) ;
               goto error ;
            }

            break ;
         }
         case LOG_TYPE_ALTER :
         {
            const CHAR * objectName = NULL ;
            RTN_ALTER_OBJECT_TYPE objectType = RTN_ALTER_INVALID_OBJECT ;
            BSONObj alterObject ;

            rc = dpsRecord2Alter( (CHAR *)recordHeader, &objectName,
                                  (INT32 &)objectType, alterObject ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }

            while ( TRUE )
            {
               rc = rtnAlter( objectName, objectType, alterObject,
                              eduCB, _dpsCB ) ;
               if ( SDB_LOCK_FAILED == rc )
               {
                  ossSleep( 100 ) ;
                  continue ;
               }
               break ;
            }
            PD_RC_CHECK( rc, PDERROR, "Failed to alter object, rc: %d", rc ) ;

            break ;
         }
         case LOG_TYPE_ADDUNIQUEID :
         {
            const CHAR * csname = NULL ;
            utilCSUniqueID csUniqueID = UTIL_UNIQUEID_NULL ;
            BSONObj clInfoObj ;

            rc = dpsRecord2AddUniqueID( (CHAR *)recordHeader, &csname,
                                         csUniqueID, clInfoObj ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }

            while ( TRUE )
            {
               rc = rtnChangeUniqueID( csname, csUniqueID, clInfoObj,
                                       eduCB, _dmsCB, _dpsCB ) ;
               if ( SDB_LOCK_FAILED == rc )
               {
                  ossSleep( 100 ) ;
                  continue ;
               }
               break ;
            }
            PD_RC_CHECK( rc, PDERROR, "Failed to add unique id, rc: %d", rc ) ;

            break ;
         }
         case LOG_TYPE_DUMMY :
         {
            rc = SDB_OK ;
            break ;
         }
         case LOG_TYPE_TS_COMMIT :
         {
            rc = SDB_OK ;
            break ;
         }
         default :
         {
            rc = SDB_CLS_REPLAY_LOG_FAILED ;
            PD_LOG( PDWARNING, "unexpected log type: %d",
                    recordHeader->_type ) ;
            break ;
         }
      }
      }
      catch ( std::exception &e )
      {
         rc = SDB_CLS_REPLAY_LOG_FAILED ;
         PD_LOG( PDERROR, "unexpected exception: %s", e.what() ) ;
         goto error ;
      }

   done:
      eduCB->resetLsn() ;
      if ( SDB_OK != rc )
      {
         dpsLogRecord record ;
         CHAR tmpBuff[4096] = {0} ;
         INT32 rcTmp = record.load( (const CHAR*)recordHeader ) ;
         if ( SDB_OK == rcTmp )
         {
            record.dump( tmpBuff, sizeof(tmpBuff)-1, DPS_DMP_OPT_FORMATTED ) ;
         }
         PD_LOG( PDERROR, "sync: replay log [type:%d, lsn:%lld, data: %s] "
                 "failed, rc: %d", recordHeader->_type, recordHeader->_lsn,
                 tmpBuff, rc ) ;
      }
      PD_TRACE_EXITRC ( SDB__CLSREP_REPLAY, rc );
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsReplayer::rollbackTrans( const dpsLogRecordHeader *recordHeader,
                                      _pmdEDUCB * eduCB,
                                      MAP_TRANS_PENDING_OBJ &mapPendingObj )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( NULL != recordHeader, "head should not be NULL" ) ;
      MAP_TRANS_PENDING_OBJ::iterator it ;
      INT64 contextID = -1 ;

      if ( !_dpsCB )
      {
         eduCB->insertLsn( recordHeader->_lsn, TRUE ) ;
      }

      if ( LOG_TYPE_DATA_INSERT == recordHeader->_type )
      {
         BSONObj obj ;
         const CHAR *fullname = NULL ;
         BSONElement idEle ;

         rc = dpsRecord2Insert( (const CHAR *)recordHeader,
                                &fullname, obj ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }

         idEle = obj.getField( DMS_ID_KEY_NAME ) ;
         if ( idEle.eoo() )
         {
            PD_LOG( PDWARNING, "replay: failed to parse"
                    " oid from bson:[%s]",obj.toString().c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         {
            BSONObjBuilder selectorBuilder ;
            selectorBuilder.append( idEle ) ;
            BSONObj selector = selectorBuilder.obj() ;
            BSONObj hint = BSON(""<<IXM_ID_KEY_NAME) ;

            it = mapPendingObj.find( selector ) ;
            if ( it != mapPendingObj.end() )
            {
               if ( LOG_TYPE_DATA_DELETE != it->second._opType )
               {
                  selector = it->second._obj ;
               }
               else
               {
                  selector = BSONObj() ;
               }
               mapPendingObj.erase( it ) ;
            }
            /// delete disk
            if ( !selector.isEmpty() )
            {
               rc = rtnDelete( fullname, selector, hint, 0, eduCB, _dmsCB,
                               _dpsCB, 1 ) ;
            }
         }
      }
      else if ( LOG_TYPE_DATA_UPDATE == recordHeader->_type )
      {
         const CHAR *fullname = NULL ;
         BSONObj oldMatch ;
         BSONObj modifierObj ;  //old modifier
         BSONObj newMatch ;     //new matcher
         BSONObj newObj ;

         dpsTransPendingKey tmpKey ;
         dpsTransPendingValue tmpVal ;
         BOOLEAN isTmpKeyExsit = FALSE ;
         UINT32 logWriteMod = DMS_LOG_WRITE_MOD_INCREMENT ;

         BSONObj hint = BSON(""<<IXM_ID_KEY_NAME) ;
         rc = dpsRecord2Update( (const CHAR *)recordHeader,
                                 &fullname, oldMatch, modifierObj,
                                 newMatch, newObj, NULL, NULL, NULL,
                                 &logWriteMod ) ;
         if ( rc )
         {
            goto error ;
         }
         if ( !modifierObj.isEmpty() )
         {
            it = mapPendingObj.find( newMatch ) ;
            if ( it != mapPendingObj.end() )
            {
               tmpVal = it->second ;
               isTmpKeyExsit = TRUE ;

               mthModifier modifier ;
               rc = modifier.loadPattern( modifierObj, NULL, TRUE, NULL, FALSE,
                                          logWriteMod ) ;
               PD_RC_CHECK( rc, PDERROR, "Load modify pattern(%s) failed, "
                            "rc: %d", modifierObj.toString().c_str(), rc ) ;

               rc = modifier.modify( it->first._obj, tmpKey._obj ) ;
               PD_RC_CHECK( rc, PDERROR, "Modify obj(%s) by(%s) failed, "
                            "rc: %d", it->first._obj.toString().c_str(),
                            modifierObj.toString().c_str(), rc ) ;

               mapPendingObj.erase( it ) ;

               if ( LOG_TYPE_DATA_DELETE != tmpVal._opType )
               {
                  newMatch = tmpVal._obj ;
                  modifierObj = BSON( "$replace" << tmpKey._obj ) ;
                  logWriteMod = DMS_LOG_WRITE_MOD_INCREMENT ;
               }
            }
            else
            {
               tmpVal._opType = LOG_TYPE_DATA_UPDATE ;
               tmpVal._obj = newMatch.getOwned() ;
            }

            if ( LOG_TYPE_DATA_DELETE == tmpVal._opType )
            {
               rc = rtnInsert( fullname, tmpKey._obj, 1, 0, eduCB,
                               _dmsCB, _dpsCB, 1 ) ;
            }
            else
            {
               rc = rtnUpdate( fullname, newMatch, modifierObj,
                               hint, 0, eduCB, _dmsCB, _dpsCB, 1,
                               NULL, NULL, NULL, logWriteMod ) ;
            }
            if ( SDB_IXM_DUP_KEY == rc )
            {
               if ( !isTmpKeyExsit )
               {
                  BSONObj curObj ;
                  rtnContextBuf buffObj ;
                  rc = rtnQuery( fullname, BSONObj(), newMatch, BSONObj(),
                                 hint, 0, eduCB, 0, 1, _dmsCB, sdbGetRTNCB(),
                                 contextID ) ;
                  PD_RC_CHECK( rc, PDERROR, "Query obj(%s) from %s failed, "
                               "rc: %d", newMatch.toString().c_str(),
                               fullname ) ;

                  rc = rtnGetMore( contextID, 1, buffObj, eduCB,
                                   sdbGetRTNCB() ) ;
                  PD_RC_CHECK( rc, PDERROR, "Query obj(%s) from %s failed, "
                               "rc: %d", newMatch.toString().c_str(),
                               fullname, rc ) ;

                  /// get obj
                  rc = buffObj.nextObj( curObj ) ;
                  PD_RC_CHECK( rc, PDERROR, "Query obj(%s) from %s failed, "
                               "rc: %d", newMatch.toString().c_str(),
                               fullname, rc ) ;

                  mthModifier modifier ;
                  rc = modifier.loadPattern( modifierObj, NULL, TRUE, NULL,
                                             FALSE, logWriteMod ) ;
                  PD_RC_CHECK( rc, PDERROR, "Load modify pattern(%s) failed, "
                               "rc: %d", modifierObj.toString().c_str(), rc ) ;

                  rc = modifier.modify( curObj, tmpKey._obj ) ;
                  PD_RC_CHECK( rc, PDERROR, "Modify obj(%s) by(%s) failed, "
                               "rc: %d", curObj.toString().c_str(),
                               modifierObj.toString().c_str(), rc ) ;
               }

               mapPendingObj[ tmpKey ] = tmpVal ;
               rc = SDB_OK ;
            }
         }
      }
      else if ( LOG_TYPE_DATA_DELETE == recordHeader->_type )
      {
         BSONObj obj ;
         const CHAR *fullname = NULL ;
         rc = dpsRecord2Delete( (const CHAR *)recordHeader,
                                &fullname,
                                obj ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }

         rc = rtnInsert( fullname, obj, 1, 0, eduCB, _dmsCB, _dpsCB, 1 ) ;
         if ( SDB_IXM_DUP_KEY == rc )
         {
            dpsTransPendingKey key( obj ) ;
            dpsTransPendingValue val( key._obj, LOG_TYPE_DATA_DELETE ) ;
            mapPendingObj[ key ] = val ;
            rc = SDB_OK ;
         }
      }
      else
      {
         rc = rollback( recordHeader, eduCB ) ;
         if ( rc )
         {
            goto error ;
         }
      }

   done:
      if ( -1 != contextID )
      {
         sdbGetRTNCB()->contextDelete( contextID, eduCB ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSREP_ROLBCK, "_clsReplayer::rollback" )
   INT32 _clsReplayer::rollback( const dpsLogRecordHeader *recordHeader,
                                 _pmdEDUCB *eduCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSREP_ROLBCK );
      SDB_ASSERT( NULL != recordHeader, "head should not be NULL" ) ;

      if ( !_dpsCB )
      {
         eduCB->insertLsn( recordHeader->_lsn, TRUE ) ;
      }

      try
      {
         switch ( recordHeader->_type )
         {
         case LOG_TYPE_DATA_INSERT :
         {
            BSONObj obj ;
            const CHAR *fullname = NULL ;
            rc = dpsRecord2Insert( (const CHAR *)recordHeader,
                                   &fullname, obj ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }

            {
               BSONElement idEle = obj.getField( DMS_ID_KEY_NAME ) ;
               if ( idEle.eoo() )
               {
                  PD_LOG( PDWARNING, "replay: failed to parse"
                          " oid from bson:[%s]",obj.toString().c_str() ) ;
                  rc = SDB_INVALIDARG ;
                  goto error ;
               }

               {
                  BSONObjBuilder selectorBuilder ;
                  selectorBuilder.append( idEle ) ;
                  BSONObj selector = selectorBuilder.obj() ;
                  BSONObj hint = BSON(""<<IXM_ID_KEY_NAME) ;
                  rc = rtnDelete( fullname, selector, hint, 0, eduCB, _dmsCB,
                                  _dpsCB, 1 ) ;
               }
            }
            break ;
         }
         case LOG_TYPE_DATA_UPDATE :
         {
            const CHAR *fullname = NULL ;
            BSONObj oldMatch ;
            BSONObj modifier ;  //old modifier
            BSONObj newMatch ;     //new matcher
            BSONObj newObj ;
            UINT32 logWriteMod = DMS_LOG_WRITE_MOD_INCREMENT ;
            BSONObj hint = BSON(""<<IXM_ID_KEY_NAME) ;
            rc = dpsRecord2Update( (const CHAR *)recordHeader,
                                    &fullname,
                                    oldMatch,
                                    modifier,
                                    newMatch,
                                    newObj, NULL, NULL, NULL, &logWriteMod ) ;
            if ( !modifier.isEmpty() )
            {
               rc = rtnUpdate( fullname, newMatch, modifier,
                               hint, 0, eduCB, _dmsCB, _dpsCB, 1, NULL, NULL,
                               NULL, logWriteMod ) ;
            }
            break ;
         }
         case LOG_TYPE_DATA_DELETE :
         {
            BSONObj obj ;
            const CHAR *fullname = NULL ;
            rc = dpsRecord2Delete( (const CHAR *)recordHeader,
                                   &fullname,
                                   obj ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }

            rc = rtnInsert( fullname, obj, 1, 0, eduCB, _dmsCB, _dpsCB, 1 ) ;
            if ( SDB_IXM_DUP_KEY == rc )
            {
               PD_LOG( PDINFO, "Record[%s] exist when rollback delete",
                       obj.toString().c_str() ) ;
#ifdef _DEBUG
               SDB_ASSERT ( (rc == SDB_OK),
                            "Rollback should not hit dup key" );
#endif
               rc = SDB_OK ;
            }
            break ;
         }
         case LOG_TYPE_CS_CRT :
         {
            const CHAR *cs = NULL ;
            utilCSUniqueID csUniqueID = UTIL_UNIQUEID_NULL ;
            INT32 pageSize = 0 ;
            INT32 lobPageSize = 0 ;
            INT32 type = 0 ;
            rc = dpsRecord2CSCrt( (const CHAR *)recordHeader, &cs, csUniqueID,
                                  pageSize, lobPageSize, type ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
            rc = rtnDropCollectionSpaceCommand( cs, eduCB, _dmsCB,
                                                _dpsCB, TRUE ) ;
            if ( SDB_DMS_CS_NOTEXIST == rc )
            {
               rc = SDB_OK ;
               PD_LOG( PDWARNING, "Collection space[%s] not exist when "
                       "rollback create cs", cs ) ;
            }
            break ;
         }
         case LOG_TYPE_CS_DELETE :
         {
            /// cant not rollback, return fail.
            rc = SDB_CLS_REPLAY_LOG_FAILED ;
            goto error ;
         }
         case LOG_TYPE_CL_CRT :
         {
            const CHAR *fullname = NULL ;
            utilCLUniqueID clUniqueID = UTIL_UNIQUEID_NULL ;
            UINT32 attribute = 0 ;
            UINT8 compType = UTIL_COMPRESSOR_INVALID ;
            BSONObj extOptions ;
            rc = dpsRecord2CLCrt( (const CHAR *)recordHeader,
                                  &fullname, clUniqueID,
                                  attribute, compType, extOptions ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
            rc = rtnDropCollectionCommand( fullname, eduCB, _dmsCB, _dpsCB ) ;
            if ( SDB_DMS_NOTEXIST == rc )
            {
               rc = SDB_OK ;
               PD_LOG( PDWARNING, "Collection[%s] not exist when rollback "
                       "create cl", fullname ) ;
            }
            break ;
         }
         case LOG_TYPE_CL_DELETE :
         {
            /// cant not rollback, return fail.
            rc = SDB_CLS_REPLAY_LOG_FAILED ;
            goto error ;
         }
         case LOG_TYPE_IX_CRT :
         {
            startIndexJob ( RTN_JOB_DROP_INDEX, recordHeader,
                            _dpsCB, TRUE ) ;
            break ;
         }
         case LOG_TYPE_IX_DELETE :
         {
            startIndexJob ( RTN_JOB_CREATE_INDEX, recordHeader,
                            _dpsCB, TRUE ) ;
            break ;
         }
         case LOG_TYPE_CL_RENAME :
         {
            const CHAR *cs = NULL ;
            const CHAR *oldCl = NULL ;
            const CHAR *newCl = NULL ;
            rc = dpsRecord2CLRename( (CHAR *)recordHeader,
                                      &cs, &oldCl, &newCl ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
            rc = rtnRenameCollectionCommand( cs, newCl, oldCl,
                                             eduCB, _dmsCB, _dpsCB ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to rename cs[%s] cl %s to %s, rc: %d",
                       cs, oldCl, newCl, rc ) ;
               goto error ;
            }
            break ;
         }
         case LOG_TYPE_CS_RENAME :
         {
            const CHAR *oldName = NULL ;
            const CHAR *newName = NULL ;
            rc = dpsRecord2CSRename( (const CHAR *)recordHeader,
                                     &oldName,
                                     &newName ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
            rc = rtnRenameCollectionSpaceCommand( newName, oldName,
                                                  eduCB, _dmsCB, _dpsCB ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to rename %s to %s, rc: %d",
                       oldName, newName, rc ) ;
               goto error ;
            }
            break ;
         }
         case LOG_TYPE_CL_TRUNC :
         {
            /// cant not rollback, return fail.
            rc = SDB_CLS_REPLAY_LOG_FAILED ;
            goto error ;
         }
         case LOG_TYPE_ALTER :
         {
            /// cant not rollback, return fail.
            rc = SDB_CLS_REPLAY_LOG_FAILED ;
            goto error ;
         }
         case LOG_TYPE_ADDUNIQUEID :
         {
            const CHAR * csname = NULL ;
            utilCSUniqueID csUniqueID = UTIL_UNIQUEID_NULL ;
            BSONObj clInfoObj, emptyObj ;

            rc = dpsRecord2AddUniqueID( (CHAR *)recordHeader, &csname,
                                         csUniqueID, clInfoObj ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }

            while ( TRUE )
            {
               rc = rtnChangeUniqueID( csname, UTIL_UNIQUEID_NULL,
                                       utilSetUniqueID( clInfoObj ),
                                       eduCB, _dmsCB, _dpsCB, FALSE ) ;
               if ( SDB_LOCK_FAILED == rc )
               {
                  ossSleep( 100 ) ;
                  continue ;
               }
               break ;
            }
            PD_RC_CHECK( rc, PDERROR, "Failed to add unique id, rc: %d", rc ) ;

            break ;
         }
         case LOG_TYPE_TS_COMMIT :
         case LOG_TYPE_DUMMY :
         case LOG_TYPE_INVALIDATE_CATA :
         {
            rc = SDB_OK ;
            break ;
         }
         case LOG_TYPE_LOB_WRITE :
         {
            const CHAR *fullName = NULL ;
            const bson::OID *oid = NULL ;
            UINT32 sequence = 0 ;
            UINT32 offset = 0 ;
            UINT32 len = 0 ;
            UINT32 hash = 0 ;
            const CHAR *data = NULL ;
            DMS_LOB_PAGEID page = DMS_LOB_INVALID_PAGEID ;
            rc = dpsRecord2LobW( (CHAR *)recordHeader,
                                  &fullName, &oid,
                                  sequence, offset,
                                  len, hash, &data, page ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }

            rc = rtnRemoveLobPiece( fullName, *oid, sequence,
                                    eduCB, 1, NULL ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to remove lob:%d", rc ) ;
               if ( SDB_LOB_SEQUENCE_NOT_EXIST == rc )
               {
                  rc = SDB_OK ;
               }
               else
               {
                  goto error ;
               }
            }
            break ;
         }
         case LOG_TYPE_LOB_UPDATE :
         {
            const CHAR *fullName = NULL ;
            const bson::OID *oid = NULL ;
            UINT32 sequence = 0 ;
            UINT32 offset = 0 ;
            UINT32 len = 0 ;
            UINT32 hash = 0 ;
            const CHAR *data = NULL ;
            DMS_LOB_PAGEID page = DMS_LOB_INVALID_PAGEID ;
            UINT32 oldLen = 0 ;
            const CHAR *oldData = NULL ;
            rc = dpsRecord2LobU( (CHAR *)recordHeader,
                                  &fullName, &oid,
                                  sequence, offset,
                                  len, hash, &data,
                                  oldLen, &oldData, page ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }

            rc = rtnUpdateLob( fullName, *oid, sequence,
                               offset, oldLen, oldData, eduCB,
                               1, NULL ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to update lob:%d", rc ) ;
               goto error ;
            }
            break ;
         }
         case LOG_TYPE_LOB_REMOVE :
         {
            const CHAR *fullName = NULL ;
            const bson::OID *oid = NULL ;
            UINT32 sequence = 0 ;
            UINT32 offset = 0 ;
            UINT32 len = 0 ;
            UINT32 hash = 0 ;
            const CHAR *data = NULL ;
            DMS_LOB_PAGEID page = DMS_LOB_INVALID_PAGEID ;
            rc = dpsRecord2LobRm( (CHAR *)recordHeader,
                                  &fullName, &oid,
                                  sequence, offset,
                                  len, hash, &data, page ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }

            rc = rtnWriteLob( fullName, *oid, sequence,
                              offset, len, data, eduCB,
                              1, NULL ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to write lob:%d", rc ) ;
               if ( SDB_LOB_SEQUENCE_EXISTS == rc )
               {
                  rc = SDB_OK ;
               }
               else
               {
                  goto error ;
               }
            }
            break ;
         }
         case LOG_TYPE_DATA_POP:
         {
            // Pop is not able to rollback, return error.
            rc = SDB_CLS_REPLAY_LOG_FAILED ;
            goto error ;
         }
         default :
         {
            rc = SDB_CLS_REPLAY_LOG_FAILED ;
            PD_LOG( PDWARNING, "unexpected log type: %d",
                    recordHeader->_type ) ;
            break ;
         }
         }
      }
      catch ( std::exception &e )
      {
         /// reuse error code.
         rc = SDB_CLS_REPLAY_LOG_FAILED ;
         PD_LOG( PDERROR, "unexpected exception: %s", e.what() ) ;
         goto error ;
      }

   done:
      eduCB->resetLsn() ;
      if ( SDB_OK != rc )
      {
         dpsLogRecord record ;
         CHAR tmpBuff[4096] = {0} ;
         INT32 rcTmp = record.load( (const CHAR*)recordHeader ) ;
         if ( SDB_OK == rcTmp )
         {
            record.dump( tmpBuff, sizeof(tmpBuff)-1, DPS_DMP_OPT_FORMATTED ) ;
         }
         PD_LOG( PDERROR, "sync: rollback log [type:%d, lsn:%lld, data: %s] "
                 "failed, rc: %d", recordHeader->_type, recordHeader->_lsn,
                 tmpBuff, rc ) ;
      }
      PD_TRACE_EXITRC ( SDB__CLSREP_ROLBCK, rc );
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsReplayer::replayCrtCS( const CHAR *cs, utilCSUniqueID csUniqueID,
                                    INT32 pageSize,
                                    INT32 lobPageSize, DMS_STORAGE_TYPE type,
                                    _pmdEDUCB *eduCB )
   {
      SDB_ASSERT( NULL != cs, "cs should not be NULL" ) ;
      INT32 rc = SDB_OK ;

      rc = rtnTestCollectionSpaceCommand( cs, _dmsCB, &csUniqueID ) ;

      if ( SDB_DMS_CS_REMAIN == rc )
      {
         rc = rtnDropCollectionSpaceCommand( cs, eduCB,
                                             _dmsCB, _dpsCB ) ;
         if ( SDB_OK == rc )
         {
            rc = SDB_DMS_CS_NOTEXIST ;
         }
         if ( SDB_DMS_CS_NOTEXIST != rc )
         {
            PD_LOG( PDERROR,
                    "Drop cs[%s] before create cs failed, rc: %d",
                    cs, rc ) ;
         }
      }

      if ( SDB_DMS_CS_NOTEXIST == rc )
      {
         rc = rtnCreateCollectionSpaceCommand( cs, eduCB, _dmsCB,
                                               _dpsCB, csUniqueID, pageSize,
                                               lobPageSize, type, TRUE ) ;
      }
      return rc ;
   }

   INT32 _clsReplayer::replayCrtCollection( const CHAR *collection,
                                            utilCLUniqueID clUniqueID,
                                            UINT32 attributes,
                                            _pmdEDUCB *eduCB,
                                            UTIL_COMPRESSOR_TYPE compType,
                                            const BSONObj *extOptions )
   {
      SDB_ASSERT( NULL != collection, "collection should not be NULL" ) ;
      INT32 rc = SDB_OK ;

      rc = rtnTestCollectionCommand( collection, _dmsCB, &clUniqueID ) ;

      if ( SDB_DMS_REMAIN == rc )
      {
         rc = rtnDropCollectionCommand( collection, eduCB, _dmsCB, _dpsCB ) ;
         if ( SDB_OK == rc )
         {
            rc = SDB_DMS_NOTEXIST ;
         }
         if ( SDB_DMS_NOTEXIST != rc )
         {
            PD_LOG( PDERROR,
                    "Drop cl[%s] before create cl failed, rc: %d",
                    collection, rc ) ;
         }
      }

      if ( SDB_DMS_NOTEXIST == rc )
      {
         rc = rtnCreateCollectionCommand( collection, attributes, eduCB,
                                          _dmsCB, _dpsCB, clUniqueID, compType,
                                          0, TRUE, extOptions ) ;
      }

      return rc ;
   }

   INT32 _clsReplayer::replayIXCrt( const CHAR *collection,
                                    BSONObj &index,
                                    _pmdEDUCB *eduCB )
   {
      SDB_ASSERT( NULL != collection, "collection should not be NULL" ) ;
      return rtnCreateIndexCommand( collection, index, eduCB,
                                    _dmsCB, _dpsCB, TRUE ) ;
   }

   INT32 _clsReplayer::replayInsert( const CHAR *collection,
                                     BSONObj &obj,
                                     _pmdEDUCB *eduCB )
   {
      SDB_ASSERT( NULL != collection, "collection should not be NULL" ) ;
      return rtnReplayInsert( collection, obj, FLG_INSERT_CONTONDUP, eduCB,
                              _dmsCB, _dpsCB ) ;
   }

   INT32 _clsReplayer::replayWriteLob( const CHAR *fullName,
                                       const bson::OID &oid,
                                       UINT32 sequence,
                                       UINT32 offset,
                                       UINT32 len,
                                       const CHAR *data,
                                       _pmdEDUCB *eduCB )
   {
      return rtnWriteLob( fullName, oid, sequence,
                          offset, len, data, eduCB,
                          1, _dpsCB ) ;
   }

   clsCLParallaInfo* _clsReplayer::_getOrCreateInfo( utilCLUniqueID clUID )
   {
      try
      {
         clsCLParallaInfo &info = _mapParallaInfo[ clUID ] ;
         return &info ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
      }
      return NULL ;
   }

   void _clsReplayer::_clearParallaInfo()
   {
      _mapParallaInfo.clear() ;
   }

   static BOOLEAN _isTextIdx( const BSONObj &index )
   {
      BSONObj idxDef = index.getObjectField( IXM_FIELD_NAME_KEY ) ;
      BSONElement ele = idxDef.firstElement() ;

      return ( 0 == ossStrcmp( ele.valuestrsafe(), IXM_TEXT_KEY_TYPE ) ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_STARTINXJOB, "startIndexJob" )
   INT32 startIndexJob ( RTN_JOB_TYPE type,
                         const dpsLogRecordHeader *recordHeader,
                         _dpsLogWrapper *dpsCB,
                         BOOLEAN isRollBack )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_STARTINXJOB );
      const CHAR *fullname = NULL ;
      BSONObj index ;
      rtnIndexJob *indexJob = NULL ;
      clsCatalogSet *pCatSet = NULL ;
      BOOLEAN useSync = FALSE ;

      if ( LOG_TYPE_IX_CRT != recordHeader->_type &&
           LOG_TYPE_IX_DELETE != recordHeader->_type )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else if ( LOG_TYPE_IX_CRT == recordHeader->_type)
      {
         rc = dpsRecord2IXCrt( (CHAR *)recordHeader,
                               &fullname,
                               index ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }
      else
      {
         rc = dpsRecord2IXDel( (CHAR *)recordHeader,
                               &fullname,
                               index ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }

      if ( pmdGetKRCB()->isRestore() || _isTextIdx( index ) )
      {
         useSync = TRUE ;
      }
      else
      {
         sdbGetShardCB()->getAndLockCataSet( fullname, &pCatSet, TRUE ) ;
         if ( pCatSet && CLS_REPLSET_MAX_NODE_SIZE == pCatSet->getW() )
         {
            useSync = TRUE ;
         }
         sdbGetShardCB()->unlockCataSet( pCatSet ) ;
      }

      indexJob = SDB_OSS_NEW rtnIndexJob( type, fullname,
                                          index, dpsCB,
                                          recordHeader->_lsn,
                                          isRollBack ) ;
      if ( NULL == indexJob )
      {
         PD_LOG ( PDERROR, "Failed to alloc memory for indexJob" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      rc = indexJob->init () ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Index job[%s] init failed, rc = %d",
                  indexJob->name(), rc ) ;
         goto error ;
      }

      /// When is $id or useSync
      if ( useSync ||
           0 == ossStrcmp( indexJob->getIndexName(), IXM_ID_KEY_NAME ) )
      {
         indexJob->doit() ;
         SDB_OSS_DEL indexJob ;
         indexJob = NULL ;
      }
      else
      {
         // if use RTN_JOB_MUTEX_STOP_RET, when create index have complete,
         // drop index should not drop really, so it's error, need to use
         // RTN_JOB_MUTEX_STOP_CONT
         rc = rtnGetJobMgr()->startJob( indexJob, RTN_JOB_MUTEX_STOP_CONT,
                                        NULL ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_STARTINXJOB, rc );
      return rc ;
   error:
      goto done ;
   }

}
