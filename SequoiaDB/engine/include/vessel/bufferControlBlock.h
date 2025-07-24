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

   Source File Name = bufferControlBlock.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_BUFFER_CONTROL_BLOCK_H_
#define VESSEL_BUFFER_CONTROL_BLOCK_H_

#include "vessel/bufferPoolDef.h"
#include "ossUtil.h"

#include <atomic> //c++11

namespace engine
{
namespace vessel
{
#pragma pack(4)
   class bufferControlBlock
   {
      /// it must be trivially copyable.
      public:
         OSS_INLINE void init(BUFFER_STATUS s, 
                              UINT32 refCnt,
                              BUFFER_CTL_FLAG_WORD flags)
         {
            UINT32 v = (static_cast<UINT32>(s) << 24);
            v |= (refCnt & _REF_COUNT_MASK);
            _statusAndRefCnt = v;
            _flags = flags;
            return;
         }

         OSS_INLINE BOOLEAN isPinned()const
         {
            return 0 < getRefCount() || 0 != _flags;
         }

         OSS_INLINE void setStatus(BUFFER_STATUS s)
         {
            _statusAndRefCnt = ((static_cast<UINT32>(s) << 24) | getRefCount());
         }
         OSS_INLINE BUFFER_STATUS getStatus()const
         {
            return static_cast<BUFFER_STATUS>(_statusAndRefCnt >> 24);
         }
         OSS_INLINE BOOLEAN isInvalid()const
         {
            return BUFFER_STATUS::INVALID == getStatus();
         }
         OSS_INLINE BOOLEAN isNormal()const
         {
            return BUFFER_STATUS::NORMAL == getStatus();
         }
         OSS_INLINE BOOLEAN isRecycling()const
         {
            return BUFFER_STATUS::RECYCLING == getStatus();
         }
         OSS_INLINE BOOLEAN isDiscarded()const
         {
            return BUFFER_STATUS::DISCARDED == getStatus();
         }

      public:
         OSS_INLINE void resetFlags() {_flags = 0;}
         OSS_INLINE void overwriteFlags(BUFFER_CTL_FLAG_WORD flags) {_flags = flags;}
         OSS_INLINE BUFFER_CTL_FLAG_WORD getFlags()const {return _flags;}
         OSS_INLINE void setFlag(BUFFER_CTL_FLAG_WORD flag)
         {
            OSS_BIT_SET(_flags, flag);
         }
         OSS_INLINE void clearFlag(BUFFER_CTL_FLAG_WORD flag)
         {
            OSS_BIT_CLEAR(_flags, flag);
         }
         OSS_INLINE BOOLEAN testFlag(BUFFER_CTL_FLAG_WORD flag)const
         {
            return 0 != OSS_BIT_TEST(_flags, flag);
         }
      public:
         OSS_INLINE UINT32 getRefCount()const {return _statusAndRefCnt & _REF_COUNT_MASK;}
         OSS_INLINE BOOLEAN isReferenced()const {return 0 != getRefCount();}
         OSS_INLINE UINT32 incRefCnt() {return ++_statusAndRefCnt;}
         OSS_INLINE UINT32 decRefCnt() {return --_statusAndRefCnt;} 

      protected:
         static constexpr UINT32 _REF_COUNT_MASK = 0xFFFFFF;

         /// high 8bits: status
         /// low 24bits: ref cnt
         UINT32 _statusAndRefCnt = 0;

         BUFFER_CTL_FLAG_WORD _flags = 0;

   };//class bufferControlBlock
   static_assert(sizeof(bufferControlBlock) == 8, "must be lock-free size");
#pragma pack()
   
   
   class atomicBufferCtlBlock : public SDBObject
   {
      public:
         atomicBufferCtlBlock(){}
         atomicBufferCtlBlock(const bufferControlBlock &o):
         _val(o){}
         ~atomicBufferCtlBlock(){}

         atomicBufferCtlBlock(const atomicBufferCtlBlock &) = delete;
         atomicBufferCtlBlock &operator=(const atomicBufferCtlBlock &) = delete;

      public:
         OSS_INLINE bufferControlBlock load(std::memory_order mo =
                                            std::memory_order_relaxed)const
         {
            return _val.load(mo);
         }

         OSS_INLINE void store(const bufferControlBlock &val)
         {
            _val.store(val, std::memory_order_relaxed);
         }

         /// WARNING: do not use this api unless you
         /// clearly know what you are doing!
         OSS_INLINE bufferControlBlock peek()const
         {
            return *((const bufferControlBlock *)(&_val));
         }

      public:
         BOOLEAN incRefCntIfNormal(bufferControlBlock *old=nullptr);
         void decRefCnt(BUFFER_CTL_FLAG_WORD flagToClear = 0,
                        bufferControlBlock *old=nullptr);
         
         /// buffer can not be pinned
         BOOLEAN setRecyclingFromNormal();

         BOOLEAN setDiscardedFromRecycling();

         BOOLEAN exchange(const bufferControlBlock &expected,
                          const bufferControlBlock &val);

         BOOLEAN exchangeAndUpdateExpected(bufferControlBlock &expected,
                                           const bufferControlBlock &val);

      public:
         BUFFER_CTL_FLAG_WORD resetFlags();
         BUFFER_CTL_FLAG_WORD clearFlag(BUFFER_CTL_FLAG_WORD flags);
         BUFFER_CTL_FLAG_WORD setFlag(BUFFER_CTL_FLAG_WORD flags);
         BUFFER_CTL_FLAG_WORD updateFlag(BUFFER_CTL_FLAG_WORD toSet,
                                         BUFFER_CTL_FLAG_WORD toClear);

         /// condition must be exclusive from flags
         BOOLEAN setFlagIfNot(BUFFER_CTL_FLAG_WORD condition,
                              BUFFER_CTL_FLAG_WORD flags,
                              BUFFER_CTL_FLAG_WORD *old=nullptr);

      protected:
         std::atomic<bufferControlBlock> _val;
   };//class atomicBufferCtlBlock

} // namespace vessel

} // namespace engine


#endif//VESSEL_BUFFER_CONTROL_BLOCK_H_
