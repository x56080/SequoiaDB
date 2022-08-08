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
