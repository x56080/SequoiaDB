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

   Source File Name = lsmLobChunkIterator.cpp

   Descriptive Name = 

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/20/2022  LYC  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/lsm/lsmLobChunkIterator.h"
namespace engine
{
namespace vessel
{
   lsmLobChunkIterator::~lsmLobChunkIterator()
   {
      close();
   }

   lsmLobChunkKey lsmLobChunkIterator::getCurrentKey()
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      lsmLobChunkKey res;
      if (LSM_LOB_CHUNK_KEY_SIZE == _itr->key().size())
      {
         res = *(lsmLobChunkKey*)(_itr->key().data());
      }
      return res;
   }

   bson::BSONObj lsmLobChunkIterator::getNotOwnedValue()
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      bson::BSONObj obj;
      if (0 != _itr->value().size())
      {
         try
         {
            obj = bson::BSONObj(_itr->value().data(), _itr->value().size());
         }
         catch(std::exception &e)
         {
            PD_LOG(PDWARNING, "invalid lsm value, exception:%s", e.what());
            obj = bson::BSONObj();
         }
      }
      return obj;
   }
   
   BOOLEAN lsmLobChunkIterator::isValid()const
   {
      return _itr != nullptr && 
             _itr->Valid();
   }

   void lsmLobChunkIterator::next()
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      _itr->Next();
   }

   void lsmLobChunkIterator::close()
   {
      if (nullptr != _itr)
      {
         delete _itr;
         _itr = nullptr;
      }
      _lowKey = rocksdb::Slice();
      _upKey = rocksdb::Slice();
      _lowBoundKey.reset();
      _upBoundKey.reset();
   }

} // namespace vessel
} // namespace engine
