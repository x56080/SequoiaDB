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

   Source File Name = checkpointLSN.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_CHECKPOINT_LSN_H_
#define VESSEL_CHECKPOINT_LSN_H_

#include "dpsDef.hpp"

namespace engine
{
namespace vessel
{
   class checkpointLSN : public SDBObject
   {
      public:
         checkpointLSN(){}
         ~checkpointLSN(){}
         checkpointLSN(const checkpointLSN &o):
         _lsn(o._lsn),
         _minDirtyLSN(o._minDirtyLSN),
         _minUncompletedLSN(o._minUncompletedLSN){}
         checkpointLSN &operator=(const checkpointLSN &o)
         {
            _lsn = o._lsn;
            _minDirtyLSN = o._minDirtyLSN;
            _minUncompletedLSN = o._minUncompletedLSN;
            return *this;
         }

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return DPS_INVALID_LSN_OFFSET != _lsn;
         }
         OSS_INLINE void set(DPS_LSN_OFFSET lsn,
                             DPS_LSN_OFFSET minDirtyLSN,
                             DPS_LSN_OFFSET minUncompletedLSN)
         {
            _lsn = lsn;
            _minDirtyLSN = minDirtyLSN;
            _minUncompletedLSN = minUncompletedLSN;
         }

      public:
         DPS_LSN_OFFSET _lsn = DPS_INVALID_LSN_OFFSET;
         DPS_LSN_OFFSET _minDirtyLSN = DPS_INVALID_LSN_OFFSET;
         DPS_LSN_OFFSET _minUncompletedLSN = DPS_INVALID_LSN_OFFSET;
   };//class checkpointLSN
} // namespace vessel

} // namespace engine

#endif//VESSEL_CHECKPOINT_LSN_H_
