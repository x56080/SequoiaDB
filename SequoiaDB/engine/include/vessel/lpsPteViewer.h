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

   Source File Name = lpsPteViewer.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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