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

   Source File Name = lsmIteratorBound.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/
#include "vessel/lsm/lsmIteratorBound.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"
#include "vessel/keyStringBuilder.h"

namespace engine
{
namespace vessel
{
   lsmIteratorBound::lsmIteratorBound(const lsmIteratorBound &o)
   {
      if (o.isValid())
      {
         _indexId = o._indexId;
         ossMemcpy(_lowBuf, o._lowBuf, _BOUND_BUF_SIZE);
         ossMemcpy(_upBuf, o._upBuf, _BOUND_BUF_SIZE);
      }
   }

   lsmIteratorBound &lsmIteratorBound::operator=(const lsmIteratorBound &o)
   {
      reset();
      if (o.isValid())
      {
         _indexId = o._indexId;
         ossMemcpy(_lowBuf, o._lowBuf, _BOUND_BUF_SIZE);
         ossMemcpy(_upBuf, o._upBuf, _BOUND_BUF_SIZE);
      }
      return *this;
   }

   INT32 lsmIteratorBound::init(const globalIndexID &id)
   {
      INT32 rc = SDB_OK;
      UINT32 size = 0;
      reset();

      if (OSS_UNLIKELY(!id.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = STACK_KEY_STRING_BUILDER::buildBoundaryKey(id, FALSE, _BOUND_BUF_SIZE,
                                                      _lowBuf, size);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build low bound:%d", rc);
         goto error;
      }

      SDB_ASSERT(_BOUND_BUF_SIZE == size, "must be same");

      rc = STACK_KEY_STRING_BUILDER::buildBoundaryKey(id, TRUE, _BOUND_BUF_SIZE,
                                                      _upBuf, size);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build up bound:%d", rc);
         goto error;
      }

      SDB_ASSERT(_BOUND_BUF_SIZE == size, "must be same");

      _indexId = id;
   done:
      return rc;
   error:
      goto done;
   }

   slice lsmIteratorBound::getEncodedIndexId()const
   {
      return isValid() ?
             slice(keyStringCoder::INDEX_ID_ENCODEING_SIZE, _lowBuf) :
             slice();
   }
} // namespace vessel
  
} // namespace engine
