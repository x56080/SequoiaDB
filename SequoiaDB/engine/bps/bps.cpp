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

   Source File Name = pmdPrefetcher.cpp

   Descriptive Name = Process MoDel Prefetcher

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main entry point for prefetcher

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/01/2013  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "core.hpp"
#include "bps.hpp"
#include "pmdEDUMgr.hpp"
#include "pmd.hpp"
#include "rtnPrefetchJob.hpp"

namespace engine
{

   INT32 _bpsCB::init ()
   {
      // 1. init param
      if ( pmdGetKRCB()->isRestore() )
      {
         _numPreLoad = 0 ;
         _maxPrefPool = 0 ;
      }
      else
      {
         _numPreLoad = pmdGetOptionCB()->numPreLoaders() ;
         _maxPrefPool = pmdGetOptionCB()->maxPrefPool() ;
      }

      return SDB_OK ;
   }

   INT32 _bpsCB::active ()
   {
      INT32 rc = SDB_OK ;
      UINT32 curPrefAgentNum = 0 ;
      for ( INT32 i = 0; i < _numPreLoad; ++i )
      {
         rc = _addPreLoader () ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to add prefetcher, rc = %d", rc ) ;
            goto error ;
         }
      }

      // start data prefetch edu
      while ( curPrefAgentNum < _maxPrefPool &&
              curPrefAgentNum < pmdGetKRCB()->getOptionCB()->maxSubQuery() )
      {
         rc = startPrefetchJob( NULL, -1 ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to start data prefetch job edu,"
                      "rc: %d", rc ) ;
         ++curPrefAgentNum ;
         _idlePrefAgentNum.inc() ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _bpsCB::deactive ()
   {
      return SDB_OK ;
   }

   INT32 _bpsCB::fini ()
   {
      bpsPreLoadReq *prefRequest = NULL ;
      while ( _dropBackQueue.try_pop ( prefRequest ) )
      {
         SDB_OSS_DEL ( prefRequest ) ;
      }
      while ( _requestQueue.try_pop ( prefRequest ) )
      {
         SDB_OSS_DEL ( prefRequest ) ;
      }

      _preLoaderList.clear() ;

      return SDB_OK ;
   }

   void _bpsCB::onConfigChange ()
   {
      if ( !pmdGetKRCB()->isRestore() )
      {
         _numPreLoad = pmdGetOptionCB()->numPreLoaders() ;
         _maxPrefPool = pmdGetOptionCB()->maxPrefPool() ;
      }
   }

   INT32 _bpsCB::_addPreLoader ()
   {
      INT32 rc = SDB_OK ;
      EDUID preLoaderEDU = 0 ;
      pmdKRCB *krcb = pmdGetKRCB () ;
      pmdEDUMgr *eduMgr = krcb->getEDUMgr () ;
      bpsPreLoadReq *request = NULL ;

      // create a new pre-load request
      request = SDB_OSS_NEW bpsPreLoadReq ( DMS_INVALID_CS, ~0, DMS_INVALID_EXTENT ) ;
      if ( !request )
      {
         PD_LOG ( PDERROR, "Failed to allocate memory for prefetch request" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      try
      {
         _dropBackQueue.push ( request ) ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception when push request: %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

      request = NULL ;

      rc = eduMgr->startEDU ( EDU_TYPE_PREFETCHER, NULL, &preLoaderEDU ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Start preload thread failed, rc: %d", rc ) ;
         goto error ;
      }

      try
      {
         _preLoaderList.push_back ( preLoaderEDU ) ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception when push edu: %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done :
      if ( request )
      {
         SDB_OSS_DEL request ;
         request = NULL ;
      }
      return rc ;
   error :
      goto done ;
   }

   INT32 _bpsCB::sendPreLoadRequest ( const bpsPreLoadReq &request )
   {
      INT32 rc = SDB_OK ;
      _bpsPreLoadReq *req = NULL ;

      // get an empty request token
      if ( !_dropBackQueue.try_pop ( req ) || !req )
      {
         rc = SDB_BUSY_PREFETCHER ;
         goto error ;
      }

      *req = request ;

      try
      {
         _requestQueue.push ( req ) ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDWARNING, "Occur exception when push request: %s", e.what() ) ;
         SDB_OSS_DEL req ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _bpsCB::sendPrefechReq( const bpsDataPref & request,
                                 BOOLEAN inPref )
   {
      INT32 rc = SDB_OK ;

      if ( 0 == _maxPrefPool )
      {
         rc = SDB_SYS ;
         goto error ;
      }

      if ( FALSE == inPref && _idlePrefAgentNum.peek() <= 1 &&
           _dataPrefetchQueue.size() > 0 &&
           _curPrefAgentNum.peek() < _maxPrefPool )
      {
         if ( SDB_OK == startPrefetchJob( NULL, 10000 ) )
         {
            _idlePrefAgentNum.inc() ;
         }
      }

      // push to queque
      try
      {
         _dataPrefetchQueue.push( request ) ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception when push prefetch request: %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      get global bps cb
   */
   SDB_BPSCB* sdbGetBPSCB()
   {
      static SDB_BPSCB s_bpsCB ;
      return &s_bpsCB ;
   }

}

