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

   Source File Name = monClass.cpp

   Descriptive Name = monitor Class source

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains functions for OSS operations.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/24/2019  CW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "monClass.hpp"
#include "monCB.hpp"
#include "pmd.hpp"
#include "pmdEnv.hpp"

namespace engine
{

// ms
#define MON_ARCHIVE_KEEPTIME_WHEN_OFF  ( 300000 )

archiveFunc monClassArchiveFP[MON_CLASS_MAX] = {
   monArchiveQuery,  // MON_CLASS_QUERY
   monArchiveLatch,  // MON_CLASS_LATCH
   monArchiveLock    // MON_CLASS_LOCK
};

MON_DATA_LEVEL monClassCreateCB[MON_CLASS_MAX] = {
   MON_DATA_LVL_BASIC,   // MON_CLASS_QUERY
   MON_DATA_LVL_DETAIL,  // MON_CLASS_LATCH
   MON_DATA_LVL_DETAIL   // MON_CLASS_LOCK
};

UINT32 monGetTID()
{
   pmdEDUCB *cb = pmdGetThreadEDUCB() ;
   if ( cb )
   {
      return cb->getTID() ;
   }

   return ossGetCurrentThreadID() ;
}

/**
 * _monClass constructor
 */
_monClass::_monClass( const ossTick *pStartTick )
   : _status(MON_CLASS_STATUS_NORMAL),
     _type(MON_CLASS_MAX),
     dataLvl( MON_DATA_LVL_NONE ),
     _pPendingDelete( NULL )
{
   if ( pStartTick && (BOOLEAN)( *pStartTick ) )
   {
      _createTSTick = *pStartTick ;
   }
   else
   {
      _createTSTick.sample() ;
   }
}

_monClass::~_monClass() {}

_monClassQueryTmpData& _monClassQueryTmpData::operator=(const _monAppCB& cb)
{
   dataRead      = cb.totalDataRead ;
   dataWrite     = cb.totalDataWrite ;
   indexRead     = cb.totalIndexRead ;
   indexWrite    = cb.totalIndexWrite ;
   lobRead       = cb.totalLobRead ;
   lobWrite      = cb.totalLobWrite ;
   lobTruncate   = cb.totalLobTruncate ;
   lobAddressing = cb.totalLobAddressing ;

   return *this ;
}

void _monClassQueryTmpData::diff(_monAppCB &cb)
{
   dataRead      = MON_APP_DELTA( dataRead , cb.totalDataRead ) ;
   dataWrite     = MON_APP_DELTA( dataWrite , cb.totalDataWrite ) ;
   indexRead     = MON_APP_DELTA( indexRead , cb.totalIndexRead ) ;
   indexWrite    = MON_APP_DELTA( indexWrite , cb.totalIndexWrite ) ;
   lobRead       = MON_APP_DELTA( lobRead , cb.totalLobRead ) ;
   lobWrite      = MON_APP_DELTA( lobWrite , cb.totalLobWrite ) ;
   lobTruncate   = MON_APP_DELTA( lobTruncate , cb.totalLobTruncate ) ;
   lobAddressing = MON_APP_DELTA( lobAddressing , cb.totalLobAddressing ) ;
}

_monClassContainer::_monClassContainer ( MON_CLASS_TYPE type )
   : _archivedListMaxLen( pmdGetKRCB()->getOptionCB()->monHistEvent() ),
     _numPendingArchive( 0 ),
     _numPendingDelete( 0 ),
     _curCollectionLvl( MON_DATA_LVL_NONE ),
     _classType( type )
{
   _doArchive = monClassArchiveFP[(INT32)type] ;
   _minOperationalLvl = monClassCreateCB[(INT32)type] ;
   _lastNonOperationTick = 0 ;
   _earliestTime = ossGetCurrentMilliseconds() ;
}

BOOLEAN _monClassContainer::_hasExpired( UINT64 curTime )
{
   if ( curTime < _earliestTime )
   {
      _earliestTime = curTime ;
   }

   if ( ( curTime - _earliestTime ) / 60000 >= monGetHistExpiredTime() )
   {
      return TRUE ;
   }
   return FALSE ;
}

/**
 * Async remove an object from the active list. The
 * object is either marked as pending archive or pending delete
 * depending on the container's archive rule.
 *
 * @param obj object to be removed.
 */
void _monClassContainer::remove ( monClass *obj )
{
   if ( MON_CLASS_STATUS_NORMAL == obj->getStatus() )
   {
      ossGetCurrentTime( obj->getEndTS() ) ;
      obj->_pPendingDelete = NULL ;

      if ( (*_doArchive)( obj ) )
      {
         obj->setPendingArchive() ;
         _numPendingArchive.inc() ;
      }
      else
      {
         obj->setPendingDelete() ;
         _numPendingDelete.inc() ;
      }
   }
}

/**
 * Remove archived obj back to capacity
 * The archived list is structured as a MRU list with the tail being most
 * recent used. So elements are removed from the head which are the oldest
 */
void _monClassContainer::_removeArchivedObj( UINT64 curTime )
{
   BOOLEAN checkExpired = _hasExpired( curTime ) ;
   SINT32 numToDelete = 0 ;

   UINT32 archivedListSize = _archivedList.size() ;

   if ( !isOperational() )
   {
      checkExpired = FALSE ;

      if ( 0 == _lastNonOperationTick )
      {
         _lastNonOperationTick = pmdGetDBTick() ;
      }

      if ( pmdGetTickSpanTime( _lastNonOperationTick ) >= MON_ARCHIVE_KEEPTIME_WHEN_OFF )
      {
         numToDelete = archivedListSize ;
      }
      else
      {
         numToDelete = archivedListSize - _archivedListMaxLen ;
      }
   }
   else
   {
      _lastNonOperationTick = 0 ;
      numToDelete = archivedListSize - _archivedListMaxLen ;
   }

   if ( numToDelete > 0 || checkExpired )
   {
      UINT64 tmpEaliestTime = _earliestTime ;
      UINT64 endTime = 0 ;

      getArchiveLatch( EXCLUSIVE ) ;
      MONCLASS_LIST::iterator it = _archivedList.begin() ;

      while ( ( numToDelete > 0 || checkExpired ) && it != _archivedList.end() )
      {
         monClass &obj = *it ;

         endTime = obj.getEndTime() ;
         if ( endTime < tmpEaliestTime )
         {
            endTime = tmpEaliestTime ;
         }
         _earliestTime = endTime ;

         if ( numToDelete <= 0 && checkExpired )
         {
            UINT32 timeSpan = 0 ; /// min

            if ( curTime > endTime )
            {
               timeSpan = ( curTime - endTime ) / 60000 ;
            }
            else if ( endTime >= curTime + MON_ARCHIVE_KEEPTIME_WHEN_OFF )
            {
               timeSpan = ( endTime - curTime ) / 60000 ;
            }

            if ( timeSpan < monGetHistExpiredTime() )
            {
               checkExpired = FALSE ;
               break ;
            }
         }

         it = _archivedList.erase(it) ;

         SDB_OSS_DEL &obj ;

         numToDelete-- ;
      }
      releaseArchiveLatch( EXCLUSIVE ) ;
   }
}

/**
 * Process pending delete or pending archive object
 */
void _monClassContainer::_processPendingObj()
{
   getArchiveLatch( EXCLUSIVE ) ;
   MON_PARTITION_LIST::iterator it = _activeList.begin() ;

   while ( it != _activeList.end() )
   {
      if ( TRUE == it->isPendingArchive() )
      {
         monClass &obj = *it ;
         it = _activeList.erase(it) ;
         _numPendingArchive.dec() ;

         try
         {
            _archivedList.push_back(obj) ;
         }
         catch( std::exception &e )
         {
            PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
            SDB_OSS_DEL &obj ;
         }
      }
      else if ( TRUE == it->isPendingDelete() )
      {
         monClass &monClass = *it ;
         it = _activeList.erase(it) ;
         SDB_OSS_DEL &monClass ;
         _numPendingDelete.dec() ;
      }
      else
      {
         it++ ;
      }
   }
   releaseArchiveLatch( EXCLUSIVE ) ;
}

/*
 * return whether this container is empty for the correspond list
 * on the listType.
 * @param listType the type of list to read
 */
BOOLEAN _monClassContainer::isEmpty(MON_CLASS_LIST_TYPE listType)
{
   return ( listType == MON_CLASS_ACTIVE_LIST ) ?
          !this->getActiveListLen() : !this->getArchivedListLen() ;
}

MON_DATA_LEVEL monGroupMaskToLevle( UINT32 groupMask, MON_CLASS_TYPE classType )
{
   MON_DATA_LEVEL monLevel = MON_DATA_LVL_NONE ;

   switch ( classType )
   {
      case MON_CLASS_QUERY :
         if ( groupMask & MON_GROUP_QUERY_DETAIL )
         {
            monLevel = MON_DATA_LVL_DETAIL ;
         }
         else if ( groupMask & MON_GROUP_QUERY_BASIC )
         {
            monLevel = MON_DATA_LVL_BASIC ;
         }
         break ;
      case MON_CLASS_LATCH :
         if ( groupMask & MON_GROUP_LATCH_DETAIL )
         {
            monLevel = MON_DATA_LVL_DETAIL ;
         }
         else if ( groupMask & MON_GROUP_LATCH_BASIC )
         {
            monLevel = MON_DATA_LVL_BASIC ;
         }
         break ;
      case MON_CLASS_LOCK :
         if ( groupMask & MON_GROUP_LOCK_DETAIL )
         {
            monLevel = MON_DATA_LVL_DETAIL ;
         }
         else if ( groupMask & MON_GROUP_LOCK_BASIC )
         {
            monLevel = MON_DATA_LVL_BASIC ;
         }
         break ;
      default :
         break ;
   }

   return monLevel ;
}

static UINT32& _monGetCurGroupMask()
{
   static OSS_THREAD_LOCAL UINT32 _curGroupMask = 0 ;
   return _curGroupMask ;
}

static UINT32& _monGetGroupMask()
{
   static UINT32 s_groupMask = 0 ;
   return s_groupMask ;
}

static UINT32& _monGetSlowQueryThreshold()
{
   static UINT32 s_lowQuery = 0 ;
   return s_lowQuery ;
}

static UINT32& _monGetSlowLatchThreshold()
{
   static UINT32 s_lowLatch = 0 ;
   return s_lowLatch ;
}

static UINT32& _monGetSlowLockThreshold()
{
   static UINT32 s_lowLock = 0 ;
   return s_lowLock ;
}

static UINT32& _monGetSlowSyncThreshold()
{
   static UINT32 s_slowSync = 0 ;
   return s_slowSync ;
}

static UINT32& _monGetOptiLevel()
{
   static UINT32 s_optiLevel = 0 ;
   return s_optiLevel ;
}

static UINT32& _monGetHistExpiredTime()
{
   static UINT32 s_histExpiredTime = 0 ;
   return s_histExpiredTime ;
}

UINT32 monGetGroupMask() { return _monGetGroupMask() ; }
void   monUpdateGroupMask( UINT32 groupMask ) { _monGetGroupMask() = groupMask ; }

UINT32 monGetCurGroupMask() { return _monGetCurGroupMask() ; }
void   monUpdateCurGroupMask( UINT32 groupMask ) { _monGetCurGroupMask() = groupMask ; }

UINT32 monGetOptiLevel() { return _monGetOptiLevel() ; }
UINT32 monGetSlowLatchThreshold() { return _monGetSlowLatchThreshold() ; }
UINT32 monGetSlowLockThreshold() { return _monGetSlowLockThreshold() ; }
UINT32 monGetSlowQueryThreshold() { return _monGetSlowQueryThreshold() ; }
UINT32 monGetSlowSyncThreshold() { return _monGetSlowSyncThreshold() ; }
UINT32 monGetHistExpiredTime() { return _monGetHistExpiredTime() ; }

void   monUpdateConf( UINT32 queryThreshold,
                      UINT32 latchThreshold,
                      UINT32 lockThreshold,
                      UINT32 syncThreshold,
                      UINT32 optiLevel,
                      UINT32 histExpiredTime )
{
   _monGetSlowQueryThreshold() = queryThreshold ;
   _monGetSlowLatchThreshold() = latchThreshold ;
   _monGetSlowLockThreshold()  = lockThreshold ;
   _monGetSlowSyncThreshold()  = syncThreshold ;
   _monGetOptiLevel()          = optiLevel ;
   _monGetHistExpiredTime()    = histExpiredTime ;
}

/**
 * archive query based on response time
 */
BOOLEAN monArchiveQuery ( monClass *obj )
{
   if ( monNeedArchiveQuery( obj ) )
   {
      pmdEDUCB *cb = pmdGetThreadEDUCB() ;
      if ( cb )
      {
         DMS_MON_OP_COUNT_INC( cb->getMonAppCB(), MON_GENERAL_SLOW_QUERY, 1 ) ;
      }

      return TRUE ;
   }
   else
   {
      return FALSE ;
   }
}

BOOLEAN monNeedArchiveQuery( _monClass * obj)
{
   monClassQuery *monQuery = (monClassQuery*)obj ;
   // in ms
   UINT64 responseTime = monQuery->responseTime.toUINT64() / 1000 ;

   if ( ( monQuery->isCommand() &&
          responseTime >= pmdGetKRCB()->getOptionCB()->slowCmdThreshold() ) ||
        ( !monQuery->isCommand() &&
          responseTime >= monGetSlowQueryThreshold() ) )
   {
      return TRUE ;
   }
   else
   {
      return FALSE ;
   }
}

/**
 * archive latch when there is latch wait
 */
BOOLEAN monArchiveLatch ( monClass *obj )
{
   monClassLatch *monLatch = (monClassLatch*)obj ;
   // in ms
   UINT64 waitTime = monLatch->waitTime.toUINT64() / 1000 ;

   if ( waitTime >= monGetSlowLatchThreshold() )
   {
      return TRUE ;
   }
   else
   {
      return FALSE ;
   }
}

/**
 * archive lock when there is lock wait
 */
BOOLEAN monArchiveLock ( monClass *obj )
{
   monClassLock *monLock = (monClassLock*)obj ;
   // in ms
   UINT64 waitTime = monLock->waitTime.toUINT64() / 1000 ;

   if ( waitTime >= monGetSlowLockThreshold() )
   {
      return TRUE ;
   }
   else
   {
      return FALSE ;
   }
}

BOOLEAN monNoArchive ( monClass *obj )
{
   return FALSE ;
}

} // namespace engine
