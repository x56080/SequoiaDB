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

   void btreeNodePath::init(indexContext *ic)
   {
      fini();
      SDB_ASSERT(NULL != ic && ic->isValid(), "can not be invali");
      _ic = ic;
      return;
   }

   void btreeNodePath::fini()
   {
      clearPath();
      _ic = NULL;
      for (UINT32 i = 0; i < _free.size(); ++i)
      {
         SDB_OSS_DEL _free[i];
      }
      _free.clear();
      return;
   }

   void btreeNodePath::clearPath()
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

   INT32 btreeNodePath::push(requestContext *context,
                             logicalPageBuffer *buffer,
                             btreeNode &out)
   {
      INT32 rc = SDB_OK;
      logicalPageBuffer *lpb = NULL;
      const btreeNodePageHead *head = NULL;

      out.reset();

      if (OSS_UNLIKELY(NULL == _ic))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      } 
      else if (OSS_UNLIKELY(NULL == context ||
                            NULL == buffer ||
                            !buffer->isValid()))
      {
         rc = SDB_INVALIDARG;
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

      out = btreeNode(lpb, _ic, _size++);
   done:
      return rc;
   error:
      out.reset();
      goto done;
   }

   INT32 btreeNodePath::getAccessingNode(UINT32 depth, btreeNode &node)const
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(NULL == _ic))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (depth < _size)
      {
         const _pathNode &pn = getPathNode(depth);
         if (!pn.isAccessing())
         {
            PD_LOG(PDERROR, "node is not accessing with depth[%d]", depth);
            rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
            goto error;
         }
         
         node = btreeNode(pn._lpb, _ic, depth);
      }
      else
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }
      
   done:
      return rc;
   error:
      node.reset();
      goto done;
   }

   INT32 btreeNodePath::validateBtreePage(requestContext *context,
                                          logicalPageBuffer *buffer,
                                          const btreeNodePageHead **out)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != _ic, "can not be invalid");
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

      head = rpb.getReadablePtrOfBody<btreeNodePageHead>(0);
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
                _ic->getIndexID(), head->indexId, gpid.toString().c_str());
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

   const btreeNodePath::_pathNode &btreeNodePath::getPathNode(UINT32 i)const
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

   INT32 btreeNodePath::getPageBuffer(UINT32 depth, logicalPageBuffer *&buffer)const
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(_size <= depth))
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      {
         const _pathNode &pn = getPathNode(depth);
         buffer = pn._lpb;
      }

   done:
      return rc;
   error:
      goto done;
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
   
   void btreeNodePath::releaseBuffer(logicalPageBuffer *buffer)
   {
      if (OSS_LIKELY(NULL != buffer))
      {
         buffer->fini();
         _free.push_back(buffer);
      }
   }
} // namespace vessel

} // namespace engine
