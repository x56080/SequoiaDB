/*******************************************************************************

   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = utilCache.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/04/2016  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#include "utilCache.hpp"
#include "ossUtil.hpp"
#include "pd.hpp"

namespace engine
{

   #define UTIL_MAX_EXCEED_SLOT_SIZE            ( 3 )
   #define UTIL_MIN_EXCEED_SLOT_SIZE            ( 5 )

   /*
      _utilCachePage implement
   */
   _utilCachePage::_utilCachePage()
   {
      _start = 0 ;
      _length = 0 ;
      _dirtyStart = 0 ;
      _dirtyLength = 0 ;
      _lastTime = 0 ;
      _lastWriteTime = 0 ;
      _readTimes = 0 ;
      _writeTimes = 0 ;
      _status = 0 ;
      _beginLSN = ~0 ;
      _endLSN = ~0 ;
      _lsnNum = 0 ;
   }

   _utilCachePage::_utilCachePage( const _utilCachePage& right )
   {
      _start = right._start ;
      _length = right._length ;
      _dirtyStart = right._dirtyStart ;
      _dirtyLength = right._dirtyLength ;
      _lastTime = right._lastTime ;
      _lastWriteTime = right._lastWriteTime ;
      _readTimes = right._readTimes ;
      _writeTimes = right._writeTimes ;
      _status = right._status ;
      _beginLSN = right._beginLSN ;
      _endLSN = right._endLSN ;
      _lsnNum = right._lsnNum ;

      _first._size = 0 ;
      _first._pBuff = NULL ;

      UINT32 pos = right.beginBlock() ;
      CHAR *pPage = NULL ;
      UINT32 pageSize = 0 ;

      while( NULL != ( pPage = right.nextBlock( pageSize, pos ) ) )
      {
         addPage( pPage, pageSize ) ;
      }
   }

   _utilCachePage::~_utilCachePage()
   {
      clear() ;
   }

   void _utilCachePage::clearDataInfo()
   {
      _start = 0 ;
      _length = 0 ;
      _dirtyStart = 0 ;
      _dirtyLength = 0 ;

      _lastTime = 0 ;
      _lastWriteTime = 0 ;
      _readTimes = 0 ;
      _writeTimes = 0 ;

      clearLSNInfo() ;
   }

   void _utilCachePage::clearLSNInfo()
   {
      _beginLSN = ~0 ;
      _endLSN = ~0 ;
      _lsnNum = 0 ;
   }

   BOOLEAN _utilCachePage::isDataEmpty() const
   {
      return 0 == _length ? TRUE : FALSE ;
   }

   void _utilCachePage::clear()
   {
      _next.clear() ;
      _start = 0 ;
      _length = 0 ;
      _dirtyStart = 0 ;
      _dirtyLength = 0 ;
      _first._size = 0 ;
      _first._pBuff = NULL ;
      _lastTime = 0 ;
      _lastWriteTime = 0 ;
      _readTimes = 0 ;
      _writeTimes = 0 ;
      _status = 0 ;
      _beginLSN = ~0 ;
      _endLSN = ~0 ;
      _lsnNum = 0 ;
   }

   UINT32 _utilCachePage::size() const
   {
      UINT32 totalSize = 0 ;
      UINT32 blockSize = 0 ;
      UINT32 pos = beginBlock() ;
      while( NULL != nextBlock( blockSize, pos ) )
      {
         totalSize += blockSize ;
      }
      return totalSize ;
   }

   _utilCachePage& _utilCachePage::operator= ( const _utilCachePage& rhs )
   {
      clear() ;

      _start = rhs._start ;
      _length = rhs._length ;
      _dirtyStart = rhs._dirtyStart ;
      _dirtyLength = rhs._dirtyLength ;
      _lastTime = rhs._lastTime ;
      _lastWriteTime = rhs._lastWriteTime ;
      _readTimes = rhs._readTimes ;
      _writeTimes = rhs._writeTimes ;
      _status = rhs._status ;
      _beginLSN = rhs._beginLSN ;
      _endLSN = rhs._endLSN ;
      _lsnNum = rhs._lsnNum ;

      UINT32 pos = rhs.beginBlock() ;
      CHAR *pPage = NULL ;
      UINT32 pageSize = 0 ;

      while( NULL != ( pPage = rhs.nextBlock( pageSize, pos ) ) )
      {
         addPage( pPage, pageSize ) ;
      }
      return *this ;
   }

   UINT32 _utilCachePage::blockNum() const
   {
      if ( _first.empty() )
      {
         return 0 ;
      }
      return 1 + _next.size() ;
   }

   INT32 _utilCachePage::_write( const CHAR *pBuf, UINT32 offset,
                                 UINT32 len, BOOLEAN dirty )
   {
      INT32 rc = SDB_OK ;
      ossTimestamp t ;
      UINT32 pos = 0 ;
      CHAR *pPage = NULL ;
      UINT32 blockSize = 0 ;
      UINT32 lastOffset = offset ;
      UINT32 lastLen = len ;
      UINT32 onceWrite = 0 ;

      if ( size() < offset + len )
      {
         /// no space for write data
         rc = SDB_NOSPC ;
         goto error ;
      }

      pos = beginBlock() ;
      while ( lastLen > 0 && NULL != ( pPage = nextBlock( blockSize, pos ) ) )
      {
         if ( blockSize <= lastOffset )
         {
            lastOffset -= blockSize ;
            continue ;
         }
         else if ( lastOffset > 0 )
         {
            pPage += lastOffset ;
            blockSize -= lastOffset ;
            lastOffset = 0 ;
         }
         onceWrite = lastLen < blockSize ? lastLen : blockSize ;
         ossMemcpy( pPage, pBuf, onceWrite ) ;
         lastLen -= onceWrite ;
         pBuf += onceWrite ;
      }

      SDB_ASSERT( lastLen == 0, "Last len must be 0" ) ;

      /// update meta
      ossGetCurrentTime( t ) ;
      _lastTime = t.time * 1000 + t.microtm / 1000 ;
      if ( dirty )
      {
         ++_writeTimes ;
         _lastWriteTime = _lastTime ;

         if ( 0 == _dirtyLength || offset < _dirtyStart )
         {
            _dirtyStart = offset ;
         }
         if ( offset + len > _dirtyLength )
         {
            _dirtyLength = offset + len ;
         }
      }

      if ( 0 == _length || offset < _start )
      {
         _start = offset ;
      }
      if ( offset + len > _length )
      {
         _length = offset + len ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilCachePage::write( const CHAR *pBuf, UINT32 offset,
                                UINT32 len, BOOLEAN &setDirty )
   {
      setDirty = FALSE ;
      INT32 rc = _write( pBuf, offset, len, TRUE ) ;
      if ( SDB_OK == rc && !isDirty() )
      {
         makeDirty() ;
         setDirty = TRUE ;
      }
      return rc ;      
   }

   INT32 _utilCachePage::load( const CHAR *pBuf, UINT32 offset, UINT32 len )
   {
      return _write( pBuf, offset, len, FALSE ) ;
   }

   INT32 _utilCachePage::loadWithoutData( UINT32 offset, UINT32 len )
   {
      INT32 rc = SDB_OK ;
      ossTimestamp t ;

      SDB_ASSERT( isNewest(), "Page should be newest" ) ;

      if ( size() < offset + len )
      {
         /// no space for write data
         rc = SDB_NOSPC ;
         goto error ;
      }

      /// update meta
      ossGetCurrentTime( t ) ;
      _lastTime = t.time * 1000 + t.microtm / 1000 ;

      if ( (UINT32)~0 == _length || offset < _start )
      {
         _start = offset ;
      }
      if ( offset + len > _length )
      {
         _length = offset + len ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   UINT32 _utilCachePage::read( CHAR *pBuf, UINT32 offset, UINT32 len )
   {
      ossTimestamp t ;
      UINT32 hasRead = 0 ;
      CHAR *pPage = NULL ;
      UINT32 blockSize = 0 ;
      UINT32 onceRead = 0 ;
      UINT32 lastOffset = offset ;
      UINT32 lastRead = len ;
      UINT32 pos = 0 ;

      pos = beginBlock() ;
      while ( lastRead > 0 && NULL != ( pPage = nextBlock( blockSize, pos ) ) )
      {
         if ( blockSize <= lastOffset )
         {
            lastOffset -= blockSize ;
            continue ;
         }
         else if ( lastOffset > 0 )
         {
            pPage += lastOffset ;
            blockSize -= lastOffset ;
            lastOffset = 0 ;
         }
         onceRead = lastRead < blockSize ? lastRead : blockSize ;
         ossMemcpy( pBuf + hasRead, pPage, onceRead ) ;
         lastRead -= onceRead ;
         hasRead += onceRead ;
      }

      /// update meta
      ++_readTimes ;
      ossGetCurrentTime( t ) ;
      _lastTime = t.time * 1000 + t.microtm / 1000 ;

      return hasRead ;
   }

   INT32 _utilCachePage::copy( const _utilCachePage &right )
   {
      INT32 rc = SDB_OK ;
      CHAR *pData = NULL ;
      UINT32 blockSz = 0 ;
      UINT32 offset = 0 ;

      UINT32 rightPos = right.beginBlock() ;
      while( NULL != ( pData = right.nextBlock( blockSz, rightPos ) ) )
      {
         rc = load( pData, offset, blockSz ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Copy data failed, rc: %d", rc ) ;
            goto error ;
         }
         offset += blockSz ;
      }

      /// other value
      _start = right._start ;
      _length = right._length ;
      _dirtyStart = right._dirtyStart ;
      _dirtyLength = right._dirtyLength ;
      _lastTime = right._lastTime ;
      _lastWriteTime = right._lastWriteTime ;
      _readTimes = right._readTimes ;
      _writeTimes = right._writeTimes ;
      _status = right._status ;
      _beginLSN = right._beginLSN ;
      _endLSN = right._endLSN ;
      _lsnNum = right._lsnNum ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilCachePage::addPage( CHAR *pPage, UINT32 size )
   {
      INT32 rc = SDB_OK ;

      if ( !pPage || 0 == size )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( _first.empty() )
      {
         _first._pBuff = pPage ;
         _first._size = size ;
      }
      else
      {
         _next.push_back( utilCacheBlock( pPage, size ) ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   const CHAR* _utilCachePage::str() const
   {
      SDB_ASSERT( 1 == blockNum(), "Block number must be 1" ) ;
      return _first._pBuff ;
   }

   CHAR* _utilCachePage::str()
   {
      SDB_ASSERT( 1 == blockNum(), "Block number must be 1" ) ;
      return _first._pBuff ;
   }

   UINT32 _utilCachePage::beginBlock() const
   {
      return 0 ;
   }

   CHAR* _utilCachePage::nextBlock( UINT32 &size, UINT32 &pos ) const
   {
      UINT32 tmpPos = pos ;
      ++pos ;

      if ( 0 == tmpPos )
      {
         if ( _first.empty() )
         {
            return NULL ;
         }
         size = _first._size ;
         return _first._pBuff ;
      }
      else if ( _next.size() >= tmpPos )
      {
         const utilCacheBlock& item = _next[ tmpPos - 1 ] ;
         size = item._size ;
         return item._pBuff ;
      }

      return NULL ;
   }

   void _utilCachePage::addLSN( UINT64 lsn )
   {
      if ( (UINT64)~0 == _beginLSN )
      {
         _beginLSN = lsn ;
         _endLSN = lsn ;
      }
      else
      {
         _endLSN = lsn ;
      }
      ++_lsnNum ;
   }

   /*
      _utilCacheMgr implement
   */
   _utilCacheMgr::_utilCacheMgr()
   :_freeSize( 0 ), _totalSize( 0 ), _totalUseTimes( 0 ), _nonEmptySlotNum( 0 )
   {
      _beginPageSizeSqrt = 0 ;
      _maxCacheSize = 0 ;
      _pStat = NULL ;
      _lastRecycleTime = 0 ;
   }

   _utilCacheMgr::~_utilCacheMgr()
   {
   }

   INT32 _utilCacheMgr::init( UINT64 cacheSize )
   {
      INT32 rc = SDB_OK ;
      blkLatch* pLatch = NULL ;
      _maxCacheSize = cacheSize * 1024 * 1024 ;

      if ( !ossIsPowerOf2( UTIL_PAGE_SLOT_BEGIN_SIZE, &_beginPageSizeSqrt ) )
      {
         rc = SDB_SYS ;
         goto error ;
      }

      for ( UINT32 i = 0 ; i < UTIL_PAGE_SLOT_SIZE ; ++i )
      {
         pLatch = SDB_OSS_NEW blkLatch() ;
         if ( pLatch )
         {
            _latch.push_back( pLatch ) ;
         }
         else
         {
            PD_LOG( PDERROR, "Failed to alloc bucket latch" ) ;
            rc = SDB_OOM ;
            goto error ;
         }
      }

      _pStat = new (std::nothrow) vector< utilCacheStat >( UTIL_PAGE_SLOT_SIZE ) ;
      if ( !_pStat )
      {
         PD_LOG( PDERROR, "Failed to alloc stat vector" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      for ( UINT32 i = 0 ; i < UTIL_PAGE_SLOT_SIZE ; ++i )
      {
         _getBucketCache( i )._pageSize = _slot2PageSize( i ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _utilCacheMgr::fini()
   {
      SDB_ASSERT( _totalSize.peek() == _freeSize.peek(),
                  "Total size must be equal with the free size" ) ;

      UINT32 size = 0 ;
      for ( UINT32 i = 0 ; i < UTIL_PAGE_SLOT_SIZE ; ++i )
      {
         size = _slot[ i ].size() ;
         for ( UINT32 j = 0 ; j < size ; ++j )
         {
            SDB_OSS_FREE( _slot[ i ][ j ] ) ;
         }
         _slot[ i ].clear() ;
      }

      for ( UINT32 i = 0 ; i < _latch.size() ; ++i )
      {
         SDB_OSS_DEL _latch[ i ] ;
      }

      if ( _pStat )
      {
         delete _pStat ;
         _pStat = NULL ;
      }
      _latch.clear() ;
   }

   void _utilCacheMgr::resetEvent()
   {
      _releaseEvent.reset() ;
   }

   INT32 _utilCacheMgr::waitEvent( INT64 millisec )
   {
      return _releaseEvent.wait( millisec ) ;
   }

   void _utilCacheMgr::getCacheStat( UINT32 bucketID,
                                     utilCacheStat &stat ) const
   {
      if ( _pStat && bucketID < UTIL_PAGE_SLOT_SIZE )
      {
         _latch[ bucketID ]->get() ;
         stat = (*_pStat)[ bucketID ] ;
         _latch[ bucketID ]->release() ;
      }
      else
      {
         stat.reset() ;
      }
   }

   INT32 _utilCacheMgr::alloc( UINT32 size, utilCachePage &item )
   {
      INT32 rc = SDB_OK ;
      UINT32 beginSlot = 0 ;
      UINT32 pageSize = 0 ;
      UINT32 extendNum = 0 ;
      UINT32 lastSize = 0 ;
      UINT32 exceedSlot = 0 ;
      blkLatch *pLatch = NULL ;

      if ( item.size() >= size )
      {
         return rc ;
      }
      lastSize = size - item.size() ;

   retry:
      beginSlot = _sizeUp2Slot( lastSize ) ;
      exceedSlot = 0 ;

      /// first to alloc a whole page
      while( beginSlot < UTIL_PAGE_SLOT_SIZE &&
             exceedSlot < UTIL_MAX_EXCEED_SLOT_SIZE &&
             _freeSize.peek() >= lastSize )
      {
         pLatch = _latch[ beginSlot ] ;
         pLatch->get() ;
         vector< CHAR* > &slotItem = _slot[ beginSlot ] ;
         if ( !slotItem.empty() )
         {
            pageSize = _slot2PageSize( beginSlot ) ;
            item.addPage( slotItem.back(), pageSize ) ;
            slotItem.pop_back() ;
            _freeSize.sub( pageSize ) ;
            /// update the bucket stat
            _getBucketCache( beginSlot )._freeSize -= pageSize ;
            _getBucketCache( beginSlot )._useTimes += 1 ;
            /// release the bucket latch
            pLatch->release() ;
            _totalUseTimes.inc() ;
            goto done ;
         }
         pLatch->release() ;
         ++beginSlot ;
         ++exceedSlot ;
      }

      beginSlot = _sizeUp2Slot( lastSize ) ;
      exceedSlot = 0 ;
      /// then not alloc, but freeSize is more than lastSize, merge the pages
      if ( beginSlot >= UTIL_PAGE_SLOT_SIZE )
      {
         beginSlot = UTIL_PAGE_SLOT_SIZE - 1 ;
      }
      while ( beginSlot > 0 &&
              exceedSlot < UTIL_MIN_EXCEED_SLOT_SIZE &&
              _freeSize.peek() >= lastSize )
      {
         --beginSlot ;
         ++exceedSlot ;
         pageSize = _slot2PageSize( beginSlot ) ;
         pLatch = _latch[ beginSlot ] ;
         pLatch->get() ;
         vector< CHAR* > &slotItem = _slot[ beginSlot ] ;
         while( !slotItem.empty() )
         {
            item.addPage( slotItem.back(), pageSize ) ;
            slotItem.pop_back() ;
            _freeSize.sub( pageSize ) ;

            /// update the bucket stat
            _getBucketCache( beginSlot )._freeSize -= pageSize ;
            _getBucketCache( beginSlot )._useTimes += 1 ;
            _totalUseTimes.inc() ;

            if ( lastSize > pageSize )
            {
               lastSize -= pageSize ;
            }
            else
            {
               pLatch->release() ;
               goto done ;
            }
         }
         pLatch->release() ;
      }

      /// no space for the lastSize, alloc and retry
      if ( _sizeUp2Slot( lastSize ) >= UTIL_PAGE_SLOT_SIZE )
      {
         pageSize = _slot2PageSize( UTIL_PAGE_SLOT_SIZE - 1 ) ;
         extendNum = ( lastSize + pageSize - 1 ) / pageSize ;
      }
      else
      {
         pageSize = _sizeUp2PageSize( lastSize ) ;
         extendNum = 1 ;
      }
      rc = _allocMem( pageSize, extendNum ) ;
      if ( SDB_OK == rc )
      {
         goto retry ;
      }
      else
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilCacheMgr::allocWholePage( UINT32 size, utilCachePage &item,
                                        BOOLEAN keepData )
   {
      INT32 rc = SDB_OK ;
      UINT32 beginSlot = 0 ;
      UINT32 pageSize = 0 ;
      UINT32 exceedSlot = 0 ;
      utilCachePage tmpPage ;

      if ( item.size() >= size )
      {
         return rc ;
      }

   retry:
      beginSlot = _sizeUp2Slot( size ) ;
      exceedSlot = 0 ;

      /// first to alloc a whole page
      while( beginSlot < UTIL_PAGE_SLOT_SIZE &&
             exceedSlot < UTIL_MAX_EXCEED_SLOT_SIZE &&
             _freeSize.peek() >= size )
      {
         _latch[ beginSlot ]->get() ;
         vector< CHAR* > &slotItem = _slot[ beginSlot ] ;
         if ( !slotItem.empty() )
         {
            pageSize = _slot2PageSize( beginSlot ) ;
            tmpPage.addPage( slotItem.back(), pageSize ) ;
            slotItem.pop_back() ;
            _freeSize.sub( pageSize ) ;
            /// update the bucket stat
            _getBucketCache( beginSlot )._freeSize -= pageSize ;
            _getBucketCache( beginSlot )._useTimes += 1 ;
            /// release the bucket latch
            _latch[ beginSlot ]->release() ;
            _totalUseTimes.inc() ;
            goto done_assign ;
         }
         _latch[ beginSlot ]->release() ;
         ++beginSlot ;
         ++exceedSlot ;
      }

      /// no space for the size, alloc and retry
      rc = _allocMem( size ) ;
      if ( SDB_OK == rc )
      {
         goto retry ;
      }
      else
      {
         goto error ;
      }

   done_assign:
      /// copy data from source page
      if ( keepData && !item.isDataEmpty() )
      {
         rc = tmpPage.copy( item ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Copy data from source page failed, rc: %d",
                    rc ) ;
            goto error ;
         }
      }
      /// release the source page
      release( item ) ;
      item = tmpPage ;

   done:
      return rc ;
   error:
      release( tmpPage ) ;
      goto done ;
   }

   void _utilCacheMgr::release( utilCachePage &item )
   {
      UINT32 pos = 0 ;
      UINT32 slot = 0 ;
      CHAR *pPage = NULL ;
      UINT32 pageSize = 0 ;

      SDB_ASSERT( !item.isDirty(), "Page can't be dirty" ) ;
      SDB_ASSERT( !item.isLocked(), "Page can't be locked" ) ;

      pos = item.beginBlock() ;

      while( NULL != ( pPage = item.nextBlock( pageSize, pos ) ) )
      {
         slot = _sizeDown2Slot( pageSize ) ;
         SDB_ASSERT( pageSize == _slot2PageSize( slot ),
                     "PageSize is not invalid" ) ;
         _latch[ slot ]->get() ;
         _slot[ slot ].push_back( pPage ) ;
         /// update the bucket stat
         _getBucketCache( slot )._freeSize += _slot2PageSize( slot ) ;
         _latch[ slot ]->release() ;
         _freeSize.add( _slot2PageSize( slot ) ) ;
      }
      _releaseEvent.signalAll() ;

      item.clear() ;
   }

   INT32 _utilCacheMgr::alloc( UINT32 size, utilCachePage &item,
                               BOOLEAN wholePage, BOOLEAN keepData )
   {
      if ( !wholePage )
      {
         return alloc( size, item ) ;
      }
      return allocWholePage( size, item, keepData ) ;
   }

   CHAR* _utilCacheMgr::allocBlock( UINT32 size, UINT32 &blockSize )
   {
      CHAR *pBlock      = NULL ;
      UINT32 beginSlot  = 0 ;
      UINT32 pageSize   = 0 ;
      UINT32 exceedSlot = 0 ;

      blockSize = 0 ;

   retry:
      beginSlot = _sizeUp2Slot( size ) ;
      exceedSlot = 0 ;

      /// first to alloc a whole page
      while( beginSlot < UTIL_PAGE_SLOT_SIZE &&
             exceedSlot < UTIL_MAX_EXCEED_SLOT_SIZE &&
             _freeSize.peek() >= size )
      {
         _latch[ beginSlot ]->get() ;
         vector< CHAR* > &slotItem = _slot[ beginSlot ] ;
         if ( !slotItem.empty() )
         {
            pageSize = _slot2PageSize( beginSlot ) ;
            pBlock = slotItem.back() ;
            blockSize = pageSize ;
            slotItem.pop_back() ;
            _freeSize.sub( pageSize ) ;
            /// update the bucket stat
            _getBucketCache( beginSlot )._freeSize -= pageSize ;
            _getBucketCache( beginSlot )._useTimes += 1 ;
            /// release the bucket latch
            _latch[ beginSlot ]->release() ;
            _totalUseTimes.inc() ;
            goto done ;
         }
         _latch[ beginSlot ]->release() ;
         ++beginSlot ;
         ++exceedSlot ;
      }

      /// no space for the size, alloc and retry
      if ( SDB_OK == _allocMem( size ) )
      {
         goto retry ;
      }
      else
      {
         goto done ;
      }

   done:
      return pBlock ;
   }

   CHAR* _utilCacheMgr::reallocBlock( UINT32 size, CHAR *pBlock,
                                      UINT32 &blockSize )
   {
      CHAR *pTmpBlock   = NULL ;
      UINT32 tmpBlockSize = 0 ;
      UINT32 beginSlot  = 0 ;
      UINT32 pageSize   = 0 ;
      UINT32 exceedSlot = 0 ;

      if ( blockSize >= size )
      {
         goto done ;
      }

   retry:
      beginSlot = _sizeUp2Slot( size ) ;
      exceedSlot = 0 ;

      /// first to alloc a whole page
      while( beginSlot < UTIL_PAGE_SLOT_SIZE &&
             exceedSlot < UTIL_MAX_EXCEED_SLOT_SIZE &&
             _freeSize.peek() >= size )
      {
         _latch[ beginSlot ]->get() ;
         vector< CHAR* > &slotItem = _slot[ beginSlot ] ;
         if ( !slotItem.empty() )
         {
            pageSize = _slot2PageSize( beginSlot ) ;
            /// save the pBlock first
            pTmpBlock = pBlock ;
            tmpBlockSize = blockSize ;
            /// assign the new block
            pBlock = slotItem.back() ;
            blockSize = pageSize ;
            slotItem.pop_back() ;
            _freeSize.sub( pageSize ) ;
            /// update the bucket stat
            _getBucketCache( beginSlot )._freeSize -= pageSize ;
            _getBucketCache( beginSlot )._useTimes += 1 ;
            /// release the bucket latch
            _latch[ beginSlot ]->release() ;
            _totalUseTimes.inc() ;
            goto done_assign ;
         }
         _latch[ beginSlot ]->release() ;
         ++beginSlot ;
         ++exceedSlot ;
      }

      /// no space for the size, alloc and retry
      if ( SDB_OK == _allocMem( size ) )
      {
         goto retry ;
      }
      else
      {
         /// error, need to release the orignal data
         releaseBlock( pBlock, blockSize ) ;
         goto done ;
      }

   done_assign:
      if ( pTmpBlock )
      {
         /// copy the orignal data
         ossMemcpy( pBlock, pTmpBlock, tmpBlockSize ) ;
         /// release the orignal data
         releaseBlock( pTmpBlock, tmpBlockSize ) ;
      }

   done:
      return pBlock ;
   }

   void _utilCacheMgr::releaseBlock( CHAR *&pBlock, UINT32 &blockSize )
   {
      UINT32 slot = 0 ;

      if ( pBlock && blockSize > 0 )
      {
         slot = _sizeDown2Slot( blockSize ) ;
         SDB_ASSERT( blockSize == _slot2PageSize( slot ),
                     "BlockSize is not invalid" ) ;

         _latch[ slot ]->get() ;
         _slot[ slot ].push_back( pBlock ) ;
         /// update the bucket stat
         _getBucketCache( slot )._freeSize += _slot2PageSize( slot ) ;
         _latch[ slot ]->release() ;
         _freeSize.add( _slot2PageSize( slot ) ) ;

         _releaseEvent.signalAll() ;
      }

      pBlock = NULL ;
      blockSize = 0 ;
   }

   INT32 _utilCacheMgr::_allocMem( UINT32 size, UINT32 pageNum )
   {
      INT32 rc = SDB_OK ;
      UINT32 count = 0 ;
      CHAR *pPage = NULL ;
      UINT32 pageSize = 0 ;
      UINT32 slot = 0 ;
      BOOLEAN addNonEmpty = FALSE ;

      if ( 0 == size )
      {
         goto done ;
      }

      pageSize = _sizeUp2PageSize( size ) ;
      slot = _sizeUp2Slot( size ) ;

      if ( slot >= UTIL_PAGE_SLOT_SIZE )
      {
         PD_LOG( PDERROR, "Size[%u] is more than the max page size[2^%u]",
                 size, _beginPageSizeSqrt + UTIL_PAGE_SLOT_SIZE - 1 ) ;
         SDB_ASSERT( slot < UTIL_PAGE_SLOT_SIZE, "Size is invalid" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      while( count < pageNum )
      {
         if ( _totalSize.peek() + pageSize > _maxCacheSize )
         {
            /// up to the limit
            rc = SDB_OSS_UP_TO_LIMIT ;
            goto error ;
         }

         pPage = (CHAR*)SDB_OSS_MALLOC( pageSize ) ;
         if ( !pPage )
         {
            rc = SDB_OOM ;
            goto error ;
         }

         _latch[ slot ]->get() ;
         /// push to vector
         if ( !addNonEmpty && 0 == _getBucketCache( slot )._totalSize &&
              pageSize > 0 )
         {
            addNonEmpty = TRUE ;
         }
         _slot[ slot ].push_back( pPage ) ;
         _getBucketCache( slot )._totalSize += pageSize ;
         _getBucketCache( slot )._freeSize += pageSize ;
         _latch[ slot ]->release() ;
         /// update the meta
         _totalSize.add( pageSize ) ;
         _freeSize.add( pageSize ) ;
         ++count ;
      }

      if ( addNonEmpty )
      {
         _nonEmptySlotNum.inc() ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _utilCacheMgr::registerUnit( _utilCacheUnit *pUnit )
   {
   }

   void _utilCacheMgr::unregUnit( _utilCacheUnit *pUnit )
   {
   }

   BOOLEAN _utilCacheMgr::canRecycle()
   {
      if ( totalSize() <= 0 )
      {
         return FALSE ;
      }
      /// when free ratio over the threshold
      else if ( totalSize() * 100 / maxCacheSize() >= UTIL_CACHE_RATIO &&
                freeSize() * 100 / totalSize() >=
                UTIL_BLOCK_RECYCLE_FREE_RATIO )
      {
         return TRUE ;
      }
      /// when page dirty timeout
      else
      {
         ossTimestamp t ;
         ossGetCurrentTime( t ) ;
         UINT64 curTime = t.time * 1000 + t.microtm / 1000 ;

         if ( ( curTime >= _lastRecycleTime &&
                curTime - _lastRecycleTime > UTIL_BLOCK_TIMEOUT ) ||
              ( curTime < _lastRecycleTime &&
                _lastRecycleTime - curTime > UTIL_BLOCK_TIMEOUT ) )
         {
            return TRUE ;
         }
      }
      return FALSE ;
   }

   UINT64 _utilCacheMgr::recycleBlocks()
   {
      UINT64 recycleSize = 0 ;
      UINT64 tmpTimes = 0 ;
      blkLatch *pLatch = NULL ;
      
      ossTimestamp t ;
      ossGetCurrentTime( t ) ;
      _lastRecycleTime = t.time * 1000 + t.microtm / 1000 ;

      /// for every bucket, is the bucket useTime < average, recycle 1/2
      /// if free/totalSize > special ratio, recycle 1/2
      for ( UINT32 i = 0 ; i < UTIL_PAGE_SLOT_SIZE ; ++i )
      {
         vector< CHAR* > &slotItem = _slot[ i ] ;
         pLatch = _latch[ i ] ;
         utilCacheStat &statItem = (*_pStat)[ i ] ;

         pLatch->get() ;

         if ( ( statItem._totalSize > 0 &&
                statItem._freeSize * 100 / statItem._totalSize >=
                UTIL_BLOCK_RECYCLE_FREE_RATIO ) ||
              ( totalUseTimes() > 0 &&
                statItem._useTimes * _nonEmptySlotNum.peek() < totalUseTimes() ) )
         {
            /// recycle the bucket
            recycleSize += _recycleBucket( slotItem, &statItem ) ;
            /// clear the use times
            tmpTimes += statItem._useTimes ;
            statItem._useTimes = 0 ;
         }

         pLatch->release() ;
      }
      _totalUseTimes.sub( tmpTimes ) ;

      PD_LOG( PDDEBUG, "Recycle %lld blocks", recycleSize ) ;

      return recycleSize ;
   }

   UINT64 _utilCacheMgr::_recycleBucket( vector<CHAR *> &slotItem,
                                         utilCacheStat *pStat )
   {
      UINT64 recycleSize = 0 ;
      UINT32 size = ( slotItem.size() + 1 ) / 2 ;
      CHAR *pBuff = NULL ;

      for ( UINT32 i = 0 ; i < size ; ++i )
      {
         /// release block memory
         pBuff = slotItem.back() ;
         SDB_OSS_FREE( pBuff ) ;
         slotItem.pop_back() ;

         recycleSize += pStat->_pageSize ;

         if ( slotItem.empty() )
         {
            break ;
         }
      }
      /// update meta data
      pStat->_freeSize -= recycleSize ;
      pStat->_totalSize -= recycleSize ;
      _freeSize.sub( recycleSize ) ;
      _totalSize.sub( recycleSize ) ;

      if ( 0 == pStat->_totalSize && recycleSize > 0 )
      {
         _nonEmptySlotNum.dec() ;
      }

      return recycleSize ;
   }

   /*
      _utilCacheBucket implement
   */
   _utilCacheBucket::_utilCacheBucket( UINT32 blkID )
   {
      _blkID = blkID ;
      _dirtyPages = 0 ;
   }

   _utilCacheBucket::~_utilCacheBucket()
   {
      SDB_ASSERT( _pages.size() == 0, "Pages must be released" ) ;
   }

   utilCachePage* _utilCacheBucket::getPage( INT32 pageID )
   {
      MAP_BLK_PAGE::iterator it = _pages.find( pageID ) ;
      if ( it != _pages.end() )
      {
         return &(it->second) ;
      }
      return NULL ;
   }

   utilCachePage* _utilCacheBucket::addPage( INT32 pageID,
                                             const utilCachePage &page )
   {
      SDB_ASSERT( !page.isInvalid(), "Page cant' be invalid" ) ;
      utilCachePage& tmpPage = _pages[ pageID ] ;
      tmpPage = page ;
      return &tmpPage ;
   }

   utilCachePage* _utilCacheBucket::delPage( INT32 pageID )
   {
      utilCachePage* pPage = NULL ;
      MAP_BLK_PAGE::iterator it = _pages.find( pageID ) ;
      if ( it != _pages.end() )
      {
         pPage = &(it->second) ;
         SDB_ASSERT( !pPage->isDirty(), "Page can't be dirty" ) ;
         _pages.erase( it ) ;
      }
      return pPage ;
   }

   INT32 _utilCacheBucket::lock( OSS_LATCH_MODE mode, INT32 millisec )
   {
      INT32 rc = SDB_OK ;

      if ( EXCLUSIVE == mode )
      {
         rc = _rwMutex.lock_w( millisec ) ;
      }
      else if ( SHARED == mode )
      {
         rc = _rwMutex.lock_r( millisec ) ;
      }
      else
      {
         goto done ;
      }

   done:
      return rc ;
   }

   void _utilCacheBucket::unlock( OSS_LATCH_MODE mode )
   {
      if ( SHARED == mode )
      {
         _rwMutex.release_r() ;
      }
      else if ( EXCLUSIVE == mode )
      {
         _rwMutex.release_w() ;
      }
   }

   /*
      _utilCacheContext implement
   */
   _utilCacheContext::_utilCacheContext()
   {
      _pData = NULL ;
      _offset = 0 ;
      _len = 0 ;
      _pageID = 0 ;
      _pPage = NULL ;
      _pBucket = NULL ;
      _pUnit = NULL ;
      _mode = -1 ;
      _isWrite = FALSE ;
      _size = 0 ;
      _writeBack = FALSE ;
      _usePage = FALSE ;
      _hasDiscard = FALSE ;
   }

   _utilCacheContext::~_utilCacheContext()
   {
      release() ;
   }

   BOOLEAN _utilCacheContext::isValid() const
   {
      return ( _pBucket && _pUnit && _pageID >= 0 ) ? TRUE : FALSE ;
   }

   BOOLEAN _utilCacheContext::isPageValid() const
   {
      return ( _pPage && isValid() ) ? TRUE : FALSE ;
   }

   BOOLEAN _utilCacheContext::isLocked() const
   {
      return ( EXCLUSIVE == _mode || SHARED == _mode ) ? TRUE : FALSE ;
   }

   BOOLEAN _utilCacheContext::isLockRead() const
   {
      return SHARED == _mode ? TRUE : FALSE ;
   }

   BOOLEAN _utilCacheContext::isLockWrite() const
   {
      return EXCLUSIVE == _mode ? TRUE : FALSE ;
   }

   void _utilCacheContext::unLock()
   {
      if ( _pBucket && isLocked() )
      {
         _pBucket->unlock( (OSS_LATCH_MODE)_mode ) ;
      }
      _mode = -1 ;
   }

   BOOLEAN _utilCacheContext::isDone() const
   {
      return _pData ? FALSE : TRUE ;
   }

   void _utilCacheContext::makeNewest()
   {
      if ( _pPage )
      {
         _pPage->makeNewest() ;
      }
   }

   void _utilCacheContext::clearNewest()
   {
      if ( _pPage )
      {
         _pPage->clearNewest() ;
      }
   }

   void _utilCacheContext::discardPage( UINT64 &beginLSN, UINT64 &endLSN )
   {
      if ( isPageValid() && _pPage->isDirty() )
      {
         beginLSN = _pPage->beginLSN() ;
         endLSN = _pPage->endLSN() ;

         _pPage->clearDirty() ;
         _pUnit->decDirtyPages( _pBucket ) ;
         _hasDiscard = TRUE ;
      }
   }

   void _utilCacheContext::restorePage( UINT64 beginLSN, UINT64 endLSN )
   {
      if ( isPageValid() && _hasDiscard && !_pPage->isDirty() )
      {
         _pPage->makeDirty() ;
         _pUnit->incDirtyPages( _pBucket ) ;

         if ( 0 == _pPage->lsnNum() && (UINT64)~0 != beginLSN )
         {
            _pPage->addLSN( beginLSN ) ;
            if ( endLSN != beginLSN )
            {
               _pPage->addLSN( endLSN ) ;
            }
         }
      }
      _hasDiscard = FALSE ;
   }

   BOOLEAN _utilCacheContext::isInCache( UINT32 offset, UINT32 len ) const
   {
      if ( _pPage && offset >= _pPage->start() &&
           offset + len <= _pPage->length() )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   INT32 _utilCacheContext::write( const CHAR *pData,
                                   UINT32 offset,
                                   UINT32 len,
                                   IExecutor *cb )
   {
      INT32 rc = SDB_OK ;

      if ( !isValid() )
      {
         SDB_ASSERT( FALSE, "Context is invalid" ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      else if ( offset + len > _size )
      {
         SDB_ASSERT( FALSE, "offset + len must <= _size" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( !isDone() )
      {
         SDB_ASSERT( FALSE, "Must done before next write" ) ;
         submit( cb ) ;
      }

      if ( isPageValid() )
      {
         /// the page
         /// -----------|<start>-------|<end>-----------------
         ///  |--A--|-C-|              |---D--|--B--|
         /// when write in A, need to load the data C
         /// when write in B, need to load the data D
         if ( !_pPage->isDataEmpty() &&
              ( offset + len < _pPage->start() ||
                _pPage->length() < offset ) )
         {
            if ( !_pPage->isDirty() )
            {
               _pPage->clearDataInfo() ;
            }
            else
            {
               rc = _loadPage( offset, len, cb ) ;
               if( rc )
               {
                  PD_LOG( PDERROR, "Load page[ID:%d,Off:%u,Len:%u] failed, "
                          "rc: %d", _pageID, offset, len, rc ) ;
                  goto error ;
               }
            }
         }

         _pData = (CHAR*)pData ;
         _offset = offset ;
         _len = len ;
         _isWrite = TRUE ;
         _usePage = TRUE ;
         _writeBack = FALSE ;
      }
      else
      {
         _pData = (CHAR*)pData ;
         _offset = offset ;
         _len = len ;
         _isWrite = TRUE ;
         _usePage = FALSE ;
         _writeBack = FALSE ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilCacheContext::read( CHAR *pBuff,
                                  UINT32 offset,
                                  UINT32 len,
                                  IExecutor *cb )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN readPage = FALSE ;

      if ( !isValid() )
      {
         SDB_ASSERT( FALSE, "Context is invalid" ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      else if ( offset + len > _size )
      {
         SDB_ASSERT( FALSE, "offset + len must <= _size" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( !isDone() )
      {
         SDB_ASSERT( FALSE, "Must done before next read" ) ;
         submit( cb ) ;
      }

      if ( isPageValid() )
      {
         /// the page
         /// -----------|<start>-------|<end>-----------------
         ///  |--A----------|     |------B--|
         /// when read in A, need to load the data
         /// when read in B, need to load the data
         if ( offset >= _pPage->start() &&
              offset + len <= _pPage->length() )
         {
            readPage = TRUE ;
         }
         else if ( offset + len <= _pPage->start() ||
                   _pPage->length() <= offset )
         {
            /// read from file
            readPage = FALSE ;
         }
         else if ( _pPage->isDirty() )
         {
            /// load the data
            if ( offset < _pPage->start() )
            {
               rc = _loadPage( offset, _pPage->start() - offset, cb ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Load page[ID:%d,Off:%u,Len:%u] data "
                          "failed, rc: %d", _pageID, offset,
                          _pPage->start() - offset, rc ) ;
                  goto error ;
               }
            }
            if ( offset + len > _pPage->length() )
            {
               rc = _loadPage( _pPage->length(),
                               offset + len - _pPage->length(), cb ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Load page[ID:%d,Off:%u,Len:%u] data "
                          "failed, rc: %d", _pageID, _pPage->length(),
                          offset + len - _pPage->length(), rc ) ;
                  goto error ;
               }
            }
            readPage = TRUE ;
         }
      }

      if ( readPage )
      {
         _pData = (CHAR*)pBuff ;
         _offset = offset ;
         _len = len ;
         _isWrite = FALSE ;
         _usePage = TRUE ;
         _writeBack = FALSE ;
      }
      else
      {
         _pData = (CHAR*)pBuff ;
         _offset = offset ;
         _len = len ;
         _isWrite = FALSE ;
         _usePage = FALSE ;
         _writeBack = FALSE ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilCacheContext::readAndCache( CHAR *pBuff,
                                          UINT32 offset,
                                          UINT32 len,
                                          IExecutor *cb )
   {
      INT32 rc = SDB_OK ;

      rc = read( pBuff, offset, len, cb ) ;
      if ( rc )
      {
         goto error ;
      }
      _writeBack = TRUE ;

   done:
      return rc ;
   error:
      goto done ;
   }

   UINT32 _utilCacheContext::submit( IExecutor *cb )
   {
      UINT32 len = 0 ;

      if ( _pData )
      {
         INT32 rc = SDB_OK ;

         if ( _isWrite )
         {
            if ( _usePage )
            {
               BOOLEAN setDirty = FALSE ;
               /// write to page
               rc = _pPage->write( _pData, _offset, _len, setDirty ) ;
               if ( setDirty )
               {
                  _pUnit->incDirtyPages( _pBucket ) ;
               }
               if( cb )
               {
                  _pPage->addLSN( cb->getEndLsn() ) ;
               }
            }
            else
            {
               /// write to file
               utilCachFileBase* pFile = _pUnit->getCacheFile() ;
               rc = pFile->write( _pageID, _pData, _len, _offset, cb ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Write page[ID:%d,Off:%u,Len:%u] to "
                          "file[%s] failed, rc: %d", _pageID, _offset, _len,
                          pFile->getFileName(), rc ) ;
               }
            }
         }
         else
         {
            if ( _usePage )
            {
               /// read from page
               len = _pPage->read( _pData, _offset, _len ) ;
            }
            else
            {
               /// read from file
               utilCachFileBase* pFile = _pUnit->getCacheFile() ;
               rc = pFile->read( _pageID, _pData, _len, _offset, len, cb ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Read page[ID:%d,Off:%u,Len:%u] from "
                          "file[%s] failed, rc: %d", _pageID, _offset,
                          _len, pFile->getFileName(), rc ) ;
               }
               /// write to cache page
               if ( _writeBack && _pPage )
               {
                  _pPage->load( _pData, _offset, len ) ;
               }
            }
         }

         if ( rc )
         {
            ossPanic() ;
         }

         /// clear data info
         _pData = NULL ;
         _len = 0 ;
         _offset = 0 ;
         _isWrite = FALSE ;
         _writeBack = FALSE ;
      }
      _hasDiscard = FALSE ;

      return len ;
   }

   void _utilCacheContext::rollback()
   {
      if ( _pData )
      {
         _pData = NULL ;
         _offset = 0 ;
         _len = 0 ;
         _isWrite = FALSE ;
         _writeBack = FALSE ;
      }
   }

   void _utilCacheContext::release()
   {
      rollback() ;
      unLock() ;
      _pPage = NULL ;
      _pBucket = NULL ;
      _pUnit = NULL ;
      _hasDiscard = FALSE ;
   }

   INT32 _utilCacheContext::_loadPage( UINT32 offset,
                                       UINT32 len,
                                       IExecutor *cb )
   {
      INT32 rc = SDB_OK ;
      utilCachFileBase* pFile = NULL ;
      CHAR *pBuff = NULL ;
      UINT32 readLen = 0 ;

      SDB_ASSERT( _pPage && _pPage->size() >= offset + len,
                  "Page size must grater than offset + len" ) ;

      if ( _pPage->isNewest() )
      {
         rc = _pPage->loadWithoutData( offset, len ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Load without data in page failed, rc: %d", rc ) ;
            goto error ;
         }
      }
      else if ( 1 == _pPage->blockNum() )
      {
         CHAR *ptr = _pPage->str() ;
         pFile = _pUnit->getCacheFile() ;
         /// read from file
         rc = pFile->read( _pageID, ptr, len, offset, readLen, cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Read from file[%s] failed, rc: %d",
                    pFile->getFileName(), rc ) ;
            goto error ;
         }
         /// load with no data
         rc = _pPage->loadWithoutData( offset, len ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Load without data in page failed, rc: %d", rc ) ;
            goto error ;
         }
      }
      else
      {
         rc = cb->allocBuff( len, &pBuff, NULL ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Alloc buff from cb failed, rc: %d", rc ) ;
            goto error ;
         }

         pFile = _pUnit->getCacheFile() ;
         /// read from file
         rc = pFile->read( _pageID, pBuff, len, offset, readLen, cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Read from file[%s] failed, rc: %d",
                    pFile->getFileName(), rc ) ;
            goto error ;
         }
         /// write to page
         rc = _pPage->load( pBuff, offset, readLen ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Write data to page failed, rc: %d", rc ) ;
            goto error ;
         }
      }

   done:
      if ( pBuff )
      {
         cb->releaseBuff( pBuff ) ;
         pBuff = NULL ;
      }
      return rc ;
   error:
      goto done ;
   }

   /*
      _utilCacheUnit implement
   */

   /*
      Max sync pages number for once time
   */
   #define UTIL_CACHE_SYNC_ONCE_NUM          ( 3000 )
   #define UTIL_CACHE_SYNC_TOTAL_THRESHOLD   ( 100 )

   _utilCacheUnit::_utilCacheUnit()
   :_dirtySize( 0 ), _totalPage( 0 )
   {
      _pMgr = NULL ;
      _pCacheFile = NULL ;
      _bucketSize = 0 ;
      _pageSize = 0 ;
      _closed = TRUE ;
      _wholePage = FALSE ;
      _lastRecycleTime = 0 ;
      _lastSyncTime = 0 ;
      _useCache = TRUE ;

      _bgDirtyRatio = UTIL_CACHEUNIT_BG_DIRTY_RATIO ;
      _dirtyTimeout = UTIL_CACHEUNIT_DIRTY_TIMEOUT ;
      _bgFreeRatio = UTIL_CACHEUNIT_BG_FREE_RATIO ;
      _pageTimeout = UTIL_CACHEUNIT_PAGE_TIMEOUT ;
   }

   _utilCacheUnit::~_utilCacheUnit()
   {
      SDB_ASSERT( 0 == _bucketSize, "Must call fini before this function" ) ;
   }

   INT32 _utilCacheUnit::init ( utilCacheMgr *pMgr,
                                utilCachFileBase *pFile,
                                UINT32 pageSize,
                                UINT32 bucketSize,
                                BOOLEAN useCache,
                                BOOLEAN wholePage )
   {
      INT32 rc = SDB_OK ;
      utilCacheBucket* pBucket = NULL ;

      if ( !pMgr || !pFile || 0 == pageSize || 0 == bucketSize )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      _pMgr = pMgr ;
      _pCacheFile = pFile ;
      _pageSize = pageSize ;
      _useCache = useCache ;
      _wholePage = wholePage ;

      for ( UINT32 i = 0 ; i < bucketSize ; ++i )
      {
         pBucket = SDB_OSS_NEW utilCacheBucket( i ) ;
         if ( !pBucket )
         {
            PD_LOG( PDERROR, "Alloc bucket[%u] failed", i ) ;
            rc = SDB_OOM ;
            goto error ;
         }
         _vecBucket.push_back( pBucket ) ;
         ++_bucketSize ;
      }

      /// register the unit
      _pMgr->registerUnit( this ) ;
      _closed = FALSE ;

   done:
      return rc ;
   error:
      goto done ;
   }

   void _utilCacheUnit::setDirtyConfig( UINT32 bgDirtyRatio,
                                        UINT32 dirtyTimeout )
   {
      _bgDirtyRatio = bgDirtyRatio ;
      _dirtyTimeout = dirtyTimeout ;
   }

   void _utilCacheUnit::setRecycleConfig( UINT32 bgFreeRatio,
                                          UINT32 pageTimeout )
   {
      _bgFreeRatio = bgFreeRatio ;
      _pageTimeout = pageTimeout ;
   }

   void _utilCacheUnit::fini( IExecutor *cb )
   {
      utilCacheBucket* pBucket = NULL ;
      utilCacheBucket::MAP_BLK_PAGE* pPages = NULL ;

      _closed = TRUE ;

      /// unregister self must be after set closed
      if ( _pMgr )
      {
         _pMgr->unregUnit( this ) ;
      }

      /// wait the page cleaner
      _pageCleaner.lock_w() ;
      _pageCleaner.release_w() ;

      /// wait all dirty page flushed to file
      while( dirtyPages() > 0 )
      {
         syncPages( cb, TRUE, TRUE ) ;
      }

      for ( UINT32 i = 0 ; i < _bucketSize ; ++i )
      {
         pBucket = _vecBucket[ i ] ;

         pPages = pBucket->getPages() ;

         utilCacheBucket::MAP_BLK_PAGE::iterator it = pPages->begin() ;
         while ( it != pPages->end() )
         {
            _pMgr->release( it->second ) ;
            ++it ;
         }
         pPages->clear() ;

         SDB_OSS_DEL pBucket ;
      }
      _vecBucket.clear() ;
      _bucketSize = 0 ;
   }

   UINT32 _utilCacheUnit::calcBucketID( INT32 pageID ) const
   {
      if ( 0 == _bucketSize )
      {
         return 0 ;
      }
      return (UINT32)pageID % _bucketSize ;
   }

   UINT64 _utilCacheUnit::totalPages()
   {
      return _totalPage.peek() ;
   }

   UINT64 _utilCacheUnit::dirtyPages()
   {
      return _dirtySize.peek() ;
   }

   void _utilCacheUnit::decDirtyPages( utilCacheBucket *pBucket )
   {
      _dirtySize.dec() ;
      pBucket->decDirty() ;
   }

   void _utilCacheUnit::incDirtyPages( utilCacheBucket *pBucket )
   {
      _dirtySize.inc() ;
      pBucket->incDirty() ;
   }

   utilCachePage* _utilCacheUnit::getAndLock( INT32 pageID, UINT32 size,
                                              utilCacheBucket **ppBucket,
                                              OSS_LATCH_MODE mode,
                                              BOOLEAN alloc,
                                              IExecutor *cb )
   {
      utilCachePage* pPage = NULL ;
      UINT32 bucketID = calcBucketID( pageID ) ;
      utilCacheBucket* pBucket = NULL ;
      OSS_LATCH_MODE tmpMode = mode ;
      BOOLEAN lockPage = FALSE ;

      pBucket = _vecBucket[ bucketID ] ;
      pBucket->lock( tmpMode ) ;
      *ppBucket = pBucket ;

      if ( pageID < 0 )
      {
         PD_LOG( PDERROR, "Page id[%d] is invalid", pageID ) ;
         SDB_ASSERT( pageID >= 0, "Page must >= 0" ) ;
         goto done ;
      }
      if ( !_useCache )
      {
         goto done ;
      }
      /// min for the _pageSize
      if ( size > 0 && size < _pageSize )
      {
         size = _pageSize ;
      }

   reget:
      pPage = pBucket->getPage( pageID ) ;
      if ( !pPage && alloc && size > 0 )
      {
         utilCachePage tmpPage ;
         INT32 rc = SDB_OK ;

         /// check the size and whether closed
         if ( _pMgr->maxCacheSize() < size || _closed )
         {
            goto done ;
         }
         if ( tmpMode != EXCLUSIVE )
         {
            /// need to switch to exclusive
            pBucket->unlock( tmpMode ) ;
            tmpMode = EXCLUSIVE ;
            pBucket->lock( tmpMode ) ;
            goto reget ;
         }
         rc = _pMgr->alloc( size, tmpPage, _wholePage ) ;
         if ( SDB_OK == rc )
         {
            /// add to bucket
            pPage = pBucket->addPage( pageID, tmpPage ) ;
            _totalPage.inc() ;
         }
         else
         {
            if ( SDB_OSS_UP_TO_LIMIT != rc )
            {
               PD_LOG( PDERROR, "Alloc page[%u] failed, rc: %d",
                       size, rc ) ;
            }
            goto done ;
         }
      }
      else if ( pPage && pPage->size() < size )
      {
         if ( tmpMode != EXCLUSIVE )
         {
            pBucket->unlock( tmpMode ) ;
            tmpMode = EXCLUSIVE ;
            pBucket->lock( tmpMode ) ;
            goto reget ;
         }
         INT32 rc = _pMgr->alloc( size, *pPage, _wholePage,
                                  pPage->isInvalid() ? FALSE : TRUE ) ;
         if ( rc )
         {
            if( !pPage->isLocked() )
            {
               pPage->lock() ;
               lockPage = TRUE ;
            }
            pBucket->unlock( tmpMode ) ;

            /// recycle and try again
            recyclePages( TRUE, size ) ;

            pBucket->lock( tmpMode ) ;

            if( lockPage )
            {
               pPage->unlock() ;
               lockPage = FALSE ;
            }

            rc = _pMgr->alloc( size, *pPage, _wholePage,
                               pPage->isInvalid() ? FALSE : TRUE ) ;
            if ( rc )
            {
               if ( SDB_OSS_UP_TO_LIMIT != rc )
               {
                  PD_LOG( PDERROR, "Alloc page[%u] failed, rc: %d",
                          size, rc ) ;
               }
               /// if the page is dirty, need to sync first
               if ( pPage->isDirty() )
               {
                  rc = _syncPage( pBucket, pPage, pageID, cb ) ;
                  if ( rc )
                  {
                     PD_LOG( PDERROR, "Sync page[%d] failed, rc: %d",
                             pageID, rc ) ;
                     ossPanic() ;
                  }
               }
               pPage->clearDataInfo() ;
               pPage = NULL ;
               goto done ;
            }
         }
      }

   done:
      if ( mode != tmpMode )
      {
         /// switch to mode
         if ( pPage && !pPage->isLocked() )
         {
            pPage->lock() ;
            lockPage = TRUE ;
         }
         pBucket->unlock( tmpMode ) ;
         pBucket->lock( mode ) ;
         if ( pPage && lockPage )
         {
            pPage->unlock() ;
         }
      }
      return pPage ;
   }

   utilCacheBucket* _utilCacheUnit::getBucket( UINT32 index )
   {
      if ( index >= _bucketSize )
      {
         return NULL ;
      }
      return _vecBucket[ index ] ;
   }

   void _utilCacheUnit::prepareWrite( INT32 pageID,
                                      UINT32 offset,
                                      UINT32 len,
                                      IExecutor *cb,
                                      utilCacheContext &context )
   {
      context.release() ;
      context._pageID = pageID ;
      context._pUnit = this ;
      context._mode = EXCLUSIVE ;
      context._size = offset + len ;
      context._pPage = getAndLock( pageID, offset + len,
                                   &context._pBucket,
                                   EXCLUSIVE, TRUE, cb ) ;
   }

   void _utilCacheUnit::prepareRead( INT32 pageID,
                                     UINT32 offset,
                                     UINT32 len,
                                     IExecutor *cb,
                                     utilCacheContext &context )
   {
      context.release() ;
      context._pageID = pageID ;
      context._pUnit = this ;
      context._mode = SHARED ;
      context._size = offset + len ;
      context._pPage = getAndLock( pageID, offset + len,
                                   &context._pBucket,
                                   SHARED, FALSE, cb ) ;
   }

   INT32 _utilCacheUnit::_syncPage( utilCacheBucket *pBucket,
                                    utilCachePage *pPage,
                                    INT32 pageID,
                                    IExecutor *cb,
                                    BOOLEAN *pSync )
   {
      INT32 rc = SDB_OK ;
      UINT32 pos = 0 ;
      UINT32 len = 0 ;
      CHAR *pBuff = NULL ;
      UINT32 offset = 0 ;
      UINT32 lastLen = pPage->dirtyLength() ;
      BOOLEAN hasSync = FALSE ;

      if ( !pPage->isDirty() )
      {
         goto done ;
      }
      else if ( pPage->isDirtyEmpty() )
      {
         hasSync = TRUE ;
         pPage->clearDirty() ;
         decDirtyPages( pBucket ) ;
         goto done ;
      }

      pos = pPage->beginBlock() ;
      while( NULL != ( pBuff = pPage->nextBlock( len, pos ) ) )
      {
         if ( pPage->dirtyStart() > offset + len )
         {
            offset += len ;
            lastLen -= len ;
            continue ;
         }
         else if ( pPage->dirtyStart() > offset )
         {
            UINT32 pageOffset = pPage->dirtyStart() - offset ;
            pBuff += pageOffset ;
            lastLen -= pageOffset ;
            offset += pageOffset ;
            len -= pageOffset ;
         }

         if ( lastLen < len )
         {
            len = lastLen ;
         }
         rc = _pCacheFile->write( pageID, pBuff, len, offset, cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Sync dirty page[ID:%d,Offset:%u,Len:%u] to "
                    "file[%s] failed, rc: %d", pageID, offset, len,
                    _pCacheFile->getFileName(), rc ) ;
            goto error ;
         }
         offset += len ;
         lastLen -= len ;
         if ( 0 == lastLen )
         {
            break ;
         }
      }

      /// clear the dirty
      hasSync = TRUE ;
      pPage->clearDirty() ;
      decDirtyPages( pBucket ) ;

   done:
      if ( pSync )
      {
         *pSync = hasSync ;
      }
      return rc ;
   error:
      goto done ;
   }

   void _utilCacheUnit::lockPageCleaner( INT32 mode )
   {
      if ( SHARED == mode )
      {     
         _pageCleaner.lock_r() ;
      }
      else
      {
         _pageCleaner.lock_w() ;
      }
   }

   void _utilCacheUnit::unlockPageCleaner( INT32 mode )
   {
      if ( SHARED == mode )
      {
         _pageCleaner.release_r() ;
      }
      else
      {
         _pageCleaner.release_w() ;
      }
   }

   BOOLEAN _utilCacheUnit::canSync( BOOLEAN &force )
   {
      if ( dirtyPages() <= 0 )
      {
         return FALSE ;
      }
      /// when dirty ratio over the threshold
      else if ( totalPages() > UTIL_CACHE_SYNC_TOTAL_THRESHOLD &&
                dirtyPages() * 100 / totalPages() >= _bgDirtyRatio )
      {
         PD_LOG( PDDEBUG, "Dirty pages: %u, Total pages: %u, Dirty ratio: %u",
                 dirtyPages(), totalPages(), _bgDirtyRatio ) ;
         force = TRUE ;
         return TRUE ;
      }
      /// when page dirty timeout
      else
      {
         ossTimestamp t ;
         ossGetCurrentTime( t ) ;
         UINT64 curTime = t.time * 1000 + t.microtm / 1000 ;

         force = FALSE ;

         if ( ( curTime >= _lastSyncTime &&
                curTime - _lastSyncTime > _dirtyTimeout ) ||
              ( curTime < _lastSyncTime &&
                _lastSyncTime - curTime > _dirtyTimeout ) )
         {
            PD_LOG( PDDEBUG, "Cur time: %llu, Last sync time: %llu, "
                    "Dirty timeout: %u", curTime, _lastSyncTime,
                    _dirtyTimeout ) ;
            return TRUE ;
         }
      }
      return FALSE ;
   }

   UINT32 _utilCacheUnit::syncPages( IExecutor *cb, BOOLEAN force,
                                     BOOLEAN ignoreClose )
   {
      UINT32 totalPages = 0 ;
      utilCacheBucket* pBucket = NULL ;
      ossTimestamp t ;
      MAP_ID_2_PAGE_PRT tmpPages ;

      /// get current time
      ossGetCurrentTime( t ) ;
      _lastSyncTime = t.time * 1000 + t.microtm / 1000 ;

      for ( UINT32 i = 0 ; i < _bucketSize ; ++i )
      {
         if ( _closed && !ignoreClose )
         {
            break ;
         }
         pBucket = _vecBucket[ i ] ;
         pBucket->lock( SHARED ) ;
         utilCacheBucket::MAP_BLK_PAGE* pPages = pBucket->getPages() ;
         utilCacheBucket::MAP_BLK_PAGE::iterator it = pPages->begin() ;
         while( it != pPages->end() )
         {
            utilCachePage &tmpPage = it->second ;

            /// un-dirty page, ignored
            if ( !tmpPage.isDirty() )
            {
               ++it ;
               continue ;
            }
            /// add to tmp map, and sort
            else if ( force || _lastSyncTime - tmpPage.lastWriteTime() >=
                      _dirtyTimeout )
            {
               tmpPages[ it->first ] = &tmpPage ;
            }
            ++it ;
         }
         pBucket->unlock( SHARED ) ;

         if ( tmpPages.size() >= UTIL_CACHE_SYNC_ONCE_NUM )
         {
            /// sync pages
            totalPages += _syncPages( tmpPages, cb ) ;
            tmpPages.clear() ;
         }
      }

      totalPages += _syncPages( tmpPages, cb ) ;

      PD_LOG( PDDEBUG, "Sync %d pages", totalPages ) ;

      return totalPages ;
   }

   UINT32 _utilCacheUnit::dropDirty()
   {
      UINT32 totalPages = 0 ;
      utilCacheBucket* pBucket = NULL ;
      utilCacheBucket::MAP_BLK_PAGE* pPages = NULL ;

      for ( UINT32 i = 0 ; i < _bucketSize ; ++i )
      {
         pBucket = _vecBucket[ i ] ;
         pBucket->lock( EXCLUSIVE ) ;
         pPages = pBucket->getPages() ;
         utilCacheBucket::MAP_BLK_PAGE::iterator it = pPages->begin() ;
         while ( it != pPages->end() )
         {
            utilCachePage& page = it->second ;

            /// locked page, ignored
            if ( page.isLocked() )
            {
               ++it ;
               continue ;
            }
            else if ( page.isDirty() )
            {
               page.clearDirty() ;
               ++totalPages ;
               decDirtyPages( pBucket ) ;
            }
            ++it ;
         }
         pBucket->unlock( EXCLUSIVE ) ;
      }

      PD_LOG( PDDEBUG, "Dropped %lld pages", totalPages ) ;

      return totalPages ;
   }

   UINT32 _utilCacheUnit::_syncPages( _utilCacheUnit::MAP_ID_2_PAGE_PRT &pageMap,
                                      IExecutor *cb )
   {
      INT32 rc = SDB_OK ;
      UINT32 totalPages = 0 ;
      BOOLEAN hasSync = FALSE ;
      utilCacheBucket *pBucket = NULL ;
      MAP_ID_2_PAGE_PRT_IT it = pageMap.begin() ;

      while( it != pageMap.end() )
      {
         pBucket = getBucket( calcBucketID( it->first ) ) ;
         pBucket->lock( SHARED ) ;
         rc = _syncPage( pBucket, it->second, it->first, cb, &hasSync ) ;
         pBucket->unlock( SHARED ) ;

         if ( rc )
         {
            PD_LOG( PDERROR, "Sync page[%d] failed, rc: %d",
                    it->first, rc ) ;
            ossPanic() ;
         }
         totalPages += ( hasSync ? 1 : 0 ) ;
         ++it ;
      }
      pageMap.clear() ;

      return totalPages ;
   }

   BOOLEAN _utilCacheUnit::canRecycle( BOOLEAN &force )
   {
      if ( totalPages() <= 0 )
      {
         return FALSE ;
      }
      /// when free ratio over the threshold
      else if ( _pMgr->totalSize() * 100 / _pMgr->maxCacheSize() >=
                UTIL_CACHE_RATIO &&
                _pMgr->freeSize() * 100 / _pMgr->totalSize() <=
                 _bgFreeRatio )
      {
         PD_LOG( PDDEBUG, "Total size: %u, Free size: %u, Free ratio: %u",
                 _pMgr->totalSize(), _pMgr->freeSize(), _bgFreeRatio ) ;
         force = TRUE ;
         return TRUE ;
      }
      /// when page dirty timeout
      else
      {
         ossTimestamp t ;
         ossGetCurrentTime( t ) ;
         UINT64 curTime = t.time * 1000 + t.microtm / 1000 ;

         force = FALSE ;

         if ( ( curTime >= _lastRecycleTime &&
                curTime - _lastRecycleTime > _pageTimeout ) ||
              ( curTime < _lastRecycleTime &&
                _lastRecycleTime - curTime > _pageTimeout ) )
         {
            PD_LOG( PDDEBUG, "Cur time: %llu, Last recycle time: %llu, "
                    "Page timeout: %u", curTime, _lastRecycleTime,
                    _pageTimeout ) ;
            return TRUE ;
         }
      }
      return FALSE ;
   }

   UINT64 _utilCacheUnit::recyclePages( BOOLEAN force, INT64 exceptSize )
   {
      UINT64 totalSize = 0 ;
      ossTimestamp t ;
      utilCacheBucket* pBucket = NULL ;
      utilCacheBucket::MAP_BLK_PAGE* pPages = NULL ;

      /// get current time
      ossGetCurrentTime( t ) ;
      _lastRecycleTime = t.time * 1000 + t.microtm / 1000 ;

      for ( UINT32 i = 0 ; i < _bucketSize ; ++i )
      {
         if ( _closed )
         {
            break ;
         }
         pBucket = _vecBucket[ i ] ;
         pBucket->lock( EXCLUSIVE ) ;
         pPages = pBucket->getPages() ;
         utilCacheBucket::MAP_BLK_PAGE::iterator it = pPages->begin() ;
         while ( it != pPages->end() )
         {
            utilCachePage& page = it->second ;

            /// dirty page and locked page, ignored
            if ( page.isDirty() || page.isLocked() )
            {
               ++it ;
               continue ;
            }
            /// not-force and time is not up to the limit, ignored
            if ( !force && _lastRecycleTime - page.lastTime() < _pageTimeout )
            {
               ++it ;
               continue ;
            }

            /// release the page
            totalSize += page.size() ;
            _pMgr->release( page ) ;
            it = pPages->erase( it ) ;
            /// update the meta
            _totalPage.dec() ;
         }
         pBucket->unlock( EXCLUSIVE ) ;

         /// when up to the size, break
         if ( exceptSize > 0 && totalSize > ( ( UINT64 )exceptSize << 1 ) )
         {
            break ;
         }
      }

      PD_LOG( PDDEBUG, "Recycled %lld bytes", totalSize ) ;

      return totalSize ;
   }

}

