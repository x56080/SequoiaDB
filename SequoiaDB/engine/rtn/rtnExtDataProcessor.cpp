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

   Source File Name = rtnExtDataProcessor.hpp

   Descriptive Name = External data processor for rtn.

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS storage unit and its methods.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          14/04/2017  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#include "rtnExtDataProcessor.hpp"
#include "rtn.hpp"
#include "rtnTrace.hpp"
#include "dmsStorageDataCapped.hpp"

// Currently we set the size limit of capped collection to 30GB. This may change
// in the future.
#define RTN_CAPPED_CL_MAXSIZE       ( 30 * 1024 * 1024 * 1024LL )
#define RTN_CAPPED_CL_MAXRECNUM     0
#define RTN_FIELD_NAME_RID          "_rid"
#define RTN_FIELD_NAME_RID_NEW      "_ridNew"
#define RTN_FIELD_NAME_SOURCE       "_source"

namespace engine
{
   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR__RTNEXTDATAPROCESSOR, "_rtnExtDataProcessor::_rtnExtDataProcessor" )
   _rtnExtDataProcessor::_rtnExtDataProcessor()
   {
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSOR__RTNEXTDATAPROCESSOR ) ;
      _stat = RTN_EXT_PROCESSOR_INVALID ;
      _su = NULL ;
      _id = RTN_EXT_PROCESSOR_INVALID_ID ;
      ossMemset( _cappedCSName, 0, DMS_COLLECTION_SPACE_NAME_SZ + 1 ) ;
      ossMemset( _cappedCLName, 0, DMS_COLLECTION_NAME_SZ + 1 ) ;
      _needUpdateLSN = FALSE ;
      _needOprRec = FALSE ;
      _freeSpace = 0 ;
      PD_TRACE_EXIT( SDB__RTNEXTDATAPROCESSOR__RTNEXTDATAPROCESSOR ) ;
   }

   _rtnExtDataProcessor::~_rtnExtDataProcessor()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR_INIT, "_rtnExtDataProcessor::init" )
   INT32 _rtnExtDataProcessor::init( INT32 id, const CHAR *csName,
                                     const CHAR *clName,
                                     const CHAR *idxName,
                                     const CHAR *targetName,
                                     const BSONObj &idxKeyDef )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSOR_INIT ) ;

      SDB_ASSERT( csName && clName && idxName && targetName,
                  "Names should not be NULL") ;

      rc = _meta.init( csName, clName, idxName, targetName, idxKeyDef ) ;
      PD_RC_CHECK( rc, PDERROR, "Processor meta init failed[ %d ]", rc ) ;

      rc = setTargetNames( targetName ) ;
      PD_RC_CHECK( rc, PDERROR, "Set target names failed[ %d ]", rc ) ;
      _id = id ;
      _stat = RTN_EXT_PROCESSOR_CREATING ;

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAPROCESSOR_INIT, rc ) ;
      return rc ;
   error:
      reset() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR_RESET, "_rtnExtDataProcessor::reset" )
   void _rtnExtDataProcessor::reset()
   {
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSOR_RESET ) ;
      _stat = RTN_EXT_PROCESSOR_INVALID ;
      _su = NULL ;
      _needUpdateLSN = FALSE ;
      _keySet.clear() ;
      _keySetNew.clear() ;
      _needOprRec = FALSE ;
      _freeSpace = 0 ;
      _id = RTN_EXT_PROCESSOR_INVALID_ID ;
      _meta.reset() ;
      ossMemset( _cappedCSName, 0, DMS_COLLECTION_SPACE_NAME_SZ + 1 ) ;
      ossMemset( _cappedCLName, 0, DMS_COLLECTION_NAME_SZ + 1 ) ;
      PD_TRACE_EXIT( SDB__RTNEXTDATAPROCESSOR_RESET ) ;
   }

   INT32 _rtnExtDataProcessor::getID() const
   {
      return _id ;
   }

   rtnExtProcessorStat _rtnExtDataProcessor::stat() const
   {
      return _stat ;
   }

   INT32 _rtnExtDataProcessor::active()
   {
      _stat = RTN_EXT_PROCESSOR_NORMAL ;

      return SDB_OK ;
   }

   BOOLEAN _rtnExtDataProcessor::isActive() const
   {
      return ( RTN_EXT_PROCESSOR_NORMAL == _stat ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR_UPDATEMETA, "_rtnExtDataProcessor::updateMeta" )
   void _rtnExtDataProcessor::updateMeta( const CHAR *csName,
                                          const CHAR *clName,
                                          const CHAR *idxName )
   {
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSOR_UPDATEMETA ) ;
      if ( csName && ( ossStrcmp( csName, _meta._csName ) != 0 ) )
      {
         ossStrncpy( _meta._csName, csName, DMS_COLLECTION_SPACE_NAME_SZ + 1 ) ;
      }

      if ( clName && ( ossStrcmp( clName, _meta._clName ) != 0 ) )
      {
         ossStrncpy( _meta._clName, clName, DMS_COLLECTION_NAME_SZ + 1 ) ;
      }

      if ( idxName && ( ossStrcmp( idxName, _meta._idxName ) != 0 ) )
      {
         ossStrncpy( _meta._idxName, idxName, IXM_INDEX_NAME_SIZE + 1 ) ;
      }
      PD_TRACE_EXIT( SDB__RTNEXTDATAPROCESSOR_UPDATEMETA ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR_SETTARGETNAMES, "_rtnExtDataProcessor::setTargetNames" )
   INT32 _rtnExtDataProcessor::setTargetNames( const CHAR *extName )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSOR_SETTARGETNAMES ) ;
      SDB_ASSERT( extName, "cs name is NULL" ) ;

      if ( ossStrlen( extName ) > DMS_COLLECTION_SPACE_NAME_SZ )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "External name too long: %s", extName ) ;
         goto error ;
      }

      ossStrncpy( _cappedCSName, extName, DMS_COLLECTION_SPACE_NAME_SZ + 1 ) ;
      ossSnprintf( _cappedCLName, DMS_COLLECTION_FULL_NAME_SZ + 1,
                   "%s.%s", extName, extName ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAPROCESSOR_SETTARGETNAMES, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR_CHECK, "_rtnExtDataProcessor::check" )
   INT32 _rtnExtDataProcessor::check( DMS_EXTOPR_TYPE type,
                                      const BSONObj *object,
                                      const BSONObj *objectNew )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSOR_CHECK ) ;

      // Only check for insert/delete/update when object is not empty.
      if ( type > DMS_EXTOPR_TYPE_UPDATE || !object )
      {
         goto done ;
      }

      _keySet.clear() ;
      _keySetNew.clear() ;
      _needOprRec = FALSE ;

      try
      {
         // Get the key set
         BSONElement arrayEle ;
         BOOLEAN canContinue = TRUE ;
         ixmIndexKeyGen keygen( _meta._idxKeyDef, GEN_OBJ_KEEP_FIELD_NAME ) ;
         rc = keygen.getKeys( *object, _keySet, &arrayEle, TRUE, TRUE ) ;
         // Records may contain multiple arrays.
         // If it's the old record, it should not have been indexed on ES. In
         // that case, if keys can be gotten from new record, the new record
         // should be indexed.
         // If it's the new record, it should not be indexed. We ignore the
         // multiple array of the old record, error will be returned when
         // parsing the new record below.
         if ( rc )
         {
            canContinue = ( SDB_IXM_MULTIPLE_ARRAY == rc &&
                            ( DMS_EXTOPR_TYPE_UPDATE == type ||
                              DMS_EXTOPR_TYPE_DELETE == type ) ) ;
         }

         if ( !canContinue )
         {
            PD_LOG( PDERROR, "Generate key from object[%] failed[%d]",
                    object->toString().c_str(), rc ) ;
            goto error ;
         }

         rc = SDB_OK ;

         // Record with array in its index key will never be indexed on
         // Elasticsearch, and no record for it will be inserted into capped
         // collection.
         if ( DMS_EXTOPR_TYPE_UPDATE != type )
         {
            // Array for insert and delete, skip.
            if ( _keySet.size() > 1 )
            {
               _keySet.clear() ;
               goto done ;
            }
            _needOprRec = ( _keySet.size() > 0 ) ;
         }
         else
         {
            SDB_ASSERT( objectNew, "New object is NULL" ) ;
            // If it's update, need to get the new key set.
            rc = keygen.getKeys( *objectNew, _keySetNew,
                                 NULL, TRUE, TRUE ) ;
            PD_RC_CHECK( rc, PDERROR, "Generate key from object[ %s ] "
                         "failed[ %d ]", objectNew->toString().c_str(), rc ) ;

            {
               BSONObjSet::iterator origItr = _keySet.begin() ;
               BSONObjSet::iterator newItr = _keySetNew.begin() ;

               // There are only two scenarios that we do not insert operation
               // record:
               // 1. Both old and new records have no index field.
               // 2. Both old and new records have index field(s) but they are the
               //    same.
               while ( origItr != _keySet.end() && newItr != _keySetNew.end() )
               {
                  if ( !( *origItr == *newItr ) )
                  {
                     break ;
                  }
                  ++origItr ;
                  ++newItr ;
               }

               if ( ( origItr == _keySet.end() ) &&
                    ( newItr == _keySetNew.end() )  )
               {
                  _needOprRec = FALSE ;
               }
               else
               {
                  _needOprRec = TRUE ;
               }
            }

            // New record contains array. It will not be inserted. If there is
            // old record, it will be deleted on ES.
            if ( _keySetNew.size() > 1 )
            {
               _keySetNew.clear() ;
            }
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR ,"Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAPROCESSOR_CHECK, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR_PROCESSINSERT, "_rtnExtDataProcessor::processInsert" )
   INT32 _rtnExtDataProcessor::processInsert( const BSONObj &inputObj,
                                              pmdEDUCB *cb, SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSOR_PROCESSINSERT ) ;
      BSONObj recordObj ;
      INT32 insertNum = 0 ;
      INT32 ignoreNum = 0 ;
      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;

      rc = _prepareInsert( inputObj, recordObj ) ;
      PD_RC_CHECK( rc, PDERROR, "Prepare data for insert record[ %s ] "
                   "failed[ %d ]", inputObj.toString().c_str(), rc ) ;

      if ( recordObj.isEmpty() )
      {
         goto done ;
      }

      rc = _spaceCheck( recordObj.objsize() ) ;
      PD_RC_CHECK( rc, PDERROR, "Space check failed[ %d ]", rc ) ;

      rc = rtnInsert( _cappedCLName, recordObj, 1, 0,
                      cb, dmsCB, dpsCB, 1, &insertNum, &ignoreNum ) ;
      PD_RC_CHECK( rc, PDERROR, "Insert record insert collection[ %s ] "
                   "failed[ %d ]", _cappedCLName, rc ) ;

      _needUpdateLSN = TRUE ;

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAPROCESSOR_PROCESSINSERT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR_PROCESSDELETE, "_rtnExtDataProcessor::processDelete" )
   INT32 _rtnExtDataProcessor::processDelete( const BSONObj &inputObj,
                                              pmdEDUCB *cb, SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSOR_PROCESSDELETE ) ;
      BSONObj recordObj ;
      INT32 insertNum = 0 ;
      INT32 ignoreNum = 0 ;
      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;

      rc = _prepareDelete( inputObj, recordObj ) ;
      PD_RC_CHECK( rc, PDERROR, "Prepare data for delete record[ %s ] "
                   "failed[ %d ]", inputObj.toString().c_str(), rc ) ;

      if ( recordObj.isEmpty() )
      {
         goto done ;
      }

      rc = _spaceCheck( recordObj.objsize() ) ;
      PD_RC_CHECK( rc, PDERROR, "Space check failed[ %d ]", rc ) ;

      rc = rtnInsert( _cappedCLName, recordObj, 1, 0,
                      cb, dmsCB, dpsCB, 1, &insertNum, &ignoreNum ) ;
      PD_RC_CHECK( rc, PDERROR, "Insert record insert collection[ %s ] "
                   "failed[ %d ]", _cappedCLName, rc ) ;

      _needUpdateLSN = TRUE ;

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAPROCESSOR_PROCESSDELETE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR_PROCESSUPDATE, "_rtnExtDataProcessor::processUpdate" )
   INT32 _rtnExtDataProcessor::processUpdate( const BSONObj &originalObj,
                                              const BSONObj &newObj,
                                              pmdEDUCB *cb, SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSOR_PROCESSUPDATE ) ;
      BSONObj recordObj ;
      INT32 insertNum = 0 ;
      INT32 ignoreNum = 0 ;
      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;

      rc = _prepareUpdate( originalObj, newObj, recordObj ) ;
      PD_RC_CHECK( rc, PDERROR, "Prepare data for update record from[ %s ] "
                   "to[ %s ] failed[ %d ]", originalObj.toString().c_str(),
                   newObj.toString().c_str(), rc ) ;

      if ( recordObj.isEmpty() )
      {
         goto done ;
      }

      rc = _spaceCheck( recordObj.objsize() ) ;
      PD_RC_CHECK( rc, PDERROR, "Space check failed[ %d ]", rc ) ;

      rc = rtnInsert( _cappedCLName, recordObj, 1, 0,
                      cb, dmsCB, dpsCB, 1, &insertNum, &ignoreNum ) ;
      PD_RC_CHECK( rc, PDERROR, "Insert record insert collection[ %s ] "
                   "failed[ %d ]", _cappedCLName, rc ) ;

      _needUpdateLSN = TRUE ;

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAPROCESSOR_PROCESSUPDATE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR_PROCESSTRUNCATE, "_rtnExtDataProcessor::processTruncate" )
   INT32 _rtnExtDataProcessor::processTruncate( pmdEDUCB *cb, SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSOR_PROCESSTRUNCATE ) ;
      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;

      rc = rtnTruncCollectionCommand( _cappedCLName, cb, dmsCB, dpsCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Truncate capped collection[%s] failed[%d]",
                   _cappedCLName,  rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAPROCESSOR_PROCESSTRUNCATE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR_DODROPP1, "_rtnExtDataProcessor::doDropP1" )
   INT32 _rtnExtDataProcessor::doDropP1( pmdEDUCB *cb, SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSOR_DODROPP1 ) ;
      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;

      rc = rtnDropCollectionSpaceP1( _cappedCSName, cb, dmsCB, dpsCB, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Phase 1 of dropping collection space[ %s ] "
                   "failed[ %d ]", _cappedCSName, rc ) ;
      rtnCB->incTextIdxVersion() ;

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAPROCESSOR_DODROPP1, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR_DODROPP1CANCEL, "_rtnExtDataProcessor::doDropP1Cancel" )
   INT32 _rtnExtDataProcessor::doDropP1Cancel( pmdEDUCB *cb, SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSOR_DODROPP1CANCEL ) ;
      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;

      rc = rtnDropCollectionSpaceP1Cancel( _cappedCSName, cb, dmsCB,
                                           dpsCB, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Cancel dropping collection space[ %s ] in "
                   "phase 1 failed[ %d ]", _cappedCSName, rc ) ;
      rtnCB->incTextIdxVersion() ;

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAPROCESSOR_DODROPP1CANCEL, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR_DODROPP2, "_rtnExtDataProcessor::doDropP2" )
   INT32 _rtnExtDataProcessor::doDropP2( pmdEDUCB *cb, SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSOR_DODROPP2 ) ;
      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;

      rc = rtnDropCollectionSpaceP2( _cappedCSName, cb, dmsCB,
                                     dpsCB, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Phase 1 of dropping collection space[ %s ] "
                   "failed[ %d ]", _cappedCSName, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAPROCESSOR_DODROPP2, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnExtDataProcessor::doLoad()
   {
      // TODO:YSD
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR_DOUNLOAD, "_rtnExtDataProcessor::doUnload" )
   INT32 _rtnExtDataProcessor::doUnload( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSOR_DOUNLOAD ) ;
      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;

      rc = rtnUnloadCollectionSpace( _cappedCSName, cb, dmsCB ) ;
      if ( SDB_DMS_CS_NOTEXIST == rc )
      {
         PD_LOG( PDWARNING, "Capped collection space[ %s ] not found when "
                 "unload", _cappedCSName ) ;
         rc = SDB_OK ;
      }
      else
      {
         PD_RC_CHECK( rc, PDERROR, "Unload capped collection space failed[ %d ]",
                      rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAPROCESSOR_DOUNLOAD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR_DOREBUILD, "_rtnExtDataProcessor::doRebuild" )
   INT32 _rtnExtDataProcessor::doRebuild( pmdEDUCB *cb, SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSOR_DOREBUILD ) ;
      pmdKRCB *krcb = pmdGetKRCB() ;
      SDB_DB_STATUS dbStatus = krcb->getDBStatus() ;
      SDB_DMSCB *dmsCB = krcb->getDMSCB() ;
      dmsStorageUnitID suID = DMS_INVALID_SUID ;

      // In case of rebuilding, we don't know whether the capped collections are
      // valid or not. So we directly drop and re-create the capped collection
      // again.
      if ( SDB_DB_REBUILDING == dbStatus )
      {
         rc = rtnDropCollectionSpaceCommand( _cappedCSName, cb,
                                             dmsCB, dpsCB, TRUE ) ;
         if ( SDB_DMS_CS_NOTEXIST == rc )
         {
            PD_LOG( PDINFO, "Capped collection space[ %s ] not found when "
                    "trying to drop it", _cappedCSName ) ;
         }
         else
         {
            PD_RC_CHECK( rc, PDERROR, "Drop collectionspace[ %s ] failed[ %d ]",
                         _cappedCSName, rc);
         }
      }
      else if ( SDB_DB_FULLSYNC == dbStatus )
      {
         goto done ;
      }

      rc = _prepareCSAndCL( _cappedCSName, _cappedCLName, cb, dpsCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Create capped collection[ %s.%s ] "
                   "failed[ %d ]", _cappedCSName, _cappedCLName, rc ) ;

   done:
      if ( DMS_INVALID_SUID != suID )
      {
         dmsCB->suUnlock( suID, SHARED ) ;
      }
      PD_TRACE_EXITRC( SDB__RTNEXTDATAPROCESSOR_DOREBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // Update the commit LSN information for the capped collection.
   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR_DONE, "_rtnExtDataProcessor::done" )
   INT32 _rtnExtDataProcessor::done( pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSOR_DONE ) ;
      dmsMBContext *context = NULL ;

      if ( _needUpdateLSN )
      {
         SDB_ASSERT( _su, "Storage unit is NULL") ;
         rc = _su->data()->getMBContext( &context, _getExtCLShortName(), -1 ) ;
         PD_RC_CHECK( rc, PDERROR, "Get mbcontext for collection[ %s ] "
                                   "failed[ %d ]", _cappedCLName, rc ) ;

         SDB_ASSERT( context, "mbContext should not be NULL" ) ;

         context->mbStat()->updateLastLSN( cb->getEndLsn(), DMS_FILE_DATA ) ;
      }
      _keySet.clear() ;
      _keySetNew.clear() ;

   done:
      if ( context )
      {
         _su->data()->releaseMBContext( context ) ;
      }

      PD_TRACE_EXITRC( SDB__RTNEXTDATAPROCESSOR_DONE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR_ABORT, "_rtnExtDataProcessor::abort" )
   INT32 _rtnExtDataProcessor::abort()
   {
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSOR_ABORT ) ;
      _needUpdateLSN = FALSE ;
      _keySet.clear() ;
      _keySetNew.clear() ;
      PD_TRACE_EXIT( SDB__RTNEXTDATAPROCESSOR_ABORT ) ;
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR__ISOWNEDBY, "_rtnExtDataProcessor::isOwnedBy" )
   BOOLEAN _rtnExtDataProcessor::isOwnedBy( const CHAR *csName,
                                            const CHAR *clName,
                                            const CHAR *idxName ) const
   {
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSOR__ISOWNEDBY ) ;
      BOOLEAN result = FALSE ;

      SDB_ASSERT( csName, "CS name can not be NULL" ) ;

      if ( 0 == ossStrcmp( csName, _meta._csName ) )
      {
         result = TRUE ;
      }

      if ( clName && 0 != ossStrcmp( clName, _meta._clName ) && result )
      {
         result = FALSE ;
      }

      if ( idxName && 0 != ossStrcmp( idxName, _meta._idxName )
           && result )
      {
         result = FALSE ;
      }

      PD_TRACE_EXIT( SDB__RTNEXTDATAPROCESSOR__ISOWNEDBY ) ;
      return result ;
   }

   BOOLEAN _rtnExtDataProcessor::isOwnedByExt( const CHAR *targetName ) const
   {
      return ( 0 == ossStrcmp( targetName, _meta._targetName ) ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR__PREPARERECORD, "_rtnExtDataProcessor::_prepareRecord" )
   INT32 _rtnExtDataProcessor::_prepareRecord( _rtnExtOprType oprType,
                                               const BSONElement &idEle,
                                               const BSONObj *dataObj,
                                               BSONObj &recordObj,
                                               const BSONElement *newIdEle )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSOR__PREPARERECORD ) ;
      BSONObjBuilder objBuilder ;

      if ( RTN_EXT_INVALID == oprType )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Invalid operation type[ %d ]", oprType ) ;
         goto error ;
      }

      try
      {
         // 1. Append operation type.
         objBuilder.append( FIELD_NAME_TYPE, oprType ) ;
         // 2. Append the _id as _rid.
         objBuilder.appendAs( idEle, RTN_FIELD_NAME_RID ) ;
         // 3. Insert new _id if the _id field has been modified.
         if ( RTN_EXT_UPDATE_WITH_ID == oprType )
         {
            if ( !newIdEle )
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "New _id is invalid" ) ;
               goto error ;
            }
            objBuilder.appendAs( *newIdEle, RTN_FIELD_NAME_RID_NEW ) ;
         }
         // 4. Append data if necessarry.
         if ( RTN_EXT_INSERT == oprType || RTN_EXT_UPDATE == oprType ||
              RTN_EXT_UPDATE_WITH_ID == oprType )
         {
            if ( !dataObj )
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "Data object is NULL for operation type[ %d ]",
                       oprType ) ;
               goto error ;
            }
            objBuilder.append( RTN_FIELD_NAME_SOURCE, *dataObj ) ;
         }

         recordObj = objBuilder.obj() ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAPROCESSOR__PREPARERECORD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR__PREPARECSANDCL, "_rtnExtDataProcessor::_prepareCSAndCL" )
   INT32 _rtnExtDataProcessor::_prepareCSAndCL( const CHAR *csName,
                                                const CHAR *clName,
                                                pmdEDUCB *cb,
                                                SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSOR__PREPARECSANDCL ) ;
      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
      BOOLEAN csCreated = FALSE ;
      BSONObjBuilder builder ;
      BSONObj extOptions ;

      rc = rtnCreateCollectionSpaceCommand( csName, cb, dmsCB, dpsCB,
                                            UTIL_UNIQUEID_NULL,
                                            DMS_PAGE_SIZE_DFT,
                                            DMS_DO_NOT_CREATE_LOB,
                                            DMS_STORAGE_CAPPED, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Create capped collection space failed[ %d ]",
                   rc ) ;

      csCreated = TRUE ;

      try
      {
         builder.append( FIELD_NAME_SIZE, RTN_CAPPED_CL_MAXSIZE ) ;
         builder.append( FIELD_NAME_MAX, RTN_CAPPED_CL_MAXRECNUM ) ;
         // Set the OverWrite option as false.
         builder.appendBool( FIELD_NAME_OVERWRITE, FALSE ) ;
         extOptions = builder.done() ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Exception occurred: %s", e.what() ) ;
         goto error ;
      }

      rc = rtnCreateCollectionCommand( clName,
                                       DMS_MB_ATTR_NOIDINDEX | DMS_MB_ATTR_CAPPED,
                                       cb, dmsCB, dpsCB, UTIL_UNIQUEID_NULL,
                                       UTIL_COMPRESSOR_INVALID, 0,
                                       TRUE, &extOptions ) ;
      PD_RC_CHECK( rc, PDERROR, "Create capped collection[ %s ] failed[ %d ]",
                   clName, rc ) ;
      rtnCB->incTextIdxVersion() ;

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAPROCESSOR__PREPARECSANDCL, rc ) ;
      return rc ;
   error:
      if ( csCreated )
      {
         INT32 rcTmp = rtnDropCollectionSpaceCommand( csName, cb, dmsCB,
                                                      dpsCB, TRUE ) ;
         if ( rcTmp )
         {
            PD_LOG( PDERROR, "Drop collectionspace[ %s ] failed[ %d ]",
                    csName, rcTmp ) ;
         }
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR_PREPAREINSERT, "_rtnExtDataProcessor::_prepareInsert" )
   INT32 _rtnExtDataProcessor::_prepareInsert( const BSONObj &inputObj,
                                               BSONObj &recordObj )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSOR_PREPAREINSERT ) ;

      if ( !_needOprRec )
      {
         goto done ;
      }

      try
      {
         BSONElement ele ;
         if ( 0 == _keySet.size() )
         {
            // No index key in the record, skip.
            goto done ;
         }

         // Get the _id from the insert object.
         ele = inputObj.getField( DMS_ID_KEY_NAME ) ;
         if ( ele.eoo() )
         {
            PD_LOG( PDERROR, "Text index can not be used if record has no _id "
                    "field" ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         {
            BSONObjSet::iterator it = _keySet.begin();
            BSONObj object( *it ) ;
            rc = _prepareRecord( RTN_EXT_INSERT, ele, &object, recordObj ) ;
            PD_RC_CHECK( rc, PDERROR, "Add operation record failed[ %d ]",
                         rc ) ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAPROCESSOR_PREPAREINSERT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR_PREPAREDELETE, "_rtnExtDataProcessor::_prepareDelete" )
   INT32 _rtnExtDataProcessor::_prepareDelete( const BSONObj &inputObj,
                                               BSONObj &recordObj )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSOR_PREPAREDELETE ) ;

      if ( !_needOprRec )
      {
         goto done ;
      }

      try
      {
         BSONElement ele ;
         if ( 0 == _keySet.size() )
         {
            // No index key in the record, skip.
            goto done ;
         }

         ele = inputObj.getField( DMS_ID_KEY_NAME ) ;
         if ( EOO == ele.type() )
         {
            PD_LOG( PDERROR, "Text index can not be used if record has no _id "
                  "field" ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         rc = _prepareRecord( RTN_EXT_DELETE, ele, NULL, recordObj ) ;
         PD_RC_CHECK( rc, PDERROR, "Add operation record failed[ %d ]",
                      rc ) ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAPROCESSOR_PREPAREDELETE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR_PREPAREUPDATE, "_rtnExtDataProcessor::_prepareUpdate" )
   INT32 _rtnExtDataProcessor::_prepareUpdate( const BSONObj &originalObj,
                                               const BSONObj &newObj,
                                               BSONObj &recordObj )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSOR_PREPAREUPDATE ) ;

      try
      {
         BOOLEAN idModified = FALSE ;
         BSONElement idEle = originalObj.getField( DMS_ID_KEY_NAME ) ;
         BSONElement idEleNew = newObj.getField( DMS_ID_KEY_NAME ) ;
         if ( idEle.eoo() || idEleNew.eoo() )
         {
            PD_LOG( PDERROR, "Text index can not be used if record has no "
                             "_id field" ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         if ( !idEle.valuesEqual( idEleNew ) )
         {
            idModified = TRUE ;
            _needOprRec = TRUE ;
         }

         if ( !_needOprRec )
         {
            goto done ;
         }

         if ( 0 == _keySetNew.size() )
         {
            rc = _prepareDelete( originalObj, recordObj ) ;
            PD_RC_CHECK( rc, PDERROR, "Prepare for delete failed[ %d ]", rc ) ;
            goto done ;
         }
         SDB_ASSERT( 1 == _keySetNew.size(), "Key set size should be 1" ) ;

         {
            BSONObjSet::iterator it = _keySetNew.begin() ;
            BSONObj object( *it ) ;
            if ( idModified )
            {
               rc = _prepareRecord( RTN_EXT_UPDATE_WITH_ID, idEle,
                                    &object, recordObj, &idEleNew ) ;
            }
            else
            {
               rc = _prepareRecord( RTN_EXT_UPDATE, idEleNew,
                                    &object, recordObj ) ;
            }
            PD_RC_CHECK( rc, PDERROR, "Add operation record failed[ %d ]",
                         rc ) ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAPROCESSOR_PREPAREUPDATE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR__GETEXTCLSHORTNAME, "_rtnExtDataProcessor::_getExtCLShortName" )
   const CHAR* _rtnExtDataProcessor::_getExtCLShortName() const
   {
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSOR__GETEXTCLSHORTNAME ) ;
      UINT32 curPos = 0 ;
      UINT32 fullLen = (UINT32)ossStrlen( _cappedCLName ) ;
      const CHAR *clShortName = NULL ;

      if ( 0 == fullLen )
      {
         clShortName = NULL ;
         goto done ;
      }

      while ( ( curPos < fullLen ) && ( '.' != _cappedCLName[curPos] ) )
      {
         curPos++ ;
      }

      if ( ( curPos == fullLen ) || ( curPos == fullLen - 1 ) )
      {
         clShortName = NULL ;
         goto done ;
      }

      clShortName = _cappedCLName + curPos + 1 ;

   done:
      PD_TRACE_EXIT( SDB__RTNEXTDATAPROCESSOR__GETEXTCLSHORTNAME ) ;
      return clShortName ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR__SPACECHECK, "_rtnExtDataProcessor::_spaceCheck" )
   INT32 _rtnExtDataProcessor::_spaceCheck( UINT32 size )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSOR__SPACECHECK ) ;
      if ( _freeSpace < size )
      {
         // If cached size is not enough, let's sync the information. The capped
         // collection may have been popped by the search engine adapter.
         rc = _updateSpaceInfo( _freeSpace ) ;
         PD_RC_CHECK( rc, PDERROR, "Update space information for collection"
                                   "[ %s ] failed[ %d ]",
                      _cappedCLName, rc ) ;

         // If space still not enough after update, report error.
         if ( _freeSpace < size )
         {
            rc = SDB_OSS_UP_TO_LIMIT ;
            PD_LOG( PDERROR, "Space not enough, request size[ %u ]", size ) ;
            goto error ;
         }
      }

      _freeSpace -= ossRoundUpToMultipleX( size, 4 ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAPROCESSOR__SPACECHECK, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR__UPDATESPACEINFO, "_rtnExtDataProcessor::_updateSpaceInfo" )
   INT32 _rtnExtDataProcessor::_updateSpaceInfo( UINT64 &freeSpace )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSOR__UPDATESPACEINFO ) ;
      const dmsMBStatInfo *mbStat = NULL ;
      UINT64 remainSize = 0 ;
      UINT32 remainExtentNum = 0 ;
      dmsMBContext *context = NULL ;

      if ( !_su )
      {
         SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
         dmsStorageUnitID suID = DMS_INVALID_CS ;
         // Delay set in open text index case.
         rc = dmsCB->nameToSUAndLock( _cappedCSName, suID, &_su ) ;
         PD_RC_CHECK( rc, PDERROR, "Lock collection space[ %s ] failed[ %d ]",
                      _cappedCSName, rc ) ;
         dmsCB->suUnlock( suID, SHARED ) ;
      }
      rc = _su->data()->getMBContext( &context, _getExtCLShortName(), -1 ) ;
      PD_RC_CHECK( rc, PDERROR, "Get mbcontext for collection[ %s ] "
                                "failed[ %d ]", _cappedCLName, rc ) ;

      mbStat = context->mbStat() ;

      remainSize = RTN_CAPPED_CL_MAXSIZE -
            ( mbStat->_totalDataPages << _su->data()->pageSize() ) ;

      remainExtentNum = remainSize / DMS_CAP_EXTENT_SZ ;
      remainSize -= ( remainExtentNum * DMS_EXTENT_METADATA_SZ ) ;

      freeSpace = mbStat->_totalDataFreeSpace + remainSize ;

   done:
      if ( context )
      {
         _su->data()->releaseMBContext( context ) ;
      }
      PD_TRACE_EXITRC( SDB__RTNEXTDATAPROCESSOR__UPDATESPACEINFO, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   _rtnExtDataProcessorMgr::_rtnExtDataProcessorMgr()
   : _number( 0 ),
     _pendingProcessor( NULL )
   {
   }

   _rtnExtDataProcessorMgr::~_rtnExtDataProcessorMgr()
   {
   }

   // activate - To make the processor visible to others.
   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSORMGR_CREATEPROCESSOR, "_rtnExtDataProcessorMgr::createProcessor" )
   INT32 _rtnExtDataProcessorMgr::createProcessor( const CHAR *csName,
                                                   const CHAR *clName,
                                                   const CHAR *idxName,
                                                   const CHAR *extName,
                                                   const BSONObj &idxKeyDef,
                                                   rtnExtDataProcessor *&processor,
                                                   BOOLEAN activate )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSORMGR_CREATEPROCESSOR ) ;
      rtnExtDataProcessor *processorLocal = NULL ;
      INT32 foundID = RTN_EXT_PROCESSOR_INVALID_ID ;
      ossRWMutex *mutex = NULL ;
      BOOLEAN locked = FALSE ;

      ossScopedLock lock( &_mutex, EXCLUSIVE ) ;

      // Loop and find a free processor. Check for the same external data at the
      // same time.
      for ( INT32 id = 0 ; id < RTN_EXT_PROCESSOR_MAX_NUM; ++id )
      {
         if ( _processors[id].isOwnedByExt( extName ) )
         {
            rc = SDB_IXM_EXIST ;
            PD_LOG( PDERROR, "External data with the same name exists. Maybe "
                             "same text index has been created" ) ;
            goto error ;
         }

         if ( !processorLocal &&
              ( RTN_EXT_PROCESSOR_INVALID == _processors[id].stat() ) )
         {
            // Lock and check again
            mutex = &_processorLocks[id] ;
            rc = mutex->lock_w( OSS_ONE_SEC * 5 ) ;
            PD_RC_CHECK( rc, PDERROR, "Lock processor failed[%d]", rc ) ;
            if ( RTN_EXT_PROCESSOR_INVALID != _processors[id].stat() )
            {
               mutex->release_w() ;
               continue ;
            }
            locked = TRUE ;

            foundID = id ;
            processorLocal = &_processors[foundID] ;
            break ;
         }
      }

      if ( !processorLocal )
      {
         PD_LOG( PDERROR, "No slot for processor found" ) ;
         rc = SDB_DMS_MAX_INDEX ;
         goto error ;
      }

      rc = processorLocal->init( foundID, csName, clName,
                                 idxName, extName, idxKeyDef ) ;
      PD_RC_CHECK( rc, PDERROR, "Init external data processor failed[ %d ]",
                   rc ) ;
      _pendingProcessor = processorLocal ;

      if ( activate )
      {
         rc = activateProcessor( processorLocal->getID(), TRUE ) ;
         PD_RC_CHECK( rc, PDERROR, "Activate processor failed[ %d ]", rc ) ;
         _pendingProcessor = NULL ;
      }

      processor = processorLocal ;
      _number++ ;

   done:
      if ( locked )
      {
         mutex->release_w() ;
      }

      PD_TRACE_EXITRC( SDB__RTNEXTDATAPROCESSORMGR_CREATEPROCESSOR, rc ) ;
      return rc ;
   error:
      if ( processorLocal )
      {
         processorLocal->reset() ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSORMGR_ACTIVATEPROCESSOR, "_rtnExtDataProcessorMgr::activateProcessor" )
   INT32 _rtnExtDataProcessorMgr::activateProcessor( INT32 id,
                                                     BOOLEAN inProtection )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY(SDB__RTNEXTDATAPROCESSORMGR_ACTIVATEPROCESSOR);

      if ( !inProtection )
      {
         _mutex.get() ;
      }
      if ( !_pendingProcessor || (id != _pendingProcessor->getID()) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid processor id to activate: %lld", id ) ;
         goto error ;
      }

      if ( RTN_EXT_PROCESSOR_NORMAL != _pendingProcessor->stat() )
      {
         rc = _pendingProcessor->active() ;
         PD_RC_CHECK( rc, PDERROR, "Active processor failed[ %d ]", rc ) ;
         _pendingProcessor = NULL ;
      }

   done:
      if ( !inProtection )
      {
         _mutex.release() ;
      }
      PD_TRACE_EXITRC( SDB__RTNEXTDATAPROCESSORMGR_ACTIVATEPROCESSOR, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   UINT32 _rtnExtDataProcessorMgr::number()
   {
      ossScopedLock lock( &_mutex, SHARED ) ;
      return _number ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSORMGR_GETPROCESSORSBYCS, "_rtnExtDataProcessorMgr::getProcessorsByCS" )
   INT32 _rtnExtDataProcessorMgr::getProcessorsByCS( const CHAR *csName,
                                                     INT32 lockType,
                                                     vector<rtnExtDataProcessor *> &processors )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSORMGR_GETPROCESSORSBYCS ) ;

      UINT16 checkNum = 0 ;
      BOOLEAN locked = ( SHARED == lockType || EXCLUSIVE == lockType ) ;
      vector<INT32> lockedIDs ;
      rtnExtDataProcessor *processor = NULL ;

      ossScopedLock lock( &_mutex, SHARED ) ;

      // One cs may contain multiple text indices, according with multiple
      // processors. We need to lock all of them, to avoid partial failure.
      // As the processors may be used by others, so we give up if any one of
      // the processors can not be locked in one second. All processors who have
      // been locked need to be unlocked.
      for ( INT32 i = 0; i < RTN_EXT_PROCESSOR_MAX_NUM; ++i )
      {
         processor = &_processors[i] ;
         if ( !processor->isActive() )
         {
            continue ;
         }

         if ( processor->isOwnedBy( csName ) )
         {
            ossRWMutex *mutex = &_processorLocks[i] ;
            if ( SHARED == lockType )
            {
               rc = mutex->lock_r( OSS_ONE_SEC ) ;
            }
            else if ( EXCLUSIVE == lockType )
            {
               rc = mutex->lock_w( OSS_ONE_SEC ) ;
            }
            else
            {
               processors.push_back( processor ) ;
               continue ;
            }

            // In case of error, all processors which have been locked need to
            // be released.
            if ( rc )
            {
               goto error ;
            }

            // Double check after aquiring the lock. As when we are waiting for
            // the lock, it may have been deleted by some drop operation.
            if ( !processor->isOwnedBy( csName ) || !processor->isActive() )
            {
               if ( SHARED == lockType )
               {
                  mutex->release_r() ;
               }
               else
               {
                  mutex->release_w() ;
               }
               continue ;
            }

            if ( locked )
            {
               lockedIDs.push_back( i ) ;
            }
            processors.push_back( processor ) ;
         }
         if ( ++checkNum >= _number )
         {
            break ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAPROCESSORMGR_GETPROCESSORSBYCS, rc ) ;
      return rc ;
   error:
      processors.clear() ;
      if ( locked )
      {
         for ( vector<INT32>::iterator itr = lockedIDs.begin();
               itr != lockedIDs.end(); ++itr )
         {
            if ( SHARED == lockType )
            {
               _processorLocks[*itr].release_r() ;
            }
            else
            {
               _processorLocks[*itr].release_w() ;
            }
         }
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSORMGR_GETPROCESSORSBYCL, "_rtnExtDataProcessorMgr::getProcessorsByCL" )
   INT32 _rtnExtDataProcessorMgr::getProcessorsByCL( const CHAR *csName,
                                                     const CHAR *clName,
                                                     INT32 lockType,
                                                     std::vector<rtnExtDataProcessor *> &processors )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSORMGR_GETPROCESSORSBYCL ) ;

      UINT16 checkNum = 0 ;
      BOOLEAN locked = ( SHARED == lockType || EXCLUSIVE == lockType ) ;
      vector<INT32> lockedIDs ;
      rtnExtDataProcessor *processor = NULL ;

      ossScopedLock lock( &_mutex, SHARED ) ;

      for ( INT32 i = 0; i < RTN_EXT_PROCESSOR_MAX_NUM; ++i )
      {
         processor = &_processors[i] ;
         if ( !processor->isActive() )
         {
            continue ;
         }

         if ( processor->isOwnedBy( csName, clName ) )
         {
            ossRWMutex *mutex = &_processorLocks[i] ;
            if ( SHARED == lockType )
            {
               rc = mutex->lock_r( OSS_ONE_SEC ) ;
            }
            else if ( EXCLUSIVE == lockType )
            {
               rc = mutex->lock_w( OSS_ONE_SEC ) ;
            }
            else
            {
               processors.push_back( processor ) ;
               continue ;
            }

            if ( rc )
            {
               goto error ;
            }

            // Double check after aquiring the lock. As when we are waiting for
            // the lock, it may have been deleted by some drop operation.
            if ( !processor->isOwnedBy( csName, clName ) ||
                 !processor->isActive() )
            {
               if ( SHARED == lockType )
               {
                  mutex->release_r() ;
               }
               else
               {
                  mutex->release_w() ;
               }
               continue ;
            }

            if ( locked )
            {
               lockedIDs.push_back( i ) ;
            }
            processors.push_back( processor ) ;
         }
         if ( ++checkNum >= _number )
         {
            break ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAPROCESSORMGR_GETPROCESSORSBYCL, rc ) ;
      return rc ;
   error:
      processors.clear() ;
      if ( locked )
      {
         for ( vector<INT32>::iterator itr = lockedIDs.begin();
               itr != lockedIDs.end(); ++itr )
         {
            if ( SHARED == lockType )
            {
               _processorLocks[*itr].release_r() ;
            }
            else
            {
                _processorLocks[*itr].release_w() ;
            }
         }
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSORMGR_GETPROCESSORBYIDX, "_rtnExtDataProcessorMgr::getProcessorByIdx" )
   INT32 _rtnExtDataProcessorMgr::getProcessorByIdx( const CHAR *csName,
                                                     const CHAR *clName,
                                                     const CHAR *idxName,
                                                     INT32 lockType,
                                                     rtnExtDataProcessor *&processor )
   {
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSORMGR_GETPROCESSORBYIDX ) ;
      UINT16 checkNum = 0 ;
      rtnExtDataProcessor *processorLocal = NULL ;

      processor = NULL ;

      ossScopedLock lock( &_mutex, SHARED ) ;

      for ( INT32 i = 0; i < RTN_EXT_PROCESSOR_MAX_NUM; ++i )
      {
         processorLocal = &_processors[i] ;
         if ( !processorLocal->isActive() )
         {
            continue ;
         }

         if ( processorLocal->isOwnedBy( csName, clName, idxName ) )
         {
            ossRWMutex *mutex = &_processorLocks[i] ;
            if ( SHARED == lockType )
            {
               mutex->lock_r() ;
            }
            else if ( EXCLUSIVE == lockType )
            {
               mutex->lock_w() ;
            }
            // Double check after aquiring the lock. As when we are waiting for
            // the lock, it may have been deleted by some drop operation.
            if ( !processorLocal->isOwnedBy( csName, clName, idxName ) ||
                 !processorLocal->isActive() )
            {
               if ( SHARED == lockType )
               {
                  mutex->release_r() ;
               }
               else
               {
                  mutex->release_w() ;
               }
               continue ;
            }
            processor = processorLocal  ;
            break ;
         }
         if ( ++checkNum >= _number )
         {
            break ;
         }
      }

      PD_TRACE_EXIT( SDB__RTNEXTDATAPROCESSORMGR_GETPROCESSORBYIDX ) ;
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSORMGR_GETPROCESSORBYEXTNAME, "_rtnExtDataProcessorMgr::getProcessorByExtName" )
   INT32 _rtnExtDataProcessorMgr::getProcessorByExtName( const CHAR *extName,
                                                         INT32 lockType,
                                                         rtnExtDataProcessor *&processor )
   {
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSORMGR_GETPROCESSORBYEXTNAME ) ;

      UINT16 checkNum = 0 ;
      processor = NULL  ;

      ossScopedLock lock( &_mutex, SHARED ) ;

      for ( INT32 i = 0; i < RTN_EXT_PROCESSOR_MAX_NUM; ++i )
      {
         if ( !_processors[i].isActive() )
         {
            continue ;
         }
         if ( _processors[i].isOwnedByExt( extName ) )
         {
            ossRWMutex *mutex = &_processorLocks[i] ;
            if ( SHARED == lockType )
            {
               mutex->lock_r() ;
            }
            else if ( EXCLUSIVE == lockType )
            {
               mutex->lock_w() ;
            }
            processor = &_processors[i] ;
            break ;
         }
         if ( ++checkNum >= _number )
         {
            break ;
         }
      }

      PD_TRACE_EXIT( SDB__RTNEXTDATAPROCESSORMGR_GETPROCESSORBYEXTNAME ) ;
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSORMGR_UNLOCKPROCESSORS, "_rtnExtDataProcessorMgr::unlockProcessors" )
   void _rtnExtDataProcessorMgr::unlockProcessors( std::vector<rtnExtDataProcessor *> &processors,
                                                   INT32 lockType )
   {
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSORMGR_UNLOCKPROCESSORS ) ;
      if ( SHARED != lockType && EXCLUSIVE != lockType )
      {
         return ;
      }

      {
         ossScopedLock lock( &_mutex, SHARED ) ;
         for ( std::vector<rtnExtDataProcessor *>::iterator itr = processors.begin() ;
               itr != processors.end(); ++itr )
         {
            ossRWMutex *mutex = &_processorLocks[ (*itr)->getID() ] ;
            if ( SHARED == lockType )
            {
               mutex->release_r() ;
            }
            else
            {
               mutex->release_w() ;
            }
         }
      }
      PD_TRACE_EXIT( SDB__RTNEXTDATAPROCESSORMGR_UNLOCKPROCESSORS ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSORMGR_DESTROYPROCESSOR, "_rtnExtDataProcessorMgr::destroyProcessor" )
   void
   _rtnExtDataProcessorMgr::destroyProcessor( rtnExtDataProcessor *processor,
                                              INT32 lockType )
   {
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSORMGR_DESTROYPROCESSOR ) ;
      if ( processor )
      {
         INT32 id = processor->getID() ;
         if ( id < 0 || id >= RTN_EXT_PROCESSOR_MAX_NUM )
         {
            PD_LOG( PDWARNING, "Invalid processor id[%d] for destroy", id ) ;
            goto done ;
         }

         ossScopedLock _lock( &_mutex, EXCLUSIVE ) ;
         processor->reset() ;
         --_number ;
         if ( SHARED == lockType )
         {
            _processorLocks[id].release_r() ;
         }
         else if ( EXCLUSIVE == lockType )
         {
            _processorLocks[id].release_w() ;
         }
      }
   done:
      PD_TRACE_EXIT( SDB__RTNEXTDATAPROCESSORMGR_DESTROYPROCESSOR ) ;
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSORMGR_DESTROYPROCESSORS, "_rtnExtDataProcessorMgr::destroyProcessors" )
   void _rtnExtDataProcessorMgr::destroyProcessors( vector<rtnExtDataProcessor*> &processors,
                                                    INT32 lockType )
   {
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSORMGR_DESTROYPROCESSORS ) ;
      ossScopedLock _lock( &_mutex, EXCLUSIVE ) ;

      for ( vector<rtnExtDataProcessor*>::iterator itr = processors.begin();
            itr != processors.end(); ++itr )
      {
         INT32 id = (*itr)->getID() ;
         if ( id < 0 || id >= RTN_EXT_PROCESSOR_MAX_NUM )
         {
            PD_LOG( PDWARNING, "Invalid processor id[%d] for destroy", id ) ;
            continue ;
         }
         _processors[id].reset() ;
         --_number ;
         if ( SHARED == lockType )
         {
            _processorLocks[id].release_r() ;
         }
         else if ( EXCLUSIVE == lockType )
         {
            _processorLocks[id].release_w() ;
         }
      }
      PD_TRACE_EXIT( SDB__RTNEXTDATAPROCESSORMGR_DESTROYPROCESSORS ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSORMGR_RENAMECS, "_rtnExtDataProcessorMgr::renameCS" )
   INT32 _rtnExtDataProcessorMgr::renameCS( const CHAR *name,
                                            const CHAR *newName )
   {
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSORMGR_RENAMECS ) ;
      ossScopedLock _lock( &_mutex, EXCLUSIVE ) ;

      for ( INT32 i = 0; i < RTN_EXT_PROCESSOR_MAX_NUM; ++i )
      {
         if ( _processors[i].isOwnedBy( name ) )
         {
            _processors[i].updateMeta( newName ) ;
         }
      }

      PD_TRACE_EXIT( SDB__RTNEXTDATAPROCESSORMGR_RENAMECS ) ;
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSORMGR_RENAMECL, "_rtnExtDataProcessorMgr::renameCL" )
   INT32 _rtnExtDataProcessorMgr::renameCL( const CHAR *csName,
                                            const CHAR *clName,
                                            const CHAR *newCLName )
   {
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSORMGR_RENAMECL ) ;
      ossScopedLock _lock( &_mutex, EXCLUSIVE ) ;

      for ( INT32 i = 0; i < RTN_EXT_PROCESSOR_MAX_NUM; ++i )
      {
         if ( _processors[i].isOwnedBy( csName, clName ) )
         {
            _processors[i].updateMeta( csName, newCLName ) ;
         }
      }

      PD_TRACE_EXIT( SDB__RTNEXTDATAPROCESSORMGR_RENAMECL ) ;
      return SDB_OK ;
   }

   rtnExtDataProcessorMgr* rtnGetExtDataProcessorMgr()
   {
      static rtnExtDataProcessorMgr s_edpMgr ;
      return &s_edpMgr ;
   }
}

