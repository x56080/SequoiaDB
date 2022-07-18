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
#include "ossUtil.hpp"
#include "pdTrace.hpp"

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
      reset();
      BSONObjIterator itr(obj);
      while (itr.more())
      {
         INT32 ordering = 0;
         BSONElement e = itr.next();
         const CHAR *fieldName = e.fieldName() ;
         if (e.eoo() || !e.isNumber())
         {
            rc = SDB_INVALIDARG;
            goto error;
         }

         if (MAX_INDEX_KEY_COLUMNS == _keyCount)
         {
            rc = SDB_INVALIDARG;
            goto error;
         }

         ordering = e.numberInt();
         if (1 == ordering)
         {
            /// do nothing
         }
         else if (-1 == ordering)
         {
            _ordering |= ((UINT32)1 << _keyCount);
         }
         else
         {
            PD_LOG(PDERROR, "invalid key pattern ordering");
            rc = SDB_INVALIDARG;
            goto error;
         }
         
         if (NULL == fieldName ||
             '\0' == fieldName[0] ||
             NULL != ossStrchr( fieldName, '$'))
         {
            rc = SDB_INVALIDARG;
            goto error;
         }

         ++_keyCount;
      }

      if (0 == _keyCount)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _pattern = obj.getOwned();

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

   BOOLEAN indexKeyPattern::operator==(const indexKeyPattern &o)const
   {
      return _keyCount == o._keyCount &&
             _ordering == o._ordering &&
             0 == _pattern.woCompare(o._pattern);
   }

   orderingWrapper indexKeyPattern::getOrdering()const
   {
      return orderingWrapper(_ordering, _keyCount);
   }

   BOOLEAN indexKeyPattern::isCoveredBy(const indexKeyPattern &other)const
   {
      SDB_ASSERT(isValid(), "must be valid");
      SDB_ASSERT(other.isValid(), "must be valid");
      BOOLEAN r = TRUE;

      bson::BSONObjIterator itr0(_pattern);
      bson::BSONObjIterator itr1(other._pattern);

      if (_keyCount > other._keyCount)
      {
         r = FALSE;
         goto done;
      }

      for (UINT32 i = 0; i < _keyCount; ++i)
      {
         bson::BSONElement ele0;
         bson::BSONElement ele1;
         UINT32 ordering0 = 0;
         UINT32 ordering1 = 0;
         UINT32 mask = (UINT32)1 << i;

         ordering0 = (mask & _ordering);
         ordering1 = (mask & other._ordering);
         if (ordering0 != ordering1)
         {
            r = FALSE;
            goto done;
         }

         ele0 = itr0.next();
         ele1 = itr1.next();

         if (0 != ossStrcmp(ele0.fieldName(), ele1.fieldName()))
         {
            r = FALSE;
            break;
         }
      }

   done:
      return r;
   }
}
}