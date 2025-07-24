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

   Source File Name = vessel.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_VESSEL_H_
#define VESSEL_VESSEL_H_

#include "interface/IDataStorageEngine.h"
#include "vessel/vesselOptions.h"
#include "vessel/outerResource.h"

namespace engine
{
namespace vessel
{
   class IVessel : public IDataStorageEngine
   {
      public:
         IVessel(){}
         virtual ~IVessel(){}
         IVessel(const IVessel &) = delete;
         IVessel &operator=(const IVessel &) = delete;

      public:
         virtual INT32 open(IExecutor *executor,
                            const outerResource *resource,
                            const openDBOptions &options) = 0;

         
         virtual INT32 close(IExecutor *executor,
                             const closeDBOptions &options) = 0;

   }; /// end of class IVessel
} /// end of namespace vessel
} /// end of namespace engine

#endif