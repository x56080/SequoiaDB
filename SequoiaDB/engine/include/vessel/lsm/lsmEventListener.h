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

   Source File Name = lsmEventListener.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/21/2022  ZHY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_LSM_EVENT_LISTENER_H_
#define VESSEL_LSM_EVENT_LISTENER_H_

#include "ossTypes.h"
#include "rocksdb/listener.h"
#include "vessel/lsm/lsmDB.h"
#include <memory>

namespace engine
{
namespace vessel
{
   class lsmEventListener : public rocksdb::EventListener
   {
   public:
      lsmEventListener() = delete;
      virtual ~lsmEventListener() = default;
      lsmEventListener(lsmDB *db);

   public:
      const CHAR *Name() const override
      {
         return "sdb.lsmEventListener";
      }

      virtual void OnTableFileCreated(
          const rocksdb::TableFileCreationInfo &info) override;

   private:
      lsmDB *_db;
   };

   extern std::shared_ptr<lsmEventListener> newLsmEventListener(lsmDB *db);

} // namespace vessel
} // namespace engine

#endif // VESSEL_LSM_EVENT_LISTENER_H_