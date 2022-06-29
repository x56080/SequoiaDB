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