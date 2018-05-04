/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = oss.hpp

   Descriptive Name = Operating System Services Header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains functions for OSS operations.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OSS_HPP_
#define OSS_HPP_
#pragma warning( disable: 4290 )
#include "oss.h"
#include "ossMem.hpp"

// we should NEVER AND EVER put any virtual function in this object
// and we should never cast an object to SDBObject and delete.
class SDBObject
{
protected :
   // never instantiate SDBObject
   SDBObject () {}
public :
   // do NOT put virtual destructor because it will change the inherited object
   // size

   // regular new
   void * operator new ( size_t size ) throw ( const char * )
   {
      void *p = SDB_OSS_MALLOC(size) ;
      if ( !p ) throw "allocation failure" ;
      return p ;
   }

   void * operator new[] ( size_t size ) throw ( const char * )
   {
      void *p = SDB_OSS_MALLOC(size) ;
      if ( !p ) throw "allocation failure" ;
      return p ;
   }

   void operator delete ( void *p )
   {
      SDB_OSS_FREE(p) ;
   }

   void operator delete[] ( void *p )
   {
      SDB_OSS_FREE(p) ;
   }

   // new with file/line number
   void * operator new ( size_t size, const CHAR *pFile, UINT32 line )
         throw ( const char * )
   {
      void *p = SDB_OSS_MALLOC3(size, pFile, line ) ;
      if ( !p ) throw "allocation failure" ;
      return p ;
   }

   void * operator new[] ( size_t size, const CHAR *pFile, UINT32 line )
         throw ( const char * )
   {
      void *p = SDB_OSS_MALLOC3(size, pFile, line ) ;
      if ( !p ) throw "allocation failure" ;
      return p ;
   }

   void operator delete ( void *p, const CHAR *pFile, UINT32 line )
   {
      SDB_OSS_FREE(p) ;
   }

   void operator delete[] ( void *p, const CHAR *pFile, UINT32 line )
   {
      SDB_OSS_FREE(p) ;
   }

   // no throw
   void * operator new ( size_t size, const std::nothrow_t & )
   {
      return SDB_OSS_MALLOC(size) ;
   }

   void * operator new[] ( size_t size, const std::nothrow_t & )
   {
      return SDB_OSS_MALLOC(size) ;
   }

   void operator delete ( void *p, const std::nothrow_t & )
   {
      SDB_OSS_FREE(p) ;
   }

   void operator delete[] ( void *p, const std::nothrow_t & )
   {
      SDB_OSS_FREE(p) ;
   }

   // no throw with line number
   void * operator new ( size_t size, const CHAR *pFile,
                         UINT32 line, const std::nothrow_t & )
   {
      return SDB_OSS_MALLOC3(size, pFile, line ) ;
   }

   void * operator new[] ( size_t size, const CHAR *pFile,
                         UINT32 line, const std::nothrow_t & )
   {
      return SDB_OSS_MALLOC3(size, pFile, line ) ;
   }

   void operator delete ( void *p, const CHAR *pFile,
                          UINT32 line, const std::nothrow_t & )
   {
      SDB_OSS_FREE(p) ;
   }

   void operator delete[] ( void *p, const CHAR *pFile,
                            UINT32 line, const std::nothrow_t & )
   {
      SDB_OSS_FREE(p) ;
   }
} ;
typedef class SDBObject SDBObject ;

#endif // OSS_HPP_
