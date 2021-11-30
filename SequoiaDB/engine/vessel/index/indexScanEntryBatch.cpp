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

   Source File Name = indexScanEntryBatch.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/indexScanEntryBatch.h"
#include "ossLikely.hpp"

namespace engine
{
namespace vessel
{
   void indexScanEntryBatch::reset()
   {
      _entries.clear();
      _buffer.reset();
      return;
   }

   INT32 indexScanEntryBatch::addFragmentsOfOneEntry(std::initializer_list<slice> il)
   {
      INT32 rc = SDB_OK;
      UINT32 size = 0;
      UINT32 offset = _buffer.len();
      for (auto i = il.begin(); i != il.end(); ++i)
      {
         if (!i->isValid())
         {
            PD_LOG(PDERROR, "can not add empty fragment");
            rc = SDB_INVALIDARG;
            goto error;
         }

         size += i->getSize();
      }

      rc = _entries.append(std::make_pair(offset, size));
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append entry:%d", rc);
         goto error;
      }

      for (auto i = il.begin(); i != il.end(); ++i)
      {
         _buffer.appendBuf(i->data(), i->getSize());
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 indexScanEntryBatch::addEntry(const slice &entry)
   {
      INT32 rc = SDB_OK;
      UINT32 offset = _buffer.len();
      if (OSS_UNLIKELY(!entry.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _entries.append(std::make_pair(offset, entry.getSize()));
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append entry:%d", rc);
         goto error;
      }

      _buffer.appendBuf(entry.data(), entry.getSize());

   done:
      return rc;
   error:
      goto done;
   }

   slice indexScanEntryBatch::operator[](UINT32 pos)const
   {
      SDB_ASSERT(pos < _entries.size(), "out of bound");
      const _ENTRY_INFO &info = _entries[pos];
      const CHAR *buf = _buffer.buf();
      return slice(info.second, buf + info.first);
   }

   slice indexScanEntryBatch::getLastEntry()const
   {
      SDB_ASSERT(0 < _entries.size(), "can not be empty");
      const _ENTRY_INFO &info = _entries[_entries.size() - 1];
      const CHAR *buf = _buffer.buf(); 
      return slice(info.second, buf + info.first);
   }
} // namespace vessel

} // namespace engine
