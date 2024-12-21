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

   Source File Name = utilArray.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/01/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef UTIL_ARRAY_HPP_
#define UTIL_ARRAY_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "utilMemListPool.hpp"
#include "ossUtil.hpp"

#define UTIL_ARRAY_DEFAULT_SIZE        4

namespace engine
{
   template <typename T, UINT32 stackSize = UTIL_ARRAY_DEFAULT_SIZE >
   class _utilArray : public SDBObject
   {
   public:
      _utilArray( UINT32 size = 0 )
      :_dynamicBuf( _staticBuf ),
       _bufSize( stackSize ),
       _eleSize( 0 )
      {
         resize( size ) ;
      }

      ~_utilArray()
      {
         clear( TRUE ) ;
      }

   public:
      class iterator
      {
      public:
         iterator( const _utilArray<T,stackSize> &t )
         :_t( &t ),
          _now( 0 )
         {
         }

         ~iterator(){}

      public:
         BOOLEAN more() const
         {
            return _now < _t->_eleSize ;
         }

         BOOLEAN next( T &t )
         {
            if ( more() )
            {
               t = _t->_dynamicBuf[_now++] ;
               return TRUE ;
            }
            return FALSE ;
         }

      private:
         const _utilArray<T,stackSize> *_t ;
         UINT32 _now ;
      } ;

   public:
      OSS_INLINE const T &operator[]( UINT32 i ) const
      {
         return _dynamicBuf[ i ] ;
      }

      OSS_INLINE T &operator[]( UINT32 i )
      {
         return _dynamicBuf[ i ] ;
      }

      OSS_INLINE UINT32 size() const
      {
         return _eleSize ;
      }

      OSS_INLINE UINT32 capacity() const
      {
         return _bufSize ;
      }

      OSS_INLINE BOOLEAN empty() const
      {
         return 0 == _eleSize ;
      }

      OSS_INLINE void clear( BOOLEAN resetMem = TRUE )
      {
         if ( resetMem && _dynamicBuf != _staticBuf )
         {
            SDB_THREAD_FREE( _dynamicBuf ) ;
            _dynamicBuf = _staticBuf ;
            _bufSize = stackSize ;
         }
         _eleSize = 0 ;
      }

      OSS_INLINE INT32 append( const T &t )
      {
         INT32 rc = SDB_OK ;

         if ( _eleSize >= _bufSize )
         {
            /// resize by 2 mutiple
            rc = resize( _bufSize << 1 ) ;
            if ( rc )
            {
               goto error ;
            }
         }
         _dynamicBuf[ _eleSize++ ] = t ;

      done:
         return rc ;
      error:
         goto done ;
      }

      INT32 resize( UINT32 size, BOOLEAN copyData = TRUE )
      {
         INT32 rc = SDB_OK ;
         if ( size <= _bufSize )
         {
            goto done ;
         }
         else if ( _dynamicBuf == _staticBuf )
         {
            T* pTmp = (T*)SDB_THREAD_ALLOC( sizeof( T ) * size ) ;
            if ( !pTmp )
            {
               rc = SDB_OOM ;
               goto error ;
            }
            if( copyData)
            {
                /// copy data
                ossMemcpy( pTmp, _dynamicBuf, sizeof( T ) * _eleSize ) ;
            }
            /// set pointer
            _dynamicBuf = pTmp ;
            _bufSize = size ;
         }
         else
         {
            T *tmp = _dynamicBuf ;
            _dynamicBuf = (T*)SDB_THREAD_REALLOC( _dynamicBuf,
                                                  sizeof( T ) * size ) ;
            if ( NULL == _dynamicBuf )
            {
               _dynamicBuf = tmp ;
               rc = SDB_OOM ;
               goto error ;
            }
            _bufSize = size ;
         }

      done:
         return rc ;
      error:
         goto done ;
      }

      INT32 reserve( UINT32 size )
      {
         return resize( size, FALSE ) ;
      }

      OSS_INLINE _utilArray<T>& operator= ( const _utilArray<T> &rhs )
      {
         if ( SDB_OK == resize( rhs.size() ) )
         {
            ossMemcpy( _dynamicBuf, rhs._dynamicBuf, rhs.size() * sizeof(T) ) ;
            _eleSize = rhs.size() ;
         }
         return *this ;
      }

      OSS_INLINE INT32 copyTo( _utilArray<T> &rhs )
      {
         INT32 rc = SDB_OK ;

         rc = rhs.resize( size() ) ;
         if ( rc )
         {
            goto error ;
         }
         ossMemcpy( rhs._dynamicBuf, _dynamicBuf, size() * sizeof(T) ) ;
         rhs._eleSize = size() ;

      done:
         return rc ;
      error:
         goto done ;
      }

   protected:
      T _staticBuf[ stackSize ] ;
      T *_dynamicBuf ;
      UINT32 _bufSize ;
      UINT32 _eleSize ;
   } ;

   template <typename T, UINT32 SIZE, UINT32 stackSize = 1, UINT32 staticSize = 16 >
   class _utilSparseArray : public SDBObject
   {
      public:
         _utilSparseArray()
         {
            _pSlotEx = NULL ;

            /// for static assert
            UINT32 __testArray[ ( SIZE >= staticSize ? 1 : -1 ) ] = { 0 } ;

            for ( UINT32 i = 0 ; i < staticSize ; ++i )
            {
               _slot[ i ] = NULL ;
            }
            for ( UINT32 i = 0 ; i < stackSize ; ++i )
            {
               _slotStackFlag[ i ] = __testArray[ 0 ] ;
            }
         }

         ~_utilSparseArray()
         {
            _release() ;
         }

         OSS_INLINE const T &operator[]( UINT32 i ) const
         {
            if ( i < staticSize && _slot[ i ] )
            {
               return *(_slot[ i ]) ;
            }
            else if ( i >= SIZE )
            {
               /// out of range
               SDB_ASSERT( FALSE, "out of the array size" ) ;
               throw pdGeneralException( SDB_SYS, "out of the array size" ) ;
            }
            else if ( i >= staticSize && _pSlotEx && _pSlotEx[ i - staticSize ] )
            {
               return *(_pSlotEx[ i - staticSize ]) ;
            }
            else
            {
               /// not init the slot
               // SDB_ASSERT( FALSE, "not alloc the slot" ) ;
               return _slotDefault ;
            }
         }

         OSS_INLINE T &operator[]( UINT32 i )
         {
            if ( i < staticSize && _slot[ i ] )
            {
               return *(_slot[ i ]) ;
            }
            else if ( i >= SIZE )
            {
               /// out of range
               SDB_ASSERT( FALSE, "out of the array size" ) ;
               throw pdGeneralException( SDB_SYS, "out of the array size" ) ;
            }
            else if ( i >= staticSize && _pSlotEx && _pSlotEx[ i - staticSize ] )
            {
               return *(_pSlotEx[ i - staticSize ]) ;
            }
            else
            {
               /// not init the slot
               // SDB_ASSERT( FALSE, "not alloc the slot" ) ;
               return _slotDefault ;
            }
         }

         INT32 allocSlot( UINT32 i )
         {
            INT32 rc = SDB_OK ;

            if ( i >= SIZE )
            {
               rc = SDB_SYS ;
               SDB_ASSERT( FALSE, "out of the array size" ) ;
               goto error ;
            }
            else if ( ( i < staticSize && _slot[ i ] ) ||
                      ( i >= staticSize && _pSlotEx && _pSlotEx[ i - staticSize ] ) )
            {
               /// already alloc
            }
            else
            {
               T *obj = NULL ;

               if ( i >= staticSize && !_pSlotEx )
               {
                  ossScopedLock lock( &_latch ) ;

                  /// double check
                  if ( !_pSlotEx )
                  {
                     _pSlotEx = new (std::nothrow) T* [ SIZE - staticSize ] ;
                     if ( !_pSlotEx )
                     {
                        rc = SDB_OOM ;
                        PD_LOG( PDERROR, "Allocate slot failed" ) ;
                        goto error ;
                     }

                     /// init
                     for ( UINT32 i = 0 ; i < SIZE - staticSize ; ++i )
                     {
                        _pSlotEx[ i ] = NULL ;
                     }
                  }
               }

               obj = _allocFromStack() ;
               if ( !obj )
               {
                  obj = SDB_OSS_NEW T() ;
                  if ( !obj )
                  {
                     rc = SDB_OOM ;
                     PD_LOG( PDERROR, "Allocate memory failed, size: %u", sizeof( T ) ) ;
                     goto error ;
                  }
               }

               if ( i < staticSize )
               {
                  _slot[ i ] = obj ;
               }
               else
               {
                  _pSlotEx[ i - staticSize ] = obj ;
               }
            }

         done:
            return rc ;
         error:
            goto done ;
         }

         void  releaseSlot( UINT32 i )
         {
            if ( i >= SIZE )
            {
               SDB_ASSERT( FALSE, "out of the array size" ) ;
            }
            else if ( ( i < staticSize && !_slot[i] ) ||
                      ( i >= staticSize && ( !_pSlotEx || !_pSlotEx[ i - staticSize ] ) ) )
            {
               /// not alloc
               SDB_ASSERT( FALSE, "not alloc the slot" ) ;
            }
            else
            {
               INT32 i = -1 ;
               if ( i < staticSize )
               {
                  i = _isInStack( _slot[i] ) ;
                  if ( -1 == i )
                  {
                     SDB_OSS_DEL _slot[i] ;
                  }
                  _slot[i] = NULL ;
               }
               else
               {
                  i = _isInStack( _pSlotEx[ i - staticSize ] ) ;
                  if ( -1 == i )
                  {
                     SDB_OSS_DEL _pSlotEx[ i - staticSize ] ;
                  }
                  _pSlotEx[ i - staticSize ] = NULL ;
               }

               if ( -1 != i )
               {
                  _slotStackFlag[i] = 0 ;
               }
            }
         }

      protected:
         INT32 _isInStack( const T* slot ) const
         {
            for ( UINT32 i = 0 ; i < stackSize ; ++i )
            {
               if ( slot == &(_slotStack[i]) )
               {
                  return i ;
               }
            }
            return -1 ;
         }

         void _release()
         {
            for ( UINT32 i = 0 ; i < SIZE ; ++i )
            {
               if ( i < staticSize )
               {
                  if ( _slot[i] && -1 == _isInStack( _slot[i] ) )
                  {
                     SDB_OSS_DEL _slot[i] ;
                  }
                  _slot[i] = NULL ;
               }
               else if ( _pSlotEx )
               {
                  if ( _pSlotEx[ i - staticSize ] &&
                       -1 == _isInStack( _pSlotEx[ i - staticSize ] ) )
                  {
                     SDB_OSS_DEL _pSlotEx[ i - staticSize ] ;
                  }
                  _pSlotEx[ i - staticSize ] = NULL ;
               }
            }

            for ( UINT32 i = 0 ; i < stackSize ; ++i )
            {
               _slotStackFlag[ i ] = 0 ;
            }

            if ( _pSlotEx )
            {
               delete [] _pSlotEx ;
               _pSlotEx = NULL ;
            }
         }

         T* _allocFromStack()
         {
            for ( UINT32 i = 0 ; i < stackSize ; ++i )
            {
               if ( 0 == _slotStackFlag[ i ] )
               {
                  _slotStackFlag[ i ] = 1 ;
                  return &_slotStack[ i ] ;
               }
            }

            return NULL ;
         }

      protected:
         T                 _slotStack[ stackSize ] ;
         T                 _slotDefault ;
         T                *_slot[ staticSize ] ;
         T               **_pSlotEx ;
         BYTE              _slotStackFlag[ stackSize ] ;
         ossSpinXLatch     _latch ;
   };

}

#endif // UTIL_ARRAY_HPP_

