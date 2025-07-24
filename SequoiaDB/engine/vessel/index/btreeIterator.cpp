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

   Source File Name = btreeIterator.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/btreeIterator.h"
#include "vessel/indexObject.h"
#include "vessel/requestContext.h"
#include "vessel/indexSpace.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"
#include "vessel/btreeNode.h"

namespace engine
{
namespace vessel
{
   btreeIterator::~btreeIterator()
   {
      _bac.reset();
   }

   INT32 btreeIterator::init(requestContext *context,
                             indexSpace *is,
                             indexObject *obj)
   {
      INT32 rc = SDB_OK;

      reset();

      if (OSS_UNLIKELY(nullptr == context ||
                       nullptr == is ||
                       nullptr == obj))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _bac.init(context, is, obj);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init btree context:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      reset();
      goto done;
   }

   void btreeIterator::reset()
   {
      _current.reset();
      _bac.reset();
      _pos = INVALID_RECORD_SLOT_POS;
      return;
   }

   DPS_TRANS_ID btreeIterator::getTransID() const
   {
      SDB_ASSERT(isReadyToRead(), "can not be invalid");
      const btreeNodePageHead *head = nullptr;
      strictBuffer buffer = _bac.getPathNode(_bac.getPathSize() - 1).
                        getPageBuffer()->getReadableBodyBuffer();
      head = buffer.getReadableObjPtr<btreeNodePageHead>(0);         
      return head->transID;
   }

   UINT64 btreeIterator::getLSN() const
   {
      SDB_ASSERT(isReadyToRead(), "can not be invalid");
      return _bac.getPathNode(_bac.getPathSize() - 1).getPageBuffer()->
             getRuntimeBuffer().getPageHead()->lsn;
   }

   UINT32 btreeIterator::getTransferTick() const
   {
      SDB_ASSERT(_bac.isValid(), "can not be invalid");
      return _bac.getTransferTick();
   }

   INT32 btreeIterator::seek(const keyString &ks)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!ks.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      _resetCacheAndLocation();

      if (!_bac.hasBtreeRoot())
      {
         goto done;
      }

      rc = _bac.pushRootIntoPath();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to push root into path:%d", rc);
         goto error;
      }

      rc = _seekFromPathEnd(ks);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to seek from path end:%d" ,rc);
         goto error;
      }

      rc = _cacheOrMove(TRUE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to cache valid entry:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      reset();
      goto done;
   }

   INT32 btreeIterator::seekForPrev(const keyString &ks)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!ks.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      _resetCacheAndLocation();

      if (!_bac.hasBtreeRoot())
      {
         goto done;
      }

      rc = _bac.pushRootIntoPath();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to push root into path:%d", rc);
         goto error;
      }

      rc = _seekFromPathEnd(ks);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to seek from path end:%d" ,rc);
         goto error;
      }

      if (_hasLocation())
      {
         rc = _moveToNext(FALSE);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to locate to prev");
            goto error;
         }
      }

      rc = _cacheOrMove(FALSE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to cache valid entry:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      reset();
      goto done;
   }

   INT32 btreeIterator::next(BOOLEAN forward)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isReadyToRead()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      _current.reset();

      rc = _moveToNext(forward);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to locate next entry:%d", rc);
         goto error;
      }

      rc = _cacheOrMove(forward);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to cache valid entry:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      reset();
      goto done;
   }

   INT32 btreeIterator::advance(const keyString &ks, BOOLEAN forPrev)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isReadyToRead()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (!ks.isValid())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else
      {
         INT32 cmp = _current.compareElements(ks);
         if ((forPrev && cmp < 0) ||
             (!forPrev && cmp > 0))
         {
            PD_LOG(PDERROR, "invalid advanced key string");
            rc = SDB_INVALID_OPERATION;
            goto error;
         }
      }

      _current.reset();
      rc = _advance(ks, !forPrev);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to advance location:%d", rc);
         goto error;
      }

      if (!_hasLocation())
      {
         goto done;
      }

      if (forPrev)
      {
         rc = _moveToNext(FALSE);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to locate to next:%d", rc);
            goto error;
         }
      }

      rc = _cacheOrMove(!forPrev);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to cache valid entry:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      reset();
      goto done;
   }

   INT32 btreeIterator::locate(const location &l)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(!l.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (_bac.getTransferTick() != l.getTransferTick())
      {
         PD_LOG(PDERROR, "unsame transfer ticks[%d, %d]",
                _bac.getTransferTick(), l.getTransferTick());
         rc = SDB_VESSEL_BTREE_LOCATION_EXPIRED;
         goto error;
      }

      _resetCacheAndLocation();

      rc = _restorePath(l._path);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to restore accessing path:%d", rc);
         goto error;
      }
      else
      {
         SDB_ASSERT(!_bac.isPathEmpty(), "impossible");
         btreeNode node = _bac.getEndNodeInPath();
         if (OSS_UNLIKELY(node.getItemCount() <= (UINT32)l.getPos()))
         {
            PD_LOG(PDERROR, "slot pos[%d, %d] unmatched in location",
                   l.getPos(), node.getItemCount());
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         _pos = l.getPos();

         rc = node.getOwnedEntry(_pos, _current);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get owned entry:%d", rc);
            goto error;
         }
      }

   done:
      return rc;
   error:
      reset();
      goto done;
   }

   btreeIterator::location btreeIterator::getLocation() const
   {
      SDB_ASSERT(_hasLocation(), "can not be invalid");
      location l;
      l._transferTick = _bac.getTransferTick();
      l._pos = _pos;
      _bac.exportPathCoding(l._path);
      return std::move(l);
   }

   void btreeIterator::_resetCacheAndLocation()
   {
      _current.reset();
      _bac.resetPath();
      _pos = INVALID_RECORD_SLOT_POS;
      return;
   }

   INT32 btreeIterator::_seekFromPathEnd(const keyString &ks,
                                         RECORD_SLOT_POS pos,
                                         BOOLEAN forward)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!_bac.isPathEmpty(), "can not be invalid");
      SDB_ASSERT(isValidRecordSlotPosition(pos), "can not be invalid");
      btreeNodeSeekResult res;
      btreeNode node;

      do
      {
         node = _bac.getEndNodeInPath();
         rc = node.seek(ks, pos, forward, res);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to seek in node:%d", rc);
            goto error;
         }
         else if (!res.hasChild())
         {
            /// 1. it is a leaf node
            /// 2. it's child has been removed
            _pos = res.getPos();
            break;
         }
         else
         {
            btreePathFootprint fp(res.getPos(), res.isUpperBound());
            rc = _bac.pushChildNodeIntoPath(res.getChild(), fp);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "faield to push child into path:%d", rc);
               goto error;
            }

            node.reset();
            res.reset();
         }

      } while (TRUE);

      if (res.isUpperBound())
      {
         _relocateToAncestorNode(TRUE);
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeIterator::_moveToNext(BOOLEAN forward)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_hasLocation(), "can not be invalid");

      if (_bac.getEndNodeInPath().isLeaf())
      {
         rc = _nextFromLeaf(forward);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get next pos from leaf node:%d", rc);
            goto error;
         }
      }
      else if (forward)
      {
         rc = _locateForwardFromNonleaf();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to scan forward from non-leaf node:%d", rc);
            goto error;
         }
      }
      else
      {
         rc = _locateBackwardFromNonleaf();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to scan backward from non-leaf node:%d", rc);
            goto error;
         }
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeIterator::_nextFromLeaf(BOOLEAN forward)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_hasLocation(), "can not be invalid");
      btreeNode node = _bac.getEndNodeInPath();
      SDB_ASSERT(node.isLeaf(), "can not be invalid");
      RECORD_SLOT_POS pos = _getNextPos(_pos, forward);

      if (pos < 0 ||
          node.getItemCount() <= (UINT32)pos)
      {
         _relocateToAncestorNode(forward);
      }
      else
      {
         _pos = pos;
      }      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeIterator::_locateForwardFromNonleaf()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_hasLocation(), "can not be invalid");
      btreeNode node = _bac.getEndNodeInPath();
      SDB_ASSERT(!node.isLeaf(), "can not be invalid");
      RECORD_SLOT_POS pos = _pos + 1;
      PAGE_ID child = node.getChild(pos);

      if (INVALID_PAGE_ID != child)
      {
         btreePathFootprint fp(pos, (UINT32)pos == node.getItemCount());
         rc = _locateToBottomStart(child, fp);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to locate to bottom start:%d", rc);
            goto error;
         }
      }
      else if ((UINT32)pos < node.getItemCount())
      {
         _pos = pos;
      }
      else
      {
         _relocateToAncestorNode(TRUE);
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeIterator::_locateBackwardFromNonleaf()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_hasLocation(), "can not be invalid");
      btreeNode node = _bac.getEndNodeInPath();
      SDB_ASSERT(!node.isLeaf(), "can not be invalid");
      RECORD_SLOT_POS pos = _pos;
      PAGE_ID child = node.getChild(pos);

      if (INVALID_PAGE_ID != child)
      {
         btreePathFootprint fp(pos, FALSE);
         rc = _locateToBottomEnd(child, fp);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to locate to bottom end:%d", rc);
            goto error;
         }
      }
      else if (0 < pos)
      {
         _pos = pos - 1;
      }
      else
      {
         _relocateToAncestorNode(FALSE);
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeIterator::_locateToBottomStart(PAGE_ID child, const btreePathFootprint &fp)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!_bac.isPathEmpty(), "can not be invalid");
      SDB_ASSERT(INVALID_PAGE_ID != child, "can not be invalid");
      SDB_ASSERT(fp.isValid(), "can not be invalid");
      PAGE_ID childToPush = child;
      btreePathFootprint footprint = fp;

      do
      {
         btreeNode node;
         rc = _bac.pushChildNodeIntoPath(childToPush, footprint, &node);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to push child into path:%d", rc);
            goto error;
         }
         else if (node.isLeaf())
         {
            break;
         }
         else
         {
            PAGE_ID pid = node.getChild(0);
            if (INVALID_PAGE_ID == pid)
            {
               break;
            }
            else
            {
               footprint.setPos(0);
               footprint.setUpperBound(FALSE);
               childToPush = pid;
               continue;
            }
         }
      } while (TRUE);
      
      _pos = 0;
   done:
      return rc;
   error:
      goto done;
   }
   
   INT32 btreeIterator::_locateToBottomEnd(PAGE_ID child, const btreePathFootprint &fp)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!_bac.isPathEmpty(), "can not be invalid");
      SDB_ASSERT(INVALID_PAGE_ID != child, "can not be invalid");
      SDB_ASSERT(fp.isValid(), "can not be invalid");
      PAGE_ID childToPush = child;
      btreePathFootprint footprint = fp;
      RECORD_SLOT_POS pos = INVALID_RECORD_SLOT_POS;

      do
      {
         btreeNode node;
         rc = _bac.pushChildNodeIntoPath(childToPush, footprint, &node);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to push child into path:%d", rc);
            goto error;
         }
         else if (node.isLeaf())
         {
            pos = node.getItemCount() - 1;
            break;
         }
         else
         {
            PAGE_ID pid = node.getChild(node.getItemCount());
            if (INVALID_PAGE_ID == pid)
            {
               pos = node.getItemCount() - 1;
               break;
            }
            else
            {
               footprint.setPos(node.getItemCount());
               footprint.setUpperBound(TRUE);
               childToPush = pid;
               continue;
            }
         }
      } while (TRUE);
      
      SDB_ASSERT(isValidRecordSlotPosition(pos), "impossible");
      _pos = pos;
   done:
      return rc;
   error:
      goto done;
   }

   void btreeIterator::_relocateToAncestorNode(BOOLEAN forward)
   {
      SDB_ASSERT(!_bac.isPathEmpty(), "can not be invalid");

      while (1 < _bac.getPathSize())
      {
         btreePathFootprint fp = _bac.getEndNodeFootprint();
         RECORD_SLOT_POS pos = forward ?
                               fp.getPos() : fp.getPos() - 1;

         _bac.popEnd();/// pn is not valid from here!

         if (pos < 0 ||
             _bac.getEndNodeInPath().getItemCount() == (UINT32)pos)
         {
            continue;
         }
         else
         {
            _pos = pos;
            goto done;
         }
      }

      _pos = INVALID_RECORD_SLOT_POS;
      _bac.resetPath();

   done:
      return;
   }

   BOOLEAN btreeIterator::_hasLocation() const
   {
      return isValidRecordSlotPosition(_pos) &&
             !_bac.isPathEmpty();
   }

   INT32 btreeIterator::_restorePath(const ossPoolVector<UINT64> &path)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_bac.isPathEmpty(), "can not be invalid");
      SDB_ASSERT(!path.empty(), "can not be invalid");
      PAGE_ID lpid = INVALID_PAGE_ID;
      btreePathFootprint childFp;
      auto itr = path.cbegin();
      btreeAccessPathNode::decode(*itr, lpid, childFp);
      if (OSS_UNLIKELY(lpid != _bac.getBtreeRoot()))
      {
         PD_LOG(PDERROR, "root node unmatched in path");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = _bac.pushRootIntoPath();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to push root node into path:%d", rc);
         goto error;
      }

      for (++itr; itr != path.cend(); ++itr)
      {
         PAGE_ID lpid = INVALID_PAGE_ID;
         btreePathFootprint fp;
         btreeAccessPathNode::decode(*itr, lpid, fp);
         rc = _bac.pushChildNodeIntoPath(lpid, childFp);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to push child into path[%d, %d, %d]:%d",
                   lpid, childFp.getPos(), childFp.getFlags(), rc);
            goto error;
         }

         childFp = fp;
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeIterator::_advance(const keyString &ks, BOOLEAN forward)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_hasLocation() && ks.isValid(), "can not be invalid");

      RECORD_SLOT_POS pos = _pos;
      btreeNode node = _bac.getEndNodeInPath();

      while (!node.isRoot())
      {
         BOOLEAN outOfBound = FALSE;
         rc = node.isOutOfKeyBound(ks, forward, outOfBound);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to check if key is out of bound:%d", rc);
            goto error;
         }
         else if (!outOfBound)
         {
            ///TODO: should we think about go down to right/ first left child
            /// if out of bound?
            break;
         }
         else
         {
            pos = _bac.getEndNodeFootprint().getPos();
            _bac.popEnd();
         }
      }

      rc = _seekFromPathEnd(ks, pos, forward);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to seek from path end:%d", rc);
         goto error;
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeIterator::_cacheOrMove(BOOLEAN forward)
   {
      INT32 rc = SDB_OK;
      _current.reset();

      while (_hasLocation())
      {
         btreeNode node = _bac.getEndNodeInPath();
         if (!node.isLeaf())
         {
            btreeItemSlot slot = node.getItemSlot(_pos);
            if (slot.isMarkedDeleted())
            {
               rc = _moveToNext(forward);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to move to next:%d", rc);
                  goto error;
               }

               continue;
            }
         }

         rc = node.getOwnedEntry(_pos, _current);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get owned entry at pos[%d], rc:%d", _pos, rc);
            goto error;
         }

         break;
      }
   done:
      return rc;
   error:
      goto done;
   }
} // namespace vessel

} // namespace engine
