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

   Source File Name = lobChunkBuffer.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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

         OSS_INLINE DPS_LSN_OFFSET getMinLSN()const {return _minLSN;}
         OSS_INLINE DPS_LSN_OFFSET getMaxLSN()const {return _maxLSN;}
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
         OSS_INLINE BOOLEAN isTrash()const {return _isTrash;}
         OSS_INLINE void setAsTrash() {_isTrash = TRUE;}
         void exportTasks(ossPoolVector<bufferFlushTask> &tasks)const;
          
      private:
         globalLobChunkKey _key;
         atomicBufferCtlBlock _ctl;

         DPS_LSN_OFFSET _minLSN = DPS_INVALID_LSN_OFFSET;
         DPS_LSN_OFFSET _maxLSN = DPS_INVALID_LSN_OFFSET;

         multiPageBufferContext _bufferCtx;

         BOOLEAN _hasMetaDataToCommit = FALSE; /// tmp code.
         BOOLEAN _isTrash = FALSE;
   };//class lobChunkBuffer
   typedef class std::shared_ptr<lobChunkBuffer> sharedLobChunkBuffer;
   typedef class ossPoolList<sharedLobChunkBuffer> SHARED_LOBC_BUFFER_LIST;

} // namespace vessel

} // namespace engine


#endif//VESSEL_LOB_CHUNK_BUFFER_H_