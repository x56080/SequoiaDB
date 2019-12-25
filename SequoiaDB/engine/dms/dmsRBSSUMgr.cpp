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

   Source File Name = dmsRBSSUMgr.cpp

   Descriptive Name = DMS Rollback Segment Storage Unit Management

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains code logic for
   rollback segment creation and release.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/24/2019  CYX Initial Draft

   Last Changed =

*******************************************************************************/


#include "dmsStorageUnit.hpp"
#include "../bson/bson.h"
#include "rtn.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"
#include "pmd.hpp"
#include "dmsCB.hpp"
#include "dmsRBSSUMgr.hpp"
#include "dmsScanner.hpp"
#include "dmsStorageDataCommon.hpp"
#include "dmsStorageDataCapped.hpp"
#include "dpsUtil.hpp"
#include "ossMem.hpp"

#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>

using namespace bson ;
namespace fs = boost::filesystem ;

namespace engine
{

   _dmsRBSSUMgr::_dmsRBSSUMgr ( SDB_DMSCB *dmsCB ) : _dmsSysSUMgr( dmsCB )
   {
      DMS_BUILD_RBS_CL_NAME( _metaCLName, DMS_META_RBS_CL ) ;

      // By default, start with second collection as the first one stores meta
      _currentCollection  = DMS_FIRST_RBS_CL ;
      _lastFreeCollection = DMS_MAX_RBS_CL ;
      // use default size for now, we may want to add config parm later on
      _maxCollectionSize  = DMS_DFT_RBSCL_SIZE ;
   }

   // Initialization of RBS during node start up
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSRBSSUMGR_INIT, "_dmsRBSSUMgr::init" )
   SINT32 _dmsRBSSUMgr::init ()
   {
      PD_TRACE_ENTRY ( SDB__DMSRBSSUMGR_INIT ) ;
      SINT32            rc    = SDB_OK ;
      dmsStorageUnitID  suID  = DMS_INVALID_CS ;
      pmdEDUCB         *eduCB = pmdGetThreadEDUCB() ;
      // we explicitly set dpsCB to NULL during init phase because we are 
      // going to assume that the SYSRBS and SYSRBS0000 exist. Every nodes 
      // with mvccon enabled must have both. So we will simply create them
      // during db start if they are not there. For newly started node, we
      // will also create SYSRBS0001 by default, which all done by this init
      // function. By using NULL dpsCB here, we will not generate LRs. During
      // runtime, we will generate LRs for creating SYSRBSxxxx and update of 
      // the meta pages (see updateMeta for details). 
      // If decision is changed, we can set to pmdGetKRCB()->getDPSCB()
      SDB_DPSCB        *dpsCB = NULL ;

      SDB_ASSERT ( _dmsCB, "dmsCB can't be NULL" ) ;


      // exclusive lock temp cb. this function should be called during process
      // initialization, so it shouldn't be called in parallel by agents
      DMSSYSSUMGR_XLOCK() ;

      // first to load collection space
      rc = rtnLoadCollectionSpace( SDB_DMSRBS_NAME,
                                   pmdGetOptionCB()->getDbPath(),
                                   pmdGetOptionCB()->getIndexPath(),
                                   pmdGetOptionCB()->getLobPath(),
                                   pmdGetOptionCB()->getLobMetaPath(),
                                   NULL, _dmsCB, FALSE ) ;
      // FIXME: working one but will generate warning message. To be removed
      //rc = rtnCollectionSpaceLock( SDB_DMSRBS_NAME, _dmsCB, TRUE,
      //                             &_su, suID ) ;
      PD_LOG ( PDDEBUG, "load RBS cs %s with rc:%d", SDB_DMSRBS_NAME, rc ) ;

      if ( SDB_DMS_CS_NOTEXIST == rc )
      {
         UINT32 pageSize ;

#if SMALL_CAP
         pageSize = DMS_PAGE_SIZE4K ;
#else
         pageSize = DMS_PAGE_SIZE_MAX ;
#endif
         // Rollback Segment not exist, create one
         PD_LOG ( PDDEBUG, "Creating RBS cs %s.", SDB_DMSRBS_NAME ) ;

         rc = rtnCreateCollectionSpaceCommand( SDB_DMSRBS_NAME, NULL, _dmsCB,
                                               dpsCB, UTIL_UNIQUEID_NULL,
                                               pageSize,
                                               DMS_DO_NOT_CREATE_LOB,
                                               DMS_STORAGE_CAPPED, TRUE ) ;

         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to create RBS collectionspace, rc: %d",
                  rc ) ;
            goto error ;
         }

         rc = rtnCollectionSpaceLock ( SDB_DMSRBS_NAME, _dmsCB, TRUE,
                                       &_su, suID ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to get collection space and lock for %s, "
                     "rc: %d", SDB_DMSTEMP_NAME, rc ) ;
            goto error ;
         }

         // now create the first RBSCL, but need to build the options first
         rc = _initRBSCS( eduCB, dpsCB ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to create initial RBS CLs: %d",
                    rc ) ;
            goto error ;
         }

      }
      else if ( SDB_OK == rc )
      {
         // verify SYSRBS0000 exist, otherwise recreate one.  It's possible
         // previous init was able to create the CS but for whatever reason
         // failed to create CL. We will simply creat it here
         CHAR clLongName[100];
         ossSnprintf ( clLongName, sizeof(clLongName), 
                       DMS_RBS_NAME_PATTERN,
                       SDB_DMSRBS_FULLNAME, DMS_META_RBS_CL ) ;
         rc = rtnFindCollection( clLongName, _dmsCB ) ;
         if ( SDB_DMS_NOTEXIST == rc )
         {
#ifdef _DEBUG
            PD_LOG ( PDDEBUG, "Creating RBS CLs in %s. ",
                  _metaCLName ) ;
#endif
            rc = _initRBSCS( eduCB, dpsCB ) ;
         }

         if ( SDB_OK != rc )
         {
            PD_LOG ( PDERROR, "Failed to find or recreate RBS meta, rc: %d",
                     rc ) ;
            goto error ;
         }

         rc = _dmsCB->nameToSUAndLock ( SDB_DMSRBS_NAME, suID, &_su ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG ( PDERROR, "Failed to lock SU, rc: %d",
                     rc ) ;
            goto error ;
         }

#ifdef _DEBUG
         PD_LOG ( PDDEBUG, "loading up RBS in memory structures from %s.",
                  _metaCLName ) ;
#endif
         // RBS CS already exist during start up.
         // based on previously saved value, setup in memory counter
         rc = loadMeta();
         if ( SDB_OK != rc )
         {
            PD_LOG ( PDERROR, "Failed to load meta records from %s, rc: %d",
                     _metaCLName, rc ) ;
            goto error ;
         }
      }
      else
      {
         PD_LOG ( PDERROR, "Failed to load RBS CS, rc: %d",
                  rc ) ;
         goto error ;
      }

   done :
      if ( DMS_INVALID_CS != suID )
      {
         _dmsCB->suUnlock ( suID ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSRBSSUMGR_INIT, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSRBSSUMGR_FINI, "_dmsRBSSUMgr::fini" )
   SINT32  _dmsRBSSUMgr::fini()
   {
      SINT32              rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB__DMSRBSSUMGR_FINI );
      // Write out the in-memory RBS hash table to meta data records
      // before destroying all in memory structure.
      // They are loaded into hashbucket during init (see loadMeta)
      SDB_DPSCB    *dpsCB = pmdGetKRCB()->getDPSCB() ;
      rc = flushMeta( DMS_MAX_RBS_CL, DMS_MAX_RBS_CL, dpsCB,
                      DMS_RBS_FLUSH_OPTION_HASHBKT ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Flush of RBS in memory meta record failed, rc=%d",
                   rc ) ;
   done :
      PD_TRACE_EXITRC ( SDB__DMSRBSSUMGR_FINI, rc );
      return rc ;
   error :
      // assert on any failure
      SDB_ASSERT( FALSE, "RBSMgr fini failed" ) ;
      
      goto done ;
   }

   // Create meta CL and first CL for SYSRBS
   // Caller must hold collectionspace lock
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSRBSSUMGR__INITRBSCS, "_dmsRBSSUMgr::_initRBSCS" )
   SINT32 _dmsRBSSUMgr::_initRBSCS( pmdEDUCB *eduCB, SDB_DPSCB *dpsCB ) 
   {
      SINT32              rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB__DMSRBSSUMGR__INITRBSCS );
      CHAR     clName[30] = {0} ;
      UINT16   collectionID = DMS_INVALID_CLID ;

      try
      {
         BSONObj        extOptions ;
         BSONObjBuilder builder ;
         UINT32         logicalID      = DMS_INVALID_CLID ;

         builder.append( FIELD_NAME_SIZE, DMS_CAP_EXTENT_SZ ) ;
         builder.append( FIELD_NAME_MAX, 0 ) ;
         builder.appendBool( FIELD_NAME_OVERWRITE, FALSE ) ;
         extOptions = builder.done() ;

         // Add the collection for meta data
         rc = _su->data()->addCollection ( _metaCLName,
                                           &collectionID,
                                           UTIL_UNIQUEID_NULL,
                                           DMS_MB_ATTR_CAPPED |
                                           DMS_MB_ATTR_NOIDINDEX,
                                           eduCB, dpsCB, 0, TRUE,
                                           UTIL_COMPRESSOR_INVALID,
                                           &logicalID,
                                           &extOptions ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to add RBS meta collection %s, rc: %d",
                     clName, rc ) ;
            goto error ;
         }
         PD_LOG ( PDDEBUG, "Created RBS collection %d successfully.",
                  logicalID );

         builder.append( FIELD_NAME_SIZE, _maxCollectionSize ) ;
         builder.append( FIELD_NAME_MAX, 0 ) ;
         builder.appendBool( FIELD_NAME_OVERWRITE, FALSE ) ;
         extOptions = builder.done() ;

         // add the first collection for RBS
         DMS_BUILD_RBS_CL_NAME( clName, DMS_FIRST_RBS_CL ) ;
         rc = _su->data()->addCollection ( clName, &collectionID,
                                           UTIL_UNIQUEID_NULL,
                                           DMS_MB_ATTR_CAPPED |
                                           DMS_MB_ATTR_NOIDINDEX,
                                           eduCB, dpsCB, 0, TRUE,
                                           UTIL_COMPRESSOR_INVALID,
                                           &logicalID,
                                           &extOptions ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to add RBS collection %s, rc: %d",
                     clName, rc ) ;
            goto error ;
         }
         // setup the curCL
         _currentCollection = DMS_FIRST_RBS_CL;
         PD_LOG ( PDDEBUG, "Created RBS collection %s(%d) successfully.",
                  clName, logicalID );

         // first meta record, 1 as the curcl and 0 as the last free cl
         // we don't write LR for this initial setup for the same reason
         // mentioned above
         rc = _insertMeta( DMS_FIRST_RBS_CL, DMS_META_RBS_CL,
                           NULL, dpsCB ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to insert RBS meta record, rc: %d",
                     rc ) ;
            goto error ;
         }

      }
      catch( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Occur exception when creating RBSCL: %s",
                 e.what() ) ;
         goto error ;
      }

      
   done :
      PD_TRACE_EXITRC ( SDB__DMSRBSSUMGR__INITRBSCS, rc );
      return rc ;
   error :
      goto done ;
   }

   // mbLatch of metaCL must be held in X or context is NULL which means
   // this is during init, there is no concurrent access
   // Note that there is ONLY ONE record in the meta CL. Because there is
   // no update interface for CAPPED CL, it's caller's responsibility to
   // pop the existing record before inserting the new one during update.
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSRBSSUMGR__INSERTMETA, "_dmsRBSSUMgr::_insertMeta" )
   SINT32 _dmsRBSSUMgr::_insertMeta ( UINT16 curCL, 
                                      UINT16 lastFree,
                                      dmsMBContext *context,
                                      SDB_DPSCB    *dpsCB )
   {
      PD_TRACE_ENTRY ( SDB__DMSRBSSUMGR__INSERTMETA );
      SINT32              rc = SDB_OK;
      BSONObj     metaRecord ;
      BSONObjBuilder builder ;
      pmdEDUCB       * eduCB = pmdGetThreadEDUCB() ;

      try
      {
         builder.append( FIELD_NAME_CUR_RBS_CL, curCL ) ;
         builder.append( FIELD_NAME_LAST_FREE_RBS_CL, lastFree ) ;
         metaRecord = builder.done() ;

         // insert the meta record
         rc = _su->insertRecord( _metaCLName, metaRecord, eduCB,
                                 dpsCB, TRUE, TRUE, context ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to insert  RBS meta record, rc: %d",
                     rc ) ;
            goto error ;
         }

      }
      catch( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Occur exception when inserting RBS meta: %s",
                 e.what() ) ;
         goto error ;
      }
#ifdef _DEBUG
      PD_LOG ( PDDEBUG, "Added meta record to RBS collection %s, rc: %d",
               _metaCLName, rc ) ;
#endif
   done:
      PD_TRACE_EXITRC ( SDB__DMSRBSSUMGR__INSERTMETA, rc );
      return rc ;
   error:
      goto done ;
   }

   // retrieve the meta record from SYSRBS000
   // Caller should hold mbLock
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSRBSSUMGR__GETMETA, "_dmsRBSSUMgr::_getMeta" )
   SINT32 _dmsRBSSUMgr::_getMeta ( UINT16 &curCL,
                                   UINT16 &lastFreeCL,
                                   dmsMBContext *context  )
   {
      PD_TRACE_ENTRY ( SDB__DMSRBSSUMGR__GETMETA );
      SINT32      rc         = SDB_OK ;
      pmdEDUCB   *eduCB      = pmdGetThreadEDUCB() ;
      _mthRecordGenerator generator ;
      dmsRecordID   recordID ;
      ossValuePtr   recordDataPtr = 0 ;
      dmsRecordData recordData ;

      // Since we have only 1 record, do a tablescan is efficient enough
      dmsTBScanner tbScanner( _su->data(), context, NULL,
                              DMS_ACCESS_TYPE_UPDATE, 1 ) ;

      SDB_ASSERT( context && context->isMBLock(), "mbLock must be held" ) ;

      rc = tbScanner.advance( recordID, generator, eduCB ) ;
      if ( SDB_OK == rc )
      {
         // retrieve the data
         generator.getDataPtr( recordDataPtr ) ;

         recordData.setData( (const CHAR*)recordDataPtr,
                             *(UINT32*)recordDataPtr,
                             UTIL_COMPRESSOR_INVALID, TRUE ) ;

         try
         {
            BSONObj obj ( recordData.data() ) ;
            curCL = obj.getField ( FIELD_NAME_CUR_RBS_CL ).numberInt() ;
            lastFreeCL = obj.getField(FIELD_NAME_LAST_FREE_RBS_CL).numberInt();

#ifdef _DEBUG
            PD_LOG ( PDDEBUG,
                     "Retrieved RBS meta record, (curCL=%d,lastFreeCL=%d)",
                     curCL, lastFreeCL ) ;
#endif
         }
         catch( std::exception &e )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Occur exception when retrieving RBS meta: %s",
                    e.what() ) ;
            goto error ;
 
         }
      }
      else  // failed on advance
      {
         if ( SDB_DMS_EOC == rc )
         {
            // try our best to handle error. If there is no meta record, we
            // will simply create one
            curCL = DMS_FIRST_RBS_CL ;
            lastFreeCL = DMS_META_RBS_CL ;
            PD_LOG ( PDWARNING,
                     "No RBS meta record, insert one with default value" ) ;
            // first meta record, 1 as the curcl and 0 as the last free cl
            // this basically reset RBS, no need to log
            rc = _insertMeta( DMS_FIRST_RBS_CL, DMS_META_RBS_CL, context, NULL ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to insert RBS meta record, rc: %d",
                        rc ) ;
               goto error ;
            }
         }
         else
         {
            PD_LOG ( PDERROR,
                     "Failed to query RBS meta record, rc: %d, rid(%d, %d)",
                     rc, recordID._extent, recordID._offset ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC ( SDB__DMSRBSSUMGR__GETMETA, rc );
      return rc ;
   error:
      goto done ;
   }

   // Go through each records in the RBSCL and update hash bucket entries
   // Caller should hold the metaCL latch exclusively
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSRBSSUMGR__REBUILDHASHBKTFROMCL, "_dmsRBSSUMgr::_rebuildHashBktFromCL" )
   SINT32 _dmsRBSSUMgr::_rebuildHashBktFromCL( dmsMBContext *context )
   {
      PD_TRACE_ENTRY ( SDB__DMSRBSSUMGR__REBUILDHASHBKTFROMCL );
      SINT32        rc         = SDB_OK ;
      pmdEDUCB     *eduCB      = pmdGetThreadEDUCB() ;
      _mthRecordGenerator generator ;
      dmsRecordID   recordID ;
      ossValuePtr   recordDataPtr = 0 ;
      dmsRecordData recordData ;
      BSONElement   eleLsnOffset;
      BSONElement   eleKey;
      DPS_LSN_OFFSET recordLSNOffset ;
      UINT32         bkt ;
      dmsRBSOffset   newOffset ;
      _dmsStorageDataCapped *sd = (_dmsStorageDataCapped*)_su->data() ;

      dmsTBScanner tbScanner( _su->data(), context, NULL,
                              DMS_ACCESS_TYPE_QUERY, -1 ) ;

      newOffset._clID = context->mbID() ;

      do
      {

         rc = tbScanner.advance( recordID, generator, eduCB ) ;
         if ( SDB_OK == rc )
         {
            // retrieve the data
            generator.getDataPtr( recordDataPtr ) ;

            recordData.setData( (const CHAR*)recordDataPtr,
                                *(UINT32*)recordDataPtr,
                                UTIL_COMPRESSOR_INVALID, TRUE ) ;

            try
            {
               BSONObj obj ( recordData.data() ) ;
               eleLsnOffset =
                     obj.getField(FIELD_NAME_RBS_RECORD_LSN_OFFSET) ;
               eleKey = obj.getField( FIELD_NAME_RBS_RECORD_KEY ) ;
               vector< BSONElement > vecKey = eleKey.Array() ;

               recordLSNOffset = eleLsnOffset.numberLong();
               bkt = _hash( vecKey[0].numberInt(),
                            vecKey[1].numberInt(), 
                            recordLSNOffset ) ;

               sd->_extLidAndOffset2RecLid( recordID._extent, 
                                            recordID._offset, 
                                            newOffset._logicalID );
               // update hashbucket here
               _rbsRecordBkt.lock( bkt );
               _rbsRecordBkt.setOffset( newOffset, bkt );
               _rbsRecordBkt.release( bkt ) ;

#ifdef _DEBUG
               PD_LOG ( PDDEBUG,
                        "Retrieved RBS record for bkt(%d), rid(%d, %d), "
                        "logicalID(%llu), lsn(%llu)",
                        bkt, recordID._extent, recordID._offset,
                        newOffset._logicalID, recordLSNOffset ) ;
#endif
            }
            catch( std::exception &e )
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "Occur exception when retrieving RBS meta: %s",
                       e.what() ) ;
               goto error ;

            }


         }
         else  // failed on advance
         {
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK ;
               break ;
            }
            else
            {
               PD_LOG ( PDERROR,
                        "Failed to retrieved RBS record, rid(%d, %d), rc=%d",
                        recordID._extent, recordID._offset, rc ) ;
               goto error ;
            }
         }
      } while(TRUE) ;
#ifdef _DEBUG
      PD_LOG ( PDDEBUG,
               "Finished updating hashbucket using RBSCL(%d). rc=%d",
               context->mbID(), rc ) ;
#endif

   done:
      PD_TRACE_EXITRC ( SDB__DMSRBSSUMGR__REBUILDHASHBKTFROMCL, rc );
      return rc ;

   error:
      goto done ;
   }
   
   // Function to find out current CL, and load them into memory 
   // Exclusive latch is taken in this function
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSRBSSUMGR_LOADMETA, "_dmsRBSSUMgr::loadMeta" )
   SINT32 _dmsRBSSUMgr::loadMeta ( )
   {
      PD_TRACE_ENTRY ( SDB__DMSRBSSUMGR_LOADMETA );
      SINT32      rc         = SDB_OK ;
      UINT16      curCL      = DMS_FIRST_RBS_CL ;
      UINT16      lastFreeCL = 0 ;
      BOOLEAN     mbLatched  = FALSE ;
      dmsMBContext *metaContext  = NULL ;
      dmsMBContext *context  = NULL ;
      CHAR        clName[30] ;

      // take mbLock here, and pass down the context
      rc = _su->data()->getMBContext( &metaContext, _metaCLName, EXCLUSIVE ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to get RBS mbLock, rc: %d",
                  rc ) ;
         goto error ;
      }
      mbLatched = TRUE ;

      rc  = _getMeta( curCL, lastFreeCL, metaContext ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to get RBS meta record, rc: %d",
                  rc ) ;
         goto error ;
      }

      // Update _currentCollection and _lastFreeCollection
      _currentCollection = curCL ;
      _lastFreeCollection = lastFreeCL ;
      PD_LOG ( PDDEBUG, "Successfully set up meta: curCL=%d, lastFreeCL=%d",
               curCL, lastFreeCL ) ;

      // read the rest of meta records and load up the in memory bucket
      rc = loadHashBkt( metaContext ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to load RBS hash bucket, rc: %d",
                  rc ) ;
         goto error ;
      }

      // TODO: we may want to scan the curCL to update the hash bucket with
      // records which might have not be reflected in the meta record on disk
      DMS_BUILD_RBS_CL_NAME( clName, curCL ) ;
      rc = _su->data()->getMBContext( &context, clName, SHARED ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to get mbLatch for %s, rc=%d",
                  clName, rc ) ;
         goto error ;
      }

      PD_LOG ( PDDEBUG, "Updating hash bucket with %s", clName ) ;

      rc = _rebuildHashBktFromCL( context ) ;
      _su->data()->releaseMBContext( context ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, 
                  "Failed to rebuild RBS hash bucket using %s, rc: %d",
                  clName, rc ) ;
         goto error ;
      }

   done:
      if ( mbLatched )
      {
         _su->data()->releaseMBContext( metaContext ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSRBSSUMGR_LOADMETA, rc );
      return rc ;
   error:
      goto done ;
   }

   // Based on the option passed in, we may update records in SYSRBS.SYSRBS000:
   // - Provided curCL and lastFreeCL, update the first meta data 
   // - Flush hash bucket to second meta data record (could be divided to 
   //   multiple records in the future)
   // If context is passed in, the caller should already hold mbLatch of 
   // SYSRBS000 for concurrency control.
   // Otherwise, mbLatch is taken exclusively in the function. Note that 
   // caller can set any one or both value to DMS_MAX_RBS_CL so we will pick
   // the in memory value to flush to disk.
   // Keep in mind that if lush curCL and lastFreeCL, we will flush hash bucket
   // as well. In the future, we may want to implement update interface for 
   // cappedCL, by which time we can update record inplace
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSRBSSUMGR_FLUSHMETA, "_dmsRBSSUMgr::flushMeta" )
   SINT32 _dmsRBSSUMgr::flushMeta( UINT16        curCL,
                                   UINT16        lastFreeCL,
                                   SDB_DPSCB    *dpsCB,
                                   UINT16        flushOption,
                                   dmsMBContext *context )
   {
      SINT32         rc          = SDB_OK;
      PD_TRACE_ENTRY ( SDB__DMSRBSSUMGR_FLUSHMETA );
      dmsMBContext  *metaContext = NULL ;
      BOOLEAN        mbLocked    = FALSE ;
      pmdEDUCB      *eduCB       = pmdGetThreadEDUCB() ;
      INT64          logicalID   = 0 ; 
      _dmsStorageDataCapped *sd  = (_dmsStorageDataCapped*)_su->data();
      utilInsertResult insertResult ;

      if ( NULL == context )
      {
         rc = sd->getMBContext( &metaContext, _metaCLName, EXCLUSIVE ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to get meta mbLock, rc: %d",
                     rc ) ;
            goto error ;
         }
         mbLocked = TRUE ;

         // When caller(like GC or fini) invoke the function without mbLatch,
         // it meant to flush out the most update to date value to disk.
         // We must recheck/set under latch so we don't overwrite most 
         // updated in memory one. 
         if ( DMS_MAX_RBS_CL == curCL )
         {
            curCL = _currentCollection ;
         }
         if ( DMS_MAX_RBS_CL == lastFreeCL )
         {
            lastFreeCL = _lastFreeCollection ;
         }
      }
      else
      {
         metaContext = context ;
      }

      // if flush curCL and lastFreeCL, we will pop and flush both.
      if ( DMS_RBS_FLUSH_OPTION_MASK & DMS_RBS_FLUSH_OPTION_COLLECTIONS )
      {
         // pop all record for now
         rc = sd->popRecord( metaContext, logicalID, 
                             eduCB, dpsCB, -1 ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to pop RBS meta record, rc: %d",
                     rc ) ;
            goto error ;
         }

         // insert the first record with new value
         rc = _insertMeta( curCL, lastFreeCL,
                           metaContext, dpsCB ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR,
                     "Failed to insert back RBS meta record orig(%d, %d), "
                     "new(%d, %d) rc: %d",
                     _currentCollection, _lastFreeCollection, 
                     curCL, lastFreeCL, rc ) ;
            goto error ;
         }
         _currentCollection = curCL;
         _lastFreeCollection = lastFreeCL ;
      }
      else if ( DMS_RBS_FLUSH_OPTION_HASHBKT == 
                (DMS_RBS_FLUSH_OPTION_MASK & DMS_RBS_FLUSH_OPTION_HASHBKT) )
      {
         // we will only flush the in memory bkt, keep first record untouched
         // as we should have maintained it through flushMeta() all the time.
         // Only pop the record holding hash bucket already
         logicalID = _rbsRecordBkt.getLogicalID() ;
         if ( DMS_INVALID_REC_LOGICALID != logicalID )
         {
            rc = _su->data()->popRecord( metaContext, logicalID, 
                                         eduCB, dpsCB, -1 ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to pop RBS meta record, rc: %d",
                        rc ) ;
               goto error ;
            }
         }
      }
      else
      {
         PD_LOG( PDERROR, "Invalid flush option: %d",
                 flushOption ) ;
         
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      // now build record for hashbkt and insert
      try
      {
         BSONObj        record ;
         BSONObjBuilder builder ;

         builder.appendBinData( FIELD_NAME_RBS_HASH_BKT, 
                                _rbsRecordBkt.getObjSize(), 
                                BinDataGeneral, 
                                _rbsRecordBkt.getObj() ) ;
         record = builder.obj() ;

         PD_LOG ( PDDEBUG, 
                  "flush out hashbucket, size=%d, bsonojbsize=%d",
                  _rbsRecordBkt.getObjSize(), record.objsize() ) ;

         // insert the meta record
         rc = _su->insertRecord( _metaCLName, record, eduCB,
                                 dpsCB, TRUE, TRUE, metaContext,
                                 -1, &insertResult ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to flush RBS hash bucket, rc: %d",
                     rc ) ;
            goto error ;
         }

         // convert physical address to logical localtion
         {
            SINT32      ext, offset ;
            INT64       location = DMS_INVALID_REC_LOGICALID ;
            const dmsExtent *extent = NULL ;
            dmsExtRW    extRW ;
            insertResult.getInsertLoc( ext, offset ) ;

            extRW = sd->extent2RW( ext, metaContext->mbID() ) ;
            extRW.setNothrow( TRUE ) ;
            extent = extRW.readPtr<dmsExtent>() ;
 
            sd->_extLidAndOffset2RecLid( extent->_logicID, offset, 
                                         location ) ;

            PD_LOG ( PDDEBUG, 
                     "flushed hash bucket to record(%d, %d), extlid=%d, "
                     "logicID=%llu",
                     ext, offset, extent->_logicID, location) ;

            // update the logicalID
            _rbsRecordBkt.setLogicalID(location) ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, 
                 "Occur exception in flushing hash bucket: %s",
                 e.what() ) ;
         rc = pdGetLastError() ? pdGetLastError() : SDB_SYS ;
         goto error ;
      }

   done:
      if ( mbLocked )
      {
         metaContext->mbUnlock() ;
      }

      PD_TRACE_EXITRC ( SDB__DMSRBSSUMGR_FLUSHMETA, rc );
      return rc ;
   error:
      goto done ;
   }

   // read record from SYSRBS0000 and load into memory hash bucket
   // caller must hold metaMBLatch in X
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSRBSSUMGR_LOADHASHBKT, "_dmsRBSSUMgr::loadHashBkt" )
   SINT32 _dmsRBSSUMgr::loadHashBkt( dmsMBContext *metaContext ) 
   {
      SINT32         rc          = SDB_OK;
      PD_TRACE_ENTRY ( SDB__DMSRBSSUMGR_LOADHASHBKT );
      pmdEDUCB      *eduCB       = pmdGetThreadEDUCB() ;
      INT64          location    = DMS_INVALID_OFFSET;
      _dmsStorageDataCapped *sd  = (_dmsStorageDataCapped*)_su->data();
      
      // only load if number of meta record on disk is vallid and we haven't
      // load before
      if ( DMS_RBS_NUM_META_RECORDS == metaContext->mbStat()->_totalRecords &&
           (DMS_INVALID_OFFSET == _rbsRecordBkt.getLogicalID()) )
      {
         // FIXME: this is a shortcut because we only has 2 record, that's
         // why we can use the _lastRecordOffset. Otherwise we have to get
         // use firstRecordOffset+recordsize
         dmsExtentID   extID      = DMS_INVALID_EXTENT ;
         dmsOffset     offset     = DMS_INVALID_OFFSET;

         dmsExtentInfo *extInfo = 
                         sd->getWorkExtInfo( metaContext->mbID() ) ;
         extID = extInfo->_id ;
         offset = extInfo->_lastRecordOffset ;
         sd->_extLidAndOffset2RecLid( extInfo->_extLogicID, 
                                      offset, location ) ;

         {
            dmsRecordID   recordID( extID, offset ) ;
            BSONObj       cappedRecord ;
            BSONElement   bktEle ;
            const CHAR*   binData ;
            INT32         dataLen = 0 ;

            // FIXME remove
            PD_LOG( PDDEBUG,
                 "loading RBS hash bucket from metarecord logicalid %llu,"
                 "rid(%d, %d)",
                 location, extID, offset ) ;

            rc = sd->fetch( metaContext, recordID, cappedRecord, eduCB, FALSE ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, 
                        "Failed to fetch rbs meta record at %llu, rc=%d",
                        location, rc ) ;
               goto error ;
            }

            bktEle = cappedRecord.getField( FIELD_NAME_RBS_HASH_BKT );
            // TODO:  Once stable, change following two assert to PD_CHECK
            SDB_ASSERT( BinData == bktEle.type(),
                        "Hash bkt data type not match" ) ;
            binData = bktEle.binData( dataLen ) ;
            if ( dataLen != _rbsRecordBkt.getObjSize() )
            {
               PD_LOG( PDERROR,
                       "hashbkt size(%d) does not real len from meta "
                       "record(%d)",
                       _rbsRecordBkt.getObjSize(), dataLen ) ;
               SDB_ASSERT( FALSE,
                          "hashbkt size does not match meta record" ) ;
            }

            ossMemcpy( _rbsRecordBkt.getObj(), binData,
                       _rbsRecordBkt.getObjSize() ) ;
         }

         // update 
         _rbsRecordBkt.setLogicalID(location) ;
      }
      else
      {
         // either first fresh init time, which means we never created record
         // for the hash bucket, or bucket was loaded
         PD_LOG( PDDEBUG,
                 "RBS hash bucket was not loaded, number of metarecord=%d,"
                 "in memory logicalID %llu",
                 metaContext->mbStat()->_totalRecords,
                 _rbsRecordBkt.getLogicalID() ) ;
      }
   done:
      
      PD_TRACE_EXITRC ( SDB__DMSRBSSUMGR_LOADHASHBKT, rc );
      return rc ;
   error:
      goto done ;
   }

/*
   // Provided curCL and lastFreeCL, update the meta data record in SYSRBS.SYSRBS000
   // The caller should already hold mbLock of SYSRBS000 for concurrency control
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSRBSSUMGR__UPDATEMETA1, "_dmsRBSSUMgr::_updateMeta" )
   SINT32 _dmsRBSSUMgr::_updateMeta ( UINT16        curCL,
                                      UINT16        lastFreeCL,
                                      dmsMBContext *context,
                                      SDB_DPSCB    *dpsCB )
   {
      SINT32      rc         = SDB_OK;
      PD_TRACE_ENTRY ( SDB__DMSRBSSUMGR__UPDATEMETA1 );
      pmdEDUCB   *eduCB      = pmdGetThreadEDUCB() ;
      INT64       logicalID  = 0 ; // we only has 1 record

      SDB_ASSERT( context && context->isMBLock( EXCLUSIVE ), "mbLock must be held in X") ;

      // This is cap cs, we can only pop+insert instead of update
      rc = _su->data()->popRecord( context, logicalID, eduCB, dpsCB, -1 ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to pop RBS meta record, rc: %d",
                  rc ) ;
         goto error ;
      }

      // insert the record with new value
      rc = _insertMeta( curCL, lastFreeCL, context, dpsCB ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR,
                  "Failed to insert back RBS meta record (%d, %d), rc: %d",
                  curCL, lastFreeCL, rc ) ;
         goto error ;
      }

      // Update _currentCollection and _lastFreeCollection under mblock
      _currentCollection = curCL ;
      _lastFreeCollection = lastFreeCL ;

      PD_LOG ( PDDEBUG, "Update RBS meta record (%d, %d) successfully",
               curCL, lastFreeCL ) ;
   done:
      PD_TRACE_EXITRC ( SDB__DMSRBSSUMGR__UPDATEMETA1, rc );
      return rc ;
   error:
      goto done ;
   }
*/
   // Find the current RBS CL and make sure it has enough space to append
   // a record with specified size. If the current one run out of space,
   // this function will automatically move to next CL.
   //
   // On normal return, the context for current RBSCL will be latched in X
   // and returned.
   // On error, nothing is held.
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSRBSSUMGR__PREPARERBSCLFORRECORD, "_dmsRBSSUMgr::_prepareRBSCLForRecord" )
   SINT32 _dmsRBSSUMgr::_prepareRBSCLForRecord( UINT32          recordSize,
                                                pmdEDUCB     *  eduCB,
                                                SDB_DPSCB    *  dpsCB,
                                                dmsMBContext *& clContext )
   {
      PD_TRACE_ENTRY ( SDB__DMSRBSSUMGR__PREPARERBSCLFORRECORD);
      SINT32       rc           = SDB_OK ;
      CHAR         clName[30]   = {0} ;
      CHAR         mclName[30]  = {0} ;
      BOOLEAN      mbLocked     = FALSE ;
      BOOLEAN      metaMBLocked = FALSE ;
      dmsMBContext *metaContext = NULL ;
      _dmsStorageDataCapped *sd = (_dmsStorageDataCapped*)_su->data();

      // TODO: need protection to lookup _currentCollection

      // latch and lookup curCL for space first
      DMS_BUILD_RBS_CL_NAME( clName, _currentCollection ) ;

      rc = _su->data()->getMBContext( &clContext, clName, EXCLUSIVE ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to get curCL mbLock, rc: %d",
                  rc ) ;
         goto error ;
      }
      mbLocked = TRUE ;

      //if( !sd->spaceEnough( clContext, recordSize ) )
      if( !sd->clDataSpaceEnough( clContext, recordSize ) )
      {
         // current CL does NOT have enough space, create the new CL
         BSONObjBuilder builder ;
         BSONObj        extOptions ;
         UINT32         logicalID    = DMS_INVALID_CLID ;
         UINT16         collectionID = DMS_INVALID_MBID ;

#ifdef _DEBUG
         {
            const dmsMBStatInfo *mbStatInfo =
                                   sd->getMBStatInfo( clContext->mbID() ) ;
            PD_LOG ( PDINFO,
                  "Out of space in %s, allocating next one. recordsize(%d),"
                  "clfreespace(%d), clTotalPages(%d), cltotalrecord(%d),"
                  "cl max(%lld), squareroot(%d)",
                  clName, recordSize,
                  mbStatInfo->_totalDataFreeSpace,
                  mbStatInfo->_totalDataPages,
                  mbStatInfo->_totalRecords,
                  (UINT64)sd->_options[clContext->mbID()]->_maxSize,
                  sd->pageSizeSquareRoot() ) ;
         }
#endif
         // release previous context
         _su->data()->releaseMBContext( clContext ) ;
         mbLocked = FALSE ;

         // create next cl
         _currentCollection++ ;
         // if reached max, wrap to first one.
         if ( _currentCollection >= DMS_MAX_RBS_CL )
         {
            _currentCollection = DMS_FIRST_RBS_CL ;
         }

         // take metaCL mbLatch in X so that no one read stale data
         rc = _su->data()->getMBContext( &metaContext, _metaCLName, EXCLUSIVE ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to lock RBS meta collection %s, rc: %d",
                     mclName, rc ) ;
            goto error ;
         }
         metaMBLocked = TRUE ;

         // create next CL
         try
         {
            builder.append( FIELD_NAME_SIZE, _maxCollectionSize ) ;
            builder.append( FIELD_NAME_MAX, 0 ) ;
            builder.appendBool( FIELD_NAME_OVERWRITE, FALSE ) ;
            extOptions = builder.done() ;

            // add the first collection for RBS
            DMS_BUILD_RBS_CL_NAME( clName, _currentCollection ) ;

            rc = _su->data()->addCollection ( clName, &collectionID,
                                              UTIL_UNIQUEID_NULL,
                                              DMS_MB_ATTR_CAPPED |
                                              DMS_MB_ATTR_NOIDINDEX,
                                              eduCB, dpsCB, 0, TRUE,
                                              UTIL_COMPRESSOR_INVALID,
                                              &logicalID,
                                              &extOptions ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to add RBS collection %s, rc: %d",
                        clName, rc ) ;
               goto error ;
            }
            PD_LOG ( PDDEBUG, "Successfully created RBS collection %s, logicalID= %d",
                     clName, logicalID ) ;
         }
         catch( std::exception &e )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Occur exception when adding RBSCL : %s",
                    e.what() ) ;
            goto error ;
         }

         // move to next CL, flush out meta records  so that replica node
         // can replay this update to its side. We will flush hashbkt
         // to keep the hashbkt as closely updated as the cur/last.
         rc = flushMeta( _currentCollection, _lastFreeCollection, dpsCB,
                         DMS_RBS_FLUSH_OPTION_COLLECTIONS, 
                         metaContext ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR,
                     "Failed to update RBS meta record from %s with (%d,%d) rc: %d",
                     clName, _currentCollection, _lastFreeCollection, rc ) ;
            goto error ;
         }
         metaContext->mbUnlock() ;
         metaMBLocked = FALSE ;

         dmsStartAsyncRBSGC() ;

         DMS_BUILD_RBS_CL_NAME( clName, _currentCollection ) ;
         // get curCL context and take mbLock here
         rc = _su->data()->getMBContext( &clContext, clName, EXCLUSIVE ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to get curCL mbLock, rc: %d", rc ) ;
            goto error ;
         }
         mbLocked = TRUE ;
      } // end of spaceEnough

   done:
      PD_TRACE_EXITRC ( SDB__DMSRBSSUMGR__PREPARERBSCLFORRECORD, rc );
      return rc ;

   error:
      if ( metaMBLocked )
      {
         metaContext->mbUnlock() ;
      }
      if ( mbLocked )
      {
         clContext->mbUnlock() ;
      }
      goto done ;
   }

   // allocate space for RBS record and return the beginning offset
   // TODO: current hold metaCL mblatch in X, consider X next
   // Dependency: caller must hold metaCL mblatch S so we are save use curCL
   // Will take curCL mblatch to see its logical ID, find if there is
   // enough space. Return the logical ID if there is and increase
   //    logical ID by size. Otherwise move to next CL.
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSRBSSUMGR__ALLOCRBSRECORDSPACE, "_dmsRBSSUMgr::_allocRBSRecordSpace" )
   SINT32 _dmsRBSSUMgr::_allocRBSRecordSpace( UINT32        size,
                                              dmsRBSOffset &newOffset,
                                              pmdEDUCB     *eduCB,
                                              SDB_DPSCB    *dpsCB,
                                              dmsMBContext *metaContext,
                                              dmsMBContext *&clContext )
   {
      PD_TRACE_ENTRY ( SDB__DMSRBSSUMGR__ALLOCRBSRECORDSPACE );
      SINT32        rc         = SDB_OK ;
      CHAR          clName[30] = {0} ;
      _dmsStorageDataCapped *sd = (_dmsStorageDataCapped*)_su->data();
      dmsRecordID   foundRID ;
      BOOLEAN       mbLocked   = FALSE ;

      // mblatch will be held after this call
      rc = _prepareRBSCLForRecord( size, eduCB, dpsCB, clContext ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR,
                  "Failed to prepare space (size=%d), rc: %d, curCL:%d",
                  size, rc, _currentCollection ) ;
         goto error ;
      }
      mbLocked = TRUE ;

      // now current CL has enough space. reserve the space
      _su->data()->_allocRecordSpace( clContext, size, foundRID, eduCB ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR,
                  "Failed to allocate space in %s (size=%d), rc: %d",
                  clName, size, rc ) ;
         goto error ;
      }

      sd->_extLidAndOffset2RecLid( foundRID._extent, foundRID._offset,
                                   newOffset._logicalID );
      newOffset._clID = _currentCollection ;

      PD_LOG ( PDDEBUG,
               "Found space in %s (size=%d) for new record at %lld",
               clName, size, newOffset._logicalID ) ;

   done:
      if ( mbLocked )
      {
         // only unlatch, do not release the context
         clContext->mbUnlock() ;
      }
      PD_TRACE_EXITRC ( SDB__DMSRBSSUMGR__ALLOCRBSRECORDSPACE, rc );
      return rc ;

   error:
      goto done ;
   }
#if 0
   // write rbs record to the location specified
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSRBSSUMGR__WRITERECTOLOCATION, "_dmsRBSSUMgr::_writeRecToLocation" )
   SINT32 _dmsRBSSUMgr::_writeRecToLocation( dmsRBSOffset         location,
                                             dmsStorageUnitID     csid,
                                             UINT16               clid,
                                             dmsRecordID          rid,
                                             DPS_TRANS_ID         recordTransID,
                                             const dmsRecord     *record,
                                             UINT32               recSize,
                                             pmdEDUCB            *eduCB,
                                             dmsMBContext        *context )
   {
      PD_TRACE_ENTRY( SDB__DMSRBSSUMGR__WRITERECTOLOCATION ) ;
      INT32                  rc        = SDB_OK ;
      dmsRecordRW            recordRW ;
      _dmsStorageDataCapped *sd        = (_dmsStorageDataCapped*)_su->data();
      _dmsRBSRecord         *rbsRecord = NULL ;
      dmsExtentID            extID     = DMS_INVALID_EXTENT ;
      dmsOffset              offset    = DMS_INVALID_OFFSET;
      UINT32                 bkt       = _hash( csid, clid, rid );
      dmsRecordData          recordData;
      recordData.setData( record->getData(), record->getDataLength(),
                          UTIL_COMPRESSOR_INVALID, TRUE ) ;

      // IO under mblock in S mode
      context->mbLock( SHARED ) ;

      dmsExtentInfo* workExtInfo = sd->getWorkExtInfo( context->mbID() ) ;

      sd->_recLid2ExtLidAndOffset( location._logicalID, extID, offset ) ;

      // Write out the record to RBS and update the offsetm, all has to be
      // done within the bkt lock. Otherwise reader could get a stale or
      // partial record
      _rbsRecordBkt.lock( bkt );

      {
         const dmsRecordID recordID( extID, offset ) ;

         recordRW = sd->record2RW( recordID, context->mbID() ) ;
         recordRW.setNothrow( TRUE ) ;
         rbsRecord = recordRW.writePtr<_dmsRBSRecord>( recSize ) ;

         rbsRecord->setRecordKey( csid, clid, rid ) ;
         rbsRecord->setGlobTransID( recordTransID ) ;
         // Update record with the previous offset
         rbsRecord->setPreOffset( _rbsRecordBkt.getOffset( bkt ) ) ;
         // copy the data
         rbsRecord->setData( recordData ) ;
         // set up the deleting attribute
         if( record->isDeleting() )
         {
            rbsRecord->setDeleting() ;
         }

         // TODO:  consider monitor related change here like:
         //DMS_MON_OP_COUNT_INC, _updateStatInfo
         sd->_updateStatInfo( context, rbsRecord->size(),
                          recordData ) ;
         if ( 1 == workExtInfo->_recCount )
         {
            dmsExtRW extRW = sd->extent2RW( extID, context->mbID() ) ;
            dmsExtent *extent = extRW.writePtr<dmsExtent>() ;
            extent->_firstRecordOffset = workExtInfo->_firstRecordOffset ;
         }
      }

      // Update bucket to point to the new record
      _rbsRecordBkt.setOffset( location, bkt );

      // unlock the bucket
      _rbsRecordBkt.release( bkt ) ;
      context->mbUnlock( ) ;

      PD_LOG ( PDDEBUG,
               "Successfully wrote record(%d, %d) to RBS%04d at location %lld(%d, %d)",
               rid._extent, rid._offset, context->mbID(),
               location._logicalID, extID, offset ) ;

      PD_TRACE_EXITRC ( SDB__DMSRBSSUMGR__WRITERECTOLOCATION, rc );
      return rc ;

   }

   // Input Parm:
   //    recordTransID:  Record version, (last creation/update trans ID)
   //    ownerTransID: transaction to put the record to in memory old version
   //                  container and now to RBS
   // Append a record to the end of the RBS, internally we will
   // 1. based on CSID+RID, hash and find the proper bucket, lock the bucket
   // 2. find correct collection, reserve space,
   // 3. append the record to the destinated collection with given offset.
   // 4. update bucket with the offset, unlock bucket
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSRBSSUMGR_APPENDRECORD, "_dmsRBSSUMgr::appendRecord" )
   SINT32 _dmsRBSSUMgr::appendRecord ( dmsStorageUnitID      csid,
                                       UINT16                clid,
                                       dmsRecordID           rid,
                                       DPS_TRANS_ID          recordTransID,
                                       DPS_TRANS_ID          ownerTransID,
                                       const dmsRecord      *record )
   {
      PD_TRACE_ENTRY ( SDB__DMSRBSSUMGR_APPENDRECORD );
      SINT32        rc          = SDB_OK ;
      dmsRBSOffset  newOffset ;
      //dmsRBSRecord *rbsRecord ;
      dmsMBContext *metaContext  = NULL ;
      dmsMBContext *clContext  = NULL ;
      BOOLEAN       mbLocked     = FALSE ;
      pmdEDUCB     *eduCB        = pmdGetThreadEDUCB() ;
      UINT32        recSize      = record->getDataLength()
                                   + DMS_RECORD_RBS_METADATA_SZ;
      recSize = ossAlignX( recSize, 4 ) ;

      // create meta context and take mbLock here
      // FIXME: for better concurrency, it's better to take S
      rc = _su->data()->getMBContext( &metaContext, _metaCLName,
                                      EXCLUSIVE ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to get RBS mbLock, rc: %d",
                  rc ) ;
         goto error ;
      }
      mbLocked = TRUE ;

      // allocate the space in RBS
      rc = _allocRBSRecordSpace( recSize, newOffset,
                                 eduCB, metaContext, clContext ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR,
                  "Failed to allocate space in RBS (size=%d), rc: %d",
                  recSize, rc ) ;
         goto error ;
      }
      SDB_ASSERT( clContext && !clContext->isMBLock( ), "mbLock must not be held ") ;

      metaContext->mbUnlock() ;
      mbLocked = FALSE ;

      // compare and update RBS's maxTransID to ownerTransID under mblock
      clContext->mbStat()->updateGlobTransIDWithComp( ownerTransID ) ;

      // write rbsrecord to RBS.  NEED something like recordRW
      // need to do this before update the offset in bucket, otherwise
      // someone else could read wrong data
      rc = _writeRecToLocation( newOffset, csid, clid, rid, recordTransID,
                                record, recSize, eduCB, clContext ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR,
                  "Failed to write record to RBS at location %d, rc: %d",
                  newOffset._logicalID, rc ) ;
         goto error ;
      }

   done:

      if ( mbLocked )
      {
         _su->data()->releaseMBContext( metaContext ) ;
      }
      if ( clContext )
      {
         _su->data()->releaseMBContext( clContext ) ;
      }

      PD_TRACE_EXITRC ( SDB__DMSRBSSUMGR_APPENDRECORD, rc );
      return  rc ;
   error:
      goto done ;
   }
#endif
   // Input Parm:
   //    recordTransID:  Record version, (last creation/update trans ID)
   //    ownerTransID: transaction to put the record to in memory old version
   //                  container and now to RBS
   // Append a record to the end of the RBS using insertRecord interface:
   // 1. based on CSID+RID, hash and find the proper bucket, lock the bucket
   // 2. build proper record
   // 3. invoke insertRecord to append the record to the destinated collection
   // 4. update bucket with the returned offset, unlock bucket
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSRBSSUMGR_APPENDRECORD1, "_dmsRBSSUMgr::appendRecord" )
   SINT32 _dmsRBSSUMgr::appendRecord ( dmsStorageUnitID      csid,
                                       UINT16                clid,
                                       DPS_LSN_OFFSET        lsn,
                                       DPS_TRANS_ID          recordTransID,
                                       DPS_TRANS_ID          ownerTransID,
                                       const BSONObj        &data )
   {
      PD_TRACE_ENTRY ( SDB__DMSRBSSUMGR_APPENDRECORD1 );
      SINT32        rc          = SDB_OK ;
      dmsRBSOffset  newOffset ;
      //dmsRBSRecord *rbsRecord ;
      dmsMBContext *clContext    = NULL ;
      CHAR          clName[30]   = {0} ;
      pmdEDUCB     *eduCB        = pmdGetThreadEDUCB() ;
      // setup dpsCB so that the replica can replay the addCollection 
      // log record. 
      SDB_DPSCB    *dpsCB = pmdGetKRCB()->getDPSCB() ;
      UINT32        bkt          = _hash( csid, clid, lsn );
      BSONObjBuilder builder ;
      utilInsertResult insertResult ;
      BSONObj       record ;
      BOOLEAN       bktLatched   = FALSE ;
      BOOLEAN       clLocked     = FALSE ;
      UINT32        recSize ;
      // type conversion for following use
      SINT32        cl           = clid ;
      //dmsRBSRecordKey recKey( csid, clid, rid ) ;
      // use array of 4 int to store on disk
      _dmsStorageDataCapped *sd = (_dmsStorageDataCapped*)_su->data();

      // Under hash bkt latch, build BSON record to include following:
      // recordKey, transID, preOffset and original recordData
      _rbsRecordBkt.lock( bkt );
      bktLatched = TRUE ;

      try
      {
         builder.append( FIELD_NAME_RBS_RECORD_KEY,
                         BSON_ARRAY( (SINT32)csid <<
                                     (SINT32)cl ) ) ;
         // has to cast to INT64
         builder.append( FIELD_NAME_RBS_RECORD_LSN_OFFSET,
                         (INT64)lsn ) ;

         // append transaction ID as BSON sub-object
         rc = dpsTransIDToBSON( recordTransID, builder,
                                FIELD_NAME_RBS_RECORD_TRANSID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build transaction ID into BSON "
                      "format, rc: %d", rc ) ;

         builder.append( FIELD_NAME_RBS_PRERECORD_CL,
                         _rbsRecordBkt.getOffset( bkt )._clID ) ;
         builder.append( FIELD_NAME_RBS_PRERECORD_OFFSET,
                         _rbsRecordBkt.getOffset( bkt )._logicalID ) ;
         builder.append( FIELD_NAME_RBS_RECORD_DATA, data ) ;
         record = builder.done() ;
         recSize = record.objsize() + DMS_RECORD_CAP_METADATA_SZ ;
         recSize = ossAlignX( recSize, 4 ) ;

         // move to proper RBS CL which has enough space, on OK return,
         // the cl is locked in X
         rc = _prepareRBSCLForRecord( recSize, eduCB, dpsCB, clContext ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to prepare RBSCL for record, rc: %d",
                     rc ) ;
            goto error ;
         }
         clLocked = TRUE ;


         // insert the record to RBS
         rc = _su->insertRecord ( clName, record, eduCB, dpsCB,
                                  TRUE, TRUE, clContext, -1, &insertResult ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to insert into RBS, rc: %d",
                     rc ) ;
            goto error ;
         }
      }
      catch( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Occur exception when insert record to RBS: %s",
                 e.what() ) ;
         goto error ;
      }
      // update the _maxGlobTransID as needed
      clContext->mbStat()->updateGlobTransIDWithComp( ownerTransID ) ;

      _su->data()->releaseMBContext( clContext ) ;
      clLocked = FALSE ;

      // Update bucket to point to the new record location
      {
         dmsRBSOffset location ;
         SINT32       ext, offset ;
         insertResult.getInsertLoc( ext, offset ) ;
         // FIXME:  is this ext correct? should it be extlid from extent
         sd->_extLidAndOffset2RecLid( ext, offset, location._logicalID ) ;
         location._clID = _currentCollection ;
         _rbsRecordBkt.setOffset( location, bkt ) ;
      }
      // unlock the bucket
      _rbsRecordBkt.release( bkt ) ;
      bktLatched  = FALSE ;

   done:
      PD_TRACE_EXITRC ( SDB__DMSRBSSUMGR_APPENDRECORD1, rc );
      return  rc ;
   error:
      if ( bktLatched )
      {
         _rbsRecordBkt.release( bkt ) ;
      }
      if ( clLocked )
      {
         _su->data()->releaseMBContext( clContext ) ;
      }
      goto done ;
   }
/*
   // Given a transactionID and beginning of a record chain, find a visiable record
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSRBSSUMGR_GETRECORD, "_dmsRBSSUMgr::getRecord" )
   SINT32 _dmsRBSSUMgr::getRecord ( dmsStorageUnitID  csid,
                                    UINT16            clid,
                                    DPS_LSN          &lsn,
                                    DPS_TRANS_ID      transid,
                                    BOOLEAN          &found,
                                    dmsRecordData    &record )
   {
      PD_TRACE_ENTRY ( SDB__DMSRBSSUMGR_GETRECORD );
      SINT32        rc         = SDB_OK ;
      UINT32        bkt        = _hash( csid, clid, lsn );
      dmsMBContext *context    = NULL ;
      dmsRecordRW   recordRW ;
      dmsRBSOffset  position ;
      CHAR          clName[30] = {0} ;
      dmsStorageDataCapped *sd = (dmsStorageDataCapped*)_su->data();
      dmsExtentID   extID      = DMS_INVALID_EXTENT ;
      dmsOffset     offset     = DMS_INVALID_OFFSET;
      dmsRBSRecordKey key( csid, clid, lsn ) ;
#ifdef _DEBUG
      PD_LOG ( PDDEBUG,
               "Transaction (%s) tries to find a proper version from RBS, "
               "csid(%d), clid(%d), record rid(%d, %d)",
               dpsTransIDToString( transid ).c_str(),
               csid, clid, rid._extent, rid._offset ) ;
#endif
      found = FALSE ;
      // 1. From the hash table, find the position
      // lock the bucket
      _rbsRecordBkt.lock( bkt );
      position = _rbsRecordBkt.getOffset( bkt );

      // When is it safe to release the latch? do we allow anybody else to
      // insert/free the position while  we got a position and are still
      // using it.
      // I "think" it should be ok as long as insert guy holds recordLock
      // in X and reader already went through the lock request but failed
      // thus decided to use a version of old record, AND the version is
      // already stored in RBS AND hasn't been recycled yet.
      _rbsRecordBkt.release( bkt ) ;

      do
      {
         // finish if the hasbucket entry is invalid
         if ( !position.isValid() )
         {
#ifdef _DEBUG
            PD_LOG ( PDDEBUG, "no more older version found" ) ;
#endif
            goto done ;
         }

         // calculate extID and offset from logicalID
         sd->_recLid2ExtLidAndOffset( position._logicalID, extID, offset ) ;

         // 2. read record from the position
         dmsRecordID   recordID( extID, offset ) ;
         const dmsRBSRecord *rbsRecord  = NULL ;

         DMS_BUILD_RBS_CL_NAME( clName, position._clID ) ;
         rc = _su->data()->getMBContext( &context, clName, SHARED ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to get mbLatch, rc=%d", rc ) ;
            goto error ;
         }

         recordRW = sd->record2RW( recordID, position._clID ) ;

         rbsRecord = recordRW.readPtr<dmsRBSRecord>() ;

         // check if the record match based on key and version
         // also check the status of the record
         if ( key == rbsRecord->getRecordKey() &&
              sdbGetTransCB()->isVersionVisible(
                    rbsRecord->getGlobTransID(), transid ) &&
              !rbsRecord->isDeleting() )
         {
            found = TRUE ;
            record.setData( rbsRecord->getData(),
                            rbsRecord->getDataLength(),
                            UTIL_COMPRESSOR_INVALID, TRUE ) ;

            SDB_ASSERT( !rbsRecord->isCompressed(),
                        "currently do not support compression in RBS") ;

#ifdef _DEBUG
            {
               BSONObj  obj(record.data()) ;
               PD_LOG ( PDDEBUG,
                        "Read record(%s) from %s at location %ld(%d, %d), "
                        "record rid(%d, %d), transid(%s)",
                        obj.toString().c_str(), clName,
                        position._logicalID,
                        extID, offset,
                        rid._extent, rid._offset,
                        dpsTransIDToString(
                                    rbsRecord->getGlobTransID() ).c_str() ) ;
            }
#endif
            _su->data()->releaseMBContext( context ) ;
            break ;
         }

         // setup next position, release mblatch and continue
         position = rbsRecord->getPreOffset() ;
         _su->data()->releaseMBContext( context ) ;
#ifdef _DEBUG
         PD_LOG ( PDDEBUG, "Moving to next position(%d, %ld)",
                  position._clID,
                  position._logicalID ) ;
#endif
         continue ;

      } while ( true );


      // 3. FIXME: update monitor counter for RBS
      // DMS_MON_OP_COUNT_INC( pMonAppCB, MON_RBS_DATA_READ, 1 ) ;

      if ( rc )
      {
         goto error ;
      }
   done:
      PD_TRACE_EXITRC ( SDB__DMSRBSSUMGR_GETRECORD, rc );
      return  rc ;
   error:
      goto done ;
   }
*/
   // Given a transactionID and beginning of a record chain, find a visiable record
   // This method uses fetch method from cappedCL
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSRBSSUMGR_GETRECORD1, "_dmsRBSSUMgr::getRecord1" )
   SINT32 _dmsRBSSUMgr::getRecord1 ( dmsStorageUnitID  csid,
                                     UINT16            clid,
                                     DPS_LSN_OFFSET    &lsn,
                                     DPS_TRANS_ID      transid,
                                     BOOLEAN          &found,
                                     dmsRecordData    &recordData )
   {
      PD_TRACE_ENTRY ( SDB__DMSRBSSUMGR_GETRECORD1 );
      SINT32        rc         = SDB_OK ;
      UINT32        bkt        = _hash( csid, clid, lsn );
      dmsMBContext *context    = NULL ;
      pmdEDUCB     *eduCB      = pmdGetThreadEDUCB() ;
      dmsRecordRW   recordRW ;
      dmsRBSOffset  position ;
      CHAR          clName[30] = {0} ;
      dmsStorageDataCapped *sd = (dmsStorageDataCapped*)_su->data();
      dmsExtentID   extID      = DMS_INVALID_EXTENT ;
      dmsOffset     offset     = DMS_INVALID_OFFSET;

#ifdef _DEBUG
      PD_LOG ( PDDEBUG,
               "Transaction (%s) tries to find a proper version from RBS, "
               "csid(%d), clid(%d), record lsn(%llu)",
               dpsTransIDToString( transid ).c_str(),
               csid, clid, lsn ) ;
#endif
      found = FALSE ;
      // 1. From the hash table, find the position
      // lock the bucket
      _rbsRecordBkt.lock( bkt );
      position = _rbsRecordBkt.getOffset( bkt );

      // When is it safe to release the latch? do we allow anybody else to
      // insert/free the position while  we got a position and are still
      // using it.
      // I "think" it should be ok as long as insert guy holds recordLock
      // in X and reader already went through the lock request but failed
      // thus decided to use a version of old record, AND the version is
      // already stored in RBS AND hasn't been recycled yet.
      _rbsRecordBkt.release( bkt ) ;

      do
      {
         // finish if the hasbucket entry is invalid
         // FIXME: is it possible that the position is pointing to a
         // version no longer exist (CL has been recycled)
         if ( !position.isValid() )
         {
#ifdef _DEBUG
            PD_LOG ( PDDEBUG, "no more older version found" ) ;
#endif
            goto done ;
         }

         // calculate extID and offset from logicalID
         sd->_recLid2ExtLidAndOffset( position._logicalID, extID, offset ) ;

         try
         {
            // 2. read record from the position
            dmsRecordID   recordID( extID, offset ) ;
            BSONObj       cappedRecord ;
            //dmsRecordData cappedRecordData ;
            DPS_TRANS_ID  recordTransID ;
            DPS_LSN_OFFSET recordLSNOffset ;
            BSONElement   eleTransID;
            BSONElement   eleLsnOffset;
            BSONElement   eleKey;

            DMS_BUILD_RBS_CL_NAME( clName, position._clID ) ;
#ifdef _DEBUG
            PD_LOG ( PDDEBUG, "Try %s at location(%d,%d) for older version",
                     clName, extID, offset  ) ;
#endif
            rc = _su->data()->getMBContext( &context, clName, SHARED ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to get mbLatch for %s, rc=%d",
                        clName, rc ) ;
               goto error ;
            }

            //rc = sd->fetch( context, recordID, cappedRecordData, eduCB ) ;
            rc = sd->fetch( context, recordID, cappedRecord, eduCB, FALSE ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, 
                        "Failed to fetch rbsrecord,rid(%d, %d), rc=%d",
                        extID, offset, rc ) ;
               goto error ;
            }

            // 3. parse the dataRecord to figure out record key and visiability
            //cappedRecord = BSONObj( cappedRecordData.data() ) ;
            eleTransID = cappedRecord.getField(FIELD_NAME_RBS_RECORD_TRANSID) ;
            eleLsnOffset = 
                     cappedRecord.getField(FIELD_NAME_RBS_RECORD_LSN_OFFSET) ;
            eleKey = cappedRecord.getField( FIELD_NAME_RBS_RECORD_KEY ) ;
            vector< BSONElement > vecKey = eleKey.Array() ;

            recordLSNOffset = eleLsnOffset.numberLong();

            // parse transaction ID
            PD_CHECK( Object == eleTransID.type(), SDB_SYS, error, PDERROR,
                      "Failed to parse transaction ID from record, field [%s] "
                      "should be an object", FIELD_NAME_RBS_RECORD_TRANSID ) ;
            rc = dpsTransIDFromBSON( eleTransID.embeddedObject(),
                                     recordTransID ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to parse transaction ID, rc: %d",
                         rc ) ;

            // 4. setup return data for qualified version
            if ( ( vecKey[0].numberInt() == csid ) &&
                 ( vecKey[1].numberInt() == clid ) &&
                 ( recordLSNOffset == lsn ) &&
                 sdbGetTransCB()->isVersionVisible( recordTransID,
                                                    transid) )
            {
               BSONElement ele =
                    cappedRecord.getField(FIELD_NAME_RBS_RECORD_DATA) ;
               found = TRUE ;
               recordData.setData( ele.value(), ele.valuesize() )  ;
#ifdef _DEBUG
               PD_LOG ( PDDEBUG, "Found version(%s) at position(%d, %ld)",
                        dpsTransIDToString( recordTransID ).c_str(),
                        position._clID,
                        position._logicalID ) ;
#endif
               _su->data()->releaseMBContext( context ) ;
               break ;
            }

            // setup next position, release mblatch and continue
            position._clID =
              cappedRecord.getField(FIELD_NAME_RBS_PRERECORD_CL).numberInt();
            position._logicalID =
              cappedRecord.getField(FIELD_NAME_RBS_PRERECORD_OFFSET).numberLong();

         }
         catch( std::exception &e )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Occur exception when getting a RBS record: %s",
                    e.what() ) ;
            goto error ;
         }
         // release mblatch before move to next position
         _su->data()->releaseMBContext( context ) ;

#ifdef _DEBUG
         PD_LOG ( PDDEBUG, "Moving to next position(%d, %ld)",
                  position._clID,
                  position._logicalID ) ;
#endif

      } while ( true );


      // 3. FIXME: update monitor counter for RBS
      // DMS_MON_OP_COUNT_INC( pMonAppCB, MON_RBS_DATA_READ, 1 ) ;

      if ( rc )
      {
         goto error ;
      }
   done:
      PD_TRACE_EXITRC ( SDB__DMSRBSSUMGR_GETRECORD1, rc );
      return  rc ;
   error:
      _su->data()->releaseMBContext( context ) ;
      goto done ;
   }

   // Given start position, try to run RBS garbage collection to recycle space
   // once finished, the new position is returned.
   // Note that the caller should hold mbLock of SYSRBS000
   // position could be stale.
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSRBSSUMGR__GCRBS, "_dmsRBSSUMgr::_gcRBS" )
   SINT32 _dmsRBSSUMgr::_gcRBS ( UINT16 &position, SDB_DPSCB *dpsCB )
   {
      PD_TRACE_ENTRY ( SDB__DMSRBSSUMGR__GCRBS );
      SINT32      rc         = SDB_OK ;
      pmdEDUCB   *eduCB      = pmdGetThreadEDUCB() ;
      CHAR        clName[30] = {0} ;
      DPS_TRANS_ID maxGlobTransID ;
      SINT32      curPos     = (position == DMS_META_RBS_CL) ? DMS_FIRST_RBS_CL :
                                                 ( position + 1 ) ;
      SINT32      begin      = curPos ;
      BOOLEAN     changed    = FALSE ;
      dmsMBContext *pContext = NULL ;

      // From start position, go through each CL, compare its maxGlobTransID
      // against current lowtran. If the the maxGlobTransID is older, we can
      // recycle the CL by dropping it.
      while ( TRUE )
      {
         // Note: Is it safe to do dirty read here? I "think" it's ok because
         // the appendRecord guy could move _cur to next, the worst case here
         // is we stopped a little early
         if ( curPos == _currentCollection )
         {
#ifdef _DEBUG
            PD_LOG ( PDDEBUG,
                     "Finished all RBS except the currently using one,"
                     " start(%d), end(%d)",
                     begin, curPos) ;
#endif
            break ;
         }

         DMS_BUILD_RBS_CL_NAME( clName, curPos ) ;

         // retrieve system lowtran

         // acquire mbLock before work on this CL, since we will try
         // to drop it, let's take X directly
         if( SDB_OK != _su->data()->getMBContext( &pContext,
                                         clName, EXCLUSIVE ) )
         {
            // early break if someone is still using this cl
            break ;
         }

         // retrieve maxGlobTransID of current CL
         maxGlobTransID.resetSN( pContext->mbStat()->getMaxGlobTransID() ) ;

#ifdef _DEBUG
         PD_LOG ( PDDEBUG,
                  "Got maxGlobTransID and lowTran (%s, %s), curPos=%d",
                   dpsTransIDToString( maxGlobTransID ).c_str(),
                   dpsTransIDToString( sdbGetTransCB()->getLowTran() ).c_str(),
                   curPos ) ;
#endif
         // Do GC when the cl max transID is older than lowtran
         // TODO: we may want to do GC when lowTran is invalid, meaning no 
         // running transaction
         if ( sdbGetTransCB()->isVersionExpired( maxGlobTransID ) )
         {

            rc = _su->data()->dropCollection( clName, eduCB, dpsCB,
                                             TRUE, pContext ) ;
            _su->data()->releaseMBContext( pContext ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR,
                        "Failed to drop RBSCL (%s), rc: %d",
                        clName, rc ) ;
               goto error ;
            }

            // Note that we update _lastFreeCollection without protection.
            // It's ok to do so because this is the only thread modifying
            // or use lastFreeCollection. is it safe to treat the UINT32
            // assignement as atomic operation
            _lastFreeCollection = curPos ;

            PD_LOG ( PDDEBUG, "Successfully recycled %s. ",
                     clName ) ;

            rc = flushMeta( DMS_MAX_RBS_CL, _lastFreeCollection, dpsCB,
                            DMS_RBS_FLUSH_OPTION_COLLECTIONS ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR,
                        "Failed to flush meta record in gc, rc: %d",
                        rc ) ;
               goto error ;
            }
         }
         else
         {
            // break out on the first one failed with the condition
            _su->data()->releaseMBContext( pContext ) ;
            break ;
         }
         changed = TRUE ;
         curPos++ ;
         // handle the logic to flip to 1
         if ( curPos >= DMS_MAX_RBS_CL )
         {
            curPos = DMS_FIRST_RBS_CL ;
         }
      } // end of while

#ifdef _DEBUG
      PD_LOG( PDDEBUG, "RBS GC on index trees." ) ;
#endif
      // clean up in memory index tree nodes
      sdbGetTransCB()->getOldVCB()->gcIdxTrees( ) ;

   done:
      if ( changed )
      {
         position = curPos ;
      }

      PD_TRACE_EXITRC ( SDB__DMSRBSSUMGR__GCRBS, rc );
      return  rc ;
   error:
      goto done ;
   }

   // This is the main interface to run garbage collection on RBS with best
   // effort. All protection is self contained.
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSRBSSUMGR_GCRBS, "_dmsRBSSUMgr::gcRBS" )
   void _dmsRBSSUMgr::gcRBS ( )
   {
      PD_TRACE_ENTRY ( SDB__DMSRBSSUMGR_GCRBS );
      SINT32      rc         = SDB_OK ;
      SDB_DPSCB  *dpsCB      = pmdGetKRCB()->getDPSCB() ;
      CHAR        clName[30] = {0} ;
      dmsMBContext *pContext = NULL ;

      // FIXME, we could start from _lastFreeCollection instead, save a read
      // but need to use proper latch protection
      // acquire SYSRBS000 mbLock in S to look up first
      if( SDB_OK != _su->data()->getMBContext( &pContext,
                                               _metaCLName, SHARED ) )
      {
         PD_LOG ( PDWARNING,
                  "Failed to get mbLock (rc=%d), aborting the GC", rc ) ;
         goto error ;
      }

      // need to deal with concurrency with runtime:
      // writer could create CL and modify/increase curCL, GC will try to drop
      // CL and modify/increase lastFreeCL. But they are going to both do pop
      // and insert the meta record in SYSRBS000. we must make sure update is
      // not lost. For better concurrency, we will take the mblock in S to get
      // starting lastFreeCL. After GC, we will take mbLock in X to do the
      // update, but need to refresh curCL

      rc = _getMeta( _currentCollection, _lastFreeCollection, pContext ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to get meta record, rc: %d", rc ) ;
         goto error ;
      }

      pContext->mbUnlock() ;

      DMS_BUILD_RBS_CL_NAME( clName, _lastFreeCollection ) ;

      PD_LOG( PDDEBUG, "RBS GC begin with %s ", clName ) ;

      // try the best to gc as much as possible
      rc = _gcRBS( _lastFreeCollection, dpsCB ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to run GC, clName=%s, meta=(%d, %d) rc: %d",
                  clName, _currentCollection, _lastFreeCollection, rc ) ;
         goto error ;
      }

      // take mbLock and update the meta record
      pContext->mbLock( EXCLUSIVE ) ;

      // flush hashbucket 
      rc = flushMeta( _currentCollection, _lastFreeCollection, dpsCB,
                      DMS_RBS_FLUSH_OPTION_HASHBKT,
                      pContext ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to flush hashbkt after gc, rc: %d",
                  rc ) ;
         goto error ;
      }

   done:
      if ( pContext )
      {
         _su->data()->releaseMBContext( pContext ) ;
      }
      PD_TRACE_EXIT ( SDB__DMSRBSSUMGR_GCRBS );
      // this is best effort
      return  ;
   error:
      goto done ;
   }

   _dmsRBSGCJob::_dmsRBSGCJob( _dmsRBSSUMgr  *rbsSUMgr )
   {
      _rbsSUMgr = rbsSUMgr ;
   }

   _dmsRBSGCJob::~_dmsRBSGCJob()
   {
   }

   const CHAR* _dmsRBSGCJob::name() const
   {
      return "RBS GC" ;
   }

   INT32 _dmsRBSGCJob::doit( IExecutor *pExe,
                             UTIL_LJOB_DO_RESULT &result,
                             UINT64 &sleepTime )
   {
      _rbsSUMgr->gcRBS() ;
      result = UTIL_LJOB_DO_FINISH ;
      return SDB_OK ;
   }

   // submit async job to do garbage collection on RBS including data and idx
   void  dmsStartAsyncRBSGC()
   {
      if ( pmdGetOptionCB()->mvccOn() )
      {
         dmsRBSGCJob * pJob = NULL ;
#ifdef _DEBUG
         PD_LOG( PDDEBUG, "Creating dmsRBSGCJob " ) ;
#endif
         pJob = SDB_OSS_NEW dmsRBSGCJob( pmdGetKRCB()->
                                         getDMSCB()->getRBSSUMgr() ) ;

         if ( !pJob )
         {
            PD_LOG( PDWARNING, "Alloc dmsRBSGCJob failed" ) ;
         }
         else
         {
            INT32 rc = pJob->submit( TRUE ) ;
            if ( rc )
            {
               PD_LOG( PDWARNING, "Submit dmsRBSGCJob failed,rc:%d",
                       rc ) ;
            }
         }
      }
   }
}
