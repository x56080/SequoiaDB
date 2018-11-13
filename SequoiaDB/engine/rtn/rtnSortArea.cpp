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
#include <rtnSortArea.hpp>

#include "rtnSortArea.hpp"
#include "rtnTrace.hpp"
#include "pd.hpp"

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

   INT32 _rtnSortAreaBlock::init()
   {
      INT32 rc = SDB_OK ;

      _buff = (CHAR *)SDB_OSS_MALLOC( _size ) ;
      if ( !_buff )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Allocate memory for sort area block failed. "
                          "Requested size: %zd", _size ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnSortAreaBlock::append( const CHAR *data, size_t len )
   {
      INT32 rc = SDB_OK ;

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

   INT32 _rtnSortAreaBlock::_resize( size_t newSize )
   {
      INT32 rc = SDB_OK ;
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
      return rc ;
   error:
      goto done ;
   }

   _rtnSortArea::_rtnSortArea()
   : _limit(0),
     _totalSize(0)
   {
   }

   _rtnSortArea::~_rtnSortArea()
   {
      for ( BLOCK_LIST_ITR itr = _blocks.begin(); itr != _blocks.end(); ++itr )
      {
         if ( *itr )
         {
            SDB_OSS_DEL *itr ;
         }
      }
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

   INT32 _rtnSortArea::allocBlock( size_t size, rtnSortAreaBlock *&block )
   {
      INT32 rc = SDB_OK ;
      rtnSortAreaBlock *newBlock = NULL ;

      if ( !hasInit() )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Sort area not initialized yet" ) ;
         goto error ;
      }

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

      block = newBlock ;
      _totalSize += size ;
      _blocks.push_back( newBlock ) ;

   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( newBlock ) ;
      goto done ;
   }

   INT32 _rtnSortArea::reallocBlock( rtnSortAreaBlock *&block, size_t newSize )
   {
      INT32 rc = SDB_OK ;
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
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnSortArea::freeBlock( rtnSortAreaBlock *&block )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( block, "Block pointer to free is NULL" ) ;

      BLOCK_LIST_ITR itr = _blocks.begin() ;
      while ( itr != _blocks.end() )
      {
         if ( block == *itr )
         {
            break ;
         }
         ++itr ;
      }

      if ( _blocks.end() == itr )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Block to free is not allocated by this sort area" ) ;
         goto error ;
      }

      _blocks.erase( itr ) ;
      _totalSize -= block->capacity() ;

      SDB_OSS_DEL block ;
      block = NULL ;

   done:
      return rc ;
   error:
      goto done ;
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

   size_t _rtnSortArea::blockNum() const
   {
      return _blocks.size() ;
   }

   rtnSortAreaBlock *_rtnSortArea::getMaxBlock() const
   {
      rtnSortAreaBlock *block = NULL ;
      if ( !_blocks.empty() )
      {
         BLOCK_LIST_CITR itr = _blocks.begin() ;
         block = *itr ;
         while ( ++itr != _blocks.end() &&
                 ( (*itr)->capacity() > block->capacity() ) )
         {
            block = *itr ;
         }
      }

      return block ;
   }

   rtnSortAreaBlock *_rtnSortArea::getWholeArea()
   {
      INT32 rc = SDB_OK ;
      rtnSortAreaBlock *block = NULL ;

      if ( _blocks.empty() )
      {
         rc = allocBlock( _limit, block ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Allocate sort area block failed[%d]. Requested "
                             "size: %zd", rc, _limit ) ;
            goto error ;
         }
         _blocks.push_back( block ) ;
      }
      else
      {
         // Free all the blocks except the biggest one, and try to resize it
         // up to limit.
         BLOCK_LIST_ITR itr = _blocks.begin() ;
         block = *itr ;
         while ( ++itr != _blocks.end() )
         {
            if ( (*itr)->capacity() > block->capacity() )
            {
               SDB_OSS_DEL block ;
               block = *itr ;
            }
            else
            {
               SDB_OSS_DEL *itr ;
            }
         }

         _blocks.clear() ;
         _blocks.push_back( block ) ;

         rc = block->_resize( _limit ) ;
         if ( rc )
         {
            // Resize failed, the block will remain unchanged.
            PD_LOG( PDDEBUG, "Resize block to %zd failed[%d]", _limit, rc ) ;
         }
         block->reset() ;
      }

   done:
      return block ;
   error:
      goto done ;
   }
}

