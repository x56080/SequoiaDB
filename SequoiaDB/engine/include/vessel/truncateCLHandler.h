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

   Source File Name = truncateCLHandler.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_TRUNCATE_CL_HANDLER_H_
#define VESSEL_TRUNCATE_CL_HANDLER_H_

#include "vessel/requestHandler.h"
#include "vessel/requestContext.h"
#include "vessel/collectionOptions.h"
#include "vessel/strSlice.h"
#include "dmsEngineOptions.hpp"

namespace engine
{
namespace vessel
{
   class truncateCLHandler : public requestHandler
   {
      public:
         truncateCLHandler(){}
         virtual ~truncateCLHandler(){}

      public:
         INT32 doit(const globalCollectionId &gcid,
                    const dmsTruncateCLOptions &o);

   };//class truncateCLHandler
}//namespace vessel
}//namespace engine

#endif//VESSEL_TRUNCATE_CL_HANDLER_H_