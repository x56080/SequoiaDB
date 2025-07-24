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

   Source File Name = dpsTrivialStrField.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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
      return _fh->size;
   }

   UINT32 _dpsTrivialStrField::getFieldSize() const
   {
      UINT32 valueSize = getValueSize();
      return DPS_TS_FIELD_HEAD_SIZE + valueSize;
   }

   const CHAR *_dpsTrivialStrField::getRawValue() const
   {
      UINT32 valueSize = getValueSize();
      return 0 == valueSize ?
             nullptr : reinterpret_cast<const CHAR *>(_fh) + DPS_TS_FIELD_HEAD_SIZE;
   }

   BOOLEAN _dpsTrivialStrField::isEndingField() const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      return _fh->isEnding();
   }
}