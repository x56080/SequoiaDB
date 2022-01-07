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

   Source File Name = fixedSizeBatch.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/fixedSizeRowBatch.h"
#include "ossLikely.hpp"
#include "vessel/strictBuffer.h"

namespace engine
{
namespace vessel
{
   fixedSizeRowBatch::~fixedSizeRowBatch()
   {
      if (NULL != _buffer)
      {
         SDB_THREAD_FREE(_buffer);
         _buffer = NULL;
      }
   }

   void fixedSizeRowBatch::init(UINT32 bufferSize, BOOLEAN adaptFirstRow)
   {
      SDB_ASSERT(0 != bufferSize, "can not be zero");
      fini();

      _adaptFirstRow = adaptFirstRow;
      _bufferSize = bufferSize;
      _backOffset = _bufferSize;
      return;
   }

   UINT32 fixedSizeRowBatch::getRowCount()const
   {
      return _rowCount;
   }

   BOOLEAN fixedSizeRowBatch::isFreeToPush(UINT32 rowSize)const
   {
      BOOLEAN r = FALSE;
      if (rowBatch::hasRowLimit() &&
          (INT32)_rowCount == rowBatch::_rowLimit)
      {
         goto done;
      }
      else if (!_adaptFirstRow || 0 < _rowCount)
      {
         if (_backOffset < (getFrontOffset() + getSavingSize(rowSize)))
         {
            goto done;
         }
      }

      r = TRUE;
   done:
      return r;
   }

   INT32 fixedSizeRowBatch::pushRow(const slice &row)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(0 < _bufferSize, "init first");

      UINT32 savingSize = 0;
      strictBuffer buffer;
      _tag *tag = NULL;
      UINT32 offset = 0;

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
      
      savingSize = getSavingSize(row.getSize());

      if (NULL == _buffer || _bufferSize < savingSize)
      {
         rc = reallocBuffer(std::max(_bufferSize, savingSize));
         if (SDB_OK != rc)
         {
            goto error;
         }
      }

      buffer.makeWritable(_bufferSize, _buffer);
      offset = _backOffset - row.getSize();
      rc = buffer.write(offset, row.getSize(), row.data());
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to write data to [%d, %d]",
                offset, row.getSize());
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      tag = buffer.getWritableObjPtr<_tag>(getFrontOffset());
      SDB_ASSERT(NULL != tag, "impossible");
      tag->offset = offset;
      tag->size = row.getSize();
      _backOffset = offset;
      ++_rowCount;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 fixedSizeRowBatch::pushRowFragments(std::initializer_list<slice> il)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(0 < _bufferSize, "init first");

      UINT32 rowSize = 0;
      UINT32 savingSize = 0;
      _tag *tag = NULL;
      UINT32 offset = 0;
      UINT32 written = 0;
      strictBuffer buffer;

      if (0 == il.size())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      for (auto i = il.begin(); i != il.end(); ++i)
      {
         if (!i->isValid())
         {
            rc = SDB_INVALIDARG;
            goto error;
         }

         rowSize += i->getSize();
      }

      if (!isFreeToPush(rowSize))
      {
         rc = SDB_VESSEL_ROW_BATCH_LIMITS;
         goto error;
      }

      savingSize = getSavingSize(rowSize);

      if (NULL == _buffer || _bufferSize < savingSize)
      {
         rc = reallocBuffer(std::max(_bufferSize, savingSize));
         if (SDB_OK != rc)
         {
            goto error;
         }
      }

      buffer.makeWritable(_bufferSize, _buffer);
      offset = _backOffset - rowSize;
      for (auto i = il.begin(); i != il.end(); ++i)
      {
         rc = buffer.write(offset + written, i->getSize(),  i->data());
         if (OSS_UNLIKELY(SDB_OK != rc))
         {
            PD_LOG(PDERROR, "failed to write data to [%d, %d]",
                   offset + written, i->getSize());
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         written += i->getSize();
      }

      tag = buffer.getWritableObjPtr<_tag>(getFrontOffset());
      SDB_ASSERT(NULL != tag, "impossible");
      tag->offset = offset;
      tag->size = rowSize;
      _backOffset = offset;
      ++_rowCount;

   done:
      return rc;
   error:
      goto done;
   }

   slice fixedSizeRowBatch::getRow(UINT32 pos)const
   {
      SDB_ASSERT(pos < _rowCount, "out of bound");
      SDB_ASSERT(NULL != _buffer, "can not be null");
      strictBuffer buffer(_bufferSize, _buffer);
      slice result;
      const _tag *tag = buffer.getReadableObjPtr<_tag>(pos << 3);
      SDB_ASSERT(NULL != tag, "impossible");
      result.reset(tag->size, _buffer + tag->offset);
      return result;
   }

   void fixedSizeRowBatch::fini()
   {
      _adaptFirstRow = FALSE;
      _bufferSize = 0;
      if (NULL != _buffer)
      {
         SDB_THREAD_FREE(_buffer);
         _buffer = NULL;
      }
      _rowCount = 0;
      _backOffset = 0;
      _rowLimit = -1;
      _bufferSizeLimit = -1;
      return;
   }

   void fixedSizeRowBatch::clearRows()
   {
      _rowCount = 0;
      _backOffset = _bufferSize;
   }

   INT32 fixedSizeRowBatch::reallocBuffer(UINT32 size)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(0 == _rowCount, "must be empty");
      SDB_ASSERT(_bufferSize <= size, "can not be shrinked");

      if (hasBufferSizeLimit() &&
          _bufferSizeLimit < (INT32)size)
      {
         rc = SDB_VESSEL_OUT_OF_RESOURCE;
         goto error;
      }

      if (_bufferSize < size)
      {
         if (NULL != _buffer)
         {
            SDB_THREAD_FREE(_buffer);
            _buffer = NULL;
         }

         _bufferSize = size;
         _backOffset = size;
      }

      if (NULL == _buffer)
      {
         _buffer = (CHAR *)SDB_THREAD_ALLOC(_bufferSize);
         if (OSS_UNLIKELY(NULL == _buffer))
         {
            PD_LOG(PDERROR, "failed to allocate mem.");
            rc = SDB_OOM;
            goto error;
         }
      }

   done:
      return rc;
   error:
      goto done;
   }
} // namespace vessel

} // namespace engine

