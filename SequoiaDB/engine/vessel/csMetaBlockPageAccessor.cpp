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

   Source File Name = csMetaBlockPageAccessor.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/csMetaBlockPageAccessor.h"
#include "pdTrace.hpp"
#include "vessel/logicalPageBuffer.h"

namespace engine
{
namespace vessel
{
   static const UINT32 UPDATE_CS_META_TYPE_LID = 0;

   csMetaBlockPageAccessor::csMetaBlockPageAccessor()
   {}

   csMetaBlockPageAccessor::~csMetaBlockPageAccessor()
   {}

   INT32 csMetaBlockPageAccessor::read(requestContext *context,
                                       const logicalPageBuffer *lpb,
                                       csMetaBlock &cmb)
   {
      INT32 rc = SDB_OK;
      const runtimePageBuffer *rpb = NULL;
      const csMetaBlock *record = NULL;
      strictBuffer buffer;

      if (NULL == context ||
          NULL == lpb ||
          !lpb->isValid())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rpb = &(lpb->getRuntimeBuffer());
      buffer = lpb->getReadableBodyBuffer();
      rc = lpb->validatePage(PAGE_TYPE_CS_META);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to validate page[%s], rc:%d",
                rpb->getGlobalPid().toString().c_str(), rc);
         goto error;
      }

      record = buffer.getReadableObjPtr<csMetaBlock>(0);
      if (NULL == record)
      {
         PD_LOG(PDERROR, "failed to get readable record ptr");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (!record->isValid())
      {
         PD_LOG(PDERROR, "collection space meta data is not valid");
         rc = SDB_VESSEL_PAGE_CRASHED;
         goto error;
      }
      cmb = *record;      
   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine