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

   Source File Name = logicalPageBuffer.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/logicalPageBuffer.h"
#include "vessel/requestContext.h"
#include "vessel/logicalPageSpace.h"

namespace engine
{
namespace vessel
{
   logicalPageBuffer::~logicalPageBuffer()
   {
      fini();
   }

   void logicalPageBuffer::fini()
   {
      _rpb.fini();
      _lh.unlock();
      _cowTrigger = copyOnWriteTrigger();
      _lps = NULL;
      return;
   }

   void logicalPageBuffer::init(logicalPageSpace *lps,
                                PAGE_SNAPSHOT_VERION psv,
                                BOOLEAN isMutable)
   {
      SDB_ASSERT(NULL != lps, "can not be null");
      SDB_ASSERT(INVALID_PAGE_SNAPSHOT_VERSION != psv, "can not be invalid");
      _lps = lps;
      _cowTrigger.reset(psv, isMutable);
      return;
   }

   INT32 logicalPageBuffer::prepareToWrite(requestContext *context)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == context))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _lps->makeBufferWritable(context, *this);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   void logicalPageBuffer::commit(DPS_LSN_OFFSET lsn)
   {
      if (OSS_LIKELY(isValid()))
      {
         _rpb.commit(lsn);
      }
   }
   
   void logicalPageBuffer::abort()
   {
      if (OSS_UNLIKELY(isValid()))
      {
         _rpb.abort();
      }
   }

   INT32 logicalPageBuffer::validatePage(PAGE_TYPE type)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "must be valid");
      SDB_ASSERT(INVALID_PAGE_TYPE != type, "can not be invalid");

      rc = ::engine::vessel::validatePage((ossValuePtr)(_rpb.getReadOnlyBuffer()),
                                          type, _rpb.getPageSize(),
                                          _rpb.getGlobalPid().page(),
                                          getLogicalPid(),
                                          getCowTrigger().getPsv());
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