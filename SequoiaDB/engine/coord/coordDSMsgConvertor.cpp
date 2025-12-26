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

   Source File Name = coordDSMsgConvertor.cpp

   Descriptive Name = Data source message convertor.

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains Runtime code for insert
   request.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/08/2020  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#include "coordDSMsgConvertor.hpp"
#include "coordTrace.hpp"
#include "msgMessage.hpp"
#include "pmdEDU.hpp"
#include "coordCB.hpp"
#include "coordResource.hpp"
#include "rtn.hpp"
#include "rtnContextDataDispatcher.hpp"

#include "../bson/lib/md5.hpp"
#include "../bson/bson.h"

using namespace bson ;

namespace engine
{

   _coordDSMsgConvertor::_coordDSMsgConvertor()
   {
   }

   _coordDSMsgConvertor::~_coordDSMsgConvertor()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__COORDDSMSGCONVERTOR__CMDSHOULDFILTEROUT, "_coordDSMsgConvertor::_cmdShouldFilterOut" )
   INT32 _coordDSMsgConvertor::_cmdShouldFilterOut( _pmdSubSession *pSub,
                                                    const MsgOpQuery *pQuery,
                                                    pmdEDUCB *cb,
                                                    BOOLEAN &ignore,
                                                    UINT32 &oprMask ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__COORDDSMSGCONVERTOR__CMDSHOULDFILTEROUT ) ;
      _rtnCommand *pCommand = NULL ;

      INT32 flag = 0 ;
      SINT64 numToSkip = 0 ;
      SINT64 numToReturn = 0 ;
      const CHAR *pCollectionName = NULL ;
      const CHAR *pMatcherBuf = NULL ;
      const CHAR *pSelectBuff = NULL ;
      const CHAR *pOrderyBuff = NULL ;
      const CHAR *pHintBuff = NULL ;

      rc = msgExtractQuery( (const CHAR*)pQuery, &flag, &pCollectionName,
                            &numToSkip, &numToReturn,
                            &pMatcherBuf, &pSelectBuff,
                            &pOrderyBuff, &pHintBuff ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Extrace command message failed, rc: %d", rc ) ;
         goto error ;
      }

      rc = rtnParserCommand( pCollectionName, &pCommand ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Parse command failed, rc: %d", rc ) ;
         goto error ;
      }

      rc = rtnInitCommand( pCommand, flag, numToSkip, numToReturn,
                           pMatcherBuf, pSelectBuff,
                           pOrderyBuff, pHintBuff ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Init command failed, rc: %d", rc ) ;
      }

      if ( pCommand->writable() )
      {
         SDB_SET_WRITE( oprMask ) ;
      }
      else
      {
         SDB_SET_READ( oprMask ) ;
      }

      // For internal operation and supported commands, they should be sent to
      // data source to process.
      if ( _isInternalOperation( pCommand->type() ) )
      {
         ignore = FALSE ;
         // Internal operations are not affected by configuration 'AccessMode'.
         oprMask = 0 ;
         goto done ;
      }
      else if ( CMD_GET_COUNT == pCommand->type() ||
                CMD_TRUNCATE == pCommand->type() ||
                CMD_LIST_LOB == pCommand->type() ||
                CMD_GET_CL_DETAIL == pCommand->type() )
      {
         ignore = FALSE ;
         goto done ;
      }
      else if ( coordIsLocalMappingCmd( pCommand ) )
      {
         ignore = TRUE ;
         goto done ;
      }

      /// if it is data operator, need to check wether report error or not
      /// according to the error control level.
      if ( ( NULL != pCommand->collectionFullName() ||
             NULL != pCommand->spaceName() ) )
      {
         CoordDataSourcePtr dsPtr ;
         const CHAR *errCtlLevel = NULL ;
         coordDataSourceMgr *pDSMgr = sdbGetCoordCB()->getDSManager() ;
         UTIL_DS_UID dsID =
               SDB_GROUPID_2_DSID( pSub->getNodeID().columns.groupID ) ;
         rc = pDSMgr->getOrUpdateDataSource( dsID, dsPtr, cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Get data source[%u] failed, rc: %d",
                    dsID, rc ) ;
            goto error ;
         }

         errCtlLevel = dsPtr->getErrCtlLevel() ;
         if ( 0 == ossStrcmp( errCtlLevel, VALUE_NAME_HIGH ) )
         {
            rc = SDB_OPERATION_INCOMPATIBLE ;
            PD_LOG_MSG( PDERROR, "The command cannot be done on object on data "
                                 "source[%s]", dsPtr->getName() ) ;
            goto error ;
         }
      }
      ignore = TRUE ;

   done:
      if ( pCommand )
      {
         rtnReleaseCommand( &pCommand ) ;
      }
      PD_TRACE_EXITRC( SDB__COORDDSMSGCONVERTOR__CMDSHOULDFILTEROUT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__COORDDSMSGCONVERTOR__LOBSHOULDFILTEROUT, "_coordDSMsgConvertor::_lobShouldFilterOut" )
   INT32 _coordDSMsgConvertor::_lobShouldFilterOut( const MsgHeader *msg,
                                                    const MsgHeader *pOrgMsg,
                                                    BOOLEAN &ignore,
                                                    UINT32 &oprMask ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__COORDDSMSGCONVERTOR__LOBSHOULDFILTEROUT ) ;

      switch( pOrgMsg->opCode )
      {
         case MSG_BS_LOB_OPEN_REQ :
         {
            const MsgOpLob *pLobHeader = NULL ;
            BSONObj obj ;
            rc = msgExtractOpenLobRequest( (const CHAR*)pOrgMsg,
                                           &pLobHeader, obj ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Extract message failed, rc: %d", rc ) ;
               goto error ;
            }

            try
            {
               BSONElement mode = obj.getField( FIELD_NAME_LOB_OPEN_MODE ) ;
               if ( NumberInt != mode.type() )
               {
                  PD_LOG( PDERROR, "Invalid mode in meta" ) ;
                  rc = SDB_INVALIDARG ;
                  goto error ;
               }

               if ( SDB_LOB_MODE_READ != mode.numberInt() )
               {
                  SDB_SET_WRITE( oprMask ) ;
               }
               else
               {
                  SDB_SET_READ( oprMask ) ;
               }
            }
            catch( std::exception &e )
            {
               rc = ossException2RC( &e ) ;
               PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
               goto error ;
            }
            break ;
         }
         case MSG_BS_LOB_WRITE_REQ :
         case MSG_BS_LOB_REMOVE_REQ :
         case MSG_BS_LOB_UPDATE_REQ :
         case MSG_BS_LOB_TRUNCATE_REQ :
            SDB_SET_WRITE( oprMask ) ;
            break ;
         default :
            SDB_SET_READ( oprMask ) ;
            break ;
      }

      ignore = FALSE ;

      if ( pOrgMsg )
      {
         if ( ( MSG_BS_LOB_WRITE_REQ == msg->opCode ||
                MSG_BS_LOB_UPDATE_REQ == msg->opCode ) &&
              MSG_BS_LOB_CLOSE_REQ == pOrgMsg->opCode )
         {
            // it is write meta data of close LOB command
            // ignore it, since we never write meta data to data source
            ignore = TRUE ;
         }
         else if ( MSG_BS_LOB_UPDATE_REQ == msg->opCode )
         {
            // coord can not process update LOB request
            // it is converted from other request, in convertor, which
            // will be converted back to origin inpput request
            SDB_ASSERT( MSG_BS_LOB_UPDATE_REQ != pOrgMsg->opCode,
                        "Coord can not process update LOB request" ) ;
            if ( MSG_BS_LOB_UPDATE_REQ == pOrgMsg->opCode )
            {
               ignore = TRUE ;
            }
         }
         else if ( ( MSG_BS_LOB_CLOSE_REQ == msg->opCode ||
                     MSG_BS_LOB_WRITE_REQ == msg->opCode ||
                     MSG_BS_LOB_UPDATE_REQ == msg->opCode ||
                     MSG_BS_LOB_REMOVE_REQ == msg->opCode ) &&
                   ( MSG_BS_LOB_REMOVE_REQ == pOrgMsg->opCode ||
                     MSG_BS_LOB_TRUNCATE_REQ == pOrgMsg->opCode ) )
         {
            // for truncate and remove, we only convert open LOB message
            // to origin message, other messages will be ignored
            ignore = TRUE ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__COORDDSMSGCONVERTOR__LOBSHOULDFILTEROUT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__COORDDSMSGCONVERTOR_FILTER, "_coordDSMsgConvertor::filter" )
   INT32 _coordDSMsgConvertor::filter( _pmdSubSession *pSub,
                                       _pmdEDUCB * cb,
                                       BOOLEAN &ignore,
                                       UINT32 &oprMask )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__COORDDSMSGCONVERTOR_FILTER ) ;
      MsgHeader *msg = pSub->getReqMsg() ;

      SDB_ASSERT( msg, "msg can't be null" ) ;

      ignore = FALSE ;
      oprMask = 0 ;

      if ( !pSub->getConnection()->isExtern() )
      {
         /// do nothing
         goto done ;
      }

      if ( coordIsSpecialMsg( msg->opCode ) )
      {
         SDB_SET_READ( oprMask ) ;
         goto done ;
      }
      else if ( coordIsLobMsg( msg->opCode ) )
      {
         const MsgHeader *pOrgMsg = NULL ;
         ISession *pSession = NULL ;
         IClient *pClient = NULL ;

         if ( cb )
         {
            pSession = cb->getSession() ;
            if ( pSession )
            {
               pClient = pSession->getClient() ;
               if ( pClient )
               {
                  pOrgMsg = pClient->getInMsg() ;

                  rc = _lobShouldFilterOut( msg, pOrgMsg, ignore, oprMask ) ;
                  if ( rc )
                  {
                     goto error ;
                  }
               }
            }
         }
      }
      else
      {
         switch ( msg->opCode )
         {
            case MSG_BS_QUERY_REQ:
            {
               MsgOpQuery *pQuery = ( MsgOpQuery* )msg ;
               if ( '$' == pQuery->name[0] )
               {
                  rc = _cmdShouldFilterOut( pSub, pQuery, cb,
                                            ignore, oprMask ) ;
                  if ( rc )
                  {
                     goto error ;
                  }
               }
               else
               {
                  SDB_SET_READ( oprMask ) ;

                  if ( pQuery->flags & FLG_QUERY_MODIFY )
                  {
                     SDB_SET_WRITE( oprMask ) ;
                  }
               }
               break ;
            }
            case MSG_BS_INSERT_REQ:
            case MSG_BS_DELETE_REQ:
            case MSG_BS_UPDATE_REQ:
            // Transaction permission check will be done in function
            // 'checkPermission'.
            case MSG_BS_TRANS_INSERT_REQ:
            case MSG_BS_TRANS_UPDATE_REQ:
            case MSG_BS_TRANS_DELETE_REQ:
               SDB_SET_WRITE( oprMask ) ;
               break ;
            case MSG_BS_GETMORE_REQ:
            case MSG_BS_ADVANCE_REQ:
            case MSG_BS_TRANS_QUERY_REQ:
               SDB_SET_READ( oprMask ) ;
               break ;
            default:
               SDB_SET_READ( oprMask ) ;
               ignore = TRUE ;
               break ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__COORDDSMSGCONVERTOR_FILTER, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__COORDDSMSGCONVERTOR_CHECKPERMISSION, "_coordDSMsgConvertor::checkPermission" )
   INT32 _coordDSMsgConvertor::checkPermission( _pmdSubSession *pSub,
                                                UINT32 oprMask,
                                                _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__COORDDSMSGCONVERTOR_CHECKPERMISSION ) ;
      MsgHeader *pMsg = pSub->getReqMsg() ;
      UTIL_DS_UID dsID = UTIL_INVALID_DS_UID ;
      CoordDataSourcePtr dsPtr ;
      coordDataSourceMgr *pDSMgr = sdbGetCoordCB()->getDSManager() ;

      if ( !pSub->getConnection()->isExtern() )
      {
         goto done ;
      }

      if ( coordIsSpecialMsg( pMsg->opCode ) )
      {
         goto done ;
      }

      dsID = SDB_GROUPID_2_DSID( pSub->getNodeID().columns.groupID ) ;

      rc = pDSMgr->getOrUpdateDataSource( dsID, dsPtr, cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Get data source[%u] failed, rc: %d",
                 dsID, rc ) ;
         goto error ;
      }

      // In case of autocommit on, the beginning of transaction may be pushed
      // down to data node. No valid transaction id will be set in eduCB on
      // coordinator, but the message will be transaction operation message.
      if ( ( cb->isTransaction() || isTransBSMsg( pMsg->opCode) )
           && ( DS_TRANS_PROPAGATE_NEVER == dsPtr->getTransPropagateMode() ) )
      {
         rc = SDB_COORD_DATASOURCE_TRANS_FORBIDDEN ;
         PD_LOG( PDERROR, "Transaction is not allowed on data source, rc: %d",
                 rc ) ;
         goto error ;
      }

      if ( !dsPtr->isReadable() && SDB_IS_READ( oprMask ) )
      {
         rc = SDB_COORD_DATASOURCE_PERM_DENIED ;
         goto error ;
      }
      else if ( !dsPtr->isWritable() && SDB_IS_WRITE( oprMask ) )
      {
         rc = SDB_COORD_DATASOURCE_PERM_DENIED ;
         goto error ;
      }
      else if ( !dsPtr->isReadable() && !dsPtr->isWritable() )
      {
         rc = SDB_COORD_DATASOURCE_PERM_DENIED ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__COORDDSMSGCONVERTOR_CHECKPERMISSION, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__COORDDSMSGCONVERTOR__BUILDEMPTYLOBMETA, "_coordDSMsgConvertor::_buildEmptyLobMeta" )
   INT32 _coordDSMsgConvertor::_buildEmptyLobMeta( BSONObj &metaObj ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__COORDDSMSGCONVERTOR__BUILDEMPTYLOBMETA ) ;

      try
      {
         BSONObjBuilder builder( 100 ) ;

         builder.append( FIELD_NAME_LOB_SIZE, (INT64)0 ) ;
         builder.append( FIELD_NAME_LOB_CREATETIME,
                         (INT64)ossGetCurrentMicroseconds() ) ;
         builder.append( FIELD_NAME_VERSION,
                         (INT32)DMS_LOB_META_MERGE_DATA_VERSION - 1 ) ;
         builder.append( FIELD_NAME_LOB_PAGE_SIZE,
                         (INT32)DMS_DEFAULT_LOB_PAGE_SZ ) ;
         metaObj = builder.obj() ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__COORDDSMSGCONVERTOR__BUILDEMPTYLOBMETA, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__COORDDSMSGCONVERTOR__BUILDLOWVERLOBMETA, "_coordDSMsgConvertor::_buildLowVerLobMeta" )
   INT32 _coordDSMsgConvertor::_buildLowVerLobMeta( const CHAR *pOrgMeta,
                                                    BSONObj &newMeta ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__COORDDSMSGCONVERTOR__BUILDLOWVERLOBMETA ) ;

      try
      {
         BSONObj orgMeta( pOrgMeta ) ;
         BSONObjIterator itr( orgMeta ) ;
         BSONObjBuilder builder( orgMeta.objsize() ) ;

         while( itr.more() )
         {
            BSONElement e = itr.next() ;
            if ( 0 != ossStrcmp( e.fieldName(), FIELD_NAME_VERSION ) )
            {
               builder.append( e ) ;
            }
         }

         builder.append( FIELD_NAME_VERSION,
                         (INT32)DMS_LOB_META_MERGE_DATA_VERSION - 1 ) ;

         newMeta = builder.obj() ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__COORDDSMSGCONVERTOR__BUILDLOWVERLOBMETA, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__COORDDSMSGCONVERTOR__BUILDNEWEVENT, "_coordDSMsgConvertor::_buildNewEvent" )
   INT32 _coordDSMsgConvertor::_buildNewEvent( const MsgOpReply *pOld,
                                               const BSONObj &meta,
                                               const pmdEDUEvent &orgEvent,
                                               pmdEDUEvent &event )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__COORDDSMSGCONVERTOR__BUILDNEWEVENT ) ;
      UINT32 newSize = 0 ;
      CHAR *pNewMsg = NULL ;

      newSize = sizeof( MsgOpReply ) + meta.objsize() ;
      pNewMsg = (CHAR*)SDB_THREAD_ALLOC( newSize ) ;
      if ( !pNewMsg )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Allocate memory[%u] failed", newSize ) ;
         goto error ;
      }

      /// copy header
      ossMemcpy( pNewMsg, (void*)pOld, sizeof( MsgOpReply ) ) ;
      /// copy meta
      ossMemcpy( pNewMsg + sizeof( MsgOpReply ),
                 meta.objdata(), meta.objsize() ) ;
      /// change length
      ((MsgHeader*)pNewMsg)->messageLength = newSize ;

      event._Data = pNewMsg ;
      event._eventType = orgEvent._eventType ;
      event._dataMemType = PMD_EDU_MEM_THREAD ;

   done:
      PD_TRACE_EXITRC( SDB__COORDDSMSGCONVERTOR__BUILDNEWEVENT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__COORDDSMSGCONVERTOR__BUILDSUBCLINFOHINT, "_coordDSMsgConvertor::_buildSubCLInfoHint" )
   INT32 _coordDSMsgConvertor::_buildSubCLInfoHint( const ossPoolVector< const CHAR * > &subCLNameList,
                                                    BSONObjBuilder &builder,
                                                    pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__COORDDSMSGCONVERTOR__BUILDSUBCLINFOHINT ) ;
      try
      {
         BSONArrayBuilder arrayBuilder( builder.subarrayStart( FIELD_NAME_SUB_COLLECTIONS ) ) ;
         for ( UINT32 i = 0 ; i < subCLNameList.size() ; ++i )
         {
            BSONObjBuilder subBuilder( arrayBuilder.subobjStart() ) ;
            coordResource *pResource = sdbGetCoordCB()->getResource() ;
            const CHAR *subCLName = subCLNameList.at( i ) ;
            string mappingName ;
            CoordCataInfoPtr subCataInfo ;
            string mainCLName ;
            CoordCataInfoPtr mainCataInfo ;
            BSONObj lowBound ;
            BSONObj upBound ;

            rc = pResource->getOrUpdateCataInfo( subCLName, subCataInfo, cb ) ;
            PD_RC_CHECK( rc, PDERROR, "Get catalogue information of "
                        "collection[%s] failed[%d]", subCLName, rc ) ;

            mainCLName = subCataInfo->getCatalogSet()->getMainCLName() ;
            rc = pResource->getOrUpdateCataInfo( mainCLName.c_str(),
                                                 mainCataInfo, cb ) ;
            PD_RC_CHECK( rc, PDERROR, "Get catalogue information of "
                        "collection[%s] failed[%d]", mainCLName.c_str(), rc ) ;
            rc = mainCataInfo->getCatalogSet()->getSubCLBounds( subCLName,
                                                                lowBound,
                                                                upBound ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to get sub collection[%s] bounds, rc: %d",
                         subCLName, rc ) ;

            if ( subCataInfo->getMappingName().size() > 0 )
            {
               mappingName = subCataInfo->getMappingName() ;
               subCLName = mappingName.c_str() ;
            }
            subBuilder.append( FIELD_NAME_NAME, subCLName ) ;
            subBuilder.append( FIELD_NAME_LOWBOUND, lowBound ) ;
            subBuilder.append( FIELD_NAME_UPBOUND, upBound ) ;
            subBuilder.done() ;
         }
         arrayBuilder.done() ;
      }
      catch ( exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Exception occurs: %s", e.what() ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__COORDDSMSGCONVERTOR__BUILDSUBCLINFOHINT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__COORDDSMSGCONVERTOR__CONVERTGETDETAILEVENT, "_coordDSMsgConvertor::_convertGetDetailEvent" )
   INT32 _coordDSMsgConvertor::_convertGetDetailEvent( const MsgOpReply *pOld,
                                                       const pmdEDUEvent &orgEvent,
                                                       pmdEDUEvent &event,
                                                       BOOLEAN &hasConvert )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__COORDDSMSGCONVERTOR__CONVERTGETDETAILEVENT ) ;
      UINT32 newSize = 0 ;
      CHAR *pNewMsg = NULL ;

      INT32 flag = SDB_OK ;
      INT64 contextID = -1 ;
      INT32 startFrom = 0 ;
      INT32 numReturned = 0 ;
      vector<BSONObj> resultSet ;
      INT32 offset = 0 ;

      try
      {
         hasConvert = FALSE ;

         rc = msgExtractReply( (CHAR *)pOld, &flag, &contextID, &startFrom,
                               &numReturned, resultSet ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to extract reply msg, rc = %d", rc ) ;

         if ( 0 == numReturned )
         {
            goto done ;
         }

         newSize = pOld->header.messageLength +
               ( numReturned * ossStrlen( VALUE_NAME_DATASOURCE_GROUPNAME ) ) ;
         pNewMsg = (CHAR*)SDB_THREAD_ALLOC( newSize ) ;
         if ( !pNewMsg )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Allocate memory[%u] failed", newSize ) ;
            goto error ;
         }
         ossMemset( pNewMsg, 0, newSize ) ;

         /// copy header
         ossMemcpy( pNewMsg, (void*)pOld, sizeof( MsgOpReply ) ) ;
         offset = ossAlign4 ( sizeof ( MsgOpReply ) ) ;

         /// convert GroupName to "$null"
         for ( UINT32 i = 0 ; i < resultSet.size() ; ++i )
         {
            BSONObjBuilder builder ;
            BSONObjIterator it1( resultSet[i] ) ;
            while ( it1.more() )
            {
               BSONElement elem1 = it1.next() ;
               if ( 0 != ossStrcmp( elem1.fieldName(), FIELD_NAME_DETAILS ) )
               {
                  builder.append( elem1 ) ;
                  continue ;
               }
               vector< BSONElement > detailsArray = elem1.Array() ;
               BSONArrayBuilder subBuilder( builder.subarrayStart( FIELD_NAME_DETAILS ) ) ;
               for ( UINT32 j = 0 ; j < detailsArray.size() ; ++j )
               {
                  BSONObjBuilder subBuilder2( subBuilder.subobjStart() ) ;
                  BSONObj detail = detailsArray[j].Obj() ;
                  BSONObjIterator it2( detail ) ;
                  while ( it2.more() )
                  {
                     BSONElement elem2 = it2.next() ;
                     if ( 0 == ossStrcmp( elem2.fieldName(), FIELD_NAME_GROUPNAME ) )
                     {
                        subBuilder2.append( FIELD_NAME_GROUPNAME,
                                            VALUE_NAME_DATASOURCE_GROUPNAME ) ;
                     }
                     else if ( 0 == ossStrcmp( elem2.fieldName(), FIELD_NAME_NODE_NAME ) )
                     {
                        subBuilder2.append( FIELD_NAME_NODE_NAME,
                                            VALUE_NAME_DATASOURCE_GROUPNAME ) ;
                     }
                     else
                     {
                        subBuilder2.append( elem2 ) ;
                     }
                  }
                  subBuilder2.done() ;
               }
               subBuilder.done() ;
            }
            BSONObj newResult = builder.obj() ;
            ossMemcpy( pNewMsg + offset,
                       newResult.objdata(), newResult.objsize() ) ;
            offset += ossAlign4( newResult.objsize() ) ;
         }

         /// change length
         ((MsgHeader*)pNewMsg)->messageLength = offset ;

         event._Data = pNewMsg ;
         event._eventType = orgEvent._eventType ;
         event._dataMemType = PMD_EDU_MEM_THREAD ;

         pNewMsg = NULL ;
         hasConvert = TRUE ;
      }
      catch ( exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Exception occurs: %s", e.what() ) ;
         goto error ;
      }

   done:
      if ( pNewMsg )
      {
         SDB_THREAD_FREE( pNewMsg ) ;
      }
      PD_TRACE_EXITRC( SDB__COORDDSMSGCONVERTOR__CONVERTGETDETAILEVENT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__COORDDSMSGCONVERTOR_CONVERTREPLY, "_coordDSMsgConvertor::convertReply" )
   INT32 _coordDSMsgConvertor::convertReply( _pmdSubSession *pSub,
                                             _pmdEDUCB *cb,
                                             const pmdEDUEvent &orgEvent,
                                             pmdEDUEvent &newEvent,
                                             BOOLEAN &hasConvert )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__COORDDSMSGCONVERTOR_CONVERTREPLY ) ;
      MsgOpReply *pReply = ( MsgOpReply* )orgEvent._Data ;
      hasConvert = FALSE ;

      if ( SDB_OK != pReply->flags )
      {
         goto done ;
      }

      if ( MSG_BS_LOB_TRUNCATE_RES == pReply->header.opCode ||
           MSG_BS_LOB_REMOVE_RES == pReply->header.opCode )
      {
         /// build empty(size=0) meta object
         BSONObj metaObj ;

         rc = _buildEmptyLobMeta( metaObj ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Build lob meta object failed, rc: %d", rc ) ;
            goto error ;
         }

         rc = _buildNewEvent( pReply, metaObj, orgEvent, newEvent ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Build new event failed, rc: %d", rc ) ;
            goto error ;
         }

         /// make a fake context ID to reply lob open request
         ((MsgOpReply*)newEvent._Data)->contextID = 1 ;

         hasConvert = TRUE ;
      }
      else if ( MSG_BS_LOB_READ_RES == pReply->header.opCode )
      {
         /// change tuple.column.sequence = 1
         MsgLobTuple *tuple = NULL ;

         if ( (UINT32)(pReply->header.messageLength) <
               sizeof( MsgOpReply ) + sizeof( MsgLobTuple ) )
         {
            rc = SDB_INVALIDSIZE ;
            PD_LOG( PDERROR, "Recive invliad lob read reply, length:%u",
                    pReply->header.messageLength ) ;
            goto error ;
         }
         tuple = ( MsgLobTuple *)( (CHAR*)orgEvent._Data +
                                   sizeof( MsgOpReply ) ) ;
         tuple->columns.sequence = 1 ;
      }
      else if ( MSG_BS_LOB_OPEN_RES == pReply->header.opCode )
      {
         const CHAR *pOrgMeta = NULL ;
         BSONObj metaObj ;

         if ( (UINT32)(pReply->header.messageLength) <
               sizeof( MsgOpReply ) + 5 )
         {
            rc = SDB_INVALIDSIZE ;
            PD_LOG( PDERROR, "Recive invliad lob open reply, length:%u",
                    pReply->header.messageLength ) ;
            goto error ;
         }

         pOrgMeta = (const CHAR*)pReply + sizeof( MsgOpReply ) ;

         rc = _buildLowVerLobMeta( pOrgMeta, metaObj ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Build lob meta object failed, rc: %d", rc ) ;
            goto error ;
         }

         rc = _buildNewEvent( pReply, metaObj, orgEvent, newEvent ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Build new event failed, rc: %d", rc ) ;
            goto error ;
         }

         hasConvert = TRUE ;
      }
      else if ( MSG_BS_QUERY_RES == pReply->header.opCode )
      {
         MsgOpQuery *pQueryMsg = (MsgOpQuery*)pSub->getReqMsg() ;
         if ( '$' != pQueryMsg->name[0] &&
              ( pQueryMsg->flags & FLG_QUERY_EXPLAIN ) )
         {
            pReply->startFrom = RTN_CTX_EXPLAIN_PROCESSOR ;
         }
         else if ( '$' == pQueryMsg->name[0] &&
                   0 == ossStrcmp( pQueryMsg->name + 1, CMD_NAME_GET_CL_DETAIL ) )
         {
            rc = _convertGetDetailEvent( pReply, orgEvent, newEvent, hasConvert ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Convert event failed, rc: %d", rc ) ;
               goto error ;
            }
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__COORDDSMSGCONVERTOR_CONVERTREPLY, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__COORDDSMSGCONVERTOR_CONVERT, "_coordDSMsgConvertor::convert" )
   INT32 _coordDSMsgConvertor::convert( _pmdSubSession *pSub,
                                        _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__COORDDSMSGCONVERTOR_CONVERT ) ;

      MsgHeader *msg = pSub->getReqMsg() ;

      if ( !pSub->getConnection()->isExtern() )
      {
         /// do nothing
         goto done ;
      }

      try
      {
         switch ( msg->opCode )
         {
            case MSG_BS_INSERT_REQ:
            case MSG_BS_TRANS_INSERT_REQ:
               rc = _rebuildDataSourceInsertMsg( pSub, cb ) ;
               break ;
            case MSG_BS_UPDATE_REQ:
            case MSG_BS_TRANS_UPDATE_REQ:
               rc = _rebuildDataSourceUpdateMsg( pSub, cb ) ;
               break ;
            case MSG_BS_DELETE_REQ:
            case MSG_BS_TRANS_DELETE_REQ:
               rc = _rebuildDataSourceDeleteMsg( pSub, cb ) ;
               break ;
            case MSG_BS_QUERY_REQ:
            case MSG_BS_TRANS_QUERY_REQ:
               rc = _rebuildDataSourceQueryMsg( pSub, cb ) ;
               break ;
            case MSG_BS_LOB_OPEN_REQ:
            case MSG_BS_LOB_TRUNCATE_REQ:
            case MSG_BS_LOB_REMOVE_REQ:
            case MSG_BS_LOB_WRITE_REQ:
            case MSG_BS_LOB_READ_REQ:
            case MSG_BS_LOB_UPDATE_REQ:
               rc = _rebuildDataSourceLobMsg( pSub, cb ) ;
               break ;
            default:
               // Other operations do not convert message
               break ;
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__COORDDSMSGCONVERTOR_CONVERT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__COORDDSMSGCONVERTOR__REBUILDDATASOURCEINSERTMSG , "_coordDSMsgConvertor::_rebuildDataSourceInsertMsg" )
   INT32 _coordDSMsgConvertor::_rebuildDataSourceInsertMsg( pmdSubSession *pSub,
                                                            _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__COORDDSMSGCONVERTOR__REBUILDDATASOURCEINSERTMSG ) ;
      MsgHeader *msg = pSub->getReqMsg() ;
      netIOVec *dataVec = pSub->getIODatas() ;
      ossPoolVector< const CHAR * > subCLNames ;

      if ( MSG_BS_TRANS_INSERT_REQ == msg->opCode )
      {
         msg->opCode = MSG_BS_INSERT_REQ ;
      }

      if ( dataVec->size() > 1 )
      {
         // Data for insertion is not in a continous buffer. This hapends when
         // using bulk insertion to insert into a sharded collection or a main
         // collection. As a sharded collection is not allowed to use data
         // source, it may only be a main collection here.
         rc = _getInsertSubCLNames( dataVec, subCLNames ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse sub collection names from "
                      "insert message, rc: %d", rc ) ;

         if ( 1 == subCLNames.size() )
         {
            rc = _rebuildDSInsertMsg4SingleSubCL( pSub, cb ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build data source insert "
                         "message for single sub collection, rc: %d", rc ) ;
         }
         else
         {
            rc = _rebuildDSInsertMsg4MultiSubCL( pSub, cb, subCLNames ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build data source insert "
                         "message for multiple sub collections, rc: %d", rc ) ;
         }
      }
      else
      {
         rc = _rebuildDSInsertMsg4GeneralCL( pSub, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build data source insert "
                        "message for general collection, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__COORDDSMSGCONVERTOR__REBUILDDATASOURCEINSERTMSG,
                       rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__COORDDSMSGCONVERTOR__GETINSERTSUBCLNAMES, "_coordDSMsgConvertor::_getInsertSubCLNames" )
   INT32 _coordDSMsgConvertor::_getInsertSubCLNames(
         netIOVec *dataVec,
         ossPoolVector< const CHAR *> &subCLNames )
   {
      INT32 rc = 0 ;
      PD_TRACE_ENTRY( SDB__COORDDSMSGCONVERTOR__GETINSERTSUBCLNAMES ) ;

      /*
         message format:
         | MsgOpInsert header | cl name | fixed
         | cl info A | fixed | insert object 1 | ... | insert object N
         | cl info B | fixed | insert object 1 | ... | insert object M |
         ...
         | hint mark | hint object |

         cl info:
         | SubObjNum | SubObjSize | SubCLName |

         fixed: the fixed item is used for 4-bytes alignment
         hint mark: 4-bytes of zero to mark that next object is hint.
      */
      try
      {
         netIOVec::iterator itr = dataVec->begin() ; // Point to fixed item.
         ++itr ; // Now point to collection information object.
         subCLNames.clear() ;

         while ( itr != dataVec->end() )
         {
            netIOV clInfo = *itr ;
            UINT32 objSize = *(UINT32 *)clInfo.iovBase ;
            if ( 0 == objSize ) // hint mark, the end of message
            {
               break ;
            }
            BSONObj clInfoObj( (CHAR *)clInfo.iovBase ) ;
            UINT32 leftSize = 0 ;
            const CHAR *subCLName =
                  clInfoObj.getField( FIELD_NAME_SUBCLNAME ).valuestrsafe() ;
            subCLNames.push_back( subCLName ) ;
            INT32 objectsSize =
                  clInfoObj.getField( FIELD_NAME_SUBOBJSSIZE ).numberInt() ;
            if ( objectsSize <= 0 )
            {
               break ;
            }

            leftSize += ( 4 - ( clInfoObj.objsize() % 4 ) ) ;
            leftSize += ossRoundUpToMultipleX( objectsSize, 4 ) ;
            while ( itr++ != dataVec->end() )
            {
               netIOV iov = *itr ;
               leftSize -= iov.iovLen ;
               if ( leftSize <= 0 )
               {
                  SDB_ASSERT( 0 == leftSize, "Wrong size" ) ;
                  break ;
               }
            }
            ++itr ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXIT( SDB__COORDDSMSGCONVERTOR__GETINSERTSUBCLNAMES ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__COORDDSMSGCONVERTOR__REBUILDDSINSERTMSG4SINGLESUBCL, "_coordDSMsgConvertor::_rebuildDSInsertMsg4SingleSubCL" )
   INT32 _coordDSMsgConvertor::_rebuildDSInsertMsg4SingleSubCL( pmdSubSession *pSub,
                                                                _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__COORDDSMSGCONVERTOR__REBUILDDSINSERTMSG4SINGLESUBCL ) ;

      MsgHeader *msg = pSub->getReqMsg() ;
      netIOVec *dataVec = pSub->getIODatas() ;
      coordResource *pResource = sdbGetCoordCB()->getResource() ;
      CoordCataInfoPtr catInfo ;
      void *buff = NULL ;

      try
      {
         netIOVec::iterator itr = dataVec->begin() ; // Point to fixed item.
         ++itr ; // Now point to collection information object.
         netIOV clInfo = *itr ;
         UINT32 offset = 0 ;

         BSONObj clInfoObj( (CHAR *)clInfo.iovBase ) ;
         const CHAR *subCLName = clInfoObj.getStringField( FIELD_NAME_SUBCLNAME ) ;

         // If field SubCLName exists, it's insertion on main
         // collection.
         UINT32 totalLen = 0 ;
         MsgOpInsert *insertMsg = NULL ;
         string mapping ;

         rc = pResource->getOrUpdateCataInfo( subCLName, catInfo, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Get Catalogue information of "
                        "collection[%s] failed[%d]", subCLName, rc ) ;

         mapping = catInfo->getMappingName() ;
         insertMsg = (MsgOpInsert *)msg;
         totalLen = msg->messageLength ;
         if ( mapping.size() > (UINT32) insertMsg->nameLength )
         {
            totalLen += ossRoundUpToMultipleX(
                  mapping.size() - insertMsg->nameLength, 4 ) ;
         }

         buff = ( CHAR * )SDB_THREAD_ALLOC( totalLen ) ;
         if ( !buff )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Allocate memory for message failed[%d]",
                     rc ) ;
            goto error ;
         }
         ossMemset( buff, 0, totalLen ) ;
         insertMsg = (MsgOpInsert *)buff;

         *insertMsg = *(MsgOpInsert *)msg ;
         if ( mapping.size() > 0 )
         {
            ossStrncpy( insertMsg->name, mapping.c_str(), mapping.size() ) ;
            insertMsg->nameLength = mapping.size() ;
         }
         else
         {
            ossStrncpy( insertMsg->name, subCLName, ossStrlen( subCLName ) ) ;
            insertMsg->nameLength = ossStrlen( subCLName ) ;
         }

         // If the cl information object is not aligned by 4, there is one
         // filling item in the dataVec(refer to
         // _coordInsertOperator::buildInsertMsg). So we need to skip the
         // that item.
         if ( !ossIsAligned4( itr->iovLen ) )
         {
            ++itr ;  // Now point to the filling item.
            SDB_ASSERT( itr->iovLen < 4,
                        "Item length should be less than 4" ) ;
         }
         ++itr ;  // Now point to the first record.

         offset = ossRoundUpToMultipleX( offsetof(MsgOpInsert, name)
                                          + insertMsg->nameLength + 1,
                                          4 ) ;

         while ( itr != dataVec->end() )
         {
            ossMemcpy( (CHAR *) buff + offset, itr->iovBase, itr->iovLen ) ;
            offset = ossRoundUpToMultipleX( offset + itr->iovLen, 4 ) ;
            ++itr ;
         }
         insertMsg->header.messageLength = offset ;

         pSub->setReqMsg( (MsgHeader *)buff,  PMD_EDU_MEM_THREAD ) ;
         buff = NULL ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }
   done:
      if ( buff )
      {
         SDB_THREAD_FREE( buff ) ;
      }
      PD_TRACE_EXITRC( SDB__COORDDSMSGCONVERTOR__REBUILDDSINSERTMSG4SINGLESUBCL,
                       rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__COORDDSMSGCONVERTOR__REBUILDDSINSERTMSG4MULTISUBCL, "_coordDSMsgConvertor::_rebuildDSInsertMsg4MultiSubCL" )
   INT32 _coordDSMsgConvertor::_rebuildDSInsertMsg4MultiSubCL( pmdSubSession *pSub,
                                                               _pmdEDUCB *cb,
                                                               ossPoolVector< const CHAR * > &subCLNameList )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__COORDDSMSGCONVERTOR__REBUILDDSINSERTMSG4MULTISUBCL ) ;

      MsgHeader *msg = pSub->getReqMsg() ;
      netIOVec *dataVec = pSub->getIODatas() ;
      coordResource *pResource = sdbGetCoordCB()->getResource() ;
      CoordCataInfoPtr catInfo ;
      void *buff = NULL ;

      try
      {
         netIOVec::iterator itr = dataVec->begin() ; // Point to fixed item.
         ++itr ; // Now point to collection information object.
         netIOV clInfo = *itr ;
         UINT32 offset = 0 ;
         BSONObjBuilder hintBuilder ;

         BSONObj clInfoObj( (CHAR *)clInfo.iovBase ) ;
         const CHAR *subCLName = clInfoObj.getStringField( FIELD_NAME_SUBCLNAME ) ;
         SINT32 subObjsSize = clInfoObj.getIntField( FIELD_NAME_SUBOBJSSIZE ) ;

         UINT32 totalLen = msg->messageLength ;
         MsgOpInsert *insertMsg = (MsgOpInsert *) msg ;
         string dsMainCLName ;

         rc = pResource->getOrUpdateCataInfo( subCLName, catInfo, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Get Catalogue information of "
                        "collection[%s] failed[%d]", subCLName, rc ) ;

         dsMainCLName = catInfo->getDSMainCLName() ;
         if ( dsMainCLName.size() > (UINT32) insertMsg->nameLength )
         {
            totalLen += ossRoundUpToMultipleX(
                  dsMainCLName.size() - insertMsg->nameLength, 4 ) ;
         }

         rc = _buildSubCLInfoHint( subCLNameList, hintBuilder, cb ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to build sub collection info hint, rc: %d",
                      rc ) ;
         totalLen += ossAlign4( hintBuilder.len() ) ;

         buff = ( CHAR * )SDB_THREAD_ALLOC( totalLen ) ;
         if ( !buff )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Allocate memory for message failed[%d]",
                     rc ) ;
            goto error ;
         }
         ossMemset( buff, 0, totalLen ) ;
         insertMsg = (MsgOpInsert *)buff;
         *insertMsg = *(MsgOpInsert *)msg ;
         ossStrncpy( insertMsg->name, dsMainCLName.c_str(), dsMainCLName.size() ) ;
         insertMsg->nameLength = dsMainCLName.size() ;

         // If the cl information object is not aligned by 4, there is one
         // filling item in the dataVec(refer to
         // _coordInsertOperator::buildInsertMsg). So we need to skip the
         // that item.
         if ( !ossIsAligned4( itr->iovLen ) )
         {
            ++itr ;  // Now point to the filling item.
            SDB_ASSERT( itr->iovLen < 4,
                        "Item length should be less than 4" ) ;
         }
         ++itr ;  // Now point to the first record.

         offset = ossRoundUpToMultipleX( offsetof(MsgOpInsert, name)
                                         + insertMsg->nameLength + 1,
                                         4 ) ;

         while ( itr != dataVec->end() )
         {
            INT32 objSize = *(INT32 *)itr->iovBase ;
            if ( 0 == objSize )
            {
               BSONObj hint( ((CHAR *)itr->iovBase) + MSG_HINT_MARK_LEN ) ;
               hintBuilder.appendElements( hint ) ;
               break ;
            }

            // Switch to next sub collection area
            if ( subObjsSize <= 0 )
            {
               // Now point to collection information object.
               SDB_ASSERT( subObjsSize == 0, "Wrong length" ) ;
               BSONObj clInfoObj2( (CHAR *)itr->iovBase ) ;
               subObjsSize = clInfoObj2.getIntField( FIELD_NAME_SUBOBJSSIZE ) ;
               ++itr ; // skip collection information object.
               if ( !ossIsAligned4( itr->iovLen ) )
               {
                  ++itr ;  // skip the filling item.
               }
            }

            ossMemcpy( (CHAR *) buff + offset, itr->iovBase, itr->iovLen ) ;
            offset = ossRoundUpToMultipleX( offset + itr->iovLen, 4 ) ;
            insertMsg->header.messageLength = offset ;
            subObjsSize -= itr->iovLen ;
            ++itr ;
         }

         // Append sub cl list to hint to ensure existence
         {
            OSS_BIT_SET( insertMsg->flags, FLG_INSERT_HASHINT ) ;
            offset += MSG_HINT_MARK_LEN ;

            BSONObj newHint = hintBuilder.obj() ;
            ossMemcpy( (CHAR *) buff + offset, newHint.objdata(), newHint.objsize() ) ;
            offset = ossRoundUpToMultipleX( offset + newHint.objsize(), 4 ) ;
            insertMsg->header.messageLength = offset ;
         }

         pSub->setReqMsg( (MsgHeader *)buff,  PMD_EDU_MEM_THREAD ) ;
         buff = NULL ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }
   done:
      if ( buff )
      {
         SDB_THREAD_FREE( buff ) ;
      }
      PD_TRACE_EXITRC( SDB__COORDDSMSGCONVERTOR__REBUILDDSINSERTMSG4MULTISUBCL,
                       rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__COORDDSMSGCONVERTOR__REBUILDDSINSERTMSG4GENERALCL, "_coordDSMsgConvertor::_rebuildDSInsertMsg4GeneralCL" )
   INT32 _coordDSMsgConvertor::_rebuildDSInsertMsg4GeneralCL( pmdSubSession *pSub,
                                                              _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__COORDDSMSGCONVERTOR__REBUILDDSINSERTMSG4GENERALCL ) ;

      MsgHeader *msg = pSub->getReqMsg() ;
      coordResource *pResource = sdbGetCoordCB()->getResource() ;
      CoordCataInfoPtr catInfo ;
      string mappingName ;
      void *buff = NULL ;

      MsgOpInsert *insertMsg = (MsgOpInsert *)msg ;
      rc = pResource->getOrUpdateCataInfo( insertMsg->name, catInfo, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Get catalogue of collection[%s] failed[%d]",
                     insertMsg->name, rc ) ;
      mappingName = catInfo->getMappingName() ;
      if ( 0 != ossStrcmp( mappingName.c_str(), insertMsg->name ) )
      {
         // Different name mapping, need to change the name in the message.
         if ( mappingName.size() == (UINT32)insertMsg->nameLength )
         {
            ossStrncpy( insertMsg->name, mappingName.c_str(),
                        mappingName.size() ) ;
         }
         else
         {
            MsgOpInsert *newInsertMsg = NULL ;
            UINT32 size = insertMsg->header.messageLength ;
            if ( mappingName.size() > (UINT32) insertMsg->nameLength )
            {
               size += ossRoundUpToMultipleX(
                     mappingName.size() - insertMsg->nameLength, 4 ) ;
            }
            buff = (CHAR *)SDB_THREAD_ALLOC( size ) ;
            PD_CHECK( buff, SDB_OOM, error, PDERROR, "Allocate for data "
                        "source message failed[%d]", SDB_OOM ) ;
            ossMemset( buff, 0, size ) ;
            newInsertMsg = (MsgOpInsert *)buff ;

            UINT32 bodyOffset = ossRoundUpToMultipleX(
                  offsetof(MsgOpInsert, name) + insertMsg->nameLength + 1,
                  4 ) ;
            CHAR *body = (CHAR *)msg + bodyOffset ;
            UINT32 bodyLen = insertMsg->header.messageLength - bodyOffset ;
            *newInsertMsg = *insertMsg ;
            ossStrncpy( newInsertMsg->name, mappingName.c_str(),
                        mappingName.size() ) ;
            newInsertMsg->nameLength = mappingName.size() ;
            UINT32 newBodyOffset = ossRoundUpToMultipleX(
                  offsetof(MsgOpInsert, name) + newInsertMsg->nameLength + 1,
                  4 ) ;
            ossMemcpy( (CHAR *) buff + newBodyOffset, body, bodyLen ) ;
            newInsertMsg->header.messageLength = newBodyOffset + bodyLen ;

            pSub->setReqMsg( (MsgHeader *)newInsertMsg, PMD_EDU_MEM_THREAD ) ;
            buff = NULL ;
         }
      }

   done:
      if ( buff )
      {
         SDB_THREAD_FREE( buff ) ;
      }
      PD_TRACE_EXITRC( SDB__COORDDSMSGCONVERTOR__REBUILDDSINSERTMSG4GENERALCL,
                       rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__COORDDSMSGCONVERTOR__REBUILDDATASOURCEUPDATEMSG , "_coordDSMsgConvertor::_rebuildDataSourceUpdateMsg" )
   INT32 _coordDSMsgConvertor::_rebuildDataSourceUpdateMsg( pmdSubSession *pSub,
                                                            _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__COORDDSMSGCONVERTOR__REBUILDDATASOURCEUPDATEMSG ) ;
      CHAR *newMsg = NULL ;
      INT32 msgLen = 0 ;
      MsgHeader *msg = pSub->getReqMsg() ;
      netIOVec *dataVec = pSub->getIODatas() ;
      netIOVec::const_iterator itr = dataVec->begin() ;

      coordResource *pResource = sdbGetCoordCB()->getResource() ;
      CoordCataInfoPtr cataInfo ;

      if ( MSG_BS_TRANS_UPDATE_REQ == msg->opCode )
      {
         msg->opCode = MSG_BS_UPDATE_REQ ;
      }

      try
      {
         if ( dataVec->size() > 1 )
         {
            // Update on a main collection
            ++itr ; // Now point to selector
            BSONObj selector( (CHAR *)itr->iovBase ) ;
            if ( selector.hasField( FIELD_NAME_SUBCLNAME ) )
            {
               MsgOpUpdate *origMsg = (MsgOpUpdate *)pSub->getReqMsg() ;
               ossPoolVector< const CHAR * > subCLNameList ;
               BSONObj newSelector ;
               BSONObj newHint ;
               BSONObjBuilder selBuilder ;
               BSONObjBuilder hintBuilder ;
               BSONObjIterator selItr( selector ) ;

               ++itr ; // Now point to updator
               BSONObj updator( (CHAR *)itr->iovBase ) ;
               BSONObj hint( ( CHAR *)itr->iovBase +
                             ossRoundUpToMultipleX( updator.objsize(), 4 ) ) ;
               while ( selItr.more() )
               {
                  BSONElement ele = selItr.next() ;
                  if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_SUBCLNAME ) )
                  {
                     BSONObjIterator selItr( ele.embeddedObject() ) ;
                     while ( selItr.more() )
                     {
                        BSONElement clEle = selItr.next() ;
                        subCLNameList.push_back( clEle.valuestrsafe() ) ;
                     }
                  }
                  else
                  {
                     selBuilder.append( ele ) ;
                  }
               }
               newSelector = selBuilder.done() ;

               SDB_ASSERT( !subCLNameList.empty(), "sub cl name array is empty" ) ;
               if ( subCLNameList.size() == 1 )
               {
                  const CHAR * subCLName = subCLNameList.at( 0 ) ;
                  string mappingName ;
                  rc = pResource->getOrUpdateCataInfo( subCLName, cataInfo, cb ) ;
                  PD_RC_CHECK( rc, PDERROR, "Get catalogue information of "
                              "collection[%s] failed[%d]", subCLName, rc ) ;
                  if ( cataInfo->getMappingName().size() > 0 )
                  {
                     mappingName = cataInfo->getMappingName() ;
                     subCLName = mappingName.c_str() ;
                  }

                  rc = msgBuildUpdateMsg( &newMsg, &msgLen, subCLName,
                                          origMsg->flags,
                                          origMsg->header.requestID, &newSelector,
                                          &updator, &hint, cb ) ;
                  PD_RC_CHECK( rc, PDERROR, "Build update message for data source "
                               "failed[%d]", rc ) ;
               }
               else
               {
                  string dsMainCLName ;
                  const CHAR *subCLName = subCLNameList.at( 0 ) ;
                  rc = pResource->getOrUpdateCataInfo( subCLName, cataInfo, cb ) ;
                  PD_RC_CHECK( rc, PDERROR, "Get catalogue information of "
                              "collection[%s] failed[%d]", subCLName, rc ) ;
                  dsMainCLName = cataInfo->getDSMainCLName() ;

                  rc = _buildSubCLInfoHint( subCLNameList, hintBuilder, cb ) ;
                  PD_RC_CHECK( rc, PDERROR,
                               "Failed to build sub collection info hint, rc: %d",
                               rc ) ;
                  hintBuilder.appendElements( hint ) ;
                  newHint = hintBuilder.obj() ;

                  rc = msgBuildUpdateMsg( &newMsg, &msgLen, dsMainCLName.c_str(),
                                          origMsg->flags,
                                          origMsg->header.requestID, &newSelector,
                                          &updator, &newHint, cb ) ;
                  PD_RC_CHECK( rc, PDERROR, "Build update message for data source "
                               "failed[%d]", rc ) ;
               }
            }
         }
         else
         {
            // Update on a non main collection.
            MsgOpUpdate *origUpdateMsg = (MsgOpUpdate *)msg ;
            rc = pResource->getOrUpdateCataInfo( origUpdateMsg->name,
                                                 cataInfo, cb ) ;
            PD_RC_CHECK( rc, PDERROR, "Get catalogue information of collection"
                         "[%s] failed[%d]", origUpdateMsg->name, rc ) ;
            if ( 0 != ossStrcmp( origUpdateMsg->name,
                                 cataInfo->getMappingName().c_str() ) )
            {
               UINT32 offset = ossRoundUpToMultipleX(
                     offsetof(MsgOpUpdate, name) +
                     origUpdateMsg->nameLength + 1, 4 ) ;
               BSONObj selector( (CHAR *)msg + offset ) ;
               offset = ossRoundUpToMultipleX( offset + selector.objsize(), 4 ) ;
               BSONObj updator( (CHAR *)msg + offset ) ;
               offset = ossRoundUpToMultipleX( offset + updator.objsize(), 4 ) ;
               BSONObj hint( (CHAR *)msg + offset ) ;

               rc = msgBuildUpdateMsg( &newMsg, &msgLen,
                                       cataInfo->getMappingName().c_str(),
                                       origUpdateMsg->flags,
                                       origUpdateMsg->header.requestID,
                                       &selector, &updator, &hint, cb ) ;
               PD_RC_CHECK( rc, PDERROR, "Rebuild update message for data "
                                         "source failed[%d]", rc ) ;
            }
         }
         if ( newMsg )
         {
            UINT32 newMsgLen = ( (MsgHeader *)newMsg)->messageLength ;
            *(MsgHeader *)newMsg = *msg ;
            ((MsgHeader *)newMsg)->messageLength = newMsgLen ;
            pSub->setReqMsg( (MsgHeader *)newMsg, PMD_EDU_MEM_THREAD ) ;
            newMsg = NULL ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      if ( newMsg )
      {
         msgReleaseBuffer( newMsg ) ;
      }
      PD_TRACE_EXITRC( SDB__COORDDSMSGCONVERTOR__REBUILDDATASOURCEUPDATEMSG,
                       rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__COORDDSMSGCONVERTOR__REBUILDDATASOURCEDELETEMSG , "_coordDSMsgConvertor::_rebuildDataSourceDeleteMsg" )
   INT32 _coordDSMsgConvertor::_rebuildDataSourceDeleteMsg( pmdSubSession *pSub,
                                                            _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__COORDDSMSGCONVERTOR__REBUILDDATASOURCEDELETEMSG ) ;
      CHAR *newMsg = NULL ;
      INT32 msgLen = 0 ;
      MsgHeader *msg = pSub->getReqMsg() ;
      netIOVec *dataVec = pSub->getIODatas() ;
      netIOVec::const_iterator itr = dataVec->begin() ;

      coordResource *pResource = sdbGetCoordCB()->getResource() ;
      CoordCataInfoPtr cataInfo ;

      if ( MSG_BS_TRANS_DELETE_REQ == msg->opCode )
      {
         msg->opCode = MSG_BS_DELETE_REQ ;
      }

      try
      {
         if ( dataVec->size() > 1 )
         {
            ++itr ;  // Now point to the deletor.
            BSONObj deletor( (CHAR *)itr->iovBase ) ;
            ++itr ;
            // Check if the deletor contains sub collection information. If yes,
            // it's an deletion on a main collection. In that case, the message
            // should be rebuilt. Otherwise, the message should not be changed.
            if ( deletor.hasField( FIELD_NAME_SUBCLNAME ) )
            {
               MsgOpDelete *origMsg = (MsgOpDelete *)pSub->getReqMsg() ;
               ossPoolVector< const CHAR * > subCLNameList ;
               BSONObj hint( (CHAR *)itr->iovBase ) ;
               BSONObj newDeletor ;
               BSONObj newHint ;
               BSONObjBuilder delBuilder ;
               BSONObjBuilder hintBuilder ;
               BSONObjIterator delItr( deletor ) ;
               while ( delItr.more() )
               {
                  BSONElement ele = delItr.next() ;
                  if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_SUBCLNAME ) )
                  {
                     BSONObjIterator delItr( ele.embeddedObject() ) ;
                     while ( delItr.more() )
                     {
                        BSONElement clEle = delItr.next() ;
                        subCLNameList.push_back( clEle.valuestrsafe() ) ;
                     }
                  }
                  else
                  {
                     delBuilder.append( ele ) ;
                  }
               }
               newDeletor = delBuilder.done() ;

               SDB_ASSERT( !subCLNameList.empty(), "sub cl name array is empty" ) ;
               if ( subCLNameList.size() == 1 )
               {
                  const CHAR *subCLName = subCLNameList.at( 0 ) ;
                  string mappingName ;
                  rc = pResource->getOrUpdateCataInfo( subCLName, cataInfo, cb ) ;
                  PD_RC_CHECK( rc, PDERROR, "Get catalogue information of "
                              "collection[%s] failed[%d]", subCLName, rc ) ;
                  if ( cataInfo->getMappingName().size() > 0 )
                  {
                     mappingName = cataInfo->getMappingName() ;
                     subCLName = mappingName.c_str() ;
                  }

                  rc = msgBuildDeleteMsg( &newMsg, &msgLen, subCLName,
                                          origMsg->flags,
                                          origMsg->header.requestID, &newDeletor,
                                          &hint, cb ) ;
                  PD_RC_CHECK( rc, PDERROR, "Build delete message for data source "
                                          "failed[%d]", rc ) ;
               }
               else
               {
                  string dsMainCLName ;
                  const CHAR *subCLName = subCLNameList.at( 0 ) ;
                  rc = pResource->getOrUpdateCataInfo( subCLName, cataInfo, cb ) ;
                  PD_RC_CHECK( rc, PDERROR, "Get catalogue information of "
                              "collection[%s] failed[%d]", subCLName, rc ) ;
                  dsMainCLName = cataInfo->getDSMainCLName() ;

                  rc = _buildSubCLInfoHint( subCLNameList, hintBuilder, cb ) ;
                  PD_RC_CHECK( rc, PDERROR,
                               "Failed to build sub collection info hint, rc: %d",
                               rc ) ;
                  hintBuilder.appendElements( hint ) ;
                  newHint = hintBuilder.obj() ;

                  rc = msgBuildDeleteMsg( &newMsg, &msgLen, dsMainCLName.c_str(),
                                          origMsg->flags,
                                          origMsg->header.requestID, &newDeletor,
                                          &newHint, cb ) ;
                  PD_RC_CHECK( rc, PDERROR, "Build delete message for data source "
                                          "failed[%d]", rc ) ;
               }
            }
         }
         else
         {
            MsgOpDelete *origDeleteMsg = (MsgOpDelete *)msg ;
            rc = pResource->getOrUpdateCataInfo( origDeleteMsg->name,
                                                 cataInfo, cb ) ;
            PD_RC_CHECK( rc, PDERROR, "Get catalogue information of collection"
                         "[%s] failed[%d]", origDeleteMsg->name, rc ) ;
            if ( 0 != ossStrcmp( origDeleteMsg->name,
                                 cataInfo->getMappingName().c_str() ) )
            {
               UINT32 offset = ossRoundUpToMultipleX(
                     offsetof(MsgOpDelete, name) +
                     origDeleteMsg->nameLength + 1, 4 ) ;
               BSONObj deletor( (CHAR *)msg + offset ) ;
               offset = ossRoundUpToMultipleX( offset + deletor.objsize(), 4 ) ;
               BSONObj hint( (CHAR *)msg + offset ) ;

               rc = msgBuildDeleteMsg( &newMsg, &msgLen,
                                       cataInfo->getMappingName().c_str(),
                                       origDeleteMsg->flags,
                                       origDeleteMsg->header.requestID,
                                       &deletor, &hint, cb ) ;
               PD_RC_CHECK( rc, PDERROR, "Rebuild delete message for data "
                                         "source failed[%d]", rc ) ;
            }
         }
         if ( newMsg )
         {
            UINT32 newMsgLen = ( (MsgHeader *)newMsg)->messageLength ;
            *(MsgHeader *)newMsg = *msg ;
            ((MsgHeader *)newMsg)->messageLength = newMsgLen ;
            pSub->setReqMsg( (MsgHeader *)newMsg, PMD_EDU_MEM_THREAD ) ;
            newMsg = NULL ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      if ( newMsg )
      {
         msgReleaseBuffer( newMsg ) ;
      }
      PD_TRACE_EXITRC( SDB__COORDDSMSGCONVERTOR__REBUILDDATASOURCEDELETEMSG,
                       rc ) ;
      return rc ;
   error:
      goto done ;
   }

   /**
    * The query may be on a main collection or a normal collection.
    */
   // PD_TRACE_DECLARE_FUNCTION ( SDB__COORDDSMSGCONVERTOR__REBUILDDATASOURCEQUERYMSG , "_coordDSMsgConvertor::_rebuildDataSourceQueryMsg" )
   INT32 _coordDSMsgConvertor::_rebuildDataSourceQueryMsg( pmdSubSession *pSub,
                                                           _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__COORDDSMSGCONVERTOR__REBUILDDATASOURCEQUERYMSG ) ;

      CHAR *newMsg = NULL ;
      INT32 msgLen = 0 ;
      MsgHeader *msg = pSub->getReqMsg() ;
      netIOVec *dataVec = pSub->getIODatas() ;
      netIOVec::const_iterator itr = dataVec->begin() ;

      coordResource *pResource = sdbGetCoordCB()->getResource() ;
      CoordCataInfoPtr cataInfo ;

      MsgOpQuery *origQuery = (MsgOpQuery *)msg ;

      // Query which is not a command.
      if ( MSG_BS_TRANS_QUERY_REQ == msg->opCode )
      {
         msg->opCode = MSG_BS_QUERY_REQ ;
      }

      // Note: Do not use the msgExtractQuery to extract the message here, as
      // it may have been broken into netIOVector coordinator processing.
      if ( CMD_ADMIN_PREFIX[0] == origQuery->name[0] )
      {
         // If it's a command, do nothing. The message will not be sent.
         rc = _rebuildDataSourceCmdMsg( pSub, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Rebuild data source command message "
                                   "failed[%d]", rc ) ;
      }
      else
      {
         try
         {
            // TODO: YSD use _parseQueryMsg
            if ( dataVec->size() > 1 )
            {
               ++itr ;  // Now point to collection information object.
               BSONObj query( (CHAR *)itr->iovBase ) ;
               if ( query.hasField( FIELD_NAME_SUBCLNAME ) )
               {
                  ossPoolVector< const CHAR* >  subCLNameList ;
                  BSONObj matcher ;
                  BSONObj selector ;
                  BSONObj orderBy ;
                  BSONObj hint ;
                  BSONObj newHint ;
                  BSONObjBuilder matchBuilder ;
                  BSONObjBuilder hintBuilder ;
                  BSONObjIterator queryItr( query ) ;

                  if ( dataVec->size() > 2 )
                  {
                     UINT32 len = 0 ;
                     ++itr ;
                     selector = BSONObj( (CHAR *)itr->iovBase ) ;
                     len = ossRoundUpToMultipleX( selector.objsize(), 4 ) ;
                     orderBy = BSONObj( (CHAR *)itr->iovBase + len ) ;
                     len += ossRoundUpToMultipleX( orderBy.objsize(), 4 ) ;
                     hint = BSONObj( (CHAR *)itr->iovBase + len ) ;
                  }

                  while ( queryItr.more() )
                  {
                     BSONElement ele = queryItr.next() ;
                     if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_SUBCLNAME ) )
                     {
                        BSONObjIterator subItr( ele.embeddedObject()) ;
                        while ( subItr.more() )
                        {
                           BSONElement clEle = subItr.next() ;
                           subCLNameList.push_back( clEle.valuestrsafe() ) ;
                        }
                     }
                     else
                     {
                        matchBuilder.append( ele ) ;
                     }
                  }
                  matcher = matchBuilder.done() ;

                  SDB_ASSERT( !subCLNameList.empty(), "sub cl name array is empty" ) ;
                  if ( subCLNameList.size() == 1 )
                  {
                     const CHAR *subCLName = subCLNameList.at( 0 ) ;
                     string mappingName ;
                     rc = pResource->getOrUpdateCataInfo( subCLName, cataInfo, cb ) ;
                     PD_RC_CHECK( rc, PDERROR, "Get catalogue information of "
                                 "collection[%s] failed[%d]", subCLName, rc ) ;
                     if ( cataInfo->getMappingName().size() > 0 )
                     {
                        mappingName = cataInfo->getMappingName() ;
                        subCLName = mappingName.c_str() ;
                     }

                     rc = msgBuildQueryMsg( &newMsg, &msgLen, subCLName,
                                            origQuery->flags,
                                            origQuery->header.requestID,
                                            origQuery->numToSkip,
                                            origQuery->numToReturn,
                                            &matcher, &selector, &orderBy, &hint, cb ) ;
                     PD_RC_CHECK( rc, PDERROR, "Build query message failed[%d]",
                                  rc ) ;
                  }
                  else
                  {
                     string dsMainCLName ;
                     const CHAR *subCLName = subCLNameList.at( 0 ) ;
                     rc = pResource->getOrUpdateCataInfo( subCLName, cataInfo, cb ) ;
                     PD_RC_CHECK( rc, PDERROR, "Get catalogue information of "
                                  "collection[%s] failed[%d]", subCLName, rc ) ;
                     dsMainCLName = cataInfo->getDSMainCLName() ;

                     rc = _buildSubCLInfoHint( subCLNameList, hintBuilder, cb ) ;
                     PD_RC_CHECK( rc, PDERROR,
                                  "Failed to build sub collection info hint, rc: %d",
                                  rc ) ;
                     hintBuilder.appendElements( hint ) ;
                     newHint = hintBuilder.obj() ;

                     rc = msgBuildQueryMsg( &newMsg, &msgLen, dsMainCLName.c_str(),
                                            origQuery->flags,
                                            origQuery->header.requestID,
                                            origQuery->numToSkip,
                                            origQuery->numToReturn,
                                            &matcher, &selector, &orderBy, &newHint, cb ) ;
                     PD_RC_CHECK( rc, PDERROR, "Build query message failed[%d]",
                                  rc ) ;
                  }
               }
            }
            else
            {
               INT32 flags = 0 ;
               const CHAR *clName = NULL ;
               INT64 numToSkip = 0 ;
               INT64 numToReturn = 0 ;
               const CHAR *query = NULL ;
               const CHAR *selector = NULL ;
               const CHAR *orderBy = NULL ;
               const CHAR *hint = NULL ;

               rc = pResource->getOrUpdateCataInfo( origQuery->name,
                                                    cataInfo, cb ) ;
               PD_RC_CHECK( rc, PDERROR, "Get catalogue information of "
                            "collection[%s] failed[%d]", origQuery->name, rc ) ;
               if ( 0 != ossStrcmp( origQuery->name,
                                    cataInfo->getMappingName().c_str() ) )
               {
                  rc = msgExtractQuery( (const CHAR *)msg, &flags, &clName,
                                        &numToSkip, &numToReturn,
                                        &query, &selector,
                                        &orderBy, &hint ) ;
                  PD_RC_CHECK( rc, PDERROR, "Extract query message failed[%d]",
                               rc ) ;
                  {
                     BSONObj queryObj( query ) ;
                     BSONObj selectorObj( selector ) ;
                     BSONObj orderByObj( orderBy ) ;
                     BSONObj hintObj( hint ) ;
                     rc = msgBuildQueryMsg( &newMsg, &msgLen,
                                            cataInfo->getMappingName().c_str(),
                                            flags, origQuery->header.requestID,
                                            numToSkip, numToReturn,
                                            &queryObj, &selectorObj,
                                            &orderByObj, &hintObj, cb ) ;
                     PD_RC_CHECK( rc, PDERROR, "Build query message failed[%d]",
                                  rc ) ;
                  }
               }
            }
            if ( newMsg )
            {
               UINT32 newMsgLen = ( (MsgHeader *)newMsg )->messageLength ;
               *(MsgHeader *)newMsg = *msg ;
               ((MsgHeader *)newMsg)->messageLength = newMsgLen ;
               pSub->setReqMsg( (MsgHeader *)newMsg, PMD_EDU_MEM_THREAD ) ;
               newMsg = NULL ;
            }
         }
         catch ( std::exception &e )
         {
            PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
            rc = ossException2RC( &e ) ;
            goto error ;
         }
      }

   done:
      if ( newMsg )
      {
         msgReleaseBuffer( newMsg ) ;
      }
      PD_TRACE_EXITRC( SDB__COORDDSMSGCONVERTOR__REBUILDDATASOURCEQUERYMSG,
                       rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__COORDDSMSGCONVERTOR__PARSEQUERYMSG, "_coordDSMsgConvertor::_parseQueryMsg" )
   INT32 _coordDSMsgConvertor::_parseQueryMsg( pmdSubSession *pSub,
                                               INT32 *flag, const
                                               CHAR **cmdName,
                                               const CHAR **query,
                                               const CHAR **selector,
                                               const CHAR **order,
                                               const CHAR **hint )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__COORDDSMSGCONVERTOR__PARSEQUERYMSG ) ;
      MsgOpQuery *queryMsg = (MsgOpQuery *)pSub->getReqMsg() ;
      netIOVec *dataVec = pSub->getIODatas() ;
      netIOVec::const_iterator itr = dataVec->begin() ;

      try
      {
         if ( dataVec->size() > 1 )
         {
            ++itr ;
            if ( query )
            {
               *query = (CHAR *)itr->iovBase ;
            }

            if ( dataVec->size() > 2 )
            {
               UINT32 offset = 0 ;
               ++itr ;
               if ( selector )
               {
                  *selector = (CHAR *)itr->iovBase ;
               }
               // The first 4 bytes at the beginning of the object is its
               // length.
               offset += *(UINT32 *)itr->iovBase ;
               offset = ossRoundUpToMultipleX( offset, 4 ) ;
               if ( order )
               {
                  *order = (CHAR *)itr->iovBase + offset ;
               }
               offset += *(UINT32 *)( ( CHAR *)itr->iovBase + offset ) ;
               offset = ossRoundUpToMultipleX( offset, 4 ) ;
               if ( hint )
               {
                  *hint = (CHAR *)itr->iovBase + offset ;
               }
            }
            else
            {
               if ( selector )
               {
                  *selector = NULL ;
               }
               if ( order )
               {
                  *order = NULL ;
               }
               if ( hint )
               {
                  *hint = NULL ;
               }
            }
            if ( flag )
            {
               *flag = queryMsg->flags ;
            }
            if ( cmdName )
            {
               *cmdName = queryMsg->name ;
            }
         }
         else
         {
            // The message is in a continous buffer. It can be parsed directly
            // by msgExtraceQuery.
            rc = msgExtractQuery( (const CHAR *)queryMsg, flag, cmdName,
                                  NULL, NULL,
                                  query, selector, order, hint ) ;
            PD_RC_CHECK( rc, PDERROR, "Extract query message failed[%d]", rc ) ;
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__COORDDSMSGCONVERTOR__PARSEQUERYMSG, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__COORDDSMSGCONVERTOR__REBUILDDATASOURCECMDMSG, "_coordDSMsgConvertor::_rebuildDataSourceCmdMsg" )
   INT32 _coordDSMsgConvertor::_rebuildDataSourceCmdMsg( pmdSubSession *pSub,
                                                         _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__COORDDSMSGCONVERTOR__REBUILDDATASOURCECMDMSG ) ;
      INT32 flag = 0 ;
      const CHAR *cmdName = NULL ;
      const CHAR *query = NULL ;
      const CHAR *selector = NULL ;
      const CHAR *orderby = NULL ;
      const CHAR *hint = NULL ;
      CHAR *newMsg = NULL ;
      INT32 buffSize = 0 ;
      BOOLEAN targetInQuery = FALSE ;
      BOOLEAN targetInHint = FALSE ;
      BOOLEAN hintToQuery = FALSE ;

      CoordCataInfoPtr cataInfo ;
      CoordDataSourcePtr dsPtr ;

      coordResource *pResource = sdbGetCoordCB()->getResource() ;
      MsgHeader *msg = pSub->getReqMsg() ;

      rc = _parseQueryMsg( pSub, &flag, &cmdName, &query, &selector,
                           &orderby, &hint ) ;
      PD_RC_CHECK( rc, PDERROR, "Parse query message failed[%d]. Message: %s",
                   rc, msg2String( msg ).c_str() ) ;

#ifdef _DEBUG
      PD_LOG( PDDEBUG, "Before convert, the message info is: query: %s, "
              "selector: %s, order: %s, hint: %s",
              query ? BSONObj(query).toString().c_str() : "",
              selector ? BSONObj(selector).toString().c_str() : "",
              orderby ? BSONObj(orderby).toString().c_str() : "",
              hint ? BSONObj(hint).toString().c_str() : "" ) ;
#endif /* _DEBUG */

      try
      {
         // The collection which the operation is original operated on. It may
         // be a normal collection or a main collection.
         const CHAR *clName = NULL ;
         // Name of sub collection, if the original operation is on a main
         // collection.
         ossPoolVector< const CHAR * > subCLNameList ;
         const CHAR *targetName = NULL ;
         string mappingName ;
         string dsMainCLName ;
         BOOLEAN doOnMainCL = FALSE ;
         BSONObj origQueryObj( query ) ;
         BSONObj queryObj( query ) ;
         BSONObj hintObj( hint ) ;
         BSONObjBuilder queryBuilder ;
         BSONObjBuilder hintBuilder ;
         BSONObjIterator queryItr( origQueryObj ) ;
         while ( queryItr.more() )
         {
            BSONElement ele = queryItr.next() ;
            // If field SubCLName exists in the query, it's a command on the
            // main collection.
            if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_SUBCLNAME ) )
            {
               // Currently we only support one data source sub collectioin
               // attached on a local main collection.
               BSONObjIterator subItr( ele.embeddedObject()) ;
               while ( subItr.more() )
               {
                  subCLNameList.push_back( subItr.next().valuestr() ) ;
               }
               doOnMainCL = TRUE ;
            }
            else if ( 0 == ossStrcmp( ele.fieldName(),
                                      FIELD_NAME_COLLECTION ) )
            {
               clName = ele.valuestr() ;
               targetInQuery = TRUE ;
            }
            else
            {
               // Collect all other fields except 'Collection' and 'SubCLName'.
               queryBuilder.append( ele ) ;
            }
         }

         if ( !clName && hintObj.hasField( FIELD_NAME_COLLECTION ) )
         {
            // If field name is not in the query, it should be in the hint.
            BSONObjIterator hintItr( hintObj ) ;
            while ( hintItr.more() )
            {
               BSONElement ele = hintItr.next() ;
               if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_COLLECTION ) )
               {
                  clName = ele.valuestr() ;
                  targetInHint = TRUE ;
               }
            }
         }

         {
            BSONObjIterator hintItr( hintObj ) ;
            while ( hintItr.more() )
            {
               BSONElement ele = hintItr.next() ;
               if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_COLLECTION ) &&
                    targetInHint )
               {
                  // This field may be moved into query object. Ignore it now.
                  continue ;
               }
               else if ( 0 == ossStrcmp( cmdName,
                                         CMD_ADMIN_PREFIX CMD_NAME_GET_CL_DETAIL ) &&
                         0 == ossStrcmp( ele.fieldName(), FIELD_NAME_HINT ) )
               {
                  BSONObjBuilder subBuilder( hintBuilder.subobjStart( FIELD_NAME_HINT ) ) ;
                  BSONObjIterator subItr( ele.Obj() ) ;
                  while ( subItr.more() )
                  {
                     BSONElement subEle = subItr.next() ;
                     if ( 0 == ossStrcmp( subEle.fieldName(), FIELD_NAME_AGGR ) )
                     {
                        // Always set aggr as true, because data source is one
                        // virtual data group.
                        continue ;
                     }
                     subBuilder.append( subEle ) ;
                  }
                  subBuilder.appendBool( FIELD_NAME_AGGR, TRUE ) ;
                  subBuilder.done() ;
               }
               else
               {
                  hintBuilder.append( ele ) ;
               }
            }
         }

         // If there is no field 'Collection' neither in query nor hint, it's
         // not a command on collection. So no change to the message will be
         // done.
         if ( !clName )
         {
            goto done ;
         }

         if ( doOnMainCL )
         {
            SDB_ASSERT( !subCLNameList.empty(), "sub cl name array is empty" ) ;
            if ( subCLNameList.size() == 1 )
            {
               clName = subCLNameList.at( 0 ) ;

               // Get information of the collection. It may be a normal collection or
               // a sub collection.
               rc = pResource->getOrUpdateCataInfo( clName, cataInfo, cb ) ;
               PD_RC_CHECK( rc, PDERROR, "Get catalogue information of "
                           "collection[%s] failed[%d]", clName, rc ) ;

               // There are two scenarios that the message needs to be rebuilt:
               // 1. The operation is on the main collection.
               // 2. The operation target name and the mapping name are not the same.
               if ( cataInfo->getMappingName().size() > 0 )
               {
                  mappingName = cataInfo->getMappingName() ;
                  targetName = mappingName.c_str() ;
               }
               else
               {
                  targetName = clName ;
               }
            }
            else
            {
               const CHAR *subCLName = subCLNameList.at( 0 ) ;
               rc = pResource->getOrUpdateCataInfo( subCLName, cataInfo, cb ) ;
               PD_RC_CHECK( rc, PDERROR, "Get catalogue information of "
                            "collection[%s] failed[%d]", subCLName, rc ) ;
               dsMainCLName = cataInfo->getDSMainCLName() ;

               rc = _buildSubCLInfoHint( subCLNameList, hintBuilder, cb ) ;
               PD_RC_CHECK( rc, PDERROR,
                            "Failed to build sub collection info hint, rc: %d",
                            rc ) ;
               targetName = dsMainCLName.c_str() ;
            }
         }
         else
         {
            rc = pResource->getOrUpdateCataInfo( clName, cataInfo, cb ) ;
            PD_RC_CHECK( rc, PDERROR, "Get catalogue information of "
                        "collection[%s] failed[%d]", clName, rc ) ;
            if ( cataInfo->getMappingName().size() > 0 )
            {
               mappingName = cataInfo->getMappingName() ;
               targetName = mappingName.c_str() ;
            }
            else
            {
               targetName = clName ;
            }
         }

         if ( 0 == ossStrcmp( cmdName,
                              CMD_ADMIN_PREFIX CMD_NAME_LIST_LOBS ) )
         {
            UTIL_DS_UID dsID = UTIL_INVALID_DS_UID ;
            dsID = SDB_GROUPID_2_DSID( pSub->getNodeID().columns.groupID ) ;
            rc = pResource->getDSManager()->getOrUpdateDataSource( dsID,
                                                                   dsPtr,
                                                                   cb ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Get data source[%u] failed, rc: %d",
                       dsID, rc ) ;
               goto error ;
            }

            // old version ( < 3.2.4 ) use query to send collection name
            if ( ( dsPtr->getDSMajorVersion() <= 2 ) ||
                 ( ( dsPtr->getDSMajorVersion() == 3 ) &&
                   ( ( dsPtr->getDSMinorVersion() < 2 ) ||
                     ( ( dsPtr->getDSMinorVersion() == 2 ) &&
                       ( dsPtr->getDSFixVersion() < 4 ) ) ) ) )
            {
               hintToQuery = TRUE ;
            }
         }

         if ( doOnMainCL || hintToQuery ||
              ( 0 != ossStrcmp( clName, targetName ) ) )
         {
            BSONObj selectObj( selector ) ;
            BSONObj orderbyObj( orderby ) ;

            if ( targetInQuery || hintToQuery )
            {
               queryBuilder.append( FIELD_NAME_COLLECTION, targetName ) ;
            }
            else if ( targetInHint )
            {
               hintBuilder.append( FIELD_NAME_COLLECTION, targetName ) ;
            }

            queryObj = queryBuilder.done() ;
            hintObj = hintBuilder.done() ;

            rc = msgBuildQueryCMDMsg( &newMsg, &buffSize, cmdName, queryObj,
                                      selectObj, orderbyObj, hintObj,
                                      0, cb ) ;
            PD_RC_CHECK( rc, PDERROR, "Build command message failed[%d]", rc ) ;

            {
               MsgOpQuery *pQueryMsg = (MsgOpQuery *)msg ;
               UINT32 newLen = ((MsgHeader *)newMsg)->messageLength ;
               *(MsgHeader *)newMsg = *msg ;
               ((MsgHeader *)newMsg)->messageLength = newLen ;
               ((MsgOpQuery *)newMsg)->flags = pQueryMsg->flags ;
               ((MsgOpQuery *)newMsg)->numToSkip = pQueryMsg->numToSkip ;
               ((MsgOpQuery *)newMsg)->numToReturn = pQueryMsg->numToReturn ;
            }

            PD_LOG( PDDEBUG, "After conversion, the message for data source "
                    "is: %s", msg2String( (MsgHeader *)newMsg).c_str() ) ;

            pSub->setReqMsg( (MsgHeader *)newMsg, PMD_EDU_MEM_THREAD ) ;
            newMsg = NULL ;
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      if ( newMsg )
      {
         msgReleaseBuffer( newMsg ) ;
      }
      PD_TRACE_EXITRC( SDB__COORDDSMSGCONVERTOR__REBUILDDATASOURCECMDMSG, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__COORDDSMSGCONVERTOR__REBUILDDATASOURCELOBMSG, "_coordDSMsgConvertor::_rebuildDataSourceLobMsg" )
   INT32 _coordDSMsgConvertor::_rebuildDataSourceLobMsg( pmdSubSession *pSub,
                                                         _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__COORDDSMSGCONVERTOR__REBUILDDATASOURCELOBMSG ) ;
      const MsgHeader *pOrgMsg = NULL ;
      ISession *pSession = NULL ;
      IClient *pClient = NULL ;
      MsgOpLob *pNewMsg = NULL ;

      /// get orignal message
      if ( cb )
      {
         pSession = cb->getSession() ;
         if ( pSession )
         {
            pClient = pSession->getClient() ;
            if ( pClient )
            {
               pOrgMsg = pClient->getInMsg() ;
            }
         }
      }

      if ( pOrgMsg )
      {
         MsgOpLob *pInLobMsg = ( MsgOpLob* )pOrgMsg ;
         UINT32 inMsgSize = (UINT32)( pOrgMsg->messageLength ) ;
         UINT32 inMetaSize = pInLobMsg->bsonLen ;

         UINT32 newMsgSize = 0, newMetaSize = 0 ;
         BSONObj newMeta ;

         MsgOpLob *pReqMsg = (MsgOpLob *)pSub->getReqMsg() ;
         BSONElement reqOIDEle, reqSubCLEle ;

         CoordDataSourcePtr dsPtr ;
         coordResource *pResource = sdbGetCoordCB()->getResource() ;
         UTIL_DS_UID dsID = UTIL_INVALID_DS_UID ;
         dsID = SDB_GROUPID_2_DSID( pSub->getNodeID().columns.groupID ) ;

         rc = pResource->getDSManager()->getOrUpdateDataSource( dsID,
                                                                dsPtr,
                                                                cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Get data source[%u] failed, rc: %d",
                    dsID, rc ) ;
            goto error ;
         }

         // for 2.x, do not support truncate LOB
         if ( pOrgMsg->opCode == MSG_BS_LOB_TRUNCATE_REQ &&
              dsPtr->getDSMajorVersion() <= 2 )
         {
            rc = SDB_OPERATION_INCOMPATIBLE ;
            PD_LOG( PDWARNING, "Failed to truncate LOB in "
                    "old version data source which is not supported" ) ;
            goto error ;
         }

         if ( pReqMsg->bsonLen > 0 )
         {
            netIOVec *dataVec = pSub->getIODatas() ;
            if ( NULL != dataVec && dataVec->size() > 1 )
            {
               netIOV &iov = ( *dataVec )[ 1 ] ;
               BSONObj reqMeta( (const CHAR *)( iov.iovBase ) ) ;
               reqOIDEle = reqMeta.getField( FIELD_NAME_LOB_OID ) ;
               reqSubCLEle = reqMeta.getField( FIELD_NAME_SUBCLNAME ) ;
            }
         }

         if ( inMetaSize > 0 )
         {
            try
            {
               BSONObjBuilder metaBuilder ;
               BSONObj inMeta( (CHAR *)pOrgMsg + sizeof( MsgOpLob ) ) ;
               BSONObjIterator iter( inMeta ) ;
               BOOLEAN hasOID = FALSE, hasMode = FALSE ;
               while ( iter.more() )
               {
                  BSONElement element = iter.next() ;
                  if ( 0 == ossStrcmp( FIELD_NAME_COLLECTION,
                                       element.fieldName() ) )
                  {
                     CoordCataInfoPtr cataInfo ;
                     const CHAR *inCollection = element.valuestrsafe() ;
                     const CHAR *reqCollection = reqSubCLEle.valuestrsafe() ;

                     if ( '\0' != reqCollection[ 0 ] &&
                          0 != ossStrcmp( inCollection, reqCollection ) )
                     {
                        inCollection = reqCollection ;
                     }

                     rc = pResource->getOrUpdateCataInfo( inCollection,
                                                          cataInfo,
                                                          cb ) ;
                     PD_RC_CHECK( rc, PDERROR, "Failed to get catalog "
                                  "information of collection [%s], rc: %d",
                                  inCollection, rc ) ;
                     SDB_ASSERT( !cataInfo->getMappingName().empty(),
                                 "mapping name is invalid" ) ;

                     metaBuilder.append( FIELD_NAME_COLLECTION,
                                         cataInfo->getMappingName().c_str() ) ;
                  }
                  else
                  {
                     if ( 0 == ossStrcmp( FIELD_NAME_LOB_OID,
                                          element.fieldName() ) )
                     {
                        hasOID = TRUE ;
                     }
                     else if ( 0 == ossStrcmp( FIELD_NAME_LOB_OPEN_MODE,
                                          element.fieldName() ) )
                     {
                        hasMode = TRUE ;
                     }
                     metaBuilder.append( element ) ;
                  }
               }
               if ( !hasMode )
               {
                  if ( MSG_BS_LOB_REMOVE_REQ == pOrgMsg->opCode )
                  {
                     // need remove mode in meta data
                     metaBuilder.append( FIELD_NAME_LOB_OPEN_MODE,
                                         (INT32)SDB_LOB_MODE_REMOVE ) ;
                  }
                  else if ( MSG_BS_LOB_TRUNCATE_REQ == pOrgMsg->opCode )
                  {
                     // need truncate mode in meta data
                     metaBuilder.append( FIELD_NAME_LOB_OPEN_MODE,
                                         (INT32)SDB_LOB_MODE_TRUNCATE ) ;
                  }
               }
               if ( !hasOID && !reqOIDEle.eoo() )
               {
                  // pass OID to data source
                  metaBuilder.append( reqOIDEle ) ;
               }
               newMeta = metaBuilder.obj() ;
               newMetaSize = newMeta.objsize() ;
            }
            catch ( std::exception &e )
            {
               rc = ossException2RC( &e ) ;
               PD_LOG( PDERROR, "Unexpected exception occurred: %s",
                       e.what() ) ;
               goto error ;
            }

            newMsgSize = inMsgSize -
                         ossRoundUpToMultipleX( inMetaSize, 4 ) +
                         ossRoundUpToMultipleX( newMetaSize, 4 ) ;
         }
         else
         {
            newMsgSize = inMsgSize ;
            newMetaSize = 0 ;
         }

         pNewMsg = (MsgOpLob *)SDB_THREAD_ALLOC( newMsgSize ) ;
         PD_CHECK( NULL != pNewMsg, SDB_OOM, error, PDERROR,
                   "Failed to allocate new message with size [%d]",
                   newMsgSize ) ;
         ossMemset( (CHAR *)pNewMsg, 0, newMsgSize ) ;
         ossMemcpy( (CHAR *)pNewMsg, (CHAR *)pOrgMsg, sizeof( MsgOpLob ) ) ;

         pNewMsg->contextID = pReqMsg->contextID ;
         pNewMsg->header.TID = pReqMsg->header.TID ;
         pNewMsg->header.requestID = pReqMsg->header.requestID ;
         pNewMsg->header.routeID.value = pReqMsg->header.routeID.value ;

         if ( newMetaSize > 0 )
         {
            ossMemcpy( (CHAR *)pNewMsg + sizeof( MsgOpLob ),
                       newMeta.objdata(),
                       newMetaSize ) ;
            ossMemcpy( (CHAR *)pNewMsg +
                       sizeof( MsgOpLob ) +
                       ossRoundUpToMultipleX( newMetaSize, 4 ),
                       (CHAR *)pOrgMsg +
                       sizeof( MsgOpLob ) +
                       ossRoundUpToMultipleX( inMetaSize, 4 ),
                       inMsgSize -
                       sizeof( MsgOpLob ) -
                       ossRoundUpToMultipleX( inMetaSize, 4 ) ) ;
         }
         else
         {
            ossMemcpy( (CHAR *)pNewMsg + sizeof( MsgOpLob ),
                       (CHAR *)pOrgMsg + sizeof( MsgOpLob ),
                       inMsgSize - sizeof( MsgOpLob ) ) ;
         }

         pNewMsg->header.messageLength = newMsgSize ;
         pNewMsg->bsonLen = newMetaSize ;

         pSub->setReqMsg( (MsgHeader *)pNewMsg, PMD_EDU_MEM_THREAD ) ;

#if defined (_DEBUG)
         if ( MSG_BS_LOB_WRITE_REQ == pNewMsg->header.opCode )
         {
            const MsgOpLob *header = NULL ;
            UINT32 len = 0 ;
            INT64 offset = -1 ;
            const CHAR *data = NULL ;

            rc = msgExtractWriteLobRequest( (const CHAR*)pNewMsg, &header, &len,
                                            &offset, &data ) ;
            if ( SDB_OK == rc )
            {
               string md5sum = md5::md5simpledigest( data, len ) ;
               PD_LOG( PDDEBUG, "Convert write LOB, context: %lld, len: %u, "
                       "offset: %llu, md5sum: %s",
                       pNewMsg->contextID, len, offset, md5sum.c_str() ) ;
            }
            else
            {
               PD_LOG( PDERROR, "Failed to convert write LOB, rc: %d", rc ) ;
            }
         }
         else if ( MSG_BS_LOB_OPEN_REQ == pNewMsg->header.opCode )
         {
            const MsgOpLob *header = NULL ;
            BSONObj obj ;

            rc = msgExtractOpenLobRequest( (const CHAR*)pNewMsg, &header, obj ) ;
            if ( SDB_OK == rc )
            {
               PD_LOG( PDDEBUG, "Convert open LOB, meta: %s",
                       obj.toString().c_str() ) ;
            }
            else
            {
               PD_LOG( PDERROR, "Failed to convert open LOB, rc: %d", rc ) ;
            }
         }
#endif
         pNewMsg = NULL ;
      }

   done:
      if ( pNewMsg )
      {
         SDB_THREAD_FREE( pNewMsg ) ;
      }
      PD_TRACE_EXITRC( SDB__COORDDSMSGCONVERTOR__REBUILDDATASOURCELOBMSG, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _coordDSMsgConvertor::_isInternalOperation( RTN_COMMAND_TYPE type ) const
   {
      return ( CMD_GET_SESSIONATTR == type || CMD_SET_SESSIONATTR == type ) ;
   }

   BOOLEAN coordIsSpecialMsg( INT32 opCode )
   {
      BOOLEAN isSpecial = FALSE ;

      switch ( opCode )
      {
         case MSG_BS_GETMORE_REQ :
         case MSG_BS_ADVANCE_REQ :
         case MSG_BS_KILL_CONTEXT_REQ :
         case MSG_BS_DISCONNECT :
         case MSG_BS_INTERRUPTE :
         case MSG_BS_INTERRUPTE_SELF :
         case MSG_BS_LOB_CLOSE_REQ :
            isSpecial = TRUE ;
            break ;
         default :
            break ;
      }
      return isSpecial ;
   }

   BOOLEAN coordIsLobMsg( INT32 opCode )
   {
      return ( opCode > ( SINT32 )MSG_LOB_BEGIN &&
               opCode < ( SINT32 )MSG_LOB_END ) ;
   }

   /**
    * Command that just operates on the mapping itself. These commands are
    * always supported and will never be forwareded to data source.
    */
   BOOLEAN coordIsLocalMappingCmd( _rtnCommand *pCommand )
   {
      BOOLEAN result = FALSE ;

      switch ( pCommand->type() )
      {
      case CMD_DROP_COLLECTIONSPACE:
      case CMD_DROP_COLLECTION:
      case CMD_LINK_COLLECTION:
      case CMD_UNLINK_COLLECTION:
      case CMD_RENAME_COLLECTIONSPACE:
      case CMD_RENAME_COLLECTION:
      case CMD_ALTER_COLLECTION:
         result = TRUE ;
         break ;
      default:
         break ;
      }

      return result ;
   }
}
