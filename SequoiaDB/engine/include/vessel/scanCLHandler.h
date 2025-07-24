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

   Source File Name = scanCLHandler.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_SCAN_CL_HANDLER_H_
#define VESSEL_SCAN_CL_HANDLER_H_

#include "vessel/requestHandler.h"

namespace engine
{
namespace vessel
{
   class scanCLCursor;
   
   class scanCLHandler : public requestHandler
   {
      public:
         scanCLHandler(){}
         virtual ~scanCLHandler(){}

         INT32 doit(scanCLCursor *cursor);
   };//class scanCLHandler
}//namespace vessel
}//namespace engine

#endif//VESSEL_SCAN_CL_HANDLER_H_