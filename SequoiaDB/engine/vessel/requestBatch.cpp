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

   Source File Name = requestBatch.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/requestBatch.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"
#include "ossTypes.hpp"

namespace engine
{
namespace vessel
{
   INT32 requestBatch::add(STRIPING_ID striping,
                           const slice &data)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(data.isEmpty()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _batch.append(std::make_pair(striping, data));
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append to batch:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 requestBatch::addObjs(UINT32 count, const BSONObj &objs)
   {
      INT32 rc = SDB_OK;
      UINT32 size = _batch.size();
      const CHAR *ptr = NULL;

      if (OSS_UNLIKELY(0 == count))
      {
         goto done;
      }
      else if (OSS_UNLIKELY(objs.isEmpty()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      ptr = objs.objdata();
      for (UINT32 i = 0; i < count; ++i)
      {
         try
         {
            bson::BSONObj obj(ptr);
            _batch.append(std::make_pair(INVALID_STRIPING_ID, slice(obj.objsize(), ptr)));
            ptr += ossAlign4((UINT32)(obj.objsize()));
         }
         catch(const std::exception& e)
         {
            PD_LOG (PDERROR, "failed to convert to BSON:%s", e.what() );
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
      }
   done:
      return rc;
   error:
      while (_batch.size() > size)
      {
         _batch.popBack();
      }
      goto done;
   }

   slice requestBatch::get(UINT32 pos, STRIPING_ID &striping)const
   {
      slice data;
      striping = INVALID_STRIPING_ID;
      if (OSS_LIKELY(pos < _batch.size()))
      {
         striping = _batch[pos].first;
         data = _batch[pos].second;
      }
      return data;
   }
} // namespace vessel

} // namespace engine
