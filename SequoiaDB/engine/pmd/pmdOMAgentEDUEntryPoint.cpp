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

   Source File Name = pmdOMAgentEDUEntryPoint.cpp

   Descriptive Name = Process MoDel Engine Dispatchable Unit Event Header

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains structure for events that
   used as inter-EDU communications.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          23/06/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "pmdEDUEntryPoint.hpp"

namespace engine
{

   pmdEntryPoint getEntryFuncByType ( EDU_TYPES type )
   {
      pmdEntryPoint rt = NULL ;
      static const _eduEntryInfo entry[] = {
         ON_EDUTYPE_TO_ENTRY1 ( EDU_TYPE_AGENT, FALSE,
                                pmdAsyncSessionAgentEntryPoint,
                                "Agent" ),
         ON_EDUTYPE_TO_ENTRY1 ( EDU_TYPE_BACKGROUND_JOB, FALSE,
                                pmdBackgroundJobEntryPoint,
                                "Task" ),
         ON_EDUTYPE_TO_ENTRY1 ( EDU_TYPE_OMMGR, TRUE,
                                pmdCBMgrEntryPoint,
                                "OMManager" ),
         ON_EDUTYPE_TO_ENTRY1 ( EDU_TYPE_OMNET, TRUE,
                                pmdAsyncNetEntryPoint,
                                "OMNet" ),
         ON_EDUTYPE_TO_ENTRY1 ( EDU_TYPE_PIPESLISTENER, TRUE,
                                pmdPipeListenerEntryPoint,
                                "PipeListener" ),

         // For the end
         ON_EDUTYPE_TO_ENTRY1 ( EDU_TYPE_MAXIMUM, FALSE,
                                NULL,
                                "Unknow" )
      };

      static const UINT32 number = sizeof ( entry ) / sizeof ( _eduEntryInfo ) ;

      UINT32 index = 0 ;
      for ( ; index < number ; index ++ )
      {
         if ( entry[index].type == type )
         {
            rt = entry[index].entryFunc ;
            goto done ;
         }
      }

   done :
      return rt ;
   }

}


