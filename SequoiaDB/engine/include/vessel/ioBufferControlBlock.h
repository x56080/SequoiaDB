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

   Source File Name = ioBufferControlBlock.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_IO_BUFFER_CONTROL_BLOCK_H_
#define VESSEL_IO_BUFFER_CONTROL_BLOCK_H_

#include "vessel/globalPageID.h"
#include "dpsDef.hpp"
#include "vessel/bufferControlBlock.h"
#include "ossSharedLatch.hpp"
#include "vessel/mmapPagePointer.h"
#include "vessel/blockBasedMemPool.h"
#include "ossMemPool.hpp"

#include <memory>//c++11

namespace engine
{
namespace vessel
{
   class ioBufferControlBlock : public SDBObject
   {
      friend class liteIOBufferPool;
      public:
         ioBufferControlBlock() = default;
         explicit ioBufferControlBlock(const globalPageID &gpid,
                                       const bufferControlBlock &ctl,
                                       const mmapPagePointer &ptr);
         ~ioBufferControlBlock() = default;

         ioBufferControlBlock(const ioBufferControlBlock &) = delete;
         ioBufferControlBlock& operator=(const ioBufferControlBlock &) = delete;

      public:
         OSS_INLINE const GLOBAL_PAGE_ID &getGlobalPid()const {return _gpid;}
         OSS_INLINE atomicBufferCtlBlock &ctl() {return _ctl;}
         OSS_INLINE ossSharedLatch &getMutex() {return _mutex;}
         OSS_INLINE const mmapPagePointer &getMPtr()const {return _mptr;}
         OSS_INLINE BOOLEAN hasMemoryBlock()const {return _mb.isValid();}
         OSS_INLINE const blockBasedMemPool::memBlock &getMemoryBlock()const {return _mb;}
         OSS_INLINE blockBasedMemPool::memBlock &getMemoryBlock() {return _mb;}
         OSS_INLINE void resetMemoryBlock() {_mb.reset();}
         void releaseMemoryBlock(blockBasedMemPool &pool);

         OSS_INLINE const CHAR *autoGetBufferPtr()const
         {
            return hasMemoryBlock() ? _mb.getBuffer() : _mptr.getBuf();
         }

         OSS_INLINE CHAR *autoGetBufferPtr()
         {
            return hasMemoryBlock() ? _mb.getBuffer() : _mptr.getBuf();
         }

         OSS_INLINE DPS_LSN_OFFSET getMinDirtyLSN()const {return _minDirtyLSN;}
         OSS_INLINE DPS_LSN_OFFSET getMaxDirtyLSN()const {return _maxDirtyLSN;}
         OSS_INLINE BOOLEAN hasDirtyLSN()const {return DPS_INVALID_LSN_OFFSET != _minDirtyLSN;}
          
         void updateLSNPair(DPS_LSN_OFFSET lsn);
         void resetLSNPair();

         OSS_INLINE BOOLEAN hasDirtyFlag()const
         {
            return 0 != OSS_BIT_TEST(_ctl.load().getFlags(),
                                     LITE_IO_BUFFER_CTL_FLAGS::DIRTY);               
         }

         // OSS_INLINE UINT64 getAccessTick()const {return _accessTick;}
         // OSS_INLINE void setAccessTick(UINT64 v) {_accessTick = v;}

      private:
         GLOBAL_PAGE_ID _gpid;
         atomicBufferCtlBlock _ctl;
         ossSharedLatch _mutex;
         mmapPagePointer _mptr;
         blockBasedMemPool::memBlock _mb;
         DPS_LSN_OFFSET _minDirtyLSN = DPS_INVALID_LSN_OFFSET;
         DPS_LSN_OFFSET _maxDirtyLSN = DPS_INVALID_LSN_OFFSET;
         //UINT64 _accessTick = 0;
   };//class ioBufferControlBlock

   typedef std::shared_ptr<ioBufferControlBlock> SHARED_IO_BUFFER_CB;
   typedef ossPoolList<SHARED_IO_BUFFER_CB> SHARED_IO_BUFFER_CB_LIST;
} // namespace vessel

} // namespace engine


#endif//VESSEL_IO_BUFFER_CONTROL_BLOCK_H_
