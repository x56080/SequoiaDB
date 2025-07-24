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

   Source File Name = btreeNode.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_BTREE_NODE_H_
#define VESSEL_BTREE_NODE_H_

#include "vessel/btreeNodeBase.h"

namespace engine
{
namespace vessel
{
   class logicalPageBuffer;
   class btreeAccessContext;

   class btreeNode : public btreeNodeBase
   {
      public:
         btreeNode() = default;
          ~btreeNode() = default;

         explicit btreeNode(UINT32 depth, btreeAccessContext *ctx);
         explicit btreeNode(UINT32 depth,
                            btreeAccessContext *ctx,
                            std::shared_ptr<logicalPageBuffer> &&buffer);

      public:
         OSS_INLINE BOOLEAN isManagedByCtx() const
         {
            return nullptr != _ctx;
         }

         void reset();

         /// node must be writable
         void commit(UINT64 lsn);

      protected:
         virtual INT32 _makeBufferWritable() override;

      private:
         logicalPageBuffer *_lbuffer = nullptr;
         std::shared_ptr<logicalPageBuffer> _bufferOwner;
   };//class btreeNode
} // namespace vessel

} // namespace engine


#endif//VESSEL_BTREE_NODE_H_