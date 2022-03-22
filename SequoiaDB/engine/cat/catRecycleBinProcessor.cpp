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
      return catGetRecycleBinRecyCL( UTIL_RECYCLE_CL ) ;
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

   /*
      _catRecycleProcessor implement
    */
   _catRecycleProcessor::_catRecycleProcessor( _catRecycleBinManager *recyBinMgr,
                                               utilRecycleItem &item,
                                               UTIL_RECYCLE_TYPE type )
   : _catRecycleBinProcessor( recyBinMgr, item ),
     _type( type )
   {
      pmdKRCB *krcb = pmdGetKRCB() ;
      _dmsCB = krcb->getDMSCB() ;
      _dpsCB = krcb->getDPSCB() ;
   }

   _catRecycleProcessor::~_catRecycleProcessor()
   {
   }

   const CHAR *_catRecycleProcessor::getCollection() const
   {
      return  catGetRecycleBinMetaCL( _type ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYPROCESS_PROCESSOBJ, "_catRecycleProcessor::processObject" )
   INT32 _catRecycleProcessor::processObject( const BSONObj &object,
                                              pmdEDUCB *cb,
                                              INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYPROCESS_PROCESSOBJ ) ;

      BSONObj returnObject ;

      rc = _buildObject( object, cb, w, returnObject ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build return object, rc: %d", rc ) ;

      rc = _saveObject( returnObject, cb, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to save object, rc: %d", rc ) ;

      rc = _postSaveObject( object, returnObject, cb, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to process recycle object after save "
                   "to recycle collection, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__CATRECYPROCESS_PROCESSOBJ, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYPROCESS__SAVEOBJ, "_catRecycleProcessor::_saveObject" )
   INT32 _catRecycleProcessor::_saveObject( const BSONObj &returnObject,
                                            pmdEDUCB *cb,
                                            INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYPROCESS__SAVEOBJ ) ;

      const CHAR *recycleCollection = catGetRecycleBinRecyCL( _type ) ;

      rc = rtnInsert( recycleCollection, returnObject, 1, 0, cb, _dmsCB,
                      _dpsCB, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to insert object to collection [%s], "
                   "rc: %d", recycleCollection, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__CATRECYPROCESS__SAVEOBJ, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYPROCESS__BLDOBJ, "_catRecycleProcessor::_buildObject" )
   INT32 _catRecycleProcessor::_buildObject( const bson::BSONObj &originObject,
                                             pmdEDUCB *cb,
                                             INT16 w,
                                             BSONObj &recycleObject )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYPROCESS__BLDOBJ ) ;

      try
      {
         BSONObjBuilder builder ;

         // initialize a new recycle OID
         OID recycleOID = OID::gen() ;
         builder.append( DMS_ID_KEY_NAME, recycleOID ) ;
         builder.append( FIELD_NAME_RECYCLE_ID,
                         (INT64)( _item.getRecycleID() ) ) ;

         BSONObjIterator iter( originObject ) ;
         while ( iter.more() )
         {
            BSONElement element = iter.next() ;
            if ( 0 == ossStrcmp( DMS_ID_KEY_NAME, element.fieldName() ) )
            {
               continue ;
            }
            else
            {
               builder.append( element ) ;
            }
         }

         recycleObject = builder.obj() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build recycle object, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CATRECYPROCESS__BLDOBJ, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _catRecycleCSProcessor implement
    */
   _catRecycleCSProcessor::_catRecycleCSProcessor( _catRecycleBinManager *recyBinMgr,
                                                   utilRecycleItem &item )
   : _catRecycleProcessor( recyBinMgr, item, UTIL_RECYCLE_CS )
   {
   }

   _catRecycleCSProcessor::~_catRecycleCSProcessor()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYCSPROCESS_GETMATCHER, "_catRecycleCSProcessor::getMatcher" )
   INT32 _catRecycleCSProcessor::getMatcher( ossPoolList< BSONObj > &matcherList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYCSPROCESS_GETMATCHER ) ;

      try
      {
         utilCSUniqueID csUniqueID = (utilCSUniqueID)( _item.getOriginID() ) ;
         BSONObj matcher = BSON( FIELD_NAME_UNIQUEID << (INT32)csUniqueID ) ;
         matcherList.push_back( matcher ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build matcher, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CATRECYCSPROCESS_GETMATCHER, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _catRecycleCLProcessor implement
    */
   _catRecycleCLProcessor::_catRecycleCLProcessor( _catRecycleBinManager *recyBinMgr,
                                                   utilRecycleItem &item )
   : _catRecycleProcessor( recyBinMgr, item, UTIL_RECYCLE_CL )
   {
   }

   _catRecycleCLProcessor::~_catRecycleCLProcessor()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYCLPROCESS_GETMATCHER, "_catRecycleCLProcessor::getMatcher" )
   INT32 _catRecycleCLProcessor::getMatcher( ossPoolList< BSONObj > &matcherList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYCLPROCESS_GETMATCHER ) ;

      try
      {
         BSONObj matcher ;

         if ( UTIL_RECYCLE_CS == _item.getType() )
         {
            utilCSUniqueID csUniqueID =
                              (utilCSUniqueID)( _item.getOriginID() ) ;
            rc = utilGetCSBounds( FIELD_NAME_UNIQUEID, csUniqueID, matcher ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get bounds with collection "
                         "space unique ID [%u], rc: %d", csUniqueID, rc ) ;
         }
         else if ( UTIL_RECYCLE_CL == _item.getType() )
         {
            utilCLUniqueID clUniqueID =
                              (utilCLUniqueID)( _item.getOriginID() ) ;
            matcher = BSON( FIELD_NAME_UNIQUEID << (INT64)clUniqueID ) ;
         }
         else
         {
            SDB_ASSERT( FALSE, "invalid recycle type" ) ;
            PD_CHECK( FALSE, SDB_SYS, error, PDERROR, "Failed to recycle "
                      "collection objects, invalid recycle type [%d]",
                      _item.getType() ) ;
         }

         matcherList.push_back( matcher ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build matcher, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CATRECYCLPROCESS_GETMATCHER, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYCLPROCESS__POSTSAVEOBJ, "_catRecycleCLProcessor::_postSaveObject" )
   INT32 _catRecycleCLProcessor::_postSaveObject( const BSONObj &originObject,
                                                  BSONObj &recycleObject,
                                                  pmdEDUCB *cb,
                                                  INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYCLPROCESS__POSTSAVEOBJ ) ;

      const CHAR *collectionName = NULL ;

      if ( UTIL_RECYCLE_CL != _item.getType() )
      {
         goto done ;
      }

      // to recycle main-collection, recycle its sub-collections
      try
      {
         BSONElement element = originObject.getField( CAT_COLLECTION_NAME ) ;
         PD_CHECK( String == element.type(), SDB_SYS, error, PDERROR,
                   "Failed to get field [%s], it is not a string" ) ;
         collectionName = element.valuestr() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to get collection name, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

      {
         clsCatalogSet originSet( collectionName ) ;
         rc = originSet.updateCatSet( originObject ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse catalog for collection "
                      "[%s], rc: %d", _item.getOriginName(), rc ) ;

         // check if a main-collection
         if ( originSet.isMainCL() )
         {
            _item.setMainCL( TRUE ) ;

            // recycle sub-collections
            catRecycleSubCLProcessor subCLProcessor( _recyBinMgr, _item ) ;

            rc = _recyBinMgr->processObjects( subCLProcessor, cb, w ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to recycle sub-collections, "
                         "rc: %d", rc ) ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__CATRECYCLPROCESS__POSTSAVEOBJ, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _catRecycleSubCLProcessor implement
    */
   _catRecycleSubCLProcessor::_catRecycleSubCLProcessor( _catRecycleBinManager *recyBinMgr,
                                                         utilRecycleItem &item )
   : _catRecycleProcessor( recyBinMgr, item, UTIL_RECYCLE_CL )
   {
   }

   _catRecycleSubCLProcessor::~_catRecycleSubCLProcessor()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYSUBCLPROCESS_GETMATCHER, "_catRecycleSubCLProcessor::getMatcher" )
   INT32 _catRecycleSubCLProcessor::getMatcher( ossPoolList< BSONObj > &matcherList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYSUBCLPROCESS_GETMATCHER ) ;

      try
      {
         BSONObj matcher ;

         if ( UTIL_RECYCLE_CL == _item.getType() )
         {
            matcher = BSON( CAT_MAINCL_NAME << _item.getOriginName() ) ;
         }
         else
         {
            SDB_ASSERT( FALSE, "invalid recycle type" ) ;
            PD_CHECK( FALSE, SDB_SYS, error, PDERROR, "Failed to recycle "
                      "collection objects, invalid recycle type [%d]",
                      _item.getType() ) ;
         }

         matcherList.push_back( matcher ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build matcher, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CATRECYSUBCLPROCESS_GETMATCHER, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYSUBCLPROCESS__POSTSAVEOBJ, "_catRecycleSubCLProcessor::_postSaveObject" )
   INT32 _catRecycleSubCLProcessor::_postSaveObject( const BSONObj &originObject,
                                                     BSONObj &recycleObject,
                                                     pmdEDUCB *cb,
                                                     INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYSUBCLPROCESS__POSTSAVEOBJ ) ;

      const CHAR *collectionName = NULL ;

      if ( UTIL_RECYCLE_CL != _item.getType() )
      {
         goto done ;
      }

      try
      {
         BSONElement element = originObject.getField( CAT_COLLECTION_NAME ) ;
         PD_CHECK( String == element.type(), SDB_SYS, error, PDERROR,
                   "Failed to get field [%s], it is not a string" ) ;
         collectionName = element.valuestr() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to get collection name, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

      {
         clsCatalogSet originSet( collectionName ) ;
         rc = originSet.updateCatSet( originObject ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse catalog for collection "
                      "[%s], rc: %d", _item.getOriginName(), rc ) ;

         SDB_ASSERT( originSet.isSubCL(), "should be sub-collection" ) ;

         if ( originSet.isSubCL() )
         {
            utilRecycleItem subItem ;
            subItem.inherit( _item,
                             collectionName,
                             (utilGlobalID)( originSet.clUniqueID() ) ) ;

            catRecycleSeqProcessor seqProcessor( _recyBinMgr, subItem ) ;

            rc = _recyBinMgr->processObjects( seqProcessor, cb, w ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to recycle auto-increment "
                         "fields for ""sub-collections, rc: %d", rc ) ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__CATRECYSUBCLPROCESS__POSTSAVEOBJ, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _catRecycleSeqProcessor implement
    */
   _catRecycleSeqProcessor::_catRecycleSeqProcessor( _catRecycleBinManager *recyBinMgr,
                                                     utilRecycleItem &item )
   : _catRecycleProcessor( recyBinMgr, item, UTIL_RECYCLE_SEQ )
   {
   }

   _catRecycleSeqProcessor::~_catRecycleSeqProcessor()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYSEQPROCESS_GETMATCHER, "_catRecycleSeqProcessor::getMatcher" )
   INT32 _catRecycleSeqProcessor::getMatcher( ossPoolList< BSONObj > &matcherList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYSEQPROCESS_GETMATCHER ) ;

      try
      {
         BSONObj matcher ;

         if ( UTIL_RECYCLE_CS == _item.getType() )
         {
            utilCSUniqueID csUniqueID =
                              (utilCSUniqueID)( _item.getOriginID() ) ;
            rc = utilGetCSBounds( FIELD_NAME_CL_UNIQUEID, csUniqueID, matcher ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get bounds with collection "
                         "space unique ID [%u], rc: %d", csUniqueID, rc ) ;
         }
         else if ( UTIL_RECYCLE_CL == _item.getType() )
         {
            utilCLUniqueID clUniqueID =
                              (utilCLUniqueID)( _item.getOriginID() ) ;
            matcher = BSON( FIELD_NAME_CL_UNIQUEID << (INT64)clUniqueID ) ;
         }
         else
         {
            SDB_ASSERT( FALSE, "invalid recycle type" ) ;
            PD_CHECK( FALSE, SDB_SYS, error, PDERROR, "Failed to recycle "
                      "sequence objects, invalid recycle type [%d]",
                      _item.getType() ) ;
         }

         matcherList.push_back( matcher ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build matcher, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CATRECYSEQPROCESS_GETMATCHER, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _catRecycleIdxProcessor implement
    */
   _catRecycleIdxProcessor::_catRecycleIdxProcessor( _catRecycleBinManager *recyBinMgr,
                                                     utilRecycleItem &item )
   : _catRecycleProcessor( recyBinMgr, item, UTIL_RECYCLE_IDX )
   {
   }

   _catRecycleIdxProcessor::~_catRecycleIdxProcessor()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYIDXPROCESS_GETMATCHER, "_catRecycleIdxProcessor::getMatcher" )
   INT32 _catRecycleIdxProcessor::getMatcher( ossPoolList< BSONObj > &matcherList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYIDXPROCESS_GETMATCHER ) ;

      try
      {
         BSONObj matcher ;

         if ( UTIL_RECYCLE_CS == _item.getType() )
         {
            utilCSUniqueID csUniqueID =
                              (utilCSUniqueID)( _item.getOriginID() ) ;
            rc = utilGetCSBounds( FIELD_NAME_CL_UNIQUEID, csUniqueID, matcher ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get bounds with collection "
                         "space unique ID [%u], rc: %d", csUniqueID, rc ) ;
         }
         else if ( UTIL_RECYCLE_CL == _item.getType() )
         {
            utilCLUniqueID clUniqueID =
                              (utilCLUniqueID)( _item.getOriginID() ) ;
            matcher = BSON( FIELD_NAME_CL_UNIQUEID << (INT64)clUniqueID ) ;
         }
         else
         {
            SDB_ASSERT( FALSE, "invalid recycle type" ) ;
            PD_CHECK( FALSE, SDB_SYS, error, PDERROR, "Failed to recycle "
                      "index objects, invalid recycle type [%d]",
                      _item.getType() ) ;
         }

         matcherList.push_back( matcher ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build matcher, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CATRECYIDXPROCESS_GETMATCHER, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
