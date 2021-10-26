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

   INT32 btreeAccessor::insert(const ixmKey &key,
                               const recordID &rid,
                               const DPS_TRANS_ID &transID)
   {
      INT32 rc = SDB_OK;
      btreeAccessContext bac;
      BOOLEAN checkpointBlocked = FALSE;
      ossSharedLatchMode mode;
      BOOLEAN obstructed = FALSE;

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

      bac.init(_ic, key, &rid, &transID);

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
         PD_LOG(PDERROR, "failed to ensure root node created:%d", rc);
         goto error;
      }

      rc = traverseDownAndInsert(bac, obstructed);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to insert key and rid:%d", rc);
         goto error;
      }

      if (obstructed)
      {
         bac.setPessimistic(TRUE);
         obstructed = FALSE;
         rc = traverseDownAndInsert(bac, obstructed);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to insert key and rid:%d", rc);
            goto error;
         }

         if (obstructed)
         {
            PD_LOG(PDERROR, "get unexpected obstructing");
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
      }
   done:
      bac.fini();
      if (checkpointBlocked)
      {
         _context->unblockCheckpoint();
      }
      return rc;
   error:
      goto done;
   }

   INT32 btreeAccessor::traverseDownAndInsert(btreeAccessContext &bac,
                                              BOOLEAN &obstructed)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isInitialized(), "must be inited");
      SDB_ASSERT(!bac.isPathEmpty(), "can not be empty");

      btreeNode node;
      obstructed = FALSE;

      if (bac.isPathEmpty())
      {
         rc = pushRootIntoPath(bac);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to push root node into path:%d", rc);
            goto error;
         }
      }

      node = bac.getEndNodeInPath();

      if (node.isLeaf())
      {  
         if (node.hasFreeSpaceToInsert(bac.getKey().dataSize()))
         {
            bac.endToAccessPathNodes(1);
            rc = insertWhenPathEndIsLeaf(bac, obstructed);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to insert into leaf node[%d,%d]:%d",
                      _ic->getIndexID(), node.getBuffer()->getLogicalPid(), rc);
               goto error;
            }
         }
         else
         {
            rc = splitAndInsertWhenPathEndIsLeaf(bac, obstructed);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to split and insert:%d", rc);
               goto error;
            }
         }

         bac.clearAccessPath();
      }
      else
      {
         btreeItemLocation location;
         rc = node.locateKeyAndRid(bac.getKey(), bac.getRid(), location);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to locate key and rid:%d", rc);
            goto error;
         }

         if (location.identical)
         {
            bac.endToAccessPathNodes(1);
            if (!node.ensureExclusiveLocking())
            {
               obstructed = TRUE;
               goto done;
            }

            rc = node.reactiveRemovedKey(location, bac.getTransID());
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to reactive non-leaf node item:%d", rc);
               goto error;
            }

            bac.clearAccessPath();
         }
         else if (node.hasExternalKey())
         {
            rc = splitNonLeafPathEnd(bac, obstructed);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to split node with ext key:%d", rc);
               goto error;
            }

            if (obstructed)
            {
               bac.clearAccessPath();
               goto done;
            }

            /// restart inserting from the last node which received raised key.
            rc = traverseDownAndInsert(bac, obstructed);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to traverse down and insert after split:%d", rc);
               goto error;
            }
         }
         else/// be sure to traverse down from this non-leaf node
         {
            /// node's free space is enough to save raised key with any size,
            /// end to access ancestors.
            if (node.isSpaceSpare())
            {
               bac.endToAccessPathNodes(1);
            }

            rc = pushNoneRootNodeIntoPath(location.child,
                                          estimateChildModeWhenWriting(bac),
                                          bac);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to push child node into path:%d", rc);
               goto error;
            }

            rc = traverseDownAndInsert(bac, obstructed);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to traverse down and insert after split:%d", rc);
               goto error;
            }
         }
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeAccessor::splitNonLeafPathEnd(btreeAccessContext &bac,
                                            BOOLEAN &obstructed)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isInitialized(), "can not be invalid");
      SDB_ASSERT(bac.isValid() && 1 < bac.getPathDepth(), "can not be invalid");

      btreeSplitRaisedKey raisedKey;
      btreeNode node = bac.getEndNodeInPath();
      SDB_ASSERT(!node.isLeaf(), "can not be leaf");
      obstructed = FALSE;

      btreeNode father = bac.getNodeInPath(node.getDepth() - 1);
      if (!node.ensureExclusiveLocking() ||
          !father.ensureExclusiveLocking())
      {
         obstructed = TRUE;
         goto done;
      }

      rc = node.split(raisedKey);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to split node[%d], rc:%d",
                node.getBuffer()->getLogicalPid(), rc);
         goto error;
      }

      bac.popEnd();
      rc = traverseUpAndInsert(bac, raisedKey);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to insert raised key into ancestors:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeAccessor::insertWhenPathEndIsLeaf(btreeAccessContext &bac,
                                                BOOLEAN &obstructed)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isInitialized(), "can not be invalid");
      SDB_ASSERT(bac.isValid() && !bac.isPathEmpty(), "can not be invalid");

      btreeNode node = bac.getEndNodeInPath();
      SDB_ASSERT(node.isLeaf(), "must be leaf node");
      obstructed = node.ensureExclusiveLocking();
      if (!obstructed)
      {
         goto done;
      }

      rc = node.leafInsert(bac.getKey(), bac.getRid(), bac.getTransID());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to insert into leaf node:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeAccessor::splitAndInsertWhenPathEndIsLeaf(btreeAccessContext &bac,
                                                        BOOLEAN &obstructed)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isInitialized(), "can not be invalid");
      SDB_ASSERT(bac.isValid() && !bac.isPathEmpty(), "can not be invalid");
      btreeNode node = bac.getEndNodeInPath();
      SDB_ASSERT(node.isLeaf(), "must be leaf");

      if (node.isRoot())
      {
         rc = splitAndInsertWhenPathEndIsRoot(bac, NULL, obstructed);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to split root and insert:%d", rc);
            goto error;
         }
      }
      else
      {
         btreeSplitRaisedKey raisedKey;
         btreeNode father = bac.getNodeInPath(node.getDepth() - 1);
         SDB_ASSERT(father.isValid(), "must be valid");
         if (!father.ensureExclusiveLocking())
         {
            obstructed = TRUE;
            goto done;
         }

         rc = node.splitLeafAndInsert(bac.getKey(), bac.getRid(),
                                      bac.getTransID(), raisedKey);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to split leaf node [%d] and insert:%d",
                   node.getBuffer()->getLogicalPid(), rc);
            goto error;
         }

         bac.popEnd();
         rc = traverseUpAndInsert(bac, raisedKey);
         if (SDB_OK != rc)
         {
            PD_LOG(PDSEVERE, "failed to traverse up and insert raised key:%d", rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeAccessor::splitAndInsertWhenPathEndIsRoot(btreeAccessContext &bac,
                                                        const btreeSplitRaisedKey *raisedKey,
                                                        BOOLEAN &obstructed)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isInitialized(), "can not be invalid");
      SDB_ASSERT(bac.isValid() && 1 == bac.getPathDepth(), "can not be invalid");
      btreeNode node = bac.getEndNodeInPath();
      SDB_ASSERT(node.isRoot(), "must be root");
      SDB_ASSERT(!(node.isLeaf() && NULL != raisedKey), "impossible");
      btreeRootPageIniter initer;
      PAGE_ID newRoot = INVALID_PAGE_ID;
      logicalPageBuffer newRootBuffer;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);
      btreeSplitRaisedKey newRaisedKey;
      logicalPageBuffer entryBuffer;
      indexEntryPageAccessor accessor;
      UINT32 rootUpdatedTimes = 0;

      if (!node.ensureExclusiveLocking())
      {
         obstructed = TRUE;
         goto done;
      }

      rc = _is->getLogicalPageBuffer(_context, _ic->getEntryLpid(), mode, entryBuffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get entry page buffer:%d", rc);
         goto error;
      }

      SDB_ASSERT(_ic->getObj().getBtreeRoot() == node.getBuffer()->getLogicalPid(),
                 "must be same");

      initer.set(_context->getLogicalCLID(), _ic->getIndexID());
      rc = _is->allocatePage(_context, &initer, newRoot);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate new root node page:%d", rc);
         goto error;
      }

      rc = _is->getLogicalPageBuffer(_context, newRoot, mode, newRootBuffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get new root node buffer:%d", rc);
         goto error;
      }

      rc = entryBuffer.prepareToWrite();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get entry page ready to write:%d", rc);
         goto error;
      }

      if (NULL == raisedKey)
      {
         rc = node.splitNonLeafAndInsert(*raisedKey, bac.getTransID(), newRaisedKey);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to split root and insert raised key:%d", rc);
            goto error;
         }
      }
      else
      {
         rc = node.splitLeafAndInsert(bac.getKey(), bac.getRid(),
                                    bac.getTransID(), newRaisedKey);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to split and insert key into root:%d", rc);
            goto error;
         }
      }

      rc = btreeNode(&newRootBuffer, _ic, 0).insertRaisedKey(newRaisedKey, bac.getTransID());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to insert raised key into new root:%d", rc);
         goto error;
      }

      rc = accessor.updateBtreeRoot(_context, _ic->getIndexID(),
                                    newRoot, &newRootBuffer,
                                    &rootUpdatedTimes);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to update btree root in entry page:%d", rc);
         goto error;
      }

      _ic->getObj().updateBtreeRoot(newRoot, &rootUpdatedTimes);
      /// clear accessing path cause we updated root node.
      bac.clearAccessPath();
   done:
      entryBuffer.fini();
      newRootBuffer.fini();
      return rc;
   error:
      if (INVALID_PAGE_ID != newRoot)
      {
         _is->releasePage(_context, newRoot);
      }
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

   INT32 btreeAccessor::pushRootIntoPath(btreeAccessContext &bac,
                                         const ossSharedLatchMode &mode)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isInitialized(), "must be inited");
      SDB_ASSERT(bac.isValid(), "can not be invalid");
      SDB_ASSERT(bac.isPathEmpty(), "must be empty");

      ossSharedLatchMode rootMode = mode.isNone() ?
                                    estimateRootMode(bac) : mode;
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

   ossSharedLatchMode btreeAccessor::estimateRootMode(const btreeAccessContext &bac)const
   {
      SDB_ASSERT(isInitialized(), "must be inited");
      SDB_ASSERT(bac.isValid(), "can not be invalid");
      SDB_ASSERT(bac.isPathEmpty(), "must be empty");

      ossSharedLatchMode mode;
      if (bac.isReadonly())
      {
         mode.setShared();
      }
      else if (bac.isPessimistic())
      {
         mode.setExclusive();
      }
      else
      {
         mode = estimateRootModeWhenWriting(_ic->getObj().getBtreeRootUpdatedTimes());
      }

      return mode;
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

   ossSharedLatchMode btreeAccessor::estimateChildModeWhenWriting(btreeAccessContext &bac)const
   {
      static const UINT32 _MAX_SHARED_DEPTH = 1;
      ossSharedLatchMode mode;
      SDB_ASSERT(!bac.isPathEmpty(), "can not be empty");
      btreeNode father = bac.getEndNodeInPath();
      ossSharedLatchMode fatherMode = father.getLockingMode();
      if (!fatherMode.isShared())
      {
         mode = fatherMode;
      }
      else if (father.getDepth() < _MAX_SHARED_DEPTH)
      {
         mode.setShared();
      }
      else
      {
         /// which means will get exclusive latch from the depth 2.
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

   INT32 btreeAccessor::traverseUpAndInsert(btreeAccessContext &bac,
                                            const btreeSplitRaisedKey &raisedKey)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isInitialized(), "can not be invalid");
      SDB_ASSERT(bac.isValid(), "can not be invalid");
      SDB_ASSERT(raisedKey.isValid(), "can not be invalid");
      btreeNode node = bac.getEndNodeInPath();
      SDB_ASSERT(node.getLockingMode().isExclusive(),
                 "must hold exlusive latch first");
      SDB_ASSERT(!node.hasExternalKey(), "must split node first");
      SDB_ASSERT(!node.isLeaf(), "can not be leaf");
      SDB_ASSERT(node.hasFreeSpaceToInsert(0),
                 "one slot should always be reserved");
      BOOLEAN obstructed = FALSE;
      
      if (node.hasFreeSpaceToInsert(raisedKey.getKeySize() + BTREE_NODE_SLOT_SIZE))
      {
         rc = node.insertRaisedKey(raisedKey, bac.getTransID());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to insert raised key into node:%d", rc);
            goto error;
         }
      }
      else if (node.isRoot())
      {
         rc = splitAndInsertWhenPathEndIsRoot(bac, &raisedKey, obstructed);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to split root and insert raised key:%d", rc);
            goto error;
         }
         else if (obstructed)
         {
            PD_LOG(PDERROR, "get unexpected obstructing when split root");
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
      }
      else
      {
         btreeNode father = bac.getNodeInPath(node.getDepth() - 1);
         if (!father.isValid())
         {
            PD_LOG(PDERROR, "failed to get father node of depth[%d]",
                   node.getDepth());
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         if (!father.ensureExclusiveLocking())
         {
            rc = node.insertRaisedKey(raisedKey, bac.getTransID());
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to insert raised key into node:%d", rc);
               goto error;
            }
         }
         else
         {
            btreeSplitRaisedKey newRaisedKey;
            rc = node.splitNonLeafAndInsert(raisedKey, bac.getTransID(), newRaisedKey);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to split node and insert raised key:%d", rc);
               goto error;
            }

            bac.popEnd();
            rc = traverseUpAndInsert(bac, newRaisedKey);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to traverse up to insert raised key:%d", rc);
               goto error;
            }
         }
      }
   done:
      return rc;
   error:
      goto done;
   }
} // namespace vessel

} // namespace engine
