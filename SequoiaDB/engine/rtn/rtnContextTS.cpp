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

   Source File Name = rtnContextTS.cpp

   Descriptive Name = RunTime Text Search Context

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for Runtime
   Context.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/30/2017  YSD Split from rtnContextData.cpp

   Last Changed =

*******************************************************************************/
#include "rtnContextTS.hpp"
#include "rtn.hpp"
#include "pmdController.hpp"
#include "rtnTrace.hpp"

namespace engine
{
   RTN_CTX_AUTO_REGISTER( _rtnContextTS, RTN_CONTEXT_TS, "TS")

   _rtnContextTS::_rtnContextTS( INT64 contextID, UINT64 eduID )
   : rtnContextMain( contextID, eduID )
   {
      _eduCB = NULL ;
      _remoteSessionID = 0 ;
      _subContext = NULL ;
      _remoteCtxID = -1 ;
   }

   _rtnContextTS::~_rtnContextTS()
   {
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
      rtnRemoteMessenger *messenger = rtnCB->getRemoteMessenger() ;

      messenger->removeSession( _remoteSessionID, pmdGetThreadEDUCB() ) ;
      if ( _subContext )
      {
         if ( _subContext->contextID() )
         {
            pmdGetKRCB()->getRTNCB()->contextDelete( _subContext->contextID(),
                                                     _eduCB ) ;
         }
         SDB_OSS_DEL _subContext ;
      }
   }

   std::string _rtnContextTS::name() const
   {
      return "TS" ;
   }

   RTN_CONTEXT_TYPE _rtnContextTS::getType() const
   {
      return RTN_CONTEXT_TS ;
   }

   _dmsStorageUnit* _rtnContextTS::getSU()
   {
      return NULL ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCONTEXTTS_OPEN, "_rtnContextTS::open" )
   INT32 _rtnContextTS::open( const rtnQueryOptions &options,
                              pmdEDUCB *eduCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNCONTEXTTS_OPEN ) ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
      rtnRemoteMessenger *messenger = rtnCB->getRemoteMessenger() ;

      _isOpened = TRUE ;
      _hitEnd = FALSE ;

      rc = messenger->prepareSession( eduCB, _remoteSessionID ) ;
      PD_RC_CHECK( rc, PDERROR, "Prepare remote task failed[ %d ]", rc ) ;

      // 1. Store the query items.
      // 2. Send the query to search engine adapter, and get the replay. Then
      //    call query interface again, to get a sub context id.
      _options = options ;
      rc = _options.getOwned() ;
      PD_RC_CHECK( rc, PDERROR, "Get owned of query options failed[ %d ]",
                   rc ) ;
      _numToReturn = _options.getLimit() ;
      _numToSkip = _options.getSkip() ;
      _eduCB = eduCB ;

#ifdef _DEBUG
      PD_LOG( PDDEBUG, "Options for search: %s", options.toString().c_str() ) ;
#endif /* _DEBUG */
      rc = _queryRemote( options, eduCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Query from remote failed[ %d ]", rc ) ;

      rc = _prepareNextSubContext( eduCB, FALSE ) ;
      if ( rc )
      {
         if ( SDB_DMS_EOC == rc )
         {
            _hitEnd = TRUE ;
            rc = SDB_OK ;
            goto done ;
         }
         else
         {
            PD_LOG( PDERROR, "Prepare next sub context failed[ %d ]", rc ) ;
         }
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNCONTEXTTS_OPEN, rc ) ;
      return rc ;
   error:
      _isOpened = FALSE ;
      _hitEnd = TRUE ;
      goto done ;
   }

   BOOLEAN _rtnContextTS::_requireExplicitSorting () const
   {
      return FALSE ;
   }

   INT32 _rtnContextTS::_prepareAllSubCtxDataByOrder( _pmdEDUCB *cb )
   {
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCONTEXTTS__GETNONEMPTYNORMALSUBCTX, "_rtnContextTS::_getNonEmptyNormalSubCtx" )
   INT32 _rtnContextTS::_getNonEmptyNormalSubCtx( _pmdEDUCB *cb,
                                                  rtnSubContext*& subCtx )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNCONTEXTTS__GETNONEMPTYNORMALSUBCTX ) ;
      SDB_RTNCB* rtnCB = pmdGetKRCB()->getRTNCB() ;

      while ( TRUE )
      {
         if ( _subContext->recordNum() <= 0 )
         {
            rc = _prepareSubCtxData( cb, -1 ) ;
            if ( rc != SDB_OK )
            {
               rtnCB->contextDelete( _subContext->contextID(), cb ) ;
               SDB_OSS_DEL _subContext ;
               _subContext = NULL ;
               if ( SDB_DMS_EOC != rc )
               {
                  goto error ;
               }
               else
               {
                  rc = _prepareNextSubContext( cb ) ;
                  if ( rc )
                  {
                     if ( SDB_DMS_EOC != rc )
                     {
                        PD_LOG( PDERROR, "Prepare next sub context "
                                "failed[ %d ]", rc ) ;
                     }
                     goto error ;
                  }
                  if ( !_subContext )
                  {
                     rc = SDB_DMS_EOC ;
                     goto error ;
                  }
                  continue ;
               }
            }
         }
         subCtx = _subContext ;
         break ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNCONTEXTTS__GETNONEMPTYNORMALSUBCTX, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnContextTS::_saveEmptyOrderedSubCtx( rtnSubContext* subCtx )
   {
      return SDB_OK ;
   }

   INT32 _rtnContextTS::_saveEmptyNormalSubCtx( rtnSubContext* subCtx )
   {
      return SDB_OK ;
   }

   INT32 _rtnContextTS::_saveNonEmptyNormalSubCtx( rtnSubContext* subCtx )
   {
      return SDB_OK ;
   }


   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCONTEXTTS__PREPARENEXTSUBCONTEXT, "_rtnContextTS::_prepareNextSubContext" )
   INT32 _rtnContextTS::_prepareNextSubContext( pmdEDUCB *eduCB,
                                                BOOLEAN getMore )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNCONTEXTTS__PREPARENEXTSUBCONTEXT ) ;
      MsgOpReply *reply = NULL ;
      INT32 flag = 0 ;
      INT32 startFrom = 0 ;
      INT32 numReturned = 0 ;
      vector< BSONObj > objList ;
      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
      INT64 subContextID = -1 ;
      MsgHeader *msg = NULL ;
      INT32 msgSize = 0 ;
      INT32 numToReturn = 0 ;
      UINT64 reqID = 0 ;

      rtnRemoteMessenger *messenger = rtnCB->getRemoteMessenger() ;

      if ( getMore )
      {
         rc = msgBuildGetMoreMsg( (CHAR **)&msg, &msgSize, numToReturn,
                                  _remoteCtxID, reqID, eduCB ) ;
         PD_RC_CHECK( rc, PDERROR, "Build get more message failed[ %d ]", rc ) ;

         msg->opCode = MSG_BS_GETMORE_REQ ;

         rc = messenger->send( _remoteSessionID, msg, eduCB ) ;
         PD_RC_CHECK( rc, PDERROR, "Send message by remote messenger failed[ %d ]",
                      rc ) ;
      }

      rc = messenger->receive( _remoteSessionID, eduCB, reply ) ;
      PD_RC_CHECK( rc, PDERROR, "Receive reply by remote messenger failed"
                   "[ %d ]", rc ) ;

      rc = msgExtractReply( (CHAR *)reply, &flag, &_remoteCtxID, &startFrom,
                            &numReturned, objList ) ;
      PD_RC_CHECK( rc, PDERROR, "Extract query respond message failed[ %d ]",
                   rc ) ;

      try
      {
         rc = flag ;
         if ( rc )
         {
            if ( SDB_DMS_EOC != rc )
            {
               if ( 0 != objList.size() )
               {
                  PD_LOG_MSG( PDERROR, "Error returned from remote: %s",
                              objList.front().toString( FALSE, TRUE ).c_str() ) ;
               }
            }
            goto error ;
         }

         // 4 objects are expected: matcher, selector, order by, hint.
         if ( objList.size() != 4 )
         {
            PD_LOG( PDERROR, "Respond message size is wrong, expect[ %d ], "
                    "actual[ %d ]", 4, objList.size() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception happened: %s", e.what() ) ;
         goto error ;
      }

      // Do a query, and get another subcontext.
      rc = rtnQuery( _options.getCLFullName(), objList[1], objList[0],
                     objList[2], objList[3], _options.getFlag(), eduCB,
                     0, -1, dmsCB, rtnCB, subContextID ) ;
      PD_RC_CHECK( rc, PDERROR, "Query data on collection[ %s ] failed[ %d ]",
                   _options.getCLFullName(), rc ) ;
      if ( _subContext )
      {
         SDB_OSS_DEL _subContext ;
         _subContext = NULL ;
      }

      _subContext = SDB_OSS_NEW rtnSubCLContext( _options.getOrderBy(), _keyGen,
                                                 subContextID ) ;
      if ( !_subContext )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Allocate memory for rtnSubCLContext failed, "
                 "size[ %d ]", sizeof( rtnSubCLContext ) ) ;
         goto error ;
      }

      rc = _checkSubContext( _subContext ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check sub context, rc: %d", rc ) ;

   done:
      if ( msg )
      {
         msgReleaseBuffer( (CHAR *)msg, eduCB ) ;
      }
      PD_TRACE_EXITRC( SDB__RTNCONTEXTTS__PREPARENEXTSUBCONTEXT, rc ) ;
      return rc ;
   error:
      if ( _subContext )
      {
         SDB_OSS_DEL _subContext ;
         _subContext = NULL ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCONTEXTTS__PREPARESUBCTXDATA, "_rtnContextTS::_prepareSubCtxData" )
   INT32 _rtnContextTS::_prepareSubCtxData( pmdEDUCB *cb,
                                            INT32 maxNumToReturn )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNCONTEXTTS__PREPARESUBCTXDATA ) ;
      rtnContextBuf contextBuf ;
      SDB_RTNCB* rtnCB = pmdGetKRCB()->getRTNCB() ;

      if ( _subContext->recordNum() > 0 )
      {
         goto done ;
      }

      rc = rtnGetMore( _subContext->contextID(), maxNumToReturn,
                       contextBuf, cb, rtnCB ) ;
      if ( rc )
      {
         if ( SDB_DMS_EOC != rc )
         {
            PD_LOG( PDERROR, "Get more for context[ %lld ] failed[ %d ]",
                    _subContext->contextID(), rc ) ;
         }
         goto error ;
      }

      {
         _subContext->setBuffer( contextBuf ) ;
         BOOLEAN skipBuffer = FALSE ;
         rc = _processSubContext( _subContext, skipBuffer ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to process sub-context [%lld], "
                    "rc: %d", _subContext->contextID(), rc ) ;
            goto error ;
         }
         if ( skipBuffer )
         {
            _subContext->popAll() ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNCONTEXTTS__PREPARESUBCTXDATA, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCONTEXTTS__QUERYREMOTE, "_rtnContextTS::_queryRemote" )
   INT32 _rtnContextTS::_queryRemote( const rtnQueryOptions &options,
                                      pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNCONTEXTTS__QUERYREMOTE ) ;
      MsgHeader *queryMsg = NULL ;
      INT32 msgSize = 0 ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
      rtnRemoteMessenger *messenger = rtnCB->getRemoteMessenger() ;

      // Format the message, and send it to search engine adapter.
      rc = options.toQueryMsg( (CHAR **)&queryMsg, msgSize, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Build query message from options failed[ %d ]",
                   rc ) ;

      queryMsg->opCode = MSG_BS_QUERY_REQ ;

      rc = messenger->send( _remoteSessionID, queryMsg, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Send message by remote messenger failed[ %d ]",
                   rc ) ;

   done:
      if ( queryMsg )
      {
         msgReleaseBuffer( (CHAR *)queryMsg, cb ) ;
      }
      PD_TRACE_EXITRC( SDB__RTNCONTEXTTS__QUERYREMOTE, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}

