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

   Source File Name = utilTrivialString.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "dpsTrivialString.hpp"
#include "pdTrace.hpp"

namespace engine
{
   _dpsTrivialString::_dpsTrivialString(const CHAR *data, UINT32 size):
   _data(data),
   _size(size)
   {
      SDB_ASSERT(nullptr != data, "can not be invalid");
   }

   UINT32 _dpsTrivialString::_calcAndCacheSize() const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      UINT32 size = 0;
      iterator itr = begin();
      while (itr.isValid())
      {
         size += itr.getField().getFieldSize();
         itr.next();
      }

      _size = size;
      return size;
   }

   _dpsTrivialString::iterator _dpsTrivialString::begin() const
   {
      return isValid() ? iterator(_data) : iterator();
   }

   dpsTrivialStrField _dpsTrivialString::seek(DPS_TS_FIELD_TAG tag) const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      dpsTrivialStrField field;
      iterator itr = begin();
      while (itr.isValid())
      {
         field = itr.getField();
         if (field.getTag() == tag)
         {
            break;
         }
         else
         {
            itr.next();
         }
      }

      return field;
   }

////////////////_dpsTrivialString::iterator
   _dpsTrivialString::iterator::iterator(const CHAR *data):
   _data(data)
   {
      SDB_ASSERT(nullptr != _data, "can not be invalid");
   }

   dpsTrivialStrField _dpsTrivialString::iterator::getField() const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      return dpsTrivialStrField(_data);
   }
   
   BOOLEAN _dpsTrivialString::iterator::next()
   {
      BOOLEAN r = isValid();
      if (r)
      {
         dpsTrivialStrField field = getField();
         if (field.isEndingField())
         {
            reset();
            r = FALSE;
         }
         else
         {
            UINT32 size = field.getFieldSize();
            _data += size;
         }
      }

      return r;
   }
} // namespace engine
