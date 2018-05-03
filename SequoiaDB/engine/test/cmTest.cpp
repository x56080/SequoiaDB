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
*******************************************************************************/

#include "core.h"
#include "pd.hpp"
#include "ossSocket.hpp"
#include "omagentDef.hpp"
#include "rtnRemoteExec.hpp"
#include "omagentDef.hpp"
#include "../bson/bson.h"

using namespace bson ;
using namespace engine ;


INT32 main ( INT32 argc, CHAR **argv )
{
   INT32 remoCode = 0 ;
   INT32 retCode = 1 ;
   CHAR hostname[OSS_MAX_HOSTNAME] ;
   BSONObj obj1, obj2 ;
   BSONObjBuilder ob1, ob2 ;
   if ( argc > 1 )
   {
      switch ( argv[1][0] )
      {
         case 'a' :
            remoCode = SDBSTART ;
            break ;
         case 'b' :
            remoCode = SDBSTOP ;
            break ;
         case 'c' :
            remoCode = SDBADD ;
            break ;
         case 'd' :
            remoCode = SDBMODIFY ;
            break ;
         default:
            remoCode = 0 ;
      }
   }
   try
   {
      if ( argc == 3 )
         ob1.append ( "svcname", argv[2] ) ;
      if ( argc == 4 )
      {
         ob1.append ( "svcname", argv[2] ) ;
         ob1.append ( "dbpath", argv[3] ) ;
      }
      if ( argc == 5 )
      {
         ob1.append ( "svcname", argv[2] ) ;
         ob2.append ( "svcname", argv[3] ) ;
         ob2.append ( "dbpath", argv[4] ) ;
      }
      obj1 = ob1.obj() ;
      obj2 = ob2.obj() ;
   }
   catch ( std::exception &e )
   {
      PD_LOG ( PDERROR, "Failed to build BSONObj" ) ;
      exit ( 0 ) ;
   }
   if ( ossSocket::getHostName ( hostname, OSS_MAX_HOSTNAME ) )
   {
      PD_LOG ( PDERROR, "Failed to get hostname" ) ;
      goto error ;
   }
   engine::rtnRemoteExec ( remoCode, hostname, &retCode, &obj1, &obj2 ) ;
   PD_LOG ( PDERROR, "rc = %d", retCode ) ;
done:
   return 0 ;
error:
   goto done ;
}
