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

   Source File Name = catCatalogManager.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/
#include "core.hpp"
#include "pmdCB.hpp"
#include "pd.hpp"
#include "rtn.hpp"
#include "catDef.hpp"
#include "catCatalogManager.hpp"
#include "rtnPredicate.hpp"
#include "msgMessage.hpp"
#include "ixmIndexKey.hpp"
#include "pdTrace.hpp"
#include "catTrace.hpp"
#include "catCommon.hpp"
#include "clsCatalogAgent.hpp"
#include "rtnAlterJob.hpp"

using namespace bson;

namespace engine
{

   /*
      catCatalogueManager implement
   */
   catCatalogueManager::catCatalogueManager()
   {
      _pEduCB     = NULL ;
      _pDpsCB     = NULL ;
      _pCatCB     = NULL ;
      _pDmsCB     = NULL ;
   }

   INT32 catCatalogueManager::active()
   {
      _taskMgr.setTaskID( catGetMaxTaskID( _pEduCB ) ) ;
      return SDB_OK ;
   }

   INT32 catCatalogueManager::deactive()
   {
      return SDB_OK ;
   }

   INT32 catCatalogueManager::init()
   {
      pmdKRCB *krcb  = pmdGetKRCB();
      _pDmsCB        = krcb->getDMSCB();
      _pDpsCB        = krcb->getDPSCB();
      _pCatCB        = krcb->getCATLOGUECB();
      return SDB_OK ;
   }

   void catCatalogueManager::attachCB( pmdEDUCB * cb )
   {
      _pEduCB = cb ;
   }

   void catCatalogueManager::detachCB( pmdEDUCB * cb )
   {
      _pEduCB = NULL ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGMGR_DROPCS, "catCatalogueManager::processCmdDropCollectionSpace" )
   INT32 catCatalogueManager::processCmdDropCollectionSpace( const CHAR *pQuery )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CATALOGMGR_DROPCS ) ;

      try
      {
         BSONObj boQuery( pQuery ) ;
         BSONElement beSpaceName =
            boQuery.getField ( CAT_COLLECTION_SPACE_NAME ) ;
         PD_CHECK ( beSpaceName.type() == String, SDB_INVALIDARG, error,
                    PDERROR, "Field[%s] type[%d] is not String",
                    CAT_COLLECTION_SPACE_NAME, beSpaceName.type() ) ;
         rc = catRemoveCSEx( beSpaceName.valuestr(), _pEduCB, _pDmsCB, _pDpsCB,
                             _majoritySize() ) ;
         PD_RC_CHECK ( rc, PDERROR, "Failed to drop collection space %s, "
                       "rc = %d", beSpaceName.valuestr(), rc ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATALOGMGR_DROPCS, rc ) ;
      return rc;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGMGR_CRT_PROCEDURES, "catCatalogueManager::processCmdCrtProcedures")
   INT32 catCatalogueManager::processCmdCrtProcedures( void *pMsg )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATALOGMGR_CRT_PROCEDURES ) ;
      try
      {
         BSONObj func( (const CHAR *)pMsg ) ;
         BSONObj parsed ;
         rc = catPraseFunc( func, parsed ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to parse store procedures:%s",
                    func.toString().c_str() ) ;
            goto error ;
         }

         rc = rtnInsert( CAT_PROCEDURES_COLLECTION,
                         parsed, 1, 0,
                         _pEduCB, _pDmsCB, _pDpsCB, _majoritySize() ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to add func:%s",
                    parsed.toString().c_str() ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected err happened:%s",e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB_CATALOGMGR_CRT_PROCEDURES, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGMGR_RM_PROCEDURES, "catCatalogueManager::processCmdRmProcedures")
   INT32 catCatalogueManager::processCmdRmProcedures( void *pMsg )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATALOGMGR_RM_PROCEDURES ) ;
      try
      {
         BSONObj obj( (const CHAR *)pMsg ) ;
         BSONElement name = obj.getField( FIELD_NAME_FUNC ) ;
         if ( name.eoo() || String != name.type() )
         {
            PD_LOG( PDERROR, "invalid type of func name[%s:%d]",
                    name.toString().c_str(), name.type()  ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         {
         BSONObjBuilder builder ;
         BSONObj deletor ;
         BSONObj dummy ;
         BSONObj func ;
         builder.appendAs( name, FMP_FUNC_NAME ) ;
         deletor = builder.obj() ;

         rc = catGetOneObj( CAT_PROCEDURES_COLLECTION,
                            dummy, deletor, dummy,
                            _pEduCB, func ) ;
         if ( SDB_DMS_EOC == rc )
         {
            PD_LOG( PDERROR, "func %s is not exist",
                    deletor.toString().c_str() ) ;
            rc = SDB_FMP_FUNC_NOT_EXIST ;
            goto error ;
         }
         else if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to get func:%s, rc=%d",
                    deletor.toString().c_str(), rc ) ;
            goto error ;
         }

         rc = rtnDelete( CAT_PROCEDURES_COLLECTION,
                         deletor, BSONObj(),
                         0, _pEduCB, _pDmsCB, _pDpsCB, _majoritySize() ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to rm func:%s",
                    deletor.toString().c_str() ) ;
            goto error ;
         }
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected err happened:%s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB_CATALOGMGR_RM_PROCEDURES, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGMGR_QUERYSPACEINFO, "catCatalogueManager::processCmdQuerySpaceInfo" )
   INT32 catCatalogueManager::processCmdQuerySpaceInfo( const CHAR * pQuery,
                                                        rtnContextBuf &ctxBuf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CATALOGMGR_QUERYSPACEINFO ) ;
      const CHAR *csName = NULL ;
      BSONObj boSpace ;
      BOOLEAN isExist = FALSE ;
      vector< UINT32 > groups ;
      BSONObjBuilder builder ;

      try
      {
         BSONObj boQuery( pQuery ) ;
         rtnGetStringElement( boQuery,  CAT_COLLECTION_SPACE_NAME, &csName ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get field[%s], rc: %d",
                      CAT_COLLECTION_SPACE_NAME, rc ) ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Occur exception: %s", e.what() ) ;
         goto error ;
      }

      PD_TRACE1 ( SDB_CATALOGMGR_QUERYSPACEINFO, PD_PACK_STRING ( csName ) ) ;

      rc = catCheckSpaceExist( csName, isExist, boSpace, _pEduCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Check collection space[%s] exist failed, "
                   "rc: %d", csName, rc ) ;
      PD_TRACE1 ( SDB_CATALOGMGR_QUERYSPACEINFO,PD_PACK_INT ( isExist ) ) ;

      if ( !isExist )
      {
         rc = SDB_DMS_CS_NOTEXIST ;
         goto error ;
      }

      // get collection space all groups
      rc = catGetCSGroupsFromCLs( csName, _pEduCB, groups, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Get collection space[%s] all groups failed, "
                   "rc: %d", csName, rc ) ;

      builder.appendElements( boSpace ) ;
      // add group info
      _pCatCB->makeGroupsObj( builder, groups, TRUE ) ;

      ctxBuf = rtnContextBuf( builder.obj() ) ;

   done:
      PD_TRACE_EXITRC ( SDB_CATALOGMGR_QUERYSPACEINFO, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // this function is for catalog collection check
   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGMGR_QUERYCATALOG, "catCatalogueManager::processQueryCatalogue" )
   INT32 catCatalogueManager::processQueryCatalogue ( const NET_HANDLE &handle,
                                                      MsgHeader *pMsg )
   {
      INT32 rc                         = SDB_OK;
      PD_TRACE_ENTRY ( SDB_CATALOGMGR_QUERYCATALOG ) ;
      MsgCatQueryCatReq *pCatReq       = (MsgCatQueryCatReq*)pMsg ;
      MsgOpReply *pReply               = NULL;
      BOOLEAN isDelay                  = FALSE ;

      // primary check
      rc = _pCatCB->primaryCheck( _pEduCB, TRUE, isDelay ) ;
      if ( isDelay )
      {
         goto done ;
      }
      else if ( rc )
      {
         PD_LOG ( PDWARNING, "service deactive but received query "
                  "catalogue request, rc: %d", rc ) ;
         goto error ;
      }

      // sanity check, header can't be too small
      PD_CHECK ( pCatReq->header.messageLength >=
                 (INT32)sizeof(MsgCatQueryCatReq),
                 SDB_INVALIDARG, error, PDERROR,
                 "recived unexpected query catalogue request, "
                 "message length(%d) is invalied",
                 pCatReq->header.messageLength ) ;
      // extract and query
      try
      {
         CHAR *pCollectionName = NULL ;
         SINT32 flag           = 0 ;
         SINT64 numToSkip      = 0 ;
         SINT64 numToReturn    = -1 ;
         CHAR *pQuery          = NULL ;
         CHAR *pFieldSelector  = NULL ;
         CHAR *pOrderBy        = NULL ;
         CHAR *pHint           = NULL ;
         rc = msgExtractQuery  ( (CHAR *)pCatReq, &flag, &pCollectionName,
                                 &numToSkip, &numToReturn, &pQuery,
                                 &pFieldSelector, &pOrderBy, &pHint ) ;
         BSONObj matcher(pQuery);
         BSONObj selector(pFieldSelector);
         BSONObj orderBy(pOrderBy);
         BSONObj hint(pHint);
         PD_RC_CHECK ( rc, PDERROR,
                       "Failed to extract message, rc = %d", rc ) ;
         // perform catalog query, result buffer will be placed in pReply, and
         // we are responsible to free it by end of the function
         rc = catQueryAndGetMore ( &pReply, CAT_COLLECTION_INFO_COLLECTION,
                                   selector, matcher, orderBy, hint, flag,
                                   _pEduCB, numToSkip, numToReturn ) ;
         PD_RC_CHECK ( rc, PDERROR,
                       "Failed to query from catalog, rc = %d", rc ) ;
         // check for how many records were returned
         // need to make sure returned record must be one
         // 1) if returned = 0, it means collection does not exist
         // 2) if returned > 1, it means possible catalog corruption
         PD_CHECK ( pReply->numReturned >= 1, SDB_DMS_NOTEXIST, error,
                    PDWARNING, "Collection does not exist:%s",
                    matcher.toString().c_str() ) ;
         PD_CHECK ( pReply->numReturned <= 1, SDB_CAT_CORRUPTION, error,
                    PDSEVERE,
                    "More than one records returned for query, "
                    "possible catalog corruption" ) ;
      }
      catch ( std::exception &e )
      {
         PD_RC_CHECK ( SDB_SYS, PDERROR,
                       "Exception during query catalogue request:%s",
                       e.what() ) ;
      }
      pReply->header.opCode        = MSG_CAT_QUERY_CATALOG_RSP ;
      pReply->header.TID           = pCatReq->header.TID ;
      pReply->header.requestID     = pCatReq->header.requestID ;
      pReply->header.routeID.value = 0 ;
   done :
      if ( !_pCatCB->isDelayed() )
      {
         if ( SDB_OK == rc && NULL != pReply )
         {
            rc = _pCatCB->netWork()->syncSend ( handle, pReply );
         }
         else
         {
            MsgOpReply replyMsg;
            replyMsg.header.messageLength = sizeof( MsgOpReply );
            replyMsg.header.opCode        = MSG_CAT_QUERY_CATALOG_RSP;
            replyMsg.header.TID           = pCatReq->header.TID;
            replyMsg.header.routeID.value = 0;
            replyMsg.header.requestID     = pCatReq->header.requestID;
            replyMsg.numReturned          = 0;
            replyMsg.flags                = rc;
            replyMsg.contextID            = -1 ;
            PD_TRACE1 ( SDB_CATALOGMGR_QUERYCATALOG,
                        PD_PACK_INT ( rc ) ) ;
            if ( SDB_CLS_NOT_PRIMARY == rc )
            {
               replyMsg.startFrom = _pCatCB->getPrimaryNode() ;
            }
            rc = _pCatCB->netWork()->syncSend ( handle, &replyMsg );
         }
      }
      if( pReply )
      {
         SDB_OSS_FREE ( pReply );
      }
      PD_TRACE_EXITRC ( SDB_CATALOGMGR_QUERYCATALOG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGMGR_DROPCOLLECTION, "catCatalogueManager::processCmdDropCollection" )
   INT32 catCatalogueManager::processCmdDropCollection( const CHAR *pQuery,
                                                        INT32 version )
   {
      INT32 rc                         = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CATALOGMGR_DROPCOLLECTION ) ;

      const CHAR *strName              = NULL ;

      try
      {
         BSONObj boDeletor = BSONObj ( pQuery ) ;
         BSONElement beName = boDeletor.getField( CAT_COLLECTION_NAME ) ;
         PD_CHECK ( String == beName.type(), SDB_INVALIDARG, error,
                    PDERROR, "Failed to drop the collection, get collection "
                    "name failed, type: %d", beName.type() ) ;
         strName = beName.valuestr() ;
         PD_TRACE1 ( SDB_CATALOGMGR_DROPCOLLECTION,
                     PD_PACK_STRING ( strName ) ) ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = catRemoveCLEx( strName, _pEduCB, _pDmsCB, _pDpsCB,
                          _majoritySize(), TRUE, version ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to drop collection %s from catalog, "
                    "rc = %d", strName, rc ) ;

   done :
      PD_TRACE_EXITRC ( SDB_CATALOGMGR_DROPCOLLECTION, rc ) ;
      return rc;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGMGR_QUERYTASK, "catCatalogueManager::processQueryTask" )
   INT32 catCatalogueManager::processQueryTask ( const NET_HANDLE &handle,
                                                 MsgHeader *pMsg )
   {
      INT32 rc                         = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CATALOGMGR_QUERYTASK ) ;
      MsgCatQueryTaskReq *pTaskRequest = (MsgCatQueryTaskReq*)pMsg ;
      MsgCatQueryTaskRes *pReply       = NULL ;
      INT32 flag                       = 0 ;
      SINT64 numToSkip                 = 0 ;
      SINT64 numToReturn               = 0 ;
      CHAR *pQuery                     = NULL ;
      CHAR *pFieldSelector             = NULL ;
      CHAR *pOrderBy                   = NULL ;
      CHAR *pHint                      = NULL ;
      CHAR *pCollectionName            = NULL ;
      BOOLEAN isDelay                  = FALSE ;

      // primary check
      rc = _pCatCB->primaryCheck( _pEduCB, TRUE, isDelay ) ;
      if ( isDelay )
      {
         goto done ;
      }
      else if ( rc )
      {
         PD_LOG ( PDWARNING, "service deactive but received query "
                  "catalogue request, rc: %d", rc ) ;
         goto error ;
      }

      // sanity check, the query length should be at least header size
      PD_CHECK ( pTaskRequest->header.messageLength >=
                 (INT32)sizeof(MsgCatQueryTaskReq),
                 SDB_INVALIDARG, error, PDERROR,
                 "received unexpected query task request, "
                 "message length(%d) is invalid",
                 pTaskRequest->header.messageLength ) ;

      try
      {
         // extract the request message
         rc = msgExtractQuery ( (CHAR*)pTaskRequest, &flag, &pCollectionName,
                                &numToSkip, &numToReturn, &pQuery,
                                &pFieldSelector, &pOrderBy, &pHint ) ;
         BSONObj matcher ( pQuery ) ;
         BSONObj selector ( pFieldSelector ) ;
         BSONObj orderBy ( pOrderBy );
         BSONObj hint ( pHint ) ;
         PD_RC_CHECK ( rc, PDERROR,
                       "Failed to extract message, rc = %d", rc ) ;
         // pReply will be allocated by catQueryAndGetMore, we are
         // responsible to free the memory
         rc = catQueryAndGetMore ( &pReply, CAT_TASK_INFO_COLLECTION,
                                   selector, matcher, orderBy, hint, flag,
                                   _pEduCB, numToSkip, numToReturn ) ;
         PD_RC_CHECK ( rc, PDERROR,
                       "Failed to perform query from catalog, rc = %d", rc ) ;

         // If there's no task satisfy the request, let's return SDB_CAT_TASK_NOTFOUND,
         // otherwise return all tasks satisfy the request
         PD_CHECK ( pReply->numReturned >= 1, SDB_CAT_TASK_NOTFOUND, error,
                    PDINFO, "Task does not exist" ) ;
      }
      catch ( std::exception &e )
      {
         PD_RC_CHECK ( SDB_SYS, PDERROR,
                       "Exception when extracting query task: %s",
                       e.what() ) ;
      }
      // construct reply header to match the request
      pReply->header.opCode        = MSG_CAT_QUERY_TASK_RSP ;
      pReply->header.TID           = pTaskRequest->header.TID ;
      pReply->header.requestID     = pTaskRequest->header.requestID ;
      pReply->header.routeID.value = 0 ;

   done :
      if ( !_pCatCB->isDelayed() )
      {
         if ( SDB_OK == rc && pReply )
         {
            rc = _pCatCB->netWork()->syncSend ( handle, pReply );
         }
         else
         {
            // if something wrong happened, return a reply with rc
            MsgOpReply replyMsg;
            replyMsg.header.messageLength = sizeof( MsgOpReply );
            replyMsg.header.opCode        = MSG_CAT_QUERY_TASK_RSP ;
            replyMsg.header.TID           = pTaskRequest->header.TID;
            replyMsg.header.routeID.value = 0;
            replyMsg.header.requestID     = pTaskRequest->header.requestID;
            replyMsg.numReturned          = 0;
            replyMsg.flags                = rc;
            replyMsg.contextID            = -1 ;
            PD_TRACE1 ( SDB_CATALOGMGR_QUERYTASK, PD_PACK_INT ( rc ) ) ;

            if ( SDB_CLS_NOT_PRIMARY == rc )
            {
               replyMsg.startFrom = _pCatCB->getPrimaryNode() ;
            }
            rc = _pCatCB->netWork()->syncSend ( handle, &replyMsg );
         }
      }
      if ( pReply )
      {
         SDB_OSS_FREE ( pReply ) ;
      }
      PD_TRACE_EXITRC ( SDB_CATALOGMGR_QUERYTASK, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGMGR_ALTERCOLLECTION, "catCatalogueManager::processAlterCollection" )
   INT32 catCatalogueManager::processAlterCollection( const CHAR *pMsg,
                                                      rtnContextBuf &ctxBuf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATALOGMGR_ALTERCOLLECTION ) ;
      BSONObj obj ;
      BOOLEAN isOld = TRUE ;
      try
      {
         obj = BSONObj( pMsg ) ;
         isOld = obj.getField( FIELD_NAME_VERSION ).eoo() ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected error happened:%s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = isOld ?
           _processAlterCollectionOld( obj, ctxBuf ) :
           _processAlterCollection( obj, ctxBuf ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to alter collection:%d", rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB_CATALOGMGR_ALTERCOLLECTION, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGMGR__ALTERCOLLECTION, "catCatalogueManager::_processAlterCollection" )
   INT32 catCatalogueManager::_processAlterCollection( const BSONObj &obj,
                                                       rtnContextBuf &ctxBuf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATALOGMGR__ALTERCOLLECTION ) ;
      _rtnAlterJob job ;
      BSONObj clObj ;
      BOOLEAN exists = FALSE ;
      rc = job.init( obj ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to init alter job:%d", rc ) ;
         goto error ;
      }

      rc = catCheckCollectionExist( job.getName(), exists,
                                    clObj, _pEduCB ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to check collection:%d", rc ) ;
         goto error ;
      }

      if ( !exists )
      {
         PD_LOG( PDERROR, "collection[%s] does not exist",
                 job.getName() ) ;
         rc = SDB_DMS_NOTEXIST ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATALOGMGR__ALTERCOLLECTION, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGMGR__ALTERCOLLECTIONOLD, "catCatalogueManager::_processAlterCollectionOld" )
   INT32 catCatalogueManager::_processAlterCollectionOld ( const BSONObj &obj,
                                                           rtnContextBuf &ctxBuf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CATALOGMGR__ALTERCOLLECTIONOLD ) ;
      // collection name related
      const CHAR *strName              = NULL ;
      BSONObj options ;
      BOOLEAN isCollectionExist        = FALSE ;
      BSONObj boCollectionRecord ;
      BSONObj matcher ;
      BSONObj updater ;
      BSONObj hint ;
      BSONObj optionsObj ;
      UINT32 mask = 0 ;
      catCollectionInfo clInfo ;
      _clsCatalogSet catSet( "" ) ;

      try
      {
         BSONObj boAlterObj ( obj ) ;
         // make sure collection name exists
         BSONElement beName = boAlterObj.getField ( CAT_COLLECTION_NAME ) ;
         BSONElement beOptions = boAlterObj.getField ( CAT_OPTIONS_NAME ) ;
         PD_CHECK ( String == beName.type(), SDB_INVALIDARG, error, PDERROR,
                    "Invalid field %s", CAT_COLLECTION_NAME ) ;
         strName = beName.valuestr() ;
         PD_TRACE1 ( SDB_CATALOGMGR_ALTERCOLLECTION,
                     PD_PACK_STRING ( strName ) ) ;
         // make sure options exists
         PD_CHECK ( Object == beOptions.type(), SDB_INVALIDARG, error, PDERROR,
                    "Invalid field %s", CAT_OPTIONS_NAME ) ;
         // make sure each elements are valid

         // make sure collection exists
         rc = catCheckCollectionExist ( strName, isCollectionExist,
                                        boCollectionRecord, _pEduCB ) ;
         PD_RC_CHECK ( rc, PDERROR,
                       "Failed to detect collection existence, rc = %d", rc ) ;

         if ( !isCollectionExist )
         {
            PD_LOG( PDERROR, "collection[%s] does not exist", strName ) ;
            rc = SDB_DMS_NOTEXIST ;
            goto error ;
         }

         optionsObj = beOptions.embeddedObject() ;
         if ( optionsObj.isEmpty() )
         {
            PD_LOG( PDERROR, "empty options object in alter request" ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         rc = _checkAndBuildCataRecord( optionsObj,
                                        mask, clInfo, FALSE ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "invalid alter request:%s",
                    boAlterObj.toString().c_str() ) ;
            goto error ;
         }

         rc = catSet.updateCatSet( boCollectionRecord ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to save json[%s] to catalogset:%d",
                    boCollectionRecord.toString(FALSE, TRUE).c_str(), rc ) ;
            goto error ;
         }
         rc = _buildAlterObjWithMetaAndObj( catSet, mask, clInfo, updater ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to build alter object, alter req[%s], "
                    "rc:%d", boAlterObj.toString(FALSE, TRUE).c_str(), rc ) ;
            goto error ;
         }

         // build search condition
         matcher = BSON ( CAT_COLLECTION_NAME << strName ) ;
         // perform update
         rc = rtnUpdate ( CAT_COLLECTION_INFO_COLLECTION,
                          matcher, BSON( "$set" << updater ), hint,
                          0, _pEduCB, _pDmsCB, _pDpsCB,
                          _majoritySize() );
         PD_RC_CHECK ( rc, PDERROR,
                       "Failed to alter collection, rc = %d", rc ) ;

         /// build reply body: group info.
         if ( !catSet.isMainCL() )
         {
            BSONArrayBuilder arrBuilder ;
            BSONElement gpInfo = boCollectionRecord.getField( CAT_CATALOGINFO_NAME ) ;
            if ( Array == gpInfo.type() ||
                 Object == gpInfo.type() )
            {
               BSONObjIterator itr( gpInfo.embeddedObject() ) ;
               while ( itr.more() )
               {
                  arrBuilder << itr.next() ;
               }
               BSONObj replyObj = BSON( CAT_GROUP_NAME << arrBuilder.arr() ) ;
               ctxBuf = rtnContextBuf( replyObj ) ;
            }
         }
         /// get all sub collections' groups
         else
         {
            BSONObj replyObj ;
            vector<string> subCLLst ;
            rc = catSet.getSubCLList( subCLLst ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to get sub cl list:%d", rc ) ;
               goto error ;
            }

            if ( !subCLLst.empty() )
            {
               rc = _getGroupsOfCollections( subCLLst, replyObj ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "failed to get groups of sub cl:%d", rc ) ;
                  goto error ;
               }
               ctxBuf = rtnContextBuf( replyObj.getOwned() ) ;
            }
         }
      }
      catch ( std::exception &e )
      {
         PD_RC_CHECK ( SDB_SYS, PDERROR,
                       "Exception hit when parsing alter collection: %s",
                       e.what() ) ;
      }
   done :
      PD_TRACE1 ( SDB_CATALOGMGR_ALTERCOLLECTION, PD_PACK_INT ( rc ) ) ;
      PD_TRACE_EXITRC ( SDB_CATALOGMGR__ALTERCOLLECTIONOLD, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGMGR_CREATECS, "catCatalogueManager::processCmdCreateCS" )
   INT32 catCatalogueManager::processCmdCreateCS( const CHAR * pQuery,
                                                  rtnContextBuf &ctxBuf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CATALOGMGR_CREATECS ) ;
      UINT32 groupID = CAT_INVALID_GROUPID ;

      try
      {
         BSONObj groupObj ;
         BSONObj query( pQuery ) ;
         rc = _createCS( query, groupID ) ;
         PD_RC_CHECK( rc, PDERROR, "Create collection space failed, rc: %d",
                      rc ) ;
      }
      catch( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Occurred exception: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_CATALOGMGR_CREATECS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGMGR_CREATECL, "catCatalogueManager::processCmdCreateCL" )
   INT32 catCatalogueManager::processCmdCreateCL( const CHAR *pQuery,
                                                  rtnContextBuf &ctxBuf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CATALOGMGR_CREATECL ) ;

      try
      {
         UINT32 groupID = CAT_INVALID_GROUPID ;
         vector< UINT32 > groups ;
         std::vector<UINT64> taskIDs ;
         BSONObjBuilder replyBuild ;

         BSONObj query( pQuery ) ;
         rc = _createCL( query, groupID, taskIDs ) ;
         PD_RC_CHECK( rc, PDERROR, "Create collection failed, rc: %d", rc ) ;

         groups.push_back( groupID ) ;

         // make reply
         replyBuild.append( CAT_CATALOGVERSION_NAME, CAT_VERSION_BEGIN ) ;
         _pCatCB->makeGroupsObj( replyBuild, groups, TRUE ) ;

         if ( !taskIDs.empty() )
         {
            BSONArrayBuilder task( replyBuild.subarrayStart( CAT_TASKID_NAME ) ) ;
            for ( UINT32 i = 0; i < taskIDs.size(); i++ )
            {
               task.append( (INT64)taskIDs.at( i ) ) ;
            }
            task.done() ;
         }
         ctxBuf = rtnContextBuf( replyBuild.obj() ) ;
      }
      catch( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Occurred exception: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_CATALOGMGR_CREATECL, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGMGR_CMDSPLIT, "catCatalogueManager::processCmdSplit" )
   INT32 catCatalogueManager::processCmdSplit( const CHAR * pQuery,
                                               INT32 opCode,
                                               rtnContextBuf &ctxBuf )
   {
      INT32 rc = SDB_OK ;
      const CHAR *clFullName = NULL ;
      clsCatalogSet *pCataSet = NULL ;
      UINT32 groupID = CAT_INVALID_GROUPID ;
      UINT64 taskID = CLS_INVALID_TASKID ;

      PD_TRACE_ENTRY ( SDB_CATALOGMGR_CMDSPLIT ) ;
      try
      {
         BSONObj cataObj ;
         BOOLEAN clExist = FALSE ;
         BSONObj query( pQuery ) ;

         if ( MSG_CAT_SPLIT_PREPARE_REQ == opCode ||
              MSG_CAT_SPLIT_READY_REQ == opCode ||
              MSG_CAT_SPLIT_CHGMETA_REQ == opCode )
         {
            rc = rtnGetStringElement( query, CAT_COLLECTION_NAME,
                                      &clFullName ) ;
            PD_RC_CHECK( rc, PDERROR, "Get split collection name failed, "
                         "rc: %d, info: %s", rc, query.toString().c_str() ) ;

            // get catalog info
            rc = catCheckCollectionExist( clFullName, clExist, cataObj,
                                          _pEduCB ) ;
            PD_RC_CHECK( rc, PDERROR, "Check collection exist failed, rc: %d",
                         rc ) ;
            PD_CHECK( clExist, SDB_DMS_NOTEXIST, error, PDWARNING,
                      "Collection[%s] is no longer existed", clFullName ) ;

            // update catalog set
            pCataSet = SDB_OSS_NEW clsCatalogSet( clFullName, TRUE ) ;
            PD_CHECK( pCataSet, SDB_OOM, error, PDERROR, "Alloc failed" ) ;
            rc = pCataSet->updateCatSet( cataObj, 0 ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to update catalog set, cata "
                         "info: %s, rc: %d", cataObj.toString().c_str(), rc ) ;

            // check collection sharding
            PD_CHECK( pCataSet->isSharding(), SDB_COLLECTION_NOTSHARD, error,
                      PDERROR, "Collection[%s] is not sharding, can't split",
                      clFullName ) ;

            // check collection MainCL. MainCL can't split
            PD_CHECK( !pCataSet->isMainCL(), SDB_MAIN_CL_OP_ERR, error,
                      PDERROR, "Collection[%s] is MainCL, can't split",
                      clFullName ) ;
         }

         // dispatch
         switch ( opCode )
         {
            case MSG_CAT_SPLIT_PREPARE_REQ :
               rc = catSplitPrepare( query, clFullName, pCataSet,
                                     groupID, _pEduCB ) ;
               break ;
            case MSG_CAT_SPLIT_READY_REQ :
               rc = catSplitReady( query, clFullName, pCataSet, groupID,
                                   _taskMgr, _pEduCB, _majoritySize(),
                                   &taskID ) ;
               break ;
            case MSG_CAT_SPLIT_CANCEL_REQ :
               rc = catSplitCancel( query, _pEduCB, groupID, _majoritySize() ) ;
               break ;
            case MSG_CAT_SPLIT_START_REQ :
               rc = catSplitStart( query, _pEduCB, _majoritySize() ) ;
               break ;
            case MSG_CAT_SPLIT_CHGMETA_REQ :
               rc = catSplitChgMeta( query, clFullName, pCataSet, _pEduCB,
                                     _majoritySize() ) ;
               break ;
            case MSG_CAT_SPLIT_CLEANUP_REQ :
               rc = catSplitCleanup( query, _pEduCB, _majoritySize() ) ;
               break ;
            case MSG_CAT_SPLIT_FINISH_REQ :
               rc = catSplitFinish( query, _pEduCB, _majoritySize() ) ;
               break ;
            default :
               rc = SDB_INVALIDARG ;
               break ;
         }

         PD_RC_CHECK( rc, PDERROR, "Split collection failed, opCode: %d, "
                      "rc: %d", opCode, rc ) ;

         // reply construct
         if ( CAT_INVALID_GROUPID != groupID )
         {
            vector< UINT32 > vecGroups ;
            vecGroups.push_back( groupID ) ;
            BSONObjBuilder replyBuild ;

            if ( _pCatCB->isImageEnabled() &&
                 !_pCatCB->getCatDCMgr()->groupInImage( groupID ) )
            {
               // the group that has no image can't be as collection' location
               PD_LOG( PDERROR, "The group[%d] that has no image can't "
                       "be as the collection's location when image is enabled",
                       groupID ) ;
               rc = SDB_CAT_GROUP_HASNOT_IMAGE ;
               goto error ;
            }

            if ( pCataSet )
            {
               replyBuild.append( CAT_CATALOGVERSION_NAME,
                                  pCataSet->getVersion() ) ;
            }
            if ( CLS_INVALID_TASKID != taskID )
            {
               replyBuild.append( CAT_TASKID_NAME, (long long)taskID ) ;
            }
            _pCatCB->makeGroupsObj( replyBuild, vecGroups, TRUE ) ;
            ctxBuf = rtnContextBuf( replyBuild.obj() ) ;
         }
      }
      catch( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Occurred exception: %s", e.what() ) ;
         goto error ;
      }

   done:
      if ( pCataSet )
      {
         SDB_OSS_DEL pCataSet ;
      }
      PD_TRACE_EXITRC ( SDB_CATALOGMGR_CMDSPLIT, rc ) ;
      return rc ;
   error:
      if ( CLS_INVALID_TASKID != taskID )
      {
         // rollback
         INT32 tmpRC = catRemoveTask( taskID, _pEduCB, 1 ) ;
         if ( tmpRC )
         {
            PD_LOG( PDERROR, "Remove task[%lld] failed, rc: %d",
                    taskID, tmpRC ) ;
         }
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGMGR__CHECKCSOBJ, "catCatalogueManager::_checkCSObj" )
   INT32 catCatalogueManager::_checkCSObj( const BSONObj & infoObj,
                                           catCSInfo & csInfo )
   {
      INT32 rc = SDB_OK ;

      csInfo._pCSName = NULL ;
      csInfo._domainName = NULL ;
      csInfo._pageSize = DMS_PAGE_SIZE_DFT ;
      csInfo._lobPageSize = DMS_DEFAULT_LOB_PAGE_SZ ;
      INT32 expected = 0 ;

      PD_TRACE_ENTRY ( SDB_CATALOGMGR__CHECKCSOBJ ) ;
      BSONObjIterator it( infoObj ) ;
      while ( it.more() )
      {
         BSONElement ele = it.next() ;

         // name
         if ( 0 == ossStrcmp( ele.fieldName(), CAT_COLLECTION_SPACE_NAME ) )
         {
            PD_CHECK( String == ele.type(), SDB_INVALIDARG, error, PDERROR,
                      "Field[%s] type[%d] error", CAT_COLLECTION_NAME,
                      ele.type() ) ;
            csInfo._pCSName = ele.valuestr() ;
            ++expected ;
         }
         // page size
         else if ( 0 == ossStrcmp( ele.fieldName(), CAT_PAGE_SIZE_NAME ) )
         {
            PD_CHECK( ele.isNumber(), SDB_INVALIDARG, error, PDERROR,
                      "Field[%s] type[%d] error", CAT_PAGE_SIZE_NAME,
                      ele.type() ) ;
            if ( 0 != ele.numberInt() )
            {
               csInfo._pageSize = ele.numberInt() ;
            }

            // check size value
            PD_CHECK ( csInfo._pageSize == DMS_PAGE_SIZE4K ||
                       csInfo._pageSize == DMS_PAGE_SIZE8K ||
                       csInfo._pageSize == DMS_PAGE_SIZE16K ||
                       csInfo._pageSize == DMS_PAGE_SIZE32K ||
                       csInfo._pageSize == DMS_PAGE_SIZE64K, SDB_INVALIDARG,
                       error, PDERROR, "PageSize must be 4K/8K/16K/32K/64K" ) ;
            ++expected ;
         }
         // domain name
         else if ( 0 == ossStrcmp( ele.fieldName(), CAT_DOMAIN_NAME ) )
         {
            PD_CHECK( String == ele.type(), SDB_INVALIDARG, error, PDERROR,
                      "Field[%s] type[%d] error", CAT_DOMAIN_NAME,
                      ele.type() ) ;
            csInfo._domainName = ele.valuestr() ;
            ++expected ;
         }
         // lob page size
         else if ( 0 == ossStrcmp( ele.fieldName(), CAT_LOB_PAGE_SZ_NAME ) )
         {
            PD_CHECK( ele.isNumber(), SDB_INVALIDARG, error, PDERROR,
                      "Field[%s] type[%d] error", CAT_LOB_PAGE_SZ_NAME,
                      ele.type() ) ;
            if ( 0 != ele.numberInt() )
            {
               csInfo._lobPageSize = ele.numberInt() ;
            }

            PD_CHECK ( csInfo._lobPageSize == DMS_PAGE_SIZE4K ||
                       csInfo._lobPageSize == DMS_PAGE_SIZE8K ||
                       csInfo._lobPageSize == DMS_PAGE_SIZE16K ||
                       csInfo._lobPageSize == DMS_PAGE_SIZE32K ||
                       csInfo._lobPageSize == DMS_PAGE_SIZE64K ||
                       csInfo._lobPageSize == DMS_PAGE_SIZE128K ||
                       csInfo._lobPageSize == DMS_PAGE_SIZE256K ||
                       csInfo._lobPageSize == DMS_PAGE_SIZE512K, SDB_INVALIDARG,
                       error, PDERROR, "PageSize must be 4K/8K/16K/32K/64K/128K/256K/512K" ) ;
            ++expected ;
         }
         else
         {
            PD_RC_CHECK ( SDB_INVALIDARG, PDERROR,
                          "Unexpected field[%s] in create collection space "
                          "command", ele.toString().c_str() ) ;
         }
      }

      PD_CHECK( csInfo._pCSName, SDB_INVALIDARG, error, PDERROR,
                "Collection space name not set" ) ;

      PD_CHECK( infoObj.nFields() == expected, SDB_INVALIDARG, error, PDERROR,
                "unexpected fields exsit." ) ;

   done:
      PD_TRACE_EXITRC ( SDB_CATALOGMGR__CHECKCSOBJ, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGMGR__CHECKANDBUILDCATARECORD, "catCatalogueManager::_checkAndBuildCataRecord" )
   INT32 catCatalogueManager::_checkAndBuildCataRecord( const BSONObj &infoObj,
                                                        UINT32 &fieldMask,
                                                        catCollectionInfo &clInfo,
                                                        BOOLEAN clNameIsNecessary )
   {
      INT32 rc = SDB_OK ;

      clInfo._pCLName            = NULL ;
      clInfo._replSize           = 1 ;
      clInfo._enSureShardIndex   = true ;
      clInfo._pShardingType      = CAT_SHARDING_TYPE_HASH ;
      clInfo._shardPartition     = CAT_SHARDING_PARTITION_DEFAULT ;
      clInfo._isHash             = TRUE ;
      clInfo._isSharding         = FALSE ;
      clInfo._isMainCL           = false;
      clInfo._assignType         = ASSIGN_RANDOM ;

      fieldMask = 0 ;

      PD_TRACE_ENTRY ( SDB_CATALOGMGR__CHECKANDBUILDCATARECORD ) ;
      BSONObjIterator it( infoObj ) ;
      while ( it.more() )
      {
         BSONElement eleTmp = it.next() ;

         // collection name
         if ( ossStrcmp( eleTmp.fieldName(), CAT_COLLECTION_NAME ) == 0 )
         {
            PD_CHECK( String == eleTmp.type(), SDB_INVALIDARG, error, PDERROR,
                      "Field[%s] type[%d] error", CAT_COLLECTION_NAME,
                      eleTmp.type() ) ;
            clInfo._pCLName = eleTmp.valuestr() ;
            fieldMask |= CAT_MASK_CLNAME ;
         }
         // sharding key
         else if ( ossStrcmp( eleTmp.fieldName(),
                              CAT_SHARDINGKEY_NAME ) == 0 )
         {
            PD_CHECK( Object == eleTmp.type(), SDB_INVALIDARG, error, PDERROR,
                      "Field[%s] type[%d] error", CAT_SHARDINGKEY_NAME,
                      eleTmp.type() ) ;
            clInfo._shardingKey = eleTmp.embeddedObject() ;
            PD_CHECK( _ixmIndexKeyGen::validateKeyDef( clInfo._shardingKey ),
                      SDB_INVALIDARG, error, PDERROR,
                      "Sharding key[%s] definition is invalid",
                      clInfo._shardingKey.toString().c_str() ) ;
            fieldMask |= CAT_MASK_SHDKEY ;
            clInfo._isSharding = TRUE ;
         }
         // repl size
         else if ( ossStrcmp( eleTmp.fieldName(), CAT_CATALOG_W_NAME ) == 0 )
         {
            PD_CHECK( NumberInt == eleTmp.type(), SDB_INVALIDARG, error,
                      PDERROR, "Field[%s] type[%d] error", CAT_CATALOG_W_NAME,
                      eleTmp.type() ) ;
            clInfo._replSize = eleTmp.numberInt() ;
            if ( 1 <= clInfo._replSize &&
                 clInfo._replSize <= CLS_REPLSET_MAX_NODE_SIZE )
            {
               /// do nothing.
            }
            else if ( clInfo._replSize == 0 )
            {
               clInfo._replSize = CLS_REPLSET_MAX_NODE_SIZE ;
            }
            else if ( -1 == clInfo._replSize )
            {
               /// do nothing
            }
            else
            {
               PD_LOG( PDERROR, "invalid repl size:%d", clInfo._replSize ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }

            fieldMask |= CAT_MASK_REPLSIZE ;
         }
         // ensure sharding index
         else if ( ossStrcmp( eleTmp.fieldName(), CAT_ENSURE_SHDINDEX ) == 0 )
         {
            PD_CHECK( Bool == eleTmp.type(), SDB_INVALIDARG, error,
                      PDERROR, "Field[%s] type[%d] error", CAT_ENSURE_SHDINDEX,
                      eleTmp.type() ) ;
            clInfo._enSureShardIndex = eleTmp.Bool() ;
            fieldMask |= CAT_MASK_SHDIDX ;
         }
         // sharding type
         else if ( ossStrcmp( eleTmp.fieldName(), CAT_SHARDING_TYPE ) == 0 )
         {
            PD_CHECK( String == eleTmp.type(), SDB_INVALIDARG, error,
                      PDERROR, "Field[%s] type[%d] error", CAT_SHARDING_TYPE,
                      eleTmp.type() ) ;

            // check string value
            clInfo._pShardingType = eleTmp.valuestr() ;
            PD_CHECK( 0 == ossStrcmp( clInfo._pShardingType,
                                      CAT_SHARDING_TYPE_HASH ) ||
                      0 == ossStrcmp( clInfo._pShardingType,
                                      CAT_SHARDING_TYPE_RANGE ),
                      SDB_INVALIDARG, error, PDERROR,
                      "Field[%s] value[%s] should be[%s/%s]",
                      CAT_SHARDING_TYPE, clInfo._pShardingType,
                      CAT_SHARDING_TYPE_HASH, CAT_SHARDING_TYPE_RANGE ) ;
            fieldMask |= CAT_MASK_SHDTYPE ;

            clInfo._isHash = ( 0 == ossStrcmp( clInfo._pShardingType,
                                               CAT_SHARDING_TYPE_HASH ) ) ;
         }
         // sharding partition
         else if ( ossStrcmp( eleTmp.fieldName(),
                              CAT_SHARDING_PARTITION ) == 0 )
         {
            PD_CHECK( NumberInt == eleTmp.type(), SDB_INVALIDARG, error,
                      PDERROR, "Field[%s] type[%d] error",
                      CAT_SHARDING_PARTITION, eleTmp.type() ) ;
            clInfo._shardPartition = eleTmp.numberInt() ;
            // must be the power of 2
            PD_CHECK( ossIsPowerOf2( (UINT32)clInfo._shardPartition ),
                      SDB_INVALIDARG, error, PDERROR,
                      "Field[%s] value must be power of 2",
                      CAT_SHARDING_PARTITION ) ;
            PD_CHECK( clInfo._shardPartition >= CAT_SHARDING_PARTITION_MIN &&
                      clInfo._shardPartition <= CAT_SHARDING_PARTITION_MAX,
                      SDB_INVALIDARG, error, PDERROR, "Field[%s] value[%d] "
                      "should between in[%d, %d]", CAT_SHARDING_PARTITION,
                      clInfo._shardPartition, CAT_SHARDING_PARTITION_MIN,
                      CAT_SHARDING_PARTITION_MAX ) ;
            fieldMask |= CAT_MASK_SHDPARTITION ;
         }
         // compression flag
         else if ( ossStrcmp ( eleTmp.fieldName(),
                               CAT_COMPRESSED ) == 0 )
         {
            PD_CHECK( Bool == eleTmp.type(), SDB_INVALIDARG, error,
                      PDERROR, "Field[%s] type[%d] error",
                      CAT_COMPRESSED, eleTmp.type() ) ;
            clInfo._isCompressed = eleTmp.boolean() ;
            clInfo._compressorType = UTIL_COMPRESSOR_SNAPPY ;
            fieldMask |= CAT_MASK_COMPRESSED ;
         }
         // main-collection flag
         else if ( ossStrcmp( eleTmp.fieldName(),
                              CAT_IS_MAINCL ) == 0 )
         {
            PD_CHECK( Bool == eleTmp.type(), SDB_INVALIDARG, error,
                      PDERROR, "Field[%s] type[%d] error",
                      CAT_IS_MAINCL, eleTmp.type() ) ;
            clInfo._isMainCL = eleTmp.boolean() ;
            fieldMask |= CAT_MASK_ISMAINCL ;
            if ( !( fieldMask & CAT_MASK_SHDTYPE ) )
            {
               clInfo._pShardingType = CAT_SHARDING_TYPE_RANGE ;
               clInfo._isHash = FALSE ;
            }
         }
         // group specified
         else if ( 0 == ossStrcmp( eleTmp.fieldName(),
                                   CAT_GROUP_NAME ) )
         {
            PD_CHECK( String == eleTmp.type(), SDB_INVALIDARG, error,
                      PDERROR, "Field[%s] type[%d] error",
                      CAT_GROUP_NAME, eleTmp.type() ) ;
            if ( 0 == ossStrcasecmp( eleTmp.valuestr(),
                                     CAT_ASSIGNGROUP_FOLLOW ) )
            {
               clInfo._assignType = ASSIGN_FOLLOW ;
            }
            else if ( 0 == ossStrcasecmp( eleTmp.valuestr(),
                                          CAT_ASSIGNGROUP_RANDOM ) )
            {
               clInfo._assignType = ASSIGN_RANDOM ;
            }
            else
            {
               clInfo._gpSpecified = eleTmp.valuestr() ;
            }
         }
         // auto split
         else if ( 0 == ossStrcmp( eleTmp.fieldName(),
                                   CAT_DOMAIN_AUTO_SPLIT ) )
         {
            PD_CHECK( Bool == eleTmp.type(), SDB_INVALIDARG, error,
                      PDERROR, "Field[%s] type[%d] error",
                      CAT_DOMAIN_AUTO_SPLIT, eleTmp.type() ) ;
            clInfo._autoSplit = eleTmp.Bool() ;
            fieldMask |= CAT_MASK_AUTOASPLIT ;
         }
         // auto rebalance
         else if ( 0 == ossStrcmp( eleTmp.fieldName(),
                                   CAT_DOMAIN_AUTO_REBALANCE ) )
         {
            PD_CHECK( Bool == eleTmp.type(), SDB_INVALIDARG, error,
                      PDERROR, "Field[%s] type[%d] error",
                      CAT_DOMAIN_AUTO_REBALANCE, eleTmp.type() ) ;
            clInfo._autoRebalance = eleTmp.Bool() ;
            fieldMask |= CAT_MASK_AUTOREBALAN ;
         }
         // auto index id
         else if ( 0 == ossStrcmp( eleTmp.fieldName(),
                                   CAT_AUTO_INDEX_ID ) )
         {
            PD_CHECK( Bool == eleTmp.type(), SDB_INVALIDARG, error,
                      PDERROR, "Field[%s] type[%d] error",
                      CAT_AUTO_INDEX_ID, eleTmp.type() ) ;
            clInfo._autoIndexId = eleTmp.Bool() ;
            fieldMask |= CAT_MASK_AUTOINDEXID ;
         }
         // compression type
         else if ( 0 == ossStrcmp( eleTmp.fieldName(),
                                   CAT_COMPRESSIONTYPE ) )
         {
            PD_CHECK( TRUE == clInfo._isCompressed, SDB_INVALIDARG, error,
                      PDERROR, "CompressionType option can only be set when "
                      "Compressed option is set as true" ) ;
            PD_CHECK( String == eleTmp.type(), SDB_INVALIDARG, error,
                      PDERROR, "Field[%s] type[%d] error",
                      CAT_COMPRESSIONTYPE, eleTmp.type() ) ;
            if ( 0 == ossStrcmp( eleTmp.valuestr(), CAT_COMPRESSOR_LZW ) )
            {
               clInfo._compressorType = UTIL_COMPRESSOR_LZW ;
            }
            else if ( 0 == ossStrcmp( eleTmp.valuestr(), CAT_COMPRESSOR_SNAPPY ) )
            {
               clInfo._compressorType = UTIL_COMPRESSOR_SNAPPY ;
            }
            /*
            else if ( 0 == ossStrcmp( eleTmp.valuestr(), CAT_COMPRESSOR_ZLIB ) )
            {
               clInfo._compressorType = UTIL_COMPRESSOR_ZLIB ;
            }
            else if ( 0 == ossStrcmp( eleTmp.valuestr(), CAT_COMPRESSOR_LZ4) )
            {
               clInfo._compressorType = UTIL_COMPRESSOR_LZ4 ;
            }
            */
            else
            {
               PD_LOG( PDERROR,
                       "Invalid Compression Type. Field[%s] value[%s] should "
                       "be [%s/%s] or leave empty",
                       CAT_COMPRESSIONTYPE, eleTmp.valuestr(),
                       CAT_COMPRESSOR_LZW, CAT_COMPRESSOR_SNAPPY );
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            fieldMask |= CAT_MASK_COMPRESSIONTYPE ;
         }
         else
         {
            PD_RC_CHECK ( SDB_INVALIDARG, PDERROR,
                          "Unexpected field[%s] in create collection command",
                          eleTmp.toString().c_str() ) ;
         }
      }
      if ( clInfo._isMainCL )
      {
         PD_CHECK ( clInfo._isSharding,
                    SDB_NO_SHARDINGKEY, error, PDERROR,
                    "main-collection must have ShardingKey!" );
         PD_CHECK ( !clInfo._isHash,
                    SDB_INVALID_MAIN_CL_TYPE, error, PDERROR,
                    "the sharding-type of main-collection must be range!" );

         PD_CHECK( !( CAT_MASK_AUTOINDEXID & fieldMask ),
                   SDB_INVALIDARG, error, PDERROR,
                   "can not set auto-index-id on main collection" ) ;
      }

      if ( clInfo._autoSplit || clInfo._autoRebalance )
      {
         PD_CHECK ( clInfo._isSharding,
                    SDB_NO_SHARDINGKEY, error, PDERROR,
                    "can not do split or rebalance with out ShardingKey!" );

         PD_CHECK ( NULL == clInfo._gpSpecified,
                    SDB_INVALIDARG, error, PDERROR,
                    "can not do split or rebalance with out more than one group" );

         PD_CHECK( clInfo._isHash,
                   SDB_INVALIDARG, error, PDERROR,
                   "auto options only can be set when shard type is hash" ) ;
      }

      if ( fieldMask & CAT_MASK_SHDIDX ||
           fieldMask & CAT_MASK_SHDTYPE ||
           fieldMask & CAT_MASK_SHDPARTITION )
      {
         PD_CHECK( fieldMask & CAT_MASK_SHDKEY,
                   SDB_INVALIDARG, error, PDERROR,
                   "these arguments are legal only when sharding key is specified." ) ;
      }

      if ( clNameIsNecessary )
      {
         PD_CHECK( clInfo._pCLName, SDB_INVALIDARG, error, PDERROR,
                   "Collection name not set" ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_CATALOGMGR__CHECKANDBUILDCATARECORD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGMGR__ASSIGNGROUP, "catCatalogueManager::_assignGroup" )
   INT32 catCatalogueManager::_assignGroup( vector < UINT32 > * pGoups,
                                            UINT32 &groupID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CATALOGMGR__ASSIGNGROUP ) ;
      if ( !pGoups || pGoups->size() == 0 )
      {
         rc = _pCatCB->getAGroupRand( groupID ) ;
      }
      else
      {
         UINT32 size = pGoups->size() ;
         groupID = (*pGoups)[ ossRand() % size ] ;
      }

      PD_TRACE_EXITRC ( SDB_CATALOGMGR__ASSIGNGROUP, rc ) ;
      return rc ;
   }

   INT32 catCatalogueManager::_checkGroupStatus( const CHAR *gpName )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN exist = FALSE ;
      if ( !_pCatCB->checkGroupActived( gpName, exist ) )
      {
         rc = SDB_REPL_GROUP_NOT_ACTIVE ;
         if ( !exist )
         {
            rc = SDB_CLS_GRP_NOT_EXIST ;
         }
      }

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGMGR__CHECKGROUPINDOMAIN, "catCatalogueManager::_checkGroupInDomain" )
   INT32 catCatalogueManager::_checkGroupInDomain( const CHAR * groupName,
                                                   const CHAR * domainName,
                                                   BOOLEAN & existed,
                                                   UINT32 *pGroupID )
   {
      INT32 rc = SDB_OK ;
      existed = FALSE ;

      PD_TRACE_ENTRY ( SDB_CATALOGMGR__CHECKGROUPINDOMAIN ) ;
      BSONObj groupInfo ;

      // Check group exist
      rc = catGetGroupObj( groupName, TRUE, groupInfo, _pEduCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Get group[%s] info failed, rc: %d",
                   groupName, rc ) ;

      // Get group ID
      if ( pGroupID )
      {
         INT32 tmpGrpID = CAT_INVALID_GROUPID ;
         rtnGetIntElement( groupInfo, CAT_GROUPID_NAME, tmpGrpID ) ;
         *pGroupID = (UINT32)tmpGrpID ;
      }

      // SYSTEM DOMAIN
      if ( 0 == ossStrcmp( domainName, CAT_SYS_DOMAIN_NAME ) )
      {
         existed = TRUE ;
      }
      // USER DOMAIN
      else
      {
         // Check domain exist
         BSONObj domainObj ;
         map<string, UINT32> groups ;
         rc = catGetDomainObj( domainName, domainObj, _pEduCB ) ;
         PD_RC_CHECK( rc, PDERROR, "Get domain[%s] failed, rc: %d",
                      domainName, rc ) ;

         rc = catGetDomainGroups( domainObj, groups ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get groups from domain info[%s], "
                      "rc: %d", domainObj.toString().c_str(), rc ) ;

         if ( groups.find( groupName ) != groups.end() )
         {
            existed = TRUE ;
         }
      }

   done:
      PD_TRACE_EXITRC ( SDB_CATALOGMGR__CHECKGROUPINDOMAIN, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGMGR__CREATECS, "catCatalogueManager::_createCS" )
   INT32 catCatalogueManager::_createCS( BSONObj &createObj,
                                         UINT32 &groupID )
   {
      INT32 rc               = SDB_OK ;
      string strGroupName ;

      const CHAR *csName     = NULL ;
      const CHAR *domainName = NULL ;
      BOOLEAN isSpaceExist   = FALSE ;
      PD_TRACE_ENTRY ( SDB_CATALOGMGR__CREATECS ) ;

      catCSInfo csInfo ;
      BSONObj spaceObj ;
      BSONObj domainObj ;
      vector< UINT32 >  domainGroups ;

      // check cs obj
      rc = _checkCSObj( createObj, csInfo ) ;
      PD_RC_CHECK( rc, PDERROR, "Check create collection space obj[%s] failed,"
                   "rc: %d", createObj.toString().c_str(), rc ) ;
      csName = csInfo._pCSName ;
      domainName = csInfo._domainName ;

      // name check
      rc = dmsCheckCSName( csName ) ;
      PD_RC_CHECK( rc, PDERROR, "Check collection space name[%s] failed, rc: "
                   "%d", csName, rc ) ;

      // check collection space is whether existed or not
      rc = catCheckSpaceExist( csName, isSpaceExist, spaceObj, _pEduCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check collection space existed, rc: "
                   "%d", rc ) ;
      PD_TRACE1 ( SDB_CATALOGMGR_CREATECS, PD_PACK_INT ( isSpaceExist ) ) ;
      PD_CHECK( FALSE == isSpaceExist, SDB_DMS_CS_EXIST, error, PDERROR,
                "Collection space[%s] is already existed", csName ) ;

      // check domain name
      if ( domainName )
      {
         rc = catGetDomainObj( domainName, domainObj, _pEduCB ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get domain[%s] obj, rc: %d",
                      domainName, rc ) ;
         rc = catGetDomainGroups( domainObj, domainGroups ) ;
         PD_RC_CHECK( rc, PDERROR, "Get domain[%s] groups failed, rc: %d",
                      domainObj.toString().c_str(), rc ) ;
      }

      // assign group
      rc = _assignGroup( &domainGroups, groupID ) ;
      PD_RC_CHECK( rc, PDERROR, "Assign group for collection space[%s] "
                   "failed, rc: %d", csName, rc ) ;
      catGroupID2Name( groupID, strGroupName, _pEduCB ) ;

      // insert new record
      {
         BSONObjBuilder newBuilder ;
         newBuilder.appendElements( csInfo.toBson() ) ;
         BSONObjBuilder sub1( newBuilder.subarrayStart( CAT_COLLECTION ) ) ;
         sub1.done() ;

         BSONObj newObj = newBuilder.obj() ;

         rc = rtnInsert( CAT_COLLECTION_SPACE_COLLECTION, newObj, 1, 0,
                         _pEduCB, _pDmsCB, _pDpsCB, _majoritySize() ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to insert collection space obj[%s] "
                      " to collection[%s], rc: %d", newObj.toString().c_str(),
                      CAT_COLLECTION_SPACE_COLLECTION, rc ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_CATALOGMGR__CREATECS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGMGR_CREATECOLLECTION, "catCatalogueManager::_createCL" )
   INT32 catCatalogueManager::_createCL( BSONObj & createObj,
                                         UINT32 &groupID,
                                         std::vector<UINT64> &taskIDs )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CATALOGMGR_CREATECOLLECTION ) ;

      UINT32 fieldMask = 0 ;
      catCollectionInfo clInfo ;
      const CHAR *collectionName = NULL ;
      BSONObj newCLRecordObj ;

      CHAR szSpace[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] = {0} ;
      CHAR szCollection[ DMS_COLLECTION_NAME_SZ + 1 ] = {0} ;

      BOOLEAN isSpaceExist = FALSE ;
      BSONObj boSpaceRecord ;
      BOOLEAN isCollectionExist = FALSE ;
      BSONObj boCollectionRecord ;
      BSONObj domainObj ;
      std::string strGroupName ;
      groupID = CAT_INVALID_GROUPID ;
      std::map<string, UINT32> range ;

      // check createObj
      rc = _checkAndBuildCataRecord( createObj, fieldMask, clInfo ) ;
      PD_RC_CHECK( rc, PDERROR, "Check create collection obj[%s] failed, rc: %d",
                   createObj.toString().c_str(), rc ) ;
      collectionName = clInfo._pCLName ;

      // get version from bucket collection
      clInfo._version = catGetBucketVersion( collectionName, _pEduCB ) ;

      PD_TRACE1 ( SDB_CATALOGMGR_CREATECOLLECTION,
                  PD_PACK_STRING ( collectionName ) ) ;

      // split collection full name to csname and clname
      rc = catResolveCollectionName( collectionName,
                                     ossStrlen ( collectionName ),
                                     szSpace, DMS_COLLECTION_SPACE_NAME_SZ,
                                     szCollection, DMS_COLLECTION_NAME_SZ );
      PD_RC_CHECK ( rc, PDERROR, "Failed to resolve collection name: %s",
                    collectionName ) ;

      // make sure the name is valid
      rc = dmsCheckCLName( szCollection, FALSE ) ;
      PD_RC_CHECK ( rc, PDERROR,
                    "Failed to check collection name: %s, rc = %d",
                    szCollection, rc ) ;

      // get collection-space
      rc = catCheckSpaceExist( szSpace, isSpaceExist,
                               boSpaceRecord, _pEduCB ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to check if collection space exist, "
                    "rc = %d", rc );
      PD_CHECK ( isSpaceExist, SDB_DMS_CS_NOTEXIST, error, PDERROR,
                 "Create failed, the collection space(%s) is not exist",
                 szSpace ) ;

      PD_TRACE1 ( SDB_CATALOGMGR_CREATECOLLECTION,
                  PD_PACK_INT ( isSpaceExist ) ) ;

      // here we do not care what the values are
      // we care how many records in the specified collection space
      {
         BSONElement ele = boSpaceRecord.getField( CAT_COLLECTION ) ;
         /// some times, the CAT_COLLECTION will be not exist
         if ( Array == ele.type() )
         {
            if ( ele.embeddedObject().nFields() >= DMS_MME_SLOTS )
            {
               PD_LOG( PDERROR, "CollectionSpace: [%s] cannot accept more "
                       "collection", szSpace );
               rc = SDB_DMS_NOSPC ;
               goto error ;
            }
         }
      }

      // check if collection exist
      rc = catCheckCollectionExist( collectionName, isCollectionExist,
                                    boCollectionRecord, _pEduCB ) ;
      PD_RC_CHECK ( rc, PDERROR,
                    "Failed to check if collection exist, rc = %d", rc ) ;
      PD_CHECK ( !isCollectionExist, SDB_DMS_EXIST, error, PDERROR,
                 "Create failed, the collection(%s) exists",
                 collectionName ) ;

      PD_TRACE1 ( SDB_CATALOGMGR_CREATECOLLECTION,
                  PD_PACK_INT ( isCollectionExist ) ) ;

      // try to get domain obj of cl.
      {
         BSONElement domainEle = boSpaceRecord.getField( CAT_DOMAIN_NAME ) ;
         if ( String == domainEle.type() )
         {
            rc = catGetDomainObj( domainEle.valuestr(), domainObj, _pEduCB ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to get domain obj os cs[%s], rc:%d",
                       szSpace, rc ) ;
               goto error ;
            }
         }
      }

      rc = _combineOptions( domainObj, boSpaceRecord, fieldMask, clInfo ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to combine options, domainObj[%s],"
                 "create cl options[%s], rc:%d", domainObj.toString().c_str(),
                 createObj.toString().c_str(), rc ) ;
         goto error ;
      }

      /// choose a group to create cl
      rc = _chooseGroupOfCl( domainObj, boSpaceRecord, clInfo,
                             strGroupName, groupID, range ) ;
      PD_RC_CHECK( rc, PDERROR, "failed to choose group for cl[%s], rc: %d",
                   collectionName, rc ) ;

      rc = _checkGroupStatus( strGroupName.c_str() ) ;
      if ( SDB_OK != rc )
      {
         if ( SDB_CLS_GRP_NOT_EXIST == rc )
         {
            PD_LOG( PDERROR, "group[%s] is not exist",
                    strGroupName.c_str() ) ;
         }
         else if ( SDB_REPL_GROUP_NOT_ACTIVE == rc )
         {
            PD_LOG( PDERROR, "group[%s] is inactive",
                    strGroupName.c_str() ) ;
         }
         goto error ;
      }


      if ( !clInfo._isMainCL && clInfo._autoSplit && !range.empty())
      {
         if (clInfo._shardPartition < range.size())
         {
            rc = SDB_INVALIDARG;
            PD_LOG_MSG(PDERROR, "Partition can not less than group number of domain."
                   "partition = %d, group number = %d", clInfo._shardPartition, range.size());
			goto error;
         }
      }

      // build new collection record for meta data.
      rc = _buildCatalogRecord( clInfo, fieldMask, 0, groupID,
                                strGroupName.c_str(),
                                newCLRecordObj ) ;
      PD_RC_CHECK( rc, PDERROR, "Build new collection catalog record failed, "
                   "rc: %d", rc ) ;

      // insert to system collectin of meta data.
      rc = rtnInsert( CAT_COLLECTION_INFO_COLLECTION, newCLRecordObj,
                      1, 0, _pEduCB, _pDmsCB, _pDpsCB, _majoritySize() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed insert record[%s] to collection[%s], "
                   "rc: %d", newCLRecordObj.toString().c_str(),
                   CAT_COLLECTION_INFO_COLLECTION, rc ) ;

      // update collection space info
      rc = catAddCL2CS( szSpace, szCollection, _pEduCB, _pDmsCB,
                        _pDpsCB, _majoritySize() ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Update collection to space failed, rc: %d", rc ) ;
         goto rollback ;
      }

      if ( clInfo._autoSplit )
      {
         rc = _autoHashSplit( newCLRecordObj, taskIDs,
                              strGroupName.c_str(), &range ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to split collection[%s], rc:%d",
                    collectionName, rc ) ;
            goto error ;
         }
      }
   done :
      PD_TRACE_EXITRC ( SDB_CATALOGMGR_CREATECOLLECTION, rc ) ;
      return rc ;
   error :
      goto done ;
   rollback:
      catRemoveCL( collectionName , _pEduCB, _pDmsCB, _pDpsCB,
                   _majoritySize() ) ;
      groupID = CAT_INVALID_GROUPID ;
      goto done ;
   }

   // build catalogue-info record:
   // {  Name: "SpaceName.CollectionName", Version: 1,
   //    ShardingKey: { Key1: 1, Key2: -1 },
   //    CataInfo:
   //       [ { GroupID: 1000, LowBound:{ "":MinKey,"":MaxKey }, UpBound:{"":MaxKey,"":MinKey} } ] }
   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGMGR_BUILDCATALOGRECORD, "catCatalogueManager::_buildCatalogRecord" )
   INT32 catCatalogueManager::_buildCatalogRecord( const catCollectionInfo & clInfo,
                                                   UINT32 mask,
                                                   UINT32 attribute,
                                                   UINT32 groupID,
                                                   const CHAR *groupName,
                                                   BSONObj & catRecord )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CATALOGMGR_BUILDCATALOGRECORD ) ;

      BSONObjBuilder builder ;
      CHAR szAttr[ 100 ] = { 0 } ;

      if ( ( mask & CAT_MASK_COMPRESSED ) && clInfo._isCompressed )
      {
         attribute |= DMS_MB_ATTR_COMPRESSED ;
      }
      if ( ( mask & CAT_MASK_AUTOINDEXID ) && !clInfo._autoIndexId )
      {
         attribute |= DMS_MB_ATTR_NOIDINDEX ;
      }
      mbAttr2String( attribute, szAttr, sizeof( szAttr ) - 1 ) ;

      if ( mask & CAT_MASK_CLNAME )
      {
         builder.append( CAT_CATALOGNAME_NAME, clInfo._pCLName ) ;
      }

      /// this is not specified by user.
      builder.append( CAT_CATALOGVERSION_NAME,
                      0 == clInfo._version ?
                      CAT_VERSION_BEGIN :
                      clInfo._version ) ;

      if ( mask & CAT_MASK_REPLSIZE )
      {
         builder.append( CAT_CATALOG_W_NAME, clInfo._replSize ) ;
      }

      builder.append( CAT_ATTRIBUTE_NAME, attribute ) ;
      builder.append( FIELD_NAME_ATTRIBUTE_DESC, szAttr ) ;

      /// only record the options specified by user.
      if ( ( mask & CAT_MASK_COMPRESSED ) && clInfo._isCompressed )
      {
         builder.append( CAT_COMPRESSIONTYPE, clInfo._compressorType ) ;
         builder.append( FIELD_NAME_COMPRESSIONTYPE_DESC,
                         utilCompressType2String( clInfo._compressorType ) ) ;
      }
      if ( mask & CAT_MASK_SHDKEY )
      {
         builder.append( CAT_SHARDINGKEY_NAME, clInfo._shardingKey ) ;
         builder.appendBool( CAT_ENSURE_SHDINDEX, clInfo._enSureShardIndex ) ;
         builder.append( CAT_SHARDING_TYPE, clInfo._pShardingType ) ;
         if( clInfo._isHash )
         {
            builder.append( CAT_SHARDING_PARTITION, clInfo._shardPartition ) ;

            /// optimize query on hash-sharding only sdb's version >= 1.12
            /// update version since 1.12.4
            builder.append( CAT_INTERNAL_VERSION, CAT_INTERNAL_VERSION_3 ) ;
         }
      }
      /// add catainfo to record even not specified by user.
      if ( clInfo._isMainCL )
      {
         builder.appendBool( CAT_IS_MAINCL, clInfo._isMainCL );
         BSONObjBuilder sub( builder.subarrayStart( CAT_CATALOGINFO_NAME ) ) ;
         sub.done() ;
      }
      else
      {
         // cata info build
         BSONObjBuilder sub( builder.subarrayStart( CAT_CATALOGINFO_NAME ) ) ;
         BSONObjBuilder cataItemBd ( sub.subobjStart ( sub.numStr(0) ) ) ;
         cataItemBd.append ( CAT_CATALOGGROUPID_NAME, (INT32)groupID ) ;
         if ( groupName )
         {
            cataItemBd.append ( CAT_GROUPNAME_NAME, groupName ) ;
         }
         if ( clInfo._isSharding )
         {
            // add LowBound and UpBound
            BSONObj lowBound, upBound ;

            if ( !clInfo._isHash )
            {
               Ordering order = Ordering::make( clInfo._shardingKey ) ;
               rc = _buildInitBound ( clInfo._shardingKey, order ,
                                      lowBound, upBound ) ;
            }
            else
            {
               rc =_buildHashBound( lowBound, upBound, clInfo._shardPartition ) ;
            }
            PD_RC_CHECK( rc, PDERROR, "Build cata info bound failed, rc: %d", rc ) ;

            cataItemBd.append ( CAT_LOWBOUND_NAME, lowBound ) ;
            cataItemBd.append ( CAT_UPBOUND_NAME, upBound ) ;
         }
         cataItemBd.done () ;
         sub.done () ;
      }

      if ( mask & CAT_MASK_AUTOASPLIT )
      {
         builder.appendBool ( CAT_DOMAIN_AUTO_SPLIT, clInfo._autoSplit ) ;
      }

      if ( mask & CAT_MASK_AUTOREBALAN )
      {
         builder.appendBool ( CAT_DOMAIN_AUTO_REBALANCE, clInfo._autoRebalance ) ;
      }

      if ( mask & CAT_MASK_AUTOINDEXID )
      {
         builder.appendBool( CAT_AUTO_INDEX_ID, clInfo._autoIndexId ) ;
      }

      catRecord = builder.obj () ;
   done:
      PD_TRACE_EXITRC ( SDB_CATALOGMGR_BUILDCATALOGRECORD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGMGR_BUILDINITBOUND, "catCatalogueManager::_buildInitBound" )
   INT32 catCatalogueManager::_buildInitBound ( const BSONObj &shardingKey,
                                                const Ordering & order,
                                                BSONObj & lowBound,
                                                BSONObj & upBound )
   {
      PD_TRACE_ENTRY ( SDB_CATALOGMGR_BUILDINITBOUND ) ;
//      PD_TRACE1 ( SDB_CATALOGMGR_BUILDINITBOUND,
//                  PD_PACK_UINT ( shardingKey.nFields() ) ) ;

      INT32 index = 0 ;
      BSONObjBuilder lowBoundBD ;
      BSONObjBuilder upBoundBD ;

      BSONObjIterator iter( shardingKey ) ;
      while ( iter.more() )
      {
         BSONElement ele        = iter.next() ;
         const CHAR * fieldName = ele.fieldName() ;
         if ( order.get( index ) == 1 )
         {
            lowBoundBD.appendMinKey ( fieldName ) ;
            upBoundBD.appendMaxKey ( fieldName ) ;
         }
         else
         {
            lowBoundBD.appendMaxKey ( fieldName ) ;
            upBoundBD.appendMinKey ( fieldName ) ;
         }

         ++index ;
      }

      lowBound = lowBoundBD.obj () ;
      upBound = upBoundBD.obj () ;
      PD_TRACE_EXIT ( SDB_CATALOGMGR_BUILDINITBOUND ) ;
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGMGR_PROCESSMSG, "catCatalogueManager::processMsg" )
   INT32 catCatalogueManager::processMsg( const NET_HANDLE &handle,
                                          MsgHeader *pMsg )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_CATALOGMGR_PROCESSMSG ) ;
      PD_TRACE1 ( SDB_CATALOGMGR_PROCESSMSG,
                  PD_PACK_INT ( pMsg->opCode ) ) ;

      switch ( pMsg->opCode )
      {
      // command dispatch, need the second dispath in the function
      case MSG_CAT_CREATE_COLLECTION_REQ :
      case MSG_CAT_DROP_COLLECTION_REQ :
      case MSG_CAT_CREATE_COLLECTION_SPACE_REQ :
      case MSG_CAT_DROP_SPACE_REQ :
      case MSG_CAT_ALTER_COLLECTION_REQ :
      case MSG_CAT_LINK_CL_REQ :
      case MSG_CAT_UNLINK_CL_REQ :
      case MSG_CAT_SPLIT_PREPARE_REQ :
      case MSG_CAT_SPLIT_READY_REQ :
      case MSG_CAT_SPLIT_CANCEL_REQ :
      case MSG_CAT_SPLIT_START_REQ :
      case MSG_CAT_SPLIT_CHGMETA_REQ :
      case MSG_CAT_SPLIT_CLEANUP_REQ :
      case MSG_CAT_SPLIT_FINISH_REQ :
      case MSG_CAT_CRT_PROCEDURES_REQ :
      case MSG_CAT_RM_PROCEDURES_REQ :
      case MSG_CAT_CREATE_DOMAIN_REQ :
      case MSG_CAT_DROP_DOMAIN_REQ :
      case MSG_CAT_ALTER_DOMAIN_REQ :
         {
            // up commands is run in cluster acitve status
            _pCatCB->getCatDCMgr()->setWritedCommand( TRUE ) ;
            rc = processCommandMsg( handle, pMsg, TRUE ) ;
            break;
         }
      case MSG_CAT_QUERY_SPACEINFO_REQ :
         {
            rc = processCommandMsg( handle, pMsg, TRUE ) ;
            break;
         }
      case MSG_CAT_QUERY_CATALOG_REQ:
         {
            rc = processQueryCatalogue( handle, pMsg ) ;
            break;
         }
      case MSG_CAT_QUERY_TASK_REQ:
         {
            rc = processQueryTask ( handle, pMsg ) ;
            break ;
         }
      default:
         {
            rc = SDB_UNKNOWN_MESSAGE;
            PD_LOG( PDWARNING, "received unknown message (opCode: [%d]%u)",
                    IS_REPLY_TYPE(pMsg->opCode),
                    GET_REQUEST_TYPE(pMsg->opCode) ) ;
            break;
         }
      }
      PD_TRACE_EXITRC ( SDB_CATALOGMGR_PROCESSMSG, rc ) ;
      return rc;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGMGR_PROCESSCOMMANDMSG, "catCatalogueManager::processCommandMsg" )
   INT32 catCatalogueManager::processCommandMsg( const NET_HANDLE &handle,
                                                 MsgHeader *pMsg,
                                                 BOOLEAN writable )
   {
      INT32 rc = SDB_OK ;
      MsgOpQuery *pQueryReq = (MsgOpQuery *)pMsg ;

      PD_TRACE_ENTRY ( SDB_CATALOGMGR_PROCESSCOMMANDMSG ) ;
      MsgOpReply replyHeader ;
      rtnContextBuf ctxBuff ;

      INT32      opCode = pQueryReq->header.opCode ;
      BOOLEAN    fillPeerRouteID = FALSE ;

      INT32 flag = 0 ;
      CHAR *pCMDName = NULL ;
      INT64 numToSkip = 0 ;
      INT64 numToReturn = 0 ;
      CHAR *pQuery = NULL ;
      CHAR *pFieldSelector = NULL ;
      CHAR *pOrderBy = NULL ;
      CHAR *pHint = NULL ;

      // init reply msg
      replyHeader.header.messageLength = sizeof( MsgOpReply ) ;
      replyHeader.contextID = -1 ;
      replyHeader.flags = SDB_OK ;
      replyHeader.numReturned = 0 ;
      replyHeader.startFrom = 0 ;
      _fillRspHeader( &(replyHeader.header), &(pQueryReq->header) ) ;

      if ( MSG_CAT_SPLIT_START_REQ == opCode ||
           MSG_CAT_SPLIT_CHGMETA_REQ == opCode ||
           MSG_CAT_SPLIT_CLEANUP_REQ == opCode ||
           MSG_CAT_SPLIT_FINISH_REQ == opCode )
      {
         fillPeerRouteID = TRUE ;
         _pCatCB->getCatDCMgr()->setWritedCommand( FALSE ) ;
      }

      // extract msg
      rc = msgExtractQuery( (CHAR*)pMsg, &flag, &pCMDName, &numToSkip,
                            &numToReturn, &pQuery, &pFieldSelector,
                            &pOrderBy, &pHint ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to extract query msg, rc: %d", rc ) ;

      if ( writable )
      {
         // primary check
         BOOLEAN isDelay = FALSE ;
         rc = _pCatCB->primaryCheck( _pEduCB, TRUE, isDelay ) ;
         if ( isDelay )
         {
            goto done ;
         }
         else if ( rc )
         {
            PD_LOG ( PDWARNING, "Service deactive but received command: %s,"
                     "opCode: %d, rc: %d", pCMDName,
                     pQueryReq->header.opCode, rc ) ;
            goto error ;
         }
      }

      if ( _pCatCB->getCatDCMgr()->isWritedCommand() &&
           _pCatCB->isDCReadonly() )
      {
         rc = SDB_CAT_CLUSTER_IS_READONLY ;
         goto error ;
      }

      // the second dispatch msg
      switch ( pQueryReq->header.opCode )
      {
         case MSG_CAT_CREATE_COLLECTION_REQ :
            rc = processCmdCreateCL( pQuery, ctxBuff ) ;
            break ;
         case MSG_CAT_CREATE_COLLECTION_SPACE_REQ :
            rc = processCmdCreateCS( pQuery, ctxBuff ) ;
            break ;
         case MSG_CAT_SPLIT_PREPARE_REQ :
         case MSG_CAT_SPLIT_READY_REQ :
         case MSG_CAT_SPLIT_CANCEL_REQ :
         case MSG_CAT_SPLIT_START_REQ :
         case MSG_CAT_SPLIT_CHGMETA_REQ :
         case MSG_CAT_SPLIT_CLEANUP_REQ :
         case MSG_CAT_SPLIT_FINISH_REQ :
            rc = processCmdSplit( pQuery, pQueryReq->header.opCode,
                                  ctxBuff ) ;
            break ;
         case MSG_CAT_QUERY_SPACEINFO_REQ :
            rc = processCmdQuerySpaceInfo( pQuery, ctxBuff ) ;
            break ;
         case MSG_CAT_DROP_COLLECTION_REQ :
            rc = processCmdDropCollection( pQuery, pQueryReq->version ) ;
            break ;
         case MSG_CAT_DROP_SPACE_REQ :
            rc = processCmdDropCollectionSpace( pQuery ) ;
            break ;
         case MSG_CAT_ALTER_COLLECTION_REQ :
            rc = processAlterCollection( pQuery, ctxBuff ) ;
            break ;
         case MSG_CAT_CRT_PROCEDURES_REQ :
            rc = processCmdCrtProcedures( pQuery ) ;
            break ;
         case MSG_CAT_RM_PROCEDURES_REQ :
            rc = processCmdRmProcedures( pQuery ) ;
            break ;
         case MSG_CAT_LINK_CL_REQ :
            rc = processCmdLinkCollection( pQuery, ctxBuff ) ;
            break;
         case MSG_CAT_UNLINK_CL_REQ :
            rc = processCmdUnlinkCollection( pQuery, ctxBuff );
            break;
         case MSG_CAT_CREATE_DOMAIN_REQ :
            rc = processCmdCreateDomain ( pQuery ) ;
            break ;
         case MSG_CAT_DROP_DOMAIN_REQ :
            rc = processCmdDropDomain ( pQuery ) ;
            break ;
         case MSG_CAT_ALTER_DOMAIN_REQ :
            rc = processCmdAlterDomain ( pQuery ) ;
            break ;
         default :
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Recieved unknow command: %s, opCode: %d",
                    pCMDName, pQueryReq->header.opCode ) ;
            break ;
      }

      PD_RC_CHECK( rc, PDERROR, "Process command[%s] failed, opCode: %d, "
                   "rc: %d", pCMDName, pQueryReq->header.opCode, rc ) ;

   done:
      if ( fillPeerRouteID )
      {
         replyHeader.header.routeID.value = pQueryReq->header.routeID.value ;
      }

      if ( !_pCatCB->isDelayed() )
      {
         // send reply
         if ( 0 == ctxBuff.size() )
         {
            rc = _pCatCB->netWork()->syncSend( handle, (void*)&replyHeader ) ;
         }
         else
         {
            replyHeader.header.messageLength += ctxBuff.size() ;
            replyHeader.numReturned = ctxBuff.recordNum() ;
            rc = _pCatCB->netWork()->syncSend( handle, &(replyHeader.header),
                                               (void*)ctxBuff.data(),
                                               ctxBuff.size() ) ;
         }
      }
      PD_TRACE_EXITRC ( SDB_CATALOGMGR_PROCESSCOMMANDMSG, rc ) ;
      return rc ;
   error:
      replyHeader.flags = rc ;
      if( SDB_CLS_NOT_PRIMARY == rc )
      {
         replyHeader.startFrom = _pCatCB->getPrimaryNode() ;
      }
      goto done ;
   }

   void catCatalogueManager::_fillRspHeader( MsgHeader * rspMsg,
                                             const MsgHeader * reqMsg )
   {
      rspMsg->opCode = MAKE_REPLY_TYPE( reqMsg->opCode ) ;
      rspMsg->requestID = reqMsg->requestID ;
      rspMsg->routeID.value = 0 ;
      rspMsg->TID = reqMsg->TID ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGMGR__BUILDHASHBOUND, "catCatalogueManager::_buildHashBound" )
   INT32 catCatalogueManager::_buildHashBound( BSONObj& lowBound,
                                               BSONObj& upBound,
                                               INT32 paritition )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATALOGMGR__BUILDHASHBOUND ) ;

      lowBound = BSON("" << CAT_HASH_LOW_BOUND ) ;
      upBound = BSON("" << paritition )  ;

      PD_TRACE_EXITRC( SDB_CATALOGMGR__BUILDHASHBOUND, rc ) ;
      return rc ;
   }

   INT16 catCatalogueManager::_majoritySize()
   {
      return _pCatCB->majoritySize() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGMGR_CMDLINKCOLLECTION, "catCatalogueManager::processCmdLinkCollection" )
   INT32 catCatalogueManager::processCmdLinkCollection( const CHAR *pQuery,
                                                        rtnContextBuf &ctxBuf )
   {
      INT32 rc = SDB_OK;
      std::string strMainCLName;
      std::string strSubCLName;
      PD_TRACE_ENTRY ( SDB_CATALOGMGR_CMDLINKCOLLECTION ) ;
      BSONObj boLowBound;
      BSONObj boUpBound;
      BSONObjBuilder retObjBuilder ;
      std::vector<UINT32>  groupList;
      try
      {
         BSONObj boQuery( pQuery );
         BSONElement beMainCLName = boQuery.getField( CAT_COLLECTION_NAME );
         PD_CHECK( beMainCLName.type() == String, SDB_INVALIDARG, error,
                   PDERROR, "failed to link the collection, get field(%s) "
                   "failed!", CAT_COLLECTION_NAME );
         strMainCLName = beMainCLName.str();
         PD_CHECK( !strMainCLName.empty(), SDB_INVALIDARG, error, PDERROR,
                   "invalid field:%s", CAT_COLLECTION_NAME );

         {
         BSONElement beSubCLName = boQuery.getField( CAT_SUBCL_NAME );
         PD_CHECK( beSubCLName.type() == String, SDB_INVALIDARG, error, PDERROR,
                   "failed to link the collection, get field(%s) failed!",
                   CAT_SUBCL_NAME );
         strSubCLName = beSubCLName.str();
         PD_CHECK( !strSubCLName.empty(), SDB_INVALIDARG, error, PDERROR,
                   "invalid field:%s", CAT_SUBCL_NAME );
         }

         {
         BSONElement beLowBound = boQuery.getField( CAT_LOWBOUND_NAME );
         PD_CHECK( beLowBound.type() == Object, SDB_INVALIDARG, error, PDERROR,
                   "invalid field:%s", CAT_LOWBOUND_NAME );
         boLowBound = beLowBound.embeddedObject();
         }

         {
         BSONElement beUpBound = boQuery.getField( CAT_UPBOUND_NAME );
         PD_CHECK( beUpBound.type() == Object, SDB_INVALIDARG, error, PDERROR,
                   "invalid field:%s", CAT_UPBOUND_NAME );
         boUpBound = beUpBound.embeddedObject();
         }

         rc = catLinkCL( strMainCLName.c_str(), strSubCLName.c_str(),
                         boLowBound, boUpBound, _pEduCB, _pDmsCB,
                         _pDpsCB, _majoritySize(), groupList );
         PD_RC_CHECK( rc, PDERROR,
                      "failed to link the sub-collection(%s) "
                      "to main-collection(%s)(rc=%d)",
                      strMainCLName.c_str(), strSubCLName.c_str(), rc );

         // make reply obj
         _pCatCB->makeGroupsObj( retObjBuilder, groupList, TRUE ) ;
         ctxBuf = rtnContextBuf( retObjBuilder.obj() ) ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC ( SDB_CATALOGMGR_CMDLINKCOLLECTION, rc ) ;
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGMGR_CMDUNLINKCOLLECTION, "catCatalogueManager::processCmdUnlinkCollection" )
   INT32 catCatalogueManager::processCmdUnlinkCollection( const CHAR *pQuery,
                                                          rtnContextBuf &ctxBuf )
   {
      INT32 rc = SDB_OK;
      std::string strMainCLName;
      std::string strSubCLName;
      std::vector<UINT32>  groupList ;
      BSONObjBuilder retObjBuilder ;
      PD_TRACE_ENTRY ( SDB_CATALOGMGR_CMDUNLINKCOLLECTION ) ;

      try
      {
         BSONObj boQuery( pQuery );
         BSONElement beMainCLName = boQuery.getField( CAT_COLLECTION_NAME );
         PD_CHECK( beMainCLName.type() == String, SDB_INVALIDARG, error,
                   PDERROR, "failed to link the collection, get field(%s) "
                   "failed!", CAT_COLLECTION_NAME );
         strMainCLName = beMainCLName.str();
         PD_CHECK( !strMainCLName.empty(), SDB_INVALIDARG, error, PDERROR,
                   "invalid field:%s", CAT_COLLECTION_NAME );

         {
         BSONElement beSubCLName = boQuery.getField( CAT_SUBCL_NAME );
         PD_CHECK( beSubCLName.type() == String, SDB_INVALIDARG, error, PDERROR,
                   "failed to link the collection, get field(%s) failed!",
                   CAT_SUBCL_NAME );
         strSubCLName = beSubCLName.str();
         PD_CHECK( !strSubCLName.empty(), SDB_INVALIDARG, error, PDERROR,
                   "invalid field:%s", CAT_SUBCL_NAME );
         }

         rc = catUnlinkCL( strMainCLName.c_str(), strSubCLName.c_str(),
                           _pEduCB, _pDmsCB, _pDpsCB, _majoritySize(),
                           groupList );
         PD_RC_CHECK( rc, PDERROR,
                      "failed to unlink the sub-collection(%s) "
                      "from main-collection(%s)(rc=%d)",
                      strMainCLName.c_str(), strSubCLName.c_str(), rc );

         // make ret obj
         _pCatCB->makeGroupsObj( retObjBuilder, groupList, TRUE ) ;
         ctxBuf = rtnContextBuf( retObjBuilder.obj() ) ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC ( SDB_CATALOGMGR_CMDUNLINKCOLLECTION, rc ) ;
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGMGR_CREATEDOMAIN, "catCatalogueManager::processCmdCreateDomain" )
   INT32 catCatalogueManager::processCmdCreateDomain ( const CHAR *pQuery )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CATALOGMGR_CREATEDOMAIN ) ;
      // first extract pQuery and find the options
      try
      {
         BSONObj tempObj ;
         BSONObj queryObj ;
         BSONObj insertObj ;
         BSONObj boQuery( pQuery );
         BSONObjBuilder ob ;
         BSONElement beDomainOptions ;
         const CHAR *pDomainName = NULL ;
         INT32 expectedObjSize   = 0 ;
         // find out the domain name
         BSONElement beDomainName = boQuery.getField ( CAT_DOMAINNAME_NAME ) ;
         PD_CHECK( beDomainName.type() == String, SDB_INVALIDARG, error,
                   PDERROR, "failed to create domain, get field(%s) "
                   "failed!", CAT_DOMAINNAME_NAME );
         pDomainName = beDomainName.valuestr() ;
         PD_TRACE1 ( SDB_CATALOGMGR_CREATEDOMAIN, PD_PACK_STRING(pDomainName) ) ;
         // domain name validation
         rc = catDomainNameValidate ( pDomainName ) ;
         PD_CHECK ( SDB_OK == rc, rc, error, PDERROR,
                    "Invalid domain name: %s, rc = %d", pDomainName, rc ) ;
         ob.append ( CAT_DOMAINNAME_NAME, pDomainName ) ;
         expectedObjSize ++ ;
         // options validation
         beDomainOptions = boQuery.getField ( CAT_OPTIONS_NAME ) ;
         if ( !beDomainOptions.eoo() && beDomainOptions.type() != Object )
         {
            PD_LOG ( PDERROR,
                     "Invalid options type, expected eoo or object" ) ;
            rc = SDB_INVALIDARG ;
         }
         // if we provide options, let's extract each option
         if ( beDomainOptions.type() == Object )
         {
            vector< string > vecGroups ;
            rc = catDomainOptionsExtract ( beDomainOptions.embeddedObject(),
                                            _pEduCB, &ob, &vecGroups ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to validate domain options, rc = %d",
                        rc ) ;
               goto error ;
            }
            expectedObjSize ++ ;

            // check group is active or not
            for ( UINT32 i = 0 ; i < vecGroups.size() ; ++i )
            {
               rc = _checkGroupStatus( vecGroups[i].c_str() ) ;
               if ( SDB_OK != rc )
               {
                  if ( SDB_CLS_GRP_NOT_EXIST == rc )
                  {
                     PD_LOG( PDERROR, "group[%s] is not exist",
                             vecGroups[i].c_str() ) ;
                  }
                  else if ( SDB_REPL_GROUP_NOT_ACTIVE == rc )
                  {
                     PD_LOG( PDERROR, "group[%s] is inactive",
                             vecGroups[i].c_str() ) ;
                  }
                  goto error ;
               }
            }

            if ( _pCatCB->isImageEnabled() )
            {
               // the group that has no image can't be added to domain when
               // image is enabled
               for ( UINT32 i = 0 ; i < vecGroups.size() ; ++i )
               {
                  if ( !_pCatCB->getCatDCMgr()->groupInImage( vecGroups[i] ) )
                  {
                     PD_LOG( PDERROR, "The group[%s] that has no image can't "
                             "be added to domain when image is enabled",
                             vecGroups[i].c_str() ) ;
                     rc = SDB_CAT_GROUP_HASNOT_IMAGE ;
                     goto error ;
                  }
               }
            }
         }

         // sanity check for garbage fields
         if ( boQuery.nFields() != expectedObjSize )
         {
            PD_LOG ( PDERROR, "Actual input doesn't match expected opt size, "
                     "there could be one or more invalid arguments" ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         // checks are done, let's insert into collection
         insertObj = ob.obj () ;
         rc = rtnInsert ( CAT_DOMAIN_COLLECTION, insertObj, 1,
                          0, _pEduCB ) ;
         if ( rc )
         {
            // if there's duplicate key exception, that means the domain is
            // already exist
            if ( SDB_IXM_DUP_KEY == rc )
            {
               PD_LOG ( PDERROR, "Domain %s is already exist",
                        pDomainName ) ;
               rc = SDB_CAT_DOMAIN_EXIST ;
               goto error ;
            }
            else
            {
               PD_LOG ( PDERROR,
                        "Failed to insert domain object into %s, rc = %d",
                        CAT_DOMAIN_COLLECTION, rc ) ;
               goto error ;
            }
         }

         PD_LOG( PDEVENT, "create domain[%s]",
                 insertObj.toString( FALSE, TRUE ).c_str() ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
   done :
      PD_TRACE_EXITRC ( SDB_CATALOGMGR_CREATEDOMAIN, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGMGR_DROPDOMAIN, "catCatalogueManager::processCmdDropDomain" )
   INT32 catCatalogueManager::processCmdDropDomain ( const CHAR *pQuery )
   {
      INT32 rc                = SDB_OK ;
      const CHAR *pDomainName = NULL ;
      INT64 numDeleted        = 0 ;
      PD_TRACE_ENTRY ( SDB_CATALOGMGR_DROPDOMAIN ) ;
      // first extract pQuery and find the options
      try
      {
         BSONObj tempObj ;
         BSONObj queryObj ;
         BSONObj resultObj ;
         BSONObj boQuery( pQuery );
         // find out the domain name
         BSONElement beDomainName = boQuery.getField( CAT_DOMAINNAME_NAME );
         PD_CHECK( beDomainName.type() == String, SDB_INVALIDARG, error,
                   PDERROR, "failed to drop domain, get field(%s) "
                   "failed!", CAT_DOMAINNAME_NAME );
         pDomainName = beDomainName.valuestr() ;
         PD_TRACE1 ( SDB_CATALOGMGR_DROPDOMAIN, PD_PACK_STRING(pDomainName) ) ;
         // validate the domain is not empty by searching SYSCOLLECTIONSPACES
         // for {Domain} field matches pDomainName
         queryObj = BSON ( CAT_DOMAIN_NAME << pDomainName ) ;
         // context will be closed when rc == 0, otherwise it should already be
         // closed in the function
         rc = catGetOneObj ( CAT_COLLECTION_SPACE_COLLECTION, tempObj,
                             queryObj, tempObj, _pEduCB, resultObj ) ;
         if ( SDB_DMS_EOC != rc )
         {
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to get object from %s, rc = %d",
                        CAT_COLLECTION_SPACE_COLLECTION, rc ) ;
               goto error ;
            }
            else
            {
               rc = SDB_DOMAIN_IS_OCCUPIED ;
               PD_LOG ( PDERROR, "There are one or more collection spaces "
                        "are using the domain, rc = %d", rc ) ;
               goto error ;
            }
         }
         // if we cannot find any record with given domain name, that's expected
         // attempt to delete from the SYSDOMAINS
         queryObj = BSON ( CAT_DOMAINNAME_NAME << pDomainName ) ;
         rc = rtnDelete ( CAT_DOMAIN_COLLECTION, queryObj,
                          tempObj, 0, _pEduCB, &numDeleted ) ;
         // if something wrong happend
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to drop domain %s, rc = %d",
                     pDomainName, rc ) ;
            goto error ;
         }
         // if delete is fine but we didn't find anything
         if ( 0 == numDeleted )
         {
            PD_LOG ( PDERROR, "Domain %s does not exist",
                     pDomainName ) ;
            rc = SDB_CAT_DOMAIN_NOT_EXIST ;
            goto error ;
         }

         PD_LOG( PDEVENT, "drop domain[%s]", pDomainName ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
   done :
      PD_TRACE_EXITRC ( SDB_CATALOGMGR_DROPDOMAIN, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGMGR_ALTERDOMAIN, "catCatalogueManager::processCmdAlterDomain" )
   INT32 catCatalogueManager::processCmdAlterDomain ( const CHAR *pQuery )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CATALOGMGR_ALTERDOMAIN ) ;
      BSONObj alterObj( pQuery ) ;
      BSONElement eleDomainName ;
      BSONObj domainObj ;
      BSONElement eleOptions ;
      BSONObjBuilder alterBuilder ;
      BSONObjBuilder reqBuilder ;
      BSONObj objReq ;
      vector< string > vecGroups ;

      /// 1. be sure that the request is legal.
      /// 2. update the record of this domain.

      eleDomainName = alterObj.getField( CAT_DOMAINNAME_NAME ) ;
      if ( String != eleDomainName.type() )
      {
         PD_LOG( PDERROR, "can not find valid [%s] in alter req [%s]",
                 CAT_DOMAINNAME_NAME, alterObj.toString().c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      eleOptions = alterObj.getField( CAT_OPTIONS_NAME ) ;
      if ( Object != eleOptions.type() )
      {
         PD_LOG( PDERROR, "can not find valid [%s] in alter req[%s]",
                 CAT_OPTIONS_NAME, alterObj.toString().c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = catGetDomainObj( eleDomainName.valuestr(),
                            domainObj,
                            _pEduCB ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to get domain[%s], rc:",
                 eleDomainName.valuestr(), rc  ) ;
         goto error ;
      }

      rc = catDomainOptionsExtract( eleOptions.embeddedObject(),
                                    _pEduCB, &reqBuilder, &vecGroups ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to validate options object:%d", rc ) ;
         goto error ;
      }

      // check group is active or not
      for ( UINT32 i = 0 ; i < vecGroups.size() ; ++i )
      {
         rc = _checkGroupStatus( vecGroups[i].c_str() ) ;
         if ( SDB_OK != rc )
         {
            if ( SDB_CLS_GRP_NOT_EXIST == rc )
            {
               PD_LOG( PDERROR, "group[%s] is not exist",
                       vecGroups[i].c_str() ) ;
            }
            else if ( SDB_REPL_GROUP_NOT_ACTIVE == rc )
            {
               PD_LOG( PDERROR, "group[%s] is inactive",
                       vecGroups[i].c_str() ) ;
            }
            goto error ;
         }
      }

      if ( _pCatCB->isImageEnabled() )
      {
         // the group that has no image can't be added to domain when
         // image is enabled
         for ( UINT32 i = 0 ; i < vecGroups.size() ; ++i )
         {
            if ( !_pCatCB->getCatDCMgr()->groupInImage( vecGroups[i] ) )
            {
               PD_LOG( PDERROR, "The group[%s] that has no image can't "
                       "be added to domain when image is enabled",
                       vecGroups[i].c_str() ) ;
               rc = SDB_CAT_GROUP_HASNOT_IMAGE ;
               goto error ;
            }
         }
      }

      objReq = reqBuilder.obj() ;

      {
         BSONElement groups = objReq.getField( CAT_GROUPS_NAME ) ;
         if ( !groups.eoo() )
         {
            rc = _buildAlterGroups( domainObj, groups, alterBuilder ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to add groups to builder:%d", rc ) ;
               goto error ;
            }
         }
      }

      {
         BSONElement autoSplit = objReq.getField( CAT_DOMAIN_AUTO_SPLIT ) ;
         if ( !autoSplit.eoo() )
         {
            alterBuilder.append( autoSplit ) ;
         }
      }

      {
         BSONElement autoRebalance = objReq.getField( CAT_DOMAIN_AUTO_REBALANCE ) ;
         if ( !autoRebalance.eoo() )
         {
            alterBuilder.append( autoRebalance ) ;
         }
      }

      {
         BSONObjBuilder matchBuilder ;
         matchBuilder.append( eleDomainName ) ;
         BSONObj alterObj = alterBuilder.obj() ;
         BSONObj dummy ;
         rc = rtnUpdate( CAT_DOMAIN_COLLECTION,
                         matchBuilder.obj(),
                         BSON( "$set" << alterObj ),
                         dummy,
                         0, _pEduCB, NULL ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to update cata info:%d", rc ) ;
            goto error ;
         }

         PD_LOG( PDEVENT, "alter domain[%s] to[%s]",
                 eleDomainName.valuestr(),
                 alterObj.toString( FALSE, TRUE ).c_str() ) ;
      }
   done :
      PD_TRACE_EXITRC ( SDB_CATALOGMGR_ALTERDOMAIN, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   static INT32 _findGroupWillBeRemoved( const map<string, UINT32> &groupsInDomain,
                                         const BSONElement &groupsInReq,
                                         map<string, UINT32> &removed )
   {
      INT32 rc = SDB_OK ;
      map<string, UINT32>::const_iterator itr = groupsInDomain.begin() ;
      for ( ; itr != groupsInDomain.end(); itr++ )
      {
         BOOLEAN found = FALSE ;
         /// ele mst be a array. we checked it in processCmdAlterDomain.
         BSONObjIterator i( groupsInReq.embeddedObject() ) ;
         while ( i.more() )
         {
            BSONElement ele = i.next() ;
            if ( Object != ele.type() )
            {
               PD_LOG( PDERROR, "invalid groups info[%s]. it should be like",
                       " {GroupID:int, GroupName:string}",
                       groupsInReq.toString().c_str() ) ;
               rc = SDB_SYS ;
               goto error ;
            }

            {
            BSONElement groupID =
                    ele.embeddedObject().getField( CAT_GROUPID_NAME ) ;
            if ( NumberInt != groupID.type() )
            {
               PD_LOG( PDERROR, "invalid groups info[%s]. it should be like",
                       " {GroupID:int, GroupName:string}",
                       groupsInReq.toString().c_str() ) ;
               rc = SDB_SYS ;
               goto error ;
            }

            if ( (UINT32)groupID.Int() == itr->second )
            {
               found = TRUE ;
               break ;
            }
            }
         }

         if ( !found )
         {
            removed.insert( std::make_pair( itr->first, itr->second ) ) ;
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGMGR__BUILDALTERGROUPS, "catCatalogueManager::_buildAlterGroups" )
   INT32 catCatalogueManager::_buildAlterGroups( const BSONObj &domain,
                                                 const BSONElement &ele,
                                                 BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATALOGMGR__BUILDALTERGROUPS ) ;
      map<string, UINT32> groupsInDomain ;
      map<string, UINT32> toBeRemoved ;
      BSONObj objToBeRemoved ;
      BSONArrayBuilder arrBuilder ;

      rc = catGetDomainGroups( domain, groupsInDomain ) ;
      if ( SDB_CAT_NO_GROUP_IN_DOMAIN == rc )
      {
         /// empty domain
         rc = SDB_OK ;
      }
      else if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to get groups from domain object:%d", rc ) ;
         goto error ;
      }

      /// be sure that no data(of this domain) on the group witch will be remove from domain.
      /// the groups will be added to domain are checked at before.
      rc = _findGroupWillBeRemoved( groupsInDomain,
                                    ele,
                                    toBeRemoved ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to get the groups those to be removed:%d", rc) ;
         goto error ;
      }

      if ( !toBeRemoved.empty() )
      {
         objToBeRemoved = arrBuilder.arr() ;
         vector< string > collectionSpaces ;

         /// Get collection spaces for domain
         rc = catGetDomainCSs ( domain, _pEduCB, collectionSpaces ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to get collection spaces for domain:%d",
                    rc ) ;
            goto error ;
         }
         for ( UINT32 i = 0 ; i < collectionSpaces.size() ; ++i )
         {
            /// For each collection space:
            /// 1. Get groups from collections
            /// 2. Get groups under splitting tasks
            vector< UINT32 > groups ;
            rc = catGetCSGroupsFromCLs( collectionSpaces[i].c_str(),
                                        _pEduCB,
                                        groups,
                                        FALSE ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Get collection space[%s] all groups failed, rc: %d",
                         collectionSpaces[i].c_str(), rc ) ;
            rc = catGetCSGroupsFromTasks( collectionSpaces[i].c_str(),
                                          _pEduCB,
                                          groups ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Get collection space[%s] all groups in task failed, "
                         "rc: %d", collectionSpaces[i].c_str(), rc ) ;
            /// Check whether the groups to be removed have data
            if ( 0 == groups.size() )
            {
               continue ;
            }
            for ( map<string, UINT32>::const_iterator itr = toBeRemoved.begin();
                  itr != toBeRemoved.end();
                  itr++ )
            {
               if ( find( groups.begin(),
                          groups.end(),
                          (INT32)itr->second ) != groups.end() )
               {
                  PD_LOG( PDERROR, "clear data(of this domain) before remove it "
                          "from domain. groups to be removed[%s]",
                          objToBeRemoved.toString( TRUE, TRUE ).c_str() ) ;
                  rc = SDB_DOMAIN_IS_OCCUPIED ;
                  goto error ;
               }
            }
         }
      }

      builder.append( ele ) ;
   done:
      PD_TRACE_EXITRC( SDB_CATALOGMGR__BUILDALTERGROUPS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGMGR__CHOOSEFGROUPOFCL, "catCatalogueManager::_chooseGroupOfCl" )
   INT32 catCatalogueManager::_chooseGroupOfCl( const BSONObj &domainObj,
                                                const BSONObj &csObj,
                                                const catCollectionInfo &clInfo,
                                                std::string &groupName,
                                                UINT32 &groupID,
                                                std::map<string, UINT32> &splitRange )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATALOGMGR__CHOOSEFGROUPOFCL ) ;
      BSONObj gpObj ;
      const CHAR *domain = NULL ;
      BOOLEAN isSysDomain = domainObj.isEmpty() ;
      std::map<string, UINT32> groupsOfDomain ;

      /// if the group is specified.
      /// 1) whether the group exsits.
      /// 2) whether the group is one of the groups of domain.
      if ( NULL != clInfo._gpSpecified )
      {
         if ( !isSysDomain )
         {
            rc = catGetDomainGroups( domainObj, groupsOfDomain ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get groups from domain "
                         "info[%s], rc: %d", domainObj.toString().c_str(),
                         rc ) ;
            {
               map<string, UINT32>::const_iterator itr =
                  groupsOfDomain.find( clInfo._gpSpecified ) ;
               if ( groupsOfDomain.end() == itr )
               {
                  PD_LOG( PDERROR, "[%s] is not a group of domain [%s]",
                          clInfo._gpSpecified, domain ) ;
                  rc = SDB_CAT_GROUP_NOT_IN_DOMAIN ;
                  goto error ;
               }

               groupID = itr->second ;
            }
            /// if the group is a group of domain, it surely exsits.
         }
         else
         {
            INT32 tmpGrpID = CAT_INVALID_GROUPID ;
            rc = catGetGroupObj( clInfo._gpSpecified, TRUE, gpObj, _pEduCB ) ;
            PD_RC_CHECK( rc, PDERROR, "Get group[%s] info failed, rc: %d",
                         clInfo._gpSpecified, rc ) ;
            rc = rtnGetIntElement( gpObj, CAT_GROUPID_NAME, tmpGrpID ) ;
            PD_RC_CHECK( rc, PDERROR, "Get groupid of group[%s] info failed, "
                         "rc: %d", clInfo._gpSpecified, rc ) ;
            groupID = tmpGrpID ;
         }

         groupName.assign( clInfo._gpSpecified ) ;

         if ( _pCatCB->isImageEnabled() &&
              !_pCatCB->getCatDCMgr()->groupInImage( groupName ) )
         {
            // the group that has no image can't be as the collection location
            PD_LOG( PDERROR, "The group[%s] that has no image can't "
                    "be as the collection's location when image is enabled",
                    groupName.c_str() ) ;
            rc = SDB_CAT_GROUP_HASNOT_IMAGE ;
            goto error ;
         }
      }
      /// get a group from groups of cs.
      else
      {
         vector< UINT32 > vecGroupID ;

         if ( ASSIGN_FOLLOW == clInfo._assignType )
         {
            BSONElement ele = csObj.getField( CAT_COLLECTION_SPACE_NAME ) ;
            rc = catGetCSGroupsFromCLs( ele.valuestrsafe(), _pEduCB,
                                        vecGroupID ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Get collection space[%s] all groups failed, "
                       "rc: %d", csObj.toString().c_str(), rc ) ;
               goto error ;
            }
         }

         if ( 0 == vecGroupID.size() && !isSysDomain )
         {
            rc = catGetDomainGroups( domainObj, vecGroupID ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Get groups from domain obj[%s] failed, "
                       "rc: %d", domainObj.toString().c_str(), rc ) ;
               goto error ;
            }
         }

         rc = _assignGroup( &vecGroupID, groupID ) ;
         PD_RC_CHECK( rc, PDERROR, "Assign group for collection[%s] "
                      "failed, rc: %d", clInfo._pCLName, rc ) ;

         rc = catGroupID2Name( groupID, groupName, _pEduCB ) ;
         PD_RC_CHECK( rc, PDERROR, "Group id[%d] to group name failed, "
                      "rc: %d", groupID, rc ) ;

         if ( !isSysDomain )
         {
            rc = catGetDomainGroups( domainObj, splitRange ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get groups from domain "
                         "info[%s], rc: %d", domainObj.toString().c_str(),
                         rc ) ;
         }
      }

      SDB_ASSERT( CAT_INVALID_GROUPID != groupID, "can not be invalid" ) ;
   done:
      PD_TRACE_EXITRC( SDB_CATALOGMGR__CHOOSEFGROUPOFCL, rc ) ;
      return rc ;
   error:
      groupID = CAT_INVALID_GROUPID ;
      groupName.clear() ;
      splitRange.clear() ;
      goto done ;
   }

   static INT32 getBoundFromClObj( const BSONObj &clObj,
                                   UINT32 &totalBound )
   {
      INT32 rc = SDB_OK ;
      BSONElement upBound =
            clObj.getFieldDotted(CAT_CATALOGINFO_NAME".0."CAT_UPBOUND_NAME);
      if ( Object != upBound.type() )
      {
         rc = SDB_SYS ;
         goto error ;
      }

      {
      BSONElement first = upBound.embeddedObject().firstElement() ;
      if ( NumberInt != first.type() )
      {
         rc = SDB_SYS ;
         goto error ;
      }

      totalBound = first.Int() ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGMGR_AUTOHASHSPLIT, "catCatalogueManager::_autoHashSplit" )
   INT32 catCatalogueManager::_autoHashSplit( const BSONObj &clObj,
                                              std::vector<UINT64> &taskIDs,
                                              const CHAR *srcGroupName,
                                              const map<string, UINT32> *range )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATALOGMGR_AUTOHASHSPLIT ) ;
      const CHAR *srcGroup = NULL ;
      const map<string, UINT32> *dstGroups = NULL ;
      BSONObj splitInfo ;
      const CHAR *fullName = NULL ;
      UINT32 totalBound = 0 ;


      BSONElement eleName = clObj.getField( CAT_CATALOGNAME_NAME ) ;
      if ( String != eleName.type() )
      {
         SDB_ASSERT( FALSE, "impossible" ) ;
         PD_LOG( PDERROR, "invalid collection record:%s",
                 clObj.toString().c_str() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      fullName = eleName.valuestr() ;

      rc = getBoundFromClObj( clObj, totalBound ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to get bound from cl obj[%s]",
                 clObj.toString().c_str() ) ;
         rc = SDB_SYS ;
         goto error ;
      }


      if ( NULL == srcGroupName )
      {
         /// TODO: get src and dst id from meta data.
         SDB_ASSERT( FALSE, "impossible" ) ;
      }
      else
      {
         SDB_ASSERT( NULL != range, "can not be NULL" ) ;
         srcGroup = srcGroupName ;
         dstGroups = range ;
      }

      if ( 1 < dstGroups->size() )
      {
         UINT32 tmpID = CAT_INVALID_GROUPID ;
         UINT64 taskID = CLS_INVALID_TASKID ;
         clsCatalogSet catSet( fullName ) ;
         UINT32 avgBound = totalBound / dstGroups->size() ;
         UINT32 endBound = totalBound ;
         UINT32 beginBound = totalBound - avgBound ;

         rc = catSet.updateCatSet( clObj ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to update catlogset:%d", rc ) ;
            goto error ;
         }

         {
         map<string, UINT32>::const_iterator itr = dstGroups->begin() ;
         for ( ; itr != dstGroups->end(); itr++ )
         {
            if ( 0 == ossStrcmp( srcGroup, itr->first.c_str() ) )
            {
               continue ;
            }

            splitInfo = _crtSplitInfo( fullName,
                                       srcGroup,
                                       itr->first.c_str(),
                                       beginBound,
                                       endBound ) ;

            rc = catSplitReady( splitInfo, fullName,
                                &catSet, tmpID, _taskMgr,
                                _pEduCB, _majoritySize(),
                                &taskID ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to split collections[%s], rc:%d",
                       fullName, rc ) ;
               goto error ;
            }

            endBound = beginBound ;
            beginBound = endBound - avgBound ;
            taskIDs.push_back( taskID ) ;
         }
         }
      }
      else
      {
         PD_LOG( PDINFO, "split range size:%d, do nothing.", dstGroups->size() ) ;
      }
   done:
      PD_TRACE_EXITRC( SDB_CATALOGMGR_AUTOHASHSPLIT, rc ) ;
      return rc ;
   error:
      {
      std::vector<UINT64>::const_iterator itr = taskIDs.begin() ;
      for ( ; itr != taskIDs.end(); itr++ )
      {
         catRemoveTask( *itr, _pEduCB, _majoritySize() ) ;
      }

      taskIDs.clear() ;
      }
      goto done ;
   }

   BSONObj catCatalogueManager::_crtSplitInfo( const CHAR *fullName,
                                               const CHAR *src,
                                               const CHAR *dst,
                                               UINT32 begin,
                                               UINT32 end )
   {
      SDB_ASSERT( NULL != fullName && NULL != src && NULL != dst,
                  "can not be NULL" ) ;
      BSONObj obj = BSON ( CAT_COLLECTION_NAME << fullName <<
                           CAT_SOURCE_NAME << src <<
                           CAT_TARGET_NAME << dst <<
                           CAT_SPLITVALUE_NAME << BSON( "" << begin ) <<
                           CAT_SPLITENDVALUE_NAME << BSON( "" << end ) ) ;
      return obj ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGMGR__COMBINEOPTIONS, "catCatalogueManager::_combineOptions" )
   INT32 catCatalogueManager::_combineOptions( const BSONObj &domain,
                                               const BSONObj &cs,
                                               UINT32 &mask,
                                               catCollectionInfo &options  )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATALOGMGR__COMBINEOPTIONS ) ;
      /// it is a sysdomain.
      if ( domain.isEmpty() )
      {
         goto done ;
      }

      if ( !( CAT_MASK_AUTOASPLIT & mask ) )
      {
         if ( options._isSharding && options._isHash )
         {
            BSONElement split = domain.getField( CAT_DOMAIN_AUTO_SPLIT ) ;
            if ( Bool == split.type() )
            {
               options._autoSplit = split.Bool() ;
               mask |= CAT_MASK_AUTOASPLIT ;
            }
         }
      }

      if ( !( CAT_MASK_AUTOREBALAN & mask ) )
      {
         if ( options._isSharding && options._isHash )
         {
            BSONElement rebalance = domain.getField( CAT_DOMAIN_AUTO_REBALANCE ) ;
            if ( Bool == rebalance.type() )
            {
               options._autoRebalance = rebalance.Bool() ;
               mask |= CAT_MASK_AUTOREBALAN ;
            }
         }
      }
   done:
      PD_TRACE_EXITRC( SDB_CATALOGMGR__COMBINEOPTIONS, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGMGR__BUILDALTEROBJWITHMETAANDOBJ, "catCatalogueManager::_buildAlterObjWithMetaAndObj" )
   INT32 catCatalogueManager::_buildAlterObjWithMetaAndObj( _clsCatalogSet &catSet,
                                                            UINT32 mask,
                                                            catCollectionInfo &alterInfo,
                                                            BSONObj &alterObj )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY(  SDB_CATALOGMGR__BUILDALTEROBJWITHMETAANDOBJ) ;
      BSONElement groupID ;
      BSONElement groupName ;
      BSONObj groupObj ;
      UINT32 attribute = 0 ;
      _clsCatalogSet::POSITION pos ;
      clsCatalogItem *item = NULL ;

      if ( ( CAT_MASK_SHDKEY & mask ) &&
           catSet.isSharding() )
      {
         rc = SDB_OPTION_NOT_SUPPORT ;
         PD_LOG( PDERROR, "can not alter a sharding collection's shardingkey" ) ;
         goto error ;
      }

      if ( CAT_MASK_ISMAINCL & mask )
      {
         rc = SDB_OPTION_NOT_SUPPORT ;
         PD_LOG( PDERROR, "can not change a collection to a main cl" ) ;
         goto error ;
      }

     if ( CAT_MASK_COMPRESSED & mask )
     {
        rc = SDB_OPTION_NOT_SUPPORT ;
        PD_LOG( PDERROR, "can not alter attribute \"Compressed\"" ) ;
        goto error ;
     }

      if ( CAT_MASK_CLNAME & mask )
      {
         rc = SDB_OPTION_NOT_SUPPORT ;
         PD_LOG( PDERROR, "can not alter attribute \"Name\"" ) ;
         goto error ;
      }

      alterInfo._version = catSet.getVersion() ;
      ++alterInfo._version ;

      if ( catSet.isSharding() || catSet.isMainCL() )
      {
         /// this is a splited collection or a main cl. we can only change replsize or auto rebalance.
         BSONObjBuilder builder ;
         builder.append( CAT_CATALOGVERSION_NAME, alterInfo._version ) ;
         if ( mask & CAT_MASK_REPLSIZE )
         {
            builder.append( CAT_CATALOG_W_NAME, alterInfo._replSize ) ;
         }
         if ( mask & CAT_MASK_AUTOREBALAN )
         {
            builder.appendBool( CAT_DOMAIN_AUTO_REBALANCE,
                                alterInfo._autoRebalance ) ;
         }

         alterObj = builder.obj() ;
         goto done ;
      }

      pos = catSet.getFirstItem() ;
      item = catSet.getNextItem( pos ) ;
      if ( NULL == item )
      {
         PD_LOG( PDERROR, "failed to get first item from catalogset" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      attribute = catSet.getAttribute() ;

      rc = _buildCatalogRecord( alterInfo, mask, attribute, item->getGroupID(),
                                item->getGroupName().c_str(),
                                alterObj ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to build cata record:%d", rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB_CATALOGMGR__BUILDALTEROBJWITHMETAANDOBJ, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGMGR__GETGROUPSOFCOLLECTIONS, "catCatalogueManager::_getGroupsOfCollections" )
   INT32 catCatalogueManager::_getGroupsOfCollections(
                              const std::vector<string> &clNames,
                              BSONObj &groups )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATALOGMGR__GETGROUPSOFCOLLECTIONS ) ;
      BSONArrayBuilder builder ;
      set<INT32> pushed ;
      vector<string>::const_iterator itr = clNames.begin() ;
      for ( ; itr != clNames.end(); ++itr )
      {
         BSONObj clInfo ;
         BOOLEAN exist = FALSE ;
         BSONElement cataInfo ;
         rc = catCheckCollectionExist( itr->c_str(),
                                       exist,
                                       clInfo,
                                       _pEduCB ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to detect collection[%s]"
                    ", rc:%d", itr->c_str(), rc ) ;
            goto error ;
         }

         if ( !exist )
         {
            PD_LOG( PDERROR, "collection[%s] does not exist" ) ;
            rc = SDB_DMS_NOTEXIST ;
            goto error ;
         }

         cataInfo = clInfo.getField( CAT_CATALOGINFO_NAME ) ;
         if ( Array != cataInfo.type() )
         {
            PD_LOG( PDERROR, "invalid cl info:%s",
                    clInfo.toString( FALSE, FALSE ).c_str()) ;
            rc = SDB_SYS ;
            goto error ;
         }

         {
         BSONObjIterator i( cataInfo.embeddedObject() ) ;
         while ( i.more() )
         {
            BSONElement ele = i.next() ;
            if ( Object == ele.type() )
            {
               BSONElement groupID = ele.embeddedObject().getField( CAT_GROUPID_NAME ) ;
               if ( NumberInt == groupID.type() )
               {
                  if ( pushed.find( groupID.Int() ) == pushed.end() )
                  {
                     builder << ele ;
                     pushed.insert( groupID.Int() ) ;
                  }
               }
            }
         }
         }
      }

      groups = BSON( CAT_GROUP_NAME << builder.arr() ) ;
   done:
      PD_TRACE_EXITRC( SDB_CATALOGMGR__GETGROUPSOFCOLLECTIONS, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}

