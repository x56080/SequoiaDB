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

   Source File Name = btreeIndexIterator.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/btreeIndexIterator.h"
#include "vessel/indexUtils.h"
#include "ixmKey.hpp"
#include "vessel/btreeScanEntryParser.h"
#include "vessel/instanceEnv.h"

namespace engine
{
namespace vessel
{
   btreeIndexIterator::btreeIndexIterator()
   {}

   btreeIndexIterator::~btreeIndexIterator()
   {
      _bac.fini();
   }

   void btreeIndexIterator::close()
   {
      _bac.fini();
      _context = NULL;
      _item.fini();
      _pos = INVALID_RECORD_SLOT_ID;
      _builder.reset();
      return;
   }

   INT32 btreeIndexIterator::open(requestContext *context,
                                  indexContext *ic)
   {
      INT32 rc = SDB_OK;
      logicalPageSpace *lps = NULL;
      close();
      if (OSS_UNLIKELY(NULL == context ||
                       DMS_INVALID_LOGICCSID == context->getLogicalCSID() ||
                       DMS_INVALID_LOGICCLID == context->getLogicalCLID() ||
                       NULL == ic ||
                       !ic->isValid() ||
                       INDEX_TYPE_BTREE != ic->getIndexType()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = context->getEnv()->dms.getLogicalPageSpace(context->getSpaceID(),
                                                      SPACE_TYPE_IDX, &lps);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get lps[%d], rc:%d", context->getSpaceID(), rc);
         goto error;
      }

      _context = NULL;
      _bac.init(ic, context, static_cast<indexSpace *>(lps));
   done:
      return rc;
   error:
      close();
      goto done;
   }

   BOOLEAN btreeIndexIterator::isReadyToRead()const
   {
      return _item.isValid();
   }

   INT32 btreeIndexIterator::seekFromCurrentPosition(const bson::BSONObj &prevKey,
                                                     INT32 fieldCountToCmpInPrev,
                                                     const VEC_ELE_CMP &matchEles,
                                                     const inclusiveVec &matchInclusive,
                                                     const options &o)
   {
      INT32 rc = SDB_OK;
      bson::BSONObj keyObj;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

   done:
      return rc;
   error:
      close();
      goto done;
   }

   INT32 btreeIndexIterator::contains(const ixmKey &key, recordID &rid)
   {
      INT32 rc = SDB_OK;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeIndexIterator::seekKey(const ixmKey &key,
                                     const options &o)
   {
      INT32 rc = SDB_OK;

      
   done:
      return rc;
   error:
      close();
      goto done;
   }

   INT32 btreeIndexIterator::seek(const bson::BSONObj &prevKey,
                                  INT32 fieldCountToCmpInPrev,
                                  const VEC_ELE_CMP &matchEles,
                                  const inclusiveVec &matchInclusive,
                                  const options &o)
   {
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = locateKeyInTree(prevKey, fieldCountToCmpInPrev,
                           matchEles, matchInclusive, o);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to locate key in btree:%d", rc);
         goto error;
      }

      if (!hasLocation())
      {
         goto done;
      }

      SDB_ASSERT(!_bac.isPathEmpty(), "can not be empty");
      if (_bac.getEndNodeInPath().getItemSlot(_pos).isMarkedDeleted())
      {
         rc = next(o.isForward());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get next item:%d", rc);
            goto error;
         }
      }
      else
      {
         rc = cacheCurrentItem();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to cache current item:%d", rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      close();
      goto done;
   }

   INT32 btreeIndexIterator::seekEntry(const slice &entry,
                                       const options &o)
   {
      return SDB_OK;
   }

   void btreeIndexIterator::resetLocation()
   {
      _bac.clearAccessPath();
      _item.fini();
      _pos = INVALID_RECORD_SLOT_ID;
      _originalKeyBuffer.reset();
      return;
   }

   INT32 btreeIndexIterator::next(BOOLEAN forward)
   {
      INT32 rc = SDB_OK;
      BOOLEAN obstructed = FALSE;

      if (!hasLocation())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      SDB_ASSERT(_item.isValid(), "can not be invalid");
      SDB_ASSERT(!_bac.isPathEmpty(), "can not be empty");
      SDB_ASSERT(_bac.isStillAccessing(_bac.getPathSize() - 1), "must be accessing");
     
      if (_bac.getEndNodeInPath().isLeaf())
      {
         rc = nextAtLeaf(forward, obstructed);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get next when end node is leaf:%d", rc);
            goto error;
         }
      }
      else
      {
         rc = nextAtNonLeaf(forward, obstructed);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get next when end node is non-leaf:%d", rc);
            goto error;
         }
      }

      if (obstructed)
      {
         rc = relocateAndMoveToNext(forward);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to relocate and move:%d", rc);
            goto error;
         }
      }

      SDB_ASSERT(hasLocation(), "must be located");
      /// always cache item before done.
      rc = cacheCurrentItem();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to cache current item:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      close();
      goto done;
   }

   void btreeIndexIterator::resetPositionOfCurrentNode(RECORD_SLOT_ID pos)
   {
      SDB_ASSERT(INVALID_RECORD_SLOT_ID != pos, "can not be invalid");
      SDB_ASSERT(!_bac.isPathEmpty(), "can not be invalid");
      SDB_ASSERT(_bac.isStillAccessing(_bac.getPathSize() - 1), "must be accessing");
      btreeNode node = _bac.getEndNodeInPath();
      SDB_ASSERT(_pos < node.getItemCount(), "out of bound");
      _pos = pos;
      _item.fini();
      _originalKeyBuffer.reset();
      return;
   }

   void btreeIndexIterator::clearPositionOfCurrentNode()
   {
      _pos = INVALID_RECORD_SLOT_ID;
      _item.fini();
      return;
   }

   INT32 btreeIndexIterator::nextAtNonLeaf(BOOLEAN forward,
                                           BOOLEAN &obstructed)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(hasLocation(), "can not be invalid");
      SDB_ASSERT(_bac.isReadonly(), "must be readonly");
      obstructed = FALSE;

      INT32 ancestorDepth = -1;
      BOOLEAN footPrintIsFaithful = FALSE;

      do
      {
         btreeNode node = _bac.getEndNodeInPath();
         SDB_ASSERT(!node.isLeaf(), "can not be leaf");
         SDB_ASSERT(_pos < node.getItemCount(), "out of bound");

         INT32 direction = forward ? 1 : -1;
         INT32 adjust = forward ? 0 : 1;
         INT32 pos = (INT32)_pos + direction;
         PAGE_ID childLpid = node.getChild((RECORD_SLOT_ID)(pos + adjust));

         if (INVALID_PAGE_ID != childLpid)
         {
            RECORD_SLOT_ID posInChild = INVALID_RECORD_SLOT_ID;
            btreeNode childNode;
            btreeItemLocation location;
            location.child = childLpid;
            location.identical = FALSE;
            location.slotPos = (RECORD_SLOT_ID)(pos + adjust);
            location.isUpperBound = (location.slotPos == node.getItemCount());

            _bac.endToAccessNonPathEndNodes();
            rc = _bac.pushChildNodeIntoPath(childLpid, location, &childNode);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to push child node[%d] into path:%d",
                      childLpid, rc);
               goto error;
            }

            SDB_ASSERT(0 < childNode.getItemCount(), "impossible");
            posInChild = forward ? 0 : (childNode.getItemCount() - 1);
            resetPositionOfCurrentNode(posInChild);

            SDB_ASSERT(!childNode.isLeaf(), "impossible to be leaf");
            continue;
         }
         
         if (0 <= pos && pos < (INT32)node.getItemCount())
         {
            /// still in the current node, just ensure it is not removed.
            resetPositionOfCurrentNode((RECORD_SLOT_ID)pos);
            if (isCurrentItemMarkedDeleted())
            {
               /// do not call 'nextAtNonLeaf' recursively here.
               /// too much removed item may exist.
               continue;
            }
            goto done;
         }

         break;
      } while (TRUE);

      /// hit the end of this node, go back to ancestor.
      rc = prepareToGoBackToAncestors(forward, obstructed,
                                      ancestorDepth,
                                      footPrintIsFaithful);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare to go back:%d", rc);
         goto error;
      }
      else if (obstructed || ancestorDepth < 0)
      {
         resetLocation();
         goto done;
      }
      else
      {
         RECORD_SLOT_ID ancestorPos = INVALID_RECORD_SLOT_ID;
         if (footPrintIsFaithful)
         {
            ancestorPos = _bac.getPathNode(ancestorDepth).getChildLocation().slotPos;
         }
         else
         {
            rc = findPosInAncestor(ancestorDepth, ancestorPos);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to find pos in ancestor:%d", rc);
               goto error;
            }
         }

         clearPositionOfCurrentNode();
         _bac.popEnds(_bac.getPathSize() - ancestorDepth - 1);
         rc = goBackToAncestor(ancestorPos, forward, obstructed);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to goback to ancestor node:%d", rc);
            goto error;
         }
      }
      
   done:
      return rc;
   error:
      resetLocation();
      goto done;
   }

   INT32 btreeIndexIterator::nextAtLeaf(BOOLEAN forward, BOOLEAN &obstructed)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(hasLocation(), "can not be invalid");
      btreeNode node = _bac.getEndNodeInPath();
      SDB_ASSERT(node.isLeaf(), "must be leaf");
      obstructed = FALSE;

      if (forward && ((UINT32)(_pos + 1) < node.getItemCount()))
      {
         resetPositionOfCurrentNode(_pos + 1);
      }
      else if (!forward && 0 < _pos)
      {
         resetPositionOfCurrentNode(_pos - 1);
      }
      else
      {
         BOOLEAN obstructed = FALSE;
         INT32 ancestorDepth = -1;
         BOOLEAN footPrintIsFaithful = FALSE;
         RECORD_SLOT_ID ancestorPos = INVALID_RECORD_SLOT_ID;

         rc = prepareToGoBackToAncestors(forward, obstructed,
                                         ancestorDepth,
                                         footPrintIsFaithful);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to prepare to got back ancestors:%d", rc);
            goto error;
         }
         else if (obstructed || ancestorDepth < 0)
         {
            resetLocation();
            goto done;
         }
         else
         {
            /// we can go back to ancestor now. find the begin position.
            if (footPrintIsFaithful)
            {
               ancestorPos = _bac.getPathNode(ancestorDepth).getChildLocation().slotPos;
            }
            else
            {
               rc = findPosInAncestor(ancestorDepth, ancestorPos);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to find pos in ancestor:%d", rc);
                  goto error;
               }
            }

            clearPositionOfCurrentNode();
            _bac.popEnds(_bac.getPathSize() - ancestorDepth - 1);
            rc = goBackToAncestor(ancestorPos, forward, obstructed);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to goback to ancestor node:%d", rc);
               goto error;
            }
         }
      }
   done:
      return rc;
   error:
      resetLocation();
      goto done;
   }

   INT32 btreeIndexIterator::goBackToAncestor(RECORD_SLOT_ID pos,
                                              BOOLEAN forward,
                                              BOOLEAN &obstructed)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_RECORD_SLOT_ID != pos, "can not be invalid");
      SDB_ASSERT(!_bac.isPathEmpty(), "can not be empty");
      SDB_ASSERT(_bac.isStillAccessing(_bac.getPathSize() - 1), "must be accessing");

      obstructed = FALSE;
   
      btreeItemSlot slot;
      const btreeAccessPathNode &pn = _bac.getPathNode(_bac.getPathSize() - 1);
      btreeNode node = _bac.getEndNodeInPath();
      SDB_ASSERT(!node.isLeaf(), "can not be leaf");
      SDB_ASSERT(0 < node.getItemCount(), "can not be empty");
      SDB_ASSERT(pos <= node.getItemCount(), "out of bound");
      SDB_ASSERT(!(pos == node.getItemCount() && forward), "should skip this node");
      SDB_ASSERT(!(0 == pos && !forward), "should skip this node");

      if (node.getItemCount() == pos && 
          node.getRightChild() != pn.getChildLocation().child)
      {
         SDB_ASSERT(FALSE, "invalid pos");
         PD_LOG(PDERROR, "right child does not match footprint in path");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      else if (pos < node.getItemCount() &&
               node.getItemSlot(pos).data.nlf.leftChild !=
               pn.getChildLocation().child)
      {
         SDB_ASSERT(FALSE, "invalid pos");
         PD_LOG(PDERROR, "left child does not match footprint in path");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (!forward)
      {
         resetPositionOfCurrentNode(pos - 1);
      }
      else
      {
         resetPositionOfCurrentNode(pos);
      }

      if (isCurrentItemMarkedDeleted())
      {
         rc = nextAtNonLeaf(forward, obstructed);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get next in non-leaf node:%d", rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeIndexIterator::prepareToGoBackToAncestors(BOOLEAN forward,
                                                        BOOLEAN &obstructed,
                                                        INT32 &ancestorDepth,
                                                        BOOLEAN &footPrintIsFaithful)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(hasLocation(), "can not be invalid");
      SDB_ASSERT(0 < _bac.getPathSize(), "no ancestors to accessing");
      SDB_ASSERT(_bac.isStillAccessing(_bac.getPathSize() - 1), "end node must be accessing");

      obstructed = FALSE;
      ancestorDepth = -1;
      footPrintIsFaithful = FALSE;

      BOOLEAN isWholePathLocking = TRUE;
      INT32 depth = -1;

      if (_bac.getEndNodeInPath().isRoot())
      {
         /// root has no ancestors
         goto done;
      }

      depth = (INT32)_bac.getPathSize() - 2;

      /// fast skip some nodes
      while (0 <= depth)
      {
         const btreeAccessPathNode &pn = _bac.getPathNode((UINT32)depth);
         if (!pn.isAccessing())
         {
            isWholePathLocking = FALSE;
         }

         SDB_ASSERT(pn.getChildLocation().isValid(), "footprint missed");
         if ((forward && pn.getChildLocation().isUpperBound) ||
              (!forward && (0 == pn.getChildLocation().slotPos)))
         {
            --depth;
            continue;
         }
         else
         {
            break;
         }
      }

      if (depth < 0)
      {
         /// hit the end
         goto done;
      }

      if (!_bac.isStillAccessing((UINT32)depth))
      {
         ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_SHARED);
         rc = _bac.tryToReaccessNode((UINT32)depth, mode, obstructed);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to reaccess node with depth[%d], rc:%d",
                   depth, rc);
            goto error;
         }

         if (obstructed)
         {
            goto done;
         }
      }

      ancestorDepth = depth;
      footPrintIsFaithful = isWholePathLocking;
   done:
      return rc;
   error:
      goto done;
   }

   UINT64 btreeIndexIterator::getLSN()const
   {
      SDB_ASSERT(isReadyToRead(), "can not be invalid");
      return _bac.getPathNode(_bac.getPathSize() - 1).getPageBuffer()->
             getRuntimeBuffer().getPageHead()->lsn;
   }
   slice btreeIndexIterator::getKey()const
   {
      SDB_ASSERT(isReadyToRead(), "can not be invalid");
      return slice();
   }
   DPS_TRANS_ID btreeIndexIterator::getTransID()const
   {
      return DPS_TRANS_ID();
   }
   recordID btreeIndexIterator::getRid()const
   {
      return recordID();
   }

   INT32 btreeIndexIterator::pushCurrentEntryToBatch(indexScanEntryBatch &batch)const
   {
      INT32 rc = SDB_OK;
      btreeScanEntryParser parser;
      DPS_TRANS_ID transID;
      const btreeNodePageHead *head = NULL;
      slice ks;

      if (!isReadyToRead())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      SDB_ASSERT(_item.isValid(), "can not be invalid");

      head = _bac.getPathNode(_bac.getPathSize() - 1).getPageBuffer()->getReadableBodySlice().
             getReadableObjPtr<btreeNodePageHead>(0);
      transID.setNodeID(head->transNode);
      transID.setSN(head->transSN);

      ks = getOriginalKeySlice();

      parser.init(getCurrentIndexRid(), _item.getRid(), transID, ks);

      rc = batch.addFragmentsOfOneEntry({parser.getFixSizedFields(), ks});
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to add entry into batch:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   BOOLEAN btreeIndexIterator::equalToCurrentKey(const ixmKey &key)const
   {
      return FALSE;
   }

   INT32 btreeIndexIterator::cacheCurrentItem()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(hasLocation(), "can not be invalid");
      SDB_ASSERT(!_bac.isPathEmpty(), "can not be empty");

      if (!_item.isValid())
      {
         btreeNode node = _bac.getEndNodeInPath();
         rc = node.getItem(_pos, _item);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get index item:%d", rc);
            goto error;
         }

         if (_item.getSlot().isKeyCompressed())
         {
            _item.exportOriginalKey(_originalKeyBuffer);
         }
      }
      else
      {
         SDB_ASSERT(_item.getSlotPos() == _pos, "must be same");
      }
   done:
      return rc;
   error:
      resetLocation();
      goto done;
   }

   void btreeIndexIterator::pause()
   {
      SDB_ASSERT(isOpen(), "can not be closed");
      resetLocation();
      return;
   }

   INT32 btreeIndexIterator::locateKeyInTree(const bson::BSONObj &prevKey,
                                             INT32 fieldCountToCmpInPrev,
                                             const VEC_ELE_CMP &matchEles,
                                             const inclusiveVec &matchInclusive,
                                             const options &o)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_bac.isValid(), "can not be invalid");
      SDB_ASSERT(_bac.isReadonly(), "must be readonly");
      resetLocation();

      orderingWrapper ow = _bac.getIndexContext()->getObj().getPattern().getOrdering();
      btreeNode node;
      RECORD_SLOT_ID pos = INVALID_RECORD_SLOT_ID;
      UINT32 locatedDepth = 0;

      if (!_bac.getIndexContext()->getObj().hasBtreeRoot())
      {
         goto done;
      }

      rc = _bac.pushRootIntoPath(&node);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to push root node into path:%d", rc);
         goto error;
      }

      do
      {
         btreeItemLocation locd; /// location of current depth
         rc = node.seek(prevKey, fieldCountToCmpInPrev,
                        !o.isInclusive(), matchEles,
                        matchInclusive,
                        o.getDirection(), locd,
                        &_builder);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to seek key in btree node:%d", rc);
            goto error;
         }

         SDB_ASSERT(locd.isValid(), "can not be invalid");
         if (!locd.isUpperBound)
         {
            locatedDepth = node.getDepth();
            pos = locd.slotPos;
         }

         if (INVALID_PAGE_ID == locd.child)
         {
            SDB_ASSERT(node.isLeaf(), "must be leaf node");
            break;
         }
         else
         {
            SDB_ASSERT(!node.isLeaf(), "can not be leaf node");
            if (!locd.isUpperBound)
            {
               _bac.endToAccessNonPathEndNodes();
            }

            rc = _bac.pushChildNodeIntoPath(locd.child, locd);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to push child into path:%d", rc);
               goto error;
            }

            node = _bac.getEndNodeInPath();
            continue;
         }
      } while (TRUE);

      if (INVALID_RECORD_SLOT_ID == pos)
      {
         resetLocation();
         goto done;
      }

      /// return to the last located node
      SDB_ASSERT(locatedDepth < _bac.getPathSize(), "impossible");
      _bac.popEnds(_bac.getPathSize() - locatedDepth - 1);
      _pos = pos;
      
   done:
      return rc;
   error:
      resetLocation();
      goto done;
   }

   INT32 btreeIndexIterator::relocateAndMoveToNext(BOOLEAN forward)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(hasLocation(), "can not be invalid");
      bson::StackBufBuilder builder;
      recordID rid;
      btreeItemLocation location;

      if (!_item.isValid())
      {
         rc = cacheCurrentItem();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to cache current item:%d", rc);
            goto error;
         }
      }

      rid = _item.getRid();
      _item.exportOriginalKey(builder);

      rc = relocateKeyAndRidInTree(ixmKey(builder.buf()), rid, location);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to relocated key and rid:%d", rc);
         goto error;
      }

      if (!hasLocation())
      {
         goto done;
      }

      SDB_ASSERT(location.slotPos == _pos, "must be same");

      if (location.identical)
      {
         /// same item was located, just get next
         rc = next(forward);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get next pos:%d", rc);
            goto error;
         }
      }
      else if (forward)
      {
         /// the postion is upper bound of target, just make sure it is not removed.
         if (isCurrentItemMarkedDeleted())
         {
            rc = next(forward);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to get next pos:%d", rc);
               goto error;
            }
         }
      }
      else /// not identical and scan backward
      {
         /// need to go back to pre item.
         rc = next(forward);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get next pos:%d", rc);
            goto error;
         }
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   BOOLEAN btreeIndexIterator::isCurrentItemMarkedDeleted()
   {
      SDB_ASSERT(hasLocation(), "can not be invalid");
      SDB_ASSERT(!_bac.isPathEmpty(), "can not be invalid");
      BOOLEAN md = FALSE;
      
      if (_item.isValid())
      {
         SDB_ASSERT(_item.getSlotPos() == _pos, "must be same");
         md = _item.getSlot().isMarkedDeleted();
      }
      else
      {
         btreeNode node = _bac.getEndNodeInPath();
         SDB_ASSERT(_pos < node.getItemCount(), "out of bound");
         md = node.getItemSlot(_pos).isMarkedDeleted();
      }

      return md;
   }

   INT32 btreeIndexIterator::findPosInAncestor(UINT32 ancestorDepth,
                                               RECORD_SLOT_ID &pos)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(hasLocation(), "can not be invalid");
      SDB_ASSERT(_bac.isStillAccessing(ancestorDepth), "must be accessing");
      SDB_ASSERT((ancestorDepth + 1) < _bac.getPathSize(), "impossible");

      bson::StackBufBuilder builder;
      recordID rid;
      btreeItemLocation location;
      rc = cacheCurrentItem();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to cache current item:%d", rc);
         goto error;
      }

      rid = _item.getRid();
      _item.exportOriginalKey(builder);
      rc = _bac.getNodeInPath(ancestorDepth).locateKeyAndRid(ixmKey(builder.buf()), rid, location);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to locate key and rid in ancestor:%d", rc);
         goto error;
      }

      /// pos may be upper bound
      pos = location.slotPos;
   done:
      return rc;
   error:
      goto done;
   }

   recordID btreeIndexIterator::getCurrentIndexRid()const
   {
      SDB_ASSERT(hasLocation(), "can not be invalid");
      PAGE_ID lpid = _bac.getPathNode(_bac.getPathSize() - 1).getPageBuffer()->getLogicalPid();
      return recordID(lpid, _pos);
   }

   slice btreeIndexIterator::getOriginalKeySlice()const
   {
      SDB_ASSERT(hasLocation(), "can not be invalid");
      SDB_ASSERT(_item.isValid(), "can not be invalid");
      slice key;
      if (_item.getSlot().isKeyCompressed())
      {
         key.reset(_originalKeyBuffer.len(), _originalKeyBuffer.buf());
      }
      else
      {
         key.reset(_item.getSavedKeyDataSize(), _item.getSavingKeyData());
      }
      return key;
   }

   INT32 btreeIndexIterator::relocateKeyAndRidInTree(const ixmKey &key,
                                                     const recordID &rid,
                                                     btreeItemLocation &location)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_bac.isValid(), "can not be invalid");
      SDB_ASSERT(_bac.isReadonly(), "must be readonly");
      SDB_ASSERT(key.isValid(), "can not be invalid");
      SDB_ASSERT(rid.isValid(), "can not be invalid");
      RECORD_SLOT_ID pos = INVALID_RECORD_SLOT_ID;
      UINT32 locatedDepth = 0;

      resetLocation();
      location = btreeItemLocation();

      rc = _bac.pushRootIntoPath();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to push root node into path:%d", rc);
         goto error;
      }

      do
      {
         btreeItemLocation locd; /// location of current depth
         btreeNode node = _bac.getEndNodeInPath();
         rc = node.locateKeyAndRid(key, rid, locd);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to locate key an rid:%d", rc);
            goto error;
         }

         SDB_ASSERT(locd.isValid(), "can not be invalid");
         
         if (!locd.isUpperBound)
         {
            locatedDepth = node.getDepth();
            pos = locd.slotPos;
         }

         if (locd.identical)
         {
            _pos = locd.slotPos;
            location = locd;
            goto done;
         }

         if (INVALID_PAGE_ID == locd.child)
         {
            SDB_ASSERT(node.isLeaf(), "must be leaf node");
            break;
         }
         else
         {
            SDB_ASSERT(!node.isLeaf(), "can not be leaf node");
            if (!locd.isUpperBound)
            {
               _bac.endToAccessNonPathEndNodes();
            }

            rc = _bac.pushChildNodeIntoPath(locd.child, locd);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to push child into path:%d", rc);
               goto error;
            }

            node = _bac.getEndNodeInPath();
            continue;
         }
      } while (TRUE);

      if (INVALID_RECORD_SLOT_ID == pos)
      {
         resetLocation();
         goto done;
      }

      location.child = _bac.getPathNode(locatedDepth).getChildLocation().child;
      location.identical = FALSE;
      location.isUpperBound = FALSE;
      location.slotPos = pos;
      /// return to the last located node
      SDB_ASSERT(locatedDepth < _bac.getPathSize(), "impossible");
      _bac.popEnds(_bac.getPathSize() - locatedDepth - 1);
      _pos = pos;
      
   done:
      return rc;
   error:
      resetLocation();
      goto done;
   }
} // namespace vessel

} // namespace engine
