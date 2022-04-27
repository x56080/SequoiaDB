/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = outerResource.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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