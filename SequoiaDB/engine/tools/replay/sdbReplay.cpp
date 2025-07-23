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

   Source File Name = sdbReplay.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          6/9/2016  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "rplOptions.hpp"
#include "rplReplayer.hpp"
#include "ossCmdRunner.hpp"
#include "ossVer.h"
#include "pd.hpp"
#include "utilCommon.hpp"
#include <iostream>
#include <string>
#include <sstream>

using namespace replay;

#define REPLAY_LOG_PATH "sdbreplay.log"

int main(int argc, char* argv[])
{
   Options options;
   INT32 rc = SDB_OK;
   CHAR verText[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;

   sdbEnablePD(REPLAY_LOG_PATH);
   setPDLevel(PDINFO);

   rc = options.parse(argc, argv);
   if (SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to parse arguments, rc=%d", rc);
      goto error;
   }

   if (options.hasHelp())
   {
      options.printHelp();
      goto done;
   }

   if (options.hasVersion())
   {
      ossPrintVersion("sdbreplay");
      goto done;
   }

   if (options.hasHelpfull())
   {
      options.printHelpfull();
      goto done;
   }

   if (options.debug())
   {
      setPDLevel(PDDEBUG);
   }

   ossSprintVersion( "Version", verText, OSS_MAX_PATHSIZE, FALSE ) ;
   PD_LOG( PDEVENT, "Start sdbreplay[%s]...", verText) ;

   if (options.daemon())
   {
      engine::ossCmdRunner runner;
      string cmd = options.buildBackgroundCmd(argc, argv);
      UINT32 exitCode = 0;

      if (options.debug())
      {
         std::cout << "start background process: " << cmd.c_str() << std::endl;
      }

      rc = runner.exec(cmd.c_str(), exitCode, TRUE, -1, FALSE, NULL, TRUE, FALSE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to run in background, rc=%d", rc);
         goto error;
      }

      if (options.debug())
      {
         std::cout << "background pid is " << runner.getPID() << std::endl;
      }

      goto done;
   }

   {
      string cmd = options.buildPrintableCmd(argc, argv);
#if defined (_LINUX)
      if (!options.password().empty())
      {
         ossEnableNameChanges(argc, argv);
         ossRenameProcess(cmd.c_str());
      }
#endif // _LINUX
      PD_LOG(PDEVENT, "command: %s", cmd.c_str());
   }

   try
   {
      Replayer replayer;

      rc = replayer.init(options);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to init replayer, rc=%d", rc);
         std::cerr << "Failed to init, see detail in sdbreplay.log"
                   << std::endl;
         goto error;
      }

      rc = replayer.run();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to run replayer, rc=%d", rc);
         if (SDB_INTERRUPT != rc)
         {
            std::cerr << "Failed to run, see detail in sdbreplay.log"
                      << std::endl;
         }
         goto error;
      }
   }
   catch(std::exception &e)
   {
      rc = SDB_SYS ;
      PD_LOG(PDERROR, "unexpected error happened:%s", e.what());
      std::cerr << "unexpected error happened: "
                << e.what()
                << std::endl;
   }

done:
   PD_LOG ( PDEVENT, "Stop programme, exit code: %d", rc ) ;
   return engine::utilRC2ShellRC(rc);
error:
   goto done;
}


