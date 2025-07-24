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
