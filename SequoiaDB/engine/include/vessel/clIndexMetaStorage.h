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

         INT32 upsert(UINT32 indexLid,
                      const indexObjectMap &im) const;

         INT32 upsert(const indexObjectMap &im);

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