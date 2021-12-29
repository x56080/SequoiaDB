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

   Source File Name = btreeAccessContext.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/btreeAccessContext.h"
#include "pdTrace.hpp"
#include "vessel/indexContext.h"
#include "vessel/logicalPageBuffer.h"
#include "vessel/indexSpace.h"
#include "vessel/requestContext.h"
#include "vessel/runtimeMbContext.h"

namespace engine
{
namespace vessel
{
   btreeAccessContext::~btreeAccessContext()
   {
      if (isValid())
      {
         fini();
      }
   }

   void btreeAccessContext::init(indexContext *ic,
                                 requestContext *context,
                                 indexSpace *is)
   {
      SDB_ASSERT(NULL != ic && ic->isValid(), "can not be invalid");
      SDB_ASSERT(ic->getIndexType() == INDEX_TYPE_BTREE, "msut be btree");
      SDB_ASSERT(NULL != context && context->isMbContextAttached(), "can not be invalid");
      SDB_ASSERT(NULL != is && is->isOpen(), "can not be invalid");

      fini();
      _ic = ic;
      _context = context;
      _is = is;
      return;
   }

   void btreeAccessContext::fini()
   {
      _ic = NULL;
      _context = NULL;
      _is = NULL;
      for (UINT32 i = 0; i < _path.size(); ++i)
      {
         btreeAccessPathNode &pn = _path[i];
         if (pn.isAccessing())
         {
            pn.getPageBuffer()->fini();
            SDB_OSS_DEL pn.getPageBuffer();
         }
      }
      _path.clear();

      for (_FREE_BUFFERS::const_iterator itr = _free.begin();
           itr != _free.end(); ++itr)
      {
         SDB_OSS_DEL (*itr);
      }
      _free.clear();
      _readonly = TRUE;
      _pessimistic = FALSE;
      return;
   }

   logicalPageBuffer *btreeAccessContext::allocateBuffer()
   {
      logicalPageBuffer *buffer = NULL;

      if (!_free.empty())
      {
         buffer = _free.back();
         _free.pop_back();
      }
      else
      {
         buffer = SDB_OSS_NEW logicalPageBuffer();
      }

      return buffer;
   }

   void btreeAccessContext::releaseBuffer(logicalPageBuffer *buffer)
   {
      SDB_ASSERT(NULL != buffer, "can not be null");
      buffer->fini();
      _free.push_back(buffer);
      return;
   }

   INT32 btreeAccessContext::pushRootIntoPath(btreeNode *node)
   {
      INT32 rc = SDB_OK;
      ossSharedLatchMode mode;
      logicalPageBuffer *buffer = NULL;

      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (!_path.empty())
      {
         PD_LOG(PDERROR, "can not push root into non-empty path");
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

      mode = estimateRootMode();
      buffer = allocateBuffer();
      if (OSS_UNLIKELY(NULL == buffer))
      {
         PD_LOG(PDERROR, "failed to allocate page buffer");
         rc = SDB_OOM;
         goto error;
      }

      do
      {
         PAGE_ID rootLpid = _ic->getObj().getBtreeRoot();
         if (INVALID_PAGE_ID == rootLpid)
         {
            PD_LOG(PDERROR, "no root node exists");
            rc = SDB_VESSEL_RESOURCES_NOT_INIT;
            goto error;
         }

         rc = _is->getLogicalPageBuffer(_context, rootLpid, mode, *buffer);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get page buffer of page[%d], rc:%d",
                  rootLpid, rc);
            goto error;
         }

         if (_ic->getObj().getBtreeRoot() != rootLpid)
         {
            buffer->fini();
            continue;
         }

         rc = validateBtreePage(*buffer);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to validate root page:%d", rc);
            goto error;
         }

         break;
      } while (TRUE);

      rc = pushIntoPath(buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to push node buffer into path:%d", rc);
         goto error;
      }

      if (NULL != node)
      {
         *node = getEndNodeInPath();
      }
      
   done:
      return rc;
   error:
      if (NULL != buffer)
      {
         buffer->fini();
         releaseBuffer(buffer);
      }
      goto done;
   }

   INT32 btreeAccessContext::pushChildNodeIntoPath(PAGE_ID lpid,
                                                   const btreePathFootprint &footprint,
                                                   btreeNode *node)
   {
      INT32 rc = SDB_OK;
      ossSharedLatchMode mode;
      logicalPageBuffer *buffer = NULL;

      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_PAGE_ID == lpid ||
                            !footprint.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (_path.empty())
      {
         PD_LOG(PDERROR, "can not push child node into path with out root");
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }
      else if (!_path[_path.size() - 1].isAccessing())
      {
         PD_LOG(PDERROR, "can not push child when not accessing father");
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

      buffer = allocateBuffer();
      if (OSS_UNLIKELY(NULL == buffer))
      {
         PD_LOG(PDERROR, "failed to allocate node buffer");
         rc = SDB_OOM;
         goto error;
      }

      mode = estimateChildMode();

      rc = _is->getLogicalPageBuffer(_context, lpid, mode, *buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page buffer of page[%d], rc:%d",
                lpid, rc);
         goto error;
      }

      rc = validateBtreePage(*buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to validate node page:%d", rc);
         goto error;
      }

      rc = pushIntoPath(buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to push node into path:%d", rc);
         goto error;
      }

      _path[_path.size() - 2].setChildFootprint(footprint);

      if (NULL != node)
      {
         *node = getEndNodeInPath();
      }
   done:
      return rc;
   error:
      if (NULL != buffer)
      {
         buffer->fini();
         releaseBuffer(buffer);
      }
      goto done;
   }

   INT32 btreeAccessContext::tryToReaccessNode(UINT32 depth,
                                               const ossSharedLatchMode &mode,
                                               BOOLEAN &obstructed)
   {
      INT32 rc = SDB_OK;
      logicalPageBuffer *buffer = NULL;

      obstructed = FALSE;

      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(mode.isNone()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(_path.size() <= depth))
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }
      else if (isStillAccessing(depth))
      {
         SDB_ASSERT(FALSE, "node is still be accessing");
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

      buffer = allocateBuffer();
      if (OSS_UNLIKELY(NULL == buffer))
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      rc = _is->tryToGetLogicalPageBuffer(_context,
                                          _path[depth].getLogicalPageId(),
                                          mode, *buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page buffer:%d", rc);
         goto error;
      }
      else if (!buffer->isValid())
      {
         obstructed = TRUE;
      }
      else
      {
         btreeNode node(buffer, depth, _ic);
         if (node.getSplitedTimes() != _path[depth].getSplitedTimes())
         {
            buffer->fini();
            PD_LOG(PDDEBUG, "current splited times[%d] not as same as[%d]",
                   node.getSplitedTimes(), _path[depth].getSplitedTimes());
            obstructed = TRUE;
         }
         else
         {
            _path[depth].reaccess(buffer);
            buffer = NULL;
         }
      }
   done:
      if (NULL != buffer)
      {
         buffer->fini();
         releaseBuffer(buffer);
      }
      return rc;
   error:
      goto done;
   }

   INT32 btreeAccessContext::pushIntoPath(logicalPageBuffer *buffer)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == buffer ||
                            !buffer->isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _path.append(btreeAccessPathNode(buffer));
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed tp append buffer to path:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   void btreeAccessContext::clearAccessPath()
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      for (UINT32 i = 0; i < _path.size(); ++i)
      {
         btreeAccessPathNode &pn = _path[i];
         if (pn.isAccessing())
         {
            pn.getPageBuffer()->fini();
            _free.push_back(pn.getPageBuffer());
         }
      }
      _path.clear();
   }

   void btreeAccessContext::endToAccessNonPathEndNodes()
   {
      endToAccessPreNodeInPath(1);
      return; 
   }

   void btreeAccessContext::endToAccessPreNodeInPath(UINT32 maxAccessingNum)
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      INT32 max = (INT32)_path.size() - (INT32)maxAccessingNum;
      for (INT32 i = 0; i < max; ++i)
      {
         btreeAccessPathNode &pn = _path[i];
         if (pn.isAccessing())
         {
            pn.getPageBuffer()->fini();
            _free.push_back(pn.getPageBuffer());
            pn.endToAccess();
         }
      }

      return;
   }

   void btreeAccessContext::popEnd()
   {
      popEnds(1);
      return;
   }

   void btreeAccessContext::popEnds(UINT32 n)
   {
      SDB_ASSERT(n <= _path.size(), "out of bound");
      for (UINT32 i = 0; i < n; ++i)
      {
         btreeAccessPathNode pn;
         if (_path.popBack(&pn))
         {
            if (pn.isAccessing())
            {
               pn.getPageBuffer()->fini();
               _free.push_back(pn.getPageBuffer());
            }

            if (!_path.empty())
            {
               _path[_path.size() - 1].clearChildFootprint();
            }
         }
      }

      return;
   }

   void btreeAccessContext::destroyEnds(UINT32 n)
   {
      SDB_ASSERT(n <= _path.size(), "out of bound");
      /// nodes to be destroyed must be all accessing.
      SDB_ASSERT(isStillAccessing(_path.size() - n), "must be accessing");
      for (UINT32 i = 0; i < n; ++i)
      {
         btreeAccessPathNode pn;
         if (_path.popBack(&pn))
         {
            if (pn.isAccessing())
            {
               pn.getPageBuffer()->destroy();
               _free.push_back(pn.getPageBuffer());
            }

            if (!_path.empty())
            {
               _path[_path.size() - 1].clearChildFootprint();
            }
         }
      }
   }

   void btreeAccessContext::destroyEnd()
   {
      destroyEnds(1);
   }
   
   btreeNode btreeAccessContext::getEndNodeInPath()
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(!_path.empty(), "can not be empty");
      UINT32 depth = _path.size() - 1;
      btreeAccessPathNode &pn = _path[depth];
      SDB_ASSERT(pn.isAccessing(), "end node should always be accessing");
      return btreeNode(pn.getPageBuffer(), depth, _ic);
   }

   btreeNode btreeAccessContext::getNodeInPath(UINT32 depth)
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(depth < _path.size(), "out of bound");
      
      btreeNode node;
      if (depth < _path.size())
      {
         btreeAccessPathNode &pn = _path[depth];
         if (pn.isAccessing())
         {
            node = btreeNode(pn.getPageBuffer(), depth, _ic);
         }
      }
      return node;
   }

   UINT32 btreeAccessContext::getPathSize()const
   {
      return _path.size();
   }

   BOOLEAN btreeAccessContext::isStillAccessing(UINT32 depth)const
   {
      SDB_ASSERT(depth < _path.size(), "out of bound");
      return _path[depth].isAccessing();
   }

   const btreeAccessPathNode &btreeAccessContext::getPathNode(UINT32 depth)const
   {
      SDB_ASSERT(depth < _path.size(), "out of bound");
      return _path[depth];
   }

   INT32 btreeAccessContext::prepareToReadAncestors(BOOLEAN forward,
                                                    BOOLEAN &obstructed,
                                                    BOOLEAN &footPrintIsFaithFul)
   {
      INT32 rc = SDB_OK;
      INT32 depth = -1;

      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (_path.size() <= 1)
      {
         SDB_ASSERT(FALSE, "no ancestors");
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }
      else if (isStillAccessing(_path.size() - 1))
      {
         SDB_ASSERT(FALSE, "end node must be accessing");
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

      depth = (INT32)_path.size() - 1;
      do
      {
         
      } while (0 <= depth);
      
   done:
      return rc;
   error:
      goto done;
   }

   ossSharedLatchMode btreeAccessContext::estimateRootMode()const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      static const UINT32 _SMALL_SCALE = 2;
      ossSharedLatchMode mode;
      if (isReadonly())
      {
         mode.setShared();
      }
      else if (isPessimistic())
      {
         mode.setExclusive();
      }
      else if (_ic->getObj().getBtreeRootSplitTimes() < _SMALL_SCALE)
      {
         mode.setExclusive();
      }
      else
      {
         mode.setShared();
      }
      return mode;
   }

   ossSharedLatchMode btreeAccessContext::estimateChildMode()const
   {
      SDB_ASSERT(!isPathEmpty(), "can not be empty");
      static const UINT32 _MAX_SHARED_SCALE = 2;
      ossSharedLatchMode mode;

      if (isReadonly())
      {
         mode.setShared();
      }
      else
      {
         const btreeAccessPathNode &pn = _path[_path.size() - 1];
         SDB_ASSERT(pn.isAccessing(), "must be accessing");
         ossSharedLatchMode fatherMode = pn.getNodeMode();
         if (!fatherMode.isShared())
         {
            mode = pn.getNodeMode();
         }
         else if (_path.size() < _MAX_SHARED_SCALE)
         {
            mode.setShared();
         }
         else
         {
            mode.setExclusive();
         }
      }
      return mode;
   }

   INT32 btreeAccessContext::validateBtreePage(const logicalPageBuffer &buffer)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(buffer.isValid(), "can not be invalid");

      const runtimePageBuffer &rpb = buffer.getRuntimeBuffer();
      strictBuffer pageBuffer;
      const btreeNodePageHead *head = NULL;
      const globalCollectionId &gcid = _context->getMbContext()->getGlobalId();

      rc = buffer.validatePage(PAGE_TYPE_BTREE_NODE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to validate btree page:[%s], rc:%d",
                rpb.getGlobalPid().toString().c_str(), rc);
         goto error;
      }

      pageBuffer = buffer.getReadableBodyBuffer();
      head = pageBuffer.getReadableObjPtr<btreeNodePageHead>(0);
      if (NULL == head)
      {
         PD_LOG(PDERROR, "failed to get btree page head");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (gcid.getCLLid() != head->clLogicalID)
      {
         PD_LOG(PDERROR, "different logical clids found[%d,%d] on page[%s]",
                gcid.getCLLid(), head->clLogicalID,
                rpb.getGlobalPid().toString().c_str());
         rc = SDB_VESSEL_PAGE_HEAD_NOT_MATCH;
         goto error;
      }

      if (_ic->getLogicalIndexId() != head->indexId)
      {
         PD_LOG(PDERROR, "different logical index ids found[%d,%d] on page[%s]",
                _ic->getLogicalIndexId(), head->indexId, rpb.getGlobalPid().toString().c_str());
         rc = SDB_VESSEL_PAGE_HEAD_NOT_MATCH;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }
} // namespace vessel

} // namespace engine
