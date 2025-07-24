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

   Source File Name = pmdMemPool.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          27/11/2012  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
#include "pmdMemPool.hpp"
#include "ossMem.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "pmdTrace.hpp"

namespace engine
{
   /*
      Global define
   */
   const UINT32 PMD_MEM_ALIGMENT_SIZE  = 1024 ;

   #define PMD_CACHE_JOB_INTERVAL            ( 100 )     /// ms
   #define PMD_MIN_CACHE_JOB                 ( 2 )
   #define PMD_CACHE_JOB_TIMEOUT             ( 300 * OSS_ONE_SEC )

   /*
      _pmdBuffPool implement
   */
   _pmdBuffPool::_pmdBuffPool()
   {
      _curAgent = 0 ;
      _idleAgent = 0 ;
      _maxCacheJob = PMD_MIN_CACHE_JOB ;
      _startCtrlJob = FALSE ;
   }

   _pmdBuffPool::~_pmdBuffPool()
   {
   }

   void _pmdBuffPool::setMaxCacheJob( UINT32 maxCacheJob )
   {
      _maxCacheJob = maxCacheJob ;
      if ( _maxCacheJob < PMD_MIN_CACHE_JOB )
      {
         _maxCacheJob = PMD_MIN_CACHE_JOB ;
      }
   }

   void _pmdBuffPool::exitJob( BOOLEAN isControl )
   {
      _unitLatch.get() ;

      --_curAgent ;
      --_idleAgent ;
      if ( isControl )
      {
         _startCtrlJob = FALSE ;
      }
      _checkAndStartJob( FALSE ) ;

      _unitLatch.release() ;
   }

   INT32 _pmdBuffPool::init( UINT64 cacheSize )
   {
      INT32 rc = _utilCacheMgr::init( cacheSize ) ;
      if ( rc )
      {
         goto error ;
      }
      _checkAndStartJob( TRUE ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   void _pmdBuffPool::fini()
   {
      _utilCacheMgr::fini() ;

      SDB_ASSERT( 0 == _unitList.size(), "List size must be 0" ) ;
      SDB_ASSERT( 0 == _curAgent && 0 == _idleAgent,
                  "Agent must be 0" ) ;
   }

   void _pmdBuffPool::_checkAndStartJob( BOOLEAN needLock )
   {
      if ( needLock )
      {
         _unitLatch.get() ;
      }
      if ( maxCacheSize() > 0 && FALSE == _startCtrlJob )
      {
         if ( SDB_OK == pmdStartCacheJob( NULL, this, -1 ) )
         {
            ++_curAgent ;
            ++_idleAgent ;
            _startCtrlJob = TRUE ;
         }
      }
      if ( needLock )
      {
         _unitLatch.release() ;
      }
   }

   void _pmdBuffPool::registerUnit( _utilCacheUnit *pUnit )
   {
      ossScopedLock lock( &_unitLatch ) ;

      _unitList.push_back( pUnit ) ;

      _checkAndStartJob( FALSE ) ;
   }

   void _pmdBuffPool::pushBackUnit( _utilCacheUnit *pUnit )
   {
      ossScopedLock lock( &_unitLatch ) ;

      if ( !pUnit->isClosed() )
      {
         _unitList.push_back( pUnit ) ;
      }
      /// release the page cleaner
      pUnit->unlockPageCleaner() ;
      /// inc idla agent
      ++_idleAgent ;
   }

   _utilCacheUnit* _pmdBuffPool::dispatchUnit()
   {
      _utilCacheUnit *pUnit = NULL ;
      LIST_UNIT::iterator it ;
      BOOLEAN force = FALSE ;

      ossScopedLock lock( &_unitLatch ) ;

      if ( _unitList.empty() )
      {
         return NULL ;
      }

      it = _unitList.begin() ;
      while( it != _unitList.end() )
      {
         pUnit = *it ;

         if ( pUnit->canSync( force ) || pUnit->canRecycle( force ) )
         {
            _unitList.erase( it ) ;
            /// lock the unit
            pUnit->lockPageCleaner() ;
            /// dec idle agent
            --_idleAgent ;
            break ; 
         }
         else
         {
            pUnit = NULL ;
         }
         ++it ;
      }

      return pUnit ;
   }

   void _pmdBuffPool::unregUnit( _utilCacheUnit *pUnit )
   {
      LIST_UNIT::iterator it ;

      ossScopedLock lock( &_unitLatch ) ;

      it = _unitList.begin() ;
      while( it != _unitList.end() )
      {
         if ( (*it) == pUnit )
         {
            _unitList.erase( it ) ;
            break ;
         }
         ++it ;
      }
   }

   void _pmdBuffPool::checkLoad()
   {
      LIST_UNIT::iterator it ;
      UINT32 readyNum = 0 ;
      BOOLEAN force = FALSE ;

      ossScopedLock lock( &_unitLatch ) ;

      it = _unitList.begin() ;
      while( it != _unitList.end() )
      {
         utilCacheUnit *pUnit = *it ;
         ++it ;
         if ( pUnit->canSync( force ) || pUnit->canRecycle( force ) )
         {
            ++readyNum ;
         }
      }

      if ( readyNum > 0 )
      {
         /// wake up the agent
         _wakeUpEvent.signalAll() ;

         /// start agent
         while ( _curAgent < PMD_MIN_CACHE_JOB ||
                 ( readyNum / 2 > _idleAgent &&
                   _curAgent < _maxCacheJob ) )
         {
            if ( SDB_OK == pmdStartCacheJob( NULL, this,
                                             PMD_CACHE_JOB_TIMEOUT ) )
            {
               ++_curAgent ;
               ++_idleAgent ;
            }
            else
            {
               break ;
            }
         }
      }
   }

   /*
      _pmdCacheJob implement
   */
   _pmdCacheJob::_pmdCacheJob( pmdBuffPool *pBuffPool, INT32 timeout )
   {
      _pBuffPool = pBuffPool ;
      _timeout = timeout ;
   }

   _pmdCacheJob::~_pmdCacheJob()
   {
   }

   RTN_JOB_TYPE _pmdCacheJob::type() const
   {
      return PMD_JOB_CACHE ;
   }

   const CHAR* _pmdCacheJob::name() const
   {
      return "CACHE-JOB" ;
   }

   BOOLEAN _pmdCacheJob::isControlJob() const
   {
      return _timeout < 0 ? TRUE : FALSE ;
   }

   BOOLEAN _pmdCacheJob::muteXOn( const _rtnBaseJob *pOther )
   {
      return FALSE ;
   }

   INT32 _pmdCacheJob::doit()
   {
      pmdEDUMgr *pEDUMgr = eduCB()->getEDUMgr() ;
      utilCacheUnit *pUnit = NULL ;
      UINT32 timeout = 0 ;

      pEDUMgr->activateEDU( eduCB() ) ;

      while( !eduCB()->isForced() )
      {
         if ( isControlJob() )
         {
            _pBuffPool->checkLoad() ;

            if ( _pBuffPool->canRecycle() )
            {
               _pBuffPool->recycleBlocks() ;
            }
            else
            {
               ossSleep( PMD_CACHE_JOB_INTERVAL ) ;
            }
         }
         else
         {
            pEDUMgr->waitEDU( eduCB() ) ;

            /// sync unit
            pUnit = _pBuffPool->dispatchUnit() ;
            if ( pUnit )
            {
               timeout = 0 ;
               _doUnit( pUnit ) ;
               /// push back
               _pBuffPool->pushBackUnit( pUnit ) ;
            }
            else
            {
               _pBuffPool->getEvent()->reset() ;
               if ( SDB_TIMEOUT ==
                    _pBuffPool->getEvent()->wait( OSS_ONE_SEC * 5 ) )
               {
                  timeout += ( 5 * OSS_ONE_SEC ) ;
               }
               else
               {
                  timeout += 100 ;
               }

               if ( timeout >= (UINT32)_timeout )
               {
                  /// over _timeout millsecs, donothing, qiut the job
                  break ;
               }
            }
         }
      }

      _pBuffPool->exitJob( isControlJob() ) ;

      return SDB_OK ;
   }

   void _pmdCacheJob::_doUnit( _utilCacheUnit *pUnit )
   {
      pmdEDUMgr *pEDUMgr = eduCB()->getEDUMgr() ;
      BOOLEAN force = FALSE ;

      pEDUMgr->activateEDU( eduCB() ) ;

      if ( pUnit->canSync( force ) )
      {
         pUnit->syncPages( eduCB(), force ) ;
      }
      if ( pUnit->canRecycle( force ) )
      {
         pUnit->recyclePages( force ) ;
      }

      pEDUMgr->waitEDU( eduCB() ) ;
   }

   INT32 pmdStartCacheJob( EDUID *pEDUID, pmdBuffPool *pBuffPool,
                           INT32 timeout )
   {
      INT32 rc = SDB_OK ;
      pmdCacheJob *pJob = NULL ;

      pJob = SDB_OSS_NEW pmdCacheJob( pBuffPool, timeout ) ;
      if ( !pJob )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Alloc cache job failed" ) ;
         goto error ;
      }
      rc = rtnGetJobMgr()->startJob( pJob, RTN_JOB_MUTEX_NONE, pEDUID  ) ;
      /// neither failed or succeed, the pJob will release in job manager

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _pmdMemPool implement
   */
   _pmdMemPool::_pmdMemPool()
   :_totalMemSize( 0 )
   {
   }

   _pmdMemPool::~_pmdMemPool()
   {
   }

   INT32 _pmdMemPool::initialize()
   {
      return SDB_OK ;
   }

   INT32 _pmdMemPool::final()
   {
      if ( 0 != totalSize() )
      {
         PD_LOG( PDERROR, "MemPool has memory leak: %llu", totalSize() ) ;
      }
      return SDB_OK ;
   }

   void _pmdMemPool::clear ()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDMEMPOL_ALLOC, "_pmdMemPool::alloc" )
   CHAR *_pmdMemPool::alloc( UINT32 size, UINT32 &assignSize )
   {
      PD_TRACE_ENTRY ( SDB__PMDMEMPOL_ALLOC );
      size = ossRoundUpToMultipleX( size, PMD_MEM_ALIGMENT_SIZE ) ;
      CHAR *pBuffer = (CHAR *)SDB_OSS_MALLOC ( size ) ;
      if ( pBuffer )
      {
         assignSize = size ;
         _totalMemSize.add( assignSize ) ;
      }
      else
      {
         assignSize = 0 ;
         PD_LOG ( PDERROR, "Failed to allocate memory" ) ;
      }

      PD_TRACE_EXIT ( SDB__PMDMEMPOL_ALLOC );
      return pBuffer ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDMEMPOL_RELEASE, "_pmdMemPool::release" )
   void _pmdMemPool::release( CHAR* pBuff, UINT32 size )
   {
      PD_TRACE_ENTRY ( SDB__PMDMEMPOL_RELEASE );
      if ( pBuff && size > 0 )
      {
         SDB_OSS_FREE ( pBuff ) ;
         _totalMemSize.sub( size ) ;
      }
      PD_TRACE_EXIT ( SDB__PMDMEMPOL_RELEASE );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDMEMPOL_REALLOC, "_pmdMemPool::realloc" )
   CHAR *_pmdMemPool::realloc ( CHAR* pBuff, UINT32 srcSize,
                                UINT32 needSize, UINT32 &assignSize )
   {
      PD_TRACE_ENTRY ( SDB__PMDMEMPOL_REALLOC );
      CHAR *p = NULL ;
      if ( srcSize >= needSize )
      {
         assignSize = srcSize ;
         p = pBuff ;
         goto done ;
      }

      release ( pBuff, srcSize );

      p = alloc ( needSize, assignSize ) ;
   done :
      PD_TRACE_EXIT ( SDB__PMDMEMPOL_REALLOC );
      return p ;
   }
}

