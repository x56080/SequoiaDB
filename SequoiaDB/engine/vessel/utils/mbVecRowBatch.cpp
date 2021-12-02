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

   Source File Name = mbVecRowBatch.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/mbVecRowBatch.h"

namespace engine
{
namespace vessel
{
   INT32 mbVecRowBatch::appendRowFragments(std::initializer_list<slice> il)
   {
      INT32 rc = SDB_OK;
      memoryBlock mb;
      UINT32 size = 0;
      for (auto i = il.begin(); i != il.end(); ++i)
      {
         if (!i->isValid())
         {
            SDB_ASSERT(FALSE, "invalid fragment");
            rc = SDB_INVALIDARG;
            goto error;
         }

         size += i->getSize();
      }

      rc = mb.reserve(size);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to reserve mb size:%d", rc);
         goto error;
      }

      for (auto i = il.begin(); i != il.end(); ++i)
      {
         mb.append(i->getSize(), i->data());
      }

      _mbs.push_back(std::move(mb));
   done:
      return rc;
   error:
      goto done;
   }

   INT32 mbVecRowBatch::appendRow(const slice &row)
   {
      return appendRowFragments({row});
   }

   slice mbVecRowBatch::getRowByOffset(UINT32 offset, UINT32 size)const
   {
      SDB_ASSERT(FALSE, "can not search by offset");
      return slice();
   }

   slice mbVecRowBatch::getRowByPos(UINT32 pos)const
   {
      SDB_ASSERT(pos < _mbs.size(), "out of bound");
      const memoryBlock &mb = _mbs.at(pos);
      return slice(mb.getSize(), mb.getBuffer());
   }
} // namespace vessel

} // namespace engine
