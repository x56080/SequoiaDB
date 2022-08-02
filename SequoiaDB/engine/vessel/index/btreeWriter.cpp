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

   Source File Name = btreeWriter.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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
                           indexObject *obj)
   {
      INT32 rc = SDB_OK;
      indexSpaceAccessCtx ac;
      reset();

      if (OSS_UNLIKELY(nullptr == context ||
                       nullptr == is ||
                       nullptr == obj))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = is->openAccessCtx(context, ac);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open accessing context:%d", rc);
         goto error;
      }

      rc = _bac.init(FALSE, obj, std::move(ac));
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init btree context:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   void btreeWriter::reset()
   {
      _bac.reset();
      return;
   }

   INT32 btreeWriter::insert(const btreeKeyStringEntry &entry)
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
      else if (OSS_UNLIKELY(!_bac.getIndexObject()->hasBtreeEntryAddr()))
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
               PD_LOG(PDERROR, "failed to create btree root:%d");
               goto error;
            }
         }

         _bac.resetPath();
         rc = _insert(entry);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to insert key and rid:%d", rc);
            goto error;
         }
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeWriter::remove(const btreeKeyStringEntry &entry)
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

      _bac.resetPath();
      rc = _remove(entry);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to remove key and rid:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeWriter::truncate()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");

      if (!_bac.hasBtreeRoot())
      {
         PD_LOG(PDDEBUG, "has no btree root");
         goto done;
      }

      _bac.resetPath();
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

      rc = _removeBtreeRoot();
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

   INT32 btreeWriter::_removeBtreeRoot()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "must be inited");
      SDB_ASSERT(_bac.hasBtreeRoot(), "must has root");

      logicalPageBuffer buffer;
      indexSpaceAccessCtx &ac = _bac.getSpaceCtx();
      indexSpace *is = ac.getIndexSpace();
      indexObject *obj = _bac.getIndexObject();
      SDB_ASSERT(obj->hasBtreeEntryAddr(), "can not be invalid");
      btreeEntryPageAccessor accessor(obj->getLogicalID());
      SDB_ASSERT(_bac.hasBtreeRoot(), "can not be invalid");
      PAGE_ID root = _bac.getBtreeRoot();

      rc = is->getLogicalPageBuffer(ac, obj->getBtreeEntryAddr(), TRUE, buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get btree entry buffer:%d", rc);
         goto error;
      }

      rc = is->makePrivateBuffer(ac, buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to make entry buffer writable:%d", rc);
         goto error;
      }

      rc = accessor.resetBtreeRoot(ac.getReqCtx(), INVALID_PAGE_ID, &buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to remove btree root in entry page:%d", rc);
         goto error;
      }

      is->removePage(ac, root);
      _bac.resetBtreeRoot(INVALID_PAGE_ID);
   done:
      buffer.fini();
      return rc;
   error:
      goto done;
   }

   INT32 btreeWriter::_insert(const btreeKeyStringEntry &entry)
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
               rc = node.leafInsert(entry);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to insert entry into leaf node:%d");
                  goto error;
               }
            }
            else
            {
               rc = node.splitLeafAndInsert(entry, raisedKey);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to split leaf node and inser entry:%d", rc);
                  goto error;
               }
            }

            break;
         }
         else/// non leaf
         {
            btreeNodeSeekResult res;
            rc = node.locateEntry(entry, res);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to locate key and rid:%d", rc);
               goto error;
            }

            if (res.isIdentical())
            {
               rc = node.reactiveRemovedKey(res.slotPos);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to reactive non-leaf node item:%d", rc);
                  goto error;
               }
               break;
            }
            else if (!res.hasChild())
            {
               /// child may be remove when removing key
               PAGE_ID child = INVALID_PAGE_ID;
               rc = _recreateChildAsLeaf(node, res.slotPos, child);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to recrate child node:%d", rc);
                  goto error;
               }
               res.child = child;
            }

            /// not else if, we may recreate it if not exists
            {
               btreePathFootprint footprint;
               footprint.setPos(res.slotPos);
               footprint.setUpperBound(res.isUpperBound);

               rc = _bac.pushChildNodeIntoPath(res.child, footprint);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to push chil node into path:%d", rc);
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

      logicalPageBuffer entryBuffer;
      btreeNodePageIniter initer;
      indexSpaceAccessCtx &ac = _bac.getSpaceCtx();
      indexSpace *is = ac.getIndexSpace();
      indexObject *obj = _bac.getIndexObject();
      SDB_ASSERT(obj->hasBtreeEntryAddr(), "can not be invalid");
      btreeEntryPageAccessor accessor(obj->getLogicalID());
      SDB_ASSERT(!_bac.hasBtreeRoot(), "do not recreate root node");
      PAGE_ID root = INVALID_PAGE_ID;

      rc = is->getLogicalPageBuffer(ac, obj->getBtreeEntryAddr(), TRUE, entryBuffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get btree entry buffer:%d", rc);
         goto error;
      }

      rc = is->makePrivateBuffer(ac, entryBuffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to make entry buffer writable:%d", rc);
         goto error;
      }
                                           
      initer._indexId = _bac.getIndexObject()->getLogicalID();
      initer._isLeaf = isLeaf;
      initer._isRoot = TRUE;

      rc = is->allocate(ac, &initer, root);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate root page:%d", rc);
         goto error;
      }

      rc = accessor.resetBtreeRoot(ac.getReqCtx(), root, &entryBuffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to update root:%d", rc);
         goto error;
      }

      _bac.resetBtreeRoot(root);
   done:
      entryBuffer.fini();
      return rc;
   error:
      if (INVALID_PAGE_ID != root)
      {
         is->removePage(ac, root);
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
               rc = node.splitNonLeafAndInsert(keyToInsert, tmp);
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
            rc = _removeEntryFromPathEnd(res.slotPos);
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
            fp.setPos(res.slotPos);
            fp.setUpperBound(res.isUpperBound);
            PAGE_ID child = node.getChild(res.slotPos);
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
         if (!node.betterToBeDestroyed())
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
      indexSpaceAccessCtx &ac = _bac.getSpaceCtx();
      indexSpace *is = ac.getIndexSpace();
      indexObject *obj = _bac.getIndexObject();

      child = INVALID_PAGE_ID;

      initer._indexId = obj->getLogicalID();
      initer._isLeaf = TRUE;
      initer._isRoot = FALSE;

      rc = is->allocate(ac, &initer, child);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate new page:%d", rc);
         goto error;
      }

      rc = father.resetRemovedChild(pos, child, TRUE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to reset child:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      if (INVALID_PAGE_ID != child)
      {
         is->removePage(ac, child);
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

      rc = node.removeEntry(pos);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to remove entry from node:%d", rc);
         goto error;
      }

      if (node.betterToBeDestroyed())
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

      indexSpaceAccessCtx &ac = _bac.getSpaceCtx();
      indexSpace *is = ac.getIndexSpace();
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
         rc = is->removePages(ac, vec.size(), vec.data());
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
} // namespace vessel
} // namespace engine
