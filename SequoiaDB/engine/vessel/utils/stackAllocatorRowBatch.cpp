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

   Source File Name = stackAllocatorRowBatch.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/stackAllocatorRowBatch.h"
#include "ossLikely.hpp"

namespace engine
{
namespace vessel
{
   stackAllocatorRowBatch::~stackAllocatorRowBatch()
   {
      fini();
   }

   void stackAllocatorRowBatch::fini()
   {
      _tags.clear();
      _builder.kill();
   }

   void stackAllocatorRowBatch::clearRows()
   {
      _tags.clear();
      _builder.reset();
   }

   INT32 stackAllocatorRowBatch::pushRow(const slice &row)
   {
      INT32 rc = SDB_OK;
      _TAG tag;

      if (OSS_UNLIKELY(!row.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (!isFreeToPush(row.getSize()))
      {
         rc = SDB_VESSEL_ROW_BATCH_LIMITS;
         goto error;
      }

      tag.first = _builder.len();
      tag.second = row.getSize();
      _tags.push_back(tag);
      _builder.appendBuf(row.data(), row.getSize());
   done:
      return rc;
   error:
      goto done;
   }

   INT32 stackAllocatorRowBatch::pushRowFragments(std::initializer_list<slice> il)
   {
      INT32 rc = SDB_OK;
      _TAG tag;
      tag.first = _builder.len();
      tag.second = 0;

      for (auto i = il.begin(); i != il.end(); ++i)
      {
         if (!i->isValid())
         {
            SDB_ASSERT(FALSE, "invalid fragment");
            rc = SDB_INVALIDARG;
            goto error;
         }
         tag.second += i->getSize();
      }

      if (!isFreeToPush(tag.second))
      {
         rc = SDB_VESSEL_ROW_BATCH_LIMITS;
         goto error;
      }

      _tags.push_back(tag);
      for (auto i = il.begin(); i != il.end(); ++i)
      {
         _builder.appendBuf(i->data(), i->getSize());
      }
   done:
      return rc;
   error:
      goto done;
   }

   slice stackAllocatorRowBatch::getRow(UINT32 pos)const
   {
      slice s;
      if (OSS_LIKELY(pos < _tags.size()))
      {
         const _TAG &tag = _tags.at(pos);
         s.reset(tag.second, _builder.buf() + tag.first);
      }
      else
      {
         SDB_ASSERT(FALSE, "out of bound");
      }
      return s;
   }

   BOOLEAN stackAllocatorRowBatch::isFreeToPush(UINT32 rowSize)const
   {
      BOOLEAN r = TRUE;
      if (0 <= _bufferSizeLimit &&
         (INT64)(_builder.len() + rowSize) > _bufferSizeLimit)
      {
         r = FALSE;
         goto done;
      }

      if (0 <= _rowLimit && (INT32)(_tags.size()) == _rowLimit)
      {
         r = FALSE;
         goto done;
      }

   done:
      return r;
   }

} // namespace vessel

} // namespace engine
