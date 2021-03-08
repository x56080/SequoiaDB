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

   Source File Name = lpidContext.h

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_LPID_CONTEXT_H_
#define VESSEL_LPID_CONTEXT_H_

#include "vessel/extentDef.h"
#include "ossLatch.hpp"

namespace engine
{
namespace vessel
{
   static const UINT32 LPID_CONTEXT_STATIC_BUF_COUNT = 4;

   class lpidContext : public SDBObject
   {
      public:
         OSS_INLINE lpidContext():
         _capacity(LPID_CONTEXT_STATIC_BUF_COUNT),
         _size(0),
         _slots(_statcBuf)
         {}

         ~lpidContext();

      private:
         struct _lpidLockSlot : public SDBObject
         {
            _lpidLockSlot():
            type(INVALID_SPACE_TYPE),
            lpid(INVALID_PAGE_ID),
            mode(SHARED){}

            SPACE_TYPE type;
            PAGE_ID lpid;
            OSS_LATCH_MODE mode;
         };//struct _lpidLockSlot

      public:
         /// exlusive lock with no timeout
         /// no recursive locking.
         INT32 lock(SPACE_TYPE type, PAGE_ID lpid, OSS_LATCH_MODE mode);
         INT32 unlock(SPACE_TYPE type, PAGE_ID lpid);
         BOOLEAN testLockMode(SPACE_TYPE type, PAGE_ID lpid, OSS_LATCH_MODE mode)const;
         BOOLEAN testLocked(SPACE_TYPE type, PAGE_ID lpid)const;
      private:
         INT32 extendBuf(UINT32 capacity);

      private:
         UINT32 _capacity;
         UINT32 _size;
         _lpidLockSlot *_slots;
         _lpidLockSlot _statcBuf[LPID_CONTEXT_STATIC_BUF_COUNT];
   };//class lpidContext
}//namespace vessel
}//namespace engine

#endif//VESSEL_LPID_CONTEXT_H_