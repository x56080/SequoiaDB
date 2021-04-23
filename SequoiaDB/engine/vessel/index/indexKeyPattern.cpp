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

   Source File Name = indexKeyPattern.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/indexKeyPattern.h"
#include "vessel/indexDef.h"

using namespace bson;

namespace engine
{
namespace vessel
{
   indexKeyPattern::indexKeyPattern()
   {}

   indexKeyPattern::~indexKeyPattern()
   {}

   INT32 indexKeyPattern::set(const bson::BSONObj &obj)
   {
      INT32 rc = SDB_OK;
      _keyCount = 0;
      _ordering = 0;
      BSONObjIterator itr(obj);
      while (itr.more())
      {
         const CHAR *fieldName = NULL;
         BSONElement e = itr.next();
         if (e.eoo())
         {
            rc = SDB_INVALIDARG;
            goto error;
         }

         if (MAX_INDEX_KEY_COUNT == _keyCount)
         {
            rc = SDB_INVALIDARG;
            goto error;
         }
         
         fieldName = e.fieldName();
         if (NULL == fieldName ||
             '\0' == fieldName[0])
         {
            rc = SDB_INVALIDARG;
            goto error;
         }

         if (e.number() < 0)
         {
            _ordering |= ((UINT32)1 << _keyCount);
         }
         ++_keyCount;
      }

      if (0 == _keyCount)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _pattern = obj;
   done:
      return rc;
   error:
      reset();
      goto done;
   }

   void indexKeyPattern::reset()
   {
      _keyCount = 0;
      _ordering = 0;
      _pattern = bson::BSONObj();
      return;
   }
}
}