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
#include "vessel/indexObject.h"
#include "vessel/logicalPageBuffer.h"
#include "vessel/indexSpace.h"
#include "vessel/requestContext.h"
#include "vessel/btreeEntryPageAccessor.h"

namespace engine
{
namespace vessel
{
   btreeAccessContext::~btreeAccessContext()
   {
      if (isValid())
      {
         reset();
      }
   }

   INT32 btreeAccessContext::init(BOOLEAN nonpte,
                                  indexObject *obj,
                                  indexSpaceAccessCtx &&ctx)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != obj && obj->isValid(), "can not be invalid");
      SDB_ASSERT(ctx.isValid(), "can not be invalid");

      reset();

      if (OSS_UNLIKELY(nullptr == obj ||
                       !obj->isValid() ||
                       !ctx.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _nonpte = nonpte;
      _obj = obj;
      _ictx = std::move(ctx);

      if (_obj->hasBtreeEntryAddr())
      {
         rc = _cacheRootAndStats();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to load btree:%d", rc);
            goto error;
         }
      }

   done:
      return rc;
   error:
      if (_ictx.isValid())
      {
         ctx = std::move(_ictx);
      }
      reset();
      goto done;
   }

   void btreeAccessContext::reset()
   {
      _nonpte = TRUE;
      _path.clear();
      _obj = nullptr;
      _ictx.reset();
      _btreeRoot = INVALID_PAGE_ID;
      _stats.reset();
      return;
   }

   void btreeAccessContext::abort()
   {
      _nonpte = TRUE;
      _path.clear();
      _obj = nullptr;
      _ictx.abort();
      _btreeRoot = INVALID_PAGE_ID;
      _stats.reset();
      return;
   }

   INT32 btreeAccessContext::pushRootIntoPath(btreeNode *node)
   {
      INT32 rc = SDB_OK;

      if (nullptr != node)
      {
         node->reset();
      }

      if (OSS_UNLIKELY(!isValid() ||
                       !hasBtreeRoot()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      _path.clear();

      rc = _pushIntoPath(_btreeRoot);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to push root into path:%d", rc);
         goto error;
      }

      if (nullptr != node)
      {
         *node = getEndNodeInPath();
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeAccessContext::pushChildNodeIntoPath(PAGE_ID lpid,
                                                   const btreePathFootprint &footprint,
                                                   btreeNode *node)
   {
      INT32 rc = SDB_OK;

      if (nullptr != node)
      {
         node->reset();
      }

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

      rc = _pushIntoPath(lpid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to push node into path:%d", rc);
         goto error;
      }

      _path[_path.size() - 2].resetChildFootprint(footprint);

      if (nullptr != node)
      {
         *node = getEndNodeInPath();
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeAccessContext::_pushIntoPath(PAGE_ID lpid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      indexSpace *is = _ictx.getIndexSpace();
      std::unique_ptr<logicalPageBuffer> buffer(SDB_OSS_NEW logicalPageBuffer());
      if (OSS_UNLIKELY(!buffer))
      {
         PD_LOG(PDERROR, "failed to allocate mem.");
         rc = SDB_OOM;
         goto error;
      }

      rc = is->getLogicalPageBuffer(_ictx, lpid, !_nonpte, *buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get buffer of page[%d], rc:%d",
                lpid, rc);
         goto error;
      }

      rc = _validateBtreeNode(*buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to validate btree node:%d", rc);
         goto error;
      }

      try
      {
         _path.emplace_back(std::move(buffer));
      }
      catch(const std::exception& e)
      {
         PD_LOG(PDERROR, "failed to push buffer into path:%s", e.what());
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   void btreeAccessContext::resetPath()
   {
      _path.clear();
   }

   void btreeAccessContext::popEnd()
   {
      popEnds(1);
      return;
   }

   void btreeAccessContext::popEnds(UINT32 n)
   {
      SDB_ASSERT(n <= _path.size(), "out of bound");
      if (n < _path.size())
      {
         for (UINT32 i = 0; i < n; ++i)
         {
            _path.pop_back();
         }
         _path.back().resetChildFootprint();
      }
      else
      {
         _path.clear();
      }
      
      return;
   }
   
   btreeNode btreeAccessContext::getEndNodeInPath()
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(!_path.empty(), "can not be empty");
      return _path.empty() ?
             btreeNode() : btreeNode((INT32)_path.size() - 1,
                                     _path.back().getPageBuffer(),
                                     this);
   }

   btreeNode btreeAccessContext::getNodeInPath(UINT32 depth)
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(depth < _path.size(), "can not be invalid");
      
      if (depth < _path.size())
      {
         return btreeNode(depth,
                          _path[depth].getPageBuffer(),
                          this);
      }
      else
      {
         return btreeNode();
      }
   }

   UINT32 btreeAccessContext::getPathSize()const
   {
      return _path.size();
   }

   const btreeAccessPathNode &btreeAccessContext::getPathNode(UINT32 depth)const
   {
      SDB_ASSERT(depth < _path.size(), "out of bound");
      return _path[depth];
   }

   INT32 btreeAccessContext::_validateBtreeNode(const logicalPageBuffer &buffer)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(buffer.isValid(), "can not be invalid");

      const runtimePageBuffer &rpb = buffer.getRuntimeBuffer();
      strictBuffer pageBuffer;
      const btreeNodePageHead *head = nullptr;

      rc = buffer.validatePage(PAGE_TYPE_BTREE_NODE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to validate btree page:[%s], rc:%d",
                rpb.getGlobalPid().toString().c_str(), rc);
         goto error;
      }

      pageBuffer = buffer.getReadableBodyBuffer();
      head = pageBuffer.getReadableObjPtr<btreeNodePageHead>(0);
      if (nullptr == head)
      {
         PD_LOG(PDERROR, "failed to get btree page head");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (_obj->getLogicalID() != head->indexId)
      {
         PD_LOG(PDERROR, "different logical index ids found[%d,%d] on page[%s]",
                _obj->getLogicalID(), 
                head->indexId, rpb.getGlobalPid().toString().c_str());
         rc = SDB_VESSEL_PAGE_HEAD_NOT_MATCH;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeAccessContext::_cacheRootAndStats()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_path.empty(), "must be empty");
      SDB_ASSERT(nullptr != _obj && _obj->hasBtreeEntryAddr(), "can not be invalid");
      logicalPageBuffer entryBuffer;
      indexSpace *is = _ictx.getIndexSpace();
      btreeEntryPageAccessor accessor(_obj->getLogicalID());

      rc = is->getLogicalPageBuffer(_ictx, _obj->getBtreeEntryAddr(),
                                    !_nonpte, entryBuffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get prior buffer of entry page[%d], rc:%d",
                  _obj->getBtreeEntryAddr(), rc);
         goto error;
      }

      rc = accessor.load(&entryBuffer, _btreeRoot, _stats);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to load btree info:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      _stats.reset();
      _btreeRoot = INVALID_PAGE_ID;
      goto done;
   }
} // namespace vessel

} // namespace engine
