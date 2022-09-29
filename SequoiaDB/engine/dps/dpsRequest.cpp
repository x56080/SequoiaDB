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

   Source File Name = dpsRequest.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "dpsRequest.hpp"
#include "ossLikely.hpp"
#include "pdTrace.hpp"

namespace engine
{
   dpsWriteRequest::dpsWriteRequest(dpsWriteRequest &&o):
   _type(o._type),
   _flags(o._flags),
   _totalDataSize(o._totalDataSize),
   _elementNum(o._elementNum),
   _elements(o._elements),
   _rep(std::move(o._rep))
   {
      o.reset();
   }

   dpsWriteRequest &dpsWriteRequest::operator=(dpsWriteRequest &&o)
   {
      reset();
      _type = o._type;
      _flags = o._flags;
      _elementNum = o._elementNum;
      _elements = o._elements;
      _totalDataSize = o._totalDataSize;
      _rep = std::move(o._rep);
      o.reset();
      return *this;
   }

   void dpsWriteRequest::reset()
   {
      _type = LOG_TYPE_DUMMY;
      _flags = 0;
      _totalDataSize = 0;
      _elementNum = 0;
      _elements = nullptr; /// _elements's memory managed by _rep.
      _rep.reset();
      return;
   }

   const utilSlice &dpsWriteRequest::getElement(UINT32 pos)const
   {
      SDB_ASSERT(pos < _elementNum, "out of bound");
      return _elements[pos];
   }

   BOOLEAN dpsWriteRequest::seek(DPS_TAG tag, utilSlice &data)const
   {
      SDB_ASSERT(DPS_INVALID_TAG != tag, "can not be invalid");
      BOOLEAN r = FALSE;
      data.reset();
      for (UINT32 i = 0; i < _elementNum; ++i)
      {
         if (tag == _elements[i].castTo<dpsRecordEle>()->tag)
         {
            data = _elements[i].getSliceFromOffsetToEnd(sizeof(dpsRecordEle));
            r = TRUE;
            break;
         }
      }

      return r;
   }
} // namespace engine
