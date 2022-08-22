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

#include "vessel/btreeContext.h"
#include "vessel/btreeAccessPathNode.h"
#include "vessel/btreeNode.h"
#include "vessel/logicalPageBuffer.h"
#include "vessel/btreeStatistics.h"
#include "vessel/logicalPageBufferPte.h"

namespace engine
{
namespace vessel
{
   class indexObject;
   class pageInitializer;
   class indexSpace;
   class spacePteAccessCtx;

   class btreeAccessContext : public btreeContext
   {
      public:
         btreeAccessContext(){}
         ~btreeAccessContext();
         btreeAccessContext(const btreeAccessContext &) = delete;
         btreeAccessContext &operator=(const btreeAccessContext &) = delete;

      public:
         OSS_INLINE BOOLEAN isValid()const {return nullptr != _obj;}
         OSS_INLINE const indexObject *getIndexObject() const {return _obj;}
         OSS_INLINE indexObject *getIndexObject() {return _obj;}
         OSS_INLINE BOOLEAN isPathEmpty()const {return _path.empty();}
         OSS_INLINE BOOLEAN isWritable()const {return nullptr != _actx;}
         OSS_INLINE BOOLEAN hasBtreeRoot()const {return INVALID_PAGE_ID != _btreeRoot;}
         OSS_INLINE PAGE_ID getBtreeRoot()const {return _btreeRoot;}
         OSS_INLINE indexSpace *getIndexSpace() {return _is;}
         OSS_INLINE requestContext *getReqCtx() {return _context;}
         OSS_INLINE UINT32 getTransferTick() const {return _transferTick;}
         OSS_INLINE UINT32 incTransferTick() {return ++_transferTick;}
         OSS_INLINE const btreeStatistics &getStats()const {return _stats;}

      public:
         INT32 init(requestContext *context,
                    indexSpace *is,
                    indexObject *obj,
                    spacePteAccessCtx *actx=nullptr);
                   
         void reset();

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

         INT32 allocateNewNode(pageInitializer *initer,
                               LPAGE_BUFFER_UPTR &buffer);

         INT32 destroyNode(LPAGE_BUFFER_UPTR &buffer);

         INT32 destroyPathEnd();

         void resetBtreeRoot(PAGE_ID root);

         void resetBtreeStats();

         void exportPathCoding(ossPoolVector<UINT64> &cv)const;

      private:
         INT32 _loadEntryPage();
         INT32 _pushIntoPath(PAGE_ID lpid);
         INT32 _validateBtreeNode(const logicalPageBuffer &buffer)const;
         INT32 _getPageBuffer(PAGE_ID lpid, logicalPageBufferPte &buffer);
      private:
         using _BTREE_PATH = ossPoolVector<btreeAccessPathNode>;

      private:
         indexObject *_obj = nullptr;
         indexSpace *_is = nullptr;
         requestContext *_context = nullptr;
         spacePteAccessCtx *_actx = nullptr;
         _BTREE_PATH _path;
         PAGE_ID _btreeRoot = INVALID_PAGE_ID;
         UINT32 _transferTick = 0;
         btreeStatistics _stats;
   };//class btreeAccessContextbt
} // namespace vessel

} // namespace engine


#endif//VESSEL_BTREE_ACCESS_CONTEXT_H_