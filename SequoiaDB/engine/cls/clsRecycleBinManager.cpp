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

   Source File Name = clsDCMgr.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/02/2015  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#include "clsRecycleBinManager.hpp"
#include "clsMgr.hpp"
#include "pdTrace.hpp"
#include "clsTrace.hpp"
#include "../bson/bson.hpp"

using namespace bson ;
using namespace std ;

namespace engine
{

   // query 1000 recycle bin items for each time
   #define CLS_QUERY_RECYCLE_ITEM_STEP ( 1000 )

   /*
      _clsRecycleBinManager implement
    */
   _clsRecycleBinManager::_clsRecycleBinManager()
   {
   }

   _clsRecycleBinManager::~_clsRecycleBinManager()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSRECYBINMGR_DROPITEMCHK, "_clsRecycleBinManager::dropItemWithCheck" )
   INT32 _clsRecycleBinManager::dropItemWithCheck( const utilRecycleItem &item,
                                                   pmdEDUCB *cb,
                                                   BOOLEAN checkCatalog,
                                                   BOOLEAN &isDropped )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSRECYBINMGR_DROPITEMCHK ) ;

      const CHAR *originName = item.getOriginName() ;
      const CHAR *recycleName = item.getRecycleName() ;

      isDropped = FALSE ;

      PD_CHECK( pmdIsPrimary(), SDB_CLS_NOT_PRIMARY, error, PDERROR,
                "Failed to check primary status" ) ;
      PD_CHECK( !cb->isInterrupted(), SDB_APP_INTERRUPT, error, PDERROR,
                "Failed to drop item, session is interrupted" ) ;

      if ( checkCatalog )
      {
         shardCB *pShdMgr = sdbGetShardCB() ;
         utilRecycleItem remoteItem ;
         rc = pShdMgr->rGetRecycleItem( cb, item.getRecycleID(), remoteItem ) ;
         if ( SDB_OK == rc )
         {
            // recycle item still exists, can not job
            goto done ;
         }
         else if ( SDB_RECYCLE_ITEMNOTEXISTS == rc )
         {
            rc = SDB_OK ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to get recycle item [%s] "
                      "from catalog, rc: %d", recycleName, rc ) ;
      }

      rc = _dropItemImpl( item, cb, 1 ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to drop recycle item "
                   "[origin %s, recycle %s], rc: %d", originName,
                   recycleName, rc ) ;

      isDropped = TRUE ;

   done:
      PD_TRACE_EXITRC( SDB__CLSRECYBINMGR_DROPITEMCHK, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSRECYBINMGR__DROPALLITEMSTYPE, "_clsRecycleBinManager::_dropAllItemInType" )
   INT32 _clsRecycleBinManager::_dropAllItemInType( UTIL_RECYCLE_TYPE type,
                                                    pmdEDUCB *cb,
                                                    BOOLEAN checkCatalog )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSRECYBINMGR__DROPALLITEMSTYPE ) ;

      BSONObj matcher, orderBy ;
      CHAR lastRecycleName[ UTIL_RECYCLE_NAME_SZ + 1 ] = { '\0' } ;

      try
      {
         orderBy = BSON( FIELD_NAME_RECYCLE_NAME << 1 ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build order-by BSON, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

      while ( TRUE )
      {
         UTIL_RECY_ITEM_LIST itemList ;

         try
         {
            matcher = BSON( FIELD_NAME_TYPE <<
                                  utilGetRecycleTypeName( type ) <<
                            FIELD_NAME_RECYCLE_NAME <<
                                  BSON( "$gt" << lastRecycleName ) ) ;
         }
         catch ( exception &e )
         {
            PD_LOG( PDERROR, "Failed to build query BSON, occur exception %s",
                             e.what() ) ;
            rc = ossException2RC( &e ) ;
            goto error ;
         }

         // query for a batch of items (1000 for each time)
         rc = getItems( matcher, orderBy, _hintName,
                        CLS_QUERY_RECYCLE_ITEM_STEP, cb, itemList ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get recycle items after [%s], "
                      "rc: %d", lastRecycleName, rc ) ;

         if ( itemList.empty() )
         {
            break ;
         }

         for ( UTIL_RECY_ITEM_LIST::iterator iter = itemList.begin() ;
               iter != itemList.end() ;
               ++ iter )
         {
            BOOLEAN isDropped = FALSE ;
            rc = dropItemWithCheck( ( *iter ), cb, checkCatalog, isDropped ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to drop recycle item [%s], "
                         "rc: %d", iter->getRecycleName(), rc ) ;
         }

         ossStrncpy( lastRecycleName, itemList.back().getRecycleName(),
                     UTIL_RECYCLE_NAME_SZ ) ;
         lastRecycleName[ UTIL_RECYCLE_NAME_SZ ] = '\0' ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSRECYBINMGR__DROPALLITEMSTYPE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSRECYBINMGR_DROPALLITEMS, "_clsRecycleBinManager::dropAllItems" )
   INT32 _clsRecycleBinManager::dropAllItems( pmdEDUCB *cb,
                                              BOOLEAN checkCatalog )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSRECYBINMGR_DROPALLITEMS ) ;

      rc = _dropAllItemInType( UTIL_RECYCLE_CS, cb, checkCatalog ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to drop collection space items, "
                   "rc: %d", rc ) ;

      rc = _dropAllItemInType( UTIL_RECYCLE_CL, cb, checkCatalog ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to drop collection items, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__CLSRECYBINMGR_DROPALLITEMS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSRECYBINMGR__DROPITEM_DONE, "_clsRecycleBinManager::_dropItem" )
   INT32 _clsRecycleBinManager::_dropItem( const utilRecycleItem &item,
                                           pmdEDUCB *cb,
                                           INT16 w,
                                           BOOLEAN ignoreNotExists,
                                           BOOLEAN &isDropped )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSRECYBINMGR__DROPITEM_DONE ) ;

      rc = dropItemWithCheck( item, cb, TRUE, isDropped ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to drop recycle item [origin %s, "
                   "recycle %s], rc: %d", item.getOriginName(),
                   item.getRecycleName(), rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__CLSRECYBINMGR__DROPITEM_DONE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSRECYBINMGR__CREATEBGJOB, "_clsRecycleBinManager::_createBGJob" )
   INT32 _clsRecycleBinManager::_createBGJob( utilLightJob **pJob )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSRECYBINMGR__CREATEBGJOB ) ;

      rtnDropRecycleBinBGJob *job = NULL ;

      SDB_ASSERT( NULL != pJob, "job pointer is invalid" ) ;

      job = SDB_OSS_NEW clsDropRecycleBinBGJob( this ) ;
      PD_CHECK( NULL != job, SDB_OOM, error, PDERROR, "Failed to allocate BG "
                "job, rc: %d", rc ) ;

      *pJob = (utilLightJob *)job ;

   done:
      PD_TRACE_EXITRC( SDB__CLSRECYBINMGR__CREATEBGJOB, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSRECYBINMGR__DROPITEMIMPL, "_clsRecycleBinManager::_dropItemImpl" )
   INT32 _clsRecycleBinManager::_dropItemImpl( const utilRecycleItem &item,
                                               pmdEDUCB *cb,
                                               INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSRECYBINMGR__DROPITEMIMPL ) ;

      if ( UTIL_RECYCLE_CS == item.getType() )
      {
         rc = _dropCSItem( item, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to drop recycle item "
                      "[origin %s, recycle %s] for collection space, rc: %d",
                      item.getOriginName(), item.getRecycleName(), rc ) ;
      }
      else if ( UTIL_RECYCLE_CL == item.getType() &&
                item.isMainCL() )
      {
         rc = _dropMainCLItem( item, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to drop recycle item "
                      "[origin %s, recycle %s] for main collection, rc: %d",
                      item.getOriginName(), item.getRecycleName(), rc ) ;
      }
      else if ( UTIL_RECYCLE_CL == item.getType() )
      {
         rc = _dropCLItem( item, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to drop recycle item "
                      "[origin %s, recycle %s] for collection, rc: %d",
                      item.getOriginName(), item.getRecycleName(), rc ) ;
      }
      else
      {
         SDB_ASSERT( FALSE, "invalid recycle type" ) ;
         PD_CHECK( FALSE, SDB_SYS, error, PDERROR, "Failed to drop item, "
                   "invalid recycle type [%d]", item.getType() ) ;
      }

      PD_LOG( PDEVENT, "Drop recycle item [origin %s, recycle %s], "
              "type [%s]", item.getOriginName(), item.getRecycleName(),
              utilGetRecycleTypeName( item.getType() ) ) ;

   done:
      PD_TRACE_EXITRC( SDB__CLSRECYBINMGR__DROPITEMIMPL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSRECYBINMGR__DROPCSITEM, "_clsRecycleBinManager::_dropCSItem" )
   INT32 _clsRecycleBinManager::_dropCSItem( const utilRecycleItem &item,
                                             pmdEDUCB *cb,
                                             INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSRECYBINMGR__DROPCSITEM ) ;

      SDB_ASSERT( UTIL_RECYCLE_CS == item.getType(),
                  "should be recycle item for collection space" ) ;

      const CHAR *recycleName = item.getRecycleName() ;
      const CHAR *originName = item.getOriginName() ;

      rc = rtnDropCollectionSpaceCommand( recycleName, cb, _dmsCB, _dpsCB ) ;
      if ( SDB_DMS_CS_NOTEXIST == rc )
      {
         // ignore not exist error
         rc = SDB_OK ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to drop collection space for "
                   "recycle item [%s], rc: %d", recycleName, rc ) ;

      // remove all recycle items inside this collection space
      // including all recycled collections and the collection space itself
      rc = _deleteItemsInCS( (utilCSUniqueID)( item.getOriginID() ),
                             TRUE, cb, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to delete recycle items in "
                   "recycled collection space item [origin %s, recycle %s], "
                   "rc: %d", originName, recycleName, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__CLSRECYBINMGR__DROPCSITEM, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSRECYBINMGR__DROPMAINCLITEM, "_clsRecycleBinManager::_dropMainCLItem" )
   INT32 _clsRecycleBinManager::_dropMainCLItem( const utilRecycleItem &item,
                                                 pmdEDUCB *cb,
                                                 INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSRECYBINMGR__DROPMAINCLITEM ) ;

      SDB_ASSERT( UTIL_RECYCLE_CL == item.getType() && item.isMainCL(),
                  "should be recycle item for main collection" ) ;

      UTIL_RECY_ITEM_LIST itemList ;

      // get sub-items
      rc = getSubItems( item, cb, itemList ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get sub-collection recycle "
                   "items for main-collection item [origin %s, recycle %s], "
                   "rc: %d", item.getOriginName(), item.getRecycleName(),
                   rc ) ;

      // drop each sub-items
      for ( UTIL_RECY_ITEM_LIST_IT iter = itemList.begin() ;
            iter != itemList.end() ;
            ++ iter )
      {
         utilRecycleItem &subItem = *iter ;
         rc = _dropCLItem( subItem, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to drop recycle item [origin %s, "
                      "recycle %s], rc: %d", subItem.getOriginName(),
                      subItem.getRecycleName(), rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSRECYBINMGR__DROPMAINCLITEM, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSRECYBINMGR__DROPCLITEM, "_clsRecycleBinManager::_dropCLItem" )
   INT32 _clsRecycleBinManager::_dropCLItem( const utilRecycleItem &item,
                                             pmdEDUCB *cb,
                                             INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSRECYBINMGR__DROPCLITEM ) ;

      SDB_ASSERT( UTIL_RECYCLE_CL == item.getType(),
                  "should be recycle item for collection" ) ;

      BOOLEAN writable = FALSE ;
      const CHAR *recycleName = item.getRecycleName() ;
      const CHAR *originName = item.getOriginName() ;
      utilCSUniqueID csUniqueID =
            utilGetCSUniqueID( (utilCLUniqueID)( item.getOriginID() ) ) ;
      dmsStorageUnitID suID = DMS_INVALID_SUID ;
      dmsStorageUnit *su = NULL ;
      BOOLEAN itemDeleted = FALSE ;

      rc = _dmsCB->writable( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Database is not writable, rc: %d", rc ) ;
      writable = TRUE ;

      rc = _dmsCB->idToSUAndLock( csUniqueID, suID, &su ) ;
      if ( SDB_DMS_CS_NOTEXIST == rc )
      {
         rc = SDB_OK ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to lock collection space, rc: %d",
                   rc ) ;

      if ( NULL != su )
      {
         INT32 tmpRC = SDB_OK ;

         CHAR szSpace[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ]  = { 0 } ;

         ossStrncpy( szSpace, su->CSName(), DMS_COLLECTION_SPACE_NAME_SZ ) ;
         szSpace[ DMS_COLLECTION_SPACE_NAME_SZ ] = '\0' ;

         rc = su->data()->dropCollection( recycleName, cb, _dpsCB ) ;
         if ( SDB_DMS_NOTEXIST == rc )
         {
            // ignore not exist error
            rc = SDB_OK ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to drop collection [%s], rc: %d",
                      recycleName, rc ) ;

         _dmsCB->suUnlock( suID ) ;
         suID = DMS_INVALID_SUID ;
         su = NULL ;

         // try drop empty collection space
         tmpRC = rtnDropCollectionSpaceCommand( szSpace, cb, _dmsCB,
                                                _dpsCB, FALSE, TRUE ) ;
         if ( SDB_OK == tmpRC &&
              0 == ossStrncmp( szSpace,
                               UTIL_RECYCLE_PREFIX,
                               UTIL_RECYCLE_PREFIX_SZ ) )
         {
            // if dropped by me, safe to remove recycle items inside this
            // collection space
            tmpRC = _deleteItemsInCS( (utilCSUniqueID)( item.getOriginID() ),
                                      TRUE, cb, w ) ;
            if ( SDB_OK == tmpRC )
            {
               itemDeleted = TRUE ;
            }
            else
            {
               // don't go to error, will try to delete the collection item
               PD_LOG( PDWARNING, "Failed to delete recycle item "
                       "[origin %s, recycle %s], rc: %d", originName,
                       recycleName, tmpRC ) ;
            }
         }
      }

      if ( !itemDeleted )
      {
         rc = _deleteItem( item, cb, w, TRUE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to delete recycle item "
                      "[origin %s, recycle %s], rc: %d", originName,
                      recycleName, rc ) ;
      }

   done:
      if ( DMS_INVALID_SUID != suID )
      {
         _dmsCB->suUnlock( suID ) ;
      }
      if ( writable )
      {
         _dmsCB->writeDown( cb ) ;
      }
      PD_TRACE_EXITRC( SDB__CLSRECYBINMGR__DROPCLITEM, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSRECYBINMGR__DELITEMSINCS, "_clsRecycleBinManager::_deleteItemsInCS" )
   INT32 _clsRecycleBinManager::_deleteItemsInCS( utilCSUniqueID csUniqueID,
                                                  BOOLEAN includeSelf,
                                                  pmdEDUCB *cb,
                                                  INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSRECYBINMGR__DELITEMSINCS ) ;

      BSONObj matcher ;
      UINT64 deletedCount = 0 ;

      if ( includeSelf )
      {
         try
         {
            BSONObjBuilder builder ;
            BSONArrayBuilder orBuilder( builder.subarrayStart( "$or" ) ) ;

            BSONObjBuilder csBuilder( orBuilder.subobjStart() ) ;
            csBuilder.append( FIELD_NAME_ORIGIN_ID, (INT32)csUniqueID ) ;
            csBuilder.doneFast() ;

            BSONObjBuilder clBuilder( orBuilder.subobjStart() ) ;
            rc = utilGetCSBounds( FIELD_NAME_ORIGIN_ID, csUniqueID, clBuilder ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get bounds of recycled "
                         "collections with collection space unique ID [%u], "
                         "rc: %d", csUniqueID, rc ) ;
            clBuilder.doneFast() ;

            orBuilder.doneFast() ;
            matcher = builder.obj() ;
         }
         catch ( exception &e )
         {
            PD_LOG( PDERROR, "Failed to build matcher, occur exception %s",
                    e.what() ) ;
            rc = ossException2RC( &e ) ;
            goto error ;
         }
      }
      else
      {
         rc = utilGetRecyCLsInCSBounds( FIELD_NAME_ORIGIN_ID,
                                        csUniqueID,
                                        matcher ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get bounds of recycled "
                      "collections with collection space unique ID [%u], "
                      "rc: %d", csUniqueID, rc ) ;
      }

      rc = _deleteItems( matcher, cb, w, deletedCount ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to delete recycle items in collection "
                   "space [%u], rc: %d", csUniqueID, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__CLSRECYBINMGR__DELITEMSINCS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSRECYBINMGR_GETSUBITEMS, "_clsRecycleBinManager::getSubItems" )
   INT32 _clsRecycleBinManager::getSubItems( const utilRecycleItem &item,
                                             pmdEDUCB *cb,
                                             UTIL_RECY_ITEM_LIST &itemList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSRECYBINMGR_GETSUBITEMS ) ;

      BSONObj matcher, dummy ;
      utilRecycleID recycleID = item.getRecycleID() ;

      try
      {
         matcher = BSON( FIELD_NAME_RECYCLE_ID << (INT64)( recycleID ) ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build matcher, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

      rc = _BASE::_getItems( matcher, dummy, _hintRecyID, -1, cb, itemList ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get recycle items with recycle "
                   "ID [%llu], rc: %d", recycleID, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__CLSRECYBINMGR_GETSUBITEMS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _clsDropRecycleBinBGJob implement
    */
   _clsDropRecycleBinBGJob::_clsDropRecycleBinBGJob(
                                          rtnRecycleBinManager *recycleBinMgr )
   : _rtnDropRecycleBinBGJob( recycleBinMgr )
   {
      SDB_ASSERT( NULL != _recycleBinMgr,
                  "recycle bin manager is invalid" ) ;
   }

   _clsDropRecycleBinBGJob::~_clsDropRecycleBinBGJob()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSRECYBINBGJOB_DOIT, "_clsDropRecycleBinBGJob::doit" )
   INT32 _clsDropRecycleBinBGJob::doit( IExecutor *pExe,
                                        UTIL_LJOB_DO_RESULT &result,
                                        UINT64 &sleepTime )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSRECYBINBGJOB_DOIT ) ;

      if ( !_recycleBinMgr->isConfValid() )
      {
         shardCB *shardCB = sdbGetShardCB() ;

         // update DC from remote
         rc = shardCB->updateDCBaseInfo() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to update DC info from CATALOG, "
                      "rc: %d", rc ) ;

         _recycleBinMgr->setConf(
               shardCB->getDCMgr()->getDCBaseInfo()->getRecycleBinConf() ) ;
      }

      rc = _rtnDropRecycleBinBGJob::doit( pExe, result, sleepTime ) ;

   done:
      PD_TRACE_EXITRC( SDB__CLSRECYBINBGJOB_DOIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
