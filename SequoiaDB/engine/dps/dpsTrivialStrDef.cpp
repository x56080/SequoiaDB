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

   Source File Name = dpsTrivialStrDef.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "dpsTrivialStrDef.hpp"
#include "pdTrace.hpp"

namespace engine
{
   void dpsInitTsFieldHead(DPS_TS_FIELD_TAG tag,
                           UINT32 valueSize,
                           UINT32 bufSize,
                           void *buf)
   {
      SDB_ASSERT(nullptr != buf, "can not be invalid");
      UINT32 headSize = dpsGetTsFieldHeadSize(valueSize);
      SDB_ASSERT((headSize + valueSize) <= bufSize, "out of buf size");
      if (valueSize < DPS_TS_SIZE_BOUND)
      {
         dpsTsFieldHeader *h = reinterpret_cast<dpsTsFieldHeader *>(buf);
         h->setTag(tag);
         h->size = valueSize;
      }
      else
      {
         dpsTsFieldHeaderExt *h = reinterpret_cast<dpsTsFieldHeaderExt *>(buf);
         h->h.setTag(tag);
         h->h.size = DPS_TS_SIZE_BOUND;
         h->size = valueSize;
      }

      return;
   }
} // namespace engine
