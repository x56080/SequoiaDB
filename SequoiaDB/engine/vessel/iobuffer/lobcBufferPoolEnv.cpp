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

   Source File Name = lobcBufferPoolEnv.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/lobcBufferPoolEnv.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"
#include "vessel/pageDef.h"

namespace engine
{
namespace vessel
{
   lobcBufferPoolEnv::~lobcBufferPoolEnv()
   {
      fini();
   }

   INT32 lobcBufferPoolEnv::init(const lobcBufferPoolOptions &o)
   {
      INT32 rc = SDB_OK;
      fini();
      
      rc = _pool.init(o.maxMemSize, DMS_PAGE_SIZE32K);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init block-based mem pool:%d", rc);
         goto error;
      }
      _entries.resize(o.buckets);
      _mutexes.resize(o.bucketLatches, nullptr);
      for (UINT32 i = 0; i < _mutexes.size(); ++i)
      {
         std::mutex *m = new(std::nothrow) std::mutex();
         if (nullptr == m)
         {
            PD_LOG(PDERROR, "failed to allocate mem.");
            rc = SDB_OOM;
            goto error;
         }
         _mutexes[i] = m;
      }
   done:
      return rc;
   error:
      fini();
      goto done;
   }

   void lobcBufferPoolEnv::fini()
   {
      _pool.fini();
      _entries.clear();
      _entries.shrink_to_fit();
      for (UINT32 i = 0; i < _mutexes.size(); ++i)
      {
         if (nullptr != _mutexes[i])
         {
            delete _mutexes[i];
         }
      }
      _mutexes.clear();
      _mutexes.shrink_to_fit();
      _dirtyList.clear();
   }
} // namespace vessel

} // namespace engine
