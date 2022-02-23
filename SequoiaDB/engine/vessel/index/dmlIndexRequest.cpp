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
      _index = NULL;
      _toInsert.clear();
      _toRemove.clear();
      _flags = 0;
   }

   INT32 dmlIndexRequest::init(indexObject *index,
                               const bson::BSONObjSet *toInsert,
                               const bson::BSONObjSet *toRemove)
   {
      SDB_ASSERT(NULL != index && index->isValid(), "must be valid");
      INT32 rc = SDB_OK;
      bson::BSONObjSet merged;
      fini();

      if (OSS_UNLIKELY(NULL == index || !index->isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _index = index;

      if (NULL != toInsert)
      {
         for (bson::BSONObjSet::const_iterator itr = toInsert->begin();
            itr != toInsert->end(); ++itr)
         {
            if ((INT32)MAX_INDEX_KEY_SIZE < itr->objsize())
            {
               PD_LOG(PDDEBUG, "index key size[%d] over limit", itr->objsize());
               rc = SDB_IXM_KEY_TOO_LARGE;
               goto error;
            }

            if (NULL != toRemove && 0 < toRemove->count(*itr))
            {
               merged.insert(*itr);
               continue;
            }

            _toInsert.push_back(itr->getOwned());
         }
      }

      if (NULL != toRemove)
      {
         for (bson::BSONObjSet::const_iterator itr = toRemove->begin();
             itr != toRemove->end(); ++itr)
         {
            if ((INT32)MAX_INDEX_KEY_SIZE < itr->objsize())
            {
               PD_LOG(PDDEBUG, "index key size[%d] over limit", itr->objsize());
               rc = SDB_IXM_KEY_TOO_LARGE;
               goto error;
            }

            if (0 < merged.count(*itr))
            {
               continue;
            }

            _toRemove.push_back(itr->getOwned());
         }
      }
   done:
      return rc;
   error:
      fini();
      goto done;
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
      _constraintIndexCount = 0;
      _building = 0;
   }

   INT32 dmlIndexRequestArray::append(indexObject *index,
                                      const bson::BSONObjSet *keysToInsert,
                                      const bson::BSONObjSet *keysToRemove)
   {
      INT32 rc = SDB_OK;
      dmlIndexRequest *req = NULL;

      if (OSS_UNLIKELY(NULL == index ||
                       !index->isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == keysToInsert && NULL == keysToRemove))
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

      rc = req->init(index, keysToInsert, keysToRemove);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init index request:%d", rc);
         goto error;
      }

      /// we can also use std::move to append req into array,
      /// which to avoid malloc memory.
      if (req->isEmpty())
      {
         SDB_OSS_DEL req;
         goto done;
      }

      _requests.push_back(req);

      if (req->withConstraint())
      {
         ++_constraintIndexCount;
      }
      if (index->isBuilding())
      {
         ++_building;
      }

   done:
      return rc;
   error:
      SAFE_OSS_DELETE(req);
      goto done;
   }

   dmlIndexRequest *dmlIndexRequestArray::get(UINT32 i)
   {
      SDB_ASSERT(i < _requests.size(), "out of bound");
      return _requests[i];
   }

   const dmlIndexRequest *dmlIndexRequestArray::get(UINT32 i)const
   {
      SDB_ASSERT(i < _requests.size(), "out of bound");
      return _requests[i];
   }
   
}//namespace vessel
}//namespace engine