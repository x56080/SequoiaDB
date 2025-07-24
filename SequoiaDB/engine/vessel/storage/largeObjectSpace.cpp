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

   Source File Name = largeObjectSpace.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/largeObjectSpace.h"
#include "pdTrace.hpp"
#include "ossLikely.hpp"
#include "vessel/threadContext.h"
#include "vessel/instanceEnv.h"
#include "vessel/storageFileMaintainer.h"
#include "vessel/requestContext.h"
#include "dmsLobDef.hpp"
#include "vessel/lobExtentMetaBlock.h"

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
      if (_metaFile.isOpen())
      {
          _metaFile.fsync();
      }
      _close();
   }

   void largeObjectSpace::_close()
   {
      _smgr.fini();
      _fcluster.close();
      _metaFile.close();
      _uberBlock.reset();
   }

   void largeObjectSpace::destroy()
   {
      if (isOpen())
      {
         _smgr.fini();
         _fcluster.destroy();
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

      fileManifest.sid = _manifest->id.getSpaceId();
      fileManifest.stype = SPACE_TYPE_LOB;
      fileManifest.ftype = FILE_TYPE_DATA_STORAGE;
      fileManifest.secretValue = _manifest->secretValue;
      fileManifest.args = _manifest->lobArgs;

      /// do not mmap
      rc = _fcluster.open(fileManifest, 0, loader);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open file cluster:%d", rc);
         goto error;
      }

      rc = _smgr.init(_uberBlock.smeEntryPid, &_metaFile, &_fcluster);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init space manager:%d", rc);
         goto error;
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
      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
      SDB_ASSERT(nullptr != tc, "can not be null");
      LOBC_LATCH_MAP::object locker;
      lobcLatchHelper lh;
      globalLobChunkKey glckey;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);

      if (OSS_UNLIKELY(nullptr == context ||
                       !context->isClPropertiesSet() ||
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

      SDB_ASSERT(context->getSpaceID() == _manifest->id.getSpaceId(), "must be same");
      glckey.set(_manifest->id.getSpaceId(), context->getMBID(), key);
      rc = lh.lock(glckey, mode, locker);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to lock lobc[%s], rc:%d",
                glckey.toString().c_str(), rc);
         goto error;
      }

      rc = _insertLobc(context, key, offset, data);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to insert lobc[%s], rc:%d", key.toString().c_str(), rc);
         goto error;
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

      rc = _createLobmFile(_manifest->id.getSpaceId(), _manifest->secretValue);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create lobm file:%d", rc);
         goto error;
      }

      rc = _initUberBlock();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init uber block:%d", rc);
         goto error;
      }

      fileManifest.sid = _manifest->id.getSpaceId();
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

      rc = _smgr.init(_uberBlock.smeEntryPid, &_metaFile, &_fcluster);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init space manager:%d", rc);
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
         rc = _metaFile.reservePid(pid, &ptr);
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
         rc = _metaFile.reservePid(pid, &ptr);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to reserve uber block page:%d", rc);
            goto error;
         }
         buffer.makeWritable(_metaFile.getPageSize(), ptr.getBuf());
         buffer.setBuffer(0xFF);
         uberBlock->smeEntryPid = pid;
      }

      /// bucket entry
      {
         strictBuffer buffer;
         PAGE_ID pid = INVALID_PAGE_ID;
         mmapPagePointer ptr;
         rc = _metaFile.reservePid(pid, &ptr);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to reserve uber block page:%d", rc);
            goto error;
         }
         buffer.makeWritable(_metaFile.getPageSize(), ptr.getBuf());
         buffer.setBuffer(0xFF);
         uberBlock->bucketEntryPid = pid;
      }

      uberBlock->version = lobmUberBlock::VERSION;
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
      SDB_ASSERT(nullptr != _manifest && INVALID_SPACE_ID != _manifest->id.getSpaceId(),
                "can not be invalid");
      SDB_ASSERT(!_metaFile.isOpen(), "already been open");
      SDB_ASSERT(nullptr != loader, "can not be null");
      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
      SDB_ASSERT(nullptr != tc, "can not be invalid");
      const storagePathOptions &po = GET_THREAD_CONTEXT()->getEnv()->options.path;
      storageFileMaintainer sfm(&po, _manifest->id.getSpaceId());
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
      rc = _smgr.reserveExtent(pcnt, pid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to reserve extent[%d]:%d", pcnt, rc);
         goto error;
      }

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

      rc = _fcluster.allocateNewSegment(1);
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

      rc = _metaFile.getPagePtr(_uberBlock.smeEntryPid, ptr);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to get lobm page[%d] ptr:%d",
                _uberBlock.smeEntryPid);
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
         rc = _metaFile.reservePid(newPid, &newPtr);
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

      rc = _metaFile.getPagePtr(_uberBlock.smeEntryPid, ptr);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to get lobm page[%d] ptr:%d",
               _uberBlock.smeEntryPid);
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
                       !context->isClPropertiesSet() ||
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

      SDB_ASSERT(context->getSpaceID() == _manifest->id.getSpaceId(), "must be same");
      glckey.set(_manifest->id.getSpaceId(), context->getMBID(), key);
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

   INT32 largeObjectSpace::removeLobChunk(requestContext *context,
                                          const lobChunkKey &key)
   {
      INT32 rc = SDB_OK;
      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
      SDB_ASSERT(nullptr != tc, "can not be null");
      LOBC_LATCH_MAP::object locker;
      lobcLatchHelper lh;
      globalLobChunkKey glckey;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);

      if (OSS_UNLIKELY(nullptr == context ||
                       !context->isClPropertiesSet() ||
                       !key.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      SDB_ASSERT(context->getSpaceID() == _manifest->id.getSpaceId(), "must be same");
      glckey.set(_manifest->id.getSpaceId(), context->getMBID(), key);
      rc = lh.lock(glckey, mode, locker);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to lock lobc[%s], rc:%d",
                glckey.toString().c_str(), rc);
         goto error;
      }

      {
         lobChunkBufferPool &pool = tc->getEnv()->lobcBufferPool;
         lobChunkSearchEntry entry(key, context->getLogicalClId());
         lobcExtentChain chain;
         lobcMetaBlockMapping mapping(_manifest, _uberBlock.bucketEntryPid, &_metaFile);

         rc = mapping.remove(entry, &chain);
         if (SDB_OK != rc)
         {
            goto error;
         }
         SDB_ASSERT(!chain.isEmpty(), "can not be empty");

         rc = pool.remove(glckey);
         if (SDB_OK != rc)
         {
            PD_LOG(PDSEVERE, "failed to remove lobc[%s] from buffer pool:%d",
                   glckey.toString().c_str(), rc);
            ossPanic();
            goto error;
         }

         /// actually, we are not sure if these pages are being flushed.
         /// but it does not matter, new writing will overwrite these pages
         /// at next flush job.
         for (UINT32 i = 0; i < chain.getChainSize(); ++i)
         {
            const lextentDescriptor &desc = chain.getChainItem(i);
            _smgr.releaseExtent(desc.pid, desc.pcnt);
         }
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

   INT32 largeObjectSpace::removeLobChunksInCL(requestContext *context)
   {
      INT32 rc = SDB_OK;
      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
      SDB_ASSERT(nullptr != tc, "can not be null");

      if (OSS_UNLIKELY(nullptr == context ||
                       !context->isClPropertiesSet()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      SDB_ASSERT(context->getSpaceID() == _manifest->id.getSpaceId(), "must be same");
      {
         tc->getEnv()->lobcBufferPool.discard(context->getSpaceID(), context->getMBID());
         lobcMetaBlockMapping mapping(_manifest, _uberBlock.bucketEntryPid, &_metaFile);
         mapping.truncate(context->getLogicalClId(), &_smgr);
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 largeObjectSpace::updateLobChunk(requestContext *context,
                                          const lobChunkKey &key,
                                          UINT32 offset,
                                          const slice &data,
                                          BOOLEAN createIfNotExists)
   {
      INT32 rc = SDB_OK;
      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
      SDB_ASSERT(nullptr != tc, "can not be null");
      LOBC_LATCH_MAP::object locker;
      lobcLatchHelper lh;
      globalLobChunkKey glckey;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);

      if (OSS_UNLIKELY(nullptr == context ||
                       !context->isClPropertiesSet() ||
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

      SDB_ASSERT(context->getSpaceID() == _manifest->id.getSpaceId(), "must be same");
      glckey.set(_manifest->id.getSpaceId(), context->getMBID(), key);
      rc = lh.lock(glckey, mode, locker);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to lock lobc[%s], rc:%d",
                glckey.toString().c_str(), rc);
         goto error;
      }

      {
         lobChunkSearchEntry entry(key, context->getLogicalClId());
         lobcMetaBlockMapping mapping(_manifest, _uberBlock.bucketEntryPid, &_metaFile);
         lobcExtentChain chain;
         rc = mapping.find(entry, chain);
         if (SDB_OK == rc)
         {
            rc = _updateLobc(context, entry, chain, offset ,data);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to update lobc[%s], rc:%d",
                      key.toString().c_str(), rc);
               goto error;
            }
         }
         else if (SDB_LOB_SEQUENCE_NOT_EXIST == rc)
         {
            if (!createIfNotExists)
            {
               goto error;
            }
            else
            {
               rc = _insertLobc(context, key, offset, data);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to upsert lobc[%s], rc:%d",
                         key.toString().c_str(), rc);
                  goto error;
               }
            }
         }
         else
         {
            PD_LOG(PDERROR, "failed to find lobc[%s], rc:%d",
                   key.toString().c_str(), rc);
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
      goto done;
   }

   INT32 largeObjectSpace::testLobChunk(requestContext *context,
                                        const lobChunkKey &key,
                                        dmsLobChunkProfile *profile)
   {
      INT32 rc = SDB_OK;
      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
      SDB_ASSERT(nullptr != tc, "can not be null");
      LOBC_LATCH_MAP::object locker;
      lobcLatchHelper lh;
      globalLobChunkKey glckey;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_SHARED);

      if (OSS_UNLIKELY(nullptr == context ||
                       !context->isClPropertiesSet() ||
                       !key.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      SDB_ASSERT(context->getSpaceID() == _manifest->id.getSpaceId(), "must be same");
      glckey.set(_manifest->id.getSpaceId(), context->getMBID(), key);
      rc = lh.lock(glckey, mode, locker);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to lock lobc[%s], rc:%d",
                glckey.toString().c_str(), rc);
         goto error;
      }

      {
         lobcExtentChain chain;
         lobChunkSearchEntry entry(key, context->getLogicalClId());
         lobcMetaBlockMapping mapping(_manifest, _uberBlock.bucketEntryPid, &_metaFile);
         rc = mapping.find(entry, chain);
         if (SDB_OK != rc)
         {
            goto error;
         }

         if (nullptr != profile)
         {
            profile->flags = 0;
            profile->chainSize = chain.getChainSize();
            profile->chunkSize = chain.getChunkSize();
         }
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

   INT32 largeObjectSpace::truncateLobChunk(requestContext *context,
                                            const lobChunkKey &key,
                                            UINT32 size,
                                            UINT32 &tsize)
   {
      INT32 rc = SDB_OK;
      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
      SDB_ASSERT(nullptr != tc, "can not be null");
      LOBC_LATCH_MAP::object locker;
      lobcLatchHelper lh;
      globalLobChunkKey glckey;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);

      if (OSS_UNLIKELY(nullptr == context ||
                       !context->isClPropertiesSet() ||
                       !key.isValid() ||
                       0 == size))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      SDB_ASSERT(context->getSpaceID() == _manifest->id.getSpaceId(), "must be same");
      glckey.set(_manifest->id.getSpaceId(), context->getMBID(), key);
      rc = lh.lock(glckey, mode, locker);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to lock lobc[%s], rc:%d",
                glckey.toString().c_str(), rc);
         goto error;
      }

      rc = _truncateLobc(context, key, size, tsize);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to truncate lob chunk:%d", rc);
         goto error;
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

   INT32 largeObjectSpace::_insertLobc(requestContext *context,
                                       const lobChunkKey &key,
                                       UINT32 offset,
                                       const slice &data)
   {
      INT32 rc = SDB_OK;
      lextentDescriptor desc;
      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
      SDB_ASSERT(nullptr != tc, "can not be null");
      lobChunkBufferPool &pool = tc->getEnv()->lobcBufferPool;
      lobChunkBufferPool::writeOptions wopts;
      globalLobChunkKey glckey;
      glckey.set(_manifest->id.getSpaceId(), context->getMBID(), key);
      SDB_ASSERT(glckey.isValid(), "can not be invalid");
      lobcExtentChain chain;
      lobExtentMetaBlock block;
      lobcMetaBlockMapping mapping(_manifest, _uberBlock.bucketEntryPid, &_metaFile);

      rc = reserveExtent(offset + data.getSize(), desc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to reserve extent:%d", rc);
         goto error;
      }

      block.init(context->getLogicalClId(),
                  context->getMBID(),
                  key, desc, 0, TRUE);
      SDB_ASSERT(block.isValid(), "can not be invalid");
      
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
   done:
      return rc;
   error:
      if (desc.isValid())
      {
         _smgr.releaseExtent(desc.pid, desc.pcnt);
      }
      goto done;
   }


   INT32 largeObjectSpace::_updateLobc(requestContext *context,
                                       const lobChunkSearchEntry &entry,
                                       lobcExtentChain &chain,
                                       UINT32 offset,
                                       const slice &data)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be invalid");
      SDB_ASSERT(entry.isValid(), "can not be invalid");
      SDB_ASSERT(!chain.isEmpty(), "can not be invalid");
      SDB_ASSERT(data.isValid(), "can not be invalid");

      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
      SDB_ASSERT(nullptr != tc, "can not be null");
      lobChunkBufferPool &pool = tc->getEnv()->lobcBufferPool;
      lobChunkBufferPool::writeOptions wopts;
      globalLobChunkKey glckey(context->getSpaceID(),
                               context->getMBID(),
                               entry.getKey());
      SDB_ASSERT(glckey.isValid(), "can not be invalid");
      wopts.originalChunkSize = chain.getChainSize();
      
      if (chain.getChunkSize() < (offset + data.getSize()))
      {
         lobcMetaBlockMapping mapping(_manifest, _uberBlock.bucketEntryPid, &_metaFile);
         UINT32 totalDeltaSize = (offset + data.getSize() - chain.getChunkSize());
         UINT32 extendedSize = chain.extendLastExtent(totalDeltaSize);
         lextentDescriptor newDesc;
         
         if (extendedSize < totalDeltaSize)
         {
            lobExtentMetaBlock newBlock;
            rc = reserveExtent(totalDeltaSize - extendedSize, newDesc);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to reserve new extent:%d", rc);
               goto error;
            }

            newBlock.init(context->getLogicalClId(), context->getMBID(),
                          entry.getKey(), newDesc, chain.getChainSize(), TRUE);
            rc = mapping.appendBlockToChain(&newBlock);
            if (SDB_OK != rc)
            {
               _smgr.releaseExtent(newDesc.pid, newDesc.pcnt);
               PD_LOG(PDERROR, "failed to append new tail to chain:%d", rc);
               goto error;
            }

            chain.pushBack(newDesc);
         }
         else
         {
            rc = mapping.extendLastBlockSize(entry, extendedSize);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to extend the last block:%d", rc);
               goto error;
            }
         }

      }

      rc = pool.write(glckey, chain, offset, data, wopts);
      if (SDB_OK != rc)
      {
         /// should panic here?
         PD_LOG(PDERROR, "failed to write buffer pool:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 largeObjectSpace::_truncateLobc(requestContext *context,
                                         const lobChunkKey &key,
                                         UINT32 size,
                                         UINT32 &tsize)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(0 < size, "can not be invalid");

      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
      SDB_ASSERT(nullptr != tc, "can not be null");
      lobChunkBufferPool &pool = tc->getEnv()->lobcBufferPool;
      lobcMetaBlockMapping mapping(_manifest, _uberBlock.bucketEntryPid, &_metaFile);
      lobChunkSearchEntry entry(key, context->getLogicalClId());
      lobcExtentChain chain;
      ossPoolList<lextentDescriptor> discarded;

      rc = mapping.truncate(entry, size, tsize, chain, discarded);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to truncate lobc:%d", rc);
         goto error;
      }

      if (0 < tsize)
      {
         globalLobChunkKey glckey(context->getSpaceID(),
                                  context->getMBID(),
                                  entry.getKey());
         pool.truncate(glckey, chain);
      }

      for (auto itr = discarded.cbegin(); itr != discarded.cend(); ++itr)
      {
         _smgr.releaseExtent(itr->pid, itr->pcnt);
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 largeObjectSpace::list(listLobChunkCursor *cursor)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(nullptr == cursor ||
                       cursor->isClosed()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      {
         lobcMetaBlockMapping mapping(_manifest, _uberBlock.bucketEntryPid, &_metaFile);
         rc = mapping.list(cursor);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }
} // namespace vessel

} // namespace engine
