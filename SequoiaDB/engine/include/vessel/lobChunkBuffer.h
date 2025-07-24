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

   Source File Name = lobChunkBuffer.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_LOB_CHUNK_BUFFER_H_
#define VESSEL_LOB_CHUNK_BUFFER_H_

#include "vessel/lobChunkKey.h"
#include "vessel/bufferControlBlock.h"
#include "ossMemPool.hpp"
#include "dpsDef.hpp"
#include "vessel/multiPageBufferContext.h"
#include "vessel/bufferFlushTask.h"

#include <memory> //c++11

namespace engine
{
namespace vessel
{
   class lobcBufferPoolEnv;

   class lobChunkBuffer : public SDBObject
   {
      public:
         lobChunkBuffer(const globalLobChunkKey &key,
                        const bufferControlBlock &blk,
                        UINT32 pageSize,
                        lobcBufferPoolEnv *env);
         ~lobChunkBuffer();
         lobChunkBuffer(const lobChunkBuffer &) = delete;
         lobChunkBuffer &operator=(const lobChunkBuffer &) = delete;

      public:
         OSS_INLINE BOOLEAN isValid()const {return _key.isValid();}
         OSS_INLINE const globalLobChunkKey &getKey()const {return _key;}

         OSS_INLINE const atomicBufferCtlBlock &ctl()const {return _ctl;}
         OSS_INLINE atomicBufferCtlBlock &ctl() {return _ctl;}
         OSS_INLINE BOOLEAN isInDirtyList()const
         {
            return 0 != OSS_BIT_TEST(_ctl.load().getFlags(),
                                     LOBC_BUFFER_CTL_FLAGS::IN_DIRTY_LIST);
         }

         OSS_INLINE DPS_LSN_OFFSET getMinDirtyLSN()const {return _minLSN;}
         OSS_INLINE DPS_LSN_OFFSET getMaxDirtyLSN()const {return _maxLSN;}
         OSS_INLINE BOOLEAN hasValidLSNPair()const
         {
            return DPS_INVALID_LSN_OFFSET != _minLSN &&
                   DPS_INVALID_LSN_OFFSET != _maxLSN;
         }

         void setLSN(const DPS_LSN_OFFSET &lsn);

         void resetLSN();

         OSS_INLINE const multiPageBufferContext &getBufferCtx()const
         {
            return _bufferCtx;
         }
         OSS_INLINE multiPageBufferContext &getBufferCtx()
         {
            return _bufferCtx;
         }
         OSS_INLINE UINT32 getPageSize()const
         {
            return _bufferCtx.getPageSize();
         }

      public:
         OSS_INLINE BOOLEAN hasMetaDataToCommint()const
         {
            return _hasMetaDataToCommit;
         }

         OSS_INLINE void clearMetaData()
         {
            _hasMetaDataToCommit = FALSE;
         }

         OSS_INLINE void setMetaDataToCommit()
         {
            _hasMetaDataToCommit = TRUE;
         }
      public:
         void exportTasks(ossPoolVector<bufferFlushTask> &tasks)const;
          
      private:
         globalLobChunkKey _key;
         atomicBufferCtlBlock _ctl;

         DPS_LSN_OFFSET _minLSN = DPS_INVALID_LSN_OFFSET;
         DPS_LSN_OFFSET _maxLSN = DPS_INVALID_LSN_OFFSET;

         multiPageBufferContext _bufferCtx;

         BOOLEAN _hasMetaDataToCommit = FALSE; /// tmp code.
   };//class lobChunkBuffer
   typedef class std::shared_ptr<lobChunkBuffer> sharedLobChunkBuffer;
   typedef class ossPoolList<sharedLobChunkBuffer> SHARED_LOBC_BUFFER_LIST;

} // namespace vessel

} // namespace engine


#endif//VESSEL_LOB_CHUNK_BUFFER_H_