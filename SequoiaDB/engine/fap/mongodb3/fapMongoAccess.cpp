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

   Source File Name = fapMongoAccess.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== ==============================================
          07/06/2021  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#include "fapMongoAccess.hpp"
#include "ossUtil.hpp"
#include "pmdOptions.hpp"
#include "fapMongoSession.hpp"

namespace engine {
   PMD_EXPORT_ACCESSPROTOCOL_DLL( fap::mongoAccess )
}

namespace fap
{

INT32 _mongoAccess::init( engine::IResource *pResource )
{
   UINT16 basePort = 0 ;
   if ( NULL != pResource )
   {
      _pResource = pResource ;
   }

   ossMemset( (void *)_serviceName, 0, OSS_MAX_SERVICENAME + 1 ) ;
   if ( NULL != _pResource )
   {
      basePort = _pResource->getLocalPort() ;
      ossItoa( basePort + PORT_OFFSET, _serviceName, OSS_MAX_SERVICENAME ) ;
   }

   return SDB_OK ;
}

INT32 _mongoAccess::active()
{
   return SDB_OK ;
}

INT32 _mongoAccess::deactive()
{
   return SDB_OK ;
}

INT32 _mongoAccess::fini()
{
   return SDB_OK ;
}

const CHAR * _mongoAccess::getServiceName() const
{
   SDB_ASSERT( '\0' != _serviceName[0], "service name should not be empty" ) ;
   return _serviceName ;
}

engine::pmdSession * _mongoAccess::getSession( SOCKET fd )
{
   mongoSession *pSession = NULL ;
   pSession = SDB_OSS_NEW mongoSession( fd, _pResource ) ;
   return pSession ;
}

void _mongoAccess::releaseSession( engine::pmdSession *pSession )
{
   if ( NULL != pSession )
   {
      SDB_OSS_DEL pSession ;
      pSession = NULL ;
   }
}

void _mongoAccess::_release()
{
   ossMemset( _serviceName, 0, OSS_MAX_SERVICENAME + 1 ) ;

   if ( NULL != _pResource )
   {
      // should not delete here, just make it point to nullptr
      _pResource = NULL ;
   }

   mongoReleaseErrorBson() ;
}

}
