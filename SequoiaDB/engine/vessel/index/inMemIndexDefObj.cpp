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

   Source File Name = inMemIndexDefObj.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/inMemIndexDefObj.h"
#include "vessel/indexDef.h"
#include "../bson/bson.hpp"

namespace engine
{
namespace vessel
{
   UINT32 inMemIndexDefObj::estimate(const strSlice &indexName,
                                     const indexKeyPattern &keyPattern)
   {
      return INDEX_DEF_RECORD_LEN +
             indexName.strLen() + 1 +
             keyPattern.getPattern().objsize();
   }

   inMemIndexDefObj::inMemIndexDefObj()
   {}

   inMemIndexDefObj::~inMemIndexDefObj()
   {
      SAFE_OSS_FREE(_buffer);
   }

   void inMemIndexDefObj::reset()
   {
      _record = indexDefRecord();
      _indexName.reset();
      _keyPattern.reset();
      return;
   }

   INT32 inMemIndexDefObj::set(const indexDefRecord &record,
                               const strSlice &indexName,
                               const bson::BSONObj &pattern)
   {
      INT32 rc = SDB_OK;
      UINT32 bufferSize = indexName.strLen() + 1 + pattern.objsize();
      reset();
      if (!record.isValid() ||
          indexName.empty() ||
          !pattern.isValid())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (record.indexNameLen != (indexName.strLen() + 1))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if ((INT32)record.keyPatternLen != pattern.objsize())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = ensureBuffer(bufferSize);
      if (SDB_OK != rc)
      {
         goto error;
      }

      _record = record;
      ossMemcpy(_buffer, indexName.str(), indexName.strLen() + 1);
      _record.indexNameOffset = 0;
      _indexName.reset(_buffer, indexName.strLen());

      ossMemcpy((_buffer + indexName.strLen() + 1),
                 pattern.objdata(),
                 pattern.objsize());
      _record.keyPatternOffset = indexName.strLen() + 1;
      rc = _keyPattern.set(bson::BSONObj(_buffer + indexName.strLen() + 1, FALSE));
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to set key pattern:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      reset();
      goto done;
   }

   INT32 inMemIndexDefObj::ensureBuffer(UINT32 bufferSize)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(0 != bufferSize, "impossible");
      if (bufferSize <= _bufferSize)
      {
         goto done;
      }
      
      SAFE_OSS_FREE(_buffer);
      bufferSize = 0;
      _buffer = (CHAR *)SDB_THREAD_ALLOC(bufferSize);
      if (NULL == _buffer)
      {
         rc = SDB_OOM;
         PD_LOG(PDERROR, "faield to allocate mem");
         goto error;
      }

      _bufferSize = bufferSize;
      ossMemset(_buffer, 0, bufferSize);
   done:
      return rc;
   error:
      goto done;
   }

}//namespace vessel
}//namespace engine
