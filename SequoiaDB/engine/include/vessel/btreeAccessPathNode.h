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

   Source File Name = btreeAccessPathNode.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_BTREE_ACCESS_PATH_NODE_H_
#define VESSEL_BTREE_ACCESS_PATH_NODE_H_

#include "vessel/pageIdentifier.h"
#include "vessel/logicalPageBuffer.h"
#include "vessel/btreePathFootprint.h"
#include "pdTrace.hpp"

#include <memory>

namespace engine
{
namespace vessel
{
   class btreeAccessPathNode : public SDBObject
   {
      public:
         btreeAccessPathNode() = default;
         ~btreeAccessPathNode() = default;
         explicit btreeAccessPathNode(std::unique_ptr<logicalPageBuffer> &&ptr):
         _lpb(std::move(ptr))
         {
            SDB_ASSERT(nullptr != _lpb.get() && _lpb->isValid(), "can not be invalid");
         }
         btreeAccessPathNode(btreeAccessPathNode &&o):
         _lpb(std::move(o._lpb)),
         _footprint(o._footprint)
         {
            o._footprint = btreePathFootprint();
         }

         btreeAccessPathNode &operator=(btreeAccessPathNode &&o)
         {
            _lpb = std::move(o._lpb);
            _footprint = o._footprint;
            o._footprint = btreePathFootprint();
            return *this;
         }
      public:
         
         OSS_INLINE logicalPageBuffer *getPageBuffer()
         {
            return _lpb.get();
         }

         OSS_INLINE const logicalPageBuffer *getPageBuffer()const
         {
            return _lpb.get();
         }

         OSS_INLINE std::unique_ptr<logicalPageBuffer> &getBufferUptr()
         {
            return _lpb;
         }
      
         OSS_INLINE BOOLEAN isValid()const
         {
            return nullptr != _lpb;
         }
         OSS_INLINE const btreePathFootprint &getChildFootprint()const
         {
            return _footprint;
         }
         void resetChildFootprint(const btreePathFootprint &fp=btreePathFootprint())
         {
            SDB_ASSERT(isValid(), "can not be invalid");
            _footprint = fp;
            return;
         }

         UINT64 encode()const
         {
            UINT64 c = _footprint.encode();
            c <<= 32;
            c |= (_lpb ? _lpb->getLogicalPid() : INVALID_PAGE_ID);
            return c;
         }

         static void decode(UINT64 c, PAGE_ID &addr, btreePathFootprint &fp)
         {
            addr = c;
            fp.decodeFrom(c >> 32);
            return;
         }
         
      private:
         std::unique_ptr<logicalPageBuffer> _lpb;
         btreePathFootprint _footprint;
   };//btreeAccessPathNode
} // namespace vessel

} // namespace engine


#endif//VESSEL_BTREE_ACCESS_PATH_NODE_H_