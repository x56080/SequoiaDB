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
   dpsJournalEntry::dpsJournalEntry(const CHAR *rawdata)
   {
      _loadData(rawdata);
   }

   dpsJournalEntry::~dpsJournalEntry()
   {
      if (nullptr != _bufferOwned)
      {
         SDB_THREAD_FREE(_bufferOwned);
      }
   }

   void dpsJournalEntry::reset()
   {
      _clearDataLoaded();
      if (nullptr != _bufferOwned)
      {
         SDB_THREAD_FREE(_bufferOwned);
         _bufferOwned = nullptr;
      }
   }

   INT32 dpsJournalEntry::getOwned()
   {
      INT32 rc = SDB_OK;
      if (isValid() && !isOwned())
      {
         _bufferOwned = (CHAR *)SDB_THREAD_ALLOC(_header->_length);
         if (OSS_UNLIKELY(nullptr == _bufferOwned))
         {
            PD_LOG(PDERROR, "failed to allocate mem.");
            rc = SDB_OOM;
            goto error;
         }

         ossMemcpy(_bufferOwned, _header, _header->_length);
         _header = reinterpret_cast<const dpsLogRecordHeader *>(_bufferOwned);
      }
   done:
      return rc;
   error:
      goto done;
   }

   dpsJournalEntry::element dpsJournalEntry::getElement(UINT32 pos)const
   {
      dpsJournalEntry::element ele;
      if (OSS_LIKELY(pos < _elementCount))
      {
         ele._meta = _elements[pos];
         if (0 < ele._meta.len)
         {
            ele._val = reinterpret_cast<const CHAR *>(_header) + _values[pos];
         }
      }
      else
      {
         SDB_ASSERT(FALSE, "out of bound");
      }
      return ele;
   }

   INT32 dpsJournalEntry::_loadData(const CHAR *rawdata)
   {
      INT32 rc = SDB_OK;
      UINT32 offset = sizeof(dpsLogRecordHeader);
      _clearDataLoaded();

      if (OSS_UNLIKELY(nullptr == rawdata))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _header = reinterpret_cast<const dpsLogRecordHeader *>(rawdata);
      if (_header->_length < sizeof(dpsLogRecordHeader) ||
          DPS_RECORD_MAX_LEN < _header->_length)
      {
         PD_LOG ( PDERROR, "the length of record is out of range: %d",
                  _header->_length ) ;
         rc = SDB_DPS_CORRUPTED_LOG ;
         goto error ;
      }

      if (LOG_TYPE_DUMMY == _header->_type)
      {
         goto done;
      }

      while ((offset + sizeof(dpsRecordEle)) <= _header->_length)
      {
         const dpsRecordEle *ele =
               reinterpret_cast<const dpsRecordEle *>(rawdata + offset);
         if (DPS_MERGE_BLOCK_MAX_DATA == _elementCount)
         {
            PD_LOG( PDERROR, "data num is larger than %d", DPS_MERGE_BLOCK_MAX_DATA ) ;
            SDB_ASSERT( FALSE, "impossible" ) ;
            rc = SDB_DPS_CORRUPTED_LOG ;
            goto error ;
         }
         else if (DPS_INVALID_TAG == ele->tag)
         {
            break;
         }
         else if (_header->_length < (offset + sizeof(dpsRecordEle) + ele->len))
         {
            PD_LOG( PDERROR, "get a invalid value size:%d, total size: %d, "
                    "load size: %d", ele->len, _header->_length, offset) ;
            rc = SDB_DPS_CORRUPTED_LOG ;
            goto error ;
         }

         offset += sizeof(dpsRecordEle);
         _elements[_elementCount] = *ele;
         _values[_elementCount] = offset;
         ++_elementCount;
         offset += ele->len;
      }
   done:
      return rc;
   error:
      _clearDataLoaded();
      goto done;
   }

   void dpsJournalEntry::_clearDataLoaded()
   {
      _header = nullptr;
      for (UINT32 i = 0; i < _elementCount; ++i)
      {
         _elements[i].tag = DPS_INVALID_TAG;
         _elements[i].len = 0;
         _values[i] = 0;
      }
      _elementCount = 0;
   }

} // namespace engine
