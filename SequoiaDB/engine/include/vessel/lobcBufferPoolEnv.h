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

   Source File Name = lobcBufferPoolEnv.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_LOBC_BUFFER_POOL_ENV_H_
#define VESSEL_LOBC_BUFFER_POOL_ENV_H_

#include "ossMemPool.hpp"
#include "vessel/blockBasedMemPool.h"
#include "vessel/lobChunkBuffer.h"
#include "vessel/dirtyLobcBufferList.h"
#include "vessel/lobcBufferPoolOptions.h"

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
         OSS_INLINE SHARED_LOBC_BUFFER_LIST &getEntry(UINT32 hash)
         {
            return _entries.at((_entries.size() - 1) & hash);
         }
         OSS_INLINE std::mutex &getEntryMutex(UINT32 hash)
         {
            return *(_mutexes.at((_mutexes.size() - 1) & hash));
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