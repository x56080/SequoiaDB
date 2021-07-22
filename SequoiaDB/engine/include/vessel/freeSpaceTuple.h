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

   Source File Name = freeSpaceTuple.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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
         freeSpaceTuple(){}
         ~freeSpaceTuple(){}
         explicit freeSpaceTuple(UINT32 seq,
                                 PAGE_ID lpid,
                                 INT32 lvl):
                  _seq(seq),
                  _lpid(lpid),
                  _lvl(lvl){}

         freeSpaceTuple(const freeSpaceTuple &o):
         _seq(o._seq),
         _lpid(o._lpid),
         _lvl(o._lvl){}

         freeSpaceTuple &operator=(const freeSpaceTuple &o)
         {
            _seq = o._seq;
            _lpid = o._lpid;
            _lvl = o._lvl;
            return *this;
         }

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return INVALID_CL_PAGE_SEQ != _seq;
         }
         OSS_INLINE void reset(UINT32 seq = INVALID_CL_PAGE_SEQ,
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
         UINT32 _seq = INVALID_CL_PAGE_SEQ;
         PAGE_ID _lpid = INVALID_PAGE_ID;
         INT32 _lvl = FSM_INVALID_SPACE_LVL;
   };//class freeSpaceTuple

   typedef forwardList<freeSpaceTuple, ossSpinLatch> FREE_SPACE_TUPLE_POOL;
}//namespace vessel
}//namespace engine

#endif//VESSEL_FREE_SPACE_TUPLE_H_