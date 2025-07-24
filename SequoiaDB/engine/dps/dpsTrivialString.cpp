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

   Source File Name = utilTrivialString.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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
