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

   Source File Name = lpsPteViewer.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_LPS_PTE_VIEWER_H_
#define VESSEL_LPS_PTE_VIEWER_H_

#include "ossSharedLatch.hpp"

namespace engine
{
namespace vessel
{
   class lpsPteViewer : public SDBObject
   {
      friend class logicalPageSpacePte;
      public:
         lpsPteViewer() = default;
         ~lpsPteViewer();
         lpsPteViewer(const lpsPteViewer &) = delete;
         lpsPteViewer &operator=(const lpsPteViewer &) = delete;
         lpsPteViewer(lpsPteViewer &&)noexcept;
         lpsPteViewer &operator=(lpsPteViewer &&) noexcept;

      public:
         OSS_INLINE BOOLEAN isValid()const {return !_mode.isNone();}
         OSS_INLINE BOOLEAN isWritable()const {return _mode.isExclusiveOrUpgrade();}
         OSS_INLINE UINT32 getPSN()const {return _psn;}
         void reset();

      private:
         lpsPteViewer(ossSharedLatch *locker,
                      ossSharedLatchMode mode,
                      UINT32 psn);

         void _transferToExclusiveLock();
         void _transferToUpgradeLock();
         void _reset();

      private:
         ossSharedLatch *_locker = nullptr;
         ossSharedLatchMode _mode;
         UINT32 _psn = 0;
   };//class lpsPteViewer
} // namespace vessel

} // namespace engine


#endif//VESSEL_LPS_PTE_VIEWER_H_