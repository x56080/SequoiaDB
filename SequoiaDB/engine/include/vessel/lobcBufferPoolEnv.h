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

   Source File Name = lobcBufferPoolEnv.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_LOBC_BUFFER_POOL_ENV_H_
#define VESSEL_LOBC_BUFFER_POOL_ENV_H_

#include "ossMemPool.hpp"
#include "vessel/blockBasedMemPool.h"
#include "vessel/lobChunkBuffer.h"
#include "vessel/dirtyLobcBufferList.h"
#include "vessel/bufferPoolOptions.h"

#include <mutex> //c++11

namespace engine
{
namespace vessel
{
   class lobcBufferPoolEnv : public SDBObject
   {
      public:
         lobcBufferPoolEnv(){}
         ~lobcBufferPoolEnv();

         lobcBufferPoolEnv(const lobcBufferPoolEnv &) = delete;
         lobcBufferPoolEnv &operator=(const lobcBufferPoolEnv &) = delete;

      public:
         OSS_INLINE blockBasedMemPool *getMemPool() {return &_pool;}
         OSS_INLINE const blockBasedMemPool *getMemPool()const {return &_pool;}
         OSS_INLINE SHARED_LOBC_BUFFER_LIST &searchBucketEntry(UINT32 hash, UINT32 &entryId)
         {
            entryId = ((_entries.size() - 1) & hash);
            return _entries.at(entryId);
         }
         OSS_INLINE SHARED_LOBC_BUFFER_LIST &getBucketEntry(UINT32 pos)
         {
            return _entries.at(pos);
         }
         OSS_INLINE std::mutex &getEntryMutex(UINT32 entryId)
         {
            return *(_mutexes.at((_mutexes.size() - 1) & entryId));
         }

         OSS_INLINE dirtyLobcBufferList &getDirtyList() {return _dirtyList;}
         OSS_INLINE const dirtyLobcBufferList &getDirtyList()const {return _dirtyList;}

         OSS_INLINE BOOLEAN isValid()const {return _pool.isValid();}

      public:
         INT32 init(const lobcBufferPoolOptions &o);
         void fini();

      private:
         typedef class std::vector<SHARED_LOBC_BUFFER_LIST> _BUFFER_ENTRY_VEC;
         typedef class std::vector<std::mutex *> _BUFFER_MUTEX_VEC;

      private:
         //UINT32 _pageSize = 0;
         blockBasedMemPool _pool;
         _BUFFER_ENTRY_VEC _entries;
         _BUFFER_MUTEX_VEC _mutexes;
         dirtyLobcBufferList _dirtyList;

   };//class lobcBufferPoolEnv
} // namespace vessel

} // namespace enginelobcBucketRegion


#endif//VESSEL_LOBC_BUFFER_POOL_ENV_H_