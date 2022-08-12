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

   Source File Name = clIndexMetaStorage.h

   Descriptive Name = 

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/30/2022  LYC  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef VESSEL_CL_INDEX_META_STORAGE_H_
#define VESSEL_CL_INDEX_META_STORAGE_H_

#include "oss.hpp"
#include "dms.hpp"
#include "vessel/indexDef.h"
#include "vessel/indexObjectMap.h"
#include "dpsDef.hpp"

namespace engine
{
namespace vessel
{
   class clIndexMetaStorage : public SDBObject
   {
      public:
         clIndexMetaStorage() = default;
         clIndexMetaStorage(UINT32 lcsid, UINT32 lclid);
         ~clIndexMetaStorage() = default;
         clIndexMetaStorage(const clIndexMetaStorage &) = delete;
         clIndexMetaStorage &operator=(const clIndexMetaStorage &) = delete;

      public:
         void init(UINT32 csLid,
                   UINT32 clLid);
         
         OSS_INLINE BOOLEAN isValid() const
         {
            return DMS_INVALID_LOGICCSID != _csLid &&
                   DMS_INVALID_LOGICCLID != _clLid;
         }

      public:
         INT32 reload(indexObjectMap &im) const;

         INT32 commit(UINT32 indexLid,
                      const indexObjectMap &im) const;

      public:
         INT32 upsert(UINT32 indexLid,
                      const bson::BSONObj &indexEntry) const;

         INT32 getIndexEntry(UINT32 indexLid,
                             bson::BSONObj &obj) const;

         INT32 list(ossPoolList<bson::BSONObj> &objs) const;

         INT32 removeEntry(UINT32 indexLid) const;

         INT32 destroy() const;

      private:
         UINT32 _csLid = DMS_INVALID_LOGICCSID;
         UINT32 _clLid = DMS_INVALID_LOGICCLID;
   };

} // namespace vessel
} // namespace engine

#endif // VESSEL_CL_INDEX_META_STORAGE_H_