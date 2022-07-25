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
#include "vessel/indexObject.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"
#include "vessel/instanceEnv.h"
#include "vessel/indexSpace.h"
#include "vessel/btreeNodePage.h"
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
                             indexSpace *is,
                             indexObject *obj)
   {
      INT32 rc = SDB_OK;
      indexSpaceAccessCtx ctx;

      fini();

      if (OSS_UNLIKELY(nullptr == context ||
                       !context->isClPropertiesSet() ||
                       nullptr == is ||
                       !is->isOpen() ||
                       nullptr == obj ||
                       !obj->isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _is = is;
      _obj = obj;

      rc = _is->openAccessCtx(context, ctx);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open accessing context:%d", rc);
         goto error;
      }

      rc = _bac.init(FALSE, _obj, std::move(ctx));
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init btree context:%d", rc);
         goto error;
      }
      
   done:
      return rc;
   error:
      fini();
      goto done;
   }

   void btreeAccessor::fini()
   {
      _is = nullptr;
      _obj = nullptr;
      _bac.fini();
      return;
   }

   INT32 btreeAccessor::insert(const ixmKey &key,
                               const recordID &rid)
   {
      INT32 rc = SDB_OK;

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

      if (!_obj->hasBtreeEntryAddr())
      {

      }

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

      if (!_obj->hasBtreeRoot())
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

      if (!_obj->hasBtreeRoot())
      {
         PD_LOG(PDDEBUG, "has no btree root");
         goto done;
      }

      rc = releaseWholeTreeExceptRoot();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to remove btree root in entry page:%d", rc);
         goto error;
      }

      rc = removeBtreeRoot();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to release btree:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeAccessor::removeBtreeRoot()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "must be inited");
      SDB_ASSERT(_obj->hasBtreeRoot(), "must has root");

      logicalPageBuffer buffer;
      indexEntryPageAccessor accessor;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);
      PAGE_ID root = INVALID_PAGE_ID;
      BOOLEAN blocked = FALSE;

      rc = _is->blockCheckpoint(_context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to block checkpoint:%d", rc);
         goto error;
      }
      blocked = TRUE;

      rc = _is->getLogicalPageBuffer(_context, _obj->getEntryLpid(),
                                     mode, buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get index entry page buffer:%d", rc);
         goto error;
      }

      rc = accessor.removeBtreeRoot(_context, &buffer, root);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to remove btree root in entry page:%d", rc);
         goto error;
      }

      SDB_ASSERT(root == _obj->getBtreeRoot(), "must be same");
      _is->releasePage(_context, root);
      _obj->removeBtreeRoot();
   done:
      buffer.fini();
      if (blocked)
      {
         _context->unblockCheckpoint();
      }
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
                         _obj->getLogicalID(), 
                         node.getBuffer()->getLogicalPid(), rc);
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

      SDB_ASSERT(_obj->getBtreeRoot() == node.getBuffer()->getLogicalPid(),
                 "must be same");

      initer._logicalCLID = _context->getLogicalClId();
      initer._indexId = _obj->getLogicalID();
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
      newRootNode = btreeNode(&newRootBuffer, 0, _obj);
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

      _obj->updateBtreeRootSplitTimes(node.getSplitedTimes());

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

      if (_obj->hasBtreeRoot())
      {
         goto done;
      }

      rc = _is->getLogicalPageBuffer(_context,
                                     _obj->getEntryLpid(),
                                     mode, entryBuffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get entry page[%d] buffer:%d",
                _obj->getEntryLpid(), rc);
         goto error;
      }

      /// check again under entry page locking.
      if (_obj->hasBtreeRoot())
      {
         goto done;
      }

      initer._logicalCLID = _context->getLogicalClId();
      initer._indexId = _obj->getLogicalID();
      initer._isLeaf = TRUE;
      initer._isRoot = TRUE;
      rc = _is->allocatePages(_context, &initer, 1, &lpid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate new page:%d", rc);
         goto error;
      }

      rc = accessor.updateBtreeRoot(_context,
                                    _obj->getLogicalID(),
                                    lpid, &entryBuffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to update root:%d", rc);
         goto error;
      }

      _obj->updateBtreeRoot(lpid, 1);

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

      initer._logicalCLID = _context->getLogicalClId();
      initer._indexId = _obj->getLogicalID();
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

      child = btreeNode(&buffer, node.getDepth() + 1, _obj);
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

   INT32 btreeAccessor::releaseWholeTreeExceptRoot()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_obj->hasBtreeRoot(), "must be has btree root");
      SDB_ASSERT(_bac.isPathEmpty(), "must be empty");

      _bac.setReadonly(FALSE);
      _bac.setPessimistic(TRUE);

      rc = _bac.pushRootIntoPath();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to push root into path:%d", rc);
         goto error;
      }

      if (_bac.getEndNodeInPath().isLeaf())
      {
         goto done;
      }

      rc = releaseNonLeafNodeRecursively();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to release tree nodes:%d", rc);
         goto error;
      }

      _bac.clearAccessPath();
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeAccessor::releaseNonLeafNodeRecursively()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!_bac.isPathEmpty(), "can not be empty");
      btreeNode node = _bac.getEndNodeInPath();
      SDB_ASSERT(!node.isLeaf(), "can not be leaf");
      RECORD_SLOT_POS begin = 0;
      
      do
      {
         RECORD_SLOT_POS pos = INVALID_RECORD_SLOT_POS;
         rc = seekChildToReleaseFirst(begin, pos);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to seek child to be released:%d", rc);
            goto error;
         }

         if (INVALID_RECORD_SLOT_POS == pos)
         {
            break;
         }
         else
         {
            PAGE_ID child = node.getChild(pos);
            btreePathFootprint fp;
            fp.setPos(pos);
            fp.setUpperBound((UINT32)pos == node.getItemCount());
            rc = _bac.pushChildNodeIntoPath(child, fp);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to push child[%d] into path:%d", child, rc);
               goto error;
            }

            rc = releaseNonLeafNodeRecursively();
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to release node[%d], rc:%d", child, rc);
               goto error;
            }

            _bac.popEnd();
            begin = pos + 1;
            continue;
         }
      } while(TRUE);

      rc = atomicReleaseNonLeafPathEnd();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to atomic release node:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeAccessor::seekChildToReleaseFirst(RECORD_SLOT_POS begin,
                                                RECORD_SLOT_POS &pos)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_RECORD_SLOT_POS != begin, "can not be invalid");
      SDB_ASSERT(!_bac.isPathEmpty(), "can not be empty");
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_SHARED);
      pos = INVALID_RECORD_SLOT_POS;

      btreeNode node = _bac.getEndNodeInPath();
      SDB_ASSERT(!node.isLeaf(), "can not be leaf");
      UINT32 itemCount = node.getItemCount();
      for (UINT32 i = begin; i <= itemCount; ++i)
      {
         BOOLEAN childIsLeaf = FALSE;
         logicalPageBuffer buffer;
         btreeNode childNode;
         PAGE_ID child = node.getChild(i);
         if (INVALID_PAGE_ID == child)
         {
            continue;
         }

         rc = _is->getLogicalPageBuffer(_context, child, mode, buffer);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get page[%d] buffer:%d", child, rc);
            goto error;
         }

         childNode = btreeNode(&buffer, _bac.getPathSize() + 1, _obj);
         childIsLeaf = childNode.isLeaf();
         buffer.fini();
         if (!childIsLeaf)
         {
            pos = i;
            break;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeAccessor::atomicReleaseNonLeafPathEnd()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!_bac.isPathEmpty(), "can not be empty");

      BOOLEAN blocked = FALSE;
      btreeNode node = _bac.getEndNodeInPath();
      SDB_ASSERT(!node.isLeaf(), "can not be leaf");

      ossPoolVector<PAGE_ID> nodes;
      node.dumpAllSubNodes(nodes);

      rc = _is->blockCheckpoint(_context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to block checkpoint:%d", rc);
         goto error;
      }
      blocked = TRUE;

      rc = node.resetAsEmptyNode();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to clear node[%d]:%d",
                node.getBuffer()->getLogicalPid(), rc);
         goto error;
      }

      _is->releasePages(_context, nodes.size(), nodes.data());
   done:
      if (blocked)
      {
         _context->unblockCheckpoint();
      }
      return rc;
   error:
      goto done;
   }

   INT32 btreeAccessor::_initBtreeEntryAndRoot()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(!_obj->hasBtreeEntryAddr(), "do not reinit");

      
   done:
      return rc;
   error:
      goto done;
   }
} // namespace vessel

} // namespace engine
