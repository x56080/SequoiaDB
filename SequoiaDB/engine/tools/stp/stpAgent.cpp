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
      _stpAgent implement
    */
   _stpAgent::_stpAgent()
   : stpMetaReader(),
     _stpPID( OSS_INVALID_PID ),
     _syncInterval( STP_DEF_SYNC_INTERVAL ),
     _available( FALSE )
   {
   }

   _stpAgent::~_stpAgent()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPAGENT_ACTIVE, "_stpAgent::active" )
   INT32 _stpAgent::active( BOOLEAN mustAvailable )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPAGENT_ACTIVE ) ;

      // check available of STP
      rc = checkAvailable() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDWARNING, "Failed to check STP available, rc: %d", rc ) ;
      }

      if ( !mustAvailable )
      {
         // ignore errors, later could re-check again
         rc = SDB_OK ;
      }

      PD_TRACE_EXITRC( SDB__STPAGENT_ACTIVE, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPAGENT_DEACTIVE, "_stpAgent::deactive" )
   INT32 _stpAgent::deactive()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPAGENT_DEACTIVE ) ;

      // clear agent
      _clear() ;

      PD_TRACE_EXITRC( SDB__STPAGENT_DEACTIVE, rc ) ;

      return rc ;

   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPAGENT_GETLOGICALTIMENS, "_stpAgent::getLogicalTimeNS" )
   INT32 _stpAgent::getLogicalTimeNS( stpLogicalTimeNS &time )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPAGENT_GETLOGICALTIMENS ) ;

      // get logical time
      rc = _getLogicalTimeNS( time ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get logical time, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPAGENT_GETLOGICALTIMENS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPAGENT_GETLOGICALTIMEUS, "_stpAgent::getLogicalTimeUS" )
   INT32 _stpAgent::getLogicalTimeUS( stpLogicalTimeUS &time )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPAGENT_GETLOGICALTIMEUS ) ;

      // get logical time
      rc = _getLogicalTimeUS( time ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get logical time, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPAGENT_GETLOGICALTIMEUS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPAGENT_CHECKAVAILABLE, "_stpAgent::checkAvailable" )
   INT32 _stpAgent::checkAvailable()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPAGENT_CHECKAVAILABLE ) ;

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
      rc = _checkMetaData( _stpServiceName.c_str() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check meta data with key [%s], "
                   "rc: %d", _stpServiceName.c_str(), rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPAGENT_CHECKAVAILABLE, rc ) ;
      return rc ;

   error:
      // when error happened, clear agent
      _clear() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPAGENT__CLEAR, "_stpAgent::_clear" )
   void _stpAgent::_clear()
   {
      PD_TRACE_ENTRY( SDB__STPAGENT__CLEAR ) ;

      // reset PID of STP
      _stpPID = OSS_INVALID_PID ;
      // reset service name of STP
      _stpServiceName.clear() ;
      // release meta data
      _releaseMetaData() ;

      PD_TRACE_EXIT( SDB__STPAGENT__CLEAR ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPAGENT__GETSTP, "_stpAgent::_getSTP" )
   INT32 _stpAgent::_getSTP()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPAGENT__GETSTP ) ;

      utilNodeInfo node ;
      UTIL_VEC_NODES listNodes ;

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
      _stpServiceName.assign( node._svcname.c_str() ) ;
      // extract PID
      _stpPID = node._pid ;

   done:
      PD_TRACE_EXITRC( SDB__STPAGENT__GETSTP, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPAGENT__TESTSTP, "_stpAgent::_testSTP" )
   INT32 _stpAgent::_testSTP()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPAGENT__TESTSTP ) ;

      INT8 test = 0 ;

      // write test command to pipe
      rc = utilWriteReadPipe( STP_PIPE_SERVICE_NAME, _stpPID,
                              STP_PIPE_MSG_TEST,
                              sizeof( STP_PIPE_MSG_TEST ),
                              (CHAR *)( &test ), sizeof( test ), FALSE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to test from STP "
                   "node [%s] pid [%u], rc: %d", _stpServiceName.c_str(),
                   _stpPID, rc ) ;

      PD_LOG( PDINFO, "Test STP node [%s] pid [%u] done",
              _stpServiceName.c_str(), _stpPID ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPAGENT__TESTSTP, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPAGENT__CHECKMETADATA, "_stpAgent::_checkMetaData" )
   INT32 _stpAgent::_checkMetaData( const CHAR *shmKey )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPAGENT__CHECKMETADATA ) ;

      SDB_ASSERT( NULL != shmKey, "shared memory key is invalid" ) ;

      BOOLEAN needAttach = FALSE ;

      ossScopedRWLock( &_metaMutex, SHARED ) ;

      // if it is not available, means the buffer is not attached
      needAttach = ( !_available ) ;

      // if available but key of shared buffer is changed, release the old one
      if ( _available && 0 != ossStrcmp( shmKey, _buffer.getKeyString() ) )
      {
         // reset available
         _available = FALSE ;
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

      if ( NULL != getMetaData() )
      {
         // extract synchronize interval from meta data
         _syncInterval = getMetaData()->getSyncInterval() ;
      }

      // set available
      _available = TRUE ;

   done:
      PD_TRACE_EXITRC( SDB__STPAGENT__CHECKMETADATA, rc ) ;
      return rc ;

   error:
      // when error happened, release shared memory buffer
      _available = FALSE ;
      _releaseSHMBuffer() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPAGENT__RELEASEMETADATA, "_stpAgent::_releaseMetaData" )
   INT32 _stpAgent::_releaseMetaData()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPAGENT__RELEASEMETADATA ) ;

      ossScopedRWLock lock( &_metaMutex, EXCLUSIVE ) ;

      // release shared memory buffer
      rc = _releaseSHMBuffer() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to release meta data, rc: %d", rc ) ;

   done:
      // reset available
      _available = FALSE ;
      PD_TRACE_EXITRC( SDB__STPAGENT__RELEASEMETADATA, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPAGENT__GETLOGICALTIMENS, "_stpAgent::_getLogicalTimeNS" )
   INT32 _stpAgent::_getLogicalTimeNS( stpLogicalTimeNS &time )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPAGENT__GETLOGICALTIMENS ) ;

      ossScopedRWLock lock( &_metaMutex, SHARED ) ;

      // check available
      PD_CHECK( _available, STP_NOT_AVAILABLE, error, PDERROR,
                "Failed to get logical time, STP is not available" ) ;
      // check meta data
      PD_CHECK( NULL != getMetaData(), STP_NOT_AVAILABLE, error, PDERROR,
                "Failed to get logical time, meta data is not available" ) ;

      // get logical time from meta data
      rc = getMetaData()->getLogicalTimeNS( time, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get logical time, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPAGENT__GETLOGICALTIMENS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPAGENT__GETLOGICALTIMEUS, "_stpAgent::_getLogicalTimeUS" )
   INT32 _stpAgent::_getLogicalTimeUS( stpLogicalTimeUS &time )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPAGENT__GETLOGICALTIMEUS ) ;

      ossScopedRWLock lock( &_metaMutex, SHARED ) ;

      // check available
      PD_CHECK( _available, STP_NOT_AVAILABLE, error, PDERROR,
                "Failed to get logical time, STP is not available" ) ;
      // check meta data
      PD_CHECK( NULL != getMetaData(), STP_NOT_AVAILABLE, error, PDERROR,
                "Failed to get logical time, meta data is not available" ) ;

      // get logical time from meta data
      rc = getMetaData()->getLogicalTimeUS( time, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get logical time, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPAGENT__GETLOGICALTIMEUS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPAGENT__ATTACHSHMBUFFER, "_stpAgent::_attachSHMBuffer" )
   INT32 _stpAgent::_attachSHMBuffer( const CHAR *shmKey )
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPAGENT__RELEASESHMBUFFER, "_stpAgent::_releaseSHMBuffer" )
   INT32 _stpAgent::_releaseSHMBuffer()
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

}
