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

   Source File Name = lsmIteratorBound.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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
