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

   Source File Name = btreeIndexWriter.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/btreeIndexWriter.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"
#include "vessel/indexSpace.h"
#include "vessel/indexContext.h"
#include "vessel/indexDefPageAccessor.h"
#include "vessel/btreeNodePageIniter.h"
#include "vessel/requestContext.h"

namespace engine
{
namespace vessel
{  
   INT32 btreeIndexWriter::insert(const bson::BSONObj &key,
                                  const recordID &rid,
                                  DPS_LSN_OFFSET lsn,
                                  const DPS_TRANS_ID &transID)
   {
      INT32 rc = SDB_OK;

      btreeNode root;
      ixmKeyOwned ownedKey(key);

      if (OSS_UNLIKELY(!isInitialized()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(!key.isValid() ||
                            !rid.valid() ||
                            DPS_INVALID_LSN_OFFSET == lsn))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if ((INT32)MAX_IXM_KEY_SIZE < ownedKey.dataSize())
      {
         rc = SDB_IXM_KEY_TOO_LARGE;
         goto error;
      }

      rc = initPathRoot(root);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init path root:%d", rc);
         goto error;
      }

   done:
      btreeIndexAccessor::clearAccessingPath();
      return rc;
   error:
      goto done;
   }

   INT32 btreeIndexWriter::initPathRoot(btreeNode &root,
                                        const ossSharedLatchMode *m)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isInitialized(), "must be inited");
      SDB_ASSERT(getNodePath().isEmpty(), "must be empty");

      indexDefPageAccessor accessor;
      ossSharedLatchMode entryMode;
      entryMode.setShared();
      ossSharedLatchMode rootMode;
      const indexDefHead *head = NULL;
      logicalPageBuffer entryPage;

      do
      {
         rc = getIndexSpace()->getLogicalPageBuffer(getContext(),
                                                 getIndexContext()->getEntryLpid(),
                                                 entryMode, entryPage);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get entry page[%d] buffer, rc:%d",
                  getIndexContext()->getEntryLpid(), rc);
            goto error;
         }

         rc = accessor.getIndexDefPageHead(getContext(),
                                          getIndexContext()->getIndexID(),
                                          &entryPage, &head);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get index def page head:%d", rc);
            goto error;
         }

         if (INVALID_PAGE_ID == head->btreeRoot)
         {
            entryPage.fini();
            PAGE_ID rootLpid = INVALID_PAGE_ID;
            rc = createRootIfNotExists(rootLpid);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to ensure root node:%d", rc);
               goto error;
            }
            continue;
         }

         break;
      } while (TRUE);
      
      if (NULL != m && !m->isNone())
      {
         rootMode = *m;
      }
      else
      {
         rootMode = estimateRootLockingMode(head->btreeRootUpdatedTimes);
      }

      rc = getBtreeNodeAndPushIntoPath(head->btreeRoot, rootMode, root);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get btree node[%d], rc:%d", head->btreeRoot, rc);
         goto error;
      }

   done:
      entryPage.fini();
      return rc;
   error:
      root.reset();
      goto done;
   }

   ossSharedLatchMode btreeIndexWriter::estimateRootLockingMode(UINT32 updatedTimes)const
   {
      static const UINT32 _SMALL_SCALE = 1;
      ossSharedLatchMode mode;
      if (updatedTimes <= _SMALL_SCALE)
      {
         mode.setUpgrade();
      }
      else
      {
         mode.setShared();
      }
      return mode;
   }

   ossSharedLatchMode btreeIndexWriter::estimateChildLockingMode(UINT32 depth,
                                                     const ossSharedLatchMode &fatherMode)const
   {
      static const UINT32 _MIN_UPGRADE_DEPTH = 2;
      SDB_ASSERT(!fatherMode.isNone(), "can not be none");
      ossSharedLatchMode mode;
      if (!fatherMode.isShared())
      {
         mode.setUpgrade();
      }
      else if (_MIN_UPGRADE_DEPTH <= depth)
      {
         mode.setUpgrade();
      }
      else
      {
         mode.setShared();
      }
      return mode;
   }

   INT32 btreeIndexWriter::createRootIfNotExists(PAGE_ID &root)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isInitialized(), "must be inited");

      indexDefPageAccessor accessor;
      btreeNodePageIniter initer;
      PAGE_ID lpid = INVALID_PAGE_ID;
      ossSharedLatchMode mode;
      mode.setUpgrade();
      logicalPageBuffer entryPage;
      const indexDefHead *head = NULL;

      rc = getIndexSpace()->getLogicalPageBuffer(getContext(),
                                                 getIndexContext()->getEntryLpid(),
                                                 mode, entryPage);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get entry page[%d] buffer:%d",
                getIndexContext()->getEntryLpid(), rc);
         goto error;
      }

      rc = accessor.getIndexDefPageHead(getContext(),
                                        getIndexContext()->getIndexID(),
                                        &entryPage, &head);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get index def page head:%d", rc);
         goto error;
      }

      lpid = head->btreeRoot;

      if (INVALID_PAGE_ID == lpid)
      {
         UINT32 updatedTimes = 0;
         initer.set(getContext()->getLogicalCLID(),
                    getIndexContext()->getIndexID());
         rc = getIndexSpace()->allocatePages(getContext(), &initer, 1, &lpid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to allocate new page:%d", rc);
            goto error;
         }

         rc = accessor.updateBtreeRoot(getContext(),
                                       getIndexContext()->getIndexID(),
                                       lpid, &entryPage, &updatedTimes);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to update root:%d", rc);
            goto error;
         }
         SDB_ASSERT(1 == updatedTimes, "impossible");
      }

      root = lpid;

   done:
      entryPage.fini();
      return rc;
   error:
      if (INVALID_PAGE_ID != lpid)
      {
         getIndexSpace()->releasePages(getContext(), 1, &lpid);
      }
      goto done;
   }

} // namespace vessel

} // namespace engine
