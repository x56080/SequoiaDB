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

   Source File Name = checkpointController.h

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_CHECKPOINT_CONTROLLER_H_
#define VESSEL_CHECKPOINT_CONTROLLER_H_

#include "core.hpp"
#include "oss.hpp"

namespace engine
{
namespace vessel
{
   class checkpointController : public SDBObject
   {
      public:
         checkpointController(){}
         ~checkpointController(){}

      public:
         UINT64 getLastCheckpointLSN()const
         {
            return 0;
         }
         BOOLEAN hasAtLeastOneCheckpoint()const
         {
            return FALSE;
         }

         void fini(){}
   };//class checkpointController
}//namespace vessel
}//namespace engine

#endif//VESSEL_CHECKPOINT_CONTROLLER_H_