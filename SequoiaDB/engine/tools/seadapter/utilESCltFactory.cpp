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

   Source File Name = utilESCltFactory.cpp

   Descriptive Name = Elasticsearch client manager.

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS storage unit and its methods.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          14/04/2017  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#include "core.hpp"
#include "pd.hpp"
#include "utilESClt.hpp"
#include "utilESCltFactory.hpp"

namespace seadapter
{
   _utilESCltFactory::_utilESCltFactory()
   {
      ossMemset( _url, 0, sizeof( _url ) ) ;
      _timeout = 0 ;
   }

   _utilESCltFactory::~_utilESCltFactory()
   {
   }

   INT32 _utilESCltFactory::init( const CHAR *url, INT32 timeout )
   {
      INT32 rc = SDB_OK ;

      if ( !url || ossStrlen( url ) > SEADPT_SE_SVCADDR_MAX_SZ )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Url to init search engine client "
                          "manager is invalid" ) ;
         goto error ;
      }
      ossStrncpy( _url, url, SEADPT_SE_SVCADDR_MAX_SZ + 1 ) ;
      _timeout = timeout ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilESCltFactory::create( utilESClt **seClt )
   {
      INT32 rc = SDB_OK ;
      utilESClt* client = NULL ;

      SDB_ASSERT( seClt, "Parameter should not be null" ) ;

      client = SDB_OSS_NEW _utilESClt() ;
      if ( !client )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Failed to allocate memory for search engine client, "
                 "rc: %d", rc ) ;
         goto error ;
      }
      rc = client->init( _url, FALSE, _timeout ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to init search engine client, rc: %d",
                 rc ) ;
         goto error ;
      }

      *seClt = client ;

   done:
      return rc ;
   error:
      if ( client )
      {
         SDB_OSS_DEL client ;
      }
      goto done ;
   }
}

