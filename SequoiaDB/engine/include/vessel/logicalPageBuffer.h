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

   Source File Name = logicalPageBuffer.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_LOGICAL_PAGE_BUFFER_H_
#define VESSEL_LOGICAL_PAGE_BUFFER_H_

#include "vessel/lpidLockHelper.h"
#include "vessel/runtimePageBuffer.h"
#include "vessel/copyOnWriteTrigger.h"
#include "utilPooledObject.hpp"

namespace engine
{
namespace vessel
{
   class logicalPageSpace;

   class logicalPageBuffer : public _utilPooledObject
   {
      friend class logicalPageSpace;
      public:
         logicalPageBuffer(){}
         ~logicalPageBuffer();
         logicalPageBuffer(const logicalPageBuffer &) = delete;
         logicalPageBuffer &operator=(const logicalPageBuffer &) = delete;

      public:
         OSS_INLINE PAGE_ID getLogicalPid()const
         {
            return _lh.getLpid();
         }
         OSS_INLINE const runtimePageBuffer &getRuntimeBuffer()const
         {
            return _rpb;
         }
         OSS_INLINE runtimePageBuffer &getRuntimeBuffer()
         {
            return _rpb;
         }
         OSS_INLINE BOOLEAN isValid()const
         {
            return NULL != _lps &&
                   _lh.isLocked() &&
                   _rpb.isValid();
         }
         OSS_INLINE const copyOnWriteTrigger &getCowTrigger()const
         {
            return _cowTrigger;
         }
         OSS_INLINE const ossSharedLatchMode &getLockingMode()const
         {
            return _lh.getLockMode();
         }

      public:
         void fini();
         INT32 prepareToWrite(requestContext *context);
         void commit(DPS_LSN_OFFSET lsn);
         void abort();

         INT32 validatePage(PAGE_TYPE type)const;

      private:
         void init(logicalPageSpace *lps,
                   PAGE_SNAPSHOT_VERION psv,
                   BOOLEAN isMutable);
      
      private:
         logicalPageSpace *_lps = NULL;
         lpidLockHelper _lh;
         runtimePageBuffer _rpb;
         copyOnWriteTrigger _cowTrigger;
   };//class logicalPageBuffer
}//namespace vessel
}//namespace engine

#endif//VESSEL_LOGICAL_PAGE_BUFFER_H_