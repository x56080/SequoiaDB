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

   Source File Name = csIindexMetaStorage.h

   Descriptive Name = 

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/08/2022  LYC  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_CS_INDEX_META_STORAGE_H_
#define VESSEL_CS_INDEX_META_STORAGE_H_

#include "oss.hpp"
#include "dms.hpp"

namespace engine
{
namespace vessel
{
   class csIndexMetaStorage : public SDBObject
   {
      public:
         csIndexMetaStorage() = default;
         csIndexMetaStorage(UINT32 csLid);
         ~csIndexMetaStorage() = default;
         csIndexMetaStorage(const csIndexMetaStorage &) = delete;
         csIndexMetaStorage &operator=(const csIndexMetaStorage &) = delete;

      public:
         void init(UINT32 csLid);

         OSS_INLINE BOOLEAN isValid() const
         {
            return DMS_INVALID_LOGICCSID != _csLid;
         }

      public:
         INT32 upsert(const bson::BSONObj &manifest) const;

         INT32 getMaxIndexLid(UINT32 &maxIndexLid) const;

         INT32 getIndexManifest(BOOLEAN &notFound,
                                bson::BSONObj &obj) const;

         INT32 destroy() const;

      private:
         UINT32 _csLid = DMS_INVALID_LOGICCSID;

   }; // class csIndexMetaStorage
} // namespace vessel
} // namespace engine

#endif // VESSEL_CS_INDEX_META_STORAGE_H_
