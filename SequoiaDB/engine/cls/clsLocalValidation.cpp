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

   Source File Name = clsLocalValidation.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/03/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "clsLocalValidation.hpp"
#include "pmdEDU.hpp"
#include "rtn.hpp"
#include "dmsCB.hpp"


using namespace bson ;

namespace engine
{
   /*static void func()
   {
      return ;
   }*/

   INT32 _clsLocalValidation::run()
   {
      INT32 rc = SDB_OK ;
      const UINT32 pLen = 512 ;
      //SDB_RTNCB *rtnCB = sdbGetRTNCB() ;
      //SDB_DMSCB *dmsCB = sdbGetDMSCB() ;
      MON_CS_LIST csList ;

      /// 1. malloc
      CHAR *p = (CHAR *)SDB_OSS_MALLOC( pLen ) ;
      if ( NULL == p )
      {
         PD_LOG( PDERROR, "failed to allocate mem." ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      *p = '\0' ;
      *( p + pLen - 1 ) = '\0' ;

      /// 2. create new thread
      /*try
      {
         boost::thread t( func ) ;
         t.join() ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "failed to create new thread:%s",
                 e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }*/

      /// 3. dump dms
      //dmsCB->dumpInfo( csList, TRUE ) ;

      /// 4. update validation tick
      pmdUpdateValidationTick() ;

   done:
      SAFE_OSS_FREE( p ) ;
      return rc ;
   error:
      goto done ;
   }

}

