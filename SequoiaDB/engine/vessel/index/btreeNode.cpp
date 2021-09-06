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

   Source File Name = btreeNode.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/btreeNode.h"
#include "vessel/requestContext.h"
#include "vessel/logicalPageBuffer.h"
#include "vessel/indexContext.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"
#include "vessel/btreeNodePath.h"
#include "vessel/indexSpace.h"

namespace engine
{
namespace vessel
{
   btreeNode::btreeNode(btreeNodePath *path,
                        logicalPageBuffer *lpb,
                        UINT32 depth)
   {
      fini();
      SDB_ASSERT(NULL != path && path->isValid(), "can not be invalid");
      SDB_ASSERT(NULL != lpb && lpb->isValid(), "can not be invalid");
      _buffer = lpb;
      _path = path;
      _depth = depth;
   }

   void btreeNode::fini()
   {
      _buffer = NULL;
      _path = NULL;
      _depth = 0;
      return;
   }

   BOOLEAN btreeNode::isRoot()const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      return _path->isRoot(*this);
   }

   BOOLEAN btreeNode::hasExtNode()const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      const btreeNodePageHead *head = getReadbleHead();
      SDB_ASSERT(NULL != head, "can not be null");
      return INVALID_PAGE_ID != head->extNode;
   }

   const btreeNodePageHead *btreeNode::getReadbleHead()const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      return _buffer->getRuntimeBuffer().getReadablePtrOfBody<btreeNodePageHead>(0);
   }
} // namespace vessel

} // namespace engine

