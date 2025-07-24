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

   Source File Name = btreeAccessContext.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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
         virtual INT32 allocateNewNode(UINT32 depth,
                                       const btreeNodePageHead &header,
                                       BTREE_NODE_UPTR &node) override;

         virtual void destroyNode(BTREE_NODE_UPTR &node) override;

         virtual void statsInsertCompressedIndex(UINT32 optimizedSize, UINT32 remainSize) override;
         virtual void statsInsertUncompressedIndex(UINT32 size) override;
         virtual void statsRemoveCompressedIndex(UINT32 optimizedSize, UINT32 remainSize) override;
         virtual void statsRemoveUncompressedIndex(UINT32 size) override;
         virtual void statsRemoveMarkedDeletedIndexes(UINT32 num) override;
         virtual void statsReleaseMarkedDeletedIndexSpace(UINT32 size) override;
         virtual void statsRecompress(UINT32 newCompressedEntryNum,
                                      UINT64 newRealTotalEntrySize,
                                      UINT32 newPrefixNum,
                                      UINT32 oldCompressedEntryNum,
                                      UINT64 oldRealTotalEntrySize,
                                      UINT32 oldPrefixNum) override;
         virtual void statsAddLeafNode(BOOLEAN isCompressed) override;
         virtual void statsRemoveLeafNode(BOOLEAN isCompressed) override;
         virtual void statsAddNonLeafNode() override;
         virtual void statsRemoveNonLeafNode() override;
         virtual void statsAllocateNode() override;
         virtual void statsDestroyNode() override;
         virtual void statsCreateNewRoot() override;
         virtual void statsRefillChildNode() override;
         virtual void statsTruncateTree() override;
         virtual void statsTruncate(UINT32 newTotalEntryNum,
                                    UINT32 newCompressedEntryNum,
                                    UINT32 oldTotalEntryNum,
                                    UINT32 oldCompressedEntryNum,
                                    UINT64 origTotalEntrySizeDiff,
                                    UINT64 realTotalEntrySizeDiff) override;
         virtual void statsCompact(UINT64 realTotalEntrySizeDiff,
                                   UINT32 newPrefixNum,
                                   UINT32 oldPrefixNum) override;
         virtual void statsSplit(UINT32 newTotalEntryNum,
                                 UINT32 newCompressedEntryNum,
                                 UINT64 newOrigTotalEntrySize,
                                 UINT64 newRealTotalEntrySize,
                                 UINT32 prefixNum) override;

      public:
         INT32 init(requestContext *context,
                    indexSpace *is,
                    indexObject *obj,
                    spacePteAccessCtx *actx = nullptr);

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