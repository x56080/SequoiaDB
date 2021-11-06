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

      _context = context;
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

   INT32 btreeIndexIterator::fastNext(const bson::BSONObj &prevKey,
                                      INT32 fieldCountToCmpInPrev,
                                      const VEC_ELE_CMP &matchEles,
                                      const inclusiveVec &matchInclusive,
                                      const options &o)
   {
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(!isReadyToRead()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = advanceInSubTree(prevKey, fieldCountToCmpInPrev,
                            matchEles, matchInclusive, o);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to advance in tree:%d", rc);
         goto error;
      }

      if (!hasLocation())
      {
         goto done;
      }
      else if (isCurrentItemMarkedDeleted())
      {
         rc = next(o.isForward());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get next:%d", rc);
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

   INT32 btreeIndexIterator::contains(const ixmKey &key, recordID &rid)
   {
      INT32 rc = SDB_OK;
      indexIterator::options o(TRUE, TRUE);
      rid = recordID();

      rc = seekKey(key, o);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to seek key:%d", rc);
         goto error;
      }

      if (!hasLocation())
      {
         goto done;
      }

      if (equalToCurrentKey(key))
      {
         rid = _item.getRid();
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeIndexIterator::seekKey(const ixmKey &key,
                                     const options &o)
   {
      INT32 rc = SDB_OK;
      recordID rid;
      btreeItemLocation location;

      if (OSS_UNLIKELY(!key.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      if (o.isForward())
      {
         rid = o.isInclusive() ?
               recordID::createMinRid() :
               recordID::createMaxRid();
      }
      else
      {
         rid = o.isInclusive() ?
               recordID::createMaxRid() :
               recordID::createMinRid();
      }

      rc = relocateKeyAndRidInTree(key, rid, location);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to locate key and rid:%d", rc);
         goto error;
      }

      if (!hasLocation())
      {
         goto done;
      }

      if (o.isForward())
      {
         if (isCurrentItemMarkedDeleted())
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
      }
      else
      {
         rc = next(o.isForward());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get next item:%d", rc);
            goto error;
         }
      }
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
      if (isCurrentItemMarkedDeleted())
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

   INT32 btreeIndexIterator::moveToTheNextOfEntry(const slice &entry,
                                                  BOOLEAN forward)
   {
      INT32 rc = SDB_OK;
      btreeScanEntryParser parser;
      ixmKey key;
      btreeItemLocation location;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = parser.parse(entry);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to parse scan entry:%d", rc);
         goto error;
      }

      key.assign(parser.getKeySlice().data());
      rc = relocateKeyAndRidInTree(key, parser.getRid(), location);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to locate key and rid in tree:%d", rc);
         goto error;
      }

      if (!hasLocation())
      {
         goto done;
      }

      if (location.identical || !forward || isCurrentItemMarkedDeleted())
      {
         rc = next(forward);
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
            PD_LOG(PDERROR, "failed to cache current item", rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      close();
      goto done;
   }

   void btreeIndexIterator::resetLocation()
   {
      _bac.clearAccessPath();
      _item.fini();
      _pos = INVALID_RECORD_SLOT_ID;
      return;
   }

   INT32 btreeIndexIterator::next(BOOLEAN forward)
   {
      INT32 rc = SDB_OK;
      if (!hasLocation())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      SDB_ASSERT(!_bac.isPathEmpty(), "can not be empty");
      SDB_ASSERT(_bac.isStillAccessing(_bac.getPathSize() - 1), "must be accessing");
     
      do
      {
         SDB_ASSERT(hasLocation(), "can not be invalid");
         BOOLEAN obstructed = FALSE;
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
            /// once we relocate position, cache item will be reset.
            /// we must copy key here.
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

            if (location.identical || !forward)
            {
               /// must move to next
               continue;
            }
            else if (isCurrentItemMarkedDeleted())
            {
               /// forward and not identical
               /// current postion is upper bound of target,
               /// only get next when it is removed.
               continue;
            }
            else
            {
               /// forward and not identical and not removed
               break;
            }
         }
         else if (!hasLocation())
         {
            goto done;
         }
         else if (!isCurrentItemMarkedDeleted())
         {
            break;
         }
         else
         {
            continue;
         }
         
      } while (TRUE);

      

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
      SDB_ASSERT(pos < node.getItemCount(), "out of bound");
      _pos = pos;
      _item.fini();
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

      btreeNode node = _bac.getEndNodeInPath();
      SDB_ASSERT(!node.isLeaf(), "can not be leaf");
      SDB_ASSERT(_pos < node.getItemCount(), "out of bound");

      INT32 direction = forward ? 1 : -1;
      INT32 adjust = forward ? 0 : 1;
      INT32 pos = (INT32)_pos + direction;
      PAGE_ID childLpid = node.getChild((RECORD_SLOT_ID)(pos + adjust));

      if (INVALID_PAGE_ID != childLpid)
      {
         btreeItemLocation location;
         location.child = childLpid;
         location.identical = FALSE;
         location.slotPos = (RECORD_SLOT_ID)(pos + adjust);
         location.isUpperBound = (location.slotPos == node.getItemCount());
         rc = traverseDownToBottom(forward, location);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to traverse down:%d", rc);
            goto error;
         }
         
      }
      else if (0 <= pos && pos < (INT32)node.getItemCount())
      {
         /// move to next in current node
         resetPositionOfCurrentNode((RECORD_SLOT_ID)pos);
      }
      else
      {
         rc = goBackToAncestor(forward, obstructed);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to go back to ancestor:%d", rc);
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
      else if (node.isRoot())
      {
         resetLocation();
         goto done;
      }
      else
      {
         rc = goBackToAncestor(forward, obstructed);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to go back to ancestor from leaf:%d", rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      resetLocation();
      goto done;
   }

   INT32 btreeIndexIterator::goBackToAncestor(BOOLEAN forward,
                                              BOOLEAN &obstructed)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!_bac.isPathEmpty(), "can not be empty");
      SDB_ASSERT(_bac.isStillAccessing(_bac.getPathSize() - 1), "must be accessing");

      obstructed = FALSE;

      INT32 ancestorDepth = -1;
      BOOLEAN footPrintIsFaithful = FALSE;
      RECORD_SLOT_ID ancestorPos = INVALID_RECORD_SLOT_ID;

      rc = prepareToGoBackToAncestors(forward, obstructed,
                                      ancestorDepth,
                                      footPrintIsFaithful);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare to go back:%d", rc);
         goto error;
      }
      else if (obstructed)
      {
         goto done;
      }
      else if (ancestorDepth < 0)
      {
         resetLocation();
         goto done;
      }
      else if (footPrintIsFaithful)
      {
         ancestorPos = _bac.getPathNode(ancestorDepth).getChildFootprint().getPos();
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

      _bac.popEnds(_bac.getPathSize() - ancestorDepth - 1);
      resetPositionOfCurrentNode(forward ? ancestorPos : (ancestorPos - 1));
      SDB_ASSERT(_pos < _bac.getEndNodeInPath().getItemCount(), "out of bound");
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
      SDB_ASSERT(1 < _bac.getPathSize(), "no ancestors to accessing");
      SDB_ASSERT(_bac.isStillAccessing(_bac.getPathSize() - 1), "end node must be accessing");

      obstructed = FALSE;
      ancestorDepth = -1;
      footPrintIsFaithful = FALSE;

      BOOLEAN isWholePathLocking = TRUE;
      INT32 depth = -1;

      depth = (INT32)_bac.getPathSize() - 2;

      /// fast skip some nodes
      while (0 <= depth)
      {
         const btreeAccessPathNode &pn = _bac.getPathNode((UINT32)depth);
         if (!pn.isAccessing())
         {
            isWholePathLocking = FALSE;
         }

         SDB_ASSERT(pn.getChildFootprint().isValid(), "footprint missed");
         if ((forward && pn.getChildFootprint().isUpperBound()) ||
              (!forward && (0 == pn.getChildFootprint().getPos())))
         {
            /// skip this node
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
   bson::BSONObj btreeIndexIterator::getKeyObj(bson::BufBuilder *builder)const
   {
      SDB_ASSERT(isReadyToRead(), "can not be invalid");
      SDB_ASSERT(!_item.getSlot().isKeyCompressed(), "TODO");
      ixmKey key(_item.getSavingKeyData());
      return key.toBson(builder);
   }
   DPS_TRANS_ID btreeIndexIterator::getTransID()const
   {
      SDB_ASSERT(isReadyToRead(), "can not be invalid");
      const btreeNodePageHead *head = NULL;
      slice nodeSlice = _bac.getPathNode(_bac.getPathSize() - 1).getPageBuffer()->getReadableBodySlice();
      head = nodeSlice.getReadableObjPtr<btreeNodePageHead>(0);         
      return DPS_TRANS_ID(head->transSN, head->transNode);
   }
   recordID btreeIndexIterator::getRid()const
   {
      SDB_ASSERT(isReadyToRead(), "can not be invalid");
      return _item.getRid();
   }

   INT32 btreeIndexIterator::pushCurrentEntryToBatch(indexScanEntryBatch &batch)const
   {
      INT32 rc = SDB_OK;
      btreeScanEntryParser parser;
      DPS_TRANS_ID transID;
      const btreeNodePageHead *head = NULL;
      bson::StackBufBuilder tmp;
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

      if (!_item.getSlot().isKeyCompressed())
      {
         ks.reset(_item.getSavedKeyDataSize(), _item.getSavingKeyData());
      }
      else
      {
         _item.exportOriginalKey(tmp);
         ks.reset(tmp.len(), tmp.buf());
      }

      parser.init(getCurrentIndexRid(), _item.getRid(), transID, ks);

      rc = batch.addFragmentsOfOneEntry({parser.getFixSizedFields(),
                                         parser.getKeySlice()});
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
      SDB_ASSERT(_item.isValid(), "can not be invalid");
      SDB_ASSERT(key.isValid(), "can not be invalid");
      return _item.woEqual(key);
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

      if (!_bac.getIndexContext()->getObj().hasBtreeRoot())
      {
         goto done;
      }

      rc = _bac.pushRootIntoPath();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to push root node into path:%d", rc);
         goto error;
      }

      SDB_ASSERT(_bac.getEndNodeInPath().isRoot(), "must be root");

      rc = locateKeyInSubTree(prevKey,
                              fieldCountToCmpInPrev,
                              matchEles,
                              matchInclusive, o);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to locate key in sub tree:%d", rc);
         goto error;
      }

      if (!hasLocation())
      {
         _bac.clearAccessPath();
      }
      
   done:
      return rc;
   error:
      resetLocation();
      goto done;
   }

   INT32 btreeIndexIterator::locateKeyInSubTree(const bson::BSONObj &prevKey,
                                                INT32 fieldCountToCmpInPrev,
                                                const VEC_ELE_CMP &matchEles,
                                                const inclusiveVec &matchInclusive,
                                                const options &o)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_bac.isValid(), "can not be invalid");
      SDB_ASSERT(_bac.isReadonly(), "must be readonly");
      SDB_ASSERT(!_bac.isPathEmpty(), "can not be empty");

      clearPositionOfCurrentNode();

      RECORD_SLOT_ID pos = INVALID_RECORD_SLOT_ID;
      UINT32 locatedDepth = 0;
      btreeNode node = _bac.getEndNodeInPath();

      do
      {
         BOOLEAN outOfBound = FALSE;
         btreeItemLocation locd; /// location of current depth
         rc = node.keyLocate(prevKey, fieldCountToCmpInPrev,
                             matchEles, matchInclusive,
                             !o.isInclusive(), o.isForward(),
                             locd, outOfBound, &_builder);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to locate key in btree node:%d", rc);
            goto error;
         }

         SDB_ASSERT(locd.isValid(), "can not be invalid");
         if (!outOfBound)
         {
            _bac.endToAccessNonPathEndNodes();
            locatedDepth = node.getDepth();
            pos = locd.slotPos;
         }

         if (INVALID_PAGE_ID == locd.child)
         {
            /// leaf node or child removed
            break;
         }
         else
         {
            SDB_ASSERT(!node.isLeaf(), "can not be leaf node");
            btreePathFootprint footprint;
            footprint.setPos(locd.slotPos);
            footprint.setUpperBound(locd.isUpperBound);
            rc = _bac.pushChildNodeIntoPath(locd.child, footprint);
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
         clearPositionOfCurrentNode();
         goto done;
      }

      /// return to the last located node
      SDB_ASSERT(locatedDepth < _bac.getPathSize(), "impossible");
      _bac.popEnds(_bac.getPathSize() - locatedDepth - 1);
      if (_bac.getEndNodeInPath().getItemCount() == pos)
      {
         --pos;
      }
      _pos = pos;
   done:
      return rc;
   error:
      clearPositionOfCurrentNode();
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

   INT32 btreeIndexIterator::relocateKeyAndRidInTree(const ixmKey &key,
                                                     const recordID &rid,
                                                     btreeItemLocation &location)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_bac.isValid(), "can not be invalid");
      SDB_ASSERT(_bac.isReadonly(), "must be readonly");
      SDB_ASSERT(key.isValid(), "can not be invalid");
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
            _bac.endToAccessNonPathEndNodes();
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
            /// leaf node or child removed
            break;
         }
         else
         {
            SDB_ASSERT(!node.isLeaf(), "can not be leaf node");
            btreePathFootprint footprint;
            footprint.setPos(locd.slotPos);
            footprint.setUpperBound(locd.isUpperBound);
            rc = _bac.pushChildNodeIntoPath(locd.child, footprint);
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

      {
         btreeNode node;
         /// return to the last located node
         SDB_ASSERT(locatedDepth < _bac.getPathSize(), "impossible");
         _bac.popEnds(_bac.getPathSize() - locatedDepth - 1);
         node = _bac.getEndNodeInPath();
         location.child = node.isLeaf() ? INVALID_PAGE_ID : node.getChild(pos);
         location.identical = FALSE;
         location.isUpperBound = FALSE;
         location.slotPos = pos;
         _pos = pos;
      }
      
   done:
      return rc;
   error:
      resetLocation();
      goto done;
   }

   INT32 btreeIndexIterator::traverseDownToBottom(BOOLEAN forward,
                                                  const btreeItemLocation &location)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(location.isValid(), "must be valid");
      SDB_ASSERT(INVALID_PAGE_ID != location.child, "can not be invalid");
      SDB_ASSERT(!_bac.isPathEmpty(), "can not be empty");
      btreeItemLocation prev = location;

      do
      {
         SDB_ASSERT(!_bac.getEndNodeInPath().isLeaf(), "can not be leaf");
         RECORD_SLOT_ID pos = INVALID_RECORD_SLOT_ID;
         PAGE_ID nextChild = INVALID_PAGE_ID;
         btreeNode node;
         btreePathFootprint fp;
         fp.setPos(prev.slotPos);
         fp.setUpperBound(prev.isUpperBound);

         rc = _bac.pushChildNodeIntoPath(prev.child, fp, &node);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to push child into path:%d", rc);
            goto error;
         }

         _bac.endToAccessNonPathEndNodes();

         SDB_ASSERT(0 < node.getItemCount(), "can not be empty");
         if (node.isLeaf())
         {
            pos = forward ? 0 : (node.getItemCount() - 1);
            resetPositionOfCurrentNode(pos);
            goto done;
         }
         else
         {
            nextChild = forward ? node.getLeftChild(0) : node.getRightChild();
            if (INVALID_PAGE_ID == nextChild)
            {
               pos = forward ? 0 : (node.getItemCount() - 1);
               resetPositionOfCurrentNode(pos);
               goto done;
            }
            else
            {
               prev.child = nextChild;
               prev.identical = FALSE;
               prev.slotPos = forward ? 0 : node.getItemCount();
               prev.isUpperBound = !forward;
               continue;
            }
         }
         
      } while (TRUE);
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeIndexIterator::advanceInSubTree(const bson::BSONObj &prevKey,
                                              INT32 fieldCountToCmpInPrev,
                                              const VEC_ELE_CMP &matchEles,
                                              const inclusiveVec &matchInclusive,
                                              const options &o)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_bac.isValid(), "can not be invalid");
      SDB_ASSERT(_bac.isReadonly(), "must be readonly");
      SDB_ASSERT(!_bac.isPathEmpty(), "can not be empty");

      do
      {
         RECORD_SLOT_ID currentPos = _pos;
         btreeNode node = _bac.getEndNodeInPath();
         BOOLEAN back = FALSE;
         btreeItemLocation locd;

         rc = node.keyAdvance(currentPos, prevKey, fieldCountToCmpInPrev,
                              matchEles, matchInclusive, !o.isInclusive(),
                              o.isForward(), back,
                              locd, &_builder);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to advance key in btree node:%d", rc);
            goto error;
         }

         if (back)
         {
            BOOLEAN obstructed = FALSE;
            INT32 ancestorDepth = -1;
            BOOLEAN footPrintIsFaithful = FALSE;

            clearPositionOfCurrentNode();
            if (node.isRoot())
            {
               break;
            }

            rc = prepareToGoBackToAncestors(o.isForward(), obstructed,
                                            ancestorDepth, footPrintIsFaithful);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to prepare to go back:%d", rc);
               goto error;
            }

            if (obstructed)
            {
               resetLocation();
               break;
            }
            else if (ancestorDepth < 0)
            {
               resetLocation();
               goto done;
            }

            _bac.popEnds(_bac.getPathSize() - ancestorDepth - 1);
            continue;
         }
         else
         {
            btreePathFootprint fp;
            fp.setPos(locd.slotPos);
            fp.setUpperBound(locd.isUpperBound);
            _bac.endToAccessNonPathEndNodes();
            rc = _bac.pushChildNodeIntoPath(locd.child, fp);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to push child into path:%d", rc);
               goto error;
            }
            break;
         }
      } while (TRUE);
      
      if (_bac.isPathEmpty())
      {
         rc = locateKeyInTree(prevKey, fieldCountToCmpInPrev,
                              matchEles, matchInclusive, o);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to relocate key in tree:%d", rc);
            goto error;
         }
      }
      else
      {
         rc = locateKeyInSubTree(prevKey, fieldCountToCmpInPrev,
                                 matchEles, matchInclusive, o);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to relocate key in sub tree:%d", rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      resetLocation();
      goto done;
   }
} // namespace vessel

} // namespace engine
