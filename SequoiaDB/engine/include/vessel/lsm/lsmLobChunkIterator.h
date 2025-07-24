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

   Source File Name = lsmLobChunkIterator.h

   Descriptive Name = 

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/20/2022  LYC  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_LSM_LOB_CHUNK_ITERATOR_H_
#define VESSEL_LSM_LOB_CHUNK_ITERATOR_H_

#include "vessel/lsm/lsmColumnFamily.h"
#include "../bson/bson.hpp"
#include "vessel/lsm/lsmLobChunkKey.h"

namespace engine
{
namespace vessel
{
   class lsmLobChunkIterator : public SDBObject
   {
      friend class lsmLobcMetaStorage;
      public:
         lsmLobChunkIterator() = default;
         ~lsmLobChunkIterator();
         lsmLobChunkIterator(const lsmLobChunkIterator &) = delete;
         lsmLobChunkIterator &operator=(const lsmLobChunkIterator &) = delete;

      public:
         BOOLEAN isValid()const;
         void close();
         void next();

         lsmLobChunkKey getCurrentKey();
         bson::BSONObj getNotOwnedValue();

      private:
         rocksdb::Iterator *_itr = nullptr;
         lsmLobChunkKey _lowBoundKey;
         lsmLobChunkKey _upBoundKey;
         rocksdb::Slice _lowKey;
         rocksdb::Slice _upKey;

   }; // class lsmLobChunkIterator
} // namespace vessel
} // namespace engine

#endif // VESSEL_LSM_LOB_CHUNK_ITERATOR_H_