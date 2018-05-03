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

   Source File Name = pmdAccessProtocolBase.hpp

   Descriptive Name = Process MoDel Engine Dispatchable Unit Event Header

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains structure for events that
   used as inter-EDU communications.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/08/2014  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef PMD_ACCESSPROTOCOLBASE_HPP__
#define PMD_ACCESSPROTOCOLBASE_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "sdbInterface.hpp"
#include "pmdSessionBase.hpp"
#include "pmdProcessorBase.hpp"

namespace engine
{

   /*
      _IPmdAccessProtocol define
   */
   class _IPmdAccessProtocol : public SDBObject
   {
      public:
         _IPmdAccessProtocol() {}
         virtual ~_IPmdAccessProtocol() {}

         virtual const CHAR*     name() const = 0 ;
         /*
            0 for unlimited
         */
         virtual UINT32          maxConnNum() const { return 0 ; }

      public:
         virtual INT32           init( IResource *pResource ) = 0 ;
         virtual INT32           active() = 0 ;
         virtual INT32           deactive() = 0 ;
         virtual INT32           fini() = 0 ;

         virtual const CHAR*     getServiceName() const = 0 ;
         virtual pmdSession*     getSession( SOCKET fd ) = 0 ;
         virtual void            releaseSession( pmdSession *pSession ) = 0 ;

   } ;
   typedef _IPmdAccessProtocol IPmdAccessProtocol ;

   /*
      export accessprotocol define
      must include in accessprotocol cpp file
   */
   #define PMD_EXPORT_ACCESSPROTOCOL_DLL( apClass ) \
   SDB_EXTERN_C_START \
   SDB_EXPORT IPmdAccessProtocol *createAccessProtocol() \
   { \
      return SDB_OSS_NEW apClass() ; \
   } \
   SDB_EXPORT void releaseAccessProtocol( IPmdAccessProtocol *&pAccessProtocol ) \
   { \
      if ( NULL != pAccessProtocol ) \
      { \
         SDB_OSS_DEL pAccessProtocol ; \
         pAccessProtocol = NULL ;      \
      } \
   } \
   SDB_EXTERN_C_END

}

#endif // PMD_ACCESSPROTOCOLBASE_HPP__

