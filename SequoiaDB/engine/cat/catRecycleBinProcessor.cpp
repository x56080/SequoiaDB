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

   Source File Name = catRecycleBinProcessor.cpp

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

#include "catRecycleBinProcessor.hpp"
#include "catCommon.hpp"
#include "catalogueCB.hpp"
#include "catRecycleBinManager.hpp"
#include "rtn.hpp"
#include "catGTSDef.hpp"
#include "clsCatalogAgent.hpp"
#include "pdTrace.hpp"
#include "catTrace.hpp"
#include "../bson/bson.hpp"

using namespace bson ;

namespace engine
{

   /*
      _catRecycleBinProcessor implement
    */
   _catRecycleBinProcessor::_catRecycleBinProcessor( _catRecycleBinManager *recyBinMgr,
                                                     utilRecycleItem &item )
   : _recyBinMgr( recyBinMgr ),
     _item( item ),
     _matchedCount( 0 ),
     _processedCount( 0 )
   {
   }

   _catRecycleBinProcessor::~_catRecycleBinProcessor()
   {
   }

   /*
      _catDropItemSubCLChecker implement
    */
   _catDropItemSubCLChecker::_catDropItemSubCLChecker( _catRecycleBinManager *recyBinMgr,
                                                       utilRecycleItem &item )
   : _catRecycleBinProcessor( recyBinMgr, item )
   {
   }

   _catDropItemSubCLChecker::~_catDropItemSubCLChecker()
   {
   }

   const CHAR *_catDropItemSubCLChecker::getCollection() const
   {
      return catGetRecycleBinCL( UTIL_RECYCLE_CL ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATDROPITEMSUBCLCHK_GETMATCHER, "_catDropItemSubCLChecker::getMatcher" )
   INT32 _catDropItemSubCLChecker::getMatcher( ossPoolList< BSONObj > &matcherList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATDROPITEMSUBCLCHK_GETMATCHER ) ;

      try
      {
         // matched by recycle ID and main-collection name
         BSONObj matcher = BSON( FIELD_NAME_RECYCLE_ID <<
                                 (INT64)( _item.getRecycleID() ) <<
                                 FIELD_NAME_MAINCLNAME <<
                                 _item.getOriginName() ) ;
         matcherList.push_back( matcher ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build matcher for return checker, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATDROPITEMSUBCLCHK_GETMATCHER, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATDROPITEMSUBCLCHK_PROCESSOBJ, "_catDropItemSubCLChecker::processObject" )
   INT32 _catDropItemSubCLChecker::processObject( const BSONObj &object,
                                                  pmdEDUCB *cb,
                                                  INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATDROPITEMSUBCLCHK_PROCESSOBJ ) ;

      utilGlobalID uniqueID = UTIL_GLOBAL_NULL ;
      utilCSUniqueID csUniqueID = UTIL_UNIQUEID_NULL ;
      utilRecycleItem csItem ;
      const CHAR *subCLName = NULL ;

      rc = catParseUniqueID( object, uniqueID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get unique ID from object, "
                   "rc: %d", rc ) ;
      csUniqueID = utilGetCSUniqueID( (utilCLUniqueID)uniqueID ) ;

      if ( _isChecked( csUniqueID ) )
      {
         // already checked
         goto done ;
      }

      // check if has a recycled collection space
      rc = _recyBinMgr->getItem( (utilRecycleID)csUniqueID, cb, csItem ) ;
      if ( SDB_RECYCLE_ITEMNOTEXISTS == rc )
      {
         // if not found, it is safe to drop
         // save to checked list to avoid duplicated checks
         rc = _saveChecked( csUniqueID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to save checked collection space, "
                      "rc: %d", rc ) ;
         goto done ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get recycle item of collection "
                   "space, rc: %d", rc ) ;

      // found recycled collection space, report error, we should not drop it
      rc = rtnGetStringElement( object, CAT_COLLECTION_NAME, &subCLName ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s] from object, "
                   "rc: %d", CAT_COLLECTION_NAME, rc ) ;
      PD_LOG_MSG_CHECK( FALSE, SDB_RECYCLE_CONFLICT, error, PDERROR,
                        "Failed to drop collection recycle item "
                        "[origin %s, recycle %s] with sub-collection [%s], "
                        "collection space recycle item "
                        "[origin: %s, recycle: %s] is also "
                        "in recycle bin", _item.getOriginName(),
                        _item.getRecycleName(), subCLName,
                        csItem.getOriginName(), csItem.getRecycleName() ) ;

   done:
      PD_TRACE_EXITRC( SDB_CATDROPITEMSUBCLCHK_PROCESSOBJ, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATDROPITEMSUBCLCHK__ISCHECKED, "_catDropItemSubCLChecker::_isChecked" )
   BOOLEAN _catDropItemSubCLChecker::_isChecked( utilCSUniqueID csUniqueID ) const
   {
      BOOLEAN isChecked = FALSE ;

      PD_TRACE_ENTRY( SDB_CATDROPITEMSUBCLCHK__ISCHECKED ) ;

      if ( _checkedSet.find( csUniqueID ) != _checkedSet.end() )
      {
         isChecked = TRUE ;
      }

      PD_TRACE_EXIT( SDB_CATDROPITEMSUBCLCHK__ISCHECKED ) ;

      return isChecked ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATDROPITEMSUBCLCHK__SAVECHECKED, "_catDropItemSubCLChecker::_saveChecked" )
   INT32 _catDropItemSubCLChecker::_saveChecked( utilCSUniqueID csUniqueID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATDROPITEMSUBCLCHK__SAVECHECKED ) ;

      try
      {
         _checkedSet.insert( csUniqueID ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to save checked collection space, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXIT( SDB_CATDROPITEMSUBCLCHK__SAVECHECKED ) ;
      return rc ;

   error:
      goto done ;
   }

}
