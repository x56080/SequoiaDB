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

   Source File Name = btreeNodePath.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/btreeNodePath.h"
#include "vessel/indexContext.h"
#include "vessel/indexSpace.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"
#include "vessel/requestContext.h"
#include "vessel/indexDefPageAccessor.h"
#include "vessel/btreeNodePageIniter.h"

namespace engine
{
namespace vessel
{
   btreeNodePath::_pathNode::_pathNode(logicalPageBuffer *lpb,
                                       UINT32 splitedTimes)
   {
      fini();
      SDB_ASSERT(NULL != lpb && lpb->isValid(), "can not be invalid");
      _lpid = lpb->getLogicalPid();
      SDB_ASSERT(INVALID_PAGE_ID != _lpid, "can not be invalid");
      _splitedTimes = splitedTimes;
   }

   btreeNodePath::_pathNode::~_pathNode()
   {

   }

   void btreeNodePath::_pathNode::fini()
   {
      _lpid = INVALID_PAGE_ID;
      _splitedTimes = 0;
      _lpb = NULL;
      return;
   }
////////////////btreeNodePath::_pathNode end

   btreeNodePath::~btreeNodePath()
   {
      fini();
   }

   void btreeNodePath::init(indexContext *ic,
                            indexSpace *is)
   {
      fini();
      SDB_ASSERT(NULL != ic, "can not be null");
      SDB_ASSERT(ic->isValid(), "can not be invalid");
      SDB_ASSERT(NULL != is, "can not be null");
      SDB_ASSERT(is->isOpen(), "can not be closed");

      _ic = ic;
      _is = is;
   }

   void btreeNodePath::fini()
   {
      if (NULL == _ic)
      {
         goto done;
      }

      for (UINT32 i = 0; i < _size; ++i)
      {
         _pathNode &pn = getPathNode(i);
         if (NULL != pn._lpb)
         {
            pn._lpb->fini();
            SDB_OSS_DEL pn._lpb;
            pn.fini();
         }
      }

      _dynamicNodes.clear();
      _size = 0;

      for (_FREE_BUFFER_LIST::const_iterator itr = _free.begin();
           itr != _free.end(); ++itr)
      {
         SDB_OSS_DEL (*itr);
      }
      _free.clear();

      _ic = NULL;
      _is = NULL;

   done:
      return;
   }

   BOOLEAN btreeNodePath::isRoot(const btreeNode &bn)const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(bn.isValid(), "can not be invalid");
      return 0 == bn._depth;
   }

   logicalPageBuffer *btreeNodePath::allocateBuffer()
   {
      logicalPageBuffer *lpb = NULL;
      if (!_free.empty())
      {
         lpb = _free.back();
         _free.pop_back();
      }
      else
      {
         lpb = SDB_OSS_NEW logicalPageBuffer();
      }
      return lpb;
   }

   void btreeNodePath::clearWholePath()
   {
      for (UINT32 i = 0; i < _size; ++i)
      {
         _pathNode &pn = getPathNode(i);
         if (NULL != pn._lpb)
         {
            pn._lpb->fini();
            releaseBuffer(pn._lpb);
         }
         pn.fini();
      }

      _dynamicNodes.clear();
      _size = 0;
      return;
   }

   INT32 btreeNodePath::pushNode(requestContext *context,
                                 PAGE_ID lpid,
                                 const ossSharedLatchMode &mode,
                                 btreeNode &out)
   {
      INT32 rc = SDB_OK;
      logicalPageBuffer *lpb = NULL;
      const btreeNodePageHead *head = NULL;

      out.fini();

      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == context ||
                            INVALID_PAGE_ID == lpid ||
                            mode.isNone()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      lpb = allocateBuffer();
      if (OSS_UNLIKELY(NULL == lpb))
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      rc = _is->getLogicalPageBuffer(context, lpid, mode, *lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get buffer of page[%d], rc:%d", lpid, rc);
         goto error;
      }

      rc = validateBtreePage(context, lpb, &head);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to validate btree page:%d", rc);
         goto error;
      }

      if (_size < _DEFAULT_CAPACITY)
      {
         _staticNodes[_size] = _pathNode(lpb, head->splitedTimes);
      }
      else
      {
         _dynamicNodes.push_back(_pathNode(lpb, head->splitedTimes));
      }
      ++_size;

      out = btreeNode(this, lpb, _size - 1);
   done:
      return rc;
   error:
      if (NULL != lpb)
      {
         lpb->fini();
         releaseBuffer(lpb);
      }
      out.fini();
      goto done;
   }

   INT32 btreeNodePath::ensureRootToWrite(requestContext *context,
                                          btreeNode &rootNode)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(isEmpty(), "must be empty");
      SDB_ASSERT(!_entryPage.isValid(), "must be invalid");

      const indexDefHead *head = NULL;
      btreeNode root;
      PAGE_ID rootLpid = INVALID_PAGE_ID;
      indexDefPageAccessor accessor;
      ossSharedLatchMode mode;
      mode.setUpgrade();

      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == context))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _is->getLogicalPageBuffer(context, _ic->getEntryLpid(), mode, _entryPage);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get lps of page[%d], rc:%d", 
                _ic->getEntryLpid(), rc);
         goto error;
      }

      rc = accessor.getIndexDefPageHead(context, _ic->getIndexID(),
                                        &_entryPage, &head);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get index def page head:%d", rc);
         goto error;
      }

      if (INVALID_PAGE_ID == head->btreeRoot)
      {
         rc = createRoot(context, rootLpid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to create btree root:%d", rc);
            goto error;
         }
      }
      else
      {
         rootLpid = head->btreeRoot;
      }

      rc = pushNode(context, rootLpid, mode, root);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to push node into path:%d", rc);
         goto error;
      }

      if (root.hasExtNode())
      {
         btreeNode newRoot;
         rc = splitRootAndCreateNewOne(context, root, &newRoot);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to create new root:%d", rc);
            goto error;
         }
         rootNode = root;
      }
      else
      {
         rootNode = root;
      }

   done:
      _entryPage.fini();
      return rc;
   error:
      goto done;
   }

   INT32 btreeNodePath::validateBtreePage(requestContext *context,
                                          logicalPageBuffer *buffer,
                                          const btreeNodePageHead **out)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(NULL != buffer, "can not be null");
      SDB_ASSERT(buffer->isValid(), "can not be invalid");
      const runtimePageBuffer &rpb = buffer->getRuntimeBuffer();
      const globalPageID &gpid = buffer->getRuntimeBuffer().getGlobalPid();
      const btreeNodePageHead *head = NULL;

      rc = buffer->validatePage(PAGE_TYPE_BTREE_NODE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to validate btree page:[%s], rc:%d",
                gpid.toString().c_str(), rc);
         goto error;
      }

      head = rpb.getWritablePtrOfBody<btreeNodePageHead>(0);
      if (NULL == head)
      {
         PD_LOG(PDERROR, "failed to get btree page head");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (context->getLogicalCLID() != head->clLogicalID)
      {
         PD_LOG(PDERROR, "different logical clids found[%d,%d] on page[%s]",
                context->getLogicalCLID(), head->clLogicalID,
                gpid.toString().c_str());
         rc = SDB_VESSEL_PAGE_HEAD_NOT_MATCH;
         goto error;
      }

      if (_ic->getIndexID() != head->indexId)
      {
         PD_LOG(PDERROR, "different logical index ids found[%d,%d] on page[%s]",
                _ic->getIndexID(), head->indexId,
                gpid.toString().c_str());
         rc = SDB_VESSEL_PAGE_HEAD_NOT_MATCH;
         goto error;
      }

      if (NULL != out)
      {
         *out = head;
      }
   done:
      return rc;
   error:
      goto done;
   }

   void btreeNodePath::releaseBuffer(logicalPageBuffer *lpb)
   {
      if (OSS_LIKELY(NULL != lpb))
      {
         lpb->fini();
         _free.push_back(lpb);
      }
      return;
   }

   btreeNodePath::_pathNode &btreeNodePath::getPathNode(UINT32 i)
   {
      SDB_ASSERT(i < _size, "out of bound");
      if (i < _DEFAULT_CAPACITY)
      {
         return _staticNodes[i];
      }
      else
      {
         return _dynamicNodes[i - _DEFAULT_CAPACITY];
      }
   }

   INT32 btreeNodePath::createRoot(requestContext *context,
                                   PAGE_ID &out)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "must be valid");
      SDB_ASSERT(_entryPage.isValid(), "can not be invalid");
      const ossSharedLatchMode &mode = _entryPage.getLockingMode();
      SDB_ASSERT(mode.isUpgrade() || mode.isExclusive(), "wrong type locking");

      out = INVALID_PAGE_ID;
      indexDefPageAccessor accessor;
      btreeNodePageIniter initer;
      PAGE_ID lpid = INVALID_PAGE_ID;

      initer.set(context->getLogicalCLID(), _ic->getIndexID(), TRUE);
      rc = _is->allocatePages(context, &initer, 1, &lpid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate new page:%d", rc);
         goto error;
      }

      rc = accessor.updateBtreeRoot(context, _ic->getIndexID(),
                                    lpid, &_entryPage);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to update root:%d", rc);
         goto error;
      }

      out = lpid;
   done:
      return rc;
   error:
      if (INVALID_PAGE_ID != lpid)
      {
         _is->releasePages(context, 1, &lpid);
      }
      goto done;
   }

   INT32 btreeNodePath::splitRootAndCreateNewOne(requestContext *context,
                                                 btreeNode &root,
                                                 btreeNode *newRoot)
   {
      SDB_ASSERT(FALSE, "TODO");
      return SDB_OK;
   }
} // namespace vessel

} // namespace engine
