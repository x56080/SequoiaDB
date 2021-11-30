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

   Source File Name = rowBatch.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/rowBatch.h"

namespace engine
{
namespace vessel
{
   void rowBatch::init(INT32 rowLimit)
   {
      fini();
      _rowLimit = rowLimit;
      return;
   }

   void rowBatch::fini()
   {
      _rows.clear();
      _rowLimit = -1;
      _fini();
   }

   void rowBatch::clearBatch()
   {
      _rows.clear();
      clearBuffer();
      return;
   }

   void rowBatch::push(const slice &row)
   {
      SDB_ASSERT(row.isValid(), "can not be invalid");
      _rows.push_back(row.getReadableSlice());
   }
} // namespace vessel

} // namespace engine
