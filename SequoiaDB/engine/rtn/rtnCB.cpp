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

   Source File Name = rtnCB.cpp

   Descriptive Name = Runtime Control Block

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   control block.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/
#include "rtnCB.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"
#include "dmsCB.hpp"
#include "rtnIxmKeySorter.hpp"
#include "rtnBackgroundJob.hpp"
#include "pmdController.hpp"

using namespace std;
namespace engine
{
   _SDB_RTNCB::_SDB_RTNCB()
      : _contextIdGenerator( 0 ),
        _maxContextNum( RTN_MAX_CTX_NUM_DFT ),
        _maxSessionContextNum( RTN_MAX_SESS_CTX_NUM_DFT ),
        _remoteMessenger( NULL ),
        _textIdxVersion((INT64)RTN_INIT_TEXT_INDEX_VERSION)
   {
      _pLTMgr = NULL ;
   }

   _SDB_RTNCB::~_SDB_RTNCB()
   {
      FOR_EACH_CMAP_ELEMENT_S( RTN_CTX_MAP, _contextMap )
      {
         SDB_OSS_DEL ((*it).second) ;
      }
      FOR_EACH_CMAP_ELEMENT_END ;

      _contextMap.clear() ;
   }

   void* _SDB_RTNCB::queryInterface( SDB_INTERFACE_TYPE type )
   {
      if ( SDB_IF_CTXMGR == type )
      {
         return dynamic_cast<IContextMgr*>( this ) ;
      }
      return IControlBlock::queryInterface( type ) ;
   }

   void _SDB_RTNCB::onPrimaryChange( BOOLEAN primary,
                                     SDB_EVENT_OCCUR_TYPE occurType )
   {
      if ( !primary && SDB_EVT_OCCUR_AFTER == occurType )
      {
         if ( _pLTMgr )
         {
            _pLTMgr->clear() ;
         }
      }
   }

   INT32 _SDB_RTNCB::init ()
   {
      INT32 rc = SDB_OK ;

      pmdOptionsCB *optionCB = pmdGetOptionCB() ;

      rtnIxmKeySorterCreator* creator = SDB_OSS_NEW _rtnIxmKeySorterCreator() ;
      if ( NULL == creator )
      {
         PD_LOG ( PDERROR, "failed to create _rtnIxmKeySorterCreator" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      _pLTMgr = SDB_OSS_NEW rtnLocalTaskMgr() ;
      if ( !_pLTMgr )
      {
         PD_LOG( PDERROR, "Failed to create local task manager" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      // register event handle
      pmdGetKRCB()->regEventHandler( this ) ;

      sdbGetDMSCB()->setIxmKeySorterCreator( creator ) ;

      // The error of initialization of APM could be ignore
      // Only data and catalog nodes could initialize plan cache
      _accessPlanManager.init(
            ( SDB_ROLE_DATA == pmdGetDBRole() ||
              SDB_ROLE_CATALOG == pmdGetDBRole() ||
              SDB_ROLE_STANDALONE == pmdGetDBRole() ||
              SDB_ROLE_OM == pmdGetDBRole() ) ?
                    optionCB->getPlanBuckets() : 0,
            (OPT_PLAN_CACHE_LEVEL)( optionCB->getPlanCacheLevel() ),
            optionCB->getSortBufSize(),
            optionCB->getOptCostThreshold(),
            optionCB->isEnabledMixCmp() ) ;

      _maxContextNum = optionCB->maxContextNum() ;
      _maxSessionContextNum = optionCB->maxSessionContextNum() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _SDB_RTNCB::active ()
   {
      INT32 rc = SDB_OK ;

      if ( SDB_ROLE_DATA       == pmdGetKRCB()->getDBRole() ||
           SDB_ROLE_STANDALONE == pmdGetKRCB()->getDBRole() )
      {
         rc = rtnStartCleanupIdxStatusJob() ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to start clean up index status job" ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _SDB_RTNCB::deactive ()
   {
      if ( _remoteMessenger )
      {
         _remoteMessenger->deactive() ;
      }
      return SDB_OK ;
   }

   INT32 _SDB_RTNCB::fini ()
   {
      _accessPlanManager.fini() ;

      // unregister event handle
      pmdGetKRCB()->unregEventHandler( this ) ;

      dmsIxmKeySorterCreator* creator = sdbGetDMSCB()->getIxmKeySorterCreator() ;
      if ( NULL != creator )
      {
         SDB_OSS_DEL( creator ) ;
         sdbGetDMSCB()->setIxmKeySorterCreator( NULL ) ;
      }

      rtnJobMgr* jobMgr = rtnGetJobMgr() ;
      jobMgr->fini() ;

      rtnIndexJobHolder *idxJobHolder = rtnGetIndexJobHolder() ;
      idxJobHolder->fini() ;

      if ( _remoteMessenger )
      {
         SDB_OSS_DEL _remoteMessenger ;
      }

      if ( _pLTMgr )
      {
         _pLTMgr->fini() ;
         SDB_OSS_DEL _pLTMgr ;
         _pLTMgr = NULL ;
      }

      return SDB_OK ;
   }

   void _SDB_RTNCB::onConfigChange ()
   {
      pmdOptionsCB *optionCB = pmdGetOptionCB() ;

      _accessPlanManager.reinit(
            ( SDB_ROLE_DATA == pmdGetDBRole() ||
              SDB_ROLE_CATALOG == pmdGetDBRole() ||
              SDB_ROLE_STANDALONE == pmdGetDBRole() ) ?
                    optionCB->getPlanBuckets() : 0,
            (OPT_PLAN_CACHE_LEVEL)( optionCB->getPlanCacheLevel() ),
            optionCB->getSortBufSize(),
            optionCB->getOptCostThreshold(),
            optionCB->isEnabledMixCmp() ) ;

      _maxContextNum = optionCB->maxContextNum() ;
      _maxSessionContextNum = optionCB->maxSessionContextNum() ;
   }

   rtnContext* _SDB_RTNCB::contextFind ( SINT64 contextID, _pmdEDUCB *cb )
   {
      rtnContext *pContext = NULL ;
      std::pair<rtnContext*, bool> ret = _contextMap.find( contextID ) ;
      if ( ret.second )
      {
         if ( cb && !cb->contextFind( contextID ) )
         {
            PD_LOG ( PDWARNING, "Context %lld does not owned by "
                     "current session", contextID ) ;
         }
         else
         {
            pContext = ret.first ;
         }
      }

      return pContext ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_RTNCB_CONTEXTDEL, "_SDB_RTNCB::contextDelete" )
   void _SDB_RTNCB::contextDelete ( INT64 contextID, IExecutor *pExe )
   {
      PD_TRACE_ENTRY ( SDB__SDB_RTNCB_CONTEXTDEL ) ;

      rtnContext *pContext = NULL ;
      pmdEDUCB *cb = ( pmdEDUCB* )pExe ;

      if ( cb )
      {
         cb->contextDelete( contextID ) ;
      }

      {
         pair<rtnContext*, bool> ret = _contextMap.find( contextID ) ;
         if ( ret.second )
         {
            _contextMap.erase( contextID ) ;
            pContext = ret.first ;
         }
      }

      if ( pContext )
      {
         INT32 reference = pContext->getReference() ;
         pContext->waitForPrefetch() ;

         /// wait for sync
         if ( pContext->isWrite() && pContext->getDPSCB() &&
              pContext->getW() > 1 )
         {
            pContext->getDPSCB()->completeOpr( cb, pContext->getW() ) ;
         }

         monClassQuery *monQueryCB = pContext->getMonQueryCB() ;
         if ( NULL != monQueryCB )
         {
            monQueryCB->anchorToContext = FALSE ;
            // Usuaully the monQuery will get removed/archived
            // at the point when pmd processMsg ends with data
            // collected at that time.
            // But if this context is cleaned and pmd currently
            // is not processing the query this context belongs to.
            // Which also means the original query this context
            // belongs to ends unexpectedly.
            // We need to clean the monQuery.
            if ( ( NULL == cb ) ||
                 ( cb->getMonQueryCB() != monQueryCB ) )
            {
               pmdGetKRCB()->getMonMgr()->removeMonitorObject( monQueryCB ) ;
            }
            pContext->setMonQueryCB( NULL ) ;
         }

         sdbGetRTNContextBuilder()->release( pContext ) ;
         PD_LOG( PDDEBUG, "delete context(contextID=%lld, reference: %d)",
                 contextID, reference ) ;
      }

      PD_TRACE_EXIT ( SDB__SDB_RTNCB_CONTEXTDEL ) ;
      return ;
   }

   INT32 _SDB_RTNCB::dumpWritingContext( RTN_CTX_PROCESS_LIST &contextProcessList,
                                         EDUID filterEDUID,
                                         UINT64 blockID )
   {
      INT32 rc = SDB_OK ;

      FOR_EACH_CMAP_ELEMENT_S( RTN_CTX_MAP, _contextMap )
      {
         rtnContext *pContext = it->second ;

         if ( pContext &&
              pContext->isOpened() &&
              pContext->isWrite() )
         {
            if ( PMD_INVALID_EDUID != filterEDUID &&
                 pContext->eduID() == filterEDUID )
            {
               continue ;
            }
            else if ( blockID > 0 &&
                      pContext->getOpID() >= blockID )
            {
               continue ;
            }
            else
            {
               const CHAR *processName = pContext->getProcessName() ;
               if ( NULL == processName || 0 == processName[ 0 ] )
               {
                  continue ;
               }
               else
               {
                  try
                  {
                     INT64 contextID = pContext->contextID() ;
                     contextProcessList.push_back(
                           make_pair( contextID, processName ) ) ;

                     PD_LOG( PDDEBUG, "Got writing context [%lld] with "
                             "writing ID [%llu] on [%s] edu [%llu]",
                             contextID, pContext->getOpID(), processName,
                             pContext->eduID() ) ;
                  }
                  catch ( exception &e )
                  {
                     PD_LOG( PDERROR, "Failed to save context, "
                             "occur exception %s", e.what() ) ;
                     rc = ossException2RC( &e ) ;
                     goto error ;
                  }
               }
            }
         }
      }
      FOR_EACH_CMAP_ELEMENT_END

   done:
      return rc ;
   error:
      goto done ;
   }

   SINT32 _SDB_RTNCB::contextNew ( RTN_CONTEXT_TYPE type,
                                   rtnContext **context,
                                   SINT64 &contextID,
                                   _pmdEDUCB * pEDUCB )
   {
      SDB_ASSERT ( context, "context pointer can't be NULL" ) ;
      monSvcTaskInfo *pTaskInfo = NULL ;

      if ( pEDUCB->isFromLocal() )
      {
         // WARNING: the check may fail when context flooding ( too many
         //          context are creating in the same time )
         if ( _maxContextNum > 0 &&
              _contextMap.size( FALSE ) >= (UINT32)( _maxContextNum ) )
         {
            PD_LOG_MSG( PDERROR, "the number of contexts exceeds the limit "
                        "[%s:%d]", PMD_OPTION_MAXCONTEXTNUM, _maxContextNum ) ;
            return SDB_DPS_CONTEXT_NUM_UP_TO_LIMIT ;
         }

         if ( _maxSessionContextNum > 0 &&
              pEDUCB->contextNum() >= (UINT32)( _maxSessionContextNum ) )
         {
            PD_LOG_MSG( PDERROR, "the number of contexts in the session "
                        "exceeds the limit [%s:%d]",
                        PMD_OPTION_MAXSESSIONCONTEXTNUM, _maxSessionContextNum ) ;
            return SDB_DPS_CONTEXT_NUM_UP_TO_LIMIT ;
         }
      }

      // if hit max signed 64 bit integer?
      if ( _contextIdGenerator.fetch() < 0 )
      {
         return SDB_SYS ;
      }

      INT64 _contextId = _contextIdGenerator.inc() ;
      if ( _contextId < 0 )
      {
         return SDB_SYS ;
      }

      (*context) = sdbGetRTNContextBuilder()->create(
                     type, _contextId, pEDUCB->getID() ) ;

      if ( !(*context) )
      {
         return SDB_OOM ;
      }

      if ( !( _contextMap.insert( _contextId, *context ).second ) )
      {
         sdbGetRTNContextBuilder()->release( *context ) ;
         *context = NULL ;
         return SDB_OOM ;
      }

      if ( !pEDUCB->contextInsert( _contextId ) )
      {
         _contextMap.erase( _contextId ) ;
         sdbGetRTNContextBuilder()->release( *context ) ;
         *context = NULL ;
         return SDB_OOM ;
      }

      contextID = _contextId ;

      pTaskInfo = pEDUCB->getMonAppCB()->getSvcTaskInfo() ;
      if ( pTaskInfo )
      {
         pTaskInfo->monContextInc( 1 ) ;
      }

      // Anchor the monQuery on the first context that gets created in a query
      monClassQuery *monQuery = pEDUCB->getMonQueryCB() ;
      if ( NULL != monQuery &&
           !monQuery->anchorToContext )
      {
         (*context)->setMonQueryCB( monQuery ) ;
         monQuery->anchorToContext = TRUE ;
      }

      if ( pEDUCB->getMonConfigCB()->timestampON )
      {
         (*context)->getMonCB()->recordStartTimestamp() ;
      }
      (*context)->setOpID( pEDUCB->getWritingID() ) ;

      PD_LOG ( PDDEBUG, "Create new context(contextID=%lld, type: %d[%s], "
               "writing ID %llu)",
               contextID, type, getContextTypeDesp(type),
               (*context)->getOpID() ) ;

      return SDB_OK ;
   }

   INT32 _SDB_RTNCB::prepareRemoteMessenger()
   {
      INT32 rc = SDB_OK ;

      // Remote messenger should be enabled on data node to support text search.
      if ( SDB_ROLE_DATA == pmdGetDBRole() )
      {
         _remoteMessenger = SDB_OSS_NEW rtnRemoteMessenger() ;
         if ( !_remoteMessenger )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Allocate memory for remote messenger failed, "
                    "size[ %d ]", sizeof( rtnRemoteMessenger ) ) ;
            goto error ;
         }
         rc = _remoteMessenger->init() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to initialize remote messenger, "
                      "rc: %d", rc ) ;
         rc = _remoteMessenger->active() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to active remote messenger, "
                                   "rc: %d", rc) ;
      }

   done:
      return rc ;
   error:
      if ( _remoteMessenger )
      {
         SDB_OSS_DEL _remoteMessenger ;
         _remoteMessenger = NULL ;
      }
      goto done ;
   }

   /*
      get global rtn cb
   */
   SDB_RTNCB* sdbGetRTNCB ()
   {
      static SDB_RTNCB s_rtnCB ;
      return &s_rtnCB ;
   }

}

