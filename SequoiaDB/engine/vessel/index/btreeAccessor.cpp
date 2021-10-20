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

   Source File Name = btreeAccessor.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/btreeAccessor.h"
#include "vessel/requestContext.h"
#include "vessel/indexContext.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"
#include "vessel/instanceEnv.h"
#include "vessel/indexSpace.h"
#include "vessel/btreeNodePage.h"
#include "vessel/indexEntryPageAccessor.h"
#include "vessel/btreeNodePageIniter.h"
#include "vessel/btreeAccessPathNode.h"

namespace engine
{
namespace vessel
{
   btreeAccessor::btreeAccessor()
   {}

   btreeAccessor::~btreeAccessor()
   {
      fini();
   }

   INT32 btreeAccessor::init(requestContext *context,
                                  indexContext *ic)
   {
      INT32 rc = SDB_OK;
      logicalPageSpace *lps = NULL;

      fini();

      if (OSS_UNLIKELY(NULL == context ||
                       !context->getCollectionHandle().isValid() ||
                       NULL == ic ||
                       !ic->isValid() ||
                       ic->getObj().getParams().type != INDEX_TYPE_BTREE))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _context = context;
      _ic = ic;

      rc = context->getEnv()->dms.getLogicalPageSpace(context->getSpaceID(),
                                                      SPACE_TYPE_IDX,
                                                      &lps);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get index space[%d]:%d", context->getSpaceID(), rc);
         goto error;
      }

      _is = static_cast<indexSpace *>(lps);
   done:
      return rc;
   error:
      fini();
      goto done;
   }

   void btreeAccessor::fini()
   {
      _context = NULL;
      _is = NULL;
      _ic = NULL;
      
      return;
   }

   INT32 btreeAccessor::insert(const bson::BSONObj &key,
                               const recordID &rid,
                               const DPS_TRANS_ID &transID)
   {
      INT32 rc = SDB_OK;
      btreeAccessContext bac;
      BOOLEAN checkpointBlocked = FALSE;
      ossSharedLatchMode mode;

      if (OSS_UNLIKELY(isInitialized()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(!key.isValid() ||
                            !rid.valid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      bac.init(_ic, ixmKeyOwned(key), &rid, &transID);

      rc = _is->blockCheckpoint(_context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to block checkpoint:%d", rc);
         goto error;
      }
      checkpointBlocked = TRUE;

      rc = createRootIfNotExists();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to ensure root node:%d", rc);
         goto error;
      }

      mode = estimateRootModeWhenWriting(_ic->getObj().getBtreeRootUpdatedTimes());
      rc = pushRootIntoPath(mode, bac);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to push root node into path:%d", rc);
         goto error;
      }
   done:
      if (checkpointBlocked)
      {
         _context->unblockCheckpoint();
      }
      return rc;
   error:
      goto done;
   }

   INT32 btreeAccessor::traverseDownToInsert(btreeAccessContext &bac,
                                             BOOLEAN &obstructed)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isInitialized(), "must be inited");
      SDB_ASSERT(!bac.isPathEmpty(), "can not be empty");

      btreeNode node = bac.getEndNodeInPath();
      obstructed = FALSE;
      btreeItemLocation location;

      if (node.isLeaf())
      {  
         if (node.hasFreeSpaceToInsert(bac.getKey().dataSize()))
         {
            bac.endToAccessPathNodes(1);
            rc = insertIntoLeafNode(node, 
                                    bac.getKey(),
                                    bac.getRid(),
                                    bac.getTransID(),
                                    obstructed);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to insert into leaf:%d", rc);
               goto error;
            }
         }
         else
         {
            rc = splitLeafNodeAndInsert(bac, obstructed);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to split and insert:%d", rc);
               goto error;
            }
         }

         goto done;
      }

      rc = node.locateKeyAndRid(bac.getKey(), bac.getRid(), location);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to locate key and rid:%d", rc);
         goto error;
      }

      if (location.identical)
      {
         SDB_ASSERT(!node.isLeaf(), "impossible");
         bac.endToAccessPathNodes(1);
         if (!node.ensureExclusiveLocking())
         {
            obstructed = TRUE;
            goto done;
         }

         rc = node.reactiveRemovedKey(location, bac.getTransID());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to reactive identical key:%d", rc);
            goto error;
         }

         goto done;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeAccessor::insertIntoLeafNode(btreeNode &node,
                                           const ixmKey &key,
                                           const recordID &rid,
                                           const DPS_TRANS_ID &transID,
                                           BOOLEAN &obstructed)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(node.isValid() && node.isLeaf(), "can not be invalid");
      SDB_ASSERT(key.isValid(), "can not be invalid");
      SDB_ASSERT(rid.valid(), "can not be invalid");
      obstructed = node.ensureExclusiveLocking();
      if (!obstructed)
      {
         goto done;
      }

      rc = node.insert(key, rid, transID);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to insert into node:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeAccessor::splitLeafNodeAndInsert(btreeAccessContext &bac,
                                               BOOLEAN &obstructed)
   {
      INT32 rc = SDB_OK;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeAccessor::tryToSplitEndNode(btreeAccessContext &bac,
                                          BOOLEAN &obstructed)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isInitialized(), "must be inited");
      SDB_ASSERT(!bac.isPathEmpty(), "can not be empty");
      btreeNode node = bac.getEndNodeInPath();

      if (node.isRoot())
      {
         rc = tryToSplitRootNode(node, obstructed);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to split root node:%d", rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeAccessor::tryToSplitRootNode(btreeNode &root,
                                           BOOLEAN &obstructed)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isInitialized(), "must be inited");
      SDB_ASSERT(root.isRoot(), "must be root");

      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeAccessor::pushNoneRootNodeIntoPath(PAGE_ID lpid,
                                                      const ossSharedLatchMode &mode,
                                                      btreeAccessContext &bac)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isInitialized(), "must be inited");
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      SDB_ASSERT(!mode.isNone(), "can not be none");

      logicalPageBuffer *buffer = bac.allocateBuffer();
      if (OSS_UNLIKELY(NULL == buffer))
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      rc = _is->getLogicalPageBuffer(_context, lpid, mode, *buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page[%d] buffer:%d", lpid, rc);
         goto error;
      }

      rc = validateBtreePage(*buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to validate btree node page:%d", rc);
         goto error;
      }

      rc = bac.pushIntoPath(buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to push node into path:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      if (NULL != buffer)
      {
         bac.releaseBuffer(buffer);
      }
      goto done;
   }

   INT32 btreeAccessor::pushRootIntoPath(ossSharedLatchMode mode,
                                              btreeAccessContext &bac)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isInitialized(), "must be inited");
      SDB_ASSERT(!mode.isNone(), "can not be none");
      SDB_ASSERT(bac.isPathEmpty(), "must be empty");

      PAGE_ID rootLpid = _ic->getObj().getBtreeRoot();
      SDB_ASSERT(INVALID_PAGE_ID != rootLpid, "can not be invalid");
      logicalPageBuffer *buffer = bac.allocateBuffer();
      if (OSS_UNLIKELY(NULL == buffer))
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      do
      {
         rc = _is->getLogicalPageBuffer(_context, rootLpid, mode, *buffer);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get page buffer of page[%d], rc:%d",
                  rootLpid, rc);
            goto error;
         }

         /// root must be checked again under locking.
         if (_ic->getObj().getBtreeRoot() == rootLpid)
         {
            rc = validateBtreePage(*buffer);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to validate root buffer:%d", rc);
               goto error;
            }

            break;
         }

         buffer->fini();
         continue;
      } while (TRUE);
      
      rc = bac.pushIntoPath(buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to push root buffer into path:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      if (NULL != buffer)
      {
         buffer->fini();
         bac.releaseBuffer(buffer);
      }
      goto done;
   }

   INT32 btreeAccessor::createRootIfNotExists()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isInitialized(), "must be inited");

      logicalPageBuffer entryBuffer;
      indexEntryPageAccessor accessor;
      btreeRootPageIniter initer;
      PAGE_ID lpid = INVALID_PAGE_ID;
      ossSharedLatchMode mode;
      mode.setExclusive();

      if (_ic->getObj().hasBtreeRoot())
      {
         goto done;
      }

      rc = _is->getLogicalPageBuffer(_context,
                                     _ic->getEntryLpid(),
                                     mode, entryBuffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get entry page[%d] buffer:%d",
                _ic->getEntryLpid(), rc);
         goto error;
      }

      /// check again under entry page locking.
      if (_ic->getObj().hasBtreeRoot())
      {
         goto done;
      }

      initer.set(getContext()->getLogicalCLID(),
                 getIndexContext()->getIndexID(),
                 INVALID_PAGE_ID);
      rc = _is->allocatePages(_context, &initer, 1, &lpid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate new page:%d", rc);
         goto error;
      }

      rc = accessor.updateBtreeRoot(_context,
                                    _ic->getIndexID(),
                                    lpid, &entryBuffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to update root:%d", rc);
         goto error;
      }

      _ic->getObj().updateBtreeRoot(lpid);

   done:
      entryBuffer.fini();
      return rc;
   error:
      if (INVALID_PAGE_ID != lpid)
      {
         _is->releasePages(getContext(), 1, &lpid);
      }
      goto done;
   }

   ossSharedLatchMode btreeAccessor::estimateRootModeWhenWriting(UINT32 updatedTimes)const
   {
      static const UINT32 _SMALL_SCALE = 2;
      ossSharedLatchMode mode;
      if (updatedTimes <= _SMALL_SCALE)
      {
         mode.setExclusive();
      }
      else
      {
         mode.setShared();
      }
      return mode;
   }

   ossSharedLatchMode btreeAccessor::estimateChildModeWhenInserting(btreeAccessContext &bac)const
   {
      ossSharedLatchMode mode;
      SDB_ASSERT(!bac.isPathEmpty(), "can not be empty");
      btreeNode father = bac.getEndNodeInPath();
      ossSharedLatchMode fatherMode = father.getLockingMode();
      if (!fatherMode.isShared())
      {
         mode = fatherMode;
      }
      else if (father.isRoot())
      {
         mode.setShared();
      }
      else
      {
         /// which means will get unshared latch from the depth 2.
         mode.setExclusive();
      }
      return mode;
   }

   INT32 btreeAccessor::validateBtreePage(const logicalPageBuffer &buffer)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isInitialized(), "must be inited");
      SDB_ASSERT(buffer.isValid(), "can not be invalid");

      const runtimePageBuffer &rpb = buffer.getRuntimeBuffer();
      const btreeNodePageHead *head = NULL;
      slice s;

      rc = buffer.validatePage(PAGE_TYPE_BTREE_NODE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to validate btree page:[%s], rc:%d",
                rpb.getGlobalPid().toString().c_str(), rc);
         goto error;
      }

      s = buffer.getReadableBodySlice();

      head = s.getReadableObjPtr<btreeNodePageHead>(0);
      if (NULL == head)
      {
         PD_LOG(PDERROR, "failed to get btree page head");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (_context->getLogicalCLID() != head->clLogicalID)
      {
         PD_LOG(PDERROR, "different logical clids found[%d,%d] on page[%s]",
                _context->getLogicalCLID(), head->clLogicalID,
                rpb.getGlobalPid().toString().c_str());
         rc = SDB_VESSEL_PAGE_HEAD_NOT_MATCH;
         goto error;
      }

      if (_ic->getIndexID() != head->indexId)
      {
         PD_LOG(PDERROR, "different logical index ids found[%d,%d] on page[%s]",
                _ic->getIndexID(), head->indexId, rpb.getGlobalPid().toString().c_str());
         rc = SDB_VESSEL_PAGE_HEAD_NOT_MATCH;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

} // namespace vessel

} // namespace engine
