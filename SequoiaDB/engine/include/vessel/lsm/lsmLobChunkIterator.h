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