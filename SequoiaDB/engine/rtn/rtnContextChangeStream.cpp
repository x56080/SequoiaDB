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

   Source File Name = rtnContextChangeStream.cpp

   Descriptive Name = RunTime Change Stream Context Source

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for Runtime
   Context.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/01/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#include "rtnContextChangeStream.hpp"
#include "monStreamMonitorManager.hpp"
#include "pmd.hpp"
#include "rtn.hpp"
#include "rtnCB.hpp"
#include "rtnStreamSource.hpp"
#include "rtnTrace.hpp"
#include "ossMemPool.hpp"

namespace engine
{

   /*
      _rtnContextChangeStreamFetcher implement
    */
   RTN_CTX_AUTO_REGISTER( _rtnContextChangeStream,
                          RTN_CONTEXT_CHANGE_STREAM,
                          "CHANGESTREAM" )
   _rtnContextChangeStream::_rtnContextChangeStream( INT64 contextID, UINT64 eduID )
   : _rtnContextStreamBase( UTIL_CHANGE_STREAM, contextID, eduID, _source ),
     _options(),
     _source( *this, _options )
   {
   }

   _rtnContextChangeStream::~_rtnContextChangeStream()
   {
      _close() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCTXCHANGESTREAM_OPEN, "_rtnContextChangeStream::open" )
   INT32 _rtnContextChangeStream::open( const bson::BSONObj &boOptions )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNCTXCHANGESTREAM_OPEN ) ;

      rtnChangeStreamNotifier *notifier = sdbGetRTNCB()->getChangeStreamNotifier() ;
      SDB_ASSERT( NULL != notifier, "change stream notifier is invalid") ;
      monStreamMonitorManager *monMgr = pmdGetKRCB()->getMonStreamMgr() ;
      SDB_ASSERT( NULL != monMgr, "stream monitor manager is invalid") ;

      // parse options
      rc = _options.fromBSON( boOptions ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse options [%s], rc: %d",
                   boOptions.toPoolString().c_str(), rc ) ;

      // initialize change stream source
      rc = _source.init( notifier->getResumableWindow(), TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to initialize change stream source, "
                   "rc: %d", rc ) ;

      // initialize monitor
      rc = _monitor.init( _options.getOptions() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to initialize monitor, rc: %d", rc ) ;

      // register source as watcher
      rc = notifier->registerWatcher( _source ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to register watcher, rc: %d", rc ) ;

      // register monitor
      rc = monMgr->registerMonitor( _monitor ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to register monitor, rc: %d", rc ) ;

      PD_LOG( PDEVENT, "Initialized change stream [%s]",
              _options.getOptions().toPoolString().c_str() ) ;


      _isOpened = TRUE ;
      _hitEnd = FALSE ;

   done:
      PD_TRACE_EXITRC( SDB_RTNCTXCHANGESTREAM_OPEN, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCTXCHANGESTREAM__CLOSE, "_rtnContextChangeStream::_close" )
   void _rtnContextChangeStream::_close()
   {
      PD_TRACE_ENTRY( SDB_RTNCTXCHANGESTREAM__CLOSE ) ;

      rtnChangeStreamNotifier *notifier = sdbGetRTNCB()->getChangeStreamNotifier() ;
      SDB_ASSERT( NULL != notifier, "change stream notifier is invalid") ;
      monStreamMonitorManager *monMgr = pmdGetKRCB()->getMonStreamMgr() ;
      SDB_ASSERT( NULL != monMgr, "stream monitor manager is invalid") ;

      // unregister source
      _source.fini() ;
      if ( _source.isAttached() )
      {
         notifier->unregisterWatcher( _source ) ;
      }

      // unregister monitor
      monMgr->unregisterMonitor( _monitor ) ;

      PD_LOG( PDEVENT, "Closed change stream [%s]: "
              "batch [%llu], return records [%llu], return size [%llu], "
              "scanned [%llu] log records, received [%llu] log records, "
              "hit cache [%llu], miss cache [%llu], "
              "return code [%d]",
              _options.getOptions().toPoolString().c_str(),
              _monitor._batchNum, _monitor._returnNum, _monitor._returnSize,
              _source.getScannedNum(), _source.getReceivedNum(),
              _source.getHitCacheNum(), _source.getMissCacheNum(),
              _source.getWatchRC() ) ;

      PD_TRACE_EXIT( SDB_RTNCTXCHANGESTREAM__CLOSE ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCTXCHANGESTREAM__PREPAREDATA, "_rtnContextChangeStream::_prepareData" )
   INT32 _rtnContextChangeStream::_prepareData( pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNCTXCHANGESTREAM__PREPAREDATA ) ;

      rc = _source.getRecords( (INT64)( _options.getMaxWaitTimeMS() ) ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get records, rc: %d", rc ) ;

      _monitor.onBatch( numRecords(), buffSize() ) ;

      if ( _source.isStopped() )
      {
         _hitEnd = TRUE ;
      }

   done:
      PD_TRACE_EXITRC( SDB_RTNCTXCHANGESTREAM__PREPAREDATA, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
