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
#include "vessel/logicalPageBuffer.h"
#include "pdTrace.hpp"
#include "vessel/indexContext.h"

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

   void btreeAccessContext::init(const indexContext *ic,
                                 const ixmKey &key,
                                 const recordID *rid,
                                 const DPS_TRANS_ID *transID)
   {
      SDB_ASSERT(NULL != ic && ic->isValid(), "can not be invalid");
      SDB_ASSERT(key.isValid(), "can not be invalid");
      fini();
      _ic = ic;
      _key.assign(key.data());
      if (NULL != rid && rid->valid())
      {
         _rid = *rid;
      }
      if (NULL != transID && transID->isValid())
      {
         _transID = *transID;
      }
      return;
   }

   void btreeAccessContext::fini()
   {
      _ic = NULL;
      _key.assign(NULL);
      _rid = recordID();
      _transID = DPS_TRANS_ID();
      for (UINT32 i = 0; i < _path.size(); ++i)
      {
         btreeAccessPathNode &pn = _path[i];
         if (pn.isAccessing())
         {
            pn.getPageBuffer()->fini();
         }
      }
      _path.clear();

      for (_FREE_BUFFERS::const_iterator itr = _free.begin();
           itr != _free.end(); ++itr)
      {
         SDB_OSS_DEL (*itr);
      }
      _free.clear();
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

   void btreeAccessContext::endToAccessPathNodes(UINT32 minActiveCount)
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(minActiveCount <= _path.size(), "out of bound");
      UINT32 max = _path.size() - minActiveCount;
      for (UINT32 i = 0; i < max; ++i)
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
      btreeAccessPathNode pn;
      if (_path.popBack(pn))
      {
         SDB_ASSERT(pn.isAccessing(), "impossible");
         if (pn.isAccessing())
         {
            pn.getPageBuffer()->fini();
            _free.push_back(pn.getPageBuffer());
         }
      }
      return;
   }

   btreeNode btreeAccessContext::getEndNodeInPath()
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(!_path.empty(), "can not be empty");
      UINT32 depth = _path.size() - 1;
      btreeAccessPathNode &pn = _path[depth];
      SDB_ASSERT(pn.isAccessing(), "end node should always be accessing");
      return btreeNode(pn.getPageBuffer(), _ic, depth);
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
            node = btreeNode(pn.getPageBuffer(), _ic, depth);
         }
      }
      return node;
   }

    UINT32 btreeAccessContext::getPathDepth()const
    {
       return _path.size();
    }
} // namespace vessel

} // namespace engine
