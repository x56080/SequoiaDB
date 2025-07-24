/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = btreeWriter.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/btreeWriter.h"
#include "vessel/requestContext.h"
#include "vessel/indexObject.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"
#include "vessel/instanceEnv.h"
#include "vessel/indexSpace.h"
#include "vessel/btreeNodePage.h"
#include "vessel/btreeNodePageIniter.h"
#include "vessel/btreeEntryPageAccessor.h"


namespace engine
{
namespace vessel
{
   INT32 btreeWriter::init(requestContext *context,
                           indexSpace *is,
                           indexObject *obj,
                           spacePteAccessCtx *ac)
   {
      INT32 rc = SDB_OK;
      reset();

      if (OSS_UNLIKELY(nullptr == context ||
                       nullptr == is ||
                       nullptr == obj ||
                       nullptr == ac))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _bac.init(context, is, obj, ac);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init btree context:%d", rc);
         goto error;
      }

      _ac = ac;
   done:
      return rc;
   error:
      goto done;
   }

   void btreeWriter::reset()
   {
      _ac = nullptr;
      _bac.reset();
      return;
   }

   INT32 btreeWriter::insert(const btreeKeyStringEntry &entry,
                             DPS_LSN_OFFSET lsn,
                             const DPS_TRANS_ID &transID)
   {
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(!entry.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(MAX_INDEX_KEY_SIZE < entry.getRawDataSize()))
      {
         PD_LOG(PDERROR, "key size too large");
         rc = SDB_IXM_KEY_TOO_LARGE;
         goto error;
      }
      else if (OSS_UNLIKELY(!_bac.getIndexObject()->getBtreeEntryAddr().isValid()))
      {
         PD_LOG(PDERROR, "btree entry page not created yet");
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else
      {
         if (!_bac.hasBtreeRoot())
         {
            rc = _createBtreeRoot(TRUE);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to create btree root:%d", rc);
               goto error;
            }
         }

         rc = _insert(entry, lsn, transID);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to insert key and rid:%d", rc);
            goto error;
         }

         _bac.resetPath();
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeWriter::remove(const btreeKeyStringEntry &entry,
                             DPS_LSN_OFFSET lsn,
                             const DPS_TRANS_ID &transID)
   {
      INT32 rc = SDB_OK;
      
      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(!entry.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (!_bac.hasBtreeRoot())
      {
         PD_LOG(PDERROR, "btree has no root yet");
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = _remove(entry);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to remove key and rid:%d", rc);
         goto error;
      }

      _bac.resetPath();

   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeWriter::truncate(BOOLEAN removeEntryPage)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");

      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      if (_bac.hasBtreeRoot())
      {
         rc = _bac.pushRootIntoPath();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to push root into path:%d", rc);
            goto error;
         }

         rc = _bac.pushRootIntoPath();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to push root into path:%d", rc);
            goto error;
         }

         if (!_bac.getEndNodeInPath().isLeaf())
         {
            rc = _destroyChildNodesRecursively();
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to destroy child nodes:%d", rc);
               goto error;
            }
         }
      }

      if (_bac.hasBtreeRoot() && !removeEntryPage)
      {
         rc = _removeBtreeRoot();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to release btree:%d", rc);
            goto error;
         }
      }  
      else if (removeEntryPage &&
               _bac.getIndexObject()->getBtreeEntryAddr().isValid())
      {
         indexSpace *is = _bac.getIndexSpace();
         if (_bac.hasBtreeRoot())
         {
            rc = is->removePage(_bac.getReqCtx(), _ac, _bac.getBtreeRoot());
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to remove btree root:%d", rc);
               goto error;
            }
         }
         _bac.resetBtreeRoot(INVALID_PAGE_ID);
         
         rc = is->removePage(_bac.getReqCtx(), _ac,
                             _bac.getIndexObject()->getBtreeEntryAddr().pid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to remove btree entry page:%d", rc);
            goto error;
         }
         _bac.getIndexObject()->resetBtreeEntryAddr();
      }
      _bac.statsTruncateTree();
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeWriter::_removeBtreeRoot()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "must be inited");
      SDB_ASSERT(_bac.hasBtreeRoot(), "must has root");
      PAGE_ID root = _bac.getBtreeRoot();
      indexSpace *is = _bac.getIndexSpace();
      rc = is->removePage(_bac.getReqCtx(), _ac, root);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to remove btree root:%d", rc);
         goto error;
      }

      _bac.resetBtreeRoot(INVALID_PAGE_ID);
      rc = refreshEntryPage();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to refresh btree entry page:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeWriter::_insert(const btreeKeyStringEntry &entry,
                              DPS_LSN_OFFSET lsn,
                              const DPS_TRANS_ID &transID)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "must be inited");
      SDB_ASSERT(entry.isValid(), "can not be invalid");
      SDB_ASSERT(_bac.isPathEmpty(), "must be empty");
      btreeSplitRaisedKey raisedKey;

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
            if (node.hasFreeSpaceToInsert(entry.getRawDataSize()))
            {
               rc = node.insert(entry, transID);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to insert entry into leaf node:%d");
                  goto error;
               }

               if (_bac.getIndexObject()->getProperties().isCompressionEnabled() &&
                   node.betterToActiveCompression())
               {
                  BOOLEAN compressed = FALSE;
                  rc = node.recompress(compressed);
                  if (SDB_OK != rc)
                  {
                     PD_LOG(PDERROR, "failed to compress btree node:%d", rc);
                     goto error;
                  }
               }
            }
            else
            {
               rc = node.splitAndInsert(entry, transID, raisedKey);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to split leaf node and inser entry:%d", rc);
                  goto error;
               }
            }

            node.commit(lsn);
            break;
         }
         else/// non leaf
         {
            PAGE_ID child = INVALID_PAGE_ID;
            btreeNodeSeekResult res;
            rc = node.locateEntry(entry, res);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to locate key and rid:%d", rc);
               goto error;
            }

            if (res.isIdentical())
            {
               rc = node.reactiveRemovedKey(res.getPos(), transID);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to reactive non-leaf node item:%d", rc);
                  goto error;
               }

               node.commit(lsn);
               break;
            }
            else if (!res.hasChild())
            {
               /// child may be removed when removing key
               rc = _recreateChildAsLeaf(node, res.getPos(), child);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to recrate child node:%d", rc);
                  goto error;
               }
            }
            else
            {
               child = res.getChild();
            }

            /// not else
            {
               btreePathFootprint footprint;
               footprint.setPos(res.getPos());
               footprint.setUpperBound(res.isUpperBound());

               rc = _bac.pushChildNodeIntoPath(child, footprint);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to push child node into path:%d", rc);
                  goto error;
               }

               continue;
            }
         }
      } while (TRUE);

      if (raisedKey.isValid())
      {
         rc = _insertRaisedKey(raisedKey);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to insert raised key:%d", rc);
            goto error;
         }
      }

   done:
      return rc;
   error:
      goto done;
   }


   INT32 btreeWriter::_createBtreeRoot(BOOLEAN isLeaf)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "must be inited");

      logicalPageBufferPte entryBuffer;
      btreeNodePageIniter initer;
      indexSpace *is = _bac.getIndexSpace();
      const indexObject *obj = _bac.getIndexObject();
      btreeEntryPageAccessor accessor(obj->getLogicalID());
      SDB_ASSERT(!_bac.hasBtreeRoot(), "do not recreate root node");
      PAGE_ID root = INVALID_PAGE_ID;
      btreeEntryAddr entryAddr = obj->getBtreeEntryAddr();
      SDB_ASSERT(entryAddr.isValid(), "can not be invalid");

      rc = is->getPageBuffer(_bac.getReqCtx(), _ac,
                             entryAddr.pid, entryBuffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get btree entry buffer:%d", rc);
         goto error;
      }

      rc = is->makePrivateBuffer(_bac.getReqCtx(), _ac, entryBuffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to make entry buffer writable:%d", rc);
         goto error;
      }
                                           
      initer._indexId = _bac.getIndexObject()->getLogicalID();
      initer._isLeaf = isLeaf;
      initer._isRoot = TRUE;

      rc = is->allocatePtePage(_bac.getReqCtx(), _ac, &initer, root);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate root page:%d", rc);
         goto error;
      }
      _bac.statsAllocateNode();
      if (isLeaf)
      {
         _bac.statsAddLeafNode(FALSE);
      }
      else
      {
         _bac.statsAddNonLeafNode();
      }
      _bac.statsCreateNewRoot();
      /// no need to update transfer tick here.
      rc = accessor.refill(_bac.getReqCtx(), root, _bac.getTransferTick(),
                           _bac.getStats(), &entryBuffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to update root:%d", rc);
         goto error;
      }

      _bac.resetBtreeRoot(root);

      PD_LOG(PDDEBUG, "btree root[%d] created", root);
   done:
      entryBuffer.fini();
      return rc;
   error:
      if (INVALID_PAGE_ID != root)
      {
         is->removePage(_bac.getReqCtx(), _ac, root);
      }
      goto done;
   }

   INT32 btreeWriter::_insertRaisedKey(const btreeSplitRaisedKey &raisedKey)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(!_bac.isPathEmpty(), "can not be invalid");
      SDB_ASSERT(raisedKey.isValid(), "can not be invalid");

      btreeSplitRaisedKey newRaisedKey;
      btreeSplitRaisedKey keyToInsert;
      keyToInsert.shallowCopy(raisedKey);

      do
      {
         btreeNode node;
         _bac.popEnd();

         if (!_bac.isPathEmpty())
         {
            node = _bac.getEndNodeInPath();
            if (node.hasFreeSpaceToInsert(keyToInsert.entry.getRawDataSize()))
            {
               rc = node.insertRaisedKey(keyToInsert);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to insert raised key:%d", rc);
                  goto error;
               }

               break;
            }
            else
            {
               btreeSplitRaisedKey tmp;
               rc = node.splitAndInsert(keyToInsert, tmp);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to split node and insert raised key:%d", rc);
                  goto error;
               }

               newRaisedKey = std::move(tmp);
               keyToInsert.shallowCopy(newRaisedKey);
               continue;
            }
         }
         else /// new root
         {
            rc = _createNewRootToInsert(keyToInsert);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to create new root node to insert raised key:%d", rc);
               goto error;
            }
            break;
         }
      } while (TRUE);
      
            
   done:
      return rc;
   error:
      goto done;
   }
   

   INT32 btreeWriter::_remove(const btreeKeyStringEntry &entry)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "must be inited");
      SDB_ASSERT(entry.isValid(), "can not be invalid");

      rc = _bac.pushRootIntoPath();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to push root node into path:%d", rc);
         goto error;
      }

      do
      {
         btreeNode node = _bac.getEndNodeInPath();
         btreeNodeSeekResult res;
         rc = node.locateEntry(entry, res);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to locate key and rid:%d", rc);
            goto error;
         }

         if (res.isIdentical())
         {
            rc = _removeEntryFromPathEnd(res.getPos());
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to remove entry from leaf node:%d", rc);
               goto error;
            }

            break;
         }
         else if (node.isLeaf())
         {
            rc = SDB_VESSEL_IXM_ITEM_NOT_FOUND;
            goto error;
         }
         else
         {
            btreePathFootprint  fp;
            fp.setPos(res.getPos());
            fp.setUpperBound(res.isUpperBound());
            PAGE_ID child = node.getChild(res.getPos());
            if (INVALID_PAGE_ID == child)
            {
               rc = SDB_VESSEL_IXM_ITEM_NOT_FOUND;
               goto error;
            }

            rc = _bac.pushChildNodeIntoPath(child, fp);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to push child into path:%d", rc);
               goto error;
            }

            continue;
         }///if (res.isIdentical())
      } while (TRUE);
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeWriter::_destroyNodesIfNecessary()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!_bac.isPathEmpty(), "can not be invalid");

      while (!_bac.isPathEmpty())
      {
         btreeNode node = _bac.getEndNodeInPath();
         BOOLEAN isLeaf = node.isLeaf();
         BOOLEAN compressed = node.hasPrefixes();
         UINT32 markedDeletedSize = node.getItemSlot(0).isMarkedDeleted()
                                        ? node.getItemSlot(0).data.key.size
                                        : 0;
         if (!node.isNeedToBeDestroyed())
         {
            break;
         }
         else if (1 < _bac.getPathSize())
         {
            btreePathFootprint fp = _bac.getEndNodeFootprint();
            rc = _bac.destroyPathEnd();
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to destroy path end:%d", rc);
               goto error;
            }

            node = _bac.getEndNodeInPath();
            rc = node.removeChild(fp.getPos());
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to remove child from node:%d", rc);
               goto error;
            }
         }
         else
         {
            SDB_ASSERT(node.isRoot(), "must be root");
            rc = _bac.destroyPathEnd();
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to destroy root node:%d", rc);
               goto error;
            }

            rc = _removeBtreeRoot();
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to remove btree root:%d", rc);
               goto error;
            }

            break;
         }
         if (isLeaf && compressed)
         {
            _bac.statsRemoveLeafNode(compressed);
         }
         else
         {
            _bac.statsReleaseMarkedDeletedIndexSpace(markedDeletedSize);
            _bac.statsRemoveNonLeafNode();
         }
      }
      
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeWriter::_recreateChildAsLeaf(btreeNode &father,
                                           RECORD_SLOT_POS pos,
                                           PAGE_ID &child)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(isValidRecordSlotPosition(pos), "can not be invalid");
      SDB_ASSERT(father.isValid(), "can not be invalid");
      SDB_ASSERT(!father.isLeaf(), "impossible");
      SDB_ASSERT((UINT32)pos <= father.getItemCount(), "out of bound");

      btreeNodePageIniter initer;
      indexSpace *is = _bac.getIndexSpace();
      const indexObject *obj = _bac.getIndexObject();

      child = INVALID_PAGE_ID;

      initer._indexId = obj->getLogicalID();
      initer._isLeaf = TRUE;
      initer._isRoot = FALSE;

      rc = is->allocatePtePage(_bac.getReqCtx(), _ac, &initer, child);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate new page:%d", rc);
         goto error;
      }

      rc = father.refillChild(pos, child, TRUE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to reset child:%d", rc);
         goto error;
      }
      _bac.statsRefillChildNode();

   done:
      return rc;
   error:
      if (INVALID_PAGE_ID != child)
      {
         is->removePage(_bac.getReqCtx(), _ac, child);
         child = INVALID_PAGE_ID;
      }
      goto done;
   }

   INT32 btreeWriter::_createNewRootToInsert(const btreeSplitRaisedKey &raisedEntry)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(raisedEntry.isValid(), "can not be invalid");
      SDB_ASSERT(_bac.isPathEmpty(), "must be empty");
      btreeNode root;

      /// we will abort whole context if get any error.
      /// so it does not matter that update nodes unorderly.

      _bac.resetBtreeRoot(INVALID_PAGE_ID);

      rc = _createBtreeRoot(FALSE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create new root:%d", rc);
         goto error;
      }

      rc = _bac.pushRootIntoPath(&root);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to push root into path:%d", rc);
         goto error;
      }

      rc = root.insertRaisedKey(raisedEntry);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to insert raised entry into root:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeWriter::_removeEntryFromPathEnd(RECORD_SLOT_POS pos)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValidRecordSlotPosition(pos), "can not be invalid");
      SDB_ASSERT(!_bac.isPathEmpty(), "can not be invalid");
      btreeNode node = _bac.getEndNodeInPath();

      rc = node.remove(pos);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to remove entry from node:%d", rc);
         goto error;
      }

      if (node.isNeedToBeDestroyed())
      {
         _destroyNodesIfNecessary();
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeWriter::_destroyChildNodesRecursively()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!_bac.isPathEmpty(), "clear path first");
      btreeNode node = _bac.getEndNodeInPath();
      SDB_ASSERT(!node.isLeaf(), "can not be leaf");

      indexSpace *is = _bac.getIndexSpace();
      UINT32 count = node.getItemCount();
      ossPoolVector<PAGE_ID> vec;
      vec.reserve(count);

      for (UINT32 i = 0; i < count; ++i)
      {
         btreeItemSlot slot = node.getItemSlot(i);
         PAGE_ID child = slot.data.nlf.leftChild;
         if (INVALID_PAGE_ID == child)
         {
            continue;
         }
         else if (!slot.isRaisedFromLeaf())
         {
            btreePathFootprint fp;
            fp.setPos(i);
            rc = _bac.pushChildNodeIntoPath(child, fp);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to push child into path:%d", rc);
               goto error;
            }

            rc = _destroyChildNodesRecursively();
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to destroy sub nodes:%d", rc);
               goto error;
            }

            _bac.popEnd();
            vec.push_back(child);
         }
         else
         {
            vec.push_back(child);
         }
      }

      if (node.hasRightChild())
      {
         if (!node.isRightChildLeaf())
         {
            btreePathFootprint fp;
            fp.setPos(node.getItemCount());
            fp.setUpperBound(TRUE);
            rc = _bac.pushChildNodeIntoPath(node.getRightChild(), fp);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to push child into path:%d", rc);
               goto error;
            }

            rc = _destroyChildNodesRecursively();
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to destroy sub nodes:%d", rc);
               goto error;
            }

            _bac.popEnd();
         }

         vec.push_back(node.getRightChild());
      }
      
      if (!vec.empty())
      {
         rc = is->removePages(_bac.getReqCtx(), _ac, vec.size(), vec.data());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to remove pages:%d", rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeWriter::refreshEntryPage()
   {
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else
      {
         const indexObject *obj = _bac.getIndexObject();
         btreeEntryPageAccessor accessor(obj->getLogicalID());
         logicalPageBufferPte buffer;
         PAGE_ID pid = obj->getBtreeEntryAddr().pid;
         if (INVALID_PAGE_ID == pid)
         {
            PD_LOG(PDERROR, "invalid btree entry address");
            rc = SDB_VESSEL_RESOURCES_NOT_INIT;
            goto error;
         }

         rc = _bac.getIndexSpace()->getPageBuffer(_bac.getReqCtx(), _ac, pid, buffer);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get entry page[%d] buffer:%d",
                   pid, rc);
            goto error;
         }

         rc = _bac.getIndexSpace()->makePrivateBuffer(_bac.getReqCtx(), _ac, buffer);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to make buffer writable:%d", rc);
            goto error;
         }

         rc = accessor.refill(_bac.getReqCtx(), _bac.getBtreeRoot(),
                              _bac.getTransferTick() + 1,
                              _bac.getStats(), &buffer);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to refill entry apge:%d", rc);
            goto error;
         }

         _bac.incTransferTick();
         PD_LOG(PDDEBUG, "refreshed btree[%s] stats:%s",
                obj->getProperties().getName().c_str(),
                _bac.getStats().toBSON().toPoolString().c_str());
      }

   done:
      return rc;
   error:
      goto done;
   }
} // namespace vessel
} // namespace engine
