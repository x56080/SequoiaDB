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

   Source File Name = checkpointLSN.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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
