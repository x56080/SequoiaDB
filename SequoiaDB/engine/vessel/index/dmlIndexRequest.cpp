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

   Source File Name = dmlIndexRequest.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/dmlIndexRequest.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"

namespace engine
{
namespace vessel
{
   void dmlIndexRequest::fini()
   {
      _indexSlot = -1;
      _keys.clear();
      _obj.fini();
      _building = FALSE;
      _pushedIntoBuildingContext = FALSE;
   }

   void dmlIndexRequest::init(INT32 indexSlot,
                              const indexObject &obj,
                              const bson::BSONObjSet &keys)
   {
      SDB_ASSERT(isValidIndexSlot(indexSlot), "must be valid");
      SDB_ASSERT(obj.isValid(), "must be valid");
      SDB_ASSERT(!keys.empty(), "can not be empty");
      _indexSlot = indexSlot;
      _keys.clear();
      _obj.shallowCopy(obj);
      _obj.getOwned();

      for (bson::BSONObjSet::const_iterator itr = keys.begin();
           itr != keys.end(); ++itr)
      {
         _keys.push_back(itr->getOwned());
      }

      return;
   }

/////////////////////////////////////////dmlIndexRequestArray

   dmlIndexRequestArray::~dmlIndexRequestArray()
   {
      clear();
   }

   void dmlIndexRequestArray::clear()
   {
      for (UINT32 i = 0; i < _requests.size(); ++i)
      {
         dmlIndexRequest *r = _requests[i];
         if (NULL != r)
         {
            SDB_OSS_DEL r;
         }
      }
      _requests.clear();
   }

   INT32 dmlIndexRequestArray::append(INT32 indexSlot,
                                      const indexObject &obj,
                                      const bson::BSONObjSet &keys,
                                      dmlIndexRequest **out)
   {
      INT32 rc = SDB_OK;
      dmlIndexRequest *req = NULL;

      if (OSS_UNLIKELY(!isValidIndexSlot(indexSlot) ||
                       !obj.isValid() ||
                       keys.empty()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(MAX_INDEX_COUNT_PER_CL == _requests.size()))
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      req = SDB_OSS_NEW dmlIndexRequest();
      if (NULL == req)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      rc = _requests.append(req);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append to array:%d", rc);
         goto error;
      }

      req->init(indexSlot, obj, keys);

      if (NULL != out)
      {
         *out = req;
      }
   done:
      return rc;
   error:
      SAFE_OSS_DELETE(req);
      goto done;
   }

   dmlIndexRequest *dmlIndexRequestArray::get(UINT32 i)const
   {
      SDB_ASSERT(i < _requests.size(), "out of bound");
      return _requests[i];
   }

   
}//namespace vessel
}//namespace engine