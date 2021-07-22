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

   Source File Name = dmlContext.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/dmlContext.h"
#include "utilMemListPool.hpp"
#include "pdTrace.hpp"
#include "ossLikely.hpp"
namespace engine
{
namespace vessel
{
   dmlContext::~dmlContext()
   {
      fini();
   }

   void dmlContext::close()
   {
      fini();
      requestContext::close();
      return;
   }

   void dmlContext::fini()
   {
      _csName.reset();
      _clName.reset();
      _clLogicalID = DMS_INVALID_LOGICCLID;
      _transID.reset();
      _uniqueIndexHash.clear();
      return;
   }

   void dmlContext::setCLInfo(const strSlice &csName,
                               const strSlice &clName,
                               UINT32 clLogicalId)
   {
      _csName = csName;
      _clName = clName;
      _clLogicalID = clLogicalId;
      return;
   }

   INT32 dmlContext::lockUniqueIndexKey()
   {
      INT32 rc = SDB_OK;
      if (!requestContext::isOpen() ||
          !requestContext::isSpaceIdLocked())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (_uniqueKeyLocked)
      {
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }
      _uniqueKeyLocked = TRUE;
   done:
      return rc;
   error:
      goto done;
   }

   void dmlContext::unlockUniqueKeys()
   {
      if (_uniqueKeyLocked)
      {
         _uniqueKeyLocked = FALSE;
      }
   }
}//namespace vessel
}//namespace engine