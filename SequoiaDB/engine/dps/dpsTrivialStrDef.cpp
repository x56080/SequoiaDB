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

   Source File Name = dpsTrivialStrDef.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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
