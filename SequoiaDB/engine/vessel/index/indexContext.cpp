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

   Source File Name = indexContext.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/indexContext.h"
#include "ossLikely.hpp"
#include "vessel/buildingIndexContext.h"
#include "ixm_common.hpp"
#include "msgDef.h"

namespace engine
{
namespace vessel
{
   indexContext::indexContext()
   {}

   indexContext::~indexContext()
   {
      fini();
   }

   INT32 indexContext::init(INT32 indexSlot,
                            const indexObject &obj,
                            INDEX_STATUS status)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isValidIndexSlot(indexSlot) ||
                       !obj.isValid() ||
                       INDEX_STATUS_INVALID == status))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _indexSlot = indexSlot;
      rc = _obj.init(obj.getIndexID(),
                     obj.getIndexName(),
                     obj.getPattern(),
                     obj.getParams());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init index obj:%d", rc);
         goto error;
      }

      if (INDEX_STATUS_BUILDING == status)
      {
         _unstatbleContext = SDB_OSS_NEW buildingIndexContext();
         if (OSS_UNLIKELY(NULL == _unstatbleContext))
         {
            PD_LOG(PDERROR, "failed to alocate mem");
            rc = SDB_OOM;
            goto error;
         }
      }

      _status = status;
   done:
      return rc;
   error:
      fini();
      goto done;
   }

   void indexContext::fini()
   {
      _indexSlot = -1;
      _obj.fini();
      _status = INDEX_STATUS_INVALID;
      SAFE_OSS_DELETE(_unstatbleContext);
      return;
   }

   void indexContext::setNormalFromBuilding()
   {
      SDB_ASSERT(isBuilding(), "must be building");
      if (NULL != _unstatbleContext)
      {
         SDB_OSS_DEL _unstatbleContext;
         _unstatbleContext = NULL;
      }
      _status = INDEX_STATUS_NORMAL;
      return;
   }

   void indexContext::setRemoving()
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      if (NULL != _unstatbleContext)
      {
         SDB_OSS_DEL _unstatbleContext;
         _unstatbleContext = NULL;
      }
      _status = INDEX_STATUS_REMOVING;
      return;
   }

   void indexContext::dump(bson::BSONObjBuilder &builder)const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      builder.append(VESSEL_INDEX_FIELD_NAME_INDEX_SLOT, _indexSlot);
      builder.append(VESSEL_INDEX_FIELD_NAME_INDEX_ID, _obj.getIndexID());
      builder.append(IXM_NAME_FIELD, _obj.getIndexName().str());
      builder.append(VESSEL_INDEX_FIELD_NAME_STATUS, _status);
      builder.append(IXM_KEY_FIELD, _obj.getPattern().getPattern());
      _obj.getParams().exportToBson(builder);
   }
} // namespace vessel

}//namespace engine
