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

#ifndef RTN_SORTAREA_HPP__
#define RTN_SORTAREA_HPP__
#include "utilList.hpp"

namespace engine
{
   /**
    * @brief A continuous block of memory in the sort area.
    */
   class _rtnSortAreaBlock : public SDBObject
   {
      friend class _rtnSortArea ;
   public:
      _rtnSortAreaBlock( size_t size ) ;

      virtual ~_rtnSortAreaBlock();

      INT32 init() ;

      /**
       * @brief Insert data into the block in append mode.
       * @param[in] data - Data to insert.
       * @param[in] len - Data length.
       */
      INT32 append( const CHAR *data, size_t len ) ;

      /**
       * @brief Get address by offset of the block.
       * @param[in] offset - Offset in the block.
       * @return A valid pointer if offset is valid. Otherwise NULL will be
       * returned.
       * @note User can directly read from or write to any valid offset in the
       * block. In this case, the user should make sure not to corrupt the data
       * in the block by themselves.
       */
      CHAR *offset2Addr( size_t offset ) const ;

      size_t capacity() const ;

      /**
       * @brief Total size of data in the block.
       */
      size_t length() const ;

      size_t freeSize() const ;

      void reset() ;

   private:
      INT32 _resize( size_t newSize ) ;

   private:
      CHAR *_buff ;
      size_t _size ;
      size_t _writePos ;
   } ;
   typedef _rtnSortAreaBlock rtnSortAreaBlock ;

   /**
    * @brief Memory area for sorting. It manages some memory as blocks.
    * @note DO NOT share sort area among multiple threads for concurrent
    * sorting. One sort area for one sorting.
    */
   class _rtnSortArea : public SDBObject
   {
      typedef _utilList<rtnSortAreaBlock *> BLOCK_LIST ;
      typedef _utilList<rtnSortAreaBlock *>::iterator BLOCK_LIST_ITR ;
      typedef _utilList<rtnSortAreaBlock *>::const_iterator BLOCK_LIST_CITR ;
      typedef multimap<size_t, rtnSortAreaBlock *> BLOCK_MAP ;
      typedef multimap<size_t, rtnSortAreaBlock *>::iterator BLOCK_MAP_ITR ;
      typedef multimap<size_t, rtnSortAreaBlock *>::const_iterator BLOCK_MAP_CITR ;

   public:
      _rtnSortArea() ;
      virtual ~_rtnSortArea() ;

      /**
       * @brief Initialize the sort area.
       * @param[in] limit - Memory usage limit for the sort area.
       */
      INT32 init( size_t limit ) ;

      BOOLEAN hasInit() const ;

      /**
       * @brief Get a block from the sort area. The block maybe a new allocated
       * one, or a block which has been released and reserved by the sort area.
       * @param[int] size - Size of the block we want.
       * @param[out] block - The block being allocated. It will be NULL if
       * allocation failed.
       * @param[in] accurateSize - If we want the block to be exactly the size
       * we required. If it's FALSE, the allocated block's size may be equal to
       * or greater than size.
       */
      INT32 allocBlock( size_t size, rtnSortAreaBlock *&block,
                        BOOLEAN accurateSize = TRUE ) ;

      /**
       * @brief Change a block to new size.
       * @param[in] block - The block to be reallocated.
       * @param[in] newSize - New size for the block.
       */
      INT32 reallocBlock( rtnSortAreaBlock *block, size_t newSize ) ;

      /**
       * @brief Release a block which has been gotten from the sort area.
       * @param[in,out] block - The block to be released.
       * @param[in] reserve - Whether to release the block directly. Note that
       * the sort area will try to reserve, but if it can not be done, the block
       * will also be freed.
       */
      INT32 releaseBlock( rtnSortAreaBlock *&block, BOOLEAN reserve = FALSE ) ;

      /**
       * @brief Reset the sort area.
       * @param[in] reserveBlocks - Whether to reserve all the blocks which have
       * been allocated. If the option is false, the sort area will try to
       * reserve them. In case of error when reserving, the block will be freed
       * directly anyway.
       */
      void reset( BOOLEAN reserveBlocks = FALSE ) ;

      /**
       * @brief Get capacity of the sort area. It's the size when
       * initialization.
       */
      size_t capacity() const ;

      /**
       * @brief Get total size of all blocks which have been allocated,
       * including reserved ones.
       */
      size_t usedSpace() const ;

      /**
       * @brief Remaining size of the sort area before touch the limit.
       */
      size_t freeSpace() const ;

      /**
       * @brief Get the size of the largest block which has been allocated.
       * @param[in] includeReserve - Whether include reserved blocks.
       */
      size_t currentMaxBlockSize( BOOLEAN includeReserve = TRUE ) const ;

      /**
       * @brief Try to get a single block as large as possible.
       * @return The allocated block.
       * @note All blocks which have been allocated will be unavailable.
       */
      rtnSortAreaBlock* getMaxSingleBlock() ;

   private:
      INT32 _allocBlock( size_t size, rtnSortAreaBlock *&block ) ;
      rtnSortAreaBlock* _getMaxBlock( BOOLEAN includeReserve = TRUE ) const ;

   private:
      size_t _limit ;            // Space limit
      size_t _totalSize ;
      BLOCK_LIST _activeBlocks ; // Blocks currently being used.
      BLOCK_MAP _idleBlocks ; // Blocks which have been released but not freed.
   } ;
   typedef _rtnSortArea rtnSortArea ;
}

#endif /* RTN_SORTAREA_HPP__ */

