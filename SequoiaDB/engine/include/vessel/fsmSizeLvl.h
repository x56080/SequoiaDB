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

   Source File Name = fsmSizeLvl.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef SDB_VESSEL_FSM_SIZE_LVL_H_
#define SDB_VESSEL_FSM_SIZE_LVL_H_

#include "vessel/freeSpaceMapDef.h"

namespace engine
{
namespace vessel
{
   class fsmSizeLvl : public SDBObject
   {
      public:
         OSS_INLINE fsmSizeLvl():
         _lvl(FSM_SPACE_LVL_INVALID),
         _delta(0){}

         OSS_INLINE ~fsmSizeLvl(){}

         OSS_INLINE fsmSizeLvl(const fsmSizeLvl &o):
         _lvl(o._lvl),
         _delta(o._delta){}

         OSS_INLINE fsmSizeLvl &operator=(const fsmSizeLvl &o)
         {
            _lvl = o._lvl;
            _delta = o._delta;
            return *this;
         }

      public:
         OSS_INLINE void reset()
         {
            _lvl = FSM_SPACE_LVL_INVALID;
            _delta = 0;
         }
         OSS_INLINE BOOLEAN isValid()const
         {
            return FSM_SPACE_LVL_MIN <= _lvl &&
                   _lvl <= FSM_SPACE_LVL_MAX;
         }

         OSS_INLINE INT8 getLvl()const
         {
            return _lvl;
         }
         OSS_INLINE UINT8 getDelta()const
         {
            return _delta;
         }


         void reset(UINT16 lvl, UINT16 delta);
         void initUnder32KBPage(UINT16 size);
         void initUnder64KBPage(UINT16 size);
         void init(UINT32 pageSize, UINT32 size);
         void incDelta();
         BOOLEAN isSimilarWithHigherLvl()const;
         UINT32 getMinFreeSize(UINT32 pageSize)const;

      private:
         UINT16 _lvl;
         UINT16 _delta;
   };
}//namespace vessel
}//namespace engine

#endif//SDB_VESSEL_FSM_SIZE_LVL_H_