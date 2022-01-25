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

   Source File Name = bigRecordStream.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/31/2021  LYC  Initial Draft

   Last Changed =

******************************************************************************/
#include "vessel/bigRecordStream.h"

namespace engine
{
namespace vessel
{
   bigRecordStream::bigRecordStream(const slice &record)
   {
      SDB_ASSERT(record.isValid(), "can not be invalid");
      _recordData = record;
      _remainingSize = record.getSize();
      _sliceAddrs.clear();
   }

   bigRecordStream::bigRecordStream(const bigRecordStream &s)
   {
      _recordData = s._recordData;
      _remainingSize = s._remainingSize;
      _sliceAddrs = s._sliceAddrs;
   }

   bigRecordStream &bigRecordStream::operator=(const bigRecordStream &s)
   {
      _recordData = s._recordData;
      _remainingSize = s._remainingSize;
      _sliceAddrs = s._sliceAddrs;
      return *this;
   }

   void bigRecordStream::reset()
   {
      _recordData.reset();
      _remainingSize = 0;
      _sliceAddrs.clear();
   }

   BOOLEAN bigRecordStream::isValid()const
   {
      return _recordData.isValid() &&
             _recordData.getSize() >= _remainingSize;
   }

   const CHAR *bigRecordStream::reserveSlice(UINT32 sliceSize)
   {
      SDB_ASSERT(sliceSize <= _remainingSize, "invalid slice size");
      UINT32 offset = _remainingSize - sliceSize;
      _remainingSize -= sliceSize;
      _sliceAddrs.push_back(recordID());
      return _recordData.getData() + offset;
   }

   void bigRecordStream::fillLastSlice(const recordID &rid)
   {
      SDB_ASSERT(rid.isValid(), "can not be invalid");
      _sliceAddrs.back() = rid;
   }

   recordID bigRecordStream::getLastSliceAddr()const
   {
      if (_sliceAddrs.empty())
      {
         return recordID();
      }
      return _sliceAddrs.back();
   }

   UINT32 bigRecordStream::getRecordSize()const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      return _recordData.getSize();
   }

   recordID bigRecordStream::getSliceAddr(UINT32 pos)const
   {
      SDB_ASSERT(pos < getSliceCount(), "out of bound");
      return _sliceAddrs[pos];
   }
}
}