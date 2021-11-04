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

   Source File Name = btreeAccessPathNode.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/btreeAccessPathNode.h"
#include "vessel/logicalPageBuffer.h"
#include "pdTrace.hpp"
#include "vessel/btreeNodePage.h"

namespace engine
{
namespace vessel
{
   btreeAccessPathNode::btreeAccessPathNode(logicalPageBuffer *lpb)
   {
      SDB_ASSERT(NULL != lpb && lpb->isValid(), "can not be invalid");
      _lpid = lpb->getLogicalPid();
      _lpb = lpb;
      const btreeNodePageHead *head = lpb->getReadableBodySlice().getReadableObjPtr<btreeNodePageHead>(0);
      SDB_ASSERT(NULL != head, "can not be null");
      _splitedTimes = head->splitedTimes;
   }

   void btreeAccessPathNode::setChildLocation(const btreeItemLocation &location)
   {
      SDB_ASSERT(isAccessing(), "must be accessing");
      SDB_ASSERT(location.isValid(), "can not be invalid");
      _childLocation = location;
      return;
   }

   void btreeAccessPathNode::reaccess(logicalPageBuffer *lpb)
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(NULL != lpb && lpb->isValid(), "can not be invalid");
      SDB_ASSERT(lpb->getLogicalPid() == _lpid, "must be same");
      SDB_ASSERT(NULL == _lpb, "must be null");
      _lpb = lpb;
      return;
   }
      
} // namespace vessel
  
} // namespace engine
