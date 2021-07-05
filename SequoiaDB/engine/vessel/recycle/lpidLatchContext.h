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

   Source File Name = lpidLatchContext.h

   Descriptive Name =

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

#include "vessel/vesselFileDef.h"
#include "vessel/pageDef.h"
#include "ossLatch.hpp"
#include "vessel/objectLatchMap.h"

namespace engine
{
namespace vessel
{
   class lpidLatchContext : public SDBObject
   {
      public:
         lpidLatchContext(){}
         ~lpidLatchContext();
         lpidLatchContext(const lpidLatchContext &) = delete;
         lpidLatchContext &operator=(const lpidLatchContext &) = delete;

      private:
         struct _lpidLatchSlot : public SDBObject
         {
            _lpidLatchSlot(){}
            ~_lpidLatchSlot(){}
            _lpidLatchSlot(const _lpidLatchSlot &) = delete;

            _lpidLatchSlot &operator=(const _lpidLatchSlot &o)
            {
               obj = o.obj;
               mode = o.mode;
               return *this;
            }

            LOGICAL_ID_LATCH_MAP::object obj;
            ossSharedLatch::mode mode = ossSharedLatch::NONE;
         };//struct _lpidLatchSlot

      public:
         void reset();
         INT32 push(const LOGICAL_ID_LATCH_MAP::object &obj,
                    ossSharedLatch::mode mode);

         INT32 update(const logicalIdLatchKey &key,
                      ossSharedLatch::mode mode);
  
         BOOLEAN pop(const logicalIdLatchKey &key,
                     LOGICAL_ID_LATCH_MAP::object &obj,
                     ossSharedLatch::mode &mode);
                  
         BOOLEAN find(const logicalIdLatchKey &key,
                      LOGICAL_ID_LATCH_MAP::object &obj,
                      ossSharedLatch::mode &mode);

         INT32 findAndUpdate(const logicalIdLatchKey &key,
                             ossSharedLatch::mode oldMode,
                             ossSharedLatch::mode newMode,
                             LOGICAL_ID_LATCH_MAP::object &obj);

         BOOLEAN test(const logicalIdLatchKey &key,
                      ossSharedLatch::mode &mod);

         OSS_INLINE BOOLEAN isEmpty()const
         {
            return 0 == _size;
         }
      private:
         INT32 ensureBuf(UINT32 size);

         _lpidLatchSlot *find(const logicalIdLatchKey &key,
                              UINT32 &pos);

         void remove(UINT32 pos);

      private:
         static constexpr UINT32 STATIC_BUF_SIZE = 4;

      private:
         UINT32 _capacity = STATIC_BUF_SIZE;
         UINT32 _size = 0;
         _lpidLatchSlot _staticBuf[STATIC_BUF_SIZE];
         _lpidLatchSlot *_slots = _staticBuf;
   };//class lpidLatchContext
}//namespace vessel
}//namespace engine

#endif//VESSEL_LPID_CONTEXT_H_