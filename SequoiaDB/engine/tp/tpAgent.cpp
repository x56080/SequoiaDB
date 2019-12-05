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

   Source File Name = tpAgent.cpp

   Descriptive Name = SequoiaDB Time Protocol Service

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for SequoiaDB
   Time Protocol Service.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/30/2019  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "tpAgent.hpp"
#include "pdTrace.hpp"
#include "tpTrace.hpp"
#include "pmd.hpp"
#include "msgTp.hpp"

namespace engine
{

   #define TP_AGENT_MAX_RETRY ( 2 )

   /*
      _tpAgent implement
    */
   _tpAgent::_tpAgent()
   : tpMetaReader(),
     _checkJobEDUID( PMD_INVALID_EDUID ),
     _tpPID( OSS_INVALID_PID ),
     _metaVersion( TP_VERSION ),
     _syncInterval( TP_DEF_SYNC_INTERVAL ),
     _available( FALSE )
   {
   }

   _tpAgent::~_tpAgent()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPAGENT_ACTIVE, "_tpAgent::active" )
   INT32 _tpAgent::active()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPAGENT_ACTIVE ) ;

      if ( SDB_OK != checkAvailable() )
      {
         PD_LOG( PDWARNING, "Failed to check TP available" ) ;
      }

      rc = tpAgentStartCheckJob( this, &_checkJobEDUID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to start TP agent check job, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPAGENT_ACTIVE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPAGENT_DEACTIVE, "_tpAgent::deactive" )
   INT32 _tpAgent::deactive()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPAGENT_DEACTIVE ) ;

      if ( PMD_INVALID_EDUID != _checkJobEDUID )
      {
         pmdGetKRCB()->getEDUMgr()->forceUserEDU( _checkJobEDUID ) ;
         _checkJobEDUID = PMD_INVALID_EDUID ;

         _checkEvent.signalAll() ;
      }

      clear() ;

      PD_TRACE_EXITRC( SDB__TPAGENT_DEACTIVE, rc ) ;

      return rc ;

   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPAGENT_GETLOGICALTIMENS, "_tpAgent::getLogicalTimeNS" )
   INT32 _tpAgent::getLogicalTimeNS( tpLogicalTimeNS &time )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPAGENT_GETLOGICALTIMENS ) ;

      rc = _getLogicalTimeNS( time ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get logical time, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPAGENT_GETLOGICALTIMENS, rc ) ;
      return rc ;

   error:
      _signalCheck() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPAGENT_GETLOGICALTIMEUS, "_tpAgent::getLogicalTimeUS" )
   INT32 _tpAgent::getLogicalTimeUS( tpLogicalTimeUS &time )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPAGENT_GETLOGICALTIMEUS ) ;

      rc = _getLogicalTimeUS( time ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get logical time, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPAGENT_GETLOGICALTIMEUS, rc ) ;
      return rc ;

   error:
      _signalCheck() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPAGENT_CHECKAVAILABLE, "_tpAgent::checkAvailable" )
   INT32 _tpAgent::checkAvailable()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPAGENT_CHECKAVAILABLE ) ;

      if ( OSS_INVALID_PID == _tpPID )
      {
         rc = _getTPNode() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get TP node, rc: %d", rc ) ;
      }

      rc = _testTPNode() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to test TP node, rc: %d", rc ) ;

      rc = _checkMetaData( _tpServiceName.c_str() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check meta data with key [%s], "
                   "rc: %d", _tpServiceName.c_str(), rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPAGENT_CHECKAVAILABLE, rc ) ;
      return rc ;

   error:
      clear() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPAGENT_WAITCHECKEVENT, "_tpAgent::waitCheckEvent" )
   void _tpAgent::waitCheckEvent()
   {
      PD_TRACE_ENTRY( SDB__TPAGENT_WAITCHECKEVENT ) ;

      if ( SDB_OK == _checkEvent.wait( _getCheckInterval() ) )
      {
         _getCheckInterval() ;
      }

      PD_TRACE_EXIT( SDB__TPAGENT_WAITCHECKEVENT ) ;
   }


   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPAGENT_CLEAR, "_tpAgent::clear" )
   void _tpAgent::clear()
   {
      PD_TRACE_ENTRY( SDB__TPAGENT_CLEAR ) ;

      _tpPID = OSS_INVALID_PID ;
      _tpServiceName.clear() ;
      _releaseMetaData() ;

      PD_TRACE_EXIT( SDB__TPAGENT_CLEAR ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPAGENT__GETTPNODE, "_tpAgent::_getTPNode" )
   INT32 _tpAgent::_getTPNode()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPAGENT__GETTPNODE ) ;

      utilNodeInfo node ;
      UTIL_VEC_NODES listNodes ;

      rc = utilListNodes( listNodes, SDB_TYPE_TP, NULL, OSS_INVALID_PID, -1 ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get TP node, rc: %d", rc ) ;

      PD_CHECK( !listNodes.empty(), SDB_CLS_NODE_NOT_EXIST, error, PDERROR,
                "Failed to get TP node, it does not exist" ) ;
      PD_CHECK( listNodes.size() > 0, SDB_SYS, error, PDERROR,
                "Failed to get TP node, too many TP nodes [%d]",
                listNodes.size() ) ;

      node = listNodes.front() ;

      PD_LOG( PDINFO, "got TP node [%s] pid [%u]", node._svcname.c_str(),
              node._pid ) ;

      _tpServiceName.assign( node._svcname.c_str() ) ;
      _tpPID = node._pid ;

   done:
      PD_TRACE_EXITRC( SDB__TPAGENT__GETTPNODE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPAGENT__TESTTPNODE, "_tpAgent::_testTPNode" )
   INT32 _tpAgent::_testTPNode()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPAGENT__TESTTPNODE ) ;

      INT8 test = 0 ;

      rc = utilWriteReadPipe( TP_PIPE_SERVICE_NAME, _tpPID,
                              TP_PIPE_MSG_TEST,
                              sizeof( TP_PIPE_MSG_TEST ),
                              (CHAR *)( &test ), sizeof( test ), FALSE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to test from TP "
                   "node [%s] pid [%u], rc: %d", _tpServiceName.c_str(),
                   _tpPID, rc ) ;

      PD_LOG( PDINFO, "Test TP node [%s] pid [%u] done",
              _tpServiceName.c_str(), _tpPID ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPAGENT__TESTTPNODE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPAGENT__CHECKMETADATA, "_tpAgent::_checkMetaData" )
   INT32 _tpAgent::_checkMetaData( const CHAR *shmKey )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPAGENT__CHECKMETADATA ) ;

      SDB_ASSERT( NULL != shmKey, "shared memory key is invalid" ) ;

      BOOLEAN needAttach = FALSE ;

      ossScopedRWLock( &_metaMutex, SHARED ) ;

      needAttach = ( !_available ) ;

      if ( _available && 0 != ossStrcmp( shmKey, _buffer.getKeyString() ) )
      {
         _available = FALSE ;
         _releaseSHMBuffer() ;
         needAttach = TRUE ;
      }

      if ( needAttach )
      {
         rc = _attachSHMBuffer( shmKey ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to attach meta data, rc: %d",
                      rc ) ;
      }

      if ( NULL != getMetaData() )
      {
         _syncInterval = getMetaData()->getSyncInterval() ;
      }

      _available = TRUE ;

   done:
      PD_TRACE_EXITRC( SDB__TPAGENT__CHECKMETADATA, rc ) ;
      return rc ;

   error:
      _available = FALSE ;
      _releaseSHMBuffer() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPAGENT__RELEASEMETADATA, "_tpAgent::_releaseMetaData" )
   INT32 _tpAgent::_releaseMetaData()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPAGENT__RELEASEMETADATA ) ;

      ossScopedRWLock lock( &_metaMutex, EXCLUSIVE ) ;

      rc = _releaseSHMBuffer() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to release meta data, rc: %d", rc ) ;

   done:
      _available = FALSE ;
      PD_TRACE_EXITRC( SDB__TPAGENT__RELEASEMETADATA, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPAGENT__GETLOGICALTIMENS, "_tpAgent::_getLogicalTimeNS" )
   INT32 _tpAgent::_getLogicalTimeNS( tpLogicalTimeNS &time )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPAGENT__GETLOGICALTIMENS ) ;

      ossScopedRWLock lock( &_metaMutex, SHARED ) ;

      PD_CHECK( _available, SDB_TP_NOT_AVAILABLE, error, PDERROR,
                "Failed to get logical time, TP is not available" ) ;
      PD_CHECK( NULL != getMetaData(), SDB_TP_NOT_AVAILABLE, error, PDERROR,
                "Failed to get logical time, meta data is not available" ) ;

      rc = getMetaData()->getLogicalTimeNS( time, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get logical time, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPAGENT__GETLOGICALTIMENS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPAGENT__GETLOGICALTIMEUS, "_tpAgent::_getLogicalTimeUS" )
   INT32 _tpAgent::_getLogicalTimeUS( tpLogicalTimeUS &time )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPAGENT__GETLOGICALTIMEUS ) ;

      ossScopedRWLock lock( &_metaMutex, SHARED ) ;

      PD_CHECK( _available, SDB_TP_NOT_AVAILABLE, error, PDERROR,
                "Failed to get logical time, TP is not available" ) ;
      PD_CHECK( NULL != getMetaData(), SDB_TP_NOT_AVAILABLE, error, PDERROR,
                "Failed to get logical time, meta data is not available" ) ;

      rc = getMetaData()->getLogicalTimeUS( time, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get logical time, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPAGENT__GETLOGICALTIMEUS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPAGENT__ATTACHSHMBUFFER, "_tpAgent::_attachSHMBuffer" )
   INT32 _tpAgent::_attachSHMBuffer( const CHAR *shmKey )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPAGENT__ATTACHSHMBUFFER ) ;

      SDB_ASSERT( NULL != shmKey, "shared memory key is invalid" ) ;

      tpMetaData *metaData = NULL ;

      _releaseSHMBuffer() ;

      PD_CHECK( _buffer.attach( shmKey, tpMetaData::getBufferSize() ),
                SDB_SYS, error, PDERROR, "Failed to attache shared memory "
                "buffer [key: %s, size: %d]", shmKey,
                tpMetaData::getBufferSize() ) ;
      PD_CHECK( NULL != _buffer.getBuffer(), SDB_OOM, error, PDERROR,
                "Failed to attach shared memory buffer [key: %s, size: %d], "
                "it is empty", shmKey, tpMetaData::getBufferSize() ) ;

      PD_LOG( PDEVENT, "Attach shared memory [key: %s, size: %d, id: %llu]",
              _buffer.getKeyString(), _buffer.getSize(), _buffer.getID() ) ;

      metaData = tpMetaData::getBuffer( _buffer.getBuffer() ) ;
      PD_CHECK( NULL != metaData, SDB_SYS, error, PDERROR,
                "Failed to attach meta data" ) ;

      PD_CHECK( _metaVersion >= metaData->getVersion(), SDB_SYS, error, PDERROR,
                "Failed to get meta data, TP version [%u] is higher than "
                "local [%u]", metaData->getVersion(), _metaVersion ) ;

      setMetaData( metaData ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPAGENT__ATTACHSHMBUFFER, rc ) ;
      return rc ;

   error:
      _releaseSHMBuffer() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPAGENT__RELEASESHMBUFFER, "_tpAgent::_releaseSHMBuffer" )
   INT32 _tpAgent::_releaseSHMBuffer()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPAGENT__RELEASESHMBUFFER ) ;

      setMetaData( NULL ) ;
      _buffer.release() ;

      PD_TRACE_EXITRC( SDB__TPAGENT__RELEASESHMBUFFER, rc ) ;

      return rc ;
   }

   /*
      _tpAgentCheckJob implement
    */
   _tpAgentCheckJob::_tpAgentCheckJob( tpAgent *agent )
   : _agent( agent )
   {
   }

   _tpAgentCheckJob::~_tpAgentCheckJob()
   {
   }

   INT32 _tpAgentCheckJob::doit()
   {
      INT32 rc = SDB_OK ;

      pmdEDUMgr *eduMgr = pmdGetKRCB()->getEDUMgr() ;
      pmdEDUCB *cb = eduCB() ;

      while ( !PMD_IS_DB_DOWN() &&
              !cb->isForced() )
      {
         rc = _agent->checkAvailable() ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDWARNING, "Failed to check TP available, "
                    "rc: %d", rc ) ;
         }
#if defined (_DEBUG)
         else
         {
            tpLogicalTimeUS time ;
            rc = _agent->getLogicalTimeUS( time ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDWARNING, "Failed to get logical time, rc: %d", rc ) ;
            }
            else
            {
               PD_LOG( PDDEBUG, "Got logical time [%llu], time error [%u]",
                       time.getTimestamp(), time.getTimeError() ) ;
            }
         }
#endif

         eduMgr->waitEDU( cb ) ;
         _agent->waitCheckEvent() ;
         eduMgr->activateEDU( cb ) ;

         rc = SDB_OK ;
      }

      PD_LOG( PDDEBUG, "%s: end job", name() ) ;

      return rc ;
   }

   /*
      helper functions
    */
   INT32 tpAgentStartCheckJob( tpAgent *agent, EDUID *eduID )
   {
      INT32 rc = SDB_OK ;

      tpAgentCheckJob *job = SDB_OSS_NEW tpAgentCheckJob( agent ) ;
      PD_CHECK( NULL != job, SDB_OOM, error, PDERROR,
                "Failed to create TP agent check job" ) ;

      rc = rtnGetJobMgr()->startJob( job, RTN_JOB_MUTEX_STOP_CONT, eduID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to start TP agent check job, rc: %d",
                   rc ) ;

   done:
      return rc ;

   error:
      goto done ;
   }

}
