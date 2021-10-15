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

   btreeNodePath::btreeNodePath(const indexContext *ic):
   _ic(ic),
   _size(0)
   {
      SDB_ASSERT(NULL != _ic && _ic->isValid(), "can not be invalid");
   }

   btreeNodePath::~btreeNodePath()
   {
      fini();
   }

   void btreeNodePath::fini()
   {
      clearPath();
      _ic = NULL;
      for (_FREE_BUFFERS::const_iterator itr = _free.begin();
           itr != _free.end(); ++itr)
      {
         SDB_OSS_DEL (*itr);
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
            _free.push_back(pn._lpb);
         }
         pn.fini();
      }
      _dynamicNodes.clear();
      _size = 0;
      
      return;
   }

   INT32 btreeNodePath::push(logicalPageBuffer *buffer,
                             btreeNode *out)
   {
      INT32 rc = SDB_OK;
      const btreeNodePageHead *head = NULL;

      if (OSS_UNLIKELY(NULL == head ||
                       !head->isValid() ||
                       NULL == buffer ||
                       !buffer->isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      head = buffer->getRuntimeBuffer().getReadablePtrOfBody<btreeNodePageHead>(0);
      SDB_ASSERT(NULL != head && head->isValid(), "can not be invalid");

      if (_size < _DEFAULT_CAPACITY)
      {
         _staticNodes[_size] = _pathNode(buffer, head->splitedTimes);
      }
      else
      {
         _dynamicNodes.push_back(_pathNode(buffer, head->splitedTimes));
      }

      ++_size;

      if (NULL != out)
      {
         *out = btreeNode(buffer, _ic, _size);
      }

   done:
      return rc;
   error:
      goto done;
   }

   btreeNode btreeNodePath::getCurrentEndNode()const
   {
      SDB_ASSERT(!isEmpty(), "can not be empty");
      const _pathNode &pn = getPathNode(_size - 1);
      SDB_ASSERT(pn.isAccessing(), "must be accessing");
      return btreeNode(pn._lpb, _ic, _size - 1);
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
      if (NULL != buffer)
      {
         buffer->fini();
         _free.push_back(buffer);
      }
   }
} // namespace vessel

} // namespace engine
