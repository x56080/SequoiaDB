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

   Source File Name = indexScanContext.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/indexScanContext.h"
#include "vessel/objectLatchHelper.hpp"
#include "vessel/instanceEnv.h"

namespace engine
{
namespace vessel
{
   indexScanContext::~indexScanContext()
   {
      closeIndexScan();
   }

   INT32 indexScanContext::openIndexScan(const indexHandle &handle,
                                         _rtnPredicateListIterator *predicate,
                                         indexEntryBuffer *entryBuffer,
                                         UNORDERED_RID_SET *ridSet,
                                         BOOLEAN forward)
   {
      INT32 rc = SDB_OK;
      closeIndexScan();

      if (OSS_UNLIKELY(!handle.isValid() ||
                       NULL == predicate ||
                       NULL == entryBuffer ||
                       NULL == ridSet))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!requestContext::isOpen()))
      {
         SDB_ASSERT(FALSE, "impossible");
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      _handle = handle;
      _predicate = predicate;
      _entryBuffer = entryBuffer;
      _ridSet = ridSet;
      _forward = forward;
   done:
      return rc;
   error:
      goto done;
   }

   void indexScanContext::close()
   {
      closeIndexScan();
      requestContext::close();
      return;
   }

   void indexScanContext::closeIndexScan()
   {
      _handle = indexHandle();
      _predicate = NULL;
      _entryBuffer = NULL;
      _ridSet = NULL;
      _forward = TRUE;
      return;
   }

} // namespace vessel

} // namespace engine
