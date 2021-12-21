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

   Source File Name = indexScanCursor.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/indexScanCursor.h"

namespace engine
{
namespace vessel
{
   indexScanCursor::~indexScanCursor()
   {
      
   }

   void indexScanCursor::saveEntry(const slice &entryData)
   {
      SDB_ASSERT(isOpen() && entryData.isValid(), "can not be invalid");

      _entry.reset();
      _entry.appendBuf(entryData.data(), entryData.getSize());
      return;
   }

   slice indexScanCursor::getEntryData()const
   {
      SDB_ASSERT(isOpen() && hasEntry(), "can not be invalid");
      return slice(_entry.len(), _entry.buf());
   }

   BOOLEAN indexScanCursor::markRidScanned(const recordID &rid)
   {
      return _scanned.insert(rid).second;
   }

   BOOLEAN indexScanCursor::testRidScanned(const recordID &rid)const
   {
      return 0 < _scanned.count(rid);
   }
} // namespace vessel

} // namespace engine
