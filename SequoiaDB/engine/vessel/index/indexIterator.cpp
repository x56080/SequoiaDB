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
                             const indexContext *ic,
                             BOOLEAN forward)
   {
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(NULL != ic, "can not be null");
      SDB_ASSERT(ic->isValid(), "can not be invalid");
      _context = context; 
      _ic = ic;
      _forward = forward;
      return;
   }

   void indexIterator::_close()
   {
      _context = NULL;
      _ic = NULL;
      _forward = TRUE;
      return;
   }

}//namespace vessel
}//namespace engine