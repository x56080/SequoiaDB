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

   Source File Name = freeSpaceTuple.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_FREE_SPACE_TUPLE_H_
#define VESSEL_FREE_SPACE_TUPLE_H_

#include "vessel/pageIdentifier.h"
#include "vessel/freeSpaceMapDef.h"


namespace engine
{
namespace vessel
{
   class freeSpaceTuple
   {
      public:
         freeSpaceTuple() = default;
         ~freeSpaceTuple() = default;
         freeSpaceTuple(const freeSpaceTuple &) = default;
         freeSpaceTuple &operator=(const freeSpaceTuple &) = default;
         explicit freeSpaceTuple(UINT32 seq,
                                 PAGE_ID lpid,
                                 INT32 lvl):
                  _seq(seq),
                  _lpid(lpid),
                  _lvl(lvl){}

      public:
         OSS_INLINE void reset(UINT32 seq = 0,
                               PAGE_ID lpid = INVALID_PAGE_ID,
                               INT32 lvl = FSM_INVALID_SPACE_LVL)
         {
            _seq = seq;
            _lpid = lpid;
            _lvl = lvl;
         }
         OSS_INLINE UINT32 getSeq()const
         {
            return _seq;
         }
         OSS_INLINE PAGE_ID getLpid()const
         {
            return _lpid;
         }
         OSS_INLINE BOOLEAN hasValidLpid()const
         {
            return INVALID_PAGE_ID != _lpid;
         }
         OSS_INLINE void setLpid(PAGE_ID lpid)
         {
            _lpid = lpid;
         }
         OSS_INLINE INT32 getSpaceLvl()const
         {
            return _lvl;
         }
         OSS_INLINE void setSpaceLvl(INT32 lvl)
         {
            _lvl = lvl;
         }

      private:
         UINT32 _seq = 0;
         PAGE_ID _lpid = INVALID_PAGE_ID;
         INT32 _lvl = FSM_INVALID_SPACE_LVL;
   };//class freeSpaceTuple

   typedef forwardList<freeSpaceTuple, ossSpinLatch> FREE_SPACE_TUPLE_POOL;
}//namespace vessel
}//namespace engine

#endif//VESSEL_FREE_SPACE_TUPLE_H_