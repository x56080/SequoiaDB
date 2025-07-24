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