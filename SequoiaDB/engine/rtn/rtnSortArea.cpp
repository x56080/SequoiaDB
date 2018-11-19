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

   Source File Name = rtnSortArea.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains declare for runtime
   functions.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/06/2018  YSD  Initial Draft

   Last Changed =

*******************************************************************************/

#include "rtnSortArea.hpp"
#include "rtnTrace.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"

namespace engine
{
   _rtnSortAreaBlock::_rtnSortAreaBlock( size_t size )
   : _buff( NULL ),
     _size( size ),
     _writePos( 0 )
   {
   }

   _rtnSortAreaBlock::~_rtnSortAreaBlock()
   {
      SAFE_OSS_FREE( _buff ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNSORTAREABLOCK_INIT, "_rtnSortAreaBlock::init" )
   INT32 _rtnSortAreaBlock::init()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNSORTAREABLOCK_INIT ) ;

      _buff = (CHAR *)SDB_OSS_MALLOC( _size ) ;
      if ( !_buff )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Allocate memory for sort area block failed. "
                          "Requested size: %zd", _size ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNSORTAREABLOCK_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNSORTAREABLOCK_APPEND, "_rtnSortAreaBlock::append" )
   INT32 _rtnSortAreaBlock::append( const CHAR *data, size_t len )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNSORTAREABLOCK_APPEND ) ;

      SDB_ASSERT( data, "Data is NULL" ) ;

      if ( !_buff )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Sort area block not initialized yet" ) ;
         goto error ;
      }

      if ( freeSize() < len )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Data size too big for the block" ) ;
         goto error ;
      }

      ossMemcpy( _buff + _writePos, data, len ) ;
      _writePos += len ;

   done:
      PD_TRACE_EXITRC( SDB__RTNSORTAREABLOCK_APPEND, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   CHAR *_rtnSortAreaBlock::offset2Addr( size_t offset ) const
   {
      return ( offset < _size ) ? ( _buff + offset ) : NULL ;
   }

   size_t _rtnSortAreaBlock::capacity() const
   {
      return _size ;
   }

   size_t _rtnSortAreaBlock::length() const
   {
      return _writePos ;
   }

   size_t _rtnSortAreaBlock::freeSize() const
   {
      return _size - _writePos ;
   }

   void _rtnSortAreaBlock::reset()
   {
      _writePos = 0 ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNSORTAREABLOCK__RESIZE, "_rtnSortAreaBlock::_resize" )
   INT32 _rtnSortAreaBlock::_resize( size_t newSize )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNSORTAREABLOCK__RESIZE ) ;
      CHAR *newBuf = NULL ;

      newBuf = ( CHAR * )SDB_OSS_REALLOC( _buff, newSize ) ;
      if ( !newBuf )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Reallocate block size from %zd to %zd failed",
                 _size, newSize ) ;
         goto error ;
      }

      _buff = newBuf ;
      _size = newSize ;

   done:
      PD_TRACE_EXITRC( SDB__RTNSORTAREABLOCK__RESIZE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   _rtnSortArea::_rtnSortArea()
   : _limit( 0 ),
     _totalSize( 0 )
   {
   }

   _rtnSortArea::~_rtnSortArea()
   {
      reset( FALSE ) ;
   }

   INT32 _rtnSortArea::init( size_t limit )
   {
      _limit = limit ;

      return SDB_OK ;
   }

   BOOLEAN _rtnSortArea::hasInit() const
   {
      return ( 0 != _limit ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNSORTAREA_ALLOCBLOCK, "_rtnSortArea::allocBlock" )
   INT32 _rtnSortArea::allocBlock( size_t size, rtnSortAreaBlock *&block,
                                   BOOLEAN accurateSize )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNSORTAREA_ALLOCBLOCK ) ;
      rtnSortAreaBlock *newBlock = NULL ;
      BLOCK_MAP_ITR itr ;

      if ( !hasInit() )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Sort area not initialized yet" ) ;
         goto error ;
      }

      if ( 0 == size )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Request size[%zd] is invalid", size ) ;
         goto error ;
      }

      // upper_bound will return the item greater than size. Equal to or greater
      // than is what we want.
      itr = _idleBlocks.upper_bound( size - 1 ) ;
      if ( _idleBlocks.end() != itr )
      {
         newBlock = itr->second ;
         if ( accurateSize && ( newBlock->capacity() != size ) )
         {
            rc = reallocBlock( newBlock, size ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Reallocate sort area block to size[%zd] "
                                "failed[%d]", size, rc ) ;
               goto error ;
            }
         }

         newBlock->reset() ;

         // Move it from idle list to active list.
         rc = _activeBlocks.push_back( newBlock ) ;
         PD_RC_CHECK( rc, PDERROR, "Insert block into active list failed[%c]",
                      rc ) ;
         _idleBlocks.erase( itr ) ;
      }
      else
      {
      retry:
         rc = _allocBlock( size, newBlock ) ;
         if ( rc )
         {
            if ( SDB_HIT_HIGH_WATERMARK != rc )
            {
               PD_LOG( PDERROR, "Allocate sort area block failed[%d]. "
                                "Requested size[%zd]", rc, size ) ;
            }
            else
            {
               // If HWM is hit, release one idle block(if any) and retry to
               // allocate.
               BLOCK_MAP_ITR firstItr = _idleBlocks.begin() ;
               if ( firstItr != _idleBlocks.end() )
               {
                  rtnSortAreaBlock *blockTmp = firstItr->second ;
                  _totalSize -= blockTmp->capacity() ;
                  SDB_OSS_DEL blockTmp ;
                  _idleBlocks.erase( firstItr ) ;
                  goto retry ;
               }
            }
            goto error ;
         }
      }

      block = newBlock ;

   done:
      PD_TRACE_EXITRC( SDB__RTNSORTAREA_ALLOCBLOCK, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNSORTAREA_REALLOCBLOCK, "_rtnSortArea::reallocBlock" )
   INT32 _rtnSortArea::reallocBlock( rtnSortAreaBlock *block, size_t newSize )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNSORTAREA_REALLOCBLOCK ) ;
      INT64 extendSize = newSize - block->capacity() ;

      if ( _totalSize + extendSize > _limit )
      {
         rc = SDB_HIT_HIGH_WATERMARK ;
         goto error ;
      }

      rc = block->_resize( newSize ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Resize block from size %zd to %zd failed[%d]",
                 block->capacity(), newSize, rc ) ;
         goto error ;
      }

      _totalSize += extendSize ;

   done:
      PD_TRACE_EXITRC( SDB__RTNSORTAREA_REALLOCBLOCK, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNSORTAREA_RELEASEBLOCK, "_rtnSortArea::releaseBlock" )
   INT32 _rtnSortArea::releaseBlock( rtnSortAreaBlock *&block, BOOLEAN reserve )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNSORTAREA_RELEASEBLOCK ) ;
      SDB_ASSERT( block, "Block pointer to free is NULL" ) ;

      // Only active blocks can be released.
      BLOCK_LIST_ITR itr = _activeBlocks.begin() ;
      while ( itr != _activeBlocks.end() )
      {
         if ( block == *itr )
         {
            break ;
         }
         ++itr ;
      }

      if ( _activeBlocks.end() == itr )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Block to free is not allocated by this sort area" ) ;
         goto error ;
      }

      _activeBlocks.erase( itr ) ;

      if ( reserve )
      {
         try
         {
            _idleBlocks.insert( std::make_pair( block->capacity(), block ) ) ;
         }
         catch ( std::exception &e )
         {
            // In case of STL exception, also free the block, even when the free
            // option is false.
            _totalSize -= block->capacity() ;
            SDB_OSS_DEL block ;

            PD_LOG( PDDEBUG, "Unexpected exception occurred when inserting "
                             "block into idle list: %s", e.what() ) ;
         }
      }
      else
      {
         _totalSize -= block->capacity() ;
         SDB_OSS_DEL block ;
      }

      block = NULL;

   done:
      PD_TRACE_EXITRC( SDB__RTNSORTAREA_RELEASEBLOCK, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNSORTAREA_RESET, "_rtnSortArea::reset" )
   void _rtnSortArea::reset( BOOLEAN reserveBlocks )
   {
      PD_TRACE_ENTRY( SDB__RTNSORTAREA_RESET ) ;
      if ( reserveBlocks )
      {
         // Try to move all active blocks into idle set.
         while ( !_activeBlocks.empty() )
         {
           rtnSortAreaBlock *block = _activeBlocks.back() ;
           block->reset() ;
           _activeBlocks.pop_back() ;
           try
           {
              _idleBlocks.insert( std::make_pair( block->capacity(), block ) ) ;
           }
           catch ( std::exception &e )
           {
              // If not able to reserve the block, release it directly.
              _totalSize -= block->capacity() ;
              SDB_OSS_DEL block ;
              PD_LOG( PDERROR, "Unexpected exception occurred when moving "
                               "block to idle list: %s. Delete it directly",
                               e.what() ) ;
           }
         }
      }
      else
      {
         // Free all active blocks.
         for ( BLOCK_LIST_ITR itr = _activeBlocks.begin();
               itr != _activeBlocks.end(); ++itr )
         {
            SDB_OSS_DEL *itr ;
         }
         _activeBlocks.clear() ;

         // Free all idle blocks.
         for ( BLOCK_MAP_ITR itr = _idleBlocks.begin();
               itr != _idleBlocks.end(); ++itr )
         {
            SDB_OSS_DEL itr->second ;
         }
         _idleBlocks.clear() ;
         _totalSize = 0 ;
      }

      PD_TRACE_EXIT( SDB__RTNSORTAREA_RESET ) ;
   }

   size_t _rtnSortArea::capacity() const
   {
      return _limit ;
   }

   size_t _rtnSortArea::usedSpace() const
   {
      return _totalSize ;
   }

   size_t _rtnSortArea::freeSpace() const
   {
      return ( _limit - _totalSize ) ;
   }

   size_t _rtnSortArea::currentMaxBlockSize( BOOLEAN includeReserve ) const
   {
      rtnSortAreaBlock *block = _getMaxBlock( includeReserve ) ;
      return block ? block->capacity() : 0 ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNSORTAREA_GETMAXSINGLEBLOCK, "_rtnSortArea::getMaxSingleBlock" )
   rtnSortAreaBlock *_rtnSortArea::getMaxSingleBlock()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNSORTAREA_GETMAXSINGLEBLOCK ) ;
      BOOLEAN isActive =  FALSE ;
      rtnSortAreaBlock *block = _getMaxBlock() ;

      if ( block )
      {
         // Free all the blocks except the largest one, and try to resize it
         // up to limit.
         BLOCK_LIST_ITR itr = _activeBlocks.begin() ;
         while ( itr != _activeBlocks.end() )
         {
            if ( *itr == block )
            {
               ++itr ;
               isActive = TRUE ;
            }
            else
            {
               SDB_OSS_DEL *itr ;
               itr = _activeBlocks.erase( itr ) ;
            }
         }

         BLOCK_MAP_ITR mItr = _idleBlocks.begin() ;
         while ( mItr != _idleBlocks.end() )
         {
            if ( mItr->second != block )
            {
               SDB_OSS_DEL mItr->second ;
            }
            // If the max block is in the idle set, try to move to active list.
            // So always erase.
            _idleBlocks.erase( mItr++ ) ;
         }

         rc = block->_resize( _limit ) ;
         if ( rc )
         {
            // Resize failed, the block will remain unchanged.
            PD_LOG( PDDEBUG, "Resize block to %zd failed[%d]", _limit, rc ) ;
         }
         block->reset() ;
      }
      else
      {
         SDB_ASSERT( 0 == _activeBlocks.size(),
                     "Active block list is not empty" ) ;
         SDB_ASSERT( 0 == _idleBlocks.size(), "Idle block list is not empty" ) ;
         rc = _allocBlock( _limit, block ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Allocate sort area block failed[%d]. Requested "
                             "size: %zd", rc, _limit ) ;
            goto error ;
         }
      }

      if ( !isActive )
      {
         rc = _activeBlocks.push_back( block ) ;
         if ( rc )
         {
            SDB_OSS_DEL block ;
            _totalSize = 0 ;
            PD_LOG( PDERROR, "Add sort area block into active list failed[%d]",
                    rc ) ;
            goto error ;
         }
      }

      _totalSize = block->capacity() ;

   done:
      PD_TRACE_EXITRC( SDB__RTNSORTAREA_GETMAXSINGLEBLOCK, rc ) ;
      return block ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNSORTAREA__ALLOCBLOCK, "_rtnSortArea::_allocBlock" )
   INT32 _rtnSortArea::_allocBlock( size_t size, rtnSortAreaBlock *&block )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNSORTAREA__ALLOCBLOCK ) ;
      rtnSortAreaBlock *newBlock = NULL ;

      if ( _totalSize + size > _limit )
      {
         rc = SDB_HIT_HIGH_WATERMARK ;
         goto error ;
      }

      newBlock = SDB_OSS_NEW rtnSortAreaBlock( size ) ;
      if ( !newBlock )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Allocate memory for sort area block failed. "
                          "Requested size: %zd", size ) ;
         goto error ;
      }

      rc = newBlock->init() ;
      PD_RC_CHECK( rc, PDERROR, "Initialize sort area block failed[%d]", rc ) ;

      rc = _activeBlocks.push_back( newBlock ) ;
      PD_RC_CHECK( rc, PDERROR, "Insert sort area block into active list "
                                "failed[%d]", rc ) ;

      block = newBlock ;
      _totalSize += size ;

   done:
      PD_TRACE_EXITRC( SDB__RTNSORTAREA__ALLOCBLOCK, rc ) ;
      return rc ;
   error:
      SAFE_OSS_DELETE( newBlock ) ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNSORTAREA__GETMAXBLOCK, "_rtnSortArea::_getMaxBlock" )
   rtnSortAreaBlock *_rtnSortArea::_getMaxBlock( BOOLEAN includeReserve ) const
   {
      PD_TRACE_ENTRY( SDB__RTNSORTAREA__GETMAXBLOCK ) ;
      rtnSortAreaBlock *block = NULL ;
      if ( !_activeBlocks.empty() )
      {
         BLOCK_LIST_CITR itr = _activeBlocks.begin() ;
         block = *itr ;
         while ( ++itr != _activeBlocks.end() &&
                 ( (*itr)->capacity() > block->capacity() ) )
         {
            block = *itr ;
         }
      }

      if ( includeReserve && _idleBlocks.empty() )
      {
         for ( BLOCK_MAP_CITR itr = _idleBlocks.begin();
               itr != _idleBlocks.end(); ++itr )
         {
            if ( itr->second->capacity() > block->capacity() )
            {
               block = itr->second ;
            }
         }
      }

      PD_TRACE_EXIT( SDB__RTNSORTAREA__GETMAXBLOCK ) ;
      return block ;
   }
}

