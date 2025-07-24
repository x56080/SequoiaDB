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

   Source File Name = elasticBlockRowBatch.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_ELASTIC_BLOCK_ROW_BATCH_HPP_
#define VESSEL_ELASTIC_BLOCK_ROW_BATCH_HPP_

#include "vessel/rowBatch.h"
#include "vessel/fixedSizeDataPad.h"
#include "utilAllocator.hpp"
#include "pdTrace.hpp"
#include "ossLikely.hpp"

namespace engine
{
namespace vessel
{
   template<class Allocator>
   class elasticBlockRowBatch : public rowBatch
   {
      public:
         elasticBlockRowBatch() = default;
         virtual ~elasticBlockRowBatch();

      public:
         void setDefaultBlockSize(UINT32 defaultBlockSize)
         {
            SDB_ASSERT(0 < defaultBlockSize, "can not be invalid");
            _defaultBlockSize = defaultBlockSize;
         }

      public:
         virtual UINT32 getRowCount()const override;
         virtual BOOLEAN isFreeToPush(UINT32 rowSize)const override;
         virtual INT32 pushRow(const slice &row) override;
         virtual INT32 pushRowFragments(std::initializer_list<slice> il) override;
         virtual slice getRow(UINT32 pos)const override;
         virtual void fini() override;
         virtual void clearRows() override;

      private:
         INT32 extendPadSize(UINT32 newRowSize);
         UINT32 getMinBufferSize(UINT32 newRowSize)const;

      private:
         UINT32 _defaultBlockSize = 0;
         Allocator _allocator;
         CHAR *_buffer = nullptr;
         UINT32 _bufferSize = 0;
         fixedSizeDataPad _pad;
   };//class elasticBlockRowBatch

   template<class Allocator>
   elasticBlockRowBatch<Allocator>::~elasticBlockRowBatch()
   {
      if (nullptr != _buffer)
      {
         _allocator.free(_buffer);
      }
   }

   template<class Allocator>
   UINT32 elasticBlockRowBatch<Allocator>::getRowCount()const
   {
      return _pad.isValid() ? _pad.getRowCount() : 0;
   }

   template<class Allocator>
   BOOLEAN elasticBlockRowBatch<Allocator>::isFreeToPush(UINT32 rowSize)const
   {
      BOOLEAN r = FALSE;
      if (0 == _bufferSize)
      {
         UINT32 size = fixedSizeDataPad::getMinBufferSizeInit(rowSize);
         r = (!hasBufferSizeLimit() || size <= _bufferSizeLimit);
      }
      else if (hasRowLimit() && _pad.getRowCount() == _rowLimit)
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

      return r;
   }

   template<class Allocator>
   INT32 elasticBlockRowBatch<Allocator>::pushRow(const slice &row)
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

   template<class Allocator>
   INT32 elasticBlockRowBatch<Allocator>::pushRowFragments(std::initializer_list<slice> il)
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

   template<class Allocator>
   slice elasticBlockRowBatch<Allocator>::getRow(UINT32 pos)const
   {
      return _pad.isValid() ? _pad.getRow(pos) : slice();
   }

   template<class Allocator>
   void elasticBlockRowBatch<Allocator>::fini()
   {
      _defaultBlockSize = 0;
      _pad.fini();
      if (nullptr != _buffer)
      {
         _allocator.free(_buffer);
         _buffer = nullptr;
      }
      _bufferSize = 0;
      _rowLimit = 0;
      _bufferSizeLimit = 0;
      return;
   }

   template<class Allocator>
   void elasticBlockRowBatch<Allocator>::clearRows()
   {
      if (_pad.isValid())
      {
         _pad.resetBuffer();
      }
   }

   template<class Allocator>
   INT32 elasticBlockRowBatch<Allocator>::extendPadSize(UINT32 newRowSize)
   {
      INT32 rc = SDB_OK;
      constexpr UINT32 _DEFAULT_NONFAST_BUFFER_SIZE = 65536;
      SDB_ASSERT(0 < newRowSize, "can not be invalid");

      fixedSizeDataPad pad;
      CHAR *buffer = nullptr;
      UINT32 minBufferSize = getMinBufferSize(newRowSize);
      UINT32 bufferSize = 0;
      UINT32 defaultBufferSize = _defaultBlockSize;
      if (0 == defaultBufferSize)
      {
         defaultBufferSize = (0 == _allocator.getFastAllocSize()) ?
                             _DEFAULT_NONFAST_BUFFER_SIZE : _allocator.getFastAllocSize();
      }

      bufferSize = 0 == _bufferSize ?
                   defaultBufferSize : (_bufferSize << 1);
      if (bufferSize < minBufferSize)
      {
         bufferSize = minBufferSize;
      }

      buffer = (CHAR *)_allocator.malloc(bufferSize);
      if (OSS_UNLIKELY(nullptr == buffer))
      {
         PD_LOG(PDERROR, "failed to allocate mem.");
         rc = SDB_OOM;
         goto error;
      }

      rc = pad.init(bufferSize, buffer, TRUE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init data pad:%d", rc);
         goto error;
      }

      if (_pad.isValid())
      {
         rc = pad.overwrite(_pad);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to overwrite pad data:%d", rc);
            goto error;
         }
      }

      std::swap(_buffer, buffer);
      _bufferSize = bufferSize;
      _pad = pad;
                          
   done:
      if (nullptr != buffer)
      {
         _allocator.free(buffer);
      }
      return rc;
   error:
      goto done;
   }

   template<class Allocator>
   UINT32 elasticBlockRowBatch<Allocator>::getMinBufferSize(UINT32 newRowSize)const
   {
      return _pad.isValid() ?
             (_pad.getUnfreeSize() + fixedSizeDataPad::getSavingSize(newRowSize)) :
             fixedSizeDataPad::getMinBufferSizeInit(newRowSize);
   }

   using STACK_ELASTIC_BLOCK_ROW_BATCH = elasticBlockRowBatch<utilStackAllocator<>>;
   using ELASTIC_BLOCK_ROW_BATCH = elasticBlockRowBatch<utilPoolAllocator>;
} // namespace vessel

} // namespace engine


#endif//VESSEL_ELASTIC_BLOCK_ROW_BATCH_HPP_