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

   Source File Name = outerResource.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_OUTER_RESOURCE_H_
#define VESSEL_OUTER_RESOURCE_H_

#include "core.hpp"
#include "oss.hpp"
#include "vessel/indexKeyGenerator.h"
#include "sdbInterface.hpp"
#include "interface/ITransLockConsole.h"
#include "interface/IDataJournal.h"

namespace engine
{
namespace vessel
{
   class outerResource : public SDBObject
   {
      public:
         BOOLEAN isValid()const
         {
            return nullptr != journal &&
                   nullptr != executorPool &&
                   !(!indexKeyGen) &&
                   nullptr != transLockConsole;
         }
         void reset()
         {
            journal = nullptr;
            executorPool = nullptr;
            indexKeyGen = indexKeyGenForBsonRecord;
            transLockConsole = nullptr;
         }

         /// expensive operation.
         /// will loop scan all working executors' min running lsn.
         UINT64 getMinUncompletedLSN();

      public:
         IDataJournal *journal = nullptr;
         IExecutorMgr *executorPool = nullptr;
         INDEX_KEY_GENERATOR indexKeyGen = indexKeyGenForBsonRecord;
         ITransLockConsole *transLockConsole = nullptr;
   };//class outerResource
}//namespace vessel
}//namespace engine

#endif//VESSEL_OUTER_RESOURCE_H_