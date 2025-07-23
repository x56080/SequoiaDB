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

   Source File Name = utilMemBlockPool.cpp

   Descriptive Name = Data Protection Services Types Header

   When/how to use: this program may be used on binary and text-formatted
   versions of dps component. This file contains declare for data types used in
   SequoiaDB.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/04/2019  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#include "utilMemBlockPool.hpp"
#include "pd.hpp"

namespace engine
{

   #define UTIL_MEM_ALLOC_MAX_TRY_LEVEL         ( 3 )

   /*
      _utilMemBlockPool implement
   */
   _utilMemBlockPool::_utilMemBlockPool()
   :_numDynamicAlloc16B( 0 ),
    _numDynamicAlloc32B( 0 ),
    _numDynamicAlloc64B( 0 ),
    _numDynamicAlloc128B( 0 ),
    _numDynamicAlloc256B( 0 ),
    _numDynamicAlloc512B( 0 ),
    _numDynamicAlloc1K( 0 ),
    _numDynamicAlloc2K( 0 ),
    _numDynamicAlloc4K( 0 ),
    _maxSize( 0 ),
    _totalSize( 0 )
   {
      _16BSeg = NULL ;
      _32BSeg = NULL ;
      _64BSeg = NULL ;
      _128BSeg = NULL ;
      _256BSeg = NULL ;
      _512BSeg = NULL ;
      _1KSeg = NULL ;
      _2KSeg = NULL ;
      _4KSeg = NULL ;
   }

   _utilMemBlockPool::~_utilMemBlockPool()
   {
      fini() ;
   }

   INT32 _utilMemBlockPool::init( UINT64 maxSize )
   {
      INT32 rc = SDB_OK ;
      UINT32 smallBlockSize = UTIL_MEM_A_SMALL_BLOCK_SIZE *
                              UTIL_MEM_A_SMALL_BLOCK_SUBPOOL_NUM ;
      UINT32 midBlockSize = UTIL_MEM_A_MID_BLOCK_SIZE *
                            UTIL_MEM_A_MID_BLOCK_SUBPOOL_NUM ;
      UINT32 bigBlockSize = UTIL_MEM_A_BIG_BLOCK_SIZE *
                            UTIL_MEM_A_BIG_BLOCK_SUBPOOL_NUM ;
      UINT64 aBlockMaxSize = 0 ;

      if ( 0 != _maxSize )
      {
         rc = SDB_INVALIDARG ;
      }
      else
      {
         _maxSize = maxSize ;
      }

      if ( 0 == _maxSize )
      {
         goto done ;
      }
      aBlockMaxSize = _maxSize >> 2 ;

      /// when alloc or init failed, ignored
      _16BSeg = SDB_OSS_NEW _utilSegmentManager<element16B>() ;
      if ( _16BSeg )
      {
         UINT32 typeSize = _type2Size( MEMBLOCKPOOL_TYPE_16 ) ;
         rc = _16BSeg->init( smallBlockSize / typeSize,
                             aBlockMaxSize / typeSize,
                             UTIL_MEM_A_SMALL_BLOCK_SUBPOOL_NUM,
                             this ) ;
         if ( rc )
         {
            SDB_OSS_DEL _16BSeg ;
            _16BSeg = NULL ;
            rc = SDB_OK ;
         }
      }

      _32BSeg = SDB_OSS_NEW _utilSegmentManager<element32B>() ;
      if ( _32BSeg )
      {
         UINT32 typeSize = _type2Size( MEMBLOCKPOOL_TYPE_32 ) ;
         rc = _32BSeg->init( smallBlockSize / typeSize,
                             aBlockMaxSize / typeSize,
                             UTIL_MEM_A_SMALL_BLOCK_SUBPOOL_NUM,
                             this ) ;
         if ( rc )
         {
            SDB_OSS_DEL _32BSeg ;
            _32BSeg = NULL ;
            rc = SDB_OK ;
         }
      }

      _64BSeg = SDB_OSS_NEW _utilSegmentManager<element64B>() ;
      if ( _64BSeg )
      {
         UINT32 typeSize = _type2Size( MEMBLOCKPOOL_TYPE_64 ) ;
         rc = _64BSeg->init( smallBlockSize / typeSize,
                             aBlockMaxSize / typeSize,
                             UTIL_MEM_A_SMALL_BLOCK_SUBPOOL_NUM,
                             this ) ;
         if ( rc )
         {
            SDB_OSS_DEL _64BSeg ;
            _64BSeg = NULL ;
            rc = SDB_OK ;
         }
      }

      _128BSeg = SDB_OSS_NEW _utilSegmentManager<element128B>() ;
      if ( _128BSeg )
      {
         UINT32 typeSize = _type2Size( MEMBLOCKPOOL_TYPE_128 ) ;
         rc = _128BSeg->init( midBlockSize / typeSize,
                              aBlockMaxSize / typeSize,
                              UTIL_MEM_A_MID_BLOCK_SUBPOOL_NUM,
                              this ) ;
         if ( rc )
         {
            SDB_OSS_DEL _128BSeg ;
            _128BSeg = NULL ;
            rc = SDB_OK ;
         }
      }

      _256BSeg = SDB_OSS_NEW _utilSegmentManager<element256B>() ;
      if ( _256BSeg )
      {
         UINT32 typeSize = _type2Size( MEMBLOCKPOOL_TYPE_256 ) ;
         rc = _256BSeg->init( midBlockSize / typeSize,
                              aBlockMaxSize / typeSize,
                              UTIL_MEM_A_MID_BLOCK_SUBPOOL_NUM,
                              this ) ;
         if ( rc )
         {
            SDB_OSS_DEL _256BSeg ;
            _256BSeg = NULL ;
            rc = SDB_OK ;
         }
      }

      _512BSeg = SDB_OSS_NEW _utilSegmentManager<element512B>() ;
      if ( _512BSeg )
      {
         UINT32 typeSize = _type2Size( MEMBLOCKPOOL_TYPE_512 ) ;
         rc = _512BSeg->init( midBlockSize / typeSize,
                              aBlockMaxSize / typeSize,
                              UTIL_MEM_A_MID_BLOCK_SUBPOOL_NUM,
                              this ) ;
         if ( rc )
         {
            SDB_OSS_DEL _512BSeg ;
            _512BSeg = NULL ;
            rc = SDB_OK ;
         }
      }

      _1KSeg = SDB_OSS_NEW _utilSegmentManager<element1K>() ;
      if ( _1KSeg )
      {
         UINT32 typeSize = _type2Size( MEMBLOCKPOOL_TYPE_1024 ) ;
         rc = _1KSeg->init( bigBlockSize / typeSize,
                            aBlockMaxSize / typeSize,
                            UTIL_MEM_A_BIG_BLOCK_SUBPOOL_NUM,
                            this ) ;
         if ( rc )
         {
            SDB_OSS_DEL _1KSeg ;
            _1KSeg = NULL ;
            rc = SDB_OK ;
         }
      }

      _2KSeg = SDB_OSS_NEW _utilSegmentManager<element2K>() ;
      if ( _2KSeg )
      {
         UINT32 typeSize = _type2Size( MEMBLOCKPOOL_TYPE_2048 ) ;
         rc = _2KSeg->init( bigBlockSize / typeSize,
                            aBlockMaxSize / typeSize,
                            UTIL_MEM_A_BIG_BLOCK_SUBPOOL_NUM,
                            this ) ;
         if ( rc )
         {
            SDB_OSS_DEL _2KSeg ;
            _2KSeg = NULL ;
            rc = SDB_OK ;
         }
      }

      _4KSeg = SDB_OSS_NEW _utilSegmentManager<element4K>() ;
      if ( _4KSeg )
      {
         UINT32 typeSize = _type2Size( MEMBLOCKPOOL_TYPE_4096 ) ;
         rc = _4KSeg->init( bigBlockSize / typeSize,
                            aBlockMaxSize / typeSize,
                            UTIL_MEM_A_BIG_BLOCK_SUBPOOL_NUM,
                            this ) ;
         if ( rc )
         {
            SDB_OSS_DEL _4KSeg ;
            _4KSeg = NULL ;
            rc = SDB_OK ;
         }
      }

   done:
      return rc ;
   }

   void _utilMemBlockPool::fini()
   {
      if ( _16BSeg )
      {
         SDB_OSS_DEL _16BSeg ;
         _16BSeg = NULL ;
      }
      if ( _32BSeg )
      {
         SDB_OSS_DEL _32BSeg ;
         _32BSeg = NULL ;
      }
      if ( _64BSeg )
      {
         SDB_OSS_DEL _64BSeg ;
         _64BSeg = NULL ;
      }
      if ( _128BSeg )
      {
         SDB_OSS_DEL _128BSeg ;
         _128BSeg = NULL ;
      }
      if ( _256BSeg )
      {
         SDB_OSS_DEL _256BSeg ;
         _256BSeg = NULL ;
      }
      if ( _512BSeg )
      {
         SDB_OSS_DEL _512BSeg ;
         _512BSeg = NULL ;
      }
      if ( _1KSeg )
      {
         SDB_OSS_DEL _1KSeg ;
         _1KSeg = NULL ;
      }
      if ( _2KSeg )
      {
         SDB_OSS_DEL _2KSeg ;
         _2KSeg = NULL ;
      }
      if ( _4KSeg )
      {
         SDB_OSS_DEL _4KSeg ;
         _4KSeg = NULL ;
      }
   }

   void _utilMemBlockPool::shrink()
   {
      UINT64 hasFreeSize = 0 ;
      UINT32 freeSegNum = 0 ;

      if ( _16BSeg )
      {
         _16BSeg->shrink( 1, &freeSegNum ) ;
         hasFreeSize += ( freeSegNum * _type2Size( MEMBLOCKPOOL_TYPE_16 ) ) ;
      }
      if ( _32BSeg )
      {
         _32BSeg->shrink( 1, &freeSegNum ) ;
         hasFreeSize += ( freeSegNum * _type2Size( MEMBLOCKPOOL_TYPE_32 ) ) ;
      }
      if ( _64BSeg )
      {
         _64BSeg->shrink( 1, &freeSegNum ) ;
         hasFreeSize += ( freeSegNum * _type2Size( MEMBLOCKPOOL_TYPE_64 ) ) ;
      }
      if ( _128BSeg )
      {
         _128BSeg->shrink( 1, &freeSegNum ) ;
         hasFreeSize += ( freeSegNum * _type2Size( MEMBLOCKPOOL_TYPE_128 ) ) ;
      }
      if ( _256BSeg )
      {
         _256BSeg->shrink( 1, &freeSegNum ) ;
         hasFreeSize += ( freeSegNum * _type2Size( MEMBLOCKPOOL_TYPE_256 ) ) ;
      }
      if ( _512BSeg )
      {
         _512BSeg->shrink( 1, &freeSegNum ) ;
         hasFreeSize += ( freeSegNum * _type2Size( MEMBLOCKPOOL_TYPE_512 ) ) ;
      }
      if ( _1KSeg )
      {
         _1KSeg->shrink( 0, &freeSegNum ) ;
         hasFreeSize += ( freeSegNum * _type2Size( MEMBLOCKPOOL_TYPE_1024 ) ) ;
      }
      if ( _2KSeg )
      {
         _2KSeg->shrink( 0, &freeSegNum ) ;
         hasFreeSize += ( freeSegNum * _type2Size( MEMBLOCKPOOL_TYPE_2048 ) ) ;
      }
      if ( _4KSeg )
      {
         _4KSeg->shrink( 0, &freeSegNum ) ;
         hasFreeSize += ( freeSegNum * _type2Size( MEMBLOCKPOOL_TYPE_4096 ) ) ;
      }

      if ( hasFreeSize > 0 )
      {
         PD_LOG( PDINFO, "Has freed %llu bytes pooled memory, Total:%llu",
                 hasFreeSize, getTotalSize() ) ;
      }
   }

   UINT64 _utilMemBlockPool::getTotalSize()
   {
      return _totalSize.fetch() ;
   }

   _utilMemBlockPool::MEMBLOCKPOOL_TYPE
      _utilMemBlockPool::_size2MemType( UINT32 size, ossAtomic64 **ppCount )
   {
      if ( size <= 16 )
      {
         *ppCount = &_numDynamicAlloc16B ;
         return MEMBLOCKPOOL_TYPE_16 ;
      }
      else if ( size <= 32 )
      {
         *ppCount = &_numDynamicAlloc32B ;
         return MEMBLOCKPOOL_TYPE_32 ;
      }
      else if ( size <= 64 )
      {
         *ppCount = &_numDynamicAlloc64B ;
         return MEMBLOCKPOOL_TYPE_64 ;
      }
      else if ( size <= 128 )
      {
         *ppCount = &_numDynamicAlloc128B ;
         return MEMBLOCKPOOL_TYPE_128 ;
      }
      else if ( size <= 256 )
      {
         *ppCount = &_numDynamicAlloc256B ;
         return MEMBLOCKPOOL_TYPE_256 ;
      }
      else if ( size <= 512 )
      {
         *ppCount = &_numDynamicAlloc512B ;
         return MEMBLOCKPOOL_TYPE_512 ;
      }
      else if ( size <= 1024 )
      {
         *ppCount = &_numDynamicAlloc1K ;
         return MEMBLOCKPOOL_TYPE_1024 ;
      }
      else if ( size <= 2048 )
      {
         *ppCount = &_numDynamicAlloc2K ;
         return MEMBLOCKPOOL_TYPE_2048 ;
      }
      else if ( size <= 4096 )
      {
         *ppCount = &_numDynamicAlloc4K ;
         return MEMBLOCKPOOL_TYPE_4096 ;
      }
      return MEMBLOCKPOOL_TYPE_DYN ;
   }

   UINT32 _utilMemBlockPool::_type2Size( MEMBLOCKPOOL_TYPE type ) const
   {
      UINT32 size = 0 ;

      switch ( type )
      {
         case MEMBLOCKPOOL_TYPE_16 :
            size = 16 ;
            break ;
         case MEMBLOCKPOOL_TYPE_32 :
            size = 32 ;
            break ;
         case MEMBLOCKPOOL_TYPE_64 :
            size = 64 ;
            break ;
         case MEMBLOCKPOOL_TYPE_128 :
            size = 128 ;
            break ;
         case MEMBLOCKPOOL_TYPE_256 :
            size = 256 ;
            break ;
         case MEMBLOCKPOOL_TYPE_512 :
            size = 512 ;
            break ;
         case MEMBLOCKPOOL_TYPE_1024 :
            size = 1024 ;
            break ;
         case MEMBLOCKPOOL_TYPE_2048 :
            size = 2048 ;
            break ;
         case MEMBLOCKPOOL_TYPE_4096 :
            size = 4096 ;
            break ;
         default :
            break ;
      }
      return size ;
   }

   void* _utilMemBlockPool::alloc( UINT32 size )
   {
      MEMBLOCKPOOL_TYPE type  = MEMBLOCKPOOL_TYPE_MAX ;
      UINT32 realSize   = 0 ;
      MEMBLOCKPOOL_TYPE realType = MEMBLOCKPOOL_TYPE_MAX ;
      CHAR * ptr        = NULL ;
      void * userPtr    = NULL ;
      UINT32 tryLevel   = 0 ;
      ossAtomic64 *pNum = NULL ;

      if ( 0 == size )
      {
         goto done ;
      }

      realSize = UTIL_MEM_SIZE_2_REALSIZE( size ) ;
      type = _size2MemType( realSize, &pNum ) ;

      switch ( type )
      {
         case MEMBLOCKPOOL_TYPE_16 :
            if ( _16BSeg && SDB_OK == _16BSeg->acquire( (element16B*&)ptr ) )
            {
               realType = MEMBLOCKPOOL_TYPE_16 ;
               break ;
            }
            if ( ++tryLevel >= UTIL_MEM_ALLOC_MAX_TRY_LEVEL )
            {
               break ;
            }
            /// don't break
         case MEMBLOCKPOOL_TYPE_32 :
            if ( _32BSeg && SDB_OK == _32BSeg->acquire( (element32B*&)ptr ) )
            {
               realType = MEMBLOCKPOOL_TYPE_32 ;
               break ;
            }
            if ( ++tryLevel >= UTIL_MEM_ALLOC_MAX_TRY_LEVEL )
            {
               break ;
            }
            /// don't break
         case MEMBLOCKPOOL_TYPE_64 :
            if ( _64BSeg && SDB_OK == _64BSeg->acquire( (element64B*&)ptr ) )
            {
               realType = MEMBLOCKPOOL_TYPE_64 ;
               break ;
            }
            if ( ++tryLevel >= UTIL_MEM_ALLOC_MAX_TRY_LEVEL )
            {
               break ;
            }
            /// don't break
         case MEMBLOCKPOOL_TYPE_128 :
            if ( _128BSeg && SDB_OK == _128BSeg->acquire( (element128B*&)ptr ) )
            {
               realType = MEMBLOCKPOOL_TYPE_128 ;
               break ;
            }
            if ( ++tryLevel >= UTIL_MEM_ALLOC_MAX_TRY_LEVEL )
            {
               break ;
            }
            /// don't break
         case MEMBLOCKPOOL_TYPE_256 :
            if ( _256BSeg && SDB_OK == _256BSeg->acquire( (element256B*&)ptr ) )
            {
               realType = MEMBLOCKPOOL_TYPE_256 ;
               break ;
            }
            if ( ++tryLevel >= UTIL_MEM_ALLOC_MAX_TRY_LEVEL )
            {
               break ;
            }
            /// don't break
         case MEMBLOCKPOOL_TYPE_512 :
            if ( _512BSeg && SDB_OK == _512BSeg->acquire( (element512B*&)ptr ) )
            {
               realType = MEMBLOCKPOOL_TYPE_512 ;
               break ;
            }
            if ( ++tryLevel >= UTIL_MEM_ALLOC_MAX_TRY_LEVEL )
            {
               break ;
            }
            /// don't break
         case MEMBLOCKPOOL_TYPE_1024 :
            if ( _1KSeg && SDB_OK == _1KSeg->acquire( (element1K*&)ptr ) )
            {
               realType = MEMBLOCKPOOL_TYPE_1024 ;
               break ;
            }
            if ( ++tryLevel >= UTIL_MEM_ALLOC_MAX_TRY_LEVEL )
            {
               break ;
            }
            /// don't break
         case MEMBLOCKPOOL_TYPE_2048 :
            if ( _2KSeg && SDB_OK == _2KSeg->acquire( (element2K*&)ptr ) )
            {
               realType = MEMBLOCKPOOL_TYPE_2048 ;
               break ;
            }
            if ( ++tryLevel >= UTIL_MEM_ALLOC_MAX_TRY_LEVEL )
            {
               break ;
            }
            /// don't break
         case MEMBLOCKPOOL_TYPE_4096 :
            if ( _4KSeg && SDB_OK == _4KSeg->acquire( (element4K*&)ptr ) )
            {
               realType = MEMBLOCKPOOL_TYPE_4096 ;
               break ;
            }
            if ( ++tryLevel >= UTIL_MEM_ALLOC_MAX_TRY_LEVEL )
            {
               break ;
            }
            /// don't break
         default :
            break ;
      }

      if ( !ptr )
      {
         ptr = (CHAR*)SDB_OSS_MALLOC( realSize ) ;
         if ( !ptr )
         {
            goto done ;
         }
         realType = MEMBLOCKPOOL_TYPE_DYN ;
         if ( pNum )
         {
            pNum->inc() ;
         }
      }
      else
      {
         realSize = _type2Size( realType ) ;
         SDB_ASSERT( realSize > 0, "Real size is invalid" ) ;
      }

      _fillPtr( ptr, (UINT16)realType, realSize ) ;
      /// set user ptr
      userPtr = (void*)UTIL_MEM_PTR_2_USERPTR( ptr ) ;

   done :
      return userPtr ;
   }

   void _utilMemBlockPool::_fillPtr( CHAR * ptr, UINT16 type, UINT32 size )
   {
      /// set b-eye
      *UTIL_MEM_PTR_B_EYE_PTR( ptr ) = UTIL_MEM_B_EYE_CHAR ;
      /// set e-eye
      *UTIL_MEM_PTR_E_EYE_PTR( ptr, size ) = UTIL_MEM_E_EYE_CHAR ;
      /// set size
      *UTIL_MEM_PTR_SIZE_PTR( ptr ) = size ;
      /// set type
      *UTIL_MEM_PTR_TYPE_PTR( ptr ) = type ;
   }

   BOOLEAN _utilMemBlockPool::_checkAndExtract( const CHAR *ptr,
                                                UINT32 *pUserSize,
                                                UINT16 *pType ) const
   {
      BOOLEAN valid = TRUE ;

      if ( UTIL_MEM_B_EYE_CHAR != *UTIL_MEM_PTR_B_EYE_PTR( ptr ) )
      {
         SDB_ASSERT( FALSE, "Invalid b-eye" ) ;
         valid = FALSE ;
      }
      else
      {
         UINT16 type = *UTIL_MEM_PTR_TYPE_PTR( ptr ) ;
         UINT32 size = *UTIL_MEM_PTR_SIZE_PTR( ptr ) ;

         if ( type < MEMBLOCKPOOL_TYPE_DYN || type >= MEMBLOCKPOOL_TYPE_MAX )
         {
            SDB_ASSERT( FALSE, "Invalid type" ) ;
            valid = FALSE ;
         }
         else if ( size < UTIL_MEM_TOTAL_FILL_LEN )
         {
            SDB_ASSERT( FALSE, "Invalid size" ) ;
            valid = FALSE ;
         }
         else if ( MEMBLOCKPOOL_TYPE_DYN != type &&
                   size != _type2Size( (MEMBLOCKPOOL_TYPE)type ) )
         {
            SDB_ASSERT( FALSE, "Invalid size" ) ;
            valid = FALSE ;
         }
         else if ( UTIL_MEM_E_EYE_CHAR != *UTIL_MEM_PTR_E_EYE_PTR( ptr, size ) )
         {
            SDB_ASSERT( FALSE, "Invalid e-eye" ) ;
            valid = FALSE ;
         }
         else
         {
            if ( pUserSize )
            {
               *pUserSize = size - UTIL_MEM_TOTAL_FILL_LEN ;
            }
            if ( pType )
            {
               *pType = type ;
            }
         }
      }

      return valid ;
   }

   void* _utilMemBlockPool::realloc( void *ptr, UINT32 size )
   {
      void  *newUserPtr = NULL ;
      UINT32 oldUserSize = 0 ;

      if ( 0 == size )
      {
         newUserPtr = ptr ;
         goto done ;
      }

      if ( ptr )
      {
         UINT16 oldType = MEMBLOCKPOOL_TYPE_MAX ;
         CHAR *oldRealPtr = UTIL_MEM_USERPTR_2_PTR( ptr ) ;
         if ( !_checkAndExtract( oldRealPtr, &oldUserSize, &oldType ) )
         {
            newUserPtr = SDB_OSS_REALLOC( ptr, size ) ;
            goto done ;
         }
         if ( oldUserSize >= size )
         {
            newUserPtr = ptr ;
            goto done ;
         }
         else if ( MEMBLOCKPOOL_TYPE_DYN == oldType )
         {
            CHAR *newPtr = NULL ;
            UINT32 newRealSize = UTIL_MEM_SIZE_2_REALSIZE( size ) ;
            newPtr = (CHAR*)SDB_OSS_REALLOC( oldRealPtr, newRealSize ) ;
            if ( newPtr )
            {
               _fillPtr( newPtr, MEMBLOCKPOOL_TYPE_DYN, newRealSize ) ;
               newUserPtr = (void*)UTIL_MEM_PTR_2_USERPTR( newPtr ) ;
            }
            goto done ;
         }
      }

      newUserPtr = alloc( size ) ;
      if ( !newUserPtr )
      {
         goto done ;
      }
      if ( ptr )
      {
         ossMemcpy( newUserPtr, ptr, oldUserSize ) ;
         release( ptr ) ;
      }

   done:
      return newUserPtr ;
   }

   void _utilMemBlockPool::release( void *&ptr )
   {
      INT32 rc       = SDB_OK ;
      UINT16 type    = MEMBLOCKPOOL_TYPE_MAX ;
      CHAR *realPtr  = NULL ;

      if ( NULL == ptr )
      {
         goto done ;
      }

      realPtr = UTIL_MEM_USERPTR_2_PTR( ptr ) ;
      if ( !_checkAndExtract( realPtr, NULL, &type ) )
      {
         SDB_OSS_FREE( ptr ) ;
         ptr = NULL ;
         goto done ;
      }

      switch (type) 
      {
         case MEMBLOCKPOOL_TYPE_16 :
            rc = _16BSeg->release( (element16B *)realPtr ) ;
            break ;
         case MEMBLOCKPOOL_TYPE_32 :
            rc = _32BSeg->release( (element32B *)realPtr ) ;
            break ;
         case MEMBLOCKPOOL_TYPE_64 :
            rc = _64BSeg->release( (element64B *)realPtr ) ;
            break ;
         case MEMBLOCKPOOL_TYPE_128:
            rc = _128BSeg->release( (element128B *)realPtr ) ;
            break ;
         case MEMBLOCKPOOL_TYPE_256:
            rc = _256BSeg->release( (element256B *)realPtr) ;
            break ;
         case MEMBLOCKPOOL_TYPE_512:
            rc = _512BSeg->release( (element512B *)realPtr ) ;
            break ;
         case MEMBLOCKPOOL_TYPE_1024:
            rc = _1KSeg->release( (element1K *)realPtr ) ;
            break ;
         case MEMBLOCKPOOL_TYPE_2048:
            rc = _2KSeg->release( (element2K *)realPtr ) ;
            break ;
         case MEMBLOCKPOOL_TYPE_4096:
            rc = _4KSeg->release( (element4K *)realPtr ) ;
            break ;
         case MEMBLOCKPOOL_TYPE_DYN :
            SDB_OSS_FREE( realPtr ) ;
            break ;
         default:
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Invalid type (%d)", type ) ;
            SDB_ASSERT( FALSE, "Invalid arguments" ) ;
            break ;
      }

      SDB_ASSERT( ( SDB_OK == rc ), "Sever error during release" ) ;
   
      if ( SDB_OK == rc )
      {
         ptr = NULL ;
      }

   done:
      return ;
   }

   BOOLEAN _utilMemBlockPool::canAllocSegment( UINT64 size )
   {
      if ( _totalSize.fetch() + size >= _maxSize )
      {
         return FALSE ;
      }
      return TRUE ;
   }

   void _utilMemBlockPool::onAllocSegment( UINT64 size )
   {
      _totalSize.add( size ) ;
   }

   BOOLEAN _utilMemBlockPool::canReleaseSegment( UINT64 size )
   {
      return TRUE ;
   }

   void _utilMemBlockPool::onReleaseSegment( UINT64 size )
   {
      _totalSize.sub( size ) ;
   }

   /*
      Global var
   */
   static _utilMemBlockPool* g_pMemPool = NULL ;

   utilMemBlockPool* utilGetGlobalMemPool()
   {
      return g_pMemPool ;
   }

   void utilSetGlobalMemPool( utilMemBlockPool *pPool )
   {
      if ( NULL == g_pMemPool )
      {
         g_pMemPool = pPool ;
      }
      else if ( NULL == pPool )
      {
         SDB_ASSERT( g_pMemPool->getTotalSize() == 0,
                     "Total size must be 0" ) ;
         g_pMemPool = pPool ;
      }
      else
      {
         SDB_ASSERT( FALSE, "Mempool is already valid" ) ;
      }
   }

}


