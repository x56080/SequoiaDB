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

   Source File Name = sdbImport.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/7/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "impOptions.hpp"
#include "impRoutine.hpp"
#include "impUtil.hpp"
#include "ossVer.h"
#include "pd.hpp"

using namespace import;

#define IMP_LOG_PATH "sdbimport.log"

int main(int argc, char* argv[])
{
   Options options;
   INT32 rc = SDB_OK;

   sdbEnablePD(IMP_LOG_PATH);
   setPDLevel(PDINFO);

   rc = options.parse(argc, argv);
   if (SDB_OK != rc)
   {
      PD_LOG(PDERROR, "failed to parse options, rc=%d", rc);
      goto error;
   }

   if (options.hasHelp())
   {
      options.printHelpInfo();
      goto done;
   }

   if (options.hasVersion())
   {
      ossPrintVersion("sdbimport");
      goto done;
   }

   if (options.hasHelpfull())
   {
      options.printHelpfullInfo();
      goto done;
   }

   try
   {
      Routine routine(options);

      rc = routine.run();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "routine running failure, rc=%d", rc);
      }

      routine.printStatistics();
   }
   catch(std::exception &e)
   {
      PD_LOG(PDERROR, "unexpected error happened:%s", e.what());
   }

done:
   return RC2ShellRC(rc);
error:
   goto done;
}

