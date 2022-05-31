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

   Source File Name = lobcBufferPoolEnv.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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
