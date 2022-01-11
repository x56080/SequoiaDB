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

   Source File Name = rtnRecycleBinManager.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for recycle bin manager.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/03/2021  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "rtnRecycleBinManager.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"

namespace engine
{

   /*
      _rtnRecycleBinManager implement
    */
   _rtnRecycleBinManager::_rtnRecycleBinManager()
   : _isConfValid( FALSE ),
     _rtnCB( NULL ),
     _dmsCB( NULL ),
     _dpsCB( NULL )
   {
   }

   _rtnRecycleBinManager::~_rtnRecycleBinManager()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNRECYBINMGR_INIT, "_rtnRecycleBinManager::init" )
   INT32 _rtnRecycleBinManager::init()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNRECYBINMGR_INIT ) ;

      pmdKRCB *krcb = pmdGetKRCB() ;
      _rtnCB = krcb->getRTNCB() ;
      _dmsCB = krcb->getDMSCB() ;
      _dpsCB = krcb->getDPSCB() ;

      PD_TRACE_EXITRC( SDB__RTNRECYBINMGR_INIT, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNRECYBINMGR_GETITEM_RECYNAME, "_rtnRecycleBinManager::getItem" )
   INT32 _rtnRecycleBinManager::getItem( const CHAR *recycleName,
                                         pmdEDUCB *cb,
                                         utilRecycleItem &item )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNRECYBINMGR_GETITEM_RECYNAME ) ;

      BSONObj itemObject ;

      rc = _getItemObject( recycleName, cb, itemObject ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get recycle item [%s], rc: %d",
                   recycleName, rc ) ;

      rc = item.fromBSON( itemObject ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse recycle item [%s] from BSON, "
                   "rc: %d", recycleName, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNRECYBINMGR_GETITEM_RECYNAME, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNRECYBINMGR_GETITEM_ORIGID, "_rtnRecycleBinManager::getItem" )
   INT32 _rtnRecycleBinManager::getItem( utilGlobalID originID,
                                         pmdEDUCB *cb,
                                         utilRecycleItem &item )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNRECYBINMGR_GETITEM_ORIGID ) ;

      BSONObj itemObject ;

      rc = _getItemObject( originID, cb, itemObject ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to get recycle item "
                   "[origin ID: %llu], rc: %d", originID, rc ) ;

      rc = item.fromBSON( itemObject ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse recycle item "
                   "[origin ID: %llu] from BSON, rc: %d", originID, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNRECYBINMGR_GETITEM_ORIGID, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNRECYBINMGR__GETITEMOBJ_RECYNAME, "_rtnRecycleBinManager::_getItemObject" )
   INT32 _rtnRecycleBinManager::_getItemObject( const CHAR *recycleName,
                                                pmdEDUCB *cb,
                                                BSONObj &object )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNRECYBINMGR__GETITEMOBJ_RECYNAME ) ;

      BSONObj matcher ;

      try
      {
         matcher = BSON( FIELD_NAME_RECYCLE_NAME << recycleName ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build matcher, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

      rc = _getItemObject( matcher, cb, object ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get recycle item [%s], rc: %d",
                   recycleName, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNRECYBINMGR__GETITEMOBJ_RECYNAME, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNRECYBINMGR__GETITEMOBJ_ORIGID, "_rtnRecycleBinManager::_getItemObject" )
   INT32 _rtnRecycleBinManager::_getItemObject( utilGlobalID originID,
                                                pmdEDUCB *cb,
                                                BSONObj &object )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNRECYBINMGR__GETITEMOBJ_ORIGID ) ;

      BSONObj matcher ;

      try
      {
         matcher = BSON( FIELD_NAME_ORIGIN_ID << (INT64)originID ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build matcher, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

      rc = _getItemObject( matcher, cb, object ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to get recycle item "
                   "[origin ID: %llu], rc: %d", originID, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNRECYBINMGR__GETITEMOBJ_ORIGID, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNRECYBINMGR__GETITEMOBJ, "_rtnRecycleBinManager::_getItemObject" )
   INT32 _rtnRecycleBinManager::_getItemObject( const BSONObj &matcher,
                                                pmdEDUCB *cb,
                                                BSONObj &object )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNRECYBINMGR__GETITEMOBJ ) ;

      BSONObj dummy ;
      INT64 contextID = -1 ;
      rtnContextBuf buffObj ;

      rc = _getItems( matcher, dummy, dummy, 1, cb, contextID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get recycle item, rc: %d", rc ) ;

      // get more
      rc = rtnGetMore( contextID, 1, buffObj, cb, _rtnCB ) ;
      if ( SDB_OK != rc )
      {
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_RECYCLE_ITEMNOTEXISTS ;
         }
         contextID = -1 ;
         PD_RC_CHECK( rc, PDWARNING, "Failed to get recycle item from context, "
                      "rc: %d", rc ) ;
      }

      try
      {
         BSONObj resultObj( buffObj.data() ) ;
         object = resultObj.copy() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to copy BSON object, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      if ( -1 == contextID )
      {
         _rtnCB->contextDelete( contextID, cb ) ;
      }
      PD_TRACE_EXITRC( SDB__RTNRECYBINMGR__GETITEMOBJ, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNRECYBINMGR__GETITEMS, "_rtnRecycleBinManager::_getItems" )
   INT32 _rtnRecycleBinManager::_getItems( const BSONObj &matcher,
                                           const BSONObj &orderBy,
                                           const BSONObj &hint,
                                           INT64 numToReturn,
                                           pmdEDUCB *cb,
                                           INT64 &contextID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNRECYBINMGR__GETITEMS ) ;

      rtnQueryOptions options ;
      options.setCLFullName( _getRecyItemCL() ) ;
      options.setQuery( matcher ) ;
      options.setOrderBy( orderBy ) ;
      options.setHint( hint ) ;
      options.setLimit( numToReturn ) ;

      // perform query
      rc = rtnQuery( options, cb, _dmsCB, _rtnCB, contextID ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to execute query on collection [%s], "
                    "rc: %d", _getRecyItemCL(), rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNRECYBINMGR__GETITEMS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNRECYBINMGR__GETITEMS_LIST, "_rtnRecycleBinManager::_getItems" )
   INT32 _rtnRecycleBinManager::_getItems( const BSONObj &matcher,
                                           const BSONObj &orderBy,
                                           const BSONObj &hint,
                                           INT64 numToReturn,
                                           pmdEDUCB *cb,
                                           UTIL_RECY_ITEM_LIST &itemList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNRECYBINMGR__GETITEMS_LIST ) ;

      INT64 contextID = -1 ;

      // perform query
      rc = _getItems( matcher, orderBy, hint, numToReturn, cb, contextID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to query recycle items, "
                   "rc: %d", rc ) ;

      while ( TRUE )
      {
         rtnContextBuf buffObj ;

         // get one result
         rc = rtnGetMore( contextID, 1, buffObj, cb, _rtnCB ) ;
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_OK ;
            contextID = -1 ;
            goto done ;
         }

         try
         {
            utilRecycleItem item ;

            // parse object
            BSONObj object( buffObj.data() ) ;
            rc = item.fromBSON( object ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get item from BSON, rc: %d",
                         rc ) ;

            // save to item list
            itemList.push_back( item ) ;
         }
         catch ( exception &e )
         {
            PD_LOG( PDERROR, "Failed to parse recycle item from BSON, "
                    "occur exception %s", e.what() ) ;
            rc = ossException2RC( &e ) ;
            goto error ;
         }
      }

   done:
      if ( -1 != contextID )
      {
         _rtnCB->contextDelete( contextID, cb ) ;
      }
      PD_TRACE_EXITRC( SDB__RTNRECYBINMGR__GETITEMS_LIST, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNRECYBINMGR__COUNTITEMS, "_rtnRecycleBinManager::_countItems" )
   INT32 _rtnRecycleBinManager::_countItems( const BSONObj &matcher,
                                             pmdEDUCB *cb,
                                             INT64 &count )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNRECYBINMGR__COUNTITEMS ) ;

      rtnQueryOptions options ;
      options.setCLFullName( _getRecyItemCL() ) ;
      options.setQuery( matcher ) ;

      rc = rtnGetCount( options, _dmsCB, cb, _rtnCB, &count ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get count on collection [%s], "
                   "rc: %d", _getRecyItemCL(), rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNRECYBINMGR__COUNTITEMS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
