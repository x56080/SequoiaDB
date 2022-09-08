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

   Source File Name = btreeNode.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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