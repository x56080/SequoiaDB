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

   Source File Name = sdbExport.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who          Description
   ====== =========== ============ =============================================
          28/07/2016  Lin Yuebang  Initial Draft

   Last Changed =

*******************************************************************************/
#include "expOptions.hpp"
#include "expCL.hpp"
#include "expExport.hpp"
#include "expUtil.hpp"
#include "ossVer.h"
#include "pd.hpp"
#include <iostream>

using namespace exprt ;
using namespace std ;

#define EXP_LOG_PATH "sdbexport.log"

int main( int argc, char* argv[] )
{
   INT32 rc = SDB_OK ;
   expOptions options ;

   sdbEnablePD(EXP_LOG_PATH);
   setPDLevel(PDINFO);

   try
   {
      expRoutine routine(options) ;

      rc = options.parseCmd( argc, argv ) ;
      if (SDB_OK != rc)
      {
         cerr << "failed to parse options" << endl ;
         PD_LOG(PDERROR, "Failed to parse cmd options, rc=%d", rc) ;
         goto error ;
      }

      if ( options.hasHelp() )
      {
         options.printHelpInfo() ;
         goto done ;
      }
      if ( options.hasVersion() )
      {
         ossPrintVersion("sdbexport") ;
         goto done ;
      }
      
      rc = routine.run() ;
      routine.printStatistics() ;
      if ( SDB_OK != rc )
      {
         PD_LOG(PDERROR, "Routine running failure, rc=%d", rc) ;
         goto error ;
      }
   }
   catch ( std::exception &e )
   {
      PD_LOG( PDERROR, "Unexpected error happened:%s", e.what() );
      goto error ;
   }

   cout << "done!" << endl ;
done:
   return RC2ShellRC(rc) ;
error:
   goto done;
}