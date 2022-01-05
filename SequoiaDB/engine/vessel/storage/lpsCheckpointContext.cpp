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

   Source File Name = lpsCheckpointContext.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/lpsCheckpointContext.h"
#include "pdTrace.hpp"

namespace engine
{
namespace vessel
{
   lpsCheckpointContext::lpsCheckpointContext()
   {}

   lpsCheckpointContext::~lpsCheckpointContext()
   {}

   void lpsCheckpointContext::fini()
   {
      _checkpoint = LPS_CHECKPOINT();
      _status.store(STATUS::NONE);
      _minDirtyLSN = DPS_INVALID_LSN_OFFSET;
      _maxDirtyLSN = DPS_INVALID_LSN_OFFSET;
      return;
   }

   void lpsCheckpointContext::setCheckpoint(const LPS_CHECKPOINT &checkpoint)
   {
      SDB_ASSERT(checkpoint.isValid(), "must be valid");
      _checkpoint = checkpoint;
      return;
   }

   void lpsCheckpointContext::updateDirtyLsn(DPS_LSN_OFFSET lsn)
   {
      if (DPS_INVALID_LSN_OFFSET == lsn)
      {
         goto done;
      }

      if (DPS_INVALID_LSN_OFFSET == _minDirtyLSN)
      {
         _minDirtyLSN = lsn;
      }
      else if (lsn < _minDirtyLSN)
      {
         _minDirtyLSN = lsn;
      }

      if (DPS_INVALID_LSN_OFFSET == _maxDirtyLSN)
      {
         _maxDirtyLSN = lsn;
      }
      else if (_maxDirtyLSN < lsn)
      {
         _maxDirtyLSN = lsn;
      }

   done:
      return;
   }

   DPS_LSN_OFFSET lpsCheckpointContext::getMinDirtyLsn()const
   {
      return _minDirtyLSN;
   }

   void lpsCheckpointContext::clearDirtyLSN()
   {
      _minDirtyLSN = DPS_INVALID_LSN_OFFSET;
      _maxDirtyLSN = DPS_INVALID_LSN_OFFSET;
   }

   BOOLEAN lpsCheckpointContext::tryToApplyCheckpoint()
   {
      INT32 status = STATUS::NONE;
      return status == _status &&
             _status.compare_exchange_strong(status,
                                             STATUS::APPLYING);
   }

   BOOLEAN lpsCheckpointContext::tryToSetRunningFromNoneOrApplying()
   {
      INT32 status = STATUS::APPLYING;
      if (_status.compare_exchange_strong(status,
                                          STATUS::RUNNING))
      {
         return TRUE;
      }

      ///status will be updated to current value if failed to swap
      if (STATUS::NONE == status)
      {
         return _status.compare_exchange_strong(status,
                                                STATUS::RUNNING);
      }
      
      return FALSE;
   }
}//namespace vessel
}//namespace engine