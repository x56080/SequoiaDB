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

   Source File Name = dpsWriteReqBuilder.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "dpsWriteReqBuilder.hpp"

namespace engine
{
   constexpr UINT32 _DEFAULT_BUFFER_SIZE = 8192;

   dpsWriteReqBuilder::dpsWriteReqBuilder()
   {
      utilFragAllocator::options ao;
      ao.defaultBlockSize = _DEFAULT_BUFFER_SIZE;
      _allocator.setOptions(ao);
   }

   dpsWriteReqBuilder::dpsWriteReqBuilder(const options &o)
   {
      SDB_ASSERT(0 < o.defaultBufferBlockSize, "can not be invalid");
      utilFragAllocator::options ao;
      ao.defaultBlockSize = o.defaultBufferBlockSize;
      _allocator.setOptions(ao);
   }

   void dpsWriteReqBuilder::reset()
   {
      _type = LOG_TYPE_DUMMY;
      _flags = 0;
      _totalDataSize = 0;
      _elementNum = 0;
      _elements = nullptr;
      _allocator.resetBlocks();

      return;
   }

   dpsWriteRequest dpsWriteReqBuilder::reap()
   {
      dpsWriteRequest req;

      req._type = _type;
      req._flags = _flags;
      req._totalDataSize = _totalDataSize;
      req._elementNum = _elementNum;
      req._elements = _elements;
      req._rep = _allocator.reap();

      reset();

      return std::move(req);
   }

   INT32 dpsWriteReqBuilder::append(DPS_TAG tag, UINT32 size, const void *data)
   {
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(DPS_INVALID_TAG == tag ||
                       0 == size ||
                       nullptr == data))
      {
         SDB_ASSERT(FALSE, "invalid arg");
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(DPS_MERGE_BLOCK_MAX_DATA == _elementNum))
      {
         SDB_ASSERT(FALSE, "out of max element cout");
         rc = SDB_DPS_CORRUPTED_LOG;
         goto error;
      }
      else
      {
         CHAR *buffer = nullptr;
         UINT32 bufferSize = getElementBufferSize(size);
      #if defined (_DEBUG)
         SDB_ASSERT(!_isTagDuplicated(tag), "duplicated tag");
      #endif//_DEBUG
         
         rc = _ensureMetaBlock();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to ensure element meta block:%d", rc);
            goto error;
         }

         buffer = (CHAR *)_allocator.malloc(bufferSize);
         if (OSS_UNLIKELY(nullptr == buffer))
         {
            PD_LOG(PDERROR, "failed to allocate mem.");
            rc = SDB_OOM;
            goto error;
         }

         reinterpret_cast<dpsRecordEle *>(buffer)->tag = tag;
         reinterpret_cast<dpsRecordEle *>(buffer)->len = size;
         ossMemcpy(buffer + sizeof(dpsRecordEle), data, size);
         _elements[_elementNum].reset(bufferSize, buffer);
         ++_elementNum;
         _totalDataSize += bufferSize;
      }
   done:
      return rc;
   error:
      /// no need to free buffer here, it managed by allocator.
      goto done;
   }

   BOOLEAN dpsWriteReqBuilder::_isTagDuplicated(DPS_TAG tag)const
   {
      BOOLEAN r = FALSE;
      SDB_ASSERT(DPS_INVALID_TAG != tag, "can not be invalid");

      for (UINT32 i = 0; i < _elementNum; ++i)
      {
         const dpsRecordEle *ele = _elements[i].castTo<dpsRecordEle>();
         if (tag == ele->tag)
         {
            r = TRUE;
            break;
         }
      }
      return r;
   }

   INT32 dpsWriteReqBuilder::_ensureMetaBlock()
   {
      INT32 rc = SDB_OK;
      if (nullptr == _elements)
      {
         constexpr UINT32 bufferSize = DPS_MERGE_BLOCK_MAX_DATA * sizeof(utilSlice);
         _elements = (utilSlice *)_allocator.malloc(bufferSize);
         if (OSS_UNLIKELY(nullptr == _elements))
         {
            PD_LOG(PDERROR, "failed to allocate mem.");
            rc = SDB_OOM;
            goto error;
         }

         ///WARNING: slices will be reset when appending,
         ///wild ptr may saved in _elements now.
      }
   done:
      return rc;
   error:
      goto done;
   }
} // namespace engine
