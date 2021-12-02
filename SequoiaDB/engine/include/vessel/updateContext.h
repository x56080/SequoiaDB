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

   Source File Name = updateContext.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSE_UPDATE_CONTEXT_H_
#define VESSE_UPDATE_CONTEXT_H_

#include "vessel/dmlContext.h"
#include "interface/IRecordUpdater.h"

namespace engine
{
namespace vessel
{
   class updateContext : public dmlContext
   {
      public:
         updateContext(){}
         virtual ~updateContext(){}

      public:
         OSS_INLINE IRecordUpdater *getUpdater()
         {
            return _updater;
         }
         OSS_INLINE void reset(IRecordUpdater *updater)
         {
            _updater = updater;
            _originalRecord.release();
            _overflow = FALSE;
            _bigRecord = FALSE;
            _overflowAddr = recordID();
         }
         OSS_INLINE memoryBlock &getOriginalRecordBuffer()
         {
            return _originalRecord;
         }
         void setOverflowInfo(BOOLEAN isBigRecord,
                              const recordID &addr)
         {
            _overflow = TRUE;
            _bigRecord = isBigRecord;
            _overflowAddr = addr;
         }

         OSS_INLINE BOOLEAN isOverflow()const
         {
            return _overflow;
         }
         OSS_INLINE BOOLEAN isBigRecord()const
         {
            return _bigRecord;
         }

         OSS_INLINE const recordID &getOverflowAddr()const
         {
            return _overflowAddr;
         }
      private:
         IRecordUpdater *_updater = NULL;
         memoryBlock _originalRecord;
         BOOLEAN _overflow = FALSE;
         BOOLEAN _bigRecord = FALSE;
         recordID _overflowAddr;
         
   };//class updateContext
} // namespace vessel

} // namespace engine


#endif//VESSE_UPDATE_CONTEXT_H_