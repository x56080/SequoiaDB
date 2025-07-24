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

   Source File Name = utilEnvCheckTest.cpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#include "utilEnvCheck.hpp"
#include "pmd.hpp"

#include <iostream>

using namespace engine;
using std::cout;
using std::endl;

int main(int argc, char *argv[])
{
   if(argc == 2)
   {
      //cout << "./envCheck.out logFilePath" << endl;
      sdbEnablePD(argv[1], 1000, 10) ;
   }
   if(argc == 1)
   {
      sdbEnablePD( pmdGetOptionCB()->getDiagLogPath(),
                   pmdGetOptionCB()->diagFileNum() ) ;
      setPDLevel( (PDLEVEL)( pmdGetOptionCB()->getDiagLevel() ) ) ;
   }
   else
   {
      cout << "wrong cmd format !" << endl ;
   }
   
  // sdbEnablePD(argv[1], 1, 100) ;
   //setPDLevel( (PDLEVEL)( pmdGetOptionCB()->getDiagLevel() ) );
   utilCheckEnv();
   return 0;
}
