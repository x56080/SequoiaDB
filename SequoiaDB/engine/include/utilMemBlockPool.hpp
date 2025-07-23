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

   Source File Name = utilMemBlockPool.hpp

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
#ifndef UTIL_MEM_BLOCK_POOL_HPP__
#define UTIL_MEM_BLOCK_POOL_HPP__

#include "utilSegment.hpp"

namespace engine
{

   #define UTIL_MEM_BLOCK_POOL_DFT_MAX_SZ          ( 8589934592LL )  ///8GB
   #define UTIL_MEM_A_SMALL_BLOCK_SIZE             ( 262144 )        ///256KB
   #define UTIL_MEM_A_MID_BLOCK_SIZE               ( 2097152 )       ///2MB
   #define UTIL_MEM_A_BIG_BLOCK_SIZE               ( 4194304 )       ///4MB
   #define UTIL_MEM_A_SMALL_BLOCK_SUBPOOL_NUM      ( 16 )
   #define UTIL_MEM_A_MID_BLOCK_SUBPOOL_NUM        ( 4 )
   #define UTIL_MEM_A_BIG_BLOCK_SUBPOOL_NUM        ( 2 )

   /// Memory info:
   /// | B-Eye(2) | Size(4) | Type(2) | User Data | E-Eye(2) |

   #define UTIL_MEM_B_EYE_CHAR         0xBE38
   #define UTIL_MEM_E_EYE_CHAR         0xAC52

   #define UTIL_MEM_B_EYE_LEN          sizeof(UINT16)
   #define UTIL_MEM_SIZE_LEN           sizeof(UINT32)
   #define UTIL_MEM_TYPE_LEN           sizeof(UINT16)
   #define UTIL_MEM_E_EYE_LEN          sizeof(UINT16)

   #define UTIL_MEM_HEAD_FILL_LEN   \
      ( UTIL_MEM_B_EYE_LEN + UTIL_MEM_SIZE_LEN + UTIL_MEM_TYPE_LEN )

   #define UTIL_MEM_TAIL_FILL_LEN      ( UTIL_MEM_E_EYE_LEN )

   #define UTIL_MEM_TOTAL_FILL_LEN  \
      ( UTIL_MEM_HEAD_FILL_LEN + UTIL_MEM_TAIL_FILL_LEN )

   #define UTIL_MEM_SIZE_2_REALSIZE(sz) \
         ( (UINT32)sz + UTIL_MEM_TOTAL_FILL_LEN )

   #define UTIL_MEM_PTR_2_USERPTR(ptr) \
         ( (CHAR*)(ptr) + UTIL_MEM_HEAD_FILL_LEN )

   #define UTIL_MEM_USERPTR_2_PTR(userPtr) \
         ( (CHAR*)(userPtr) - UTIL_MEM_HEAD_FILL_LEN )

   #define UTIL_MEM_PTR_B_EYE_PTR(ptr) \
      ( (UINT16*)(CHAR*)(ptr) )

   #define UTIL_MEM_PTR_SIZE_PTR(ptr)  \
      ( (UINT32*)((CHAR*)UTIL_MEM_PTR_B_EYE_PTR(ptr)+UTIL_MEM_B_EYE_LEN) )

   #define UTIL_MEM_PTR_TYPE_PTR(ptr)  \
      ( (UINT16*)((CHAR*)UTIL_MEM_PTR_SIZE_PTR(ptr)+UTIL_MEM_SIZE_LEN) )

   #define UTIL_MEM_PTR_E_EYE_PTR(ptr, sz) \
      ( (UINT16*)( (CHAR*)(ptr) + (UINT32)(sz) - UTIL_MEM_E_EYE_LEN ) )

   /** definition of _utilMemBlockPool
    *  _utilMemBlockPool is a place holder for a set of memory pools based on 
    *  the fixed size of element in the pool
    **/
   class _utilMemBlockPool : public SDBObject, public _utilSegmentHandler
   {
      /*
         Small: 16,   32,   64
         Mid  : 128,  256,  512
         Big  : 1024, 2048, 4096
      */
      typedef  CHAR    element16B[16] ;
      typedef  CHAR    element32B[32] ;
      typedef  CHAR    element64B[64] ;
      typedef  CHAR    element128B[128] ;
      typedef  CHAR    element256B[256] ;
      typedef  CHAR    element512B[512] ;
      typedef  CHAR    element1K[1024] ;
      typedef  CHAR    element2K[2048] ;
      typedef  CHAR    element4K[4096] ;

      enum MEMBLOCKPOOL_TYPE
      {
         MEMBLOCKPOOL_TYPE_DYN = 1,  // dynamically allocate space
         MEMBLOCKPOOL_TYPE_16,
         MEMBLOCKPOOL_TYPE_32,
         MEMBLOCKPOOL_TYPE_64,       // allocate from 64B pool
         MEMBLOCKPOOL_TYPE_128,
         MEMBLOCKPOOL_TYPE_256,
         MEMBLOCKPOOL_TYPE_512,
         MEMBLOCKPOOL_TYPE_1024,
         MEMBLOCKPOOL_TYPE_2048,
         MEMBLOCKPOOL_TYPE_4096,
         MEMBLOCKPOOL_TYPE_MAX
      } ;

   public: 
      _utilMemBlockPool() ;
      ~_utilMemBlockPool() ;

      INT32       init( UINT64 maxSize = UTIL_MEM_BLOCK_POOL_DFT_MAX_SZ ) ;
      void        fini() ;
      void        shrink() ;

      UINT64      getTotalSize() ;

      void*       alloc( UINT32 size ) ;
      void*       realloc( void* ptr, UINT32 size ) ;
      void        release( void*& ptr ) ;

   public:
      virtual BOOLEAN   canAllocSegment( UINT64 size ) ;
      virtual void      onAllocSegment( UINT64 size ) ;
      virtual BOOLEAN   canReleaseSegment( UINT64 size ) ;
      virtual void      onReleaseSegment( UINT64 size ) ;

   protected:
      MEMBLOCKPOOL_TYPE       _size2MemType( UINT32 size,
                                             ossAtomic64 **ppCount ) ;

      UINT32                  _type2Size( MEMBLOCKPOOL_TYPE type ) const ;

      void                    _fillPtr( CHAR *ptr, UINT16 type, UINT32 size ) ;

      BOOLEAN                 _checkAndExtract( const CHAR *ptr,
                                                UINT32 *pUserSize,
                                                UINT16 *pType ) const ;

   // private attributes:
   private:
      _utilSegmentManager<element16B>  *_16BSeg; // mem segs with 16B  element
      _utilSegmentManager<element32B>  *_32BSeg; // mem segs with 32B  element
      _utilSegmentManager<element64B>  *_64BSeg; // mem segs with 64B  element
      _utilSegmentManager<element128B> *_128BSeg;// mem segs with 128B element
      _utilSegmentManager<element256B> *_256BSeg;// mem segs with 256B element
      _utilSegmentManager<element512B> *_512BSeg;// mem segs with 512B element
      _utilSegmentManager<element1K>   *_1KSeg;  // mem segs with 1 KB element
      _utilSegmentManager<element2K>   *_2KSeg;  // mem segs with 2 KB element
      _utilSegmentManager<element4K>   *_4KSeg;  // mem segs with 4 KB element

      // counters for monitor

      // how many times we dynamic alloc because we failed in each segment
      ossAtomic64    _numDynamicAlloc16B ;
      ossAtomic64    _numDynamicAlloc32B ;
      ossAtomic64    _numDynamicAlloc64B ;
      ossAtomic64    _numDynamicAlloc128B ;
      ossAtomic64    _numDynamicAlloc256B ;
      ossAtomic64    _numDynamicAlloc512B ;
      ossAtomic64    _numDynamicAlloc1K ;
      ossAtomic64    _numDynamicAlloc2K ;
      ossAtomic64    _numDynamicAlloc4K ;

      UINT64         _maxSize ;
      ossAtomic64    _totalSize ;

      ossSpinXLatch  _latch ;
   } ;
   typedef _utilMemBlockPool utilMemBlockPool ;

   /*
      Global function
   */
   utilMemBlockPool* utilGetGlobalMemPool() ;
   void utilSetGlobalMemPool( utilMemBlockPool *pPool ) ;

}

#endif //UTIL_MEM_BLOCK_POOL_HPP__

