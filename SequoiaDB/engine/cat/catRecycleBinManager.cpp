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

   Source File Name = catRecycleBinManager.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for catalog node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/03/2021  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "catCommon.hpp"
#include "catRecycleBinManager.hpp"
#include "rtn.hpp"
#include "pdTrace.hpp"
#include "catTrace.hpp"
#include "../bson/bson.hpp"

using namespace bson ;

namespace engine
{

   // maximum retry count for dropping oldest recycle item
   #define CAT_RECYCLE_MAX_RETRY ( 3 )

   /*
      _catRecycleBinManager implement
    */
   _catRecycleBinManager::_catRecycleBinManager()
   : _BASE()
   {
   }

   _catRecycleBinManager::~_catRecycleBinManager()
   {
   }



   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYBINMGR_INIT, "_catRecycleBinManager::init" )
   INT32 _catRecycleBinManager::init( const utilRecycleBinConf &conf )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYBINMGR_INIT ) ;

      rc = _BASE::init() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init based recycle bin manager, "
                   "rc: %d", rc ) ;

      // set configure in cache
      setConf( conf ) ;

   done:
      PD_TRACE_EXITRC( SDB__CATRECYBINMGR_INIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }


   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYBINMGR_ACTIVE, "_catRecycleBinManager::active" )
   INT32 _catRecycleBinManager::active( const utilRecycleBinConf &conf )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYBINMGR_ACTIVE ) ;

      // set configure in cache
      setConf( conf ) ;

      PD_TRACE_EXITRC( SDB__CATRECYBINMGR_ACTIVE, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYBINMGR_UPDATECONF, "_catRecycleBinManager::updateConf" )
   INT32 _catRecycleBinManager::updateConf( const utilRecycleBinConf &newConf,
                                            pmdEDUCB *cb,
                                            INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYBINMGR_UPDATECONF ) ;

      rc = catUpdateRecycleBinConf( newConf, cb, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update recycle bin conf, "
                   "rc: %d", rc ) ;

      setConf( newConf ) ;

   done:
      PD_TRACE_EXITRC( SDB__CATRECYBINMGR_UPDATECONF, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYBINMGR__GETITEMSINCS, "_catRecycleBinManager::_getItemsInCS" )
   INT32 _catRecycleBinManager::_getItemsInCS( utilCSUniqueID csUniqueID,
                                               pmdEDUCB *cb,
                                               UTIL_RECY_ITEM_LIST &itemList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYBINMGR__GETITEMSINCS ) ;

      BSONObj matcher, dummy ;

      rc = utilGetRecyCLsInCSBounds( FIELD_NAME_ORIGIN_ID,
                                     csUniqueID,
                                     matcher ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get bounds of recycled collections "
                   "with collection space unique ID [%u], rc: %d", csUniqueID,
                   rc ) ;

      rc = _getItems( matcher, dummy, dummy, -1, cb, itemList ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get recycle items from "
                   "collection space [%u], rc: %d", csUniqueID, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__CATRECYBINMGR__GETITEMSINCS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYBINMGR_DROPITEM, "_catRecycleBinManager::dropItem" )
   INT32 _catRecycleBinManager::dropItem( const utilRecycleItem &item,
                                          pmdEDUCB *cb,
                                          INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYBINMGR_DROPITEM ) ;

      BOOLEAN isDropped = FALSE ;

      rc = _dropItem( item, cb, w, TRUE, isDropped ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to drop recycle item [origin %s, "
                   "recycle %s], rc: %d", item.getOriginName(),
                   item.getRecycleName(), rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__CATRECYBINMGR_DROPITEM, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYBINMGR__DROPITEM, "_catRecycleBinManager::_dropItem" )
   INT32 _catRecycleBinManager::_dropItem( const utilRecycleItem &item,
                                           pmdEDUCB *cb,
                                           INT16 w,
                                           BOOLEAN ignoreNotExists,
                                           BOOLEAN &isDropped )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYBINMGR__DROPITEM ) ;

      isDropped = FALSE ;

      if ( UTIL_RECYCLE_CS == item.getType() )
      {
         UTIL_RECY_ITEM_LIST relatedItems ;
         utilCSUniqueID csUniqueID = (utilCSUniqueID)( item.getOriginID() ) ;
         rc = _getItemsInCS( csUniqueID, cb, relatedItems ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get related collection "
                      "recycle items, rc: %d", rc ) ;

         for ( UTIL_RECY_ITEM_LIST_IT iter = relatedItems.begin() ;
               iter != relatedItems.end() ;
               ++ iter )
         {
            utilRecycleItem &tmpItem = *iter ;
            rc = _dropItemImpl( tmpItem, cb, w, TRUE ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to drop recycle item "
                         "[origin %s, recycle %s], rc: %d",
                         tmpItem.getOriginName(), tmpItem.getRecycleName(),
                         rc ) ;
         }
      }

      rc = _dropItemImpl( item, cb, w, ignoreNotExists ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to drop recycle item [origin %s, "
                   "recycle %s], rc: %d",  item.getOriginName(),
                   item.getRecycleName(), rc ) ;

      isDropped = TRUE ;

   done:
      PD_TRACE_EXITRC( SDB__CATRECYBINMGR__DROPITEM, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYBINMGR__DROPITEMIMPL, "_catRecycleBinManager::_dropItemImpl" )
   INT32 _catRecycleBinManager::_dropItemImpl( const utilRecycleItem &item,
                                               pmdEDUCB *cb,
                                               INT16 w,
                                               BOOLEAN ignoreNotExists )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYBINMGR__DROPITEMIMPL ) ;

      BSONObj matcher ;

      try
      {
         matcher = BSON( FIELD_NAME_RECYCLE_ID <<
                         (INT64)( item.getRecycleID() ) ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build matcher, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

      if ( UTIL_RECYCLE_CS == item.getType() )
      {
         rc = _deleteObjects( UTIL_RECYCLE_IDX, matcher, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to delete recycle "
                      "index objects, rc: %d", rc ) ;

         rc = _deleteObjects( UTIL_RECYCLE_SEQ, matcher, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to delete recycle "
                      "sequence objects, rc: %d", rc ) ;

         rc = _deleteObjects( UTIL_RECYCLE_CL, matcher, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to delete recycle "
                      "collection objects, rc: %d", rc ) ;

         rc = _deleteObjects( UTIL_RECYCLE_CS, matcher, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to delete recycle "
                      "collection space objects, rc: %d", rc ) ;
      }
      else if ( UTIL_RECYCLE_CL == item.getType() )
      {
         rc = _deleteObjects( UTIL_RECYCLE_IDX, matcher, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to delete recycle "
                      "index objects, rc: %d", rc ) ;

         rc = _deleteObjects( UTIL_RECYCLE_SEQ, matcher, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to delete recycle "
                      "sequence objects, rc: %d", rc ) ;

         rc = _deleteObjects( UTIL_RECYCLE_CL, matcher, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to delete recycle "
                      "collection objects, rc: %d", rc ) ;
      }
      else
      {
         SDB_ASSERT( FALSE, "invalid recycle type" ) ;
         PD_LOG( PDWARNING, "Found invalid recycle type [%d]",
                 item.getType() ) ;
      }

      rc = _deleteItem( item, cb, w, ignoreNotExists ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to delete recycle item [%s], rc: %d",
                   item.getRecycleName(), rc ) ;

      PD_LOG( PDEVENT, "Dropped recycle item [origin %s, recycle %s]",
              item.getOriginName(), item.getRecycleName() ) ;

   done:
      PD_TRACE_EXITRC( SDB__CATRECYBINMGR__DROPITEMIMPL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYBINMGR_DROPALLITEMS, "_catRecycleBinManager::dropAllItems" )
   INT32 _catRecycleBinManager::dropAllItems( pmdEDUCB *cb, INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYBINMGR_DROPALLITEMS ) ;

      BSONObj dummy ;

      rc = _deleteObjects( UTIL_RECYCLE_IDX, dummy, cb, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to delete recycle "
                   "index objects, rc: %d", rc ) ;

      rc = _deleteObjects( UTIL_RECYCLE_SEQ, dummy, cb, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to delete all recycle "
                   "sequence objects, rc: %d", rc ) ;

      rc = _deleteObjects( UTIL_RECYCLE_CL, dummy, cb, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to delete all recycle "
                   "collection objects, rc: %d", rc ) ;

      rc = _deleteObjects( UTIL_RECYCLE_CS, dummy, cb, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to delete all recycle "
                   "collection space objects, rc: %d", rc ) ;

      rc = _deleteAllItems( cb, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to delete all recycle items, rc: %d",
                   rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__CATRECYBINMGR_DROPALLITEMS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYBINMGR__DELOBJS, "_catRecycleBinManager::_deleteObjects" )
   INT32 _catRecycleBinManager::_deleteObjects( UTIL_RECYCLE_TYPE type,
                                                const bson::BSONObj &matcher,
                                                pmdEDUCB *cb,
                                                INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYBINMGR__DELOBJS ) ;

      rtnQueryOptions options ;
      const CHAR *recycleCollection = catGetRecycleBinCL( type ) ;

      options.setCLFullName( recycleCollection ) ;
      options.setQuery( matcher ) ;

      rc = rtnDelete( options, cb, _dmsCB, _dpsCB, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to delete objects from recycle "
                   "collection [%s], rc: %d", recycleCollection, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__CATRECYBINMGR__DELOBJS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYBINMGR_COUNTITEMSINCS, "_catRecycleBinManager::countItemsInCS" )
   INT32 _catRecycleBinManager::countItemsInCS( utilCSUniqueID csUniqueID,
                                                pmdEDUCB *cb,
                                                INT64 &count )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYBINMGR_COUNTITEMSINCS ) ;

      BSONObj matcher ;

      rc = utilGetRecyCLsInCSBounds( FIELD_NAME_ORIGIN_ID,
                                     csUniqueID,
                                     matcher ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get bounds of recycled collections "
                   "with collection space unique ID [%u], rc: %d", csUniqueID,
                   rc ) ;

      rc = _countItems( matcher, cb, count ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get counts of items for "
                   "collection space [%u], rc: %d", csUniqueID, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__CATRECYBINMGR_COUNTITEMSINCS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYBINMGR_PROCESSOBJS, "_catRecycleBinManager::processObjects" )
   INT32 _catRecycleBinManager::processObjects( catRecycleBinProcessor &processor,
                                                pmdEDUCB *cb,
                                                INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYBINMGR_PROCESSOBJS ) ;

      ossPoolList< BSONObj > matcherList ;

      rc = processor.getMatcher( matcherList ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get matcher from processor [%s], "
                   "rc: %d", processor.getName(), rc ) ;

      for ( ossPoolList< BSONObj >::iterator iter = matcherList.begin() ;
            iter != matcherList.end() ;
            ++ iter )
      {
         const BSONObj &matcher = (*iter) ;
         rc = _processObjects( processor, matcher, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to process objects with "
                      "processor [%s], rc: %d", processor.getName(), rc ) ;
      }

      if ( processor.getExpectedCount() > 0 )
      {
         PD_CHECK( processor.getMatchedCount() ==
                                     (UINT32)( processor.getExpectedCount() ),
                   SDB_CAT_CORRUPTION, error, PDERROR,
                   "Failed to check processor [%s], should have [%d] "
                   "matched objects, only found [%u]",
                   processor.getName(), processor.getExpectedCount(),
                   processor.getMatchedCount() ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CATRECYBINMGR_PROCESSOBJS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYBINMGR__PROCESSOBJS, "_catRecycleBinManager::_processObjects" )
   INT32 _catRecycleBinManager::_processObjects( catRecycleBinProcessor &processor,
                                                 const BSONObj &matcher,
                                                 pmdEDUCB *cb,
                                                 INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYBINMGR__PROCESSOBJS ) ;

      rtnQueryOptions options ;
      const CHAR *collection = NULL ;
      INT64 contextID = -1 ;

      collection = processor.getCollection() ;
      SDB_ASSERT( NULL != collection, "collection is invalid" ) ;

      options.setCLFullName( collection ) ;
      options.setQuery( matcher ) ;

      rc = rtnQuery( options, cb, _dmsCB, _rtnCB, contextID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to query collection [%s], rc: %d",
                   collection, rc ) ;

      while ( TRUE )
      {
         rtnContextBuf buffObj ;
         BSONObj object ;

         rc = rtnGetMore( contextID, 1, buffObj, cb, _rtnCB ) ;
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_OK ;
            contextID = -1 ;
            goto done ;
         }

         try
         {
            object = BSONObj( buffObj.data() ) ;
         }
         catch ( exception &e )
         {
            PD_LOG( PDERROR, "Failed to parse origin object, "
                    "occur exception %s", e.what() ) ;
            rc = ossException2RC( &e ) ;
            goto error ;
         }

         processor.increaseMatchedCount() ;

         rc = processor.processObject( object, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to process object, rc: %d", rc ) ;

         processor.increaseProcessedCount() ;
      }

   done:
      if ( -1 != contextID )
      {
         _rtnCB->contextDelete( contextID, cb ) ;
      }
      PD_TRACE_EXITRC( SDB__CATRECYBINMGR__PROCESSOBJS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
