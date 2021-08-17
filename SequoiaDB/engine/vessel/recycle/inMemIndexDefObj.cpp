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

   Source File Name = inMemIndexDefObj.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/inMemIndexDefObj.h"
#include "vessel/indexDef.h"
#include "ixm_common.hpp"
#include "msgDef.h"
#include "../bson/bson.hpp"

namespace engine
{
namespace vessel
{

   inMemIndexDefObj::inMemIndexDefObj()
   {}

   inMemIndexDefObj::~inMemIndexDefObj()
   {
      
   }

   void inMemIndexDefObj::fini()
   {
      _head = indexDefHead();
      _indexName.reset(NULL);
      _keyPattern.reset();
      _params = indexParameters();
      _defObj.reset();
      _mb.release();
      _lpid = INVALID_PAGE_ID;
      return;
   }

   INT32 inMemIndexDefObj::init(const indexDefHead &head,
                                const slice &defObj,
                                PAGE_ID lpid)
   {
      INT32 rc = SDB_OK;
      bson::BSONObj obj;

      fini();
      if (!head.isValid() ||
          !defObj.isValid() ||
          INVALID_PAGE_ID == lpid)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _head = head;
      _defObj = defObj;
      _lpid = lpid;
      rc = _initFromDefObj(defObj);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init from def obj:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      fini();
      goto done;
   }

   INT32 inMemIndexDefObj::getOwned()
   {
      INT32 rc = SDB_OK;
      if (!isValid())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (isOwned())
      {
         goto done;
      }

      rc = _mb.reserve(_defObj.len());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to reserve memory block:%d", rc);
         goto error;
      }

      rc = _mb.copy(_defObj.len(), _defObj.data());
      if (SDB_OK != rc)
      {
         goto error;
      }

      _defObj.reset(_mb.getSize(), _mb.getBuffer());
      rc = _initFromDefObj(_defObj);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init from def obj:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      fini();
      goto done;
   }

   INT32 inMemIndexDefObj::_initFromDefObj(const slice &defObj)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(defObj.isValid(), "can not be empty");
      SDB_ASSERT(_head.isValid(), "must be valid");
      bson::BSONObj obj(defObj.data());
      bson::BSONElement ele;

      ele = obj.getField(IXM_NAME_FIELD);
      _indexName.reset(ele.valuestrsafe());
      if (_indexName.empty())
      {
         PD_LOG(PDERROR, "failed to extract index name");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      ele = obj.getField(IXM_KEY_FIELD);
      if (bson::Object != ele.type())
      {
         PD_LOG(PDERROR, "failed to extract index key");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      rc = _keyPattern.set(ele.embeddedObject());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to set key pattern from obj:%d", rc);
         goto error;
      }

      if (!_params.extractFromBson(obj))
      {
         PD_LOG(PDERROR, "failed to extract common options");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   void inMemIndexDefObj::setStatus(INDEX_STATUS status)
   {
      SDB_ASSERT(isValid(), "must be valid");
      _head.status = status;
      return;
   }
}//namespace vessel
}//namespace engine
