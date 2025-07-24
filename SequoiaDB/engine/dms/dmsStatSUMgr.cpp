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

   Source File Name = dmsStatSUMgr.cpp

   Descriptive Name = Data Management Service Statistics Table Control Block

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains code logic for
   statistics table creation and release.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/
#include "dmsStatSUMgr.hpp"
#include "clsMgr.hpp"
#include "dmsStorageUnit.hpp"
#include "dmsCB.hpp"
#include "rtn.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"
#include "../bson/bson.h"

using namespace bson ;

namespace bson
{
   extern BSONObj staticNull ;
}

namespace engine
{
<<<<<<< HEAD

#define DMS_STAT_CL_IDX_DEF \
   "{ " IXM_FIELD_NAME_NAME "      : \"" DMS_STAT_CL_IDX_NAME "\", \
      " IXM_FIELD_NAME_KEY "       : { " DMS_STAT_COLLECTION_SPACE " : 1, \
                                     " DMS_STAT_COLLECTION " : 1 }, \
      " IXM_FIELD_NAME_UNIQUE "    : true, \
      " IXM_FIELD_NAME_ENFORCED "  : true }"

#define DMS_STAT_IDX_IDX_DEF \
   "{ " IXM_FIELD_NAME_NAME "      : \"" DMS_STAT_IDX_IDX_NAME "\", \
      " IXM_FIELD_NAME_KEY "       : { " DMS_STAT_COLLECTION_SPACE " : 1, \
                                     " DMS_STAT_COLLECTION " : 1, \
                                     " DMS_STAT_IDX_INDEX " : 1 } , \
      " IXM_FIELD_NAME_UNIQUE "    : true, \
      " IXM_FIELD_NAME_ENFORCED "  : true }"

   const utilCSUniqueID DMS_STAT_CSUID = UTIL_CSUNIQUEID_SYS_MIN + 4 ;
   const utilCLUniqueID DMS_STAT_CL_CLUID =
               utilBuildCLUniqueID( DMS_STAT_CSUID, UTIL_CSUNIQUEID_SYS_MIN + 1 ) ;
   const utilCLUniqueID DMS_STAT_IDX_CLUID =
               utilBuildCLUniqueID( DMS_STAT_CSUID, UTIL_CSUNIQUEID_SYS_MIN + 2 ) ;

=======
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
   /*
      _dmsStatSUMgr implement
    */
   _dmsStatSUMgr::_dmsStatSUMgr ( SDB_DMSCB *dmsCB )
   : _dmsSysSUMgr( dmsCB )
   {
      _initialized = FALSE ;
      _tbScanHint = staticNull ;
      _collectionHint = BSON( "" << DMS_STAT_CL_IDX_NAME ) ;
      _indexHint = BSON( "" << DMS_STAT_IDX_IDX_NAME ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTATSUMGR_INIT, "_dmsStatSUMgr::init" )
   INT32 _dmsStatSUMgr::init ()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB__DMSSTATSUMGR_INIT ) ;

      _pmdEDUCB *cb = pmdGetThreadEDUCB() ;
      dmsStorageUnitID suID = DMS_INVALID_CS ;

      SDB_ASSERT ( _dmsCB, "dmsCB can't be NULL" ) ;
      SDB_ASSERT ( cb, "cb can't be NULL" ) ;

      // exclusive lock SYSSTAT cb. this function should be called during
      // process initialization, so it shouldn't be called in parallel by
      // agents
      DMSSYSSUMGR_XLOCK() ;

      // first to load collection space
      rc = rtnCollectionSpaceLock( DMS_STAT_SPACE_NAME, _dmsCB, TRUE,
                                   &_su, suID ) ;
      if ( SDB_DMS_CS_NOTEXIST == rc )
      {
         // create new SYSSTAT collection space
         rc = rtnCreateCollectionSpaceCommand ( DMS_STAT_SPACE_NAME, NULL,
                                                _dmsCB, NULL,
                                                DMS_STAT_CSUID,
                                                DMS_PAGE_SIZE_MAX,
                                                DMS_DO_NOT_CREATE_LOB,
                                                DMS_STORAGE_NORMAL, TRUE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to create %s collection "
                      "space, rc: %d", DMS_STAT_SPACE_NAME, rc ) ;

         rc = rtnCollectionSpaceLock ( DMS_STAT_SPACE_NAME, _dmsCB, TRUE,
                                       &_su, suID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to lock %s collection space, "
                      "rc: %d", DMS_STAT_SPACE_NAME, rc ) ;
      }
      else if ( SDB_OK != rc )
      {
         PD_RC_CHECK( rc, PDERROR, "Failed to lock collection space [%s], "
                      "rc: %d", DMS_STAT_SPACE_NAME, rc ) ;
      }

      _su->data()->setTransSupport( FALSE ) ;

      _dmsCB->suUnlock( suID ) ;
      suID = DMS_INVALID_CS ;

      rc = _ensureStatMetadata( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to create statistics collections or "
                   "indexes, rc: %d", rc ) ;

      _initialized = TRUE ;

   done :
      if ( DMS_INVALID_CS != suID )
      {
         _dmsCB->suUnlock ( suID ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSSTATSUMGR_INIT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   INT32 _dmsStatSUMgr::loadAllStats( pmdEDUCB *cb )
   {
      return pmdGetKRCB()->getRTNCB()->getObjectStatCache()->reloadAllStats( cb );
   }

   INT32 _dmsStatSUMgr::loadCSStats( const CHAR *csName, pmdEDUCB *cb )
   {
      return pmdGetKRCB()->getRTNCB()->getObjectStatCache()->reloadCSStats( cb, csName );
   }

   INT32 _dmsStatSUMgr::loadCLStats( const CHAR *clFullName, pmdEDUCB *cb )
   {
      return pmdGetKRCB()->getRTNCB()->getObjectStatCache()->reloadCLStats( cb, clFullName );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTATSUMGR_UPDATECLSTAT, "_dmsStatSUMgr::updateCollectionStat" )
   INT32 _dmsStatSUMgr::updateCollectionStat ( const BSONObj &collectionStat,
                                               pmdEDUCB *cb,
                                               _dpsLogWrapper *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSSTATSUMGR_UPDATECLSTAT ) ;

      const CHAR *pCSName = collectionStat.getStringField( RTN_STAT_COLLECTION_SPACE ) ;
      const CHAR *pCLName = collectionStat.getStringField( RTN_STAT_COLLECTION ) ;

      PD_CHECK( *pCSName && *pCLName, SDB_INVALIDARG, error, PDERROR,
                "bson must have fields %s and %s, rc: %d", RTN_STAT_COLLECTION_SPACE,
                RTN_STAT_COLLECTION, SDB_INVALIDARG );

      {
         BSONObj boMatcher( BSON( RTN_STAT_COLLECTION_SPACE << pCSName <<
                                 RTN_STAT_COLLECTION << pCLName ) ) ;
         BSONObj boUpdator = BSON( "$set" << collectionStat ) ;

         rc = rtnUpdate( DMS_STAT_COLLECTION_CL_NAME, boMatcher,
                        boUpdator, _collectionHint, FLG_UPDATE_UPSERT,
                        cb, _dmsCB, dpsCB ) ;
         PD_RC_CHECK( rc, PDWARNING, "Failed to update collection statistics "
                     "[%s.%s], rc: %d", pCSName, pCLName, rc ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_DMSSTATSUMGR_UPDATECLSTAT, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTATSUMGR_UPDATEIDXSTAT, "_dmsStatSUMgr::updateIndexStat" )
   INT32 _dmsStatSUMgr::updateIndexStat ( const BSONObj &indexStat,
                                          BOOLEAN isValidForEstimate,
                                          pmdEDUCB *cb,
                                          _dpsLogWrapper *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSSTATSUMGR_UPDATEIDXSTAT ) ;

      const CHAR *pCSName = indexStat.getStringField( RTN_STAT_COLLECTION_SPACE ) ;
      const CHAR *pCLName = indexStat.getStringField( RTN_STAT_COLLECTION ) ;
      const CHAR *pIXName = indexStat.getStringField( RTN_STAT_IDX_INDEX ) ;

      PD_CHECK( *pCSName && *pCLName && *pIXName, SDB_INVALIDARG, error, PDERROR,
                "bson must have fields %s, %s and %s, rc: %d", RTN_STAT_COLLECTION_SPACE,
                RTN_STAT_COLLECTION, RTN_STAT_IDX_INDEX, SDB_INVALIDARG ) ;
      {
         BSONObj boMatcher( BSON( RTN_STAT_COLLECTION_SPACE << pCSName <<
                                 RTN_STAT_COLLECTION << pCLName <<
                                 RTN_STAT_IDX_INDEX << pIXName ) ) ;
         BSONObj boUpdator ;

         if ( isValidForEstimate )
         {
            boUpdator = BSON( "$set" << indexStat ) ;
         }
         else
         {
            // Unset optional fields, which do not exist in default statistics
            boUpdator = BSON( "$set" << indexStat <<
                              "$unset" << BSON( RTN_STAT_IDX_MCV << "" ) ) ;
         }


         rc = rtnUpdate( DMS_STAT_INDEX_CL_NAME, boMatcher,
                        boUpdator, _indexHint, FLG_UPDATE_UPSERT,
                        cb, _dmsCB, dpsCB ) ;
         PD_RC_CHECK( rc, PDWARNING, "Failed to update index statistics "
                     "[%s.%s %s], rc: %d", pCSName, pCLName, pIXName, rc ) ;
      }
   done :
      PD_TRACE_EXITRC( SDB_DMSSTATSUMGR_UPDATEIDXSTAT, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTATSUMGR_ONUNLOADCS, "_dmsStatSUMgr::onUnloadCS" )
   INT32 _dmsStatSUMgr::onUnloadCS ( IDmsEventHolder *pEventHolder,
                                     IDmsSUCacheHolder *pCacheHolder,
                                     pmdEDUCB *cb,
                                     SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSSTATSUMGR_ONUNLOADCS ) ;

      SDB_ASSERT( pEventHolder, "Event holder is invalid" ) ;

      PD_CHECK( _initialized, SDB_INVALIDARG, error, PDWARNING,
                "Statistics SU is not initialized" ) ;


   done :
      PD_TRACE_EXITRC( SDB_DMSSTATSUMGR_ONUNLOADCS, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTATSUMGR_ONRENAMECS, "_dmsStatSUMgr::onRenameCS" )
   INT32 _dmsStatSUMgr::onRenameCS ( IDmsEventHolder *pEventHolder,
                                     IDmsSUCacheHolder *pCacheHolder,
                                     const CHAR *pOldCSName,
                                     const CHAR *pNewCSName,
                                     pmdEDUCB *cb,
                                     SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSSTATSUMGR_ONRENAMECS ) ;

      SDB_ASSERT( pEventHolder, "Event holder is invalid" ) ;

      PD_CHECK( _initialized, SDB_INVALIDARG, error, PDWARNING,
                "Statistics SU is not initialized" ) ;

      pmdGetKRCB()->getRTNCB()->getObjectStatCache()->removeCLStatInCS( pOldCSName );

      if ( pEventHolder && SDB_DB_NORMAL == PMD_DB_STATUS() )
      {
         BSONObj boMatcher( BSON( RTN_STAT_COLLECTION_SPACE << pOldCSName ) ) ;
         BSONObj boNewName( BSON( RTN_STAT_COLLECTION_SPACE << pNewCSName ) ) ;
         BSONObj boUpdator( BSON( "$set" << boNewName ) ) ;

         rc = _updateCollectionStat( boMatcher, boUpdator, cb, NULL ) ;
         PD_RC_CHECK( rc, PDWARNING,
                      "Failed to update collection statistics when rename "
                      "collection space [%s] to [%s], rc: %d",
                      pOldCSName, pNewCSName, rc ) ;

         rc = _updateIndexStat( boMatcher, boUpdator, cb, NULL ) ;
         PD_RC_CHECK( rc, PDWARNING,
                      "Failed to update index statistics when rename "
                      "collection space [%s] to [%s], rc: %d",
                      pOldCSName, pNewCSName, rc ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_DMSSTATSUMGR_ONRENAMECS, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTATSUMGR_ONDROPCS, "_dmsStatSUMgr::onDropCS" )
   INT32 _dmsStatSUMgr::onDropCS ( SDB_EVENT_OCCUR_TYPE type,
                                   IDmsEventHolder *pEventHolder,
                                   IDmsSUCacheHolder *pCacheHolder,
                                   const dmsEventSUItem &suItem,
                                   dmsDropCSOptions *options,
                                   pmdEDUCB *cb,
                                   SDB_DPSCB *dpsCB )
   {
      PD_TRACE_ENTRY( SDB_DMSSTATSUMGR_ONDROPCS ) ;
<<<<<<< HEAD

      BOOLEAN needDelete = FALSE ;

      if ( SDB_EVT_OCCUR_BEFORE == type )
      {
         goto done ;
      }

      if ( !_initialized )
      {
         PD_LOG( PDWARNING, "Statistics SU is not initialized" ) ;
         goto done ;
      }

      SDB_ASSERT( pEventHolder, "Event holder is invalid" ) ;
=======

      if ( SDB_EVT_OCCUR_BEFORE == type )
      {
         goto done ;
      }
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2

      if ( !_initialized )
      {
         PD_LOG( PDWARNING, "Statistics SU is not initialized" ) ;
         goto done ;
      }

      SDB_ASSERT( pEventHolder, "Event holder is invalid" ) ;

      pmdGetKRCB()->getRTNCB()->getObjectStatCache()->removeCLStatInCS( suItem._pCSName );

      if ( pEventHolder && SDB_DB_NORMAL == PMD_DB_STATUS() )
      {
         INT32 tmpRC = SDB_OK ;

         const CHAR *pCSName = suItem._pCSName ;

         BSONObj boMatcher ;

         try
         {
<<<<<<< HEAD
            boMatcher = BSON( DMS_STAT_COLLECTION_SPACE << pCSName ) ;
=======
            boMatcher = BSON( RTN_STAT_COLLECTION_SPACE << pCSName ) ;
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
         }
         catch ( exception &e )
         {
            PD_LOG( PDWARNING, "Failed to build matcher, occur exception %s",
                    e.what() ) ;
            goto done ;
         }

         tmpRC = _deleteCollectionStat( boMatcher, cb, NULL ) ;
<<<<<<< HEAD
         if ( SDB_OK != tmpRC )
=======
         if ( tmpRC )
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
         {
            PD_LOG( PDWARNING,
                    "Failed to drop collection statistics when dropping "
                    "collection space [%s], rc: %d", pCSName, tmpRC ) ;
         }
         tmpRC = _deleteIndexStat( boMatcher, cb, NULL ) ;
<<<<<<< HEAD
         if ( SDB_OK != tmpRC )
=======
         if ( tmpRC )
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
         {
            PD_LOG( PDWARNING,
                    "Failed to delete index statistics when dropping "
                    "collection space [%s], rc: %d", pCSName, tmpRC ) ;
         }
      }

   done :
      PD_TRACE_EXIT( SDB_DMSSTATSUMGR_ONDROPCS ) ;
      // ignore errors
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTATSUMGR_ONRENAMECL, "_dmsStatSUMgr::onRenameCL" )
   INT32 _dmsStatSUMgr::onRenameCL ( IDmsEventHolder *pEventHolder,
                                     IDmsSUCacheHolder *pCacheHolder,
                                     const dmsEventCLItem &clItem,
                                     const CHAR *pNewCLName,
                                     pmdEDUCB *cb,
                                     SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSSTATSUMGR_ONRENAMECL ) ;
      const CHAR *pCSName = pEventHolder->getCSName() ;
      CHAR clFullName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] = {};
      SDB_ASSERT( pEventHolder, "Event holder is invalid" ) ;

      PD_CHECK( _initialized, SDB_INVALIDARG, error, PDWARNING,
                "Statistics SU is not initialized" ) ;

      ossSnprintf( clFullName, sizeof( clFullName ), "%s.%s", pCSName, clItem._pCLName );

      pmdGetKRCB()->getRTNCB()->getObjectStatCache()->removeCLStat( clFullName );

      if ( pEventHolder && SDB_DB_NORMAL == PMD_DB_STATUS() )
      {
         const CHAR *pOldCLName = clItem._pCLName ;

         BSONObj boMatcher( BSON( RTN_STAT_COLLECTION_SPACE << pCSName <<
                                  RTN_STAT_COLLECTION << pOldCLName ) ) ;
         BSONObj boNewName( BSON( RTN_STAT_COLLECTION << pNewCLName ) ) ;
         BSONObj boUpdator( BSON( "$set" << boNewName ) ) ;

         rc = _updateCollectionStat( boMatcher, boUpdator, cb, NULL ) ;
         PD_RC_CHECK( rc, PDWARNING,
                      "Failed to update collection statistics when rename "
                      "collection [%s.%s] to [%s.%s], rc: %d",
                      pCSName, pOldCLName, pCSName, pNewCLName, rc ) ;

         rc = _updateIndexStat( boMatcher, boUpdator, cb, NULL ) ;
         PD_RC_CHECK( rc, PDWARNING,
                      "Failed to update index statistics when rename "
                      "collection [%s.%s] to [%s.%s], rc: %d",
                      pCSName, pOldCLName, pCSName, pNewCLName, rc ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_DMSSTATSUMGR_ONRENAMECL, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTATSUMGR_ONTRUNCCL, "_dmsStatSUMgr::onTruncateCL" )
   INT32 _dmsStatSUMgr::onTruncateCL ( SDB_EVENT_OCCUR_TYPE type,
                                       IDmsEventHolder *pEventHolder,
                                       IDmsSUCacheHolder *pCacheHolder,
                                       const dmsEventCLItem &clItem,
                                       dmsTruncCLOptions *options,
                                       pmdEDUCB *cb,
                                       SDB_DPSCB *dpsCB )
   {
      PD_TRACE_ENTRY( SDB_DMSSTATSUMGR_ONTRUNCCL ) ;

      if ( SDB_EVT_OCCUR_AFTER == type )
      {
         if ( _initialized )
         {
<<<<<<< HEAD
            BOOLEAN needDelete = FALSE ;

            SDB_ASSERT( pCacheHolder, "Event holder is invalid" ) ;

            if ( pCacheHolder )
            {
               dmsSUCache *pCache = pCacheHolder->getSUCache( DMS_CACHE_TYPE_STAT ) ;
               if ( pCache )
               {
                  if ( UTIL_SU_CACHE_UNIT_STATUS_EMPTY == pCache->getStatus( clItem._mbID ) )
                  {
                     needDelete = TRUE ;
                  }
                  else
                  {
                     // For statistics cache, mbID is key of cache unit
                     needDelete = pCache->removeCacheUnit( clItem._mbID, TRUE ) ;
                  }
               }
            }

            if ( needDelete && pEventHolder && SDB_DB_NORMAL == PMD_DB_STATUS() )
            {
               INT32 tmpRC = SDB_OK ;

               const CHAR *pCSName = pEventHolder->getCSName() ;
=======
            SDB_ASSERT( pCacheHolder, "Event holder is invalid" ) ;
            const CHAR *pCSName = pEventHolder->getCSName() ;
            CHAR clFullName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] = {};
            ossSnprintf( clFullName, sizeof( clFullName ), "%s.%s", pCSName, clItem._pCLName );

            pmdGetKRCB()->getRTNCB()->getObjectStatCache()->removeCLStat( clFullName );

            if ( pEventHolder && SDB_DB_NORMAL == PMD_DB_STATUS() )
            {
               INT32 tmpRC = SDB_OK ;

               
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
               const CHAR *pCLName = clItem._pCLName ;

               BSONObj boMatcher ;

               try
               {
<<<<<<< HEAD
                  boMatcher = BSON( DMS_STAT_COLLECTION_SPACE << pCSName <<
                                    DMS_STAT_COLLECTION << pCLName ) ;
=======
                  boMatcher = BSON( RTN_STAT_COLLECTION_SPACE << pCSName <<
                                    RTN_STAT_COLLECTION << pCLName ) ;
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
               }
               catch ( exception &e )
               {
                  PD_LOG( PDWARNING, "Failed to build matcher, occur exception %s",
                          e.what() ) ;
                  goto done ;
               }

               tmpRC = _deleteCollectionStat( boMatcher, cb, NULL ) ;
               if ( SDB_OK != tmpRC )
               {
                  PD_LOG( PDWARNING,
                          "Failed to delete collection statistics when truncating "
                          "collection [%s.%s], rc: %d", pCSName, pCLName, tmpRC ) ;
               }

               tmpRC = _deleteIndexStat( boMatcher, cb, NULL ) ;
               if ( SDB_OK != tmpRC )
               {
                  PD_LOG( PDWARNING,
                          "Failed to delete index statistics when truncating "
                          "collection [%s.%s], rc: %d", pCSName, pCLName, tmpRC ) ;
               }
            }
         }
         else
         {
            PD_LOG( PDWARNING, "Statistics SU is not initialized" ) ;
         }
      }

   done :
      PD_TRACE_EXIT( SDB_DMSSTATSUMGR_ONTRUNCCL ) ;
      // ignore errors
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTATSUMGR_ONDROPCL, "_dmsStatSUMgr::onDropCL" )
   INT32 _dmsStatSUMgr::onDropCL ( SDB_EVENT_OCCUR_TYPE type,
                                   IDmsEventHolder *pEventHolder,
                                   IDmsSUCacheHolder *pCacheHolder,
                                   const dmsEventCLItem &clItem,
                                   dmsDropCLOptions *options,
                                   pmdEDUCB *cb,
                                   SDB_DPSCB *dpsCB )
   {
      PD_TRACE_ENTRY( SDB_DMSSTATSUMGR_ONDROPCL ) ;

      if ( SDB_EVT_OCCUR_BEFORE == type )
      {
         if ( _initialized )
         {
<<<<<<< HEAD
            BOOLEAN needDelete = FALSE ;

            SDB_ASSERT( pEventHolder, "Event holder is invalid" ) ;

            if ( pCacheHolder )
            {
               dmsSUCache *pCache = pCacheHolder->getSUCache( DMS_CACHE_TYPE_STAT ) ;
               if ( pCache )
               {
                  if ( UTIL_SU_CACHE_UNIT_STATUS_EMPTY == pCache->getStatus( clItem._mbID ) )
                  {
                     needDelete = TRUE ;
                  }
                  else
                  {
                     // For statistics cache, mbID is key of cache unit
                     needDelete = pCache->removeCacheUnit( clItem._mbID, TRUE ) ;
                  }
               }
            }

            if ( needDelete && pEventHolder && SDB_DB_NORMAL == PMD_DB_STATUS() )
=======
            SDB_ASSERT( pEventHolder, "Event holder is invalid" ) ;
            const CHAR *pCSName = pEventHolder->getCSName() ;
            CHAR clFullName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] = {};
            ossSnprintf( clFullName, sizeof( clFullName ), "%s.%s", pCSName, clItem._pCLName );

            pmdGetKRCB()->getRTNCB()->getObjectStatCache()->removeCLStat( clFullName );

            if ( pEventHolder && SDB_DB_NORMAL == PMD_DB_STATUS() )
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
            {
               INT32 tmpRC = SDB_OK ;

               const CHAR *pCSName = pEventHolder->getCSName() ;
               const CHAR *pCLName = clItem._pCLName ;

               BSONObj boMatcher ;

               try
               {
<<<<<<< HEAD
                  boMatcher = BSON( DMS_STAT_COLLECTION_SPACE << pCSName <<
                                    DMS_STAT_COLLECTION << pCLName ) ;
=======
                  boMatcher = BSON( RTN_STAT_COLLECTION_SPACE << pCSName <<
                                    RTN_STAT_COLLECTION << pCLName ) ;
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
               }
               catch ( exception &e )
               {
                  PD_LOG( PDWARNING, "Failed to build matcher, occur exception %s",
                          e.what() ) ;
                  goto done ;
               }

               tmpRC = _deleteCollectionStat( boMatcher, cb, NULL ) ;
               if ( SDB_OK != tmpRC )
               {
                  PD_LOG( PDWARNING,
                          "Failed to delete collection statistics when dropping "
                          "collection [%s.%s], rc: %d", pCSName, pCLName, tmpRC ) ;
               }

               tmpRC = _deleteIndexStat( boMatcher, cb, NULL ) ;
               if ( SDB_OK != tmpRC )
               {
                  PD_LOG( PDWARNING,
                          "Failed to delete index statistics when dropping "
                          "collection [%s.%s], rc: %d", pCSName, pCLName, tmpRC ) ;
               }
            }
         }
         else
         {
            PD_LOG( PDWARNING, "Statistics SU is not initialized" ) ;
         }
      }

   done :
      PD_TRACE_EXIT( SDB_DMSSTATSUMGR_ONDROPCL ) ;
      // ignore errors
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTATSUMGR__ONIDXOPTR, "_dmsStatSUMgr::_onIndexOperator" )
   INT32 _dmsStatSUMgr::_onIndexOperator ( IDmsEventHolder *pEventHolder,
                                           IDmsSUCacheHolder *pCacheHolder,
                                           const dmsEventCLItem &clItem,
                                           const dmsEventIdxItem &idxItem,
                                           pmdEDUCB *cb,
                                           SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSSTATSUMGR__ONIDXOPTR ) ;

      SDB_ASSERT( pEventHolder, "Event holder is invalid" ) ;

      const CHAR *pCSName = pEventHolder->getCSName();
      CHAR clFullName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] = {};
            
      PD_CHECK( _initialized, SDB_INVALIDARG, error, PDWARNING,
                "Statistics SU is not initialized" ) ;

      ossSnprintf( clFullName, sizeof( clFullName ), "%s.%s", pCSName, clItem._pCLName );

      pmdGetKRCB()->getRTNCB()->getObjectStatCache()->removeCLStat( clFullName );

      if ( pEventHolder && SDB_DB_NORMAL == PMD_DB_STATUS() )
      {
         const CHAR *pCSName = pEventHolder->getCSName() ;
         const CHAR *pCLName = clItem._pCLName ;
         const CHAR *pIXName = idxItem._pIXName ;

         BSONObj boMatcher( BSON( RTN_STAT_COLLECTION_SPACE << pCSName <<
                                  RTN_STAT_COLLECTION << pCLName <<
                                  RTN_STAT_IDX_INDEX << pIXName ) ) ;

         rc = _deleteIndexStat( boMatcher, cb, NULL ) ;
         PD_RC_CHECK( rc, PDWARNING, "Failed to delete index statistics "
                      "when operating on index [%s.%s %s] , rc: %d", pCSName,
                      pCLName, pIXName, rc ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_DMSSTATSUMGR__ONIDXOPTR, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTATSUMGR_ONCREATEIDX, "_dmsStatSUMgr::onCreateIndex" )
   INT32 _dmsStatSUMgr::onCreateIndex ( IDmsEventHolder *pEventHolder,
                                        IDmsSUCacheHolder *pCacheHolder,
                                        const dmsEventCLItem &clItem,
                                        const dmsEventIdxItem &idxItem,
                                        pmdEDUCB *cb,
                                        SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSSTATSUMGR_ONCREATEIDX ) ;

      SDB_ASSERT( pEventHolder, "Event holder is invalid" ) ;

      rc = _onIndexOperator( pEventHolder, pCacheHolder, clItem, idxItem, cb,
                             dpsCB ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to delete statistics when creating "
                   "index, rc: %d", rc ) ;

   done :
      PD_TRACE_EXITRC( SDB_DMSSTATSUMGR_ONCREATEIDX, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTATSUMGR_ONDROPIDX, "_dmsStatSUMgr::onDropIndex" )
   INT32 _dmsStatSUMgr::onDropIndex ( IDmsEventHolder *pEventHolder,
                                      IDmsSUCacheHolder *pCacheHolder,
                                      const dmsEventCLItem &clItem,
                                      const dmsEventIdxItem &idxItem,
                                      pmdEDUCB *cb, SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSSTATSUMGR_ONDROPIDX ) ;

      SDB_ASSERT( pEventHolder, "Event holder is invalid" ) ;

      rc = _onIndexOperator( pEventHolder, pCacheHolder, clItem, idxItem, cb,
                             dpsCB ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to delete statistics when dropping "
                   "index, rc: %d", rc ) ;

   done :
      PD_TRACE_EXITRC( SDB_DMSSTATSUMGR_ONDROPIDX, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTATSUMGR_ONCLRSUCACHES, "_dmsStatSUMgr::onClearSUCaches" )
   INT32 _dmsStatSUMgr::onClearSUCaches ( IDmsEventHolder *pEventHolder,
                                          IDmsSUCacheHolder *pCacheHolder )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSSTATSUMGR_ONCLRSUCACHES ) ;

      SDB_ASSERT( pEventHolder, "Event holder is invalid" ) ;

      PD_CHECK( _initialized, SDB_INVALIDARG, error, PDWARNING,
                "Statistics SU is not initialized" ) ;

   done :
      PD_TRACE_EXITRC( SDB_DMSSTATSUMGR_ONCLRSUCACHES, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTATSUMGR_ONCLRCLCACHES, "_dmsStatSUMgr::onClearCLCaches" )
   INT32 _dmsStatSUMgr::onClearCLCaches ( IDmsEventHolder *pEventHolder,
                                          IDmsSUCacheHolder *pCacheHolder,
                                          const dmsEventCLItem &clItem )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSSTATSUMGR_ONCLRCLCACHES ) ;

      SDB_ASSERT( pEventHolder, "Event holder is invalid" ) ;

      PD_CHECK( _initialized, SDB_INVALIDARG, error, PDWARNING,
                "Statistics SU is not initialized" ) ;

   done :
      PD_TRACE_EXITRC( SDB_DMSSTATSUMGR_ONCLRCLCACHES, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   INT32 _dmsStatSUMgr::_ensureStatMetadata ( pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      rc = rtnTestAndCreateCL( DMS_STAT_COLLECTION_CL_NAME, cb, _dmsCB, NULL,
                               DMS_STAT_CL_CLUID, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to create collection [%s], rc: %d",
                   DMS_STAT_COLLECTION_CL_NAME, rc ) ;

      rc = rtnTestAndCreateCL( DMS_STAT_INDEX_CL_NAME, cb, _dmsCB, NULL, DMS_STAT_IDX_CLUID, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to create collection [%s], rc: %d",
                   DMS_STAT_INDEX_CL_NAME, rc ) ;

      {
         BSONObj DMS_STAT_CL_IDX_DEF = BSON(
         IXM_FIELD_NAME_NAME << DMS_STAT_CL_IDX_NAME << IXM_FIELD_NAME_KEY
                           << BSON( RTN_STAT_COLLECTION_SPACE << 1 << RTN_STAT_COLLECTION << 1 )
                           << IXM_FIELD_NAME_UNIQUE << true << IXM_FIELD_NAME_ENFORCED << true );
         // Initialized before rtn, so no sorterCreator could be used, set
         // sort buffer size to 0 to build index without sorterCreator
         rc = rtnTestAndCreateIndex( DMS_STAT_COLLECTION_CL_NAME, DMS_STAT_CL_IDX_DEF, cb, _dmsCB,
                                     NULL, TRUE, 0 ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to create index [%s], rc: %d",
                      DMS_STAT_CL_IDX_DEF.toString( 0, 1, 1 ).c_str(), rc ) ;
      }

      {
         BSONObj DMS_STAT_IDX_IDX_DEF =
            BSON( IXM_FIELD_NAME_NAME
                  << DMS_STAT_IDX_IDX_NAME << IXM_FIELD_NAME_KEY
                  << BSON( RTN_STAT_COLLECTION_SPACE << 1 << RTN_STAT_COLLECTION << 1
                                                     << RTN_STAT_IDX_INDEX << 1 )
                  << IXM_FIELD_NAME_UNIQUE << true << IXM_FIELD_NAME_ENFORCED << true );
         // Initialized before rtn, so no sorterCreator could be used, set
         // sort buffer size to 0 to build index without sorterCreator
         rc = rtnTestAndCreateIndex( DMS_STAT_INDEX_CL_NAME, DMS_STAT_IDX_IDX_DEF, cb, _dmsCB, NULL,
                                     TRUE, 0 ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to create index [%s], rc: %d",
                      DMS_STAT_IDX_IDX_DEF.toString( 0, 1, 1 ).c_str(), rc ) ;
      }

   done :
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTATSUMGR__ADDCLSTAT, "_dmsStatSUMgr::_addCollectionStat" )
   INT32 _dmsStatSUMgr::_addCollectionStat ( const MON_CS_SIM_LIST &monCSList,
                                             const BSONObj &collectionStat,
                                             BOOLEAN ignoreCrtTime )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSSTATSUMGR__ADDCLSTAT ) ;

      const CHAR *pCSName = collectionStat.getStringField( RTN_STAT_COLLECTION_SPACE ) ;
      const CHAR *pCLName = collectionStat.getStringField( RTN_STAT_COLLECTION ) ;
      
      const monCSSimple *pMonCS = NULL ;

      PD_CHECK( *pCSName && *pCLName, SDB_INVALIDARG, error, PDERROR,
                "bson must have fields %s and %s", RTN_STAT_COLLECTION_SPACE, RTN_STAT_COLLECTION ) ;

      // Get collection space information
      pMonCS = monCSSimple::getCollectionSpace( monCSList, pCSName ) ;
      PD_CHECK( pMonCS, SDB_DMS_CS_NOTEXIST, error, PDWARNING,
                "Could not get collection space [%s] for statistics",
                pCSName ) ;

      rc = _addSUCollectionStat( pMonCS, NULL, collectionStat, FALSE ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to add collection statistics [%s.%s], rc: %d",
                   pCSName, pCLName, rc ) ;
  done :
      PD_TRACE_EXITRC( SDB_DMSSTATSUMGR__ADDCLSTAT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTATSUMGR__ADDIDXSTAT, "_dmsStatSUMgr::_addIndexStat" )
   INT32 _dmsStatSUMgr::_addIndexStat ( const MON_CS_SIM_LIST &monCSList,
                                        const BSONObj &indexStat,
                                        BOOLEAN ignoreCrtTime )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSSTATSUMGR__ADDIDXSTAT ) ;

      const CHAR *pCSName = indexStat.getStringField( RTN_STAT_COLLECTION_SPACE ) ;

      const monCSSimple *pMonCS = NULL ;

      PD_CHECK( *pCSName, SDB_INVALIDARG, error, PDERROR, "bson must have fields %s and %s",
                RTN_STAT_COLLECTION_SPACE, RTN_STAT_COLLECTION );

      // Get collection space information
      pMonCS = monCSSimple::getCollectionSpace( monCSList, pCSName ) ;
      PD_CHECK( pMonCS, SDB_DMS_CS_NOTEXIST, error, PDWARNING,
                "Could not get collection space [%s] for statistics",
                pCSName ) ;

      rc = _addSUIndexStat( pMonCS, NULL, NULL, indexStat, ignoreCrtTime );
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to add index statistics [%s.%s, %s], rc: %d",
                   pCSName, indexStat.getStringField(RTN_STAT_COLLECTION),
                   indexStat.getStringField(RTN_STAT_IDX_INDEX), rc ) ;

  done :
      PD_TRACE_EXITRC( SDB_DMSSTATSUMGR__ADDIDXSTAT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTATSUMGR__ADDSUCLSTAT, "_dmsStatSUMgr::_addSUCollectionStat" )
   INT32 _dmsStatSUMgr::_addSUCollectionStat ( const monCSSimple *pMonCS,
                                               const monCLSimple *pMonCL,
                                               const BSONObj &collectionStat,
                                               BOOLEAN ignoreCrtTime )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSSTATSUMGR__ADDSUCLSTAT ) ;

      const CHAR *pCSName = collectionStat.getStringField( RTN_STAT_COLLECTION_SPACE ) ;
      const CHAR *pCLName = collectionStat.getStringField( RTN_STAT_COLLECTION ) ;
      PD_CHECK( *pCSName && *pCLName, SDB_INVALIDARG, error, PDERROR, "bson must have fields %s and %s",
                RTN_STAT_COLLECTION_SPACE, RTN_STAT_COLLECTION ) ;

      if ( pMonCS )
      {
         PD_CHECK( 0 == ossStrcmp( pMonCS->_name, pCSName ),
                   SDB_SYS, error, PDWARNING,
                   "Names of collection spaces are different, dump [%s], "
                   "statistics [%s]", pMonCS->_name, pCSName ) ;

         if ( NULL == pMonCL )
         {
            pMonCL = pMonCS->getCollection( pCLName ) ;
            PD_CHECK( pMonCL, SDB_DMS_NOTEXIST, error, PDWARNING,
                      "Could not get collection [%s.%s] for statistics",
                      pCSName, pCLName ) ;
         }
         else
         {
            PD_CHECK( 0 == ossStrcmp( pMonCL->_clname, pCLName ),
                      SDB_SYS, error, PDWARNING,
                      "Names of collection are different, dump [%s], "
                      "statistics [%s]", pMonCL->_clname, pCLName ) ;
         }

      }

   done :
      PD_TRACE_EXITRC( SDB_DMSSTATSUMGR__ADDSUCLSTAT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTATSUMGR__ADDSUIDXSTAT, "_dmsStatSUMgr::_addSUIndexStat" )
   INT32 _dmsStatSUMgr::_addSUIndexStat ( const monCSSimple *pMonCS,
                                          const monCLSimple *pMonCL,
                                          const monIndex *pMonIX,
                                          const BSONObj &indexStat,
                                          BOOLEAN ignoreCrtTime )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSSTATSUMGR__ADDSUIDXSTAT ) ;

      const CHAR *pCSName = indexStat.getStringField( RTN_STAT_COLLECTION_SPACE ) ;
      const CHAR *pCLName = indexStat.getStringField( RTN_STAT_COLLECTION ) ;
      const CHAR *pIXName = indexStat.getStringField( RTN_STAT_IDX_INDEX ) ;

      if ( pMonCS )
      {
         PD_CHECK( 0 == ossStrcmp( pMonCS->_name, pCSName ),
                   SDB_SYS, error, PDWARNING,
                   "Names of collection spaces are different, dump [%s], "
                   "statistics [%s]", pMonCS->_name, pCSName ) ;

         if ( pMonCL == NULL )
         {
            pMonCL = pMonCS->getCollection( pCLName ) ;
            PD_CHECK( pMonCL, SDB_DMS_NOTEXIST, error, PDWARNING,
                      "Could not get collection [%s.%s] for statistics",
                      pCSName, pCLName ) ;
         }
         else
         {
            PD_CHECK( 0 == ossStrcmp( pMonCL->_clname, pCLName ),
                      SDB_SYS, error, PDWARNING,
                      "Names of collection are different, dump [%s], "
                      "statistics [%s]", pMonCL->_clname, pCLName ) ;
         }

         if ( NULL == pMonIX )
         {
            pMonIX = pMonCL->getIndex( pIXName ) ;
            PD_CHECK( pMonIX, SDB_IXM_NOTEXIST, error, PDWARNING,
                      "Could not get index [%s.%s %s] for statistics",
                      pCSName, pCLName, pIXName ) ;
         }
         else
         {
            PD_CHECK( 0 == ossStrcmp( pMonIX->getIndexName(), pIXName ),
                      SDB_SYS, error, PDWARNING,
                      "Names of indexes are different, dump [%s], "
                      "statistics [%s]", pMonIX->getIndexName(), pIXName ) ;
         }

         try
         {
            BSONObj dumpKeyPattern = pMonIX->getKeyPattern() ;
            BSONObj statKeyPattern = indexStat.getObjectField( IXM_KEY_FIELD ) ;
            BOOLEAN isUnique = indexStat.getField( IXM_UNIQUE_FIELD ).Bool();
            PD_CHECK( 0 == dumpKeyPattern.woCompare( statKeyPattern, BSONObj(),
                                                     TRUE ),
                      SDB_IXM_NOTEXIST, error, PDWARNING,
                      "Index [%s.%s %s] is not found for statistics: "
                      "different key pattern, dump [%s], stat [%s]",
                      pCSName, pCLName, pIXName,
                      dumpKeyPattern.toString( FALSE, TRUE ).c_str(),
                      statKeyPattern.toString( FALSE, TRUE ).c_str() ) ;

            PD_CHECK( pMonIX->isUnique() == isUnique,
                      SDB_IXM_NOTEXIST, error, PDWARNING,
                      "Index [%s.%s %s] is not found for statistics: "
                      "different unique definition, dump [%s], stat [%s]",
                      pCSName, pCLName, pIXName,
                      pMonIX->isUnique() ? "true" : "false",
                      isUnique ? "true" : "false" ) ;
         }
         catch( std::exception &e )
         {
            PD_LOG( PDWARNING, "Checking index [%s.%s, %s] occur exception: %s",
                    pCSName, pCLName, pIXName, e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }

      }

  done :
      PD_TRACE_EXITRC( SDB_DMSSTATSUMGR__ADDSUIDXSTAT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTATSUMGR__DELCLSTAT, "_dmsStatSUMgr::_deleteCollectionStat" )
   INT32 _dmsStatSUMgr::_deleteCollectionStat ( const BSONObj &boMatcher,
                                                _pmdEDUCB *cb,
                                                SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSSTATSUMGR__DELCLSTAT ) ;

      rc = rtnDelete( DMS_STAT_COLLECTION_CL_NAME, boMatcher, _collectionHint,
                      0, cb, _dmsCB, dpsCB, 1 ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Delete collection statistics [%s] failed, rc: %d",
                   boMatcher.toString().c_str(), rc ) ;

   done :
      PD_TRACE_EXITRC( SDB_DMSSTATSUMGR__DELCLSTAT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTATSUMGR__DELIDXSTAT, "_dmsStatSUMgr::_deleteIndexStat" )
   INT32 _dmsStatSUMgr::_deleteIndexStat ( const BSONObj &boMatcher,
                                           _pmdEDUCB *cb,
                                           SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSSTATSUMGR__DELIDXSTAT ) ;

      rc = rtnDelete( DMS_STAT_INDEX_CL_NAME, boMatcher, _indexHint, 0, cb,
                      _dmsCB, dpsCB, 1 ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Delete index statistics [%s] failed, rc: %d",
                   boMatcher.toString().c_str(), rc ) ;

   done :
      PD_TRACE_EXITRC( SDB_DMSSTATSUMGR__DELIDXSTAT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTATSUMGR__UPDATECLSTAT, "_dmsStatSUMgr::_updateCollectionStat" )
   INT32 _dmsStatSUMgr::_updateCollectionStat ( const BSONObj &boMatcher,
                                                const BSONObj &boUpdator,
                                                _pmdEDUCB *cb, SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSSTATSUMGR__UPDATECLSTAT ) ;

      rc = rtnUpdate( DMS_STAT_COLLECTION_CL_NAME, boMatcher, boUpdator,
                      _collectionHint, 0, cb, _dmsCB, dpsCB, 1 ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Update collection statistics [%s] with [%s] failed, rc: %d",
                   boMatcher.toString( FALSE, TRUE ).c_str(),
                   boUpdator.toString( FALSE, TRUE ).c_str(), rc ) ;

   done :
      PD_TRACE_EXITRC( SDB_DMSSTATSUMGR__UPDATECLSTAT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTATSUMGR__UPDATEIDXSTAT, "_dmsStatSUMgr::_updateIndexStat" )
   INT32 _dmsStatSUMgr::_updateIndexStat ( const BSONObj &boMatcher,
                                           const BSONObj &boUpdator,
                                           _pmdEDUCB *cb, SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSSTATSUMGR__UPDATEIDXSTAT ) ;

      rc = rtnUpdate( DMS_STAT_INDEX_CL_NAME, boMatcher, boUpdator,
                      _indexHint, 0, cb, _dmsCB, dpsCB, 1 ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Update index statistics [%s] with [%s] failed, rc: %d",
                   boMatcher.toString( FALSE, TRUE ).c_str(),
                   boUpdator.toString( FALSE, TRUE ).c_str(), rc ) ;

   done :
      PD_TRACE_EXITRC( SDB_DMSSTATSUMGR__UPDATEIDXSTAT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTATSUMGR__LOADCLSTATS, "_dmsStatSUMgr::_loadCollectionStats" )
   INT32 _dmsStatSUMgr::_loadCollectionStats ( const monCSSimple *pMonCS,
                                               const monCLSimple *pMonCL,
                                               const BSONObj &boMatcher,
                                               pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSSTATSUMGR__LOADCLSTATS ) ;

      BSONObj boDummy ;
      INT64 contextID = -1 ;

      // The collection is specified, could not skip errors
      BOOLEAN couldContinue = !pMonCL ;

      SDB_DMSCB *dmsCB = _dmsCB ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;

      // query
      rc = rtnQuery( DMS_STAT_COLLECTION_CL_NAME, boDummy, boMatcher, boDummy,
                     _collectionHint, 0, cb, 0, -1, dmsCB, rtnCB, contextID ) ;
      PD_RC_CHECK( rc, PDWARNING, "Query statistics [%s] from [%s] failed, "
                   "rc: %d", boMatcher.toString( FALSE, TRUE ).c_str(),
                   DMS_STAT_COLLECTION_CL_NAME, rc ) ;

      // get more
      while ( TRUE )
      {
         rtnContextBuf contextBuf ;

         rc = rtnGetMore( contextID, 1, contextBuf, cb, rtnCB ) ;
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_OK ;
            break ;
         }
         PD_RC_CHECK( rc, PDWARNING, "Get more failed, rc: %d", rc ) ;

         try
         {
            BSONObj boCollectionStat = BSONObj( contextBuf.data() ) ;
            rc = _addSUCollectionStat( pMonCS, pMonCL, boCollectionStat, FALSE ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( couldContinue ? PDWARNING : PDERROR,
                       "Failed to add collection statistics bsonobj, rc: %d", rc ) ;
               if ( !couldContinue )
               {
                  rc = SDB_OK ;
               }
               else
               {
                  goto error ;
               }
            }
         }
         catch( std::exception &e )
         {
            PD_LOG( PDWARNING,
                    "Get index statistics for collection occur exception: %s",
                    e.what() ) ;
            rc = SDB_SYS ;
         }

<<<<<<< HEAD
         if ( SDB_OK != rc )
         {
            SAFE_OSS_DELETE( pCollectionStat ) ;
            if ( couldContinue )
            {
               rc = SDB_OK ;
               continue ;
            }
            else
            {
               goto error ;
            }
         }

         rc = _addSUCollectionStat( pMonCS, pMonCL, pStatCache,
                                    pCollectionStat, FALSE ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( ( couldContinue ? PDWARNING : PDERROR ),
                    "Failed to add collection statistics [%s.%s], rc: %d",
                    pCollectionStat->getCSName(), pCollectionStat->getCLName(),
                    rc ) ;
            SAFE_OSS_DELETE( pCollectionStat ) ;

            if ( !couldContinue )
            {
               rc = SDB_OK ;
            }
            else
            {
               goto error ;
            }
         }
=======
         
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
      }

   done :
      if ( -1 != contextID )
      {
         rtnKillContexts( 1 , &contextID, cb, rtnCB ) ;
      }
      PD_TRACE_EXITRC( SDB_DMSSTATSUMGR__LOADCLSTATS, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTATSUMGR__LOADIDXSTATS, "_dmsStatSUMgr::_loadIndexStats" )
   INT32 _dmsStatSUMgr::_loadIndexStats ( const monCSSimple *pMonCS,
                                          const monCLSimple *pMonCL,
                                          const monIndex *pMonIX,
                                          const BSONObj &boMatcher,
                                          pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSSTATSUMGR__LOADIDXSTATS ) ;

      BSONObj boDummy ;
      INT64 contextID = -1 ;

      // The index is specified, could not skip errors
      BOOLEAN couldContinue = !pMonIX ;

      SDB_DMSCB *dmsCB = _dmsCB ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;

      // query
      rc = rtnQuery( DMS_STAT_INDEX_CL_NAME, boDummy, boMatcher, boDummy,
                     _indexHint, 0, cb, 0, -1, dmsCB, rtnCB, contextID ) ;
      PD_RC_CHECK( rc, PDWARNING, "Query statistics [%s] from [%s] failed, "
                   "rc: %d", boMatcher.toString( FALSE, TRUE ).c_str(),
                   DMS_STAT_INDEX_CL_NAME, rc ) ;

      // get more
      while ( TRUE )
      {
         rtnContextBuf contextBuf ;

         rc = rtnGetMore( contextID, 1, contextBuf, cb, rtnCB ) ;
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_OK ;
            break ;
         }
         PD_RC_CHECK( rc, PDWARNING, "Get more failed, rc: %d", rc ) ;


         try
         {
            BSONObj boIndexStat = BSONObj( contextBuf.data() ) ;

            rc = _addSUIndexStat( pMonCS, pMonCL, pMonIX, boIndexStat, FALSE ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( couldContinue ? PDWARNING : PDERROR,
                       "Failed to add index statistics bsonobj, rc: %d", rc ) ;

               if ( !couldContinue )
               {
                  rc = SDB_OK ;
               }
               else
               {
                  goto error ;
               }
            }
         }
         catch( std::exception &e )
         {
            PD_LOG( PDWARNING,
                    "Get index statistics for index occur exception: %s",
                    e.what() ) ;
            rc = SDB_SYS ;
         }

         if ( SDB_OK != rc )
         {
            if ( couldContinue )
            {
               rc = SDB_OK ;
               continue ;
            }
            else
            {
               goto error ;
            }
         }
<<<<<<< HEAD

         rc = _addSUIndexStat( pMonCS, pMonCL, pMonIX, pStatCache,
                               pIndexStat, FALSE ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( ( couldContinue ? PDWARNING : PDERROR ),
                    "Failed to add index statistics [%s.%s, %s], rc: %d",
                    pIndexStat->getCSName(), pIndexStat->getCLName(),
                    pIndexStat->getIndexName(), rc ) ;
            SAFE_OSS_DELETE( pIndexStat ) ;

            if ( !couldContinue )
            {
               rc = SDB_OK ;
            }
            else
            {
               goto error ;
            }
         }
=======
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
      }

   done :
      if ( -1 != contextID )
      {
         rtnKillContexts( 1 , &contextID, cb, rtnCB ) ;
      }
      PD_TRACE_EXITRC( SDB_DMSSTATSUMGR__LOADIDXSTATS, rc ) ;
      return rc ;

   error :
      goto done ;
   }

}

