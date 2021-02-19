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

   Source File Name = collectionObject.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/collectionObject.h"
#include "ossLikely.hpp"

namespace engine
{
namespace vessel
{
   INT32 collectionObject::open(vessel *db,
                                 UINT32 cslid,
                                 UINT32 cllid,
                                 SPACE_ID sid,
                                 CL_MB_ID mbid)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(NULL == db ||
                       DMS_INVALID_LOGICCSID == cslid ||
                       DMS_INVALID_LOGICCLID == cllid ||
                       INVALID_SPACE_ID == sid ||
                       INVALID_CL_MB_ID == mbid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (isOpen())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _db = db;
      _cslid = cslid;
      _cllid = cllid;
      _sid = sid;
      _mbid = mbid;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collectionObject::close()
   {
      _cslid = DMS_INVALID_LOGICCSID;
      _cllid = DMS_INVALID_LOGICCLID;
      _sid = INVALID_SPACE_ID;
      _mbid = INVALID_CL_MB_ID;
      _db = NULL;
      return SDB_OK;
   }
}//namespace vessel
}//namespace engine