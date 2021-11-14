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

   Source File Name = liteCacheDef.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_COPY_ON_WRITE_TRIGGER_H_
#define VESSEL_COPY_ON_WRITE_TRIGGER_H_

#include "vessel/vesselIdDef.h"

namespace engine
{
namespace vessel
{
   class copyOnWriteTrigger : public SDBObject
   {
      public:
         copyOnWriteTrigger(){}
         copyOnWriteTrigger(PAGE_SNAPSHOT_VERION psv,
                            BOOLEAN mutablePid):
         _psv(psv),
         _mutablePid(mutablePid){}
         copyOnWriteTrigger(const copyOnWriteTrigger &o):
         _psv(o._psv),
         _mutablePid(o._mutablePid){}
         ~copyOnWriteTrigger(){}
         copyOnWriteTrigger &operator=(const copyOnWriteTrigger &o)
         {
            _psv = o._psv;
            _mutablePid = o._mutablePid;
            return *this;
         }
      
      public:
         OSS_INLINE PAGE_SNAPSHOT_VERION getPsv()const
         {
            return _psv;
         }
         OSS_INLINE BOOLEAN isMutablePid()const
         {
            return _mutablePid;
         }
         OSS_INLINE void reset(PAGE_SNAPSHOT_VERION psv,
                               BOOLEAN isMutablePid)
         {
            _psv = psv;
            _mutablePid = isMutablePid;
            return;
         }

      private:
         PAGE_SNAPSHOT_VERION _psv = INVALID_PAGE_SNAPSHOT_VERSION;
         BOOLEAN _mutablePid = TRUE;
   };//class copyOnWriteTrigger
}//namespace vessel
}//namespace engine

#endif//VESSEL_COPY_ON_WRITE_TRIGGER_H_