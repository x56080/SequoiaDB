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

   Source File Name = btreeIndexAccessor.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/btreeIndexAccessor.h"
#include "vessel/requestContext.h"
#include "vessel/indexContext.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"
#include "vessel/instanceEnv.h"
#include "vessel/indexSpace.h"
#include "vessel/btreeNodePage.h"
#include "vessel/indexEntryPageAccessor.h"
#include "vessel/btreeNodePageIniter.h"

namespace engine
{
namespace vessel
{
   btreeIndexAccessor::btreeIndexAccessor()
   {}

   btreeIndexAccessor::~btreeIndexAccessor()
   {
      fini();
   }

   INT32 btreeIndexAccessor::init(requestContext *context,
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

   void btreeIndexAccessor::fini()
   {
      _context = NULL;
      _is = NULL;
      _ic = NULL;
      
      return;
   }

   INT32 btreeIndexAccessor::insert(const bson::BSONObj &key,
                                    const recordID &rid,
                                    const DPS_TRANS_ID &transID)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isInitialized(), "must be inited");
      btreeInsertContext bic(_ic);
      BOOLEAN checkpointBlocked = FALSE;
      ossSharedLatchMode mode;

      rc = bic.init(ixmKeyOwned(key), rid, transID);
      if (SDB_OK != rc)
      {
         goto error;
      }

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
      rc = pushRootIntoPath(mode, bic.getPath());
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

   INT32 btreeIndexAccessor::traverseDownToInsert(btreeInsertContext &bic)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isInitialized(), "must be inited");
      SDB_ASSERT(bic.getPath().isEmpty(), "must be empty");
      SDB_ASSERT(!bic.isObstructed(), "can not be obstructed");

      btreeNode node = bic.getPath().getCurrentEndNodeInPath();
      btreeNode::locateResult lr;
      BOOLEAN obstructed = FALSE;

      if (node.hasExtNode())
      {
         rc = tryToSplitNode(bic.getPath(), node, obstructed);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to split node with ext node:%d", rc);
            goto error;
         }
      }

      rc = node.locateKeyAndRid(bic.getKey(), bic.getRid(), lr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to locate in node:%d", rc);
         goto error;
      }

      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeIndexAccessor::tryToSplitNode(btreeNodePath &path,
                                            btreeNode &node,
                                            BOOLEAN &obstructed)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isInitialized(), "must be inited");
      SDB_ASSERT(path.isEmpty(), "must be empty");
      SDB_ASSERT(node.isValid(), "can not be invalid");

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

   INT32 btreeIndexAccessor::tryToSplitRootNode(btreeNode &root,
                                                BOOLEAN &obstructed)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isInitialized(), "must be inited");
      SDB_ASSERT(root.isRoot(), "must be root");

      PAGE_ID brotherLpid = INVALID_PAGE_ID;
      btreeNodePageIniter initer;

      if (!root.tryToEnsureLockExlusive())
      {
         obstructed = TRUE;
         goto done;
      }

      initer.set(_context->getLogicalCLID(), _ic->getIndexID());
      rc = _is->allocatePages(_context, &initer, 1, &brotherLpid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate new brother page:%d", rc);
         goto error;
      }
   done:
      if (INVALID_PAGE_ID != brotherLpid)
      {
         _is->releasePages(_context, 1, &brotherLpid);
      }
      return rc;
   error:
      goto done;
   }

   INT32 btreeIndexAccessor::pushNodeIntoPath(PAGE_ID lpid,
                                              const ossSharedLatchMode &mode,
                                              btreeNodePath &path,
                                              btreeNode *out)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isInitialized(), "must be inited");
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      SDB_ASSERT(!mode.isNone(), "can not be none");

      logicalPageBuffer *buffer = path.allocateBuffer();
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

      rc = path.push(buffer, out);
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
         path.releaseBuffer(buffer);
      }
      goto done;
   }

   INT32 btreeIndexAccessor::pushRootIntoPath(ossSharedLatchMode mode,
                                              btreeNodePath &path)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isInitialized(), "must be inited");
      SDB_ASSERT(!mode.isNone(), "can not be none");
      SDB_ASSERT(path.isEmpty(), "must be empty");

      PAGE_ID rootLpid = _ic->getObj().getBtreeRoot();
      SDB_ASSERT(INVALID_PAGE_ID != rootLpid, "can not be invalid");
      logicalPageBuffer *buffer = path.allocateBuffer();
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
            break;
         }

         buffer->fini();
         continue;
      } while (TRUE);
      
      
   done:
      return rc;
   error:
      if (NULL != buffer)
      {
         buffer->fini();
         path.releaseBuffer(buffer);
      }
      goto done;
   }

   INT32 btreeIndexAccessor::createRootIfNotExists()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isInitialized(), "must be inited");

      logicalPageBuffer entryBuffer;
      indexEntryPageAccessor accessor;
      btreeNodePageIniter initer;
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
                 getIndexContext()->getIndexID());
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

   ossSharedLatchMode btreeIndexAccessor::estimateRootModeWhenWriting(UINT32 updatedTimes)const
   {
      static const UINT32 _SMALL_SCALE = 2;
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

   ossSharedLatchMode btreeIndexAccessor::estimateChildModeWhenInserting(const btreeNodePath &path)const
   {
      ossSharedLatchMode mode;
      SDB_ASSERT(!path.isEmpty(), "can not be empty");
      btreeNode father = path.getCurrentEndNodeInPath();
      ossSharedLatchMode fatherMode = father.getMode();
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
         /// which means will get unshared latch from the third level of tree.
         mode.setUpgrade();
      }
      return mode;
   }

   INT32 btreeIndexAccessor::validateBtreePage(const logicalPageBuffer &buffer)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isInitialized(), "must be inited");
      SDB_ASSERT(buffer.isValid(), "can not be invalid");

      const runtimePageBuffer &rpb = buffer.getRuntimeBuffer();
      const btreeNodePageHead *head = NULL;

      rc = buffer.validatePage(PAGE_TYPE_BTREE_NODE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to validate btree page:[%s], rc:%d",
                rpb.getGlobalPid().toString().c_str(), rc);
         goto error;
      }

      head = rpb.getReadablePtrOfBody<btreeNodePageHead>(0);
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
