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

   Source File Name = utilSPSCBlockQueue.hpp

   Descriptive Name = Single Producer Single Customer Block Queue

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/01/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef UTIL_SPSC_BLOCK_QUEUE_HPP__
#define UTIL_SPSC_BLOCK_QUEUE_HPP__

#include "oss.hpp"
#include "ossMemPool.hpp"
#include "ossTypes.h"
#include "pd.hpp"
#include "utilCircularQueue.hpp"
#include "utilPooledObject.hpp"

namespace engine
{

   /*
      _utilBlockList define
    */
   // block list is a list of fixed size buffers
   template< typename T, UINT32 blockSize >
   class _utilSPSCBlockQueue : public _utilPooledObject
   {
   protected:
      class _utilItem ;

      class _utilItemPtr : public ossAtomicPtr
      {
      public:
         _utilItemPtr()
         : ossAtomicPtr( (UINT64)NULL )
         {
         }

         ~_utilItemPtr() = default ;

         _utilItemPtr( const _utilItemPtr & ) = delete ;
         _utilItemPtr &operator =( const _utilItemPtr * ) = delete ;

         _utilItem *getPtr()
         {
            return (_utilItem *)( peek() ) ;
         }

         const _utilItem *getPtr() const
         {
            return (const _utilItem *)( peek() ) ;
         }

         _utilItem *syncGetPtr()
         {
            return (_utilItem *)( fetch() ) ;
         }

         void setPtr( _utilItem *ptr )
         {
            poke( (UINT64)ptr ) ;
         }

         void syncSetPtr( _utilItem *ptr )
         {
            swap( (UINT64)ptr ) ;
         }
      } ;

      class _utilItem : public _utilStackSPSCQueue< T, blockSize >
      {
      public:
         _utilItem() = default ;
         ~_utilItem() = default ;

         _utilItem( const _utilItem & ) = delete ;
         _utilItem &operator=( const _utilItem & ) = delete ;

      public:
         _utilItemPtr _next ;
      } ;

   public:
      _utilSPSCBlockQueue() = default ;

      ~_utilSPSCBlockQueue()
      {
         clear() ;
      }

      _utilSPSCBlockQueue( const _utilSPSCBlockQueue &other ) = delete ;
      _utilSPSCBlockQueue &operator =( const _utilSPSCBlockQueue &other ) = delete ;

   public:
      BOOLEAN isEmpty() const
      {
         return ( NULL == _tailer.getPtr() ) ||
                ( _tailer.getPtr()->isEmpty() ) ;
      }

      BOOLEAN push( T &&value )
      {
         _utilItem *curItem = _tailer.getPtr() ;
         if ( NULL != curItem &&
              curItem->push( value ) )
         {
            return TRUE ;
         }
         _utilItem *newItem = SDB_OSS_NEW _utilItem() ;
         if ( NULL == newItem )
         {
            // failed to allocate
            return FALSE ;
         }
         newItem->push( value ) ;
         if ( NULL != curItem )
         {
            curItem->_next.syncSetPtr( newItem ) ;
         }
         else
         {
            _header.syncSetPtr( newItem ) ;
         }
         _tailer.setPtr( newItem ) ;
         return TRUE ;
      }

      BOOLEAN pop( T &value )
      {
         _utilItem *curItem = _header.getPtr() ;
         if ( NULL == curItem )
         {
            return FALSE ;
         }
         else if ( curItem->pop( value ) )
         {
            return TRUE ;
         }
         _utilItem *nextItem = curItem->_next.syncGetPtr() ;
         if ( NULL == nextItem )
         {
            return FALSE ;
         }
         SDB_OSS_DEL curItem ;
         _header.setPtr( nextItem ) ;
         return nextItem->pop( value ) ;
      }

      void clear()
      {
         _utilItem *curItem = (_utilItem *)( _header.peek() ) ;
         while ( NULL != curItem )
         {
            _utilItem *tmpItem = curItem->_next.getPtr() ;
            SDB_OSS_DEL curItem ;
            curItem = tmpItem ;
         }
         _header.setPtr( NULL ) ;
         _tailer.setPtr( NULL ) ;
      }

   protected:
      _utilItemPtr _header ;
      _utilItemPtr _tailer ;
   } ;

}

#endif // UTIL_SPSC_BLOCK_QUEUE_HPP__
