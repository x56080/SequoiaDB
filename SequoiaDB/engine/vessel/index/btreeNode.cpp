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

   Source File Name = btreeNode.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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
   btreeNode::btreeNode(UINT32 depth, btreeAccessContext *ctx)
       : btreeNodeBase(ctx->getBuffer(depth)->getLogicalPid(),
                       depth,
                       ctx,
                       ctx->getBuffer(depth)->getReadableBodyBuffer()),
         _lbuffer(ctx->getBuffer(depth))
   {
      SDB_ASSERT(nullptr != _ctx, "can not be invalid");
      SDB_ASSERT(nullptr != _lbuffer, "can not be invalid");
   }

   btreeNode::btreeNode(UINT32 depth,
                        btreeAccessContext *ctx,
                        std::shared_ptr<logicalPageBuffer> &&buffer)
       : btreeNodeBase(buffer.get()->getLogicalPid(),
                       depth,
                       ctx,
                       buffer.get()->getReadableBodyBuffer()),
         _lbuffer(buffer.get()), _bufferOwner(std::move(buffer))
   {
      SDB_ASSERT(nullptr != _lbuffer, "can not be invalid");
      SDB_ASSERT(nullptr != ctx, "can not be invalid");
   }

   void btreeNode::reset()
   {
      btreeNodeBase::_reset();
      _lbuffer = nullptr;
      _ctx = nullptr;
      _bufferOwner.reset();
      return;
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

