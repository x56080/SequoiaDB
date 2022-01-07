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

   Source File Name = elasticBlockRowBatch.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/elasticBlockRowBatch.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"
#include "vessel/strictBuffer.h"

namespace engine
{
namespace vessel
{
   constexpr UINT32 _DEFAULT_BLOCK_SIZE = 65536;

   void elasticBlockRowBatch::fini()
   {
      _defaultBlockSize = 0;
      _pad.fini();
      _block.release();
   }

   void elasticBlockRowBatch::clearRows()
   {
      _pad.clear();
   }

   void elasticBlockRowBatch::setDefaultBlockSize(UINT32 defaultBlockSize)
   {
      SDB_ASSERT(0 < defaultBlockSize, "can not be invalid");
      _defaultBlockSize = defaultBlockSize;
   }

   UINT32 elasticBlockRowBatch::getRowCount()const
   {
      return _pad.getCount();
   }

   BOOLEAN elasticBlockRowBatch::isFreeToPush(UINT32 rowSize)const
   {
      BOOLEAN r = FALSE;
      if (hasRowLimit() && _pad.getCount() == _rowLimit)
      {
         r = FALSE;
      }
      else if (_pad.isFreeToPush(rowSize))
      {
         r = TRUE;
      }
      else if (hasBufferSizeLimit())
      {
         UINT32 nextMinBlockSize = fixedSizeDataPad::getSavingSize(rowSize) +
                                   _pad.getUnfreeSize();
         r = nextMinBlockSize <= _bufferSizeLimit;
      }
      else
      {
         r = TRUE;
      }
   done:
      return r;
   }

   INT32 elasticBlockRowBatch::pushRow(const slice &row)
   {
      INT32 rc = SDB_OK;
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
      else if (!_pad.isFreeToPush(row.getSize()))
      {
         rc = extendPadSize(row.getSize());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to extend block size: %d", rc);
            goto error;
         }
      }

      rc = _pad.push(row);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to push row into pad:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 elasticBlockRowBatch::pushRowFragments(std::initializer_list<slice> il)
   {
      INT32 rc = SDB_OK;
      UINT32 size = 0;

      for (auto i = il.begin(); i != il.end(); ++i)
      {
         if (!i->isValid())
         {
            SDB_ASSERT(FALSE, "invalid fragment");
            rc = SDB_INVALIDARG;
            goto error;
         }
         size += i->getSize();
      }

      if (!isFreeToPush(size))
      {
         rc = SDB_VESSEL_ROW_BATCH_LIMITS;
         goto error;
      }
      else if (!_pad.isFreeToPush(size))
      {
         rc = extendPadSize(size);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to extend block size: %d", rc);
            goto error;
         }
      }

      rc = _pad.pushRowFragments(il);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to push row into pad:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   slice elasticBlockRowBatch::getRow(UINT32 pos)const
   {
      return _pad.getRow(pos);
   }

   INT32 elasticBlockRowBatch::extendPadSize(UINT32 newRowSize)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(0 < newRowSize, "can not be invalid");
      fixedSizeDataPad pad;
      memoryBlock block;
      UINT32 deafultBlockSize = 0 == _defaultBlockSize ?
                                _DEFAULT_BLOCK_SIZE : _defaultBlockSize;
      UINT32 blockSize = (0 == _block.getCapacity()) ?
                         _defaultBlockSize : (_block.getCapacity() << 1);
      UINT32 minSize = _pad.getUnfreeSize() + fixedSizeDataPad::getSavingSize(newRowSize);
      if (blockSize < minSize)
      {
         blockSize = minSize;
      }

      rc = block.reserve(blockSize);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to reserve space[%d] in block:%d", blockSize, rc);
         goto error;
      }

      pad.init(block.getCapacity(), block.getBuffer());
      if (0 < _pad.getCount())
      {
         rc = pad.overwrite(_pad);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to overwrite new pad:%d", rc);
            goto error;
         }
      }

      _pad = std::move(pad);
      _block = std::move(block);

   done:
      return rc;
   error:
      goto done;
   }

} // namespace vessel

} // namespace engine
