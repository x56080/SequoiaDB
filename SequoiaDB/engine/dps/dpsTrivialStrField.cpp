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

   Source File Name = dpsTrivialStrField.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "dpsTrivialStrField.hpp"
#include "pdTrace.hpp"

namespace engine
{
   _dpsTrivialStrField::_dpsTrivialStrField(const void *data)
   {
      SDB_ASSERT(nullptr != data, "can not be invalid");
      _fh = reinterpret_cast<const dpsTsFieldHeader *>(data);
   }

   DPS_TS_FIELD_TAG _dpsTrivialStrField::getTag() const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      return _fh->getTag();
   }

   UINT32 _dpsTrivialStrField::getValueSize() const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      UINT32 size = _fh->size;
      if (size < DPS_TS_SIZE_BOUND)
      {
         return size;
      }
      else
      {
         return reinterpret_cast<const dpsTsFieldHeaderExt *>(_fh)->size;
      }
   }

   UINT32 _dpsTrivialStrField::getFieldSize() const
   {
      UINT32 valueSize = getValueSize();
      return dpsGetTsFieldHeadSize(valueSize) + valueSize;
   }

   const CHAR *_dpsTrivialStrField::getRawValue() const
   {
      UINT32 valueSize = getValueSize();
      return 0 == valueSize ?
             nullptr : reinterpret_cast<const CHAR *>(_fh) + dpsGetTsFieldHeadSize(valueSize);
   }

   BOOLEAN _dpsTrivialStrField::isEndingField() const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      return _fh->isEnding();
   }
}