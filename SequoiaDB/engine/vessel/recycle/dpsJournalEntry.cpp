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

   Source File Name = dpsJournalEntry.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "dpsJournalEntry.hpp"
#include "ossLikely.hpp"
#include "pdTrace.hpp"

namespace engine
{
   dpsJournalEntry::dpsJournalEntry(const CHAR *rawdata,
                                    UINT32 bufferSize)
   {
      load(rawdata, bufferSize);
   }

   dpsJournalEntry::dpsJournalEntry(const dpsJournalEntry &o)
   {
      if (o.isValid())
      {
         _header = o._header;
         _elementCount = o._elementCount;
         for (UINT32 i = 0; i < _elementCount; ++i)
         {
            _elements[i] = o._elements[i];
            _values[i] = o._values[i];
         }
         _elementBuffer = o._elementBuffer;
      }
   }

   dpsJournalEntry &dpsJournalEntry::operator=(const dpsJournalEntry &o)
   {
      reset();
      if (o.isValid())
      {
         _header = o._header;
         _elementCount = o._elementCount;
         for (UINT32 i = 0; i < _elementCount; ++i)
         {
            _elements[i] = o._elements[i];
            _values[i] = o._values[i];
         }
         _elementBuffer = o._elementBuffer;
      }
      return *this;
   }

   void dpsJournalEntry::reset()
   {
      _header.clear();
      for (UINT32 i = 0; i < _elementCount; ++i)
      {
         _elements[i].tag = DPS_INVALID_TAG;
         _elements[i].len = 0;
         _values[i] = 0;
      }
      _elementCount = 0;
      _elementBuffer = nullptr;
      return;
   }

   dpsJournalEntry::element dpsJournalEntry::getElement(UINT32 pos)const
   {
      dpsJournalEntry::element ele;
      if (OSS_LIKELY(pos < _elementCount))
      {
         ele._meta = _elements[pos];
         if (0 < ele._meta.len)
         {
            ele._value = _elementBuffer + _values[pos];
         }
      }
      else
      {
         SDB_ASSERT(FALSE, "out of bound");
      }
      return ele;
   }

   INT32 dpsJournalEntry::load(const CHAR *rawdata, UINT32 size)
   {
      INT32 rc = SDB_OK;
      reset();

      if (OSS_UNLIKELY(nullptr == rawdata ||
                       size < DPS_LOG_HEAD_SIZE))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _header = *reinterpret_cast<const dpsLogRecordHeader *>(rawdata);
      if (_header._length < DPS_LOG_HEAD_SIZE ||
          DPS_RECORD_MAX_LEN < _header._length)
      {
         PD_LOG ( PDERROR, "the length of record is out of range: %d",
                  _header._length ) ;
         rc = SDB_DPS_CORRUPTED_LOG ;
         goto error ;
      }
      else if (size < _header._length)
      {
         PD_LOG(PDERROR, "invalid buffer size[%d] to parse log record[%d]",
                size, _header._length);
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (LOG_TYPE_DUMMY == _header._type)
      {
         goto done;
      }

      rc = _loadElements(rawdata + DPS_LOG_HEAD_SIZE,
                         size - DPS_LOG_HEAD_SIZE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to load elements:%d", rc);
         goto error;
      }
   done:
      reset();
      return rc;
   error:
      goto done;
   }

   INT32 dpsJournalEntry::init(UINT16 type,
                               UINT16 flags,
                               UINT32 elementRawSize,
                               const CHAR *elementRawData)
   {
      INT32 rc = SDB_OK;
      reset();

      if (OSS_UNLIKELY(LOG_TYPE_DUMMY == type ||
                       elementRawSize < sizeof(dpsRecordEle) ||
                       nullptr == elementRawData))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _header._type = type;
      _header._flags = flags;
      _header._length = DPS_LOG_HEAD_SIZE + elementRawSize;

      rc = _loadElements(elementRawData, elementRawSize);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to load elements:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      reset();
      goto done;
   }

   INT32 dpsJournalEntry::_loadElements(const CHAR *elementRawData,
                                        UINT32 size)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != elementRawData, "can not be null");
      SDB_ASSERT(sizeof(dpsRecordEle) <= size, "can not be invalid");

      UINT32 offset = 0;
      _elementBuffer = elementRawData;
      _elementCount = 0;

      while ((offset + sizeof(dpsRecordEle)) < size)
      {
         const dpsRecordEle *ele =
               reinterpret_cast<const dpsRecordEle *>(_elementBuffer + offset);
         if (DPS_MERGE_BLOCK_MAX_DATA == _elementCount)
         {
            PD_LOG( PDERROR, "data num is larger than %d", DPS_MERGE_BLOCK_MAX_DATA ) ;
            rc = SDB_DPS_CORRUPTED_LOG ;
            goto error ;
         }
         else if (DPS_INVALID_TAG == ele->tag)
         {
            /// tag may be reset as the ending one.
            break;
         }
         else if (size < (offset + sizeof(dpsRecordEle) + ele->len))
         {
            PD_LOG( PDERROR, "get a invalid value size:%d, total element size: %d, "
                    "load size: %d", ele->len, size, offset) ;
            rc = SDB_DPS_CORRUPTED_LOG ;
            goto error ;
         }

         _elements[_elementCount] = *ele;
         offset += sizeof(dpsRecordEle);
         _values[_elementCount] = offset;
         ++_elementCount;
         offset += ele->len;
      }
   done:
      return rc;
   error:
      _elementBuffer = nullptr;
      _elementCount = 0;
      goto done;
   }
} // namespace engine
