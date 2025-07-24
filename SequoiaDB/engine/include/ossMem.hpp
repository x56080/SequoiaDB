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

   Source File Name = ossMem.hpp

   Descriptive Name = Operating System Services Memory Header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains declares for all memory
   allocation/free operations.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OSSMEM_HPP_
#define OSSMEM_HPP_
#include "ossMem.h"
#include <new>

#define SDB_SYS_MEMDUMPNAME      ".memsysstat"
#define SDB_SYS_MEMINFONAME      ".memsysinfo_xml"
#define SDB_OSS_MEMDUMPNAME      ".memossdump"
#define SDB_POOL_MEMDUMPNAME     ".mempooldump"
#define SDB_TC_MEMDUMPNAME       ".memtcdump"
#define SDB_OSS_NEW              new(__FILE__,__LINE__,std::nothrow)
#define SDB_OSS_DEL              delete

#define SAFE_OSS_DELETE(p) \
   do {                    \
      if (p) {             \
         SDB_OSS_DEL p ;   \
         p = NULL ;        \
      }                    \
   } while (0)

/*
void* operator new (size_t size, const CHAR *file, UINT32 line ) ;

void operator delete ( void *p, const CHAR *file, UINT32 line ) ;*/

#if defined ( SDB_ENGINE ) || defined ( SDB_FMP ) || defined ( SDB_TOOL ) || defined ( SDB_SHELL )

#define OSS_MEMDEBUG_MASK_OSSMALLOC       0x00000001
#define OSS_MEMDEBUG_MASK_POOLALLOC       0x00000002
#define OSS_MEMDEBUG_MASK_THREADALLOC     0x00000004

#define OSS_MEMDEBUG_MASK_DFT             OSS_MEMDEBUG_MASK_OSSMALLOC|\
                                          OSS_MEMDEBUG_MASK_POOLALLOC

#define OSS_MEMDEBUG_MASK_ALL             0xFFFFFFFF

#define OSS_MEMDEBUG_MASK_OSSMALLOC_STR   "OSS"
#define OSS_MEMDEBUG_MASK_POOLALLOC_STR   "POOL"
#define OSS_MEMDEBUG_MASK_THREADALLOC_STR "TC"
#define OSS_MEMDEBUG_MASK_ALL_STR         "ALL"
#define OSS_MEMDEBUG_MASK_DFT_STR         "OSS|POOL"

BOOLEAN ossString2MemDebugMask( const CHAR *pStr, UINT32 &mask ) ;

void ossPoolMemTrack( void *p, UINT64 userSize, UINT32 file, UINT32 line, const CHAR *pInfo = NULL ) ;
void ossPoolMemUnTrack( void *p ) ;

typedef BOOLEAN (*OSS_POOL_MEMCHECK_FUNC)( void* p ) ;
typedef void    (*OSS_POOL_MEMINFO_FUNC)( void* p, UINT64 &size,
                                          INT32 &pool, INT32 &index,
                                          BOOLEAN *pFreeInTc ) ;
typedef void*   (*OSS_POOL_MEMUSERPTR_FUNC)( void* p ) ;

void ossSetPoolMemcheckFunc( OSS_POOL_MEMCHECK_FUNC pFunc ) ;
void ossSetPoolMemInfoFunc( OSS_POOL_MEMINFO_FUNC pFunc ) ;
void ossSetPoolMemUserPtrFunc( OSS_POOL_MEMUSERPTR_FUNC pFunc ) ;

void ossThreadMemTrack( void *p, UINT64 userSize, UINT32 file, UINT32 line, const CHAR *pInfo = NULL ) ;
void ossThreadMemUnTrack( void *p ) ;

typedef void*   (*OSS_TC_MEMREALPTR_FUNC)( void* p ) ;

void ossSetTCMemRealPtrFunc( OSS_TC_MEMREALPTR_FUNC pFunc ) ;

#endif

#endif // OSSMEM_HPP_

