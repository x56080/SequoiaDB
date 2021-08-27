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

   Source File Name = indexIterator.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/indexIterator.h"
#include "vessel/indexUtils.h"
#include "vessel/instanceEnv.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"
#include "vessel/lsm/lsmIndexIterator.h"

namespace engine
{
namespace vessel
{
   indexIterator *createIndexIterator(INDEX_TYPE type)
   {
      if (INDEX_TYPE_LSM == type)
      {
         return SDB_OSS_NEW lsmIndexIterator();
      }
      else if (INDEX_TYPE_BTREE)
      {
         SDB_ASSERT(FALSE, "TODO");
         return NULL;
      }
      else
      {
         SDB_ASSERT(FALSE, "invalid type");
         return NULL;
      }
   }

   indexIterator::~indexIterator()
   {
      _close();
   }

   void indexIterator::_open(requestContext *context,
                           const indexHandle &handle,
                           const orderingWrapper &ordering,
                           INT32 direction)
   {
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(handle.isValid(), "can not be invalid");
      _context = context; 
      _handle = handle;
      _ordering = ordering;
      _forwardDirection = indexUtils::isForwardDirection(direction);
      return;
   }

   void indexIterator::_close()
   {
      if (NULL != _context)
      {
         unlockAllRids();
         _context = NULL;
         _handle = indexHandle();
         _ordering = orderingWrapper();
         _forwardDirection = TRUE;
      }
      return;
   }

   void indexIterator::unlockAllRids()
   {
      if (NULL == _context)
      {
         SDB_ASSERT(_rlc.isEmpty(), "must be empty");
      }
      else if (!_rlc.isEmpty())
      {
         RECORD_ID_LATCH_MAP::object obj;
         ossSharedLatchMode mode;
         RECORD_ID_LATCH_MAP &lm = _context->getEnv()->ridLatchMap;

         while (_rlc.pop(obj, mode))
         {
            obj.getValue().unlockWith(mode);
            lm.release(obj);
         }
      }

      return;
   }

}//namespace vessel
}//namespace engine