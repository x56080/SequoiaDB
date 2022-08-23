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

   Source File Name = btreeNode.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/btreeNode.h"
#include "ossErr.h"
#include "pd.hpp"
#include "vessel/requestContext.h"
#include "vessel/indexObject.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"
#include "vessel/orderingWrapper.h"
#include "vessel/indexSpace.h"
#include "vessel/btreeNodePageIniter.h"
#include "vessel/logicalPageBuffer.h"
#include "vessel/memoryBlock.h"
#include "ossMemPool.hpp"
#include "vessel/btreeAccessContext.h"
#include "vessel/indexUtils.h"
#include "vessel/prefixGenerator.h"
#include <iterator>

namespace engine
{
namespace vessel
{
   btreeNode::btreeNode(UINT32 depth, btreeAccessContext *ctx):
   _ctx(ctx)
   {
      SDB_ASSERT(nullptr != _ctx, "can not be invalid");
      _lbuffer = _ctx->getBuffer(depth);
      SDB_ASSERT(nullptr != _lbuffer, "can not be invalid");
      btreeNodeBase::_nodeId = _lbuffer->getLogicalPid();
      btreeNodeBase::_depth = depth;
      btreeNodeBase::_buffer = _lbuffer->getReadableBodyBuffer();
   }

   btreeNode::btreeNode(UINT32 depth,
                        std::shared_ptr<logicalPageBuffer> &&buffer):
   _lbuffer(buffer.get()),
   _bufferOwner(std::move(buffer))
   {
      SDB_ASSERT(nullptr != _lbuffer, "can not be invalid");
      btreeNodeBase::_nodeId = _lbuffer->getLogicalPid();
      btreeNodeBase::_depth = depth;
      btreeNodeBase::_buffer = _lbuffer->getReadableBodyBuffer();
   }

   void btreeNode::reset()
   {
      btreeNodeBase::_reset();
      _lbuffer = nullptr;
      _ctx = nullptr;
      _bufferOwner.reset();
      return;
   }

   btreeContext *btreeNode::_getTreeCtx()
   {
      return static_cast<btreeContext *>(_ctx);
   }

   INT32 btreeNode::_makeBufferWritable()
   {
      INT32 rc = SDB_OK;
      
      if (_buffer.isWritable())
      {
         goto done;
      }
      else if (nullptr == _lbuffer)
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else
      {
         strictBuffer buffer;
         rc = _lbuffer->autoGetWritableBodyBuffer(buffer);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to make buffer writable:%d", rc);
            goto error;
         }

         _buffer.makeWritable(buffer.getSize(), buffer.getWPtr());
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   void btreeNode::commit(UINT64 lsn)
   {
      SDB_ASSERT(nullptr != _lbuffer && _lbuffer->isWritable(), "can not be invalid");
      _lbuffer->commit(lsn);
   }
} // namespace vessel

} // namespace engine

