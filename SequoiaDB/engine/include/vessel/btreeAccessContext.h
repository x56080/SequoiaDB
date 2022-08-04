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

   Source File Name = btreeAccessContext.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_BTREE_ACCESS_CONTEXT_H_
#define VESSEL_BTREE_ACCESS_CONTEXT_H_

#include "vessel/btreeAccessPathNode.h"
#include "vessel/btreeNode.h"
#include "vessel/logicalPageBuffer.h"
#include "ossMemPool.hpp"
#include "vessel/indexSpaceAccessCtx.h"
#include "vessel/btreeStatistics.h"

namespace engine
{
namespace vessel
{
   class indexObject;
   class pageInitializer;

   class btreeAccessContext : public SDBObject
   {
      public:
         btreeAccessContext(){}
         ~btreeAccessContext();
         btreeAccessContext(const btreeAccessContext &) = delete;
         btreeAccessContext &operator=(const btreeAccessContext &) = delete;

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return nullptr != _obj;
         }

         OSS_INLINE indexObject *getIndexObject()
         {
            return _obj;
         }

         OSS_INLINE BOOLEAN isPathEmpty()const
         {
            return _path.empty();
         }

         OSS_INLINE indexSpaceAccessCtx &getSpaceCtx() {return _ictx;}

         OSS_INLINE BOOLEAN isNonPte()const {return _nonpte;}
         OSS_INLINE BOOLEAN hasBtreeRoot()const {return INVALID_PAGE_ID != _btreeRoot;}
         OSS_INLINE PAGE_ID getBtreeRoot()const {return _btreeRoot;}

      public:
         INT32 init(BOOLEAN nonpte,
                    indexObject *obj,
                    indexSpaceAccessCtx &&ctx);
                   
         void reset();

         void abort();

         /// ensure has btree root first.
         /// will reset whole path inside first.
         INT32 pushRootIntoPath(btreeNode *node=nullptr);

         INT32 pushChildNodeIntoPath(PAGE_ID lpid,
                                     const btreePathFootprint &footprint,
                                     btreeNode *node=nullptr);

         /// get info saved in end node's father.
         /// ensure path size is over 1
         btreePathFootprint getEndNodeFootprint()const;

         void resetPath();

         void popEnd();

         void popEnds(UINT32 n);

         btreeNode getEndNodeInPath();
         UINT32 getPathSize()const;
         btreeNode getNodeInPath(UINT32 depth);
         logicalPageBuffer *getBuffer(UINT32 depth);

         const btreeAccessPathNode &getPathNode(UINT32 depth)const;

         INT32 makeWritable(logicalPageBuffer &buffer);

         INT32 allocateNewNode(pageInitializer *initer,
                               logicalPageBuffer &buffer);

         INT32 destroyNode(logicalPageBuffer &buffer);

         INT32 destroyPathEnd();

         void resetBtreeRoot(PAGE_ID root);

         void resetBtreeStats();

      private:
         INT32 _cacheRootAndStats();
         INT32 _pushIntoPath(PAGE_ID lpid);
         INT32 _validateBtreeNode(const logicalPageBuffer &buffer)const;
      private:
         using _BTREE_PATH = ossPoolVector<btreeAccessPathNode>;

      private:
         BOOLEAN _nonpte = TRUE;
         indexObject *_obj = nullptr;
         indexSpaceAccessCtx _ictx;
         PAGE_ID _btreeRoot = INVALID_PAGE_ID;
         btreeStatistics _stats;
         _BTREE_PATH _path;
   };//class btreeAccessContextbt
} // namespace vessel

} // namespace engine


#endif//VESSEL_BTREE_ACCESS_CONTEXT_H_