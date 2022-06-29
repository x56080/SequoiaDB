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

   Source File Name = lsmLobcMetaStorage.h

   Descriptive Name = 

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/20/2022  LYC  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_LSM_LOBC_META_STORAGE_H_
#define VESSEL_LSM_LOBC_META_STORAGE_H_

#include "vessel/lsm/lsmLobChunkIterator.h"

namespace engine
{
namespace vessel
{
   class lsmLobcMetaStorage : public SDBObject
   {
      public:
         lsmLobcMetaStorage() = default;
         ~lsmLobcMetaStorage() = default;
         lsmLobcMetaStorage(const lsmLobcMetaStorage &) = delete;
         lsmLobcMetaStorage &operator= (const lsmLobcMetaStorage &) = delete;

      public:
         void init(const lsmColumnFamily &cf);
         void fini();

         OSS_INLINE BOOLEAN isValid()
         {
            return _cf.isValid();
         }

         INT32 put(const lsmLobChunkKey &key,
                   const bson::BSONObj &value);

         INT32 get(const lsmLobChunkKey &key,
                   bson::BSONObj &value);

         INT32 remove(const lsmLobChunkKey &key);

         INT32 truncate(UINT32 csid);

         INT32 truncate(UINT32 csid,
                        UINT32 clid);
         
         INT32 openIterator(UINT32 csid,
                            lsmLobChunkIterator &itr);
      
      private:
         lsmColumnFamily _cf;

   }; // class lsmLobcMetaStorage
} // namespace vessel
} // namespace engine

#endif // VESSEL_LSM_LOBC_META_STORAGE_H_