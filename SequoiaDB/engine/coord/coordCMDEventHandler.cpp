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

   Source File Name = coordCMDEventHandler.cpp

   Descriptive Name = Coord Command Event Handler

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   user command processing on coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/27/2017  XJH Init
   Last Changed =

*******************************************************************************/

#include "coordCMDEventHandler.hpp"
#include "msgMessage.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "pdTrace.hpp"
#include "coordTrace.hpp"
#include "coordCommandWithLocation.hpp"
#include "coordCommandRecycleBin.hpp"
#include "coordCommandData.hpp"

using namespace bson;

namespace engine
{

   /*
      _coordDataCMDHelper implement
    */
   // PD_TRACE_DECLARE_FUNCTION( COORD_DATACMDHELPER_DROPCL, "_coordDataCMDHelper::dropCL" )
   INT32 _coordDataCMDHelper::dropCL( coordResource *resource,
                                      const CHAR *clName,
                                      BOOLEAN skipRecycleBin,
                                      BOOLEAN ignoreLock,
                                      pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_DATACMDHELPER_DROPCL ) ;

      INT64 contextID = -1 ;
      rtnContextBuf contextBuff ;
      CHAR *pMsg = NULL ;
      INT32 buffSize = 0 ;

      coordCMDDropCollection cmdDropCL ;

      rc = cmdDropCL.init( resource, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init drop cl command:rc=%d", rc ) ;

      rc = msgBuildDropCLMsg( &pMsg, &buffSize, clName, skipRecycleBin,
                              ignoreLock, 0, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build drop cl request:"
                   "cl=%s,rc=%d", clName, rc ) ;

      rc = cmdDropCL.execute( (MsgHeader *)pMsg, cb, contextID, &contextBuff ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to drop cl(%s):rc=%d",
                   clName, rc ) ;
      SDB_ASSERT( -1 == contextID, "contextID must be -1" ) ;

   done:
      if ( NULL != pMsg )
      {
         msgReleaseBuffer( pMsg, cb ) ;
      }
      PD_TRACE_EXITRC( COORD_DATACMDHELPER_DROPCL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DATACMDHELPER_TRUNCCL, "_coordDataCMDHelper::truncateCL" )
   INT32 _coordDataCMDHelper::truncateCL( coordResource *resource,
                                          const CHAR *clName,
                                          BOOLEAN skipRecycleBin,
                                          BOOLEAN ignoreLock,
                                          pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_DATACMDHELPER_TRUNCCL ) ;

      INT64 contextID = -1 ;
      rtnContextBuf contextBuff ;
      CHAR *pMsg = NULL ;
      INT32 buffSize = 0 ;

      coordCMDTruncate cmdTruncateCL ;

      rc = cmdTruncateCL.init( resource, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init truncate collection "
                   "command, rc: %d", rc ) ;

      rc = msgBuildTruncateCLMsg( &pMsg, &buffSize, clName, skipRecycleBin,
                                  ignoreLock, 0, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build truncate collection "
                   "request [%s], rc: %d", clName, rc ) ;

      rc = cmdTruncateCL.execute( (MsgHeader *)pMsg, cb, contextID,
                                  &contextBuff ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to truncate collection [%s], rc: %d",
                   clName, rc ) ;
      SDB_ASSERT( -1 == contextID, "contextID must be -1" ) ;

   done:
      if ( NULL != pMsg )
      {
         msgReleaseBuffer( pMsg, cb ) ;
      }
      PD_TRACE_EXITRC( COORD_DATACMDHELPER_TRUNCCL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DATACMDHELPER_ALTERCL, "_coordDataCMDHelper::alterCL" )
   INT32 _coordDataCMDHelper::alterCL( coordResource *resource,
                                       const CHAR *clName,
                                       const BSONObj &options,
                                       pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_DATACMDHELPER_ALTERCL ) ;

      INT64 contextID = -1 ;
      rtnContextBuf contextBuff ;
      CHAR *pMsg = NULL ;
      INT32 buffSize = 0 ;

      coordCMDAlterCollection cmdAlterCL ;

      rc = cmdAlterCL.init( resource, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init alter collection "
                   "command, rc: %d", rc ) ;

      rc = msgBuildAlterCLMsg( &pMsg, &buffSize, clName, options, 0, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build alter collection "
                   "request [%s], rc: %d", clName, rc ) ;

      rc = cmdAlterCL.execute( (MsgHeader *)pMsg, cb, contextID,
                               &contextBuff ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to alter collection [%s], rc: %d",
                   clName, rc ) ;
      SDB_ASSERT( -1 == contextID, "contextID must be -1" ) ;

   done:
      if ( NULL != pMsg )
      {
         msgReleaseBuffer( pMsg, cb ) ;
      }
      PD_TRACE_EXITRC( COORD_DATACMDHELPER_ALTERCL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DATACMDHELPER_DROPCS, "_coordDataCMDHelper::dropCS" )
   INT32 _coordDataCMDHelper::dropCS( coordResource *resource,
                                      const CHAR *csName,
                                      BOOLEAN skipRecycleBin,
                                      BOOLEAN ignoreLock,
                                      pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_DATACMDHELPER_DROPCS ) ;

      INT64 contextID = -1 ;
      rtnContextBuf contextBuff ;
      CHAR *pMsg = NULL ;
      INT32 buffSize = 0 ;

      _coordCMDDropCollectionSpace cmdDropCS ;

      rc = cmdDropCS.init( resource, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init drop cs command:rc=%d", rc ) ;

      rc = msgBuildDropCSMsg( &pMsg, &buffSize, csName, skipRecycleBin,
                              ignoreLock, 0, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build drop cs request:"
                   "cs=%s,rc=%d", csName, rc ) ;

      rc = cmdDropCS.execute( (MsgHeader *)pMsg, cb, contextID, &contextBuff ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to drop cs(%s):rc=%d",
                   csName, rc ) ;
      SDB_ASSERT( -1 == contextID, "contextID must be -1" ) ;

   done:
      if ( NULL != pMsg )
      {
         msgReleaseBuffer( pMsg, cb ) ;
      }
      PD_TRACE_EXITRC( COORD_DATACMDHELPER_DROPCS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   /*
      _coordCMDGlobIdxHandler implement
    */
   // PD_TRACE_DECLARE_FUNCTION( COORD_DATACMDGLOBIDXHANDLER_PARSECATRETURN, "_coordCMDGlobIdxHandler::parseCatReturn" )
   INT32 _coordCMDGlobIdxHandler::parseCatReturn( coordCMDArguments *pArgs,
                                                  const vector<BSONObj> &cataObjs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_DATACMDGLOBIDXHANDLER_PARSECATRETURN ) ;

      BSONObj cataReplyObj ;

      if ( cataObjs.empty() )
      {
         goto done ;
      }

      cataReplyObj = cataObjs[ 0 ] ;

      /* cataObj:
       * { GlobalIndex:[ { Collection: "GIDX_1.100_a", CLUniqueID: 123 },
       *                 { Collection: "GIDX_2.200_a", CLUniqueID: 456 }, ... ]
       */
      try
      {
         BSONObj gIndexObjs ;
         BOOLEAN haveGlobalIndex = FALSE ;

         rc = rtnGetArrayElement( cataReplyObj, CAT_GLOBAL_INDEX, gIndexObjs ) ;
         if ( SDB_OK == rc )
         {
            haveGlobalIndex = TRUE ;
         }
         else if ( SDB_FIELD_NOT_EXIST == rc )
         {
            haveGlobalIndex = FALSE ;
            rc = SDB_OK ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s] from obj [%s], "
                      "rc: %d", CAT_GLOBAL_INDEX,
                      cataReplyObj.toPoolString().c_str(), rc ) ;

         if ( haveGlobalIndex )
         {
            BSONElement element ;
            BSONObj gIndexInfo ;
            BSONObjIterator indexIter( gIndexObjs ) ;
            while ( indexIter.more() )
            {
               const CHAR *clName = NULL ;

               element = indexIter.next() ;
               PD_CHECK( Object == element.type(),
                         SDB_INVALIDARG, error, PDERROR,
                         "Failed to get element from field [%s], "
                         "element [%s] should be object",
                         CAT_GLOBAL_INDEX, element.toPoolString().c_str(),
                         rc ) ;

               gIndexInfo = element.embeddedObject() ;
               rc = rtnGetStringElement( gIndexInfo, CAT_COLLECTION,
                                         &clName ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s] from "
                            "index info [%s], rc: %d", CAT_COLLECTION,
                            gIndexInfo.toPoolString().c_str(), rc ) ;

               _globalIndexes.push_back( clName ) ;
            }
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to get global index name list, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( COORD_DATACMDGLOBIDXHANDLER_PARSECATRETURN, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DATACMDGLOBIDXHANDLER_ONBEGINEVENT, "_coordCMDGlobIdxHandler::onBeginEvent" )
   INT32 _coordCMDGlobIdxHandler::onBeginEvent( coordResource *resource,
                                                coordCMDArguments *arguments,
                                                pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_DATACMDGLOBIDXHANDLER_ONBEGINEVENT ) ;

      _globalIndexes.clear() ;

      PD_TRACE_EXITRC( COORD_DATACMDGLOBIDXHANDLER_ONBEGINEVENT, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DATACMDGLOBIDXHANDLER_ONDATAP1EVENT, "_coordCMDGlobIdxHandler::onDataP1Event" )
   INT32 _coordCMDGlobIdxHandler::onDataP1Event( SDB_EVENT_OCCUR_TYPE type,
                                                 coordResource *pResource,
                                                 coordCMDArguments *pArgs,
                                                 pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_DATACMDGLOBIDXHANDLER_ONDATAP1EVENT ) ;

      if ( SDB_EVT_OCCUR_AFTER == type )
      {
         // now collection on data nodes are locked,
         // alter global indexes to enable repair check
         rc = _repairCheckGlobIdxCLs( pResource, TRUE, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to enable repair check on global "
                      "index collections, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( COORD_DATACMDGLOBIDXHANDLER_ONDATAP1EVENT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DATACMDGLOBIDXHANDLER__REPAIRCHECKGLOBIDXCLS, "_coordCMDGlobIdxHandler::_repairCheckGlobIdxCLs" )
   INT32 _coordCMDGlobIdxHandler::_repairCheckGlobIdxCLs( coordResource *resource,
                                                          BOOLEAN enableRepairCheck,
                                                          pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_DATACMDGLOBIDXHANDLER__REPAIRCHECKGLOBIDXCLS ) ;

      BSONObj options ;
      coordDataCMDHelper helper ;

      try
      {
         BSONObjBuilder builder ;
         builder.appendBool( FIELD_NAME_REPARECHECK, enableRepairCheck ) ;
         options = builder.obj() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build alter options, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

      for ( COORD_GLOBIDXCL_NAME_LIST_CIT iter = _globalIndexes.begin() ;
            iter != _globalIndexes.end() ;
            ++ iter )
      {
         const CHAR *globIdxCLName = iter->c_str() ;

         rc = helper.alterCL( resource, globIdxCLName, options, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to alter collection [%s] with "
                      "[%s] : [%s], rc: %d",
                      globIdxCLName, FIELD_NAME_REPARECHECK,
                      enableRepairCheck ? "TRUE" : "FALSE", rc ) ;

         PD_LOG( PDEVENT, "Alter global index collection [%s] to [%s] "
                 "repair check success", globIdxCLName,
                 enableRepairCheck ? "enable" : "disable" ) ;
      }

   done:
      PD_TRACE_EXITRC( COORD_DATACMDGLOBIDXHANDLER__REPAIRCHECKGLOBIDXCLS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _coordDropGlobIdxHelper implement
    */
   // PD_TRACE_DECLARE_FUNCTION( COORD_DATADROPGLOBIDXHANDLER_ONDATAP2EVENT, "_coordDropGlobIdxHandler::onDataP2Event" )
   INT32 _coordDropGlobIdxHandler::onDataP2Event( SDB_EVENT_OCCUR_TYPE type,
                                                  coordResource *pResource,
                                                  coordCMDArguments *pArgs,
                                                  pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_DATADROPGLOBIDXHANDLER_ONDATAP2EVENT ) ;

      if ( SDB_EVT_OCCUR_BEFORE == type )
      {
         rc = _dropGlobIdxCLs( pResource, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to drop global index collections, "
                      "rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( COORD_DATADROPGLOBIDXHANDLER_ONDATAP2EVENT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DATADROPGLOBIDXHANDLER__DROPGLOBIDXCLS, "_coordDropGlobIdxHandler::_dropGlobIdxCLs" )
   INT32 _coordDropGlobIdxHandler::_dropGlobIdxCLs( coordResource *resource,
                                                    pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_DATADROPGLOBIDXHANDLER__DROPGLOBIDXCLS ) ;

      coordDataCMDHelper helper ;

      for ( COORD_GLOBIDXCL_NAME_LIST_CIT iter = _globalIndexes.begin() ;
            iter != _globalIndexes.end() ;
            ++ iter )
      {
         const CHAR *globIdxCLName = iter->c_str() ;
         // skip recycle bin
         rc = helper.dropCL( resource, globIdxCLName, TRUE, FALSE, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to drop collection [%s], "
                      "rc: %d", globIdxCLName, rc ) ;

         PD_LOG( PDEVENT, "Drop global index collection [%s] success",
                 globIdxCLName ) ;
      }

   done:
      PD_TRACE_EXITRC( COORD_DATADROPGLOBIDXHANDLER__DROPGLOBIDXCLS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _coordTruncGlobIdxHelper implement
    */
   // PD_TRACE_DECLARE_FUNCTION( COORD_DATATRUNCGLOBIDXHANDLER_ONDATAP2EVENT, "_coordTruncGlobIdxHandler::onDataP2Event" )
   INT32 _coordTruncGlobIdxHandler::onDataP2Event( SDB_EVENT_OCCUR_TYPE type,
                                                   coordResource *resource,
                                                   coordCMDArguments *arguments,
                                                   pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_DATATRUNCGLOBIDXHANDLER_ONDATAP2EVENT ) ;

      if ( SDB_EVT_OCCUR_BEFORE == type )
      {
         rc = _truncGlobIdxCLs( resource, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to drop global index collections, "
                      "rc: %d", rc ) ;
      }
      else if ( SDB_EVT_OCCUR_AFTER == type )
      {
         // collection is truncated, disable repair check
         rc = _repairCheckGlobIdxCLs( resource, FALSE, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to disable repair check on global "
                      "index collections, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( COORD_DATATRUNCGLOBIDXHANDLER_ONDATAP2EVENT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DATATRUNCGLOBIDXHANDLER__TRUNCGLOBIDXCLS, "_coordTruncGlobIdxHandler::_truncGlobIdxCLs" )
   INT32 _coordTruncGlobIdxHandler::_truncGlobIdxCLs( coordResource *resource,
                                                      pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_DATATRUNCGLOBIDXHANDLER__TRUNCGLOBIDXCLS ) ;

      coordDataCMDHelper helper ;

      for ( COORD_GLOBIDXCL_NAME_LIST_CIT iter = _globalIndexes.begin() ;
            iter != _globalIndexes.end() ;
            ++ iter )
      {
         const CHAR *globIdxCLName = iter->c_str() ;
         // skip recycle bin
         rc = helper.truncateCL( resource, globIdxCLName, TRUE, FALSE, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to truncate collection [%s], "
                      "rc: %d", globIdxCLName, rc ) ;

         PD_LOG( PDEVENT, "Truncate global index collection [%s] success",
                 globIdxCLName ) ;
      }

   done:
      PD_TRACE_EXITRC( COORD_DATATRUNCGLOBIDXHANDLER__TRUNCGLOBIDXCLS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _coordCMDRecycleHandler implement
    */
   // PD_TRACE_DECLARE_FUNCTION( COORD_DATARECYHANDLER_PARSECATRETURN, "_coordCMDRecycleHandler::parseCatReturn" )
   INT32 _coordCMDRecycleHandler::parseCatReturn( coordCMDArguments *pArgs,
                                                  const vector<BSONObj> &cataObjs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_DATARECYHANDLER_PARSECATRETURN ) ;

      BSONObj cataReplyObj ;

      if ( cataObjs.empty() )
      {
         goto done ;
      }

      cataReplyObj = cataObjs[ 0 ] ;

      try
      {
         BSONElement element ;

         // get recycle item
         element = cataReplyObj.getField( FIELD_NAME_RECYCLE_ITEM ) ;
         if ( EOO != element.type() )
         {
            PD_CHECK( Object == element.type(), SDB_SYS, error, PDERROR,
                      "Failed to get field [%s], it is not object",
                      FIELD_NAME_RECYCLE_ITEM ) ;

            _recycleOptions = element.embeddedObject().copy() ;

            try
            {
               // as rename, retry when lock failed in data node
               pArgs->_retryRCList.insert( SDB_LOCK_FAILED ) ;
            }
            catch ( exception  &e )
            {
               PD_LOG( PDERROR, "Failed to save retry return code, "
                       "occur exception %s", e.what() ) ;
               rc = ossException2RC( &e ) ;
               goto error ;
            }

            PD_LOG( PDDEBUG, "Got recycle options [%s]",
                    _recycleOptions.toPoolString().c_str() ) ;
         }
         else
         {
            goto done ;
         }

         // get dropping items
         element = cataReplyObj.getField( FIELD_NAME_DROP_RECYCLE_ITEM ) ;
         if ( Array == element.type() )
         {
            BSONObjIterator iter( element.embeddedObject() ) ;
            while ( iter.more() )
            {
               BSONElement subEle = iter.next() ;
               PD_CHECK( String == subEle.type(), SDB_SYS, error, PDERROR,
                         "Failed to get dropping recycle items, "
                         "sub-element should be string" ) ;
               _droppingItems.push_back( subEle.valuestrsafe() ) ;
            }
         }
         else if ( EOO != element.type() )
         {
            PD_CHECK( FALSE, SDB_SYS, error, PDERROR,
                      "Failed to get dropping recycle items, "
                      "field [%s] should be array",
                      FIELD_NAME_DROP_RECYCLE_ITEM ) ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse catalog return objects, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( COORD_DATARECYHANDLER_PARSECATRETURN, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DATARECYHANDLER_REWRITEDATAMSG, "_coordCMDRecycleHandler::rewriteDataMsg" )
   INT32 _coordCMDRecycleHandler::rewriteDataMsg( BSONObjBuilder &queryBuilder,
                                                  BSONObjBuilder &hintBuilder )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_DATARECYHANDLER_REWRITEDATAMSG ) ;

      try
      {
         hintBuilder.append( FIELD_NAME_RECYCLE_ITEM, _recycleOptions ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to rewrite data message, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( COORD_DATARECYHANDLER_REWRITEDATAMSG, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DATARECYHANDLER_ONBEGINEVENT, "_coordCMDRecycleHandler::onBeginEvent" )
   INT32 _coordCMDRecycleHandler::onBeginEvent( coordResource *resource,
                                                coordCMDArguments *arguments,
                                                pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_DATARECYHANDLER_ONBEGINEVENT ) ;

      _recycleOptions = BSONObj() ;
      _droppingItems.clear() ;

      PD_TRACE_EXITRC( COORD_DATARECYHANDLER_ONBEGINEVENT, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DATARECYHANDLER_ONDATAP1EVENT, "_coordCMDRecycleHandler::onDataP1Event" )
   INT32 _coordCMDRecycleHandler::onDataP1Event( SDB_EVENT_OCCUR_TYPE type,
                                                 coordResource *resource,
                                                 coordCMDArguments *arguments,
                                                 pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_DATARECYHANDLER_ONDATAP1EVENT ) ;

      if ( SDB_EVT_OCCUR_AFTER == type )
      {
         // drop old recycle items first to give room for current recycle item
         // if needed
         rc = _dropRecycleItems( resource, TRUE, TRUE, TRUE, TRUE, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to drop dropping recycle items "
                      "from in recycle bin, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( COORD_DATARECYHANDLER_ONDATAP1EVENT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DATARECYHANDLER__DROPRECYITEM, "_coordCMDRecycleHandler::_dropRecycleItem" )
   INT32 _coordCMDRecycleHandler::_dropRecycleItem( coordResource *resource,
                                                    const CHAR *recycleName,
                                                    BOOLEAN ignoreIfNotExists,
                                                    BOOLEAN isRecursive,
                                                    BOOLEAN isEnforced,
                                                    BOOLEAN ignoreLock,
                                                    pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_DATARECYHANDLER__DROPRECYITEM ) ;

      INT64 contextID = -1 ;
      rtnContextBuf contextBuff ;
      CHAR *pMsg = NULL ;
      INT32 buffSize = 0 ;

      coordDropRecycleBinItem command ;

      rc = command.init( resource, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init drop recycle bin item command, "
                   "rc: %d", rc ) ;

      rc = msgBuildDropRecyBinItemMsg( &pMsg, &buffSize, recycleName,
                                       isRecursive, isEnforced, ignoreLock,
                                       0, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build drop recycle bin item "
                   "request [%s], rc: %d", recycleName, rc ) ;

      rc = command.execute( (MsgHeader *)pMsg, cb, contextID, &contextBuff ) ;
      if ( ignoreIfNotExists && SDB_RECYCLE_ITEMNOTEXISTS == rc )
      {
         rc = SDB_OK ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to drop recycle bin item [%s], rc: %d",
                   recycleName, rc ) ;
      SDB_ASSERT( -1 == contextID, "contextID must be -1" ) ;

   done:
      if ( NULL != pMsg )
      {
         msgReleaseBuffer( pMsg, cb ) ;
      }
      PD_TRACE_EXITRC( COORD_DATARECYHANDLER__DROPRECYITEM, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DATARECYHANDLER__DROPRECYITEMS, "_coordCMDRecycleHandler::_dropRecycleItems" )
   INT32 _coordCMDRecycleHandler::_dropRecycleItems( coordResource *resource,
                                                     BOOLEAN ignoreIfNotExists,
                                                     BOOLEAN isRecursive,
                                                     BOOLEAN isEnforced,
                                                     BOOLEAN ignoreLock,
                                                     pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_DATARECYHANDLER__DROPRECYITEMS ) ;

      for ( UTIL_RECY_ITEM_NAME_LIST_CIT iter = _droppingItems.begin() ;
            iter != _droppingItems.end() ;
            ++ iter )
      {
         const CHAR *recycleName = iter->c_str() ;

         rc = _dropRecycleItem( resource, recycleName, ignoreIfNotExists,
                                isRecursive, isEnforced, ignoreLock, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to drop recycle bin item [%s], "
                      "rc: %d", recycleName, rc ) ;
      }

   done:
      PD_TRACE_EXITRC( COORD_DATARECYHANDLER__DROPRECYITEMS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
