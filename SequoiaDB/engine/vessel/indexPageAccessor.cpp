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

   Source File Name = indexPageAccessor.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/indexPageAccessor.h"
#include "vessel/indexSpace.h"

namespace engine
{
namespace vessel
{
   indexPageAccessor::indexPageAccessor()
   {}

   indexPageAccessor::~indexPageAccessor()
   {}

   INT32 indexPageAccessor::init(requestContext *context,
                                 PAGE_ID lpid,
                                 indexSpace *space,
                                 BOOLEAN readOnly)
   {
      INT32 rc = SDB_OK;
      pageAccessor::options o;
      o.cacheMode = FALSE;
      o.readOnly = readOnly;
                     
      if (NULL == context ||
          INVALID_PAGE_ID == lpid ||
          NULL == space)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = logicalPageAccessor::init(context, FILE_TYPE_IDX_D, lpid, o, space);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine