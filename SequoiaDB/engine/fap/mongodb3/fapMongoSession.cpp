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

   Source File Name = fapMongoSession.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== ==============================================
          07/03/2021  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#include "fapMongoSession.hpp"
#include "pmdEDUMgr.hpp"
#include "pmdEnv.hpp"
#include "monCB.hpp"
#include "msg.hpp"
#include "../../bson/bson.hpp"
#include "rtnCommandDef.hpp"
#include "rtn.hpp"
#include "pmd.hpp"
#include "sdbInterface.hpp"
#include "fapMongoTrace.hpp"
#include "pdTrace.hpp"

using namespace engine ;

namespace fap
{

/*
   common define
*/
#define FAP_RECV_DATA_AFTER_LENGTH_TIMEOUT         ( 300 * OSS_ONE_SEC )

/*
   common functions
*/
static INT32 buildGetMoreSdbMsg( UINT64 requestID, INT64 contextID,
                                 mongoMsgBuffer &out )
{
   INT32 rc = SDB_OK ;
   MsgOpGetMore *pGetmore = NULL ;

   if ( !out.empty() )
   {
      out.zero() ;
   }

   rc = out.reserve( sizeof( MsgOpGetMore ) ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = out.advance( sizeof( MsgOpGetMore ) ) ;
   if ( rc )
   {
      goto error ;
   }

   pGetmore = (MsgOpGetMore *)out.data() ;
   mongoInitMsgHeader( &(pGetmore->header), MSG_BS_GETMORE_REQ, requestID ) ;

   pGetmore->header.messageLength = sizeof( MsgOpGetMore ) ;
   pGetmore->contextID = contextID ;
   pGetmore->numToReturn = -1 ;

done:
   return rc ;
error:
   goto done ;
}

/*
   _mongoSession implement
*/
_mongoSession::_mongoSession( SOCKET fd, engine::IResource *pResource )
: engine::pmdSession( fd ), _pResource( pResource ),
  _clFullName( NULL )
{
}

_mongoSession::~_mongoSession()
{
   _pResource = NULL ;
   _resetBuffers() ;
}

void _mongoSession::_resetBuffers()
{
   if ( 0 != _contextBuff.size() )
   {
      _contextBuff.release() ;
   }

   if ( !_inBuffer.empty() )
   {
      _inBuffer.zero() ;
   }

   _storeBuff.release() ;
}

INT32 _mongoSession::getServiceType() const
{
   return CMD_SPACE_SERVICE_LOCAL ;
}

engine::SDB_SESSION_TYPE _mongoSession::sessionType() const
{
   return engine::SDB_SESSION_PROTOCOL ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_PROCESSCLIENTMSG, "_mongoSession::_processClientMsg" )
INT32 _mongoSession::_processClientMsg( const CHAR* pMsg,
                                        _mongoCommand *&pCommand,
                                        mongoSessionCtx &sessCtx )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_PROCESSCLIENTMSG ) ;
   INT32 rc = SDB_OK ;
   BOOLEAN needNext = FALSE ;

   rc = mongoGetAndInitCommand( pMsg, &pCommand, sessCtx ) ;
   if ( rc )
   {
      goto error ;
   }

   /// set operation max time
   eduCB()->getOperator()->setMaxTime( sessCtx.maxTimeMS ) ;

   while ( TRUE )
   {
      needNext = FALSE ;
      rc = _processOwnedClientMsg( pMsg, _inBuffer, pCommand, sessCtx, needNext ) ;
      if ( rc )
      {
         if ( SDB_DMS_EOC != rc )
         {
            PD_LOG( PDERROR, "Session[%s] failed to process owned client "
                    "msg, rc: %d", sessionName(), rc ) ;
         }
         goto error ;
      }

      if ( needNext )
      {
         continue ;
      }

      break ;
   }

done:
   {
      /// return detach context before reply
      eduCB()->returnDetachContext() ;

      INT32 rcTmp = _reply( pCommand, pMsg, rc, sessCtx.errorObj ) ;
      if ( rcTmp )
      {
         PD_LOG( PDERROR, "Session[%s] failed to reply, rc: %d",
                 sessionName(), rcTmp ) ;
         disconnect() ;
         rc = rcTmp ;
      }
      else
      {
         rc = SDB_OK ;
      }
   }
   PD_TRACE_EXITRC( SDB_FAPMONGO_PROCESSCLIENTMSG, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_PROCESSOWNEDCLIENTMSG, "_mongoSession::_processOwnedClientMsg" )
INT32 _mongoSession::_processOwnedClientMsg( const CHAR* pMsg,
                                             mongoMsgBuffer &sdbMsgBuff,
                                             _mongoCommand *pCommand,
                                             mongoSessionCtx &sessCtx,
                                             BOOLEAN &needNext )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_PROCESSOWNEDCLIENTMSG ) ;
   INT32 rc = SDB_OK ;
   needNext = FALSE ;

   if ( pCommand->needProcessByEngine() )
   {
      BOOLEAN getMoreAll = FALSE ;

      sdbMsgBuff.zero() ;

      rc = pCommand->buildSdbRequest( sdbMsgBuff, sessCtx, getMoreAll ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to build sdb message for command[%s], rc: %d",
                   pCommand->name(), rc ) ;

      // double check
      // some commands must be parse the message to know whether it needs to be processed by engine,
      // such as the aggregate operation containing $indexStat
      if ( !pCommand->needProcessByEngine() )
      {
         goto done ;
      }

      // All mongo session's reply shoule be limited to a bson object, except getMoreAll = true
      if ( getMoreAll )
      {
         _pEDUCB->getOperator()->disableContextBatchSizeLimited() ;
      }
      else
      {
         _pEDUCB->getOperator()->enableContextBatchSizeLimited() ;
      }

      PD_LOG( PDDEBUG, "Build sdb msg[ tid: %d, session: %s, "
              "command: %s, clFullName: %s, eduID: %llu ] done",
              ossGetCurrentThreadID(),
              sessCtx.sessionName,
              pCommand->name() ? pCommand->name() : "",
              pCommand->clFullName() ? pCommand->clFullName() : "",
              sessCtx.eduID ) ;

      rc = _processMsg( sdbMsgBuff.data(), pCommand, getMoreAll, sessCtx.errorObj ) ;
      /// should parse the result
      INT32 rcTmp = pCommand->parseSdbReply( _replyHeader, _contextBuff, rc ) ;
      if ( rc )
      {
         goto error ;
      }
      else if ( rcTmp )
      {
         rc = rcTmp ;
         goto error ;
      }
   }

   if ( !pCommand->hasProcessAllMsg() )
   {
      needNext = TRUE ;
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_PROCESSOWNEDCLIENTMSG, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_RUN, "_mongoSession::run" )
INT32 _mongoSession::run()
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_RUN ) ;
   INT32 rc                = SDB_OK ;
   CHAR *pMsg              = NULL ;
   _mongoCommand* pCommand = NULL ;
   mongoSessionCtx sessCtx ;
   pmdEDUMgr *pmdEDUMgr    = NULL ;
   monDBCB *mondbcb        = pmdGetKRCB()->getMonDBCB () ;

   PD_CHECK( _pEDUCB, SDB_SYS, error, PDERROR,
             "_pEDUCB is null" ) ;
   PD_CHECK( FALSE == mongoCheckBigEndian(), SDB_SYS, error, PDERROR,
             "Big endian is not support" ) ;

   pmdEDUMgr = _pEDUCB->getEDUMgr() ;

   while ( !_pEDUCB->isDisconnected() && !_socket.isClosed() )
   {
      // clear interrupt flag
      _pEDUCB->resetInterrupt() ;
      _pEDUCB->resetInfo( engine::EDU_INFO_ERROR ) ;
      _pEDUCB->resetLsn() ;
      pdClearLastError() ;

      _resetBuffers() ;
      sessCtx.resetError() ;
      sessCtx.sessionName = sessionName() ;
      sessCtx.eduID = eduID() ;

      if ( pCommand )
      {
         mongoReleaseCommand( &pCommand ) ;
      }

      /// recv message
      rc = _recvMsgFromClient( pMsg ) ;
      if ( rc )
      {
         if ( SDB_NETWORK_CLOSE == rc )
         {
            rc = SDB_OK ;
         }
         break ;
      }

      /// update conf should here
      _pEDUCB->updateConf() ;

      // increase process event count
      _pEDUCB->incEventCount() ;
      mondbcb->addReceiveNum() ;

      // activate edu
      if ( SDB_OK != ( rc = pmdEDUMgr->activateEDU( _pEDUCB ) ) )
      {
         PD_LOG( PDERROR, "Session[%s] activate edu failed, rc: %d",
                 sessionName(), rc ) ;
         break ;
      }

      /// process message
      rc = _processClientMsg( pMsg, pCommand, sessCtx ) ;
      if ( rc )
      {
         break ;
      }

      // wait edu
      if ( SDB_OK != ( rc = pmdEDUMgr->waitEDU( _pEDUCB ) ) )
      {
         PD_LOG( PDERROR, "Session[%s] wait edu failed, rc: %d",
                 sessionName(), rc ) ;
         break ;
      }
   }

done:
   if ( pCommand )
   {
      mongoReleaseCommand( &pCommand ) ;
   }
   disconnect() ;
   PD_TRACE_EXITRC( SDB_FAPMONGO_RUN, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_RECVMSGFROMCLIENT, "_mongoSession::_recvMsgFromClient" )
INT32 _mongoSession::_recvMsgFromClient( CHAR *&pMsg )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY( SDB_FAPMONGO_RECVMSGFROMCLIENT ) ;
   UINT32 msgSize = 0 ;
   INT32 hasReceived = 0 ;

   /// first recieved message size
   rc = recvData( (CHAR*)&msgSize, sizeof(UINT32) ) ;
   if ( rc )
   {
      if ( SDB_NETWORK_CLOSE == rc )
      {
         /// peer disconnect
         PD_LOG( PDINFO, "Session[%s] peer disconnect", sessionName() ) ;
      }
      else if ( SDB_APP_FORCED != rc )
      {
         PD_LOG( PDERROR, "Session[%s] failed to recv msg size, "
                 "rc: %d", sessionName(), rc ) ;
      }
      goto error ;
   }

   /// check message size
   PD_CHECK( msgSize >= sizeof( mongoMsgHeader ) && msgSize <= SDB_MAX_MSG_LENGTH,
             SDB_INVALIDARG, error, PDERROR,
             "Session[%s] receive message size[%d] is invalid",
             sessionName(), msgSize ) ;

   // alloc memory
   pMsg = getBuff( msgSize + 1 ) ;
   PD_CHECK( pMsg, SDB_OOM, error, PDERROR, "Out of memory" ) ;

   *(UINT32*)pMsg = msgSize ;

   // receive rest of message
   rc = recvData( pMsg + sizeof(UINT32), msgSize - sizeof(UINT32),
                  FAP_RECV_DATA_AFTER_LENGTH_TIMEOUT, TRUE, &hasReceived ) ;
   if ( rc )
   {
      if ( SDB_APP_FORCED != rc )
      {
         PD_LOG( PDERROR, "Session[%s] failed to recv msg[len: %u, "
                 "recieved: %d], rc: %d",
                 sessionName(), msgSize - sizeof(UINT32),
                 hasReceived, rc ) ;
      }
      goto error ;
   }
   pMsg[ msgSize ] = 0 ;

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_RECVMSGFROMCLIENT, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_AUTOCREATECS, "_mongoSession::_autoCreateCS" )
INT32 _mongoSession::_autoCreateCS( const CHAR *pCsName, BSONObj &errorObj )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_AUTOCREATECS ) ;
   INT32 rc             = SDB_OK ;
   MsgOpQuery *pQuery   = NULL ;
   const CHAR *pCmdName = CMD_ADMIN_PREFIX CMD_NAME_CREATE_COLLECTIONSPACE ;
   BSONObj obj, empty ;

   _tmpBuffer.zero() ;

   rc = _tmpBuffer.reserve( sizeof( MsgOpQuery ) ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = _tmpBuffer.advance( sizeof( MsgOpQuery ) - 4 ) ;
   if ( rc )
   {
      goto error ;
   }

   pQuery = ( MsgOpQuery * )_tmpBuffer.data() ;
   mongoInitMsgHeader( &(pQuery->header), MSG_BS_QUERY_REQ ) ;

   pQuery->version = 0 ;
   pQuery->w = 0 ;
   pQuery->padding = 0 ;
   pQuery->flags = 0 ;
   pQuery->numToSkip = 0 ;
   pQuery->numToReturn = -1 ;
   pQuery->nameLength = ossStrlen( pCmdName ) ;

   try
   {
      obj = BSON( FIELD_NAME_NAME << pCsName << FIELD_NAME_PAGE_SIZE << 65536 ) ;
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when building sdb createCS "
              "request: %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

   rc = _tmpBuffer.write( pCmdName, pQuery->nameLength + 1, TRUE ) ;
   if ( rc )
   {
      goto error ;
   }

   /// query
   rc = _tmpBuffer.write( obj, TRUE ) ;
   if ( rc )
   {
      goto error ;
   }
   /// selector
   rc = _tmpBuffer.write( empty, TRUE ) ;
   if ( rc )
   {
      goto error ;
   }
   /// orderby
   rc = _tmpBuffer.write( empty, TRUE ) ;
   if ( rc )
   {
      goto error ;
   }
   /// hint
   rc = _tmpBuffer.write( empty, TRUE ) ;
   if ( rc )
   {
      goto error ;
   }

   _tmpBuffer.doneLen() ;

   rc = _processMsg( (const CHAR*)pQuery, errorObj, _contextBuff, _replyHeader ) ;
   if ( SDB_OK == rc )
   {
      PD_LOG( PDEVENT,
              "Session[%s]: Create collection space[%s] automatically",
              sessionName(), pCsName ) ;
   }
   else
   {
      PD_LOG( PDWARNING,
              "Session[%s]: Failed to create collection space[%s] automatically"
              ", rc: %d", sessionName(), pCsName, rc ) ;
      if ( SDB_DMS_CS_EXIST == rc )
      {
         rc = SDB_OK ;
         _clearErrorInfo( errorObj ) ;
      }
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_AUTOCREATECS, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_AUTOINSERT, "_mongoSession::_autoInsert" )
INT32 _mongoSession::_autoInsert( const CHAR *pClFullName,
                                  const BSONObj &matcher,
                                  const BSONObj &updatorObj,
                                  const BSONObj &setOnInsert,
                                  BSONObj &target,
                                  BSONObj &errorObj )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_AUTOINSERT ) ;
   INT32 rc = SDB_OK ;
   MsgOpInsert *pInsert = NULL ;

   rc = mongoGenerateNewRecord( matcher, updatorObj, setOnInsert, target ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to generate new record, rc: %d", rc ) ;
      goto error ;
   }

   _tmpBuffer.zero() ;

   rc = _tmpBuffer.reserve( sizeof( MsgOpInsert ) ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = _tmpBuffer.advance( sizeof( MsgOpInsert ) - 4 ) ;
   if ( rc )
   {
      goto error ;
   }

   pInsert = ( MsgOpInsert *)_tmpBuffer.data() ;
   mongoInitMsgHeader( &(pInsert->header), MSG_BS_INSERT_REQ ) ;
   pInsert->version = 0 ;
   pInsert->w = 0 ;
   pInsert->padding = 0 ;
   pInsert->flags = FLG_INSERT_RETURNNUM | FLG_INSERT_HAS_ID_FIELD ;

   pInsert->nameLength = ossStrlen( pClFullName ) ;

   rc = _tmpBuffer.write( pClFullName, pInsert->nameLength + 1, TRUE ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = _tmpBuffer.write( target, TRUE ) ;
   if ( rc )
   {
      goto error ;
   }

   _tmpBuffer.doneLen() ;

   rc = _processMsg( (CHAR*)pInsert, errorObj, _contextBuff, _replyHeader ) ;
   if ( rc )
   {
      PD_LOG( PDERROR,
              "Session[%s]: failed to insert automatically"
              ", rc: %d", sessionName(), rc ) ;
      goto error ;
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_AUTOINSERT, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_AUTOCEATECL, "_mongoSession::_autoCreateCL" )
INT32 _mongoSession::_autoCreateCL( const CHAR *pCSName,
                                    const CHAR *pClFullName,
                                    BSONObj &errorObj )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_AUTOCEATECL ) ;
   INT32 rc             = SDB_OK ;
   MsgOpQuery *pQuery   = NULL ;
   const CHAR *pCmdName = CMD_ADMIN_PREFIX CMD_NAME_CREATE_COLLECTION ;
   BSONObj obj, empty ;

   while( TRUE )
   {
      _tmpBuffer.zero() ;

      rc = _tmpBuffer.reserve( sizeof( MsgOpQuery ) ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = _tmpBuffer.advance( sizeof( MsgOpQuery ) - 4 ) ;
      if ( rc )
      {
         goto error ;
      }

      pQuery = ( MsgOpQuery * )_tmpBuffer.data() ;
      mongoInitMsgHeader( &(pQuery->header), MSG_BS_QUERY_REQ ) ;
      pQuery->version = 0 ;
      pQuery->w = 0 ;
      pQuery->padding = 0 ;
      pQuery->flags = 0 ;
      pQuery->nameLength = ossStrlen( pCmdName ) ;
      pQuery->numToSkip = 0 ;
      pQuery->numToReturn = -1 ;

      try
      {
         obj = BSON( FIELD_NAME_NAME << pClFullName ) ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "An exception occurred when building sdb createCL "
                 "request: %s, rc: %d", e.what(), rc ) ;
         goto error ;
      }

      rc = _tmpBuffer.write( pCmdName, pQuery->nameLength + 1, TRUE ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = _tmpBuffer.write( obj, TRUE ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = _tmpBuffer.write( empty, TRUE ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = _tmpBuffer.write( empty, TRUE ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = _tmpBuffer.write( empty, TRUE ) ;
      if ( rc )
      {
         goto error ;
      }

      _tmpBuffer.doneLen() ;

      rc = _processMsg( (CHAR*)pQuery, errorObj, _contextBuff, _replyHeader ) ;
      if ( SDB_DMS_CS_NOTEXIST == rc )
      {
         rc = _autoCreateCS( pCSName, errorObj ) ;
         if ( rc )
         {
            break ;
         }
      }
      else
      {
         break ;
      }
   }

   if ( SDB_OK == rc )
   {
      PD_LOG( PDEVENT,
              "Session[%s]: Create collection[%s] automatically",
              sessionName(), pClFullName ) ;
   }
   else
   {
      PD_LOG( PDWARNING,
              "Session[%s]: Failed to create collection[%s] automatically"
              ", rc: %d", sessionName(), pClFullName, rc ) ;
      if ( SDB_DMS_EXIST == rc )
      {
         rc = SDB_OK ;
         _clearErrorInfo( errorObj ) ;
      }
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_AUTOCEATECL, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_AUTOKILLCURRSOR, "_mongoSession::_autoKillCursor" )
INT32 _mongoSession::_autoKillCursor( UINT64 requestID, INT64 contextID )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_AUTOKILLCURRSOR ) ;
   INT32 rc = SDB_OK ;
   MsgOpKillContexts *pKill = NULL ;
   BSONObj errorObj ;
   engine::rtnContextBuf buff ;
   MsgOpReply tmpReplyHeader ;

   _tmpBuffer.zero() ;

   rc = _tmpBuffer.reserve( sizeof( MsgOpKillContexts ) ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = _tmpBuffer.advance( sizeof( MsgOpKillContexts ) - sizeof( SINT64 ) ) ;
   if ( rc )
   {
      goto error ;
   }

   pKill = ( MsgOpKillContexts * )_tmpBuffer.data() ;
   mongoInitMsgHeader( &(pKill->header), MSG_BS_KILL_CONTEXT_REQ, requestID ) ;
   pKill->ZERO = 0 ;
   pKill->numContexts = 1 ;

   rc = _tmpBuffer.write( (CHAR*)&contextID, sizeof( SINT64 ) ) ;
   if ( rc )
   {
      goto error ;
   }

   _tmpBuffer.doneLen() ;

   rc = _processMsg( (const CHAR*)pKill, errorObj, buff, tmpReplyHeader ) ;
   if ( rc )
   {
      PD_LOG( PDWARNING,
              "Session[%s]: Failed to kill cursor[cursorID: %lld] "
              "automatically, rc: %d",
              sessionName(), contextID, rc ) ;
      goto error ;
   }
   else
   {
      PD_LOG( PDINFO, "Session[%s] kill cursor[cursorID: %lld] "
              "automatically", sessionName(), contextID ) ;
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_AUTOKILLCURRSOR, rc ) ;
   return rc ;
error:
   goto done ;
}

BOOLEAN _mongoSession::_shouldAutoCrtCS( const _mongoCommand *pCommand )
{
   SDB_ASSERT( pCommand != NULL , "pCommand can't be NULL!" ) ;

   MONGO_CMD_TYPE cmdType = pCommand->type() ;
   BOOLEAN isUpsert = ( CMD_UPDATE == cmdType &&
                        ((_mongoUpdateCommand*)pCommand)->isUpsert() ) ;
   BOOLEAN isFindAndUpsert = ( CMD_FINDANDMODIFY == cmdType &&
                        ((_mongoFindandmodifyCommand*)pCommand)->isUpsert() ) ;

   if ( CMD_COLLECTION_CREATE == cmdType ||
        CMD_INSERT == cmdType ||
        CMD_INDEX_CREATE == cmdType ||
        isUpsert ||
        isFindAndUpsert )
   {
      return TRUE ;
   }
   else
   {
      return FALSE ;
   }
}

BOOLEAN _mongoSession::_shouldAutoCrtCL( const _mongoCommand *pCommand )
{
   SDB_ASSERT( pCommand != NULL , "pCommand can't be NULL!" ) ;

   MONGO_CMD_TYPE cmdType = pCommand->type() ;
   BOOLEAN isUpsert = ( CMD_UPDATE == cmdType &&
                        ((_mongoUpdateCommand*)pCommand)->isUpsert() ) ;
   BOOLEAN isFindAndUpsert = ( CMD_FINDANDMODIFY == cmdType &&
                        ((_mongoFindandmodifyCommand*)pCommand)->isUpsert() ) ;

   if ( CMD_INSERT == cmdType ||
        CMD_INDEX_CREATE == cmdType ||
        isUpsert ||
        isFindAndUpsert )
   {
      return TRUE ;
   }
   else
   {
      return FALSE ;
   }
}

BOOLEAN _mongoSession::_shouldBuildGetMoreMsg( const _mongoCommand *pCommand,
                                               const engine::rtnContextBuf &buf,
                                               const MsgOpReply &replyHeader )
{
   SDB_ASSERT( pCommand != NULL , "pCommand can't be NULL!" ) ;

   MONGO_CMD_TYPE cmdType = pCommand->type() ;

   if ( SDB_OK != replyHeader.flags ||
        SDB_INVALID_CONTEXTID == replyHeader.contextID ||
        buf.size() > 0 )
   {
      return FALSE ;
   }

   if ( CMD_COUNT     == cmdType || CMD_LIST_INDEX      == cmdType ||
        CMD_AGGREGATE == cmdType || CMD_LIST_COLLECTION == cmdType ||
        CMD_DISTINCT  == cmdType || CMD_LIST_DATABASE   == cmdType ||
        CMD_LIST_USER == cmdType || CMD_DB_STATS == cmdType ||
        CMD_COLL_STATS == cmdType || CMD_TOP == cmdType ||
        CMD_SERVER_STATUS == cmdType || CMD_CURRENT_OP == cmdType )
   {
      return TRUE ;
   }
   else
   {
      return FALSE ;
   }
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_PROCESSMSG1, "_mongoSession::_processMsg" )
INT32 _mongoSession::_processMsg( const CHAR *pMsg,
                                  const _mongoCommand *pCommand,
                                  BOOLEAN getMoreAll,
                                  BSONObj &errorObj )
{
   SDB_ASSERT( pCommand != NULL , "pCommand can't be NULL!" ) ;
   PD_TRACE_ENTRY( SDB_FAPMONGO_PROCESSMSG1 ) ;
   INT32 rc = SDB_OK ;
   INT32 orgOpCode = ((MsgHeader*)pMsg)->opCode ;
   BOOLEAN hasBuildGetMore = FALSE ;
   MONGO_CMD_TYPE cmdType = pCommand->type() ;
   BOOLEAN isNewCS = FALSE ;
   BOOLEAN needKillCursor = getMoreAll ? TRUE : FALSE ;
   BOOLEAN hasSave2Store = FALSE ;

   /// set collection name
   _clFullName = pCommand->clFullName() ;

   while ( TRUE )
   {
      rc = _processMsg( pMsg, errorObj, _contextBuff, _replyHeader ) ;

      // auto create cs/cl
      if ( SDB_DMS_CS_NOTEXIST == _replyHeader.flags )
      {
         if ( _shouldAutoCrtCS( pCommand ) )
         {
            rc = _autoCreateCS( pCommand->csName(), errorObj ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Failed to auto create cs, rc: %d", rc ) ;
               goto error ;
            }
            /// coord will change the opCode to catalog, so need restore
            ((MsgHeader*)pMsg)->opCode = orgOpCode ;
            isNewCS = TRUE ;

            if ( CMD_COLLECTION_CREATE == cmdType )
            {
               continue ;
            }
         }
      }

      if ( isNewCS || SDB_DMS_NOTEXIST == _replyHeader.flags )
      {
         if ( _shouldAutoCrtCL( pCommand ) )
         {
            rc = _autoCreateCL( pCommand->csName(), pCommand->clFullName(),
                                errorObj ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Failed to auto create cl, rc: %d", rc ) ;
               goto error ;
            }

            /// coord will change the opCode to catalog, so need restore
            ((MsgHeader*)pMsg)->opCode = orgOpCode ;
            isNewCS = FALSE ;
            continue ;
         }
      }
      // findAndUpsert is executed in two steps

      // first: findAndModify( matcher, update )
      // if the first step returns empty data, it means that the record we are
      // looking for doesn't exist. Then we need to perform the second step.
      // if the first step doesn't return empty data, we don't need to
      // perform the second step.

      // second: insert( new record )
      // we should generate a new record we will insert based on
      // matcher condition and update condition firstly. Then we insert this
      // new record.
      if ( ( CMD_FINDANDMODIFY == cmdType &&
           ((_mongoFindandmodifyCommand*)pCommand)->isUpsert()) &&
           ( -1 == _replyHeader.contextID && _contextBuff.size() == 0 ) &&
           SDB_OK == _replyHeader.flags )
      {
         _mongoFindandmodifyCommand* pCmd = (_mongoFindandmodifyCommand*)pCommand ;

         rc = _autoInsert( pCmd->clFullName(), pCmd->getCond(),
                           pCmd->getUpdater(), pCmd->getSetOnInsert(),
                           pCmd->getUpsertReturnRecord(),
                           errorObj ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to auto insert, rc: %d", rc ) ;
            goto error ;
         }

         _contextBuff = engine::rtnContextBuf( pCmd->getUpsertReturnRecord() ) ;
         pCmd->setHasInsertRecord( TRUE ) ;
         break ;
      }
      else if ( !hasBuildGetMore &&
                SDB_OK == _replyHeader.flags &&
                _shouldBuildGetMoreMsg( pCommand, _contextBuff, _replyHeader ) )
      {
         rc = buildGetMoreSdbMsg( _replyHeader.header.requestID,
                                  _replyHeader.contextID, _inBuffer ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to build sdb getMore msg, rc: %d", rc ) ;
            goto error;
         }

         hasBuildGetMore = TRUE ;
         continue ;
      }
      else if ( getMoreAll )
      {
         if ( SDB_OK == _replyHeader.flags &&
              _contextBuff.size() > 0 &&
              ( hasSave2Store || SDB_INVALID_CONTEXTID != _replyHeader.contextID ) )
         {
            /// save to store buff
            rc = _storeBuff.appendObjs( _contextBuff.data(), _contextBuff.size(),
                                        _contextBuff.recordNum(), TRUE ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Save result to store buff failed, rc: %d", rc ) ;
               goto error ;
            }
            hasSave2Store = TRUE ;
         }
         else if ( SDB_OK != _replyHeader.flags && SDB_DMS_EOC != _replyHeader.flags )
         {
            PD_LOG( PDERROR, "Process failed, rc: %d", _replyHeader.flags ) ;
            goto error ;
         }

         /// build getmore
         if ( SDB_INVALID_CONTEXTID != _replyHeader.contextID )
         {
            rc = buildGetMoreSdbMsg( _replyHeader.header.requestID,
                                     _replyHeader.contextID, _inBuffer ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to build sdb getMore msg, rc: %d", rc ) ;
               goto error;
            }

            hasBuildGetMore = TRUE ;
            continue ;
         }
      }

      break ;
   }

   // In SequoiaDB, count command is executed in three steps:
   // first: count, and return a cursor
   // second: getMore, and return count number
   // third: close cursor
   if ( CMD_COUNT == cmdType || CMD_DISTINCT == cmdType )
   {
      needKillCursor = TRUE ;
   }

   if ( hasSave2Store )
   {
      _contextBuff.release() ;
      rc = _storeBuff.get( -1, _contextBuff ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Get data from store buffer failed, rc: %d", rc ) ;
         goto error ;
      }
      _replyHeader.numReturned = _contextBuff.recordNum() ;
      _replyHeader.header.messageLength = sizeof( MsgOpReply ) + _contextBuff.size() ;

      if ( SDB_DMS_EOC == _replyHeader.flags && _contextBuff.size() > 0 )
      {
         _replyHeader.flags = SDB_OK ;
         /// clear the error
         errorObj = BSONObj() ;
         PD_MSG_RESET() ;
      }
   }

done:
   /// post process
   if ( needKillCursor && SDB_INVALID_CONTEXTID != _replyHeader.contextID )
   {
      _autoKillCursor( _replyHeader.header.requestID,
                       _replyHeader.contextID ) ;
   }

   PD_TRACE_EXITRC( SDB_FAPMONGO_PROCESSMSG1, rc ) ;
   return rc ;
error:
   /// when failed, need to kill context
   needKillCursor = TRUE ;
   _replyHeader.flags = rc ;
   if ( errorObj.isEmpty() )
   {
      errorObj = mongoGetErrorBson( rc, eduCB()->getInfo( EDU_INFO_ERROR ) ) ;
      _contextBuff = engine::rtnContextBuf( errorObj ) ;
   }
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_PROCESSMSG2, "_mongoSession::_processMsg" )
INT32 _mongoSession::_processMsg( const CHAR *pMsg, BSONObj &errorObj,
                                  engine::rtnContextBuf &buf, MsgOpReply &replyHeader )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_PROCESSMSG2 ) ;
   INT32   rc           = SDB_OK ;
   BOOLEAN needReply    = FALSE ;
   BOOLEAN needRollback = FALSE ;
   BOOLEAN isAutoCommit = FALSE ;
   BOOLEAN isDoCommit   = FALSE ;
   bson::BSONObjBuilder retBuilder( PMD_RETBUILDER_DFT_SIZE ) ;

   rc = _onMsgBegin( (MsgHeader *)pMsg, replyHeader ) ;

   buf.release() ;

   if ( SDB_OK == rc )
   {
      if ( MSG_BS_TRANS_COMMIT_REQ == ((MsgHeader *)pMsg)->opCode )
      {
         isDoCommit = TRUE ;
      }

      rc = getProcessor()->processMsg( (MsgHeader *) pMsg, buf,
                                       replyHeader.contextID,
                                       needReply, needRollback, retBuilder ) ;

      replyHeader.numReturned = buf.recordNum() ;
      replyHeader.startFrom   = (INT32)buf.getStartFrom() ;

      if ( eduCB()->isAutoCommitTrans() &&
           -1 == eduCB()->getCurAutoTransCtxID() )
      {
         isAutoCommit = TRUE ;
         if ( SDB_OK == rc || SDB_DMS_EOC == rc )
         {
            INT32 rcTmp = _processor->doCommit() ;
            rc = rcTmp ? rcTmp : rc ;
         }
      }

      if ( SDB_OK != rc && SDB_RTN_ALREADY_IN_AUTO_TRANS != rc &&
           eduCB()->isTransaction() &&
           ( isAutoCommit || isDoCommit ||
             ( needRollback &&
               eduCB()->getTransExecutor()->isTransAutoRollback() )
           )
         )
      {
         PD_LOG( PDDEBUG, "Session[%s] rolling back operation "
                 "(opCode=%d, rc=%d)", sessionName(), ((MsgHeader *)pMsg)->opCode, rc ) ;

         INT32 rcTmp = _processor->doRollback() ;
         if ( rcTmp )
         {
            PD_LOG( PDERROR, "Session[%s] failed to rollback trans "
                    "info, rc: %d", sessionName(), rcTmp ) ;
         }
      }
   }
   replyHeader.flags       = rc ;

   if ( rc )
   {
      if ( SDB_APP_INTERRUPT == rc &&
           SDB_OK != _pEDUCB->getInterruptRC() )
      {
         rc = _pEDUCB->getInterruptRC() ;
         PD_LOG_MSG ( PDDEBUG, "Interrupted EDU [%llu] with return code %d",
                      _pEDUCB->getID(), rc ) ;
      }

      _buildErrorObj( buf, replyHeader.flags, retBuilder ) ;
      errorObj = retBuilder.obj() ;
      buf = engine::rtnContextBuf( errorObj ) ;
      replyHeader.numReturned = 1 ;
   }
   else
   {
      errorObj = BSONObj() ;

      // we can get InsertedNum, DuplicatedNum, UpdatedNum,
      // ModifiedNum and DeletedNum from retBuilder.obj().
      if ( !retBuilder.isEmpty() && 0 == buf.size() )
      {
         buf = engine::rtnContextBuf( retBuilder.obj() ) ;
         replyHeader.numReturned = 1 ;
      }
   }

   _onMsgEnd( rc, (MsgHeader *) pMsg ) ;

   PD_TRACE_EXITRC( SDB_FAPMONGO_PROCESSMSG2, rc ) ;
   return rc ;
}

void _mongoSession::_buildErrorObj( const engine::rtnContextBuf &contextBuff,
                                    INT32 errCode, BSONObjBuilder &builder )
{
   try
   {
      if ( contextBuff.recordNum() >= 1 )
      {
         BSONObj obj( contextBuff.data() ) ;

         if ( SDB_IXM_DUP_KEY == errCode )
         {
            mongoBuildDupkeyErrObj( obj, _clFullName, builder ) ;
         }
         else
         {
            mongoBuildErrorBson( builder, errCode, NULL, obj ) ;
         }
      }
      else
      {
         mongoBuildErrorBson( builder, errCode, eduCB()->getInfo( EDU_INFO_ERROR ) ) ;
      }
   }
   catch( std::exception &e )
   {
      PD_LOG( PDWARNING, "Occur exception: %s", e.what() ) ;
   }
}

void _mongoSession::_saveOrSetMsgGlobalID( MsgHeader *pMsg )
{
   SDB_ASSERT( pMsg, "msg can't be NULL" ) ;
   pmdOperator *pOperator = (pmdOperator*)getOperator() ;

   if ( pMsg->globalID.getQueryID().isInvalid() )
   {
      MsgGlobalID globalID = pOperator->getGlobalID() ;
      globalID.incQueryID() ;
      pMsg->globalID = globalID ;
   }

   pOperator->setMsg( pMsg, eduCB() ) ;
}

void _mongoSession::_clearErrorInfo( BSONObj &errObj, mongoSessionCtx *pSessCtx )
{
   errObj = BSONObj() ;
   _replyHeader.flags = SDB_OK ;
   _contextBuff.release() ;
}

INT32 _mongoSession::_onMsgBegin( MsgHeader *pMsg, MsgOpReply &replyHeader )
{
   _pEDUCB->clearProcessInfo() ;
   _saveOrSetMsgGlobalID( pMsg ) ;
   getClient()->registerInMsg( pMsg ) ;

   // set reply header ( except flags, length )
   msgFillReplyByReq( replyHeader, pMsg, engine::pmdGetNodeID().value ) ;

   // start operator
   MON_START_OP( _pEDUCB->getMonAppCB() ) ;
   _pEDUCB->getMonAppCB()->setLastOpType( pMsg->opCode ) ;

   return SDB_OK ;
}

void _mongoSession::_onMsgEnd( INT32 result, MsgHeader *pMsg )
{
   pmdOperator *pOperator = (pmdOperator*)getOperator() ;

   if ( result && SDB_DMS_EOC != result )
   {
      PD_LOG( PDWARNING, "Session[%s] process msg[opCode=%d, len: %d, "
              "TID: %u, requestID: %llu] failed, rc: %d",
              sessionName(), pMsg->opCode, pMsg->messageLength, pMsg->TID,
              pMsg->requestID, result ) ;
   }

   if ( result != SDB_OK )
   {
      pmdIncErrNum( result ) ;
   }

   // end operator
   MON_END_OP( _pEDUCB->getMonAppCB() ) ;

   getClient()->unregisterInMsg() ;

   _pEDUCB->clearProcessInfo() ;
   pOperator->reset() ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_REPLY1, "_mongoSession::_reply" )
INT32 _mongoSession::_reply( _mongoCommand *pCommand, const CHAR* pMsg,
                             INT32 errCode, BSONObj &errObj )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_REPLY1 ) ;
   INT32 rc = SDB_OK ;
   _mongoResponseBuffer headerBuf ;

   if ( errCode )
   {
      if ( errObj.isEmpty() )
      {
         errObj = mongoGetErrorBson( errCode, eduCB()->getInfo( EDU_INFO_ERROR ) ) ;
      }

      _contextBuff = engine::rtnContextBuf( errObj ) ;
      _replyHeader.numReturned = 1 ;
      _replyHeader.flags = errCode ;
   }

   if ( pCommand && pCommand->isInitialized() )
   {
      rc = pCommand->buildMongoReply( _replyHeader, _contextBuff, headerBuf ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Session[%s] failed to build response, rc: %d",
                   sessionName(), rc ) ;

      /// when has set new eror, need update the error
      if ( headerBuf.hasNewError )
      {
         errObj = headerBuf.objNewError ;
      }
   }
   else
   {
      _mongoMessage mongoMsg ;
      rc = mongoMsg.init( pMsg ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Session[%s] failed to init message, rc: %d",
                   sessionName(), rc ) ;
      if ( MONGO_OP_COMMAND == mongoMsg.opCode() )
      {
         BSONObj empty ;
         BSONObj org( _contextBuff.data() ) ;

         _tmpBuffer.zero() ;

         rc = _tmpBuffer.write( org.objdata(), org.objsize() ) ;
         if ( rc )
         {
            goto error ;
         }

         rc = _tmpBuffer.write( empty.objdata(), empty.objsize() ) ;
         if ( rc )
         {
            goto error ;
         }

         _contextBuff = rtnContextBuf( _tmpBuffer.data(), _tmpBuffer.size(), 2 ) ;

         mongoCommandResponse res ;
         res.header.msgLen = sizeof( mongoCommandResponse ) +
                             _contextBuff.size() ;
         res.header.responseTo = mongoMsg.requestID() ;
         headerBuf.setData( (const CHAR*)&res, sizeof( res ) ) ;
      }
      else
      {
         mongoResponse res ;
         res.header.msgLen = sizeof( mongoResponse ) + _contextBuff.size() ;
         res.header.responseTo = mongoMsg.requestID() ;
         res.nReturned = _contextBuff.recordNum() ;
         headerBuf.setData( (const CHAR*)&res, sizeof( res ) ) ;
      }
   }

   PD_LOG( PDDEBUG, "Build mongodb reply msg[ tid: %d, session: %s, "
           "command: %s, clFullName: %s, eduID: %llu ] done",
           ossGetCurrentThreadID(),
           sessionName(),
           pCommand ? ( (pCommand)->name() ? (pCommand)->name() : "" ) : "",
           pCommand ? ( (pCommand)->clFullName() ?
           (pCommand)->clFullName() : "" ) : "",
           eduID() ) ;

   // send response
   if ( headerBuf.usedSize > 0 )
   {
      INT32 rcTmp = SDB_OK ;
      const mongoResponse *pRes = (mongoResponse*)headerBuf.data ;

      rcTmp = sendData( (CHAR *)&headerBuf, headerBuf.usedSize ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Session[%s] failed to send response header, rc: %d",
                   sessionName(), rcTmp ) ;

      if ( _contextBuff.data() )
      {
         rcTmp = sendData( _contextBuff.data(), _contextBuff.size() ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Session[%s] failed to send response body, rc: %d",
                      sessionName(), rcTmp ) ;
      }

      PD_LOG( PDDEBUG, "Send mongodb reply msg[ tid: %d, session: %s, "
              "command: %s, clFullName: %s, msgHead: { msgLen: %d,"
              " requestId: %d, responseTo: %d, opCode: %d, reservedFlags: %d,"
              " cursorId: %llu, startingFrom: %d, nReturned: %d }, "
              "eduID: %llu ] done",
              ossGetCurrentThreadID(),
              sessionName(),
              pCommand ? ( (pCommand)->name() ? (pCommand)->name() : "" ) : "",
              pCommand ? ( (pCommand)->clFullName() ?
              (pCommand)->clFullName() : "" ) : "",
              pRes->header.msgLen, pRes->header.requestId,
              pRes->header.responseTo, pRes->header.opCode, pRes->reservedFlags,
              pRes->cursorId, pRes->startingFrom, pRes->nReturned, eduID() ) ;
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_REPLY1, rc ) ;
   return rc ;
error:
   goto done ;
}

}
