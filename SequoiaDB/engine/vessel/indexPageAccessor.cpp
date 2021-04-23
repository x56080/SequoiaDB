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

namespace engine
{
namespace vessel
{
   indexPageAccessor::indexPageAccessor()
   {}

   indexPageAccessor::~indexPageAccessor()
   {}

   INT32 indexPageAccessor::init(requestContext *context,
                                 FILE_TYPE type,
                                 PAGE_ID pid,
                                 BOOLEAN readOnly,
                                 storageUnit *su)
   {
      INT32 rc = SDB_OK;
      UINT32 flags = PAGE_ACCESSOR_FLAG_DIRECT;
      if (!readOnly)
      {
         flags |= PAGE_ACCESSOR_FLAG_NON_READONLY;
      }
                     
      if (NULL == context ||
          (FILE_TYPE_IDX_D != type && FILE_TYPE_IDX_M != type) ||
          INVALID_PAGE_ID == pid)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = pageAccessor::init(context, type, pid, flags, su);
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