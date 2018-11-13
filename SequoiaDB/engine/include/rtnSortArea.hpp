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
   class _rtnSortAreaBlock : public SDBObject
   {
      friend class _rtnSortArea ;
   public:
      _rtnSortAreaBlock( size_t size ) ;

      virtual ~_rtnSortAreaBlock();

      INT32 init() ;

      INT32 append( const CHAR *data, size_t len ) ;

      // Note:
      // User can directly read from or write to any valid offset in the block.
      // In this case, the user should make sure not to corrupt the data in the
      // block by themselves.
      CHAR *offset2Addr( size_t offset ) const ;

      size_t capacity() const ;
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

   /*
    * Memory area for sorting. It manages memory in blocks.
    */
   class _rtnSortArea : public SDBObject
   {
      typedef _utilList<rtnSortAreaBlock *> BLOCK_LIST ;
      typedef _utilList<rtnSortAreaBlock *>::iterator BLOCK_LIST_ITR ;
      typedef _utilList<rtnSortAreaBlock *>::const_iterator BLOCK_LIST_CITR ;
   public:
      _rtnSortArea() ;
      virtual ~_rtnSortArea() ;

      INT32 init( size_t limit ) ;
      BOOLEAN hasInit() const ;

      INT32 allocBlock( size_t size, rtnSortAreaBlock *&block ) ;
      INT32 reallocBlock( rtnSortAreaBlock *&block, size_t newSize ) ;
      INT32 freeBlock( rtnSortAreaBlock *&block ) ;

      size_t capacity() const ;
      size_t usedSpace() const ;
      size_t freeSpace() const ;
      size_t blockNum() const ;

      // Get the largest block which has been allocated in the area.
      rtnSortAreaBlock* getMaxBlock() const ;

      // Get memory as large as possible(not exceeding the limit) as one whole
      // block. This may free all the current blocks.
      rtnSortAreaBlock* getWholeArea() ;

   private:
      size_t _limit ;   // Space limit
      size_t _totalSize ;
      BLOCK_LIST _blocks ;
   } ;
   typedef _rtnSortArea rtnSortArea ;
}

#endif /* RTN_SORTAREA_HPP__ */

