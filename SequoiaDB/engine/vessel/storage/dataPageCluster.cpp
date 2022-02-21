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

   Source File Name = dataPageCluster.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/dataPageCluster.h"
#include "pdTrace.hpp"
#include "ossLatchGuard.hpp"
#include "vessel/storageFileLoader.h"
#include "vessel/requestContext.h"

namespace engine
{
namespace vessel
{
   INT32 dataPageCluster::allocatePage(PAGE_ID &pid)
   {
      return allocatePages(1, &pid);
   }

   INT32 dataPageCluster::occupyPage(PAGE_ID pid)
   {
      return occupyPages(1, &pid);
   }

   void dataPageCluster::releasePage(PAGE_ID pid)
   {
      releasePages(1, &pid);
   }

   void dataPageCluster::_reset()
   {
      _sid = INVALID_SPACE_ID;
      _type = INVALID_SPACE_TYPE;
      _secretValue = 0;
      _args.reset();
      _o = options();
      return;
   }

   

   INT32 dataPageCluster::getPagePtr(FILE_TYPE type,
                                     PAGE_ID pid,
                                     mmapPagePointer &ptr)const
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_FILE_TYPE == type ||
                            INVALID_PAGE_ID == pid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (type != getDataFileType())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = getDataPagePtr(pid, ptr);
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