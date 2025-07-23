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

   Source File Name = stpAgent.cpp

   Descriptive Name = Serial Time Protocol

   When/how to use: this program may be used on binary and text-formatted
   versions of STP component. This file contains structure for Serial Time
   Protocol.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/30/2019  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#include "stpAgent.hpp"
#include "pdTrace.hpp"
#include "stpTrace.hpp"
#include "pmd.hpp"
#include "stpMsg.hpp"

namespace engine
{

   // max count to retry to get logical time
   #define STP_AGENT_MAX_RETRY ( 2 )

   /*
      _stpAgentService define and implement
    */
   class _stpAgentService : public stpMetaReader
   {
   public:
      _stpAgentService() ;
      ~_stpAgentService() ;

   public:
      // quick check if STP is available
      BOOLEAN isAvailable() ;

      // check if STP is available
      INT32 checkAvailable() ;

      // notify the STP to synchronize with server
      INT32 notifySync() ;

      // get logical time in nanosecond in given timeout
      // output:
      // - time: global logical time returned from STP
      // input:
      // - timeout: timeout to get global logical time
      // - monotonic: indicate if monotonic time is required
      // return:
      // - SDB_OK: succeed to get global logical time
      // - other return code: failed to get global logical time
      // NOTE: `timeout` is -1 means never timeout
      //       `timeout` is 0 means only try once
      INT32 getLogicalTimeNS( stpLogicalTimeNS &time,
                              INT32 timeout = -1,
                              BOOLEAN monotonic = TRUE ) ;

      INT32 getClient( stpClient &client ) ;

   protected:
      // clear agent
      void  _clear() ;
      // get STP node by checking PID
      INT32 _getSTP() ;
      // test alive of STP node
      INT32 _testSTP() ;
      // notify STP to synchronize
      INT32 _notifySTPSync() ;
      // check and attach meta data
      INT32 _checkMetaData( const CHAR *shmKey ) ;
      // release meta data
      INT32 _releaseMetaData() ;
      // get logical time in nanoseconds
      INT32 _getLogicalTimeNS( stpLogicalTimeNS &time,
                               BOOLEAN monotonic,
                               UINT32 &waitTimeUS ) ;

      // attach shared memory buffer
      INT32 _attachSHMBuffer( const CHAR *shmKey ) ;
      // release shared memory buffer
      INT32 _releaseSHMBuffer() ;
      // re-check if we could retry to get logical time
      BOOLEAN _recheckAvailable( INT32 rc, BOOLEAN &needWait ) ;

   protected:
      // indicate if STP is available for service
      ossAtomic32       _availableFlag ;

      // latch to protect checking STP
      // NOTE: only one thread could launch STP checking
      ossAtomicXLatch   _metaCheckLatch ;

      // version of STP meta data ( increase for each attach )
      ossAtomic32       _metaVersion ;

      // PID of STP
      OSSPID            _stpPID ;
      // host name
      CHAR              _hostName[ OSS_MAX_HOSTNAME + 1 ] ;
      // service name ( port ) of STP
      CHAR              _serviceName[ OSS_MAX_SERVICENAME + 1 ] ;

      // lock to protect meta data from shared memory
      // - when reads meta data, should get the shared lock
      // - when attaches or releases meta data, should get the exclusive lock
      ossRWMutex        _metaMutex ;
      // shared memory buffer
      utilSHMBuffer     _buffer ;

      // latch to protect notification of STP synchronization
      ossAtomicXLatch   _syncLatch ;
      // last tick to synchronize notification
      ossAtomic64       _lastSyncTick ;
   } ;

   typedef class _stpAgentService stpAgentService ;

   _stpAgentService::_stpAgentService()
   : stpMetaReader(),
     _availableFlag( 0 ),
     _metaVersion( 0 ),
     _stpPID( OSS_INVALID_PID ),
     _lastSyncTick( 0LL )
   {
      if ( SDB_OK != ossGetHostName( _hostName, OSS_MAX_HOSTNAME ) )
      {
         _hostName[ 0 ] = '\0' ;
      }
      _serviceName[ 0 ] = '\0' ;
   }

   _stpAgentService::~_stpAgentService()
   {
      _clear() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPAGENTSERVICE_ISAVAILABLE, "_stpAgentService::isAvailable" )
   BOOLEAN _stpAgentService::isAvailable()
   {
      BOOLEAN available = FALSE ;

      PD_TRACE_ENTRY( SDB__STPAGENTSERVICE_ISAVAILABLE ) ;

      available = _availableFlag.compare( 1 ) ;

      PD_TRACE_EXIT( SDB__STPAGENTSERVICE_ISAVAILABLE ) ;

      return available ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPAGENTSERVICE_CHECKAVAILABLE, "_stpAgentService::checkAvailable" )
   INT32 _stpAgentService::checkAvailable()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPAGENTSERVICE_CHECKAVAILABLE ) ;

      BOOLEAN gotCheckLatch = FALSE ;

      // the caller try to check available of STP, to avoid too many threads
      // to access the test pipe of STP node, we could only allow one thread
      // to do the testing

      // when entering the critical section, current thread needn't to wait,
      // just try to get the latch, if failed to get the latch, means someone
      // is doing the same check, so current thread could leave the check to
      // that one

      // also, we need to check meta version, if some one changed meta version
      // during current thread to get the latch, which means a check just
      // finished, so current thread could use the result of that finished
      // check

      // fetch meta version before entry critical section
      UINT32 metaVersion = _metaVersion.fetch() ;

      // critical section: only one thread could check available in concurrent
      if ( !_metaCheckLatch.try_get() )
      {
         // if we failed to get latch, means someone else is updating,
         // just goto done
         goto done ;
      }

      // entered critical section
      gotCheckLatch = TRUE ;

      if ( _metaVersion.fetch() != metaVersion )
      {
         // someone else have updated, to avoid checking too frequently,
         // just goto done
         if ( !isAvailable() )
         {
            PD_LOG( PDERROR, "Failed to check available, still not available "
                   "after check" ) ;
            rc = STP_NOT_AVAILABLE ;
         }
         goto done ;
      }

      _metaVersion.inc() ;

      // check PID of STP
      if ( OSS_INVALID_PID == _stpPID )
      {
         // if PID is invalid, get STP node
         rc = _getSTP() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get STP node, rc: %d", rc ) ;
      }

      // test alive of STP
      rc = _testSTP() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to test STP node, rc: %d", rc ) ;

      // check meta data
      rc = _checkMetaData( _serviceName ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check meta data with key [%s], "
                   "rc: %d", _serviceName, rc ) ;

   done:
      // exit critical section
      if ( gotCheckLatch )
      {
         _metaCheckLatch.release() ;
      }
      PD_TRACE_EXITRC( SDB__STPAGENTSERVICE_CHECKAVAILABLE, rc ) ;
      return rc ;

   error:
      // when error happened, clear agent
      SDB_ASSERT( gotCheckLatch, "should in critical section" ) ;
      _clear() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPAGENTSERVICE_NOTIFYSYNC, "_stpAgentService::notifySync" )
   INT32 _stpAgentService::notifySync()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPAGENTSERVICE_NOTIFYSYNC ) ;

      BOOLEAN available = isAvailable() ;

      if ( available )
      {
         // to avoid notify too frequency
         UINT64 notifyPassed = pmdGetTickSpanTime( _lastSyncTick.fetch() ) ;
         if ( notifyPassed <= STP_SEC_TO_MILLISEC( STP_MIN_SYNC_INTERVAL ) )
         {
            goto done ;
         }
         // try to get synchronize latch
         if ( !_syncLatch.try_get() )
         {
            goto done ;
         }
         // notify STP to synchronize
         rc = _notifySTPSync() ;
         if ( SDB_OK != rc )
         {
            // if not succeed, check available later
            PD_LOG( PDWARNING, "Failed to notify STP to synchronize, rc: %d",
                    rc ) ;
            available = FALSE ;
         }
         else
         {
            // set last synchronize time
            _lastSyncTick.swapGreaterThan( pmdGetDBTick() ) ;
         }
         _syncLatch.release() ;
      }

      // check available if needed
      if ( !available )
      {
         rc = checkAvailable() ;
         PD_RC_CHECK( rc, PDWARNING, "Failed to check STP available with "
                      "notifying synchronize, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPAGENTSERVICE_NOTIFYSYNC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPAGENTSERVICE_GETLOGICALTIMENS, "_stpAgentService::getLogicalTimeNS" )
   INT32 _stpAgentService::getLogicalTimeNS( stpLogicalTimeNS &time,
                                             INT32 timeout,
                                             BOOLEAN monotonic )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPAGENTSERVICE_GETLOGICALTIMENS ) ;

      INT32 totalTimeout = 0 ;

      while ( TRUE )
      {
         BOOLEAN needWait = FALSE ;
         UINT32 waitTimeUS = 0 ;

         // get logical time
         rc = _getLogicalTimeNS( time, monotonic, waitTimeUS ) ;
         if ( SDB_OK == rc )
         {
            // get time is OK, break loop
            break ;
         }
         else if ( timeout > 0 && totalTimeout > timeout )
         {
            // check timeout
            // NOTE: timeout < 0 means never timeout
            rc = SDB_TIMEOUT ;
            break ;
         }
         else if ( 0 == timeout )
         {
            // timeout is 0, means try once
            break ;
         }
         else if ( _recheckAvailable( rc, needWait ) )
         {
            if ( needWait )
            {
               UINT32 waitTime = STP_MICROSEC_TO_MILLISEC( waitTimeUS ) ;

               if ( 0 == waitTime )
               {
                  waitTime = STP_AGENT_RETRY_INTERVAL ;
               }

               // reset wait time against timeout
               // don't make it sleep for a long time
               if ( timeout > 0 && waitTime > (UINT32)timeout )
               {
                  waitTime = (UINT32)timeout ;
               }
               else if ( waitTime > OSS_ONE_SEC )
               {
                  waitTime = OSS_ONE_SEC ;
               }

               // we could retry, sleep and continue loop
               ossSleep( waitTime ) ;
               if ( timeout > 0 )
               {
                  totalTimeout += waitTime ;
               }
            }
            continue ;
         }
         // we could not retry, break loop
         break ;
      }

      PD_RC_CHECK( rc, PDERROR, "Failed to get logical time, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPAGENTSERVICE_GETLOGICALTIMENS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPAGENTSERVICE_GETCLIENT, "_stpAgentService::getClient" )
   INT32 _stpAgentService::getClient( stpClient &client )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPAGENTSERVICE_GETCLIENT ) ;

      ossScopedRWLock lock( &_metaMutex, SHARED ) ;

      // check available
      PD_CHECK( isAvailable(), STP_NOT_AVAILABLE, error, PDERROR,
                "Failed to get logical time, STP is not available" ) ;
      // check meta data
      PD_CHECK( NULL != getMetaData(), STP_NOT_AVAILABLE, error, PDERROR,
                "Failed to get logical time, meta data is not available" ) ;

      rc = client.setConnInfo( _hostName, _serviceName ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to set connection information for "
                   "STP client, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPAGENTSERVICE_GETCLIENT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPAGENTSERVICE__CLEAR, "_stpAgentService::_clear" )
   void _stpAgentService::_clear()
   {
      PD_TRACE_ENTRY( SDB__STPAGENTSERVICE__CLEAR ) ;

      // reset PID of STP
      _stpPID = OSS_INVALID_PID ;
      // reset service name of STP
      _serviceName[ 0 ] = '\0' ;
      // release meta data
      _releaseMetaData() ;
      // reset synchronize notification tick
      _lastSyncTick.swap( 0LL ) ;

      PD_TRACE_EXIT( SDB__STPAGENTSERVICE__CLEAR ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPAGENTSERVICE__GETSTP, "_stpAgentService::_getSTP" )
   INT32 _stpAgentService::_getSTP()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPAGENTSERVICE__GETSTP ) ;

      utilNodeInfo node ;
      UTIL_VEC_NODES listNodes ;

      // check host name if needed
      if ( '\0' == _hostName[ 0 ] )
      {
         rc = ossGetHostName( _hostName, OSS_MAX_HOSTNAME ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDWARNING, "Failed to get host name, rc: %d", rc ) ;
            _hostName[ 0 ] = '\0' ;
         }
      }

      // list running nodes, filtered by STP
      rc = utilListNodes( listNodes, SDB_TYPE_STP, NULL, OSS_INVALID_PID, -1 ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get STP node, rc: %d", rc ) ;

      // check if list is empty
      PD_CHECK( !listNodes.empty(), SDB_CLS_NODE_NOT_EXIST, error, PDERROR,
                "Failed to get STP node, it does not exist" ) ;
      // should have only one STP
      PD_CHECK( listNodes.size() > 0, SDB_SYS, error, PDERROR,
                "Failed to get STP node, too many STP nodes [%d]",
                listNodes.size() ) ;

      // get node
      node = listNodes.front() ;

      PD_LOG( PDINFO, "got STP node [%s] pid [%u]", node._svcname.c_str(),
              node._pid ) ;

      // extract service name
      ossStrncpy( _serviceName, node._svcname.c_str(), OSS_MAX_SERVICENAME ) ;

      // extract PID
      _stpPID = node._pid ;

   done:
      PD_TRACE_EXITRC( SDB__STPAGENTSERVICE__GETSTP, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPAGENTSERVICE__TESTSTP, "_stpAgentService::_testSTP" )
   INT32 _stpAgentService::_testSTP()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPAGENTSERVICE__TESTSTP ) ;

      INT8 test = 0 ;

      // write test command to pipe
      rc = utilWriteReadPipe( STP_PIPE_SERVICE_NAME,
                              _stpPID,
                              STP_PIPE_MSG_TEST,
                              sizeof( STP_PIPE_MSG_TEST ),
                              (CHAR *)( &test ),
                              sizeof( test ),
                              FALSE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to test from STP "
                   "node [%s] pid [%u], rc: %d", _serviceName,
                   _stpPID, rc ) ;

      PD_LOG( PDINFO, "Send STP node [%s] pid [%u] with command [%s] done",
              _serviceName, _stpPID, STP_PIPE_MSG_TEST ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPAGENTSERVICE__TESTSTP, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPAGENTSERVICE__NOTIFYSTPSYNC, "_stpAgentService::_notifySTPSync" )
   INT32 _stpAgentService::_notifySTPSync()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPAGENTSERVICE__NOTIFYSTPSYNC ) ;

      // write sync command to pipe ( no need to reply )
      rc = utilWritePipe( STP_PIPE_SERVICE_NAME,
                          _stpPID,
                          STP_PIPE_MSG_SYNC,
                          sizeof( STP_PIPE_MSG_SYNC ) ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to notify synchronize to STP "
                   "node [%s] pid [%u], rc: %d", _serviceName,
                   _stpPID, rc ) ;

      PD_LOG( PDINFO, "Send STP node [%s] pid [%u] with command [%s] done",
              _serviceName, _stpPID, STP_PIPE_MSG_SYNC ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPAGENTSERVICE__NOTIFYSTPSYNC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPAGENTSERVICE__CHECKMETADATA, "_stpAgentService::_checkMetaData" )
   INT32 _stpAgentService::_checkMetaData( const CHAR *shmKey )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPAGENTSERVICE__CHECKMETADATA ) ;

      SDB_ASSERT( NULL != shmKey, "shared memory key is invalid" ) ;

      BOOLEAN needAttach = FALSE ;

      ossScopedRWLock( &_metaMutex, SHARED ) ;

      // if it is not available, means the buffer is not attached
      needAttach = _availableFlag.compare( 0 ) ;

      // if available but key of shared buffer is changed, release the old one
      if ( !needAttach && 0 != ossStrcmp( shmKey, _buffer.getKeyString() ) )
      {
         // reset available
         _availableFlag.swap( 0 ) ;
         // release buffer
         _releaseSHMBuffer() ;
         // need re-attach
         needAttach = TRUE ;
      }

      if ( needAttach )
      {
         // attach shared memory buffer with given key
         rc = _attachSHMBuffer( shmKey ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to attach meta data [%s], rc: %d",
                      shmKey, rc ) ;
      }

      // set available
      _availableFlag.swap( 1 ) ;


   done:
      PD_TRACE_EXITRC( SDB__STPAGENTSERVICE__CHECKMETADATA, rc ) ;
      return rc ;

   error:
      // when error happened, release shared memory buffer
      _availableFlag.swap( 0 ) ;
      _releaseSHMBuffer() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPAGENT__RELEASEMETADATA, "_stpAgentService::_releaseMetaData" )
   INT32 _stpAgentService::_releaseMetaData()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPAGENT__RELEASEMETADATA ) ;

      // reset available
      _availableFlag.swap( 0 ) ;

      ossScopedRWLock lock( &_metaMutex, EXCLUSIVE ) ;

      // release shared memory buffer
      rc = _releaseSHMBuffer() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to release meta data, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPAGENT__RELEASEMETADATA, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPAGENT__GETLOGICALTIMENS, "_stpAgentService::_getLogicalTimeNS" )
   INT32 _stpAgentService::_getLogicalTimeNS( stpLogicalTimeNS &time,
                                              BOOLEAN monotonic,
                                              UINT32 &waitTimeUS )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPAGENT__GETLOGICALTIMENS ) ;

      ossScopedRWLock lock( &_metaMutex, SHARED ) ;

      // check available
      PD_CHECK( isAvailable(), STP_NOT_AVAILABLE, error, PDERROR,
                "Failed to get logical time, STP is not available" ) ;
      // check meta data
      PD_CHECK( NULL != getMetaData(), STP_NOT_AVAILABLE, error, PDERROR,
                "Failed to get logical time, meta data is not available" ) ;

      // get logical time from meta data
      rc = getMetaData()->getLogicalTimeNS( time,
                                            monotonic,
                                            TRUE,
                                            waitTimeUS ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get logical time, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPAGENT__GETLOGICALTIMENS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPAGENT__ATTACHSHMBUFFER, "_stpAgentService::_attachSHMBuffer" )
   INT32 _stpAgentService::_attachSHMBuffer( const CHAR *shmKey )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPAGENT__ATTACHSHMBUFFER ) ;

      SDB_ASSERT( NULL != shmKey, "shared memory key is invalid" ) ;

      stpMetaData *metaData = NULL ;

      // release old shared memory buffer
      _releaseSHMBuffer() ;

      // try attach buffer
      PD_CHECK( _buffer.attach( shmKey, stpMetaData::getBufferSize() ),
                SDB_SYS, error, PDERROR, "Failed to attache shared memory "
                "buffer [key: %s, size: %d]", shmKey,
                stpMetaData::getBufferSize() ) ;
      // check if buffer is valid
      PD_CHECK( NULL != _buffer.getBuffer(), SDB_OOM, error, PDERROR,
                "Failed to attach shared memory buffer [key: %s, size: %d], "
                "it is empty", shmKey, stpMetaData::getBufferSize() ) ;

      PD_LOG( PDEVENT, "Attach shared memory [key: %s, size: %d, id: %llu]",
              _buffer.getKeyString(), _buffer.getSize(), _buffer.getID() ) ;

      // get meta data from shared memory buffer
      metaData = stpMetaData::getBuffer( _buffer.getBuffer() ) ;
      PD_CHECK( NULL != metaData, SDB_SYS, error, PDERROR,
                "Failed to attach meta data" ) ;

      // check version support
      PD_CHECK( STP_VERSION >= metaData->getVersion(), SDB_SYS, error, PDERROR,
                "Failed to get meta data, STP version [%u] is higher than "
                "local [%u]", metaData->getVersion(), STP_VERSION ) ;

      // set meta data
      setMetaData( metaData ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPAGENT__ATTACHSHMBUFFER, rc ) ;
      return rc ;

   error:
      // when error happened, release shared memory buffer
      _releaseSHMBuffer() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPAGENT__RELEASESHMBUFFER, "_stpAgentService::_releaseSHMBuffer" )
   INT32 _stpAgentService::_releaseSHMBuffer()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPAGENT__RELEASESHMBUFFER ) ;

      // reset meta data
      setMetaData( NULL ) ;
      // release shared memory buffer
      _buffer.release() ;

      PD_TRACE_EXITRC( SDB__STPAGENT__RELEASESHMBUFFER, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPAGENT__RECHECKAVAILABLE, "_stpAgentService::_recheckAvailable" )
   BOOLEAN _stpAgentService::_recheckAvailable( INT32 rc, BOOLEAN &needWait )
   {
      BOOLEAN canRetry = FALSE ;

      PD_TRACE_ENTRY( SDB__STPAGENT__RECHECKAVAILABLE ) ;

      needWait = FALSE ;

      switch ( rc )
      {
         case STP_NOT_AVAILABLE :
         case STP_SYNC_FAILED :
         {
            // it is not synchronized or not available
            // in these cases, STP might not started, so check available
            if ( SDB_OK == checkAvailable() )
            {
               // it is available now, go retry
               canRetry = TRUE ;
               needWait = FALSE ;
            }
            break ;
         }
         case STP_TIME_AHEAD_AFTER_SYNC :
         {
            // time of STP is ahead of global
            canRetry = TRUE ;
            needWait = TRUE ;
            break ;
         }
         case STP_SYNC_BUSY :
         {
            // time of STP is busy synchronizing
            canRetry = TRUE ;
            needWait = FALSE ;
            break ;
         }
         default :
         {
            // do noting
            break ;
         }
      }

      PD_TRACE_EXIT( SDB__STPAGENT__RECHECKAVAILABLE ) ;

      return canRetry ;
   }

   static stpAgentService *_stpGetAgentService()
   {
      static stpAgentService s_service ;
      return ( &s_service ) ;
   }

   /*
      _stpAgent implement
    */
   _stpAgent::_stpAgent()
   {
   }

   _stpAgent::~_stpAgent()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPAGENT_ISAVAILABLE, "_stpAgent::isAvailable" )
   BOOLEAN _stpAgent::isAvailable()
   {
      BOOLEAN available = FALSE ;

      PD_TRACE_ENTRY( SDB__STPAGENT_ISAVAILABLE ) ;

      stpAgentService *service = _stpGetAgentService() ;

      SDB_ASSERT( NULL != service, "service is invalid" ) ;

      if ( NULL != service )
      {
         available = service->isAvailable() ;
      }

      PD_TRACE_EXIT( SDB__STPAGENT_ISAVAILABLE ) ;

      return available ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPAGENT_CHECKAVAILABLE, "_stpAgent::checkAvailable" )
   INT32 _stpAgent::checkAvailable()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPAGENT_CHECKAVAILABLE ) ;

      stpAgentService *service = _stpGetAgentService() ;

      SDB_ASSERT( NULL != service, "service is invalid" ) ;

      PD_CHECK( NULL != service, STP_NOT_AVAILABLE, error, PDERROR,
                "Failed to get STP agent service" ) ;

      rc = service->checkAvailable() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check STP available, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPAGENT_CHECKAVAILABLE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPAGENT_NOTIFYSYNC, "_stpAgent::notifySync" )
   INT32 _stpAgent::notifySync()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPAGENT_NOTIFYSYNC ) ;

      stpAgentService *service = _stpGetAgentService() ;

      SDB_ASSERT( NULL != service, "service is invalid" ) ;

      PD_CHECK( NULL != service, STP_NOT_AVAILABLE, error, PDERROR,
                "Failed to get STP agent service" ) ;

      rc = service->notifySync() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to notify STP to synchronize "
                   "with server, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPAGENT_NOTIFYSYNC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPAGENT_GETLOGICALTIMENS, "_stpAgent::getLogicalTimeNS" )
   INT32 _stpAgent::getLogicalTimeNS( stpLogicalTimeNS &time,
                                      INT32 timeout,
                                      BOOLEAN monotonic )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPAGENT_GETLOGICALTIMENS ) ;

      stpAgentService *service = _stpGetAgentService() ;

      SDB_ASSERT( NULL != service, "service is invalid" ) ;

      PD_CHECK( NULL != service, STP_NOT_AVAILABLE, error, PDERROR,
                "Failed to get STP agent service" ) ;

      rc = service->getLogicalTimeNS( time, timeout, monotonic ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get logical time, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPAGENT_GETLOGICALTIMENS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPAGENT_GETLOGICALTIMEUS, "_stpAgent::getLogicalTimeUS" )
   INT32 _stpAgent::getLogicalTimeUS( stpLogicalTimeUS &time,
                                      INT32 timeout,
                                      BOOLEAN monotonic )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPAGENT_GETLOGICALTIMEUS ) ;

      stpLogicalTimeNS timeNS ;

      // get logical time
      rc = getLogicalTimeNS( timeNS, timeout, monotonic ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get logical time, rc: %d", rc ) ;

      time = timeNS ;

   done:
      PD_TRACE_EXITRC( SDB__STPAGENT_GETLOGICALTIMEUS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPAGENT_GETCLIENT, "_stpAgent::getClient" )
   INT32 _stpAgent::getClient( stpClient &client )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPAGENT_GETCLIENT ) ;

      stpAgentService *service = _stpGetAgentService() ;

      SDB_ASSERT( NULL != service, "service is invalid" ) ;

      rc = service->getClient( client ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to get STP client, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPAGENT_GETCLIENT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
