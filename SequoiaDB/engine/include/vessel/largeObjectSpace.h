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

   Source File Name = largeObjectSpace.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_LARGE_OBJECT_SPACE_H_
#define VESSEL_LARGE_OBJECT_SPACE_H_

#include "vessel/lobMetaDataFile.h"
#include "vessel/storageFileCluster.h"
#include "vessel/metaDataUberBlock.h"
#include "vessel/variableExtentAllocator.h"
#include "vessel/lobcExtentChain.h"
#include "vessel/lobChunkKey.h"
#include "vessel/strictBuffer.h"
#include "vessel/lobChunkSearchEntry.h"
#include "vessel/listLobChunkCursor.h"
#include "dmsLobDef.hpp"
#include "vessel/fclusterSpaceManager.h"

#include <mutex> //c++11

namespace engine
{
namespace vessel
{
   class requestContext;
   class storageFileLoader;
   
   class largeObjectSpace : public SDBObject
   {
      public:
         largeObjectSpace(const storageUnitManifest *manifest);
         ~largeObjectSpace();

         largeObjectSpace(const largeObjectSpace &) = delete;
         largeObjectSpace &operator=(const largeObjectSpace &) = delete;

      public:
         BOOLEAN isOpen()const {return _metaFile.isOpen();}

         INT32 ensureCreated();

         void close();
         void destroy();

         INT32 open(const storageFileLoader *loader);

         storageFileCluster *getFileCluster() {return &_fcluster;}

      public:
         INT32 insertLobChunk(requestContext *context,
                              const lobChunkKey &key,
                              UINT32 offset,
                              const slice &data);

         INT32 readLobChunk(requestContext *context,
                            const lobChunkKey &key,
                            UINT32 offset,
                            UINT32 size,
                            CHAR *buffer,
                            UINT32 &readSize);

         INT32 removeLobChunk(requestContext *context,
                              const lobChunkKey &key);

         INT32 updateLobChunk(requestContext *context,
                              const lobChunkKey &key,
                              UINT32 offset,
                              const slice &data,
                              BOOLEAN createIfNotExists);

         /// size can not be zero.
         /// truncate lobc only current size is greater than size.
         INT32 truncateLobChunk(requestContext *context,
                                const lobChunkKey &key,
                                UINT32 size,
                                UINT32 &tsize);

         INT32 removeLobChunksInCL(requestContext *context);

         INT32 testLobChunk(requestContext *context,
                            const lobChunkKey &key,
                            dmsLobChunkProfile *profile);


         INT32 list(listLobChunkCursor *cursor);

      private:
         UINT32 getLobdPageSize()const;

         UINT32 getLobdSmePageCapacity()const;

         UINT32 getLobdSmeSize()const;

      private:
         void _close();
         INT32 _create();
         INT32 _createLobmFile(SPACE_ID sid,
                               UINT32 secretValue);

         INT32 _openLobmFile(const storageFileLoader *loader);
         INT32 _initUberBlock();
         INT32 _loadUberBlock(lobmUberBlock &block)const;
         INT32 _saveUberBlock(const lobmUberBlock &block);
         INT32 reserveExtent(UINT32 size, lextentDescriptor &desc);

      private:/// lock lobc key first
         INT32 _insertLobc(requestContext *context,
                           const lobChunkKey &key,
                           UINT32 offset,
                           const slice &data);
                           
         INT32 _updateLobc(requestContext *context,
                           const lobChunkSearchEntry &entry,
                           lobcExtentChain &chain,
                           UINT32 offset,
                           const slice &data);

         INT32 _truncateLobc(requestContext *context,
                             const lobChunkKey &key,
                             UINT32 size,
                             UINT32 &tsize);
                               

      private:
         /// must be under _mutex
         INT32 extendNewLobdSegment(strictBuffer &smeBuffer);

         INT32 ensureLobdSme(UINT32 segmentId, strictBuffer &buffer);

         INT32 getLobdSme(UINT32 segmentId, strictBuffer &buffer);

      private:
         const storageUnitManifest *_manifest = nullptr;
         std::mutex _mutex;
         lobmUberBlock _uberBlock;
         lobMetaDataFile _metaFile;
         storageFileCluster _fcluster;
         fclusterSpaceManager _smgr;
   };//class largeObjectSpace
} // namespace vessel

} // namespace engine


#endif//VESSEL_LARGE_OBJECT_SPACE_H_
