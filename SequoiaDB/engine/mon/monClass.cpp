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

namespace engine
{

// microsecond
#define MON_LATCH_ARCHIVE_THRESHOLD 1000

// microsecond
#define MON_LOCK_ARCHIVE_THRESHOLD 1000

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

/**
 * _monClass constructor
 */
_monClass::_monClass()
   : _status(MON_CLASS_STATUS_NORMAL),
     _type(MON_CLASS_MAX),
     dataLvl( MON_DATA_LVL_NONE )
{
   _createTSTick.sample() ;
}

_monClass::~_monClass() {}

_monClassQueryTmpData& _monClassQueryTmpData::operator=(const _monAppCB& cb)
{
   dataRead  = cb.totalDataRead ;
   dataWrite = cb.totalDataWrite ;
   indexRead = cb.totalIndexRead ;
   indexWrite = cb.totalIndexWrite ;
   return *this ;
}

void _monClassQueryTmpData::diff(_monAppCB &cb)
{
   dataRead  = cb.totalDataRead - dataRead ;
   dataWrite = cb.totalDataWrite - dataWrite ;
   indexRead = cb.totalIndexRead - indexRead ;
   indexWrite = cb.totalIndexWrite - indexWrite ;
}

_monClassContainer::_monClassContainer ( MON_CLASS_TYPE type )
   : _activeListLen( 0 ),
     _archivedListLen( 0 ),
     _archivedListMaxLen( pmdGetKRCB()->getOptionCB()->monHistEvent() ),
     _numPendingArchive( 0 ),
     _numPendingDelete( 0 ),
     _curCollectionLvl( MON_DATA_LVL_NONE )
{
   _doArchive = monClassArchiveFP[(INT32)type] ;
   _minOperationalLvl = monClassCreateCB[(INT32)type] ;
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
   ossGetCurrentTime( obj->getEndTS() ) ;

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

/**
 * Remove archived obj back to capacity
 * The archived list is structured as a MRU list with the tail being most
 * recent used. So elements are removed from the head which are the oldest
 */
void _monClassContainer::_removeArchivedObj()
{
   getArchiveLatch( EXCLUSIVE ) ;
   SINT32 numToDelete = 0 ;

   UINT32 archivedListSize = _archivedList.size() ;

   if ( !isOperational() )
   {
      numToDelete = archivedListSize ;
   }
   else
   {
      numToDelete = archivedListSize - _archivedListMaxLen ;
   }

   if ( numToDelete > 0 )
   {
     MONCLASS_LIST::iterator it = _archivedList.begin() ;

      while ( numToDelete > 0 )
      {
         monClass &monClass = *it ;

         it = _archivedList.erase(it) ;

         SDB_OSS_DEL &monClass ;

         numToDelete-- ;
      }
   }
   releaseArchiveLatch( EXCLUSIVE ) ;
}

/**
 * Process pending delete or pending archive object
 */
void _monClassContainer::_processPendingObj()
{
   getListLatch( EXCLUSIVE ) ;
   getArchiveLatch( EXCLUSIVE ) ;
   getHeadLatch( EXCLUSIVE ) ;

   BOOLEAN headLatchHeld = TRUE ;
   MONCLASS_LIST::iterator it = _activeList.begin() ;

   while ( it != _activeList.end() )
   {
      if ( TRUE == it->isPendingArchive() )
      {
         monClass &obj = *it ;
         it = _activeList.erase(it) ;
         _activeListLen.dec() ;
         _numPendingArchive.dec() ;
         _archivedList.push_back(obj) ;
      }
      else if ( TRUE == it->isPendingDelete() )
      {
         monClass &monClass = *it ;
         it = _activeList.erase(it) ;
         _activeListLen.dec() ;
         SDB_OSS_DEL &monClass ;
         _numPendingDelete.dec() ;
      }
      else
      {
         it++ ;
      }
   }

   //TODO: fix this
   if (headLatchHeld)
   {
      releaseHeadLatch( EXCLUSIVE ) ;
   }
   releaseArchiveLatch( EXCLUSIVE ) ;
   releaseListLatch( EXCLUSIVE ) ;
}

/**
 * archive query based on response time
 */
BOOLEAN monArchiveQuery ( monClass *obj )
{
   monClassQuery *monQuery = (monClassQuery*)obj ;
   // in microsecond
   UINT64 responseTime = monQuery->responseTime.toUINT64() ;

   if ((responseTime/1000) >= pmdGetKRCB()->getOptionCB()->slowQueryThreshold())
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
   // in microsecond
   UINT64 waitTime = monLatch->waitTime.toUINT64() ;;

   if ( waitTime > MON_LATCH_ARCHIVE_THRESHOLD )
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
   // in microsecond
   UINT64 waitTime = monLock->waitTime.toUINT64() ;

   if ( waitTime > MON_LOCK_ARCHIVE_THRESHOLD )
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

// monClassScanner implements

void _monClassReadScanner::initScan()
{
   _container->getListLatch( SHARED ) ;
   _hasListLatch = TRUE ;

   if ( _listType == MON_CLASS_ARCHIVED_LIST )
   {
      _container->getArchiveLatch( SHARED ) ;
      _hasArchiveLatch = TRUE ;
   }

   _container->getHeadLatch( SHARED ) ;
   _hasHeadLatch = TRUE ;

   _currentList = MON_CLASS_ACTIVE_LIST ;

   // If reading active list, or (if reading archive list and
   // there is pending archive in the active list)
   if ( ( _listType == MON_CLASS_ACTIVE_LIST &&
          _container->getActiveListLen() > 0 ) ||
        _container->getNumPendingArchive() > 0 )
   {
      _itr = _container->begin( _currentList ) ;
   }
   else
   {
      if ( _listType == MON_CLASS_ARCHIVED_LIST &&
           _container->getArchivedListLen() > 0 )
      {
         _currentList = MON_CLASS_ARCHIVED_LIST ;
         _itr = _container->begin( _currentList ) ;
      }
      else
      {
         _endReached = TRUE ;
      }
   }
   _initCalled = TRUE ;
}

monClass* _monClassReadScanner::getNext()
{
   monClass *ret = NULL ;
   BOOLEAN found = FALSE ;
   if ( !_initCalled )
   {
      initScan() ;
   }

   // If we have not reached the end of the lists
   while (!_endReached && !found)
   {
      // 1. We skip any pending deletes
      // 2. Skip if this is a pending archive and we are only interested in
      //    the active list
      // 3. Skip if this is active and we are only interested in the
      //    archived list
      if ( _itr->isPendingDelete() ||
           (_itr->isPendingArchive() && _listType == MON_CLASS_ACTIVE_LIST ) ||
           (!_itr->isPendingArchive() && _listType == MON_CLASS_ARCHIVED_LIST ) )
      {
         _itr++ ;
      }
      else
      {
         // Found a match, return
         ret = &(*_itr) ;
         _itr++ ;
         found = TRUE ;
      }

      // Reached the end of the current list, move to the next list
      // if there is one
      if ( _itr == _container->end(_currentList) )
      {
         if ( _currentList == MON_CLASS_ACTIVE_LIST &&
              _listType == MON_CLASS_ARCHIVED_LIST &&
              _container->getArchivedListLen() > 0 )
         {
            _itr = _container->begin( MON_CLASS_ARCHIVED_LIST ) ;
            _currentList = MON_CLASS_ARCHIVED_LIST ;
         }
         else
         {
            _endReached = TRUE ;
         }
      }
   }

   // The headLatch is required when reading the first node of the active list.
   // By the time we get here, it is guaranteed that we have already read the
   // first node. So we can now release the headLatch.
   if ( _hasHeadLatch )
   {
      _container->releaseHeadLatch(SHARED) ;
      _hasHeadLatch = FALSE ;
   }

   return ret ;
}

void _monClassReadScanner::doneScan()
{
   if (_hasListLatch)
   {
      _container->releaseListLatch(SHARED) ;
   }
   if (_hasHeadLatch )
   {
      _container->releaseHeadLatch(SHARED) ;
   }
   if ( _hasArchiveLatch)
   {
      _container->releaseArchiveLatch(SHARED) ;
   }
}

} // namespace engine
