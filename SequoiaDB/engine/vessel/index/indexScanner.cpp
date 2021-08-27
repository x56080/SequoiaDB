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

   Source File Name = indexScanner.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/indexScanner.h"
#include "vessel/indexDef.h"
#include "vessel/requestContext.h"
#include "vessel/indexIterator.h"
#include "pdTrace.hpp"
#include "ossLikely.hpp"

namespace engine
{
namespace vessel
{
   indexScanner::~indexScanner()
   {
      close();
   }

   INT32 indexScanner::open(requestContext *context,
                            INT32 indexSlot,
                            const indexObject &indexObj, 
                            INT32 direction)
   {
      INT32 rc = SDB_OK;
      indexHandle handle;
      close();

      if (OSS_UNLIKELY(NULL == context ||
                       !isValidIndexSlot(indexSlot) ||
                       !indexObj.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _context = context;
      _itr = createIndexIterator(indexObj.getIndexType());
      if (NULL == _itr)
      {
         PD_LOG(PDERROR, "failed to create new itr obj");
         rc = SDB_OOM;
         goto error;
      }

      handle = indexHandle(indexSlot, indexObj.getIndexID());
      rc = _itr->open(context, handle,
                      indexObj.getPattern().getOrdering(),
                      direction);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open index iterator:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      close();
      goto done;
   }

   void indexScanner::close()
   {
      if (NULL != _context)
      {
         if (NULL != _itr)
         {
            _itr->close();
            SDB_OSS_DEL _itr;
         }

         _seeked = FALSE;
         _context = NULL;
      }
      return;
   }

   INT32 indexScanner::findOne(requestContext *context,
                               INT32 indexSlot,
                               const indexObject &indexObj,
                               const bson::BSONObj &key,
                               recordID &rid)
   {
      INT32 rc = SDB_OK;
      indexHandle handle;
      indexIterator *iterator = NULL;

      if (OSS_UNLIKELY(NULL == context ||
                       DMS_INVALID_LOGICCSID == context->getLogicalCSID() ||
                       DMS_INVALID_LOGICCLID == context->getLogicalCLID() ||
                       !isValidIndexSlot(indexSlot) ||
                       !indexObj.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rid = recordID();
      handle = indexHandle(indexSlot, indexObj.getIndexID());
      iterator = createIndexIterator(indexObj.getIndexType());
      if (NULL == iterator)
      {
         PD_LOG(PDERROR, "failed to allocate itr obj");
         rc = SDB_OOM;
         goto error;
      }

      rc = iterator->open(context, handle,
                          indexObj.getPattern().getOrdering(), 1);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open iterator:%d", rc);
         goto error;
      }

      rc = iterator->seek(key, FALSE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to seek key:%d", rc);
         goto error;
      }

      while (iterator->isReadyToRead())
      {
         if (iterator->isMarkedRemoved())
         {
            rc = iterator->nextDiffKeyOrRid();
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to get next tuple:%d", rc);
               goto error;
            }
         }
         else
         {
            ixmKey ik;
            iterator->getKey(ik);
            _ixmKeyOwned ownedKey(key);
            if (ik.woEqual(ownedKey))
            {
               rid = iterator->getRid();
               SDB_ASSERT(rid.valid(), "impossible");
            }
            break;
         }
      }
   done:
      if (NULL != iterator)
      {
         iterator->close();
         SDB_OSS_DEL iterator;
      }
      return rc;
   error:
      goto done;
   }
} // namespace vessel   
} // namespace vessel
