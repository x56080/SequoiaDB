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

   Source File Name = largeObjectSpace.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/largeObjectSpace.h"
#include "pdTrace.hpp"
#include "ossLikely.hpp"
#include "vessel/threadContext.h"
#include "vessel/instanceEnv.h"
#include "vessel/storageFileMaintainer.h"
#include "vessel/requestContext.h"
#include "dmsLobDef.hpp"
#include "vessel/lobExtentMetaBlock.h"
#include "vessel/runtimeMbContext.h"
#include "vessel/lobcMetaBlockMapping.h"
#include "vessel/lobcLatchHelper.h"
#include "vessel/storageFileLoader.h"

namespace engine
{
namespace vessel
{
   constexpr PAGE_ID UBER_BLOCK_PID = 0;

   largeObjectSpace::largeObjectSpace(const storageUnitManifest *manifest):
   _manifest(manifest)
   {
      SDB_ASSERT(nullptr != _manifest, "can not be null");
   }

   largeObjectSpace::~largeObjectSpace()
   {
      _close();
   }

   INT32 largeObjectSpace::ensureCreated()
   {
      INT32 rc = SDB_OK;
      if (isOpen())
      {
         goto done;
      }

      rc = _create();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create lob space:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   void largeObjectSpace::close()
   {
      if (_allocator.isValid() &&
          _uberBlock.isValid() &&
          _allocator.peekSegmentCount() != _uberBlock.totalLobdSegments)
      {
         mmapPagePointer ptr;
         INT32 rc = _metaFile.getPagePtr(UBER_BLOCK_PID, ptr);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get block page:%d", rc);
         }
         else
         {
            strictBuffer buffer;
            buffer.makeWritable(_metaFile.getPageSize(), ptr.getBuf());
            buffer.getWritableObjPtr<lobmUberBlock>(0)->totalLobdSegments =
                                            _allocator.peekSegmentCount();
            _metaFile.fsync();
         }
      }
      _close();
   }

   void largeObjectSpace::_close()
   {
      _fcluster.close();
      _allocator.clear();
      _metaFile.close();
      _uberBlock.reset();
   }

   void largeObjectSpace::destroy()
   {
      if (isOpen())
      {
         _fcluster.destroy();
         _allocator.clear();
         _metaFile.destroy();
         _uberBlock.reset();
      }
   }

   INT32 largeObjectSpace::open(const storageFileLoader *loader)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!isOpen(), "can not be open");
      storageFileManifest fileManifest;

      if (OSS_UNLIKELY(nullptr == loader))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (nullptr == _manifest || !_manifest->isValid())
      {
         SDB_ASSERT(FALSE, "init manifest first");
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = _openLobmFile(loader);
      if (SDB_FNE == rc)
      {
         /// lob space may not be created yet.
         rc = SDB_OK;
         goto done;
      }
      else if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open meta file:%d", rc);
         goto error;
      }

      rc = _loadUberBlock(_uberBlock);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to load uber block:%d", rc);
         goto error;
      }

      fileManifest.sid = _manifest->sid;
      fileManifest.stype = SPACE_TYPE_LOB;
      fileManifest.ftype = FILE_TYPE_DATA_STORAGE;
      fileManifest.secretValue = _manifest->secretValue;
      fileManifest.args = _manifest->lobArgs;

      rc = _fcluster.open(fileManifest, 0, loader);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open file cluster:%d", rc);
         goto error;
      }

      if (_fcluster.getTotalSegmentCount() < _uberBlock.totalLobdSegments)
      {
         PD_LOG(PDERROR, "segment count in uber block[%d] does not match cluster[%d]",
                _uberBlock.totalLobdSegments, _fcluster.getTotalSegmentCount());
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      _allocator.init(_manifest->lobArgs.maxPageCountPerSeg,
                      MAX_LOB_CHUNK_SIZE / _manifest->lobArgs.pageSize);
      for (UINT32 i = 0; i < _uberBlock.totalLobdSegments; ++i)
      {
         strictBuffer smeBuffer;
         rc = getLobdSme(i, smeBuffer);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get sme of segment[%d], rc:%d", i, rc);
            goto error;
         }

         rc = _allocator.depositWithSme((UINT64 *)(smeBuffer.getWPtr()), FALSE);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to deposit segment[%d], rc:%d", i, rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      _close();
      goto done;
   }

   INT32 largeObjectSpace::insertLobChunk(requestContext *context,
                                          const lobChunkKey &key,
                                          UINT32 offset,
                                          const slice &data)
   {
      INT32 rc = SDB_OK;
      lextentDescriptor desc;
      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
      SDB_ASSERT(nullptr != tc, "can not be null");
      LOBC_LATCH_MAP::object locker;
      lobcLatchHelper lh;
      globalLobChunkKey glckey;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);

      if (OSS_UNLIKELY(nullptr == context ||
                       !context->isMbContextAttached() ||
                       !key.isValid() ||
                       !data.isValid() ||
                       MAX_LOB_CHUNK_SIZE < (offset + data.getSize())))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      SDB_ASSERT(context->getSpaceID() == _manifest->sid, "must be same");
      glckey.set(_manifest->sid, context->getMBID(), key);
      rc = lh.lock(glckey, mode, locker);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to lock lobc[%s], rc:%d",
                glckey.toString().c_str(), rc);
         goto error;
      }

      rc = reserveExtent(offset + data.getSize(), desc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to reserve extent:%d", rc);
         goto error;
      }
      else
      {
         lobChunkBufferPool &pool = tc->getEnv()->lobcBufferPool;
         lobChunkBufferPool::writeOptions wopts;
         wopts.commitMetaData = TRUE;
         lobcExtentChain chain;
         lobExtentMetaBlock block;
         block.init(context->getLogicalClId(),
                    context->getMBID(),
                    key, desc, 0, TRUE);
         lobcMetaBlockMapping mapping(_manifest, _uberBlock.bucketEntryPid, &_metaFile);
         rc = mapping.insert(&block);
         if (SDB_OK != rc)
         {
            goto error;
         }

         chain.init(_manifest->lobArgs.pageSize);
         chain.pushBack(desc);
         rc = pool.write(glckey, chain, offset, data, wopts);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to write buffer pool:%d", rc);
            lobChunkSearchEntry entry(key, context->getLogicalClId(), 0);
            INT32 tmp = mapping.remove(entry, nullptr);
            if (SDB_OK != tmp)
            {
               PD_LOG(PDSEVERE, "failed to rollback entry[%s], rc:%d",
                      glckey.toString().c_str(), rc);
               desc.reset();/// failed to rollback meta data, just leave it there.
            }
            goto error;
         }
      }
   done:
      if (locker.isValid())
      {
         lh.unlock(mode, locker);
      }
      return rc;
   error:
      if (desc.isValid())
      {
         _allocator.release(desc.pid, desc.pcnt);
      }
      goto done;
   }

   INT32 largeObjectSpace::_create()
   {
      INT32 rc = SDB_OK;
      storageFileManifest fileManifest;

      std::unique_lock<std::mutex> guard(_mutex);
      if (isOpen())
      {
         goto done;
      }
      else if (nullptr == _manifest || !_manifest->isValid())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = _createLobmFile(_manifest->sid, _manifest->secretValue);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create lobm file:%d", rc);
         goto error;
      }

      _allocator.init(_manifest->lobArgs.maxPageCountPerSeg,
                      MAX_LOB_CHUNK_SIZE / _manifest->lobArgs.pageSize);

      rc = _initUberBlock();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init uber block:%d", rc);
         goto error;
      }

      fileManifest.sid = _manifest->sid;
      fileManifest.stype = SPACE_TYPE_LOB;
      fileManifest.ftype = FILE_TYPE_DATA_STORAGE;
      fileManifest.secretValue = _manifest->secretValue;
      fileManifest.args = _manifest->lobArgs;
      rc = _fcluster.open(fileManifest, 0, nullptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open file cluster:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      destroy();
      goto done;
   }

   INT32 largeObjectSpace::_initUberBlock()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_metaFile.isOpen(), "can not be invalid");

      lobmUberBlock *uberBlock = nullptr;
      {
         strictBuffer buffer;
         PAGE_ID pid = INVALID_PAGE_ID;
         mmapPagePointer ptr;
         rc = _metaFile.reservePage(pid, ptr);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to reserve uber block page:%d", rc);
            goto error;
         }
         else if (UBER_BLOCK_PID != pid)
         {
            PD_LOG(PDERROR, "wrong pid[%d] of uber block page", pid);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         buffer.makeWritable(_metaFile.getPageSize(), ptr.getBuf());
         buffer.setBuffer(0x00);
         uberBlock = buffer.getWritableObjPtr<lobmUberBlock>(0);
      }

      /// lobd sme entry
      {
         strictBuffer buffer;
         PAGE_ID pid = INVALID_PAGE_ID;
         mmapPagePointer ptr;
         rc = _metaFile.reservePage(pid, ptr);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to reserve uber block page:%d", rc);
            goto error;
         }
         buffer.makeWritable(_metaFile.getPageSize(), ptr.getBuf());
         buffer.setBuffer(0xFF);
         uberBlock->lobdSmeEntryPid = pid;
         uberBlock->totalLobdSegments = 0;
      }

      /// bucket entry
      {
         strictBuffer buffer;
         PAGE_ID pid = INVALID_PAGE_ID;
         mmapPagePointer ptr;
         rc = _metaFile.reservePage(pid, ptr);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to reserve uber block page:%d", rc);
            goto error;
         }
         buffer.makeWritable(_metaFile.getPageSize(), ptr.getBuf());
         buffer.setBuffer(0xFF);
         uberBlock->bucketEntryPid = pid;
      }

      uberBlock->version = LOBM_UBER_BLOCK_VERSION;
      _uberBlock = *uberBlock;

   done:
      return rc;
   error:
      goto done;
   }

   INT32 largeObjectSpace::_loadUberBlock(lobmUberBlock &block)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_metaFile.isOpen(), "invalid meta file");
      block.reset();

      strictBuffer buffer;
      mmapPagePointer ptr;
      rc = _metaFile.getPagePtr(UBER_BLOCK_PID, ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get uber block from file:%d", rc);
         goto error;
      }

      buffer.reset(_metaFile.getPageSize(), ptr.getBuf());
      block = *(buffer.getReadableObjPtr<lobmUberBlock>(0));
      if (!block.isValid())
      {
         PD_LOG(PDERROR, "invalid uber block");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
   done:
      return rc;
   error:
      block.reset();
      goto done;
   }

   INT32 largeObjectSpace::_createLobmFile(SPACE_ID sid,
                                           UINT32 secretValue)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!_metaFile.isOpen(), "do not reinit");
      const storagePathOptions &po = GET_THREAD_CONTEXT()->getEnv()->options.path;
      storageFileMaintainer sfm(&po, sid);
      storageFileName fn;
      createStorageFileOptions options;
      storageCoreArgs args(lobMetaDataFile::PAGE_SIZE,
                           lobMetaDataFile::PAGE_COUNT_PER_SEG,
                           lobMetaDataFile::MAX_SEG_COUNT);

      if (!fn.build(FILE_TYPE_LOBM, SPACE_TYPE_LOB, 0))
      {
         PD_LOG(PDERROR, "failed to build file name");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      options.secretValue = secretValue;
      options.args = args;
      options.createAsTmpFile = TRUE;
      options.replaceWhenCreate = TRUE;
      options.flags = storageFileCtlFlag::MMAP_DATA_SEGMENT;

      rc = sfm.createStorageFile(fn, options, _metaFile);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create lobm file[%s], rc:%d", fn.getFileName(), rc);
         goto error;
      }

      rc = _metaFile.removeShadowSuffix();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to remove file's shadow suffix:%d", rc);
         goto error;
      }

      _metaFile.fsync();
   done:
      return rc;
   error:
      if (_metaFile.isOpen())
      {
         _metaFile.destroy();
      }
      goto done;
   }

   INT32 largeObjectSpace::_openLobmFile(const storageFileLoader *loader)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != _manifest && INVALID_SPACE_ID != _manifest->sid,
                "can not be invalid");
      SDB_ASSERT(!_metaFile.isOpen(), "already been open");
      SDB_ASSERT(nullptr != loader, "can not be null");
      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
      SDB_ASSERT(nullptr != tc, "can not be invalid");
      const storagePathOptions &po = GET_THREAD_CONTEXT()->getEnv()->options.path;
      storageFileMaintainer sfm(&po, _manifest->sid);
      UINT32 fileCtlFlags = storageFileCtlFlag::MMAP_DATA_SEGMENT;

      const STORAGE_FILE_NAME_LIST *fl = loader->getFileList(SPACE_TYPE_LOB,
                                                             FILE_TYPE_LOBM);
      if (nullptr == fl || fl->empty())
      {
         rc = SDB_FNE;
         goto error;
      }
      else
      {
         const storageFileName &fn = fl->front();
         SDB_ASSERT(fn.isValid(), "can not be invalid");
         SDB_ASSERT(SPACE_TYPE_LOB == fn.getSpaceType(), "must be lob");
         SDB_ASSERT(FILE_TYPE_LOBM == fn.getFileType(), "must be lobm file");
         SDB_ASSERT(!fn.hasShadowSuffix(), "impossible");

         rc = sfm.openStorageFile(fn, fileCtlFlags, _metaFile);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to open lobm file:%d", rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 largeObjectSpace::reserveExtent(UINT32 size, lextentDescriptor &desc)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be invalid");
      SDB_ASSERT(0 < size && size <= MAX_LOB_CHUNK_SIZE, "can not be invalid");

      PAGE_ID pid = INVALID_PAGE_ID;
      UINT32 pcnt = ossAlignX(size, getLobdPageSize()) / getLobdPageSize();
      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
      SDB_ASSERT(nullptr != tc, "can not be invalid");
      PAGE_SNAPSHOT_VERION psv = tc->getEnv()->dms.getOnlinePageSnapshotVersion();
      desc.reset();

      do
      {
         UINT32 segmentCount = 0;
         pid = _allocator.reserve(pcnt, &segmentCount);
         if (INVALID_PAGE_ID != pid)
         {
            break;
         }
         else
         {
            strictBuffer smeBuffer;
            std::unique_lock<std::mutex> extendingLock(_mutex);
            if (segmentCount < _allocator.peekSegmentCount())
            {
               continue;
            }
            else if (_allocator.peekSegmentCount() < _fcluster.getTotalSegmentCount())
            {
               rc = ensureLobdSme(_allocator.peekSegmentCount(), smeBuffer);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to ensure lobd sme[%d]:%d",
                         _allocator.peekSegmentCount(), rc);
                  goto error;
               }
            }
            else
            {
               SDB_ASSERT(_allocator.peekSegmentCount() == _fcluster.getTotalSegmentCount(), "impossible");
               rc = extendNewLobdSegment(smeBuffer);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to extend new segment:%d", rc);
                  goto error;
               }
            }

            SDB_ASSERT(smeBuffer.isWritable(), "must be writable");
            smeBuffer.setBuffer(0xFF);
            rc = _allocator.depositWithSme(smeBuffer.getWritableObjPtr<UINT64>(0), TRUE);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to deposit segment:%d", rc);
               goto error;
            }

            continue;
         }
      } while (TRUE);

      SDB_ASSERT(INVALID_PAGE_ID != pid, "can not be invalid");
      desc.pcnt = pcnt;
      desc.pid = pid;
      desc.psv = psv;
      desc.size = size;
      
   done:
      return rc;
   error:
      goto done;
   }

   UINT32 largeObjectSpace::getLobdPageSize()const
   {
      SDB_ASSERT(nullptr != _manifest && _manifest->isValid(), "can not be invalid");
      return _manifest->lobArgs.pageSize;
   }

   UINT32 largeObjectSpace::getLobdSmePageCapacity()const
   {
      return _metaFile.getPageSize() / getLobdSmeSize();
   }

   UINT32 largeObjectSpace::getLobdSmeSize()const
   {
      return _manifest->lobArgs.maxPageCountPerSeg >> 3;
   }

   INT32 largeObjectSpace::extendNewLobdSegment(strictBuffer &smeBuffer)
   {
      INT32 rc = SDB_OK;
      rc = ensureLobdSme(_fcluster.getTotalSegmentCount(), smeBuffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to ensure lobd sme:%d", rc);
         goto error;
      }

      rc = _fcluster.ensureSegmentCount(_fcluster.getTotalSegmentCount() + 1);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to extend file cluster:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      smeBuffer.reset();
      goto done;
   }

   INT32 largeObjectSpace::ensureLobdSme(UINT32 segmentId, strictBuffer &buffer)
   {
      INT32 rc = SDB_OK;
      mmapPagePointer ptr;
      UINT32 capacity = getLobdSmePageCapacity();
      UINT32 pidPos = segmentId / capacity;
      UINT32 offset = (segmentId % capacity) * getLobdSmeSize();
      strictBuffer pageBuffer;
      const PAGE_ID *pidPtr = nullptr;
      PAGE_ID pid = INVALID_PAGE_ID;
      PAGE_ID newPid = INVALID_PAGE_ID;
      buffer.reset();

      rc = _metaFile.getPagePtr(_uberBlock.lobdSmeEntryPid, ptr);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to get lobm page[%d] ptr:%d",
                _uberBlock.lobdSmeEntryPid);
         goto error;
      }

      pageBuffer.makeWritable(_metaFile.getPageSize(), ptr.getBuf());
      pidPtr = pageBuffer.getReadableObjPtr<PAGE_ID>(sizeof(PAGE_ID) * pidPos);
      if (OSS_UNLIKELY(nullptr == pidPtr))
      {
         PD_LOG(PDERROR, "failed to get pid ptr:%d", pidPos);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (INVALID_PAGE_ID == *pidPtr)
      {
         mmapPagePointer newPtr;
         rc = _metaFile.reservePage(newPid, newPtr);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to reserve new pid from mfile:%d", rc);
            goto error;
         }
         *(pageBuffer.getWritableObjPtr<PAGE_ID>(sizeof(PAGE_ID) * pidPos)) = newPid;
         pid = newPid;
      }
      else
      {
         pid = *pidPtr;
      }

      /// reuse ptr and pageBuffer.
      rc = _metaFile.getPagePtr(pid, ptr);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to get lobm page[%d] ptr, rc:%d", pid, rc);
         goto error;
      }

      pageBuffer.makeWritable(_metaFile.getPageSize(), ptr.getBuf());
      buffer = pageBuffer.getWritableBuffer(getLobdSmeSize(), offset);
      if (OSS_UNLIKELY(!buffer.isWritable()))
      {
         PD_LOG(PDERROR, "failed to get writable buffer");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 largeObjectSpace::getLobdSme(UINT32 segmentId, strictBuffer &buffer)
   {
      INT32 rc = SDB_OK;
      mmapPagePointer ptr;
      UINT32 capacity = getLobdSmePageCapacity();
      UINT32 pidPos = segmentId / capacity;
      UINT32 offset = (segmentId % capacity) * getLobdSmeSize();
      strictBuffer pageBuffer;
      const PAGE_ID *pidPtr = nullptr;

      buffer.reset();

      rc = _metaFile.getPagePtr(_uberBlock.lobdSmeEntryPid, ptr);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to get lobm page[%d] ptr:%d",
                _uberBlock.lobdSmeEntryPid);
         goto error;
      }

      pageBuffer.makeWritable(_metaFile.getPageSize(), ptr.getBuf());
      pidPtr = pageBuffer.getReadableObjPtr<PAGE_ID>(sizeof(PAGE_ID) * pidPos);
      if (OSS_UNLIKELY(nullptr == pidPtr))
      {
         PD_LOG(PDERROR, "failed to get pid ptr:%d", pidPos);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      else if (INVALID_PAGE_ID == *pidPtr)
      {
         PD_LOG(PDERROR, "invalid pid of segment[%d]", segmentId);
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      /// reuse ptr and pageBuffer.
      rc = _metaFile.getPagePtr(*pidPtr, ptr);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to get lobm page[%d] ptr, rc:%d", *pidPtr, rc);
         goto error;
      }

      pageBuffer.makeWritable(_metaFile.getPageSize(), ptr.getBuf());
      buffer = pageBuffer.getWritableBuffer(getLobdSmeSize(), offset);
      if (OSS_UNLIKELY(!buffer.isWritable()))
      {
         PD_LOG(PDERROR, "failed to get writable buffer");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

   done:
      return rc;
   error:
      buffer.reset();
      goto done;
   }

   INT32 largeObjectSpace::readLobChunk(requestContext *context,
                                        const lobChunkKey &key,
                                        UINT32 offset,
                                        UINT32 size,
                                        CHAR *buffer,
                                        UINT32 &readSize)
   {
      INT32 rc = SDB_OK;
      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
      SDB_ASSERT(nullptr != tc, "can not be null");
      LOBC_LATCH_MAP::object locker;
      lobcLatchHelper lh;
      globalLobChunkKey glckey;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_SHARED);
      readSize = 0;

      if (OSS_UNLIKELY(nullptr == context ||
                       !context->isMbContextAttached() ||
                       !key.isValid() ||
                       nullptr == buffer ||
                       MAX_LOB_CHUNK_SIZE < (offset + size)))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      SDB_ASSERT(context->getSpaceID() == _manifest->sid, "must be same");
      glckey.set(_manifest->sid, context->getMBID(), key);
      rc = lh.lock(glckey, mode, locker);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to lock lobc[%s], rc:%d",
                glckey.toString().c_str(), rc);
         goto error;
      }

      {
         UINT32 expectedSize = 0;
         lobChunkBufferPool &pool = tc->getEnv()->lobcBufferPool;
         lobChunkSearchEntry entry(key, context->getLogicalClId());
         lobcExtentChain chain;
         lobcMetaBlockMapping mapping(_manifest, _uberBlock.bucketEntryPid, &_metaFile);
         rc = mapping.find(entry, chain);
         if (SDB_OK != rc)
         {
            goto error;
         }

         if (chain.getChunkSize() <= offset)
         {
            goto done;
         }
         else if (chain.getChunkSize() < (offset + size))
         {
            expectedSize = chain.getChunkSize() - offset;
         }
         else
         {
            expectedSize = size;
         }

         rc = pool.read(glckey, chain, offset, expectedSize, buffer);
         if (SDB_OK != rc)
         {
            goto error;
         }

         readSize = expectedSize;
      }
   done:
      if (locker.isValid())
      {
         lh.unlock(mode, locker);
      }
      return rc;
   error:
      goto done;
   }
} // namespace vessel

} // namespace engine
