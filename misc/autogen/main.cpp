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

   Source File Name = main.cpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#include "options.hpp"
#include "generateInterface.hpp"

int main( int argc, char** argv )
{
   int rc = 0 ;
   cmdOptions *opt = getCmdOptions() ;
   generateRunner *runner = getGenerateRunner() ;

   rc = opt->parse( argc, argv ) ;
   if ( -1 == rc )
   {
      rc = 0 ;
      goto done ;
   }
   else if ( rc )
   {
      goto error ;
   }

   rc = runner->init() ;
   if ( rc )
   {
      goto error ;
   }

   rc = runner->run() ;
   if ( rc )
   {
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}
