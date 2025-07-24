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

   Source File Name = btreeAccessContext.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/btreeAccessContext.h"
#include "pdTrace.hpp"
#include "vessel/btreeNode.h"
#include "vessel/indexObject.h"
#include "vessel/logicalPageBuffer.h"
#include "vessel/indexSpace.h"
#include "vessel/requestContext.h"
#include "vessel/btreeEntryPageAccessor.h"
#include "vessel/pageInitializer.h"
#include "vessel/btreeNodePageIniter.h"
#include "utilSharedPtrMaker.hpp"

namespace engine
{
namespace vessel
{
   btreeAccessContext::~btreeAccessContext()
   {
      if (isValid())
      {
         reset();
      }
   }

   INT32 btreeAccessContext::init(requestContext *context,
                                  indexSpace *is,
                                  indexObject *obj,
                                  spacePteAccessCtx *actx)
   {
      INT32 rc = SDB_OK;
      btreeEntryAddr entryAddr;
      reset();

      if (OSS_UNLIKELY(nullptr == context ||
                       nullptr == is ||
                       !is->isOpen() ||
                       nullptr == obj ||
                       !obj->isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _obj = obj;
      _is = is;
      _context = context;
      _actx = actx;

      entryAddr = _obj->getBtreeEntryAddr();
      if (entryAddr.isValid() &&
          (isWritable() || INVALID_PAGE_ID != entryAddr.getVisiblePid(is->getPSN())))
      {
         rc = _loadEntryPage();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to load btree entry page:%d", rc);
            goto error;
         }
      }

   done:
      return rc;
   error:
      reset();
      goto done;
   }

   void btreeAccessContext::reset()
   {
      _path.clear();
      _obj = nullptr;
      _is = nullptr;
      _context = nullptr;
      _actx = nullptr;
      _btreeRoot = INVALID_PAGE_ID;
      _transferTick = 0;
      _stats.reset();
      return;
   }

   INT32 btreeAccessContext::pushRootIntoPath(btreeNode *node)
   {
      INT32 rc = SDB_OK;

      if (nullptr != node)
      {
         node->reset();
      }

      if (OSS_UNLIKELY(!isValid() ||
                       !hasBtreeRoot()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      _path.clear();

      rc = _pushIntoPath(_btreeRoot);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to push root into path:%d", rc);
         goto error;
      }

      if (nullptr != node)
      {
         *node = getEndNodeInPath();
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeAccessContext::pushChildNodeIntoPath(PAGE_ID lpid,
                                                   const btreePathFootprint &footprint,
                                                   btreeNode *node)
   {
      INT32 rc = SDB_OK;

      if (nullptr != node)
      {
         node->reset();
      }

      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_PAGE_ID == lpid ||
                            !footprint.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (_path.empty())
      {
         PD_LOG(PDERROR, "can not push child node into path with out root");
         rc = SDB_INVALID_OPERATION;
         goto error;
      }

      rc = _pushIntoPath(lpid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to push node into path:%d", rc);
         goto error;
      }

      _path[_path.size() - 2].resetChildFootprint(footprint);

      if (nullptr != node)
      {
         *node = getEndNodeInPath();
      }
   done:
      return rc;
   error:
      goto done;
   }

   btreePathFootprint btreeAccessContext::getEndNodeFootprint()const
   {
      if (OSS_LIKELY(1 < _path.size()))
      {
         return _path[_path.size() - 2].getChildFootprint();
      }
      else
      {
         SDB_ASSERT(FALSE, "invalid path");
         return btreePathFootprint();
      }
   }

   INT32 btreeAccessContext::_pushIntoPath(PAGE_ID lpid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      LPAGE_PTE_BUFFER_PTR buffer(SDB_OSS_NEW logicalPageBufferPte());
      if (OSS_UNLIKELY(!buffer))
      {
         PD_LOG(PDERROR, "failed to allocate mem.");
         rc = SDB_OOM;
         goto error;
      }

      rc = _getPageBuffer(lpid, *buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get buffer of page[%d], rc:%d",
                lpid, rc);
         goto error;
      }

      rc = _validateBtreeNode(*buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to validate btree node:%d", rc);
         goto error;
      }

      try
      {
         _path.emplace_back(std::move(buffer));
      }
      catch(const std::exception& e)
      {
         PD_LOG(PDERROR, "failed to push buffer into path:%s", e.what());
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   void btreeAccessContext::resetPath()
   {
      _path.clear();
   }

   void btreeAccessContext::popEnd()
   {
      popEnds(1);
      return;
   }

   void btreeAccessContext::popEnds(UINT32 n)
   {
      SDB_ASSERT(n <= _path.size(), "out of bound");
      if (n < _path.size())
      {
         for (UINT32 i = 0; i < n; ++i)
         {
            _path.pop_back();
         }
         _path.back().resetChildFootprint();
      }
      else
      {
         _path.clear();
      }
      
      return;
   }
   
   btreeNode btreeAccessContext::getEndNodeInPath()
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(!_path.empty(), "can not be empty");
      return _path.empty() ?
             btreeNode() : btreeNode(_path.size() - 1,
                                     this);
   }

   btreeNode btreeAccessContext::getNodeInPath(UINT32 depth)
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(depth < _path.size(), "can not be invalid");
      
      if (depth < _path.size())
      {
         return btreeNode(depth, this);
      }
      else
      {
         return btreeNode();
      }
   }

   logicalPageBuffer *btreeAccessContext::getBuffer(UINT32 depth)
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      if (OSS_LIKELY(depth < _path.size()))
      {
         return _path[depth].getPageBuffer();
      }
      else
      {
         SDB_ASSERT(FALSE, "out of bound");
         return nullptr;
      }
   }

   UINT32 btreeAccessContext::getPathSize()const
   {
      return _path.size();
   }

   const btreeAccessPathNode &btreeAccessContext::getPathNode(UINT32 depth)const
   {
      SDB_ASSERT(depth < _path.size(), "out of bound");
      return _path[depth];
   }

   INT32 btreeAccessContext::_validateBtreeNode(const logicalPageBuffer &buffer)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(buffer.isValid(), "can not be invalid");

      const runtimePageBuffer &rpb = buffer.getRuntimeBuffer();
      strictBuffer pageBuffer;
      const btreeNodePageHead *head = nullptr;

      rc = buffer.validatePage(PAGE_TYPE_BTREE_NODE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to validate btree page:[%d, %s], rc:%d",
                buffer.getLogicalPid(), rpb.getGlobalPid().toString().c_str(), rc);
         goto error;
      }

      pageBuffer = buffer.getReadableBodyBuffer();
      head = pageBuffer.getReadableObjPtr<btreeNodePageHead>(0);
      if (nullptr == head)
      {
         PD_LOG(PDERROR, "failed to get btree page head");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (_obj->getLogicalID() != head->indexId)
      {
         PD_LOG(PDERROR, "different logical index ids found[%d,%d] on page[%s]",
                _obj->getLogicalID(), 
                head->indexId, rpb.getGlobalPid().toString().c_str());
         rc = SDB_VESSEL_PAGE_HEAD_NOT_MATCH;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeAccessContext::_loadEntryPage()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_path.empty(), "must be empty");
      SDB_ASSERT(nullptr != _obj, "can not be invalid");
      logicalPageBufferPte entryBuffer;
      btreeEntryPageAccessor accessor(_obj->getLogicalID());
      btreeEntryAddr addr = _obj->getBtreeEntryAddr();
      rc = _getPageBuffer(addr.pid, entryBuffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get prior buffer of entry page[%d], rc:%d",
                addr.pid, rc);
         goto error;
      }

      rc = accessor.load(&entryBuffer, _btreeRoot, _transferTick, _stats);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to load btree info:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      _stats.reset();
      _btreeRoot = INVALID_PAGE_ID;
      goto done;
   }

   INT32 btreeAccessContext::allocateNewNode(pageInitializer *initer,
                                             LPAGE_BUFFER_UPTR &buffer)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != initer, "can not be invalid");
      PAGE_ID lpid = INVALID_PAGE_ID;

      buffer.reset();

      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (!isWritable())
      {
         PD_LOG(PDERROR, "context is not writable");
         rc = SDB_INVALID_OPERATION;
         goto error;
      }

      buffer.reset(SDB_OSS_NEW logicalPageBufferPte());
      if (OSS_UNLIKELY(!buffer))
      {
         PD_LOG(PDERROR, "failed to allocate mem.");
         rc = SDB_OOM;
         goto error;
      }

      rc = _is->allocatePtePage(_context, _actx, initer, lpid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate private page:%d", rc);
         goto error;
      }

      rc = _getPageBuffer(lpid, *(static_cast<logicalPageBufferPte*>(buffer.get())));
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page buffer of[%d], rc:%d", lpid, rc);
         goto error;
      }
      statsAllocateNode();
   done:
      return rc;
   error:
      buffer.reset();
      goto done;
   }

   INT32 btreeAccessContext::destroyNode(LPAGE_BUFFER_UPTR &buffer)
   {
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(!isWritable()))
      {
         rc = SDB_INVALID_OPERATION;
         goto error;
      }
      else if (!buffer)
      {
         goto done;
      }
      else if (buffer->isValid())
      {
         PAGE_ID lpid = buffer->getLogicalPid();
         buffer.reset();
         rc = _is->removePage(_context, _actx, lpid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to remove page[%d], rc:%d", lpid, rc);
            goto error;
         }
      }
      else
      {
         buffer.reset();
      }
      statsDestroyNode();
   done:
      return rc;
   error:
      goto done;
   }

   void btreeAccessContext::resetBtreeRoot(PAGE_ID root)
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      _btreeRoot = root;
   }

   void btreeAccessContext::resetBtreeStats()
   {
      _stats.reset();
   }

   INT32 btreeAccessContext::destroyPathEnd()
   {
      INT32 rc = SDB_OK;
      if (_path.empty())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else
      {
         btreeAccessPathNode &node = _path.back();
         rc = destroyNode(node.getBufferUptr());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to destroy node:%d", rc);
            goto error;
         }

         popEnd();
      }
   done:
      return rc;
   error:
      goto done;
   }

   void btreeAccessContext::exportPathCoding(ossPoolVector<UINT64> &cv)const
   {
      cv.clear();
      if (!_path.empty())
      {
         cv.reserve(_path.size());
         for (UINT32 i = 0; i < _path.size(); ++i)
         {
            cv.push_back(_path[i].encode());
         }
      }

      return;
   }

   INT32 btreeAccessContext::_getPageBuffer(PAGE_ID lpid, logicalPageBufferPte &buffer)
   {
      INT32 rc = SDB_OK;
      buffer.fini();

      if (isWritable())
      {
         rc = _is->getPageBuffer(_context, _actx, lpid, buffer);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get page[%d] buffer:%d", lpid, rc);
            goto error;
         }
      }
      else
      {
         rc = _is->getPublicPageBuffer(_context, lpid, buffer);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get page[%d] public buffer:%d", lpid, rc);
            goto error;
         }
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeAccessContext::allocateNewNode(UINT32 depth,
                                             const btreeNodePageHead &header,
                                             BTREE_NODE_UPTR &node)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (!isWritable())
      {
         PD_LOG(PDERROR, "context is not writable");
         rc = SDB_INVALID_OPERATION;
         goto error;
      }
      else
      {
         PAGE_ID lpid = INVALID_PAGE_ID;
         btreeNodePageSplitIniter initer;
         slice s(BTREE_NODE_PAGE_HEAD_SIZE, &header);
         initer.set(s);
         LPAGE_BUFFER_SPTR sptr = makeSharedPtrFromPool<logicalPageBufferPte>();
         if (!sptr)
         {
            PD_LOG(PDERROR, "failed to allocate mem.");
            rc = SDB_OOM;
            goto error;
         }
         
         rc = _is->allocatePtePage(_context, _actx, &initer, lpid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to allocate new page:%d", rc);
            goto error;
         }

         rc = _getPageBuffer(lpid, *(static_cast<logicalPageBufferPte*>(sptr.get())));
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get page buffer:%d", rc);
            goto error;
         }

         
         node.reset(SDB_OSS_NEW btreeNode(depth, this, std::move(sptr)));
         if (OSS_UNLIKELY(!node))
         {
            /// page allocated will be removed by spacePteAccessCtx
            PD_LOG(PDERROR, "failed to allocate mem.");
            rc = SDB_OOM;
            goto error;
         }
         statsAllocateNode();
      }
   done:
      return rc;
   error:
      node.reset();
      goto done;
   }

   void btreeAccessContext::destroyNode(BTREE_NODE_UPTR &node)
   {
      SDB_ASSERT(isWritable(), "can not be invalid");
      if (nullptr != node)
      {
         PAGE_ID lpid = node->getNodeId();
         node.reset();
         INT32 rc = _is->removePage(_context, _actx, lpid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to remove page[%d], rc:%d", rc);
         }
         else
         {
            statsDestroyNode();
         }
      }
      return;
   }

   void btreeAccessContext::statsInsertCompressedIndex(UINT32 optimizedSize,
                                                       UINT32 remainSize)
   {
      _stats.insertCompressedIndex(optimizedSize, remainSize);
   }
   void btreeAccessContext::statsInsertUncompressedIndex(UINT32 size)
   {
      _stats.insertUncompressedIndex(size);
   }
   void btreeAccessContext::statsRemoveCompressedIndex(UINT32 optimizedSize,
                                                       UINT32 remainSize)
   {
      _stats.removeCompressedIndex(optimizedSize, remainSize);
   }
   void btreeAccessContext::statsRemoveUncompressedIndex(UINT32 size)
   {
      _stats.removeUncompressedIndex(size);
   }
   void btreeAccessContext::statsRemoveMarkedDeletedIndexes(UINT32 num)
   {
      _stats.removeMarkedDeletedIndexes(num);
   }
   void btreeAccessContext::statsReleaseMarkedDeletedIndexSpace(UINT32 size)
   {
      _stats.releaseMarkedDeletedIndexSpace(size);
   }
   void btreeAccessContext::statsRecompress(UINT32 newCompressedEntryNum,
                                            UINT64 newRealTotalEntrySize,
                                            UINT32 newPrefixNum,
                                            UINT32 oldCompressedEntryNum,
                                            UINT64 oldRealTotalEntrySize,
                                            UINT32 oldPrefixNum)
   {
      _stats.recompress(newCompressedEntryNum,
                        newRealTotalEntrySize,
                        newPrefixNum,
                        oldCompressedEntryNum,
                        oldRealTotalEntrySize,
                        oldPrefixNum);
   }
   void btreeAccessContext::statsAddLeafNode(BOOLEAN isCompressed)
   {
      _stats.addLeafNode(isCompressed);
   }
   void btreeAccessContext::statsRemoveLeafNode(BOOLEAN isCompressed)
   {
      _stats.removeLeafNode(isCompressed);
   }

   void btreeAccessContext::statsAddNonLeafNode()
   {
      _stats.addNonLeafNode();
   }
   void btreeAccessContext::statsRemoveNonLeafNode()
   {
      _stats.removeNonLeafNode();
   }
   void btreeAccessContext::statsAllocateNode()
   {
      _stats.allocateNode();
   }
   void btreeAccessContext::statsDestroyNode()
   {
      _stats.destroyNode();
   }
   void btreeAccessContext::statsCreateNewRoot()
   {
      _stats.createNewRoot();
   }
   void btreeAccessContext::statsRefillChildNode()
   {
      _stats.refillChildNode();
   }
   void btreeAccessContext::statsTruncateTree()
   {
      _stats.truncateTree();
   }
   void btreeAccessContext::statsTruncate(UINT32 newTotalEntryNum,
                                          UINT32 newCompressedEntryNum,
                                          UINT32 oldTotalEntryNum,
                                          UINT32 oldCompressedEntryNum,
                                          UINT64 origTotalEntrySizeDiff,
                                          UINT64 realTotalEntrySizeDiff)
   {
      _stats.truncate(newTotalEntryNum,
                      newCompressedEntryNum,
                      oldTotalEntryNum,
                      oldCompressedEntryNum,
                      origTotalEntrySizeDiff,
                      realTotalEntrySizeDiff);
   }
   void btreeAccessContext::statsCompact(UINT64 realTotalEntrySizeDiff,
                                         UINT32 newPrefixNum,
                                         UINT32 oldPrefixNum)
   {
      _stats.compact(realTotalEntrySizeDiff, newPrefixNum, oldPrefixNum);
   }
   void btreeAccessContext::statsSplit(UINT32 newTotalEntryNum,
                                       UINT32 newCompressedEntryNum,
                                       UINT64 newOrigTotalEntrySize,
                                       UINT64 newRealTotalEntrySize,
                                       UINT32 prefixNum)
   {
      _stats.split(newTotalEntryNum,
                   newCompressedEntryNum,
                   newOrigTotalEntrySize,
                   newRealTotalEntrySize,
                   prefixNum);
   }
} // namespace vessel

} // namespace engine
