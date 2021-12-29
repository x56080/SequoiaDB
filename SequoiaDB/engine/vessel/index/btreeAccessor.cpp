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
#include "vessel/runtimeMbContext.h"

namespace engine
{
namespace vessel
{
   constexpr UINT32 BATCH_RELEASE_COUNT = 8;

   btreeAccessor::btreeAccessor()
   {}

   btreeAccessor::~btreeAccessor()
   {
      fini();
   }

   INT32 btreeAccessor::init(requestContext *context,
                             indexContext *ic,
                             const DPS_TRANS_ID &transID)
   {
      INT32 rc = SDB_OK;
      logicalPageSpace *lps = NULL;

      fini();

      if (OSS_UNLIKELY(NULL == context ||
                       !context->isMbContextAttached() ||
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
      _bac.init(_ic, _context, _is);
      _transID = transID;
      
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
      _bac.fini();
      _transID = DPS_TRANS_ID();
      return;
   }

   INT32 btreeAccessor::insert(const ixmKey &key,
                               const recordID &rid)
   {
      INT32 rc = SDB_OK;
      BOOLEAN checkpointBlocked = FALSE;
      BOOLEAN obstructed = FALSE;

      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(!key.isValid() ||
                            !rid.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if ((INT32)MAX_INDEX_KEY_SIZE < key.dataSize())
      {
         PD_LOG(PDERROR, "key size too large");
         rc = SDB_IXM_KEY_TOO_LARGE;
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
         PD_LOG(PDERROR, "failed to ensure root node created:%d", rc);
         goto error;
      }

      _bac.clearAccessPath();
      _bac.setReadonly(FALSE);
      //_bac.setPessimistic(TRUE);
      rc = traverseDownAndInsert(key, rid, obstructed);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to insert key and rid:%d", rc);
         goto error;
      }

      if (obstructed)
      {
         _bac.clearAccessPath();
         _bac.setReadonly(FALSE);
         _bac.setPessimistic(TRUE);
         
         obstructed = FALSE;
         rc = traverseDownAndInsert(key, rid, obstructed);
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
      if (_bac.isValid())
      {
         _bac.clearAccessPath();
         _bac.setReadonly(FALSE);
         _bac.setPessimistic(FALSE);
      }
      if (checkpointBlocked)
      {
         _context->unblockCheckpoint();
      }
      return rc;
   error:
      goto done;
   }

   INT32 btreeAccessor::remove(const ixmKey &key,
                               const recordID &rid)
   {
      INT32 rc = SDB_OK;
      BOOLEAN checkpointBlocked = FALSE;
      BOOLEAN obstructed = FALSE;
      
      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(!key.isValid() ||
                            !rid.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _is->blockCheckpoint(_context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to block checkpoint:%d", rc);
         goto error;
      }
      checkpointBlocked = TRUE;

      if (!_ic->getObj().hasBtreeRoot())
      {
         PD_LOG(PDERROR, "btree has no root yet");
         rc = SDB_VESSEL_IXM_ITEM_NOT_FOUND;
         goto error;
      }

      _bac.clearAccessPath();
      _bac.setReadonly(FALSE);

      rc = traverseDownAndRemove(key, rid, obstructed);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to remove key and rid:%d", rc);
         goto error;
      }

      if (obstructed)
      {
         obstructed = FALSE;
         _bac.clearAccessPath();
         _bac.setReadonly(FALSE);
         _bac.setPessimistic(TRUE);
         rc = traverseDownAndRemove(key, rid, obstructed);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to remove key and rid:%d", rc);
            goto error;
         }
         else if (obstructed)
         {
            PD_LOG(PDERROR, "get unexpected obstructing");
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
      }
   done:
      if (_bac.isValid())
      {
         _bac.clearAccessPath();
         _bac.setReadonly(FALSE);
         _bac.setPessimistic(FALSE);
      }

      if (checkpointBlocked)
      {
         _context->unblockCheckpoint();
      }
      return rc;
   error:
      goto done;
   }

   INT32 btreeAccessor::truncate()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");

      if (!_ic->getObj().hasBtreeRoot())
      {
         PD_LOG(PDDEBUG, "has no btree root");
         goto done;
      }

      rc = removeBtreeRootInEntry();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to remove btree root in entry page:%d", rc);
         goto error;
      }

      rc = releaseWholeTree();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to release btree:%d", rc);
         goto error;
      }
   done:
      _ic->getObj().removeBtreeRoot();
      return rc;
   error:
      goto done;
   }

   INT32 btreeAccessor::removeBtreeRootInEntry()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "must be inited");
      SDB_ASSERT(_ic->getObj().hasBtreeRoot(), "must has root");

      logicalPageBuffer buffer;
      indexEntryPageAccessor accessor;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);
      PAGE_ID root = INVALID_PAGE_ID;

      rc = _is->getLogicalPageBuffer(_context, _ic->getEntryLpid(),
                                     mode, buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get index entry page buffer:%d", rc);
         goto error;
      }

      rc = accessor.removeBtreeRoot(_context, &buffer, root);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      buffer.fini();
      return rc;
   error:
      goto done;
   }

   INT32 btreeAccessor::traverseDownAndInsert(const ixmKey &key,
                                              const recordID &rid,
                                              BOOLEAN &obstructed)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "must be inited");
      SDB_ASSERT(key.isValid() && rid.isValid(), "can not be invalid");
      SDB_ASSERT(!_bac.isReadonly(), "can not be readonly");
      SDB_ASSERT(_bac.isPathEmpty(), "must be empty");

      obstructed = FALSE;

      rc = _bac.pushRootIntoPath();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to push root node into path:%d", rc);
         goto error;
      }

      do
      {
         btreeNode node = _bac.getEndNodeInPath();

         if (node.isLeaf())
         {  
            if (node.hasFreeSpaceToInsert(key.dataSize()))
            {
               _bac.endToAccessNonPathEndNodes();
               rc = insertWhenPathEndIsLeaf(key, rid, obstructed);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to insert into leaf node[%d,%d]:%d",
                        _ic->getLogicalIndexId(), node.getBuffer()->getLogicalPid(), rc);
                  goto error;
               }
            }
            else
            {
               rc = splitAndInsertWhenPathEndIsLeaf(key, rid, obstructed);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to split and insert:%d", rc);
                  goto error;
               }
            }
            
            goto done;
         }
         else
         {
            btreeItemLocation location;
            rc = node.locateKeyAndRid(key, rid, location);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to locate key and rid:%d", rc);
               goto error;
            }

            if (location.identical)
            {
               _bac.endToAccessNonPathEndNodes();
               if (!node.ensureExclusiveLocking())
               {
                  obstructed = TRUE;
                  goto done;
               }

               rc = node.reactiveRemovedKey(location);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to reactive non-leaf node item:%d", rc);
                  goto error;
               }

               goto done;
            }
            else if (node.hasExternalKey())
            {
               rc = splitNonLeafPathEnd(obstructed);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to split node with ext key:%d", rc);
                  goto error;
               }

               if (obstructed)
               {
                  goto done;
               }

               continue;
            }
            else/// be sure to traverse down from this non-leaf node
            {
               SDB_ASSERT(node.isRoot() || node.hasFreeSpaceToInsert(0), "impossible");
               btreePathFootprint footprint;
               footprint.setPos(location.slotPos);
               footprint.setUpperBound(location.isUpperBound);

               /// WARNING: the key may be raised is not the current key. 
               /// Here we are sure external key can be
               /// saved in current node. And we think most key sizes
               /// are similar. So if it is free to insert current key,
               /// just unlock ancestors.
               UINT32 raisedKeySizeEstimated = key.dataSize() * 1.5f;
               if (MAX_INDEX_KEY_SIZE < raisedKeySizeEstimated)
               {
                  raisedKeySizeEstimated = MAX_INDEX_KEY_SIZE;
               }
               
               if (node.hasFreeSpaceToInsertRaisedKey(raisedKeySizeEstimated))
               {
                  _bac.endToAccessNonPathEndNodes();
               }

               if (INVALID_PAGE_ID != location.child)
               {
                  rc = _bac.pushChildNodeIntoPath(location.child, footprint);
                  if (SDB_OK != rc)
                  {
                     PD_LOG(PDERROR, "failed to push child node into path:%d", rc);
                     goto error;
                  }

                  continue;
               }
               else
               {
                  if (!node.ensureExclusiveLocking())
                  {
                     obstructed = TRUE;
                     goto done;
                  }

                  rc = insertWithRecreatingChild(key, rid, location.slotPos);
                  if (SDB_OK != rc)
                  {
                     PD_LOG(PDERROR, "failed to rebuild child:%d", rc);
                     goto error;
                  }

                  break;
               }
               
            }
         }
      } while (TRUE);

   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeAccessor::splitNonLeafPathEnd(BOOLEAN &obstructed)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(!_bac.isPathEmpty(), "can not be empty");
      SDB_ASSERT(!_bac.isReadonly(), "can not be readonly");

      btreeSplitRaisedKey raisedKey;
      DPS_TRANS_ID transID;
      btreeNode father;
      btreeNode node = _bac.getEndNodeInPath();
      SDB_ASSERT(!node.isLeaf(), "can not be leaf");
      SDB_ASSERT(!node.isRoot(), "can not be root");
      obstructed = FALSE;

      if (!node.ensureExclusiveLocking())
      {
         obstructed = TRUE;
         goto done;
      }

      SDB_ASSERT(_bac.isStillAccessing(node.getDepth() - 1), "must be accessing");
      father = _bac.getNodeInPath(node.getDepth() - 1);
      if (!father.ensureExclusiveLocking())
      {
         obstructed = TRUE;
         goto done;
      }

      rc = father.prepareToWrite();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get father node ready to write:%d", rc);
         goto error;
      }

      SDB_ASSERT(!father.hasExternalKey(), "impossible");
      SDB_ASSERT(father.isRoot() || father.hasFreeSpaceToInsert(0), "impossible");

      transID = node.getTransID();
      rc = node.split(raisedKey);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to split node[%d], rc:%d",
               node.getBuffer()->getLogicalPid(), rc);
         goto error;
      }

      _bac.popEnd();
      rc = insertRaisedKeyRecursively(raisedKey);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to insert raised key recursively:%d", rc);
         goto error;
      }
   
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeAccessor::insertWhenPathEndIsLeaf(const ixmKey &key,
                                                const recordID &rid,
                                                BOOLEAN &obstructed)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(!_bac.isReadonly(), "can not be invalid");
      SDB_ASSERT(!_bac.isPathEmpty(), "can not be empty");

      btreeNode node = _bac.getEndNodeInPath();
      SDB_ASSERT(node.isLeaf(), "must be leaf node");

      obstructed = FALSE;
      if (!node.ensureExclusiveLocking())
      {
         obstructed = TRUE;
         goto done;
      }

      rc = node.leafInsert(key, rid);
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

   INT32 btreeAccessor::splitAndInsertWhenPathEndIsLeaf(const ixmKey &key,
                                                        const recordID &rid,
                                                        BOOLEAN &obstructed)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(!_bac.isPathEmpty(), "can not be invalid");
      SDB_ASSERT(!_bac.isReadonly(), "can not be readonly");
      btreeNode node = _bac.getEndNodeInPath();
      SDB_ASSERT(node.isLeaf(), "must be leaf");

      obstructed = FALSE;
      if (!node.ensureExclusiveLocking())
      {
         obstructed = TRUE;
         goto done;
      }

      if (node.isRoot())
      {
         rc = splitAndInsertWhenPathEndIsRoot(key, rid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to split root and insert:%d", rc);
            goto error;
         }
      }
      else
      {
         btreeSplitRaisedKey raisedKey;
         btreeNode father = _bac.getNodeInPath(node.getDepth() - 1);
         SDB_ASSERT(father.isValid(), "must be valid");
         if (!father.ensureExclusiveLocking())
         {
            obstructed = TRUE;
            goto done;
         }

         SDB_ASSERT(father.isRoot() || father.hasFreeSpaceToInsert(0), "impossible");
         rc = father.prepareToWrite();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get father node ready to write:%d", rc);
            goto error;
         }

         rc = node.splitLeafAndInsert(key, rid, raisedKey);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to split leaf node [%d] and insert:%d",
                   node.getBuffer()->getLogicalPid(), rc);
            goto error;
         }

         _bac.popEnd();
         rc = insertRaisedKeyRecursively(raisedKey);
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

   INT32 btreeAccessor::splitAndInsertWhenPathEndIsRoot(const ixmKey &key,
                                                        const recordID &rid,
                                                        const btreeSplitRaisedKey *raisedKey)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(((key.isValid() && rid.isValid()) ||
                   (NULL != raisedKey && raisedKey->isValid())), "can not be invalid");
      SDB_ASSERT(1 == _bac.getPathSize() && !_bac.isReadonly(), "can not be invalid");

      btreeNode node = _bac.getEndNodeInPath();
      SDB_ASSERT(node.isRoot(), "must be root");
      SDB_ASSERT(node.getLockingMode().isExclusive(), "must be exclusive");
      SDB_ASSERT(!(node.isLeaf() && NULL != raisedKey),
                 "can not insert raised key into leaf");

      btreeNodePageIniter initer;
      PAGE_ID newRoot = INVALID_PAGE_ID;
      logicalPageBuffer newRootBuffer;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);
      btreeSplitRaisedKey newRaisedKey;
      btreeNode newRootNode;
      strictBuffer newRootStrictBuffer;
      //logicalPageBuffer entryBuffer;
      //indexEntryPageAccessor accessor;
      //UINT32 rootUpdatedTimes = 0;
      BOOLEAN wasLeaf = node.isLeaf();

      SDB_ASSERT(_ic->getObj().getBtreeRoot() == node.getBuffer()->getLogicalPid(),
                 "must be same");

      initer._logicalCLID = _context->getMbContext()->getGlobalId().getCLLid();
      initer._indexId = _ic->getLogicalIndexId();
      initer._isLeaf = FALSE;
      initer._isRoot = TRUE;
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

      rc = newRootBuffer.autoGetWritableBodyBuffer(newRootStrictBuffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get writable slice:%d", rc);
         goto error;
      }

/*
      rc = _is->getLogicalPageBuffer(_context, _ic->getEntryLpid(), mode, entryBuffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get entry page buffer:%d", rc);
         goto error;
      }

      rc = entryBuffer.prepareToWrite();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get entry page ready to write:%d", rc);
         goto error;
      }
      */

      if (NULL == raisedKey)
      {
         rc = node.splitLeafAndInsert(key, rid, newRaisedKey);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to split leaf and insert:%d", rc);
            goto error;
         }
      }
      else
      {
         rc = node.splitNonLeafAndInsert(*raisedKey, newRaisedKey);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to split non leaf root and insert:%d", rc);
            goto error;
         }
      }

      /// set right child of new root first, which init it as non-leaf node.
      newRootStrictBuffer.getWritableObjPtr<btreeNodePageHead>(0)->rightChild = newRaisedKey.rightChild;
      newRootNode = btreeNode(&newRootBuffer, 0, _ic);
      rc = newRootNode.insertRaisedKey(newRaisedKey);
      if (SDB_OK != rc)
      {
         PD_LOG(PDSEVERE, "failed to insert raised key into new root:%d", rc);
         ossPanic();
         goto error;
      }

/*
      rc = accessor.updateBtreeRoot(_context, _ic->getIndexID(),
                                    newRoot, &entryBuffer,
                                    &rootUpdatedTimes);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to update btree root in entry page:%d", rc);
         goto error;
      }
*/
   
      rc = node.exchangeWithNewRoot(newRootNode);
      if (SDB_OK != rc)
      {
         PD_LOG(PDSEVERE, "failed to exchange old/new root:%d", rc);
         ossPanic();
         goto error;
      }

      _ic->getObj().updateBtreeRootSplitTimes(node.getSplitedTimes());

      SDB_ASSERT(node.isRoot(), "must be root");
      SDB_ASSERT(!node.isLeaf(), "can not be leaf");
      SDB_ASSERT(!newRootNode.isRoot(), "can not be root");
      if (wasLeaf)
      {
         SDB_ASSERT(newRootNode.isLeaf(), "must be leaf");
      }
   done:
      //entryBuffer.fini();
      newRootBuffer.fini();
      return rc;
   error:
      if (INVALID_PAGE_ID != newRoot)
      {
         _is->releasePage(_context, newRoot);
      }
      goto done;
   }


   INT32 btreeAccessor::createRootIfNotExists()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "must be inited");

      logicalPageBuffer entryBuffer;
      indexEntryPageAccessor accessor;
      btreeNodePageIniter initer;
      PAGE_ID lpid = INVALID_PAGE_ID;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);

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

      initer._logicalCLID = _context->getMbContext()->getGlobalId().getCLLid();
      initer._indexId = _ic->getLogicalIndexId();
      initer._isLeaf = TRUE;
      initer._isRoot = TRUE;
      rc = _is->allocatePages(_context, &initer, 1, &lpid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate new page:%d", rc);
         goto error;
      }

      rc = accessor.updateBtreeRoot(_context,
                                    _ic->getLogicalIndexId(),
                                    lpid, &entryBuffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to update root:%d", rc);
         goto error;
      }

      _ic->getObj().updateBtreeRoot(lpid, 1);

   done:
      entryBuffer.fini();
      return rc;
   error:
      if (INVALID_PAGE_ID != lpid)
      {
         _is->releasePage(_context, lpid);
      }
      goto done;
   }

   INT32 btreeAccessor::insertRaisedKeyRecursively(const btreeSplitRaisedKey &raisedKey)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(!_bac.isPathEmpty(), "can not be invalid");
      SDB_ASSERT(raisedKey.isValid(), "can not be invalid");

      btreeNode fatherNode;
      btreeNode node = _bac.getEndNodeInPath();
      SDB_ASSERT(node.getLockingMode().isExclusive(),
                 "must hold exlusive latch first");
      SDB_ASSERT(node.getBuffer()->isWritable(), "must be writable");
      SDB_ASSERT(!node.hasExternalKey(), "must split node first");
      SDB_ASSERT(!node.isLeaf(), "can not be leaf");
      SDB_ASSERT(node.isRoot() || node.hasFreeSpaceToInsert(0),
                 "one slot should always be reserved");
      
      /// one slot always be reserved
      if (node.hasFreeSpaceToInsertRaisedKey(raisedKey.getKeySize()))
      {
         rc = node.insertRaisedKey(raisedKey);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to insert raised key into node:%d", rc);
            goto error;
         }

         goto done;
      }
      else if (node.isRoot())
      {
         rc = splitAndInsertWhenPathEndIsRoot(ixmKey(), recordID(),
                                              &raisedKey);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to split root and insert raised key:%d", rc);
            goto error;
         }

         goto done;
      }
      else if (_bac.isStillAccessing(node.getDepth() - 1))
      {
         btreeNode tmp = _bac.getNodeInPath(node.getDepth() - 1);
         if (tmp.ensureExclusiveLocking())
         {
            fatherNode = tmp;
         }
      }
      else
      {
         BOOLEAN obstructed = FALSE;
         ossSharedLatchMode mode;
         mode.setExclusive();
         rc = _bac.tryToReaccessNode(node.getDepth() - 1, mode, obstructed);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to reaccess father node:%d", rc);
            rc = SDB_OK; /// do not goto error, just insert ext key in current node.
         }
         else if (!obstructed)
         {
            fatherNode = _bac.getNodeInPath(node.getDepth() - 1);
         }
      }

      if (fatherNode.isValid())
      {
         SDB_ASSERT(fatherNode.getLockingMode().isExclusive(), "impossible");
         btreeSplitRaisedKey newRaisedKey;

         rc = fatherNode.prepareToWrite();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get father node ready to write:%d", rc);
            goto error;
         }

         rc = node.splitNonLeafAndInsert(raisedKey,
                                          newRaisedKey);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to split and insert raised key:%d", rc);
            goto error;
         }

         _bac.popEnd();
         rc = insertRaisedKeyRecursively(newRaisedKey);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to insert raised key recursively:%d", rc);
            goto error;
         }
      }
      else
      {
         /// will create external key page
         rc = node.insertRaisedKeyAsExtKey(raisedKey);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to insert raised key into node:%d", rc);
         goto error;
         }
      }
            
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeAccessor::traverseDownAndRemove(const ixmKey &key,
                                              const recordID &rid,
                                              BOOLEAN &obstructed)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "must be inited");
      SDB_ASSERT(key.isValid() && rid.isValid(), "can not be invalid");
      SDB_ASSERT(!_bac.isReadonly(), "can not be readonly");
      SDB_ASSERT(_bac.isPathEmpty(), "must be empty");

      obstructed = FALSE;

      rc = _bac.pushRootIntoPath();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to push root node into path:%d", rc);
         goto error;
      }

      do
      {
         btreeNode node = _bac.getEndNodeInPath();
         btreeItemLocation location;
         rc = node.locateKeyAndRid(key, rid, location);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to locate key and rid:%d", rc);
            goto error;
         }

         if (node.isLeaf())
         {
            if (!location.identical)
            {
               rc = SDB_VESSEL_IXM_ITEM_NOT_FOUND;
               goto error;
            }

            node = btreeNode();
            rc = removeFromLeafPathEnd(location, obstructed);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to remove item from leaf:%d", rc);
               goto error;
            }
            break;
         }
         else if (location.identical) /// non-leaf
         {
            node = btreeNode();
            rc = removeFromNonleafPathEnd(location, obstructed);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to remove item from non-leaf:%d", rc);
               goto error;
            }
            break;
         }
         else /// non-leaf
         {
            btreePathFootprint  fp;
            fp.setPos(location.slotPos);
            fp.setUpperBound(location.isUpperBound);
            PAGE_ID child = node.getChild(location.slotPos);
            if (INVALID_PAGE_ID == child)
            {
               rc = SDB_VESSEL_IXM_ITEM_NOT_FOUND;
               goto error;
            }

            /// impossible to be empty
            if (node.hasRightChild() || 1 < node.getItemCount())
            {
               _bac.endToAccessNonPathEndNodes();
            }

            rc = _bac.pushChildNodeIntoPath(child, fp);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to push child into path:%d", rc);
               goto error;
            }

            continue;
         }
      } while (TRUE);
      
   done:
      _bac.clearAccessPath();
      return rc;
   error:
      goto done;
   }

   void btreeAccessor::tryToDestroyNodesIfNecessary()
   {
      SDB_ASSERT(!_bac.isPathEmpty(), "can not be empty");

      do
      {
         btreeNode node = _bac.getEndNodeInPath();
         SDB_ASSERT(!(!node.isRoot() && node.isLeaf()), "can not begin from leaf node");
         SDB_ASSERT(node.getLockingMode().isExclusive(), "must be exclusive");
         if (node.isRoot() ||
             node.hasRightChild() ||
             1 < node.getItemCount() ||
             !node.getItemSlot(0).isMarkedDeleted())
         {
            break;
         }

         ///TODO:
      } while (TRUE);
      
   done:
      return;
   }

   INT32 btreeAccessor::removeFromLeafPathEnd(const btreeItemLocation &location,
                                              BOOLEAN &obstructed)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(location.isValid() && location.identical, "can not be invalid");
      btreeNode node = _bac.getEndNodeInPath();
      SDB_ASSERT(node.isLeaf(), "must be leaf");
      obstructed = FALSE;

      if (!node.ensureExclusiveLocking())
      {
         obstructed = TRUE;
         goto done;
      }

      if (node.isRoot() || !node.becameEmptyAfterRemoving(location.slotPos))
      {
         /// leaf node will not be released
         _bac.endToAccessNonPathEndNodes();
         rc = node.leafRemove(location.slotPos);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to remove item in leaf:%d", rc);
            goto error;
         }
      }
      else
      {
         SDB_ASSERT(_bac.isStillAccessing(node.getDepth() - 1), "must be accessing");
         const btreePathFootprint &fp = _bac.getPathNode(node.getDepth() - 1).getChildFootprint();
         SDB_ASSERT(fp.isValid(), "must be valid");
         btreeNode fatherNode = _bac.getNodeInPath(node.getDepth() - 1);
         if (!fatherNode.ensureExclusiveLocking())
         {
            obstructed = TRUE;
            goto done;
         }

         rc = fatherNode.removeChild(fp.getPos());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to remove child in father node:%d", rc);
            goto error;
         }

         _bac.destroyEnd();
         tryToDestroyNodesIfNecessary();
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeAccessor::removeFromNonleafPathEnd(const btreeItemLocation &location,
                                                 BOOLEAN &obstructed)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(location.isValid() && location.identical, "can not be invalid");
      btreeNode node = _bac.getEndNodeInPath();
      SDB_ASSERT(!node.isLeaf(), "can not be leaf");
      obstructed = FALSE;

      if (!node.ensureExclusiveLocking())
      {
         obstructed = TRUE;
         goto done;
      }

      if (node.isRoot() || !node.becameEmptyAfterRemoving(location.slotPos))
      {
         _bac.endToAccessNonPathEndNodes();
         rc = node.nonleafRemove(location.slotPos);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to remove pos[%d] from non-leaf:%d",
                   location.slotPos, rc);
            goto error;
         }
      }
      else
      {
         SDB_ASSERT(_bac.isStillAccessing(node.getDepth() - 1), "must be accessing");
         const btreePathFootprint &fp = _bac.getPathNode(node.getDepth() - 1).getChildFootprint();
         SDB_ASSERT(fp.isValid(), "must be valid");
         btreeNode fatherNode = _bac.getNodeInPath(node.getDepth() - 1);
         if (!fatherNode.ensureExclusiveLocking())
         {
            obstructed = TRUE;
            goto done;
         }

         rc = fatherNode.removeChild(fp.getPos());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to remove child in father node:%d", rc);
            goto error;
         }

         _bac.destroyEnd();
         tryToDestroyNodesIfNecessary();
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeAccessor::insertWithRecreatingChild(const ixmKey &key,
                                                  const recordID &rid,
                                                  RECORD_SLOT_POS pos)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(key.isValid() && rid.isValid(), "can not be invalid");
      SDB_ASSERT(isValidRecordSlotPosition(pos), "can not be invalid");

      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);
      btreeNodePageIniter initer;
      PAGE_ID lpid = INVALID_PAGE_ID;
      logicalPageBuffer buffer;
      btreeNode child;
      btreeNode node = _bac.getEndNodeInPath();
      SDB_ASSERT(!node.isLeaf() && node.getLockingMode().isExclusive(),
                 "can not be invalid");
      SDB_ASSERT((UINT32)pos <= node.getItemCount(), "out of bound");

      initer._logicalCLID = _context->getMbContext()->getGlobalId().getCLLid();
      initer._indexId = _ic->getLogicalIndexId();
      initer._isLeaf = TRUE;
      initer._isRoot = FALSE;

      rc = _is->allocatePage(_context, &initer, lpid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate new page:%d", rc);
         goto error;
      }

      rc = _is->getLogicalPageBuffer(_context, lpid, mode, buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get logical page buffer[%d], rc:%d", lpid, rc);
         goto error;
      }

      child = btreeNode(&buffer, node.getDepth() + 1, _ic);
      rc = child.leafInsert(key, rid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to key and rid into leaf:%d", rc);
         goto error;
      }

      rc = node.resetRemovedChild(pos, lpid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to reset child:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      if (INVALID_PAGE_ID != lpid)
      {
         _is->releasePage(_context, lpid);
      }
      goto done;
   }

   INT32 btreeAccessor::releaseWholeTree()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_ic->getObj().hasBtreeRoot(), "must be has btree root");
      ossPoolVector<PAGE_ID> batch;
      batch.reserve(BATCH_RELEASE_COUNT);

      rc = _bac.pushRootIntoPath();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to push root into path:%d", rc);
         goto error;
      }

      rc = releaseTreeNodeRecursively(batch);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to release tree nodes:%d", rc);
         goto error;
      }

      SDB_ASSERT(batch.empty(), "must be empty");
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeAccessor::releaseTreeNodeRecursively(ossPoolVector<PAGE_ID> &batch)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!_bac.isPathEmpty(), "can not be empty");
      btreeNode node = _bac.getEndNodeInPath();
      if (node.isLeaf())
      {
         batch.push_back(node.getBuffer()->getLogicalPid());
         _bac.popEnd();
      }
      else
      {
         UINT32 itemCount = node.getItemCount();
         for (UINT32 i = 0; i <= itemCount; ++i)
         {
            PAGE_ID child = node.getChild(i);
            if (INVALID_PAGE_ID != child)
            {
               btreePathFootprint fp;
               fp.setPos(i);
               fp.setUpperBound(i == itemCount);
               rc = _bac.pushChildNodeIntoPath(child, fp);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to push child[%d] into path:%d", child, rc);
                  goto error;
               }

               rc = releaseTreeNodeRecursively(batch);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to release tree node:%d", rc);
                  goto error;
               }
            }
         }

         if (node.hasExternalKey())
         {
            batch.push_back(node.getExternalKeyPage());
         }

         batch.push_back(node.getBuffer()->getLogicalPid());
         _bac.popEnd();
      }

      if (BATCH_RELEASE_COUNT <= batch.size() ||
          (_bac.isPathEmpty() && !batch.empty()))
      {
         rc = _is->releasePages(_context, batch.size(), batch.data());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to release pages:%d", rc);
            goto error;
         }

         batch.clear();
      }

   done:
      return rc;
   error:
      goto done;
   }
} // namespace vessel

} // namespace engine
