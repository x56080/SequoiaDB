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
#include "ossLikely.hpp"
#include "pdTrace.hpp"

namespace engine
{
namespace vessel
{
///////////////  indexIteratorKernal


INT32 indexIteratorKernal::_open(requestContext *context,
                                 const indexHandle &handle,
                                 const orderingWrapper &ordering,
                                 INT32 direction,
                                 rtnPredicateListIterator *predicate,
                                 memoryBlock &entryBuffer)
{
   INT32 rc = SDB_OK;
   if (OSS_UNLIKELY(NULL == context ||
                    !handle.isValid() ||
                    (1 != direction && -1 != direction) ||
                    NULL == predicate))
   {
      rc = SDB_INVALIDARG;
      goto error;
   }

   _context = context;
   _handle = handle;
   _ordering = ordering;
   _direction = direction;
   _predicate = predicate;
   _entryBuffer = &entryBuffer;

done:
   return rc;
error:
   goto done;
}

void indexIteratorKernal::_close()
{
   _context = NULL;
   _handle = indexHandle();
   _ordering = orderingWrapper();
   _direction = 1;
   _predicate = NULL;
   _entryBuffer = NULL;
   return;
}

///////////////  indexIteratorKernal end



///////////////  indexIterator

}//namespace vessel
}//namespace engine
