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
#include "rtnBackgroundJob.hpp"
#include "pmdEnv.hpp"
#include "pmdDummySession.hpp"
#include "pmdController.hpp"

using namespace std;
namespace engine
{

   #define RTN_FIND_CONTEXT_TIMEOUT          ( OSS_ONE_SEC )

   /*
      _rtnClearExpireContextJob implement
   */
   _rtnClearExpireContextJob::_rtnClearExpireContextJob()
   {
   }

   _rtnClearExpireContextJob::~_rtnClearExpireContextJob()
   {
   }

   RTN_JOB_TYPE _rtnClearExpireContextJob::type() const
   {
      return RTN_JOB_CLEAR_EXPIRED_CONTEXT ;
   }

   const CHAR* _rtnClearExpireContextJob::name() const
   {
      return "CLEAR-EXPIRED-CONTEXT" ;
   }

   BOOLEAN _rtnClearExpireContextJob::muteXOn( const _rtnBaseJob *pOther )
   {
      if ( type() == pOther->type() )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   INT32 _rtnClearExpireContextJob::doit()
   {
      SDB_RTNCB *pRtnCB = sdbGetRTNCB() ;
      pmdDummySession dummySession ;
      pmdEDUEvent event ;
      UINT64 lastTick = pmdGetDBTick() ;

      /// attach session
      dummySession.attachCB( _pEDUCB ) ;

      /// register to remote session manager
      if ( sdbGetPMDController()->getRSManager() )
      {
         sdbGetPMDController()->getRSManager()->registerEDU( _pEDUCB ) ;
      }

      /// reset the event
      pRtnCB->getEvent()->reset() ;

      while ( PMD_IS_DB_UP() )
      {
         if ( _pEDUCB->waitEvent( event, OSS_ONE_SEC ) )
         {
            pmdEduEventRelease( event, _pEDUCB ) ;
         }

         if( pmdGetTickSpanTime( lastTick ) >= RTN_CTX_CHECK_INTERVAL )
         {
            // clear interrupt flag
            _pEDUCB->resetInterrupt() ;
            _pEDUCB->resetInfo( EDU_INFO_ERROR ) ;
            _pEDUCB->resetLsn() ;
            pdClearLastError() ;

            try
            {
               pRtnCB->preDelExpiredContext( _pEDUCB ) ;
            }
            catch( std::exception &e )
            {
               PD_LOG( PDWARNING, "Occur exception: %s", e.what() ) ;
               /// ignore
            }
            lastTick = pmdGetDBTick() ;
         }
      }

      try
      {
         pRtnCB->preDelExpiredContext( _pEDUCB, TRUE ) ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDWARNING, "Occur exception: %s", e.what() ) ;
         /// ignore
      }

      /// signal the event
      pRtnCB->getEvent()->signal() ;

      /// unreg from remote session manager
      if ( sdbGetPMDController()->getRSManager() )
      {
         sdbGetPMDController()->getRSManager()->unregEUD( _pEDUCB ) ;
      }
      /// detach session
      dummySession.detachCB() ;

      return SDB_OK ;
   }

   /*
      _rtnClearUserCacheJob define
   */
   class _rtnClearUserCacheJob : public utilLightJob
   {
   public:
      _rtnClearUserCacheJob( SDB_RTNCB *rtnCB ) : _rtnCB( rtnCB )
      {
         SDB_ASSERT( NULL != rtnCB, "rtnCB is invalid" );
      }

      virtual ~_rtnClearUserCacheJob() {}

      virtual const CHAR *name() const
      {
         return "ClearUserCacheJob";
      }

      virtual INT32 doit( IExecutor *pExe, UTIL_LJOB_DO_RESULT &result, UINT64 &sleepTime )
      {
         // UINT64 microseconds
         sleepTime = pmdGetOptionCB()->getUserCacheInterval() * 1000ULL;

         if ( PMD_IS_DB_DOWN() )
         {
            result = UTIL_LJOB_DO_FINISH;
         }
         else
         {
            _rtnCB->getUserCacheMgr()->clear();
            result = UTIL_LJOB_DO_CONT;
         }

         return SDB_OK;
      }

   protected:
      SDB_RTNCB *_rtnCB;
   };
   typedef class _rtnClearUserCacheJob rtnClearUserCacheJob;

   /*
      _SDB_RTNCB implement
   */

   _SDB_RTNCB::_SDB_RTNCB()
      : _contextIdGenerator( 0 ),
        _maxContextNum( RTN_MAX_CTX_NUM_DFT ),
        _maxSessionContextNum( RTN_MAX_SESS_CTX_NUM_DFT ),
        _contextTimeout( RTN_CTX_TIMEOUT_DFT ),
        _remoteMessenger( NULL ),
        _textIdxVersion((INT64)RTN_INIT_TEXT_INDEX_VERSION)
   {
      _pLTMgr = NULL ;
   }

   _SDB_RTNCB::~_SDB_RTNCB()
   {
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

      /// set signal
      _event.signal() ;

      _pLTMgr = SDB_OSS_NEW rtnLocalTaskMgr() ;
      if ( !_pLTMgr )
      {
         PD_LOG( PDERROR, "Failed to create local task manager" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      // register event handle
      pmdGetKRCB()->regEventHandler( this ) ;

      sdbGetDMSCB()->setIxmKeySorterCreator( &_sorterCreator ) ;
      sdbGetDMSCB()->setScannerCheckerCreator( &_checkerCreator ) ;

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
            optionCB->isEnabledMixCmp(),
            optionCB->getOptStartCostLimit() ) ;

      _maxContextNum = optionCB->maxContextNum() ;
      _maxSessionContextNum = optionCB->maxSessionContextNum() ;
      _contextTimeout = optionCB->contextTimeout() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _SDB_RTNCB::active ()
   {
      INT32 rc = SDB_OK ;

      if ( SDB_ROLE_DATA == pmdGetDBRole() ||
           SDB_ROLE_CATALOG == pmdGetDBRole() ||
           SDB_ROLE_STANDALONE == pmdGetDBRole() ||
           SDB_ROLE_OM == pmdGetDBRole() )
      {
         rc = pmdGetKRCB()->getDMSCB()->regHandler( &_accessPlanManager ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to register event handler of "
                      "access plan manager to DMS, rc: %d", rc ) ;
      }

      {
         UINT64 jobEDUID = 0 ;
         rtnClearExpireContextJob *job = SDB_OSS_NEW rtnClearExpireContextJob() ;
         PD_CHECK( NULL != job, SDB_OOM, error, PDERROR,
                  "Failed to allocate clear context job" ) ;

         rc = rtnGetJobMgr()->startJob( job, RTN_JOB_MUTEX_REUSE, &jobEDUID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to start clear-expired-context job, rc: %d", rc ) ;

         PD_LOG( PDDEBUG, "Start clear-expired-context job [%llu]", jobEDUID ) ;
      }

      if ( pmdGetOptionCB()->getUserCacheInterval() > 0 )
      {
         UINT64 jobID = 0 ;
         rtnClearUserCacheJob *job = SDB_OSS_NEW rtnClearUserCacheJob( this ) ;
         PD_CHECK( NULL != job, SDB_OOM, error, PDERROR, "Failed to allocate clear user cache job" ) ;

         rc = job->submit( TRUE, UTIL_LJOB_PRI_LOWEST, UTIL_LJOB_DFT_AVG_COST, &jobID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to submit clear user cache job, rc: %d", rc ) ;
         PD_LOG( PDDEBUG, "submit clear user cache job [%llu]", jobID ) ;
      }
   
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
      /// wait event
      _event.wait( RTN_CTX_CHECK_INTERVAL ) ;

      if ( _remoteMessenger )
      {
         _remoteMessenger->deactive() ;
      }
      if ( SDB_ROLE_DATA == pmdGetDBRole() ||
           SDB_ROLE_CATALOG == pmdGetDBRole() ||
           SDB_ROLE_STANDALONE == pmdGetDBRole() ||
           SDB_ROLE_OM == pmdGetDBRole() )
      {
         pmdGetKRCB()->getDMSCB()->unregHandler( &_accessPlanManager ) ;
      }
      return SDB_OK ;
   }

   INT32 _SDB_RTNCB::fini ()
   {
      _accessPlanManager.fini() ;

      // unregister event handle
      pmdGetKRCB()->unregEventHandler( this ) ;

      sdbGetDMSCB()->setIxmKeySorterCreator( NULL ) ;
      sdbGetDMSCB()->setScannerCheckerCreator( NULL ) ;

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

      _unloadCSSet.clear() ;

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
            optionCB->isEnabledMixCmp(),
            optionCB->getOptStartCostLimit() ) ;

      _maxContextNum = optionCB->maxContextNum() ;
      _maxSessionContextNum = optionCB->maxSessionContextNum() ;
      _contextTimeout = optionCB->contextTimeout() ;
   }

   INT32 _SDB_RTNCB::_fixContextInfo( _pmdEDUCB *cb, rtnContextInternalPtr &pContext )
   {
      INT32 rc = SDB_OK ;

      if ( cb )
      {
         pmdOperator *pOperator = cb->getOperator() ;
         MsgGlobalID sessionOpGlobalID  = pOperator->getGlobalID() ;
         MsgGlobalID contextGlobalID    = pContext->getGlobalID() ;

         /// reset the eduID when the it is invalid
         if ( PMD_INVALID_EDUID == pContext->eduID() )
         {
            SDB_ASSERT( pContext->isDetachMode(), "Context is not detach mode" ) ;
            if ( cb->contextInsert( pContext->contextID(), pContext->isDetachMode() ) )
            {
               if ( !ossCompareAndSwap64( &(pContext->_eduID),
                                          PMD_INVALID_EDUID,
                                          cb->getID() ) )
               {
                  cb->contextDelete( pContext->contextID() ) ;

                  PD_LOG ( PDWARNING, "Context %lld does not owned by "
                           "current session, owned by %llu", pContext->contextID(),
                           pContext->eduID() ) ;
                  rc = SDB_RTN_CONTEXT_NOTOWNED ;
                  goto error ;
               }

               /// rebind
               rc = pContext->_onRebind() ;
               if ( rc )
               {
                  goto error ;
               }

               if ( cb->getID() == pContext->_createEduID )
               {
                  pContext->_needAuth = FALSE ;
               }
               else
               {
                  pContext->_needAuth = TRUE ;
               }
            }
            else
            {
               rc = SDB_OOM ;
               goto error ;
            }
         }

         /// set global ID
         if ( sessionOpGlobalID.getQueryID() != contextGlobalID.getQueryID() )
         {
            // when getMore, the queryOpID should add 1
            contextGlobalID.incQueryOpID() ;
            pContext->_setGlobalID( contextGlobalID ) ;
            pOperator->updateGlobalID( contextGlobalID ) ;
         }
         else if ( sessionOpGlobalID.getQueryOpID() != contextGlobalID.getQueryOpID() )
         {
            pContext->_setGlobalID( sessionOpGlobalID ) ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _SDB_RTNCB::contextFind( INT64 contextID, UINT64 &ownedEDUID )
   {
      RTN_CTX_MAP::Bucket &blk = _contextMap.getBucket( contextID ) ;
      ossScopedLock lock( &blk, SHARED ) ;
      RTN_CTX_MAP::map_const_iterator cit = blk.find( contextID ) ;
      if ( cit != blk.end() )
      {
         ownedEDUID = cit->second->eduID() ;
         return TRUE ;
      }
      return FALSE ;
   }

   INT32 _SDB_RTNCB::contextFind( INT64 contextID,
                                  rtnContextPtr &context,
                                  _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      UINT64 bTick = pmdGetDBTick() ;
      BOOLEAN hasPrint = FALSE ;

      while( TRUE )
      {
         std::pair<rtnContextInternalPtr, bool> ret = _contextMap.find( contextID ) ;
         if ( ret.second )
         {
            if ( PMD_INVALID_EDUID != (ret.first)->eduID() &&
                 cb && !cb->contextFind( contextID ) )
            {
               if ( !hasPrint )
               {
                  PD_LOG_MSG ( PDWARNING, "Context %lld does not owned by "
                               "current session, owned by %llu", contextID,
                               (ret.first)->eduID() ) ;
                  hasPrint = TRUE ;
               }
               rc = SDB_RTN_CONTEXT_NOTOWNED ;
            }
            else
            {
               rc = _fixContextInfo( cb, ret.first ) ;
               if ( SDB_OK == rc )
               {
                  context.init( ret.first, cb ) ;
               }
            }
         }
         else
         {
            rc = SDB_RTN_CONTEXT_NOTEXIST ;
         }

         if ( SDB_RTN_CONTEXT_NOTOWNED == rc &&
              pmdGetTickSpanTime( bTick ) < RTN_FIND_CONTEXT_TIMEOUT  )
         {
            ossSleep( 10 ) ;
            continue ;
         }

         break ;
      }

      return rc ;
   }

   INT32 _SDB_RTNCB::contextFind( INT64 contextID,
                                  RTN_CONTEXT_TYPE type,
                                  rtnContextPtr &context,
                                  _pmdEDUCB *cb,
                                  BOOLEAN closeOnUnexpectType )
   {
      INT32 rc = SDB_OK ;

      rtnContextPtr tempContext ;
      rc = contextFind( contextID, tempContext, cb ) ;
      if ( SDB_OK == rc )
      {
         if ( type == tempContext->getType() )
         {
            context = tempContext ;
         }
         else
         {
            PD_LOG( PDWARNING, "Failed to find context [%llu] of type %d[%s], "
                    "current is %d[%s]", contextID, type,
                    getContextTypeDesp( type ), tempContext->getType(),
                    getContextTypeDesp( tempContext->getType() ) ) ;
            if ( closeOnUnexpectType )
            {
               contextDelete( contextID, cb ) ;
            }
            rc = SDB_SYS ;
         }
      }

      return rc ;
   }

   BOOLEAN _SDB_RTNCB::contextExist( INT64 contextID )
   {
      return _contextMap.find( contextID ).second ? TRUE : FALSE ;
   }

   BOOLEAN _SDB_RTNCB::returnContext( INT64 contextID )
   {
      /// set the context eduID to invalid
      std::pair<rtnContextInternalPtr, bool> ret = _contextMap.find( contextID ) ;
      if ( ret.second && PMD_IS_DB_UP() )
      {
         SDB_ASSERT( ret.first->isDetachMode(), "Context is not detach mode" ) ;
         (ret.first)->_onReturn() ;
         (ret.first)->_eduID = PMD_INVALID_EDUID ;
         (ret.first)->_needAuth = FALSE ;

         return TRUE ;
      }
      return FALSE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_RTNCB_CONTEXTDEL, "_SDB_RTNCB::contextDelete" )
   void _SDB_RTNCB::contextDelete ( INT64 contextID, IExecutor *pExe )
   {
      PD_TRACE_ENTRY ( SDB__SDB_RTNCB_CONTEXTDEL ) ;

      rtnContextInternalPtr pContext ;
      pmdEDUCB *cb = ( pmdEDUCB* )pExe ;

      if ( cb )
      {
         cb->contextDelete( contextID ) ;
      }

      {
         pair<rtnContextInternalPtr, bool> ret = _contextMap.find( contextID ) ;
         if ( ret.second )
         {
            /// must use rtnContextInternalPtr&, because rtnContextInternalPtr is thread local
            /// self. When many thread call the context in the same time, the ref maybe occur
            /// some wrongs.
            rtnContextInternalPtr &tmpPtr = ret.first ;

            /// reset the eduID when the it is invalid
            if ( PMD_INVALID_EDUID == tmpPtr->eduID() )
            {
               SDB_ASSERT( tmpPtr->isDetachMode(), "Context is not detach mode" ) ;
               if ( !ossCompareAndSwap64( &(tmpPtr->_eduID),
                                          PMD_INVALID_EDUID,
                                          cb->getID() ) )
               {         
                  PD_LOG ( PDWARNING, "Context %lld does not owned by "
                           "current session", tmpPtr->contextID() ) ;
                  goto done ;
               }

               /// rebind
               INT32 rcTmp = tmpPtr->_onRebind() ;
               if ( rcTmp )
               {
                  PD_LOG( PDWARNING, "Rebind context(%lld) to edu(%lld) failed when "
                          "deleting context, rc: %d", tmpPtr->contextID(),
                          tmpPtr->eduID(), rcTmp ) ;
                  tmpPtr->_eduID = PMD_INVALID_EDUID ;
                  goto done ;
               }
            }

            pContext = tmpPtr ;
            _contextMap.erase( contextID ) ;
         }
      }

      if ( pContext )
      {
         INT32 bufRef = pContext->getReference() ;
         INT64 ctxRef = pContext.refCount() ;

         // wait for pre-fetching
         pContext->waitForPrefetch() ;

         /// wait for sync
         if ( pContext->isWrite() && pContext->getDPSCB() &&
              pContext->getW() > 1 )
         {
            if ( NULL != cb )
            {
               if ( DPS_INVALID_LSN_OFFSET == cb->getEndLsn() &&
                    DPS_INVALID_LSN_OFFSET != pContext->getEndLSN() )
               {
                  cb->insertLsn( (UINT64)( pContext->getEndLSN() ) ) ;
               }
               cb->setOrgReplSize( pContext->getW() ) ;
            }
            pContext->getDPSCB()->completeOpr( cb, pContext->getW() ) ;
            pContext->resetEndLSN() ;
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
         pContext.release() ;

         PD_LOG( PDDEBUG, "delete context(contextID=%lld, reference: %u, "
                 "buffer reference: %d)", contextID, ctxRef, bufRef ) ;
      }

   done:
      PD_TRACE_EXIT ( SDB__SDB_RTNCB_CONTEXTDEL ) ;
      return ;
   }

   UINT32 _SDB_RTNCB::preDelContext( const CHAR *csName, UINT32 suLogicalID )
   {
      _RTN_EDU_CTX_MAP contexts ;

      SDB_ASSERT ( NULL != csName,
                   "collection space name should be valid" ) ;
      SDB_ASSERT ( DMS_INVALID_LOGICCSID != suLogicalID,
                   "logical ID should be valid" ) ;

      if ( 0 == _contextMap.size( TRUE ) ||
           DMS_INVALID_LOGICCSID == suLogicalID )
      {
         goto done ;
      }

      FOR_EACH_CMAP_ELEMENT_S( RTN_CTX_MAP, _contextMap )
      {
         rtnContext *pContext = it->second.get() ;

         if ( pContext &&
              pContext->isOpened() &&
              suLogicalID == pContext->getSULogicalID() )
         {
            try
            {
               EDUID eduID = pContext->eduID() ;
               INT64 contextID = pContext->contextID () ;
               contexts.insert( make_pair( eduID, contextID ) ) ;

               PD_LOG( PDDEBUG, "Pre-deleting context [%lld] of EDU [%llu] on "
                       "collection space [%s]", contextID, eduID, csName ) ;
            }
            catch ( exception &e )
            {
               PD_LOG( PDERROR, "Failed to save delete context, "
                       "occur exception  %s", e.what() ) ;
               // can continue
            }
         }
      }
      FOR_EACH_CMAP_ELEMENT_END

      _notifyKillContexts( contexts, sdbGetThreadExecutor() ) ;

   done:
      return contexts.size() ;
   }

   UINT32 _SDB_RTNCB::preDelExpiredContext( IExecutor *cb, BOOLEAN forceDetached )
   {
      _RTN_EDU_CTX_MAP contexts ;

      // config is in minutes, convert to milliseconds
      UINT64 contextTimeoutMS = _contextTimeout * 60 * OSS_ONE_SEC ;

      if ( !forceDetached && ( 0 >= contextTimeoutMS || 0 == _contextMap.size( TRUE ) ) )
      {
         goto done ;
      }

      FOR_EACH_CMAP_ELEMENT_S( RTN_CTX_MAP, _contextMap )
      {
         rtnContext *pContext = it->second.get() ;

         if ( pContext &&
              pContext->isOpened() &&
              ( ( pContext->needTimeout() && 1 == it->second.refCount() &&
                  pmdGetTickSpanTime( pContext->getLastProcessTick() ) > contextTimeoutMS ) ||
                ( forceDetached && PMD_INVALID_EDUID == pContext->eduID() ) ) )
         {
            try
            {
               contexts.insert( make_pair( pContext->eduID(),
                                           pContext->contextID() ) ) ;
               PD_LOG( PDEVENT, "Pre-deleting idle timeout context [%lld] "
                       "of EDU [%llu]", pContext->contextID(),
                       pContext->eduID() ) ;
            }
            catch ( exception &e )
            {
               PD_LOG( PDERROR, "Failed to save delete context, "
                       "occur exception  %s", e.what() ) ;
               // can continue
            }
         }
      }
      FOR_EACH_CMAP_ELEMENT_END

      _notifyKillContexts( contexts, cb ) ;

   done:
      return contexts.size() ;
   }

   INT32 _SDB_RTNCB::dumpWritingContext( RTN_CTX_PROCESS_LIST &contextProcessList,
                                         EDUID filterEDUID,
                                         UINT64 blockID )
   {
      INT32 rc = SDB_OK ;

      FOR_EACH_CMAP_ELEMENT_S( RTN_CTX_MAP, _contextMap )
      {
         rtnContext *pContext = it->second.get() ;

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
                     rtnCtxProcessInfo info ;
                     info._opID = pContext->getOpID() ;
                     info._ctxID = pContext->contextID() ;
                     info._eduID = pContext->eduID() ;
                     info._processName.assign( processName ) ;
                     contextProcessList.push_back( info ) ;
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

   void _SDB_RTNCB::_notifyKillContexts( const _RTN_EDU_CTX_MAP &contexts, IExecutor *cb )
   {
      pmdEDUMgr *pEDUMgr = pmdGetKRCB()->getEDUMgr() ;
      EDUID eduID = PMD_INVALID_EDUID ;

      for ( _RTN_EDU_CTX_MAP::const_iterator iter = contexts.begin() ;
            iter != contexts.end() ;
            ++ iter )
      {
         eduID = iter->first ;

         if ( PMD_INVALID_EDUID == eduID )
         {
            eduID = cb->getID() ;
         }

         try
         {
            pEDUMgr->postEDUPost( eduID,
                                  PMD_EDU_EVENT_KILLCONTEXT,
                                  PMD_EDU_MEM_NONE,
                                  NULL,
                                  (UINT64)( iter->second ) ) ;
         }
         catch ( exception &e )
         {
            PD_LOG( PDERROR, "Failed to post event to EDU [%llu], "
                    "occur exception %s", iter->first, e.what() ) ;
            // can continue
         }

         PD_LOG( PDDEBUG, "post kill context [%lld] to EDU [%llu]",
                 iter->second, iter->first ) ;
      }

      /// need to check urgent events
      cb->isInterrupted( TRUE ) ;
   }

   INT32 _SDB_RTNCB::contextNew( RTN_CONTEXT_TYPE type,
                                 rtnContextPtr &context,
                                 INT64 &contextID,
                                 _pmdEDUCB * pEDUCB )
   {
      rtnContextInternalPtr newContext ;
      monSvcTaskInfo *pTaskInfo = NULL ;
      BOOLEAN isDetachMode = FALSE ;

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

      if ( PMD_IS_DB_UP() && pEDUCB->getOperator()->isContextDetachMode() )
      {
         isDetachMode = TRUE ;
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

      newContext = sdbGetRTNContextBuilder()->create( type, _contextId, pEDUCB->getID() ) ;

      if ( !newContext )
      {
         return SDB_OOM ;
      }

      newContext->_enableDetachMode( isDetachMode ) ;
      newContext->_setUsername( pEDUCB->getUserNameStr() ) ;
      /// when the context is not detached, but the operator enabled detach mode,
      /// so, shoud disable the operator's detach mode
      if ( pEDUCB->getOperator()->isContextDetachMode() && !newContext->isDetachMode() )
      {
         pEDUCB->getOperator()->disableContextDetachMode( pEDUCB ) ;
      }

      newContext->_setBatchLimited( pEDUCB->getOperator()->isContextBatchLimited() ) ;

      if ( !( _contextMap.insert( _contextId, newContext ).second ) )
      {
         newContext.release() ;
         return SDB_OOM ;
      }

      if ( !pEDUCB->contextInsert( _contextId, newContext->isDetachMode() ) )
      {
         _contextMap.erase( _contextId ) ;
         newContext.release() ;
         return SDB_OOM ;
      }

      context.init( newContext, pEDUCB, FALSE ) ;
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
         context->setMonQueryCB( monQuery ) ;
         monQuery->anchorToContext = TRUE ;
      }

      if ( pEDUCB->getMonConfigCB()->timestampON )
      {
         context->getMonCB()->recordStartTimestamp() ;
      }
      context->setOpID( pEDUCB->getWritingID() ) ;

      // only check timeout for contexts from local service
      if ( !pEDUCB->isFromLocal() && !context->isDetachMode() )
      {
         context->disableTimeout() ;
      }

      context->_setGlobalID( pEDUCB->getOperator()->getGlobalID() ) ;
      context->_setRemainingMaxTime( pEDUCB->getOperator()->getRemainingMaxTime() ) ;

      PD_LOG ( PDDEBUG, "Create new context(contextID=%lld, type: %d[%s], "
               "writing ID %llu)",
               contextID, type, getContextTypeDesp(type),
               context->getOpID() ) ;

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

   INT32 _SDB_RTNCB::addUnloadCS( const CHAR* csName )
   {
      ossScopedLock lock( &_csLatch, EXCLUSIVE ) ;
      try
      {
         _unloadCSSet.insert( csName ) ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         return ossException2RC( &e ) ;
      }
      return SDB_OK ;
   }

   void _SDB_RTNCB::delUnloadCS( const CHAR* csName )
   {
      ossScopedLock lock( &_csLatch, EXCLUSIVE ) ;
      _unloadCSSet.erase( csName ) ;
   }

   BOOLEAN _SDB_RTNCB::hasUnloadCS( const CHAR* csName )
   {
      BOOLEAN has = FALSE ;

      if ( _unloadCSSet.size() != 0 )
      {
         ossScopedLock lock( &_csLatch, SHARED ) ;
         if ( _unloadCSSet.count( csName ) != 0 )
         {
            has = TRUE ;
         }
      }

      return has ;
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

