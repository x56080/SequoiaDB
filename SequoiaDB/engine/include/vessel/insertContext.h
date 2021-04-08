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

   Source File Name = insertContext.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef SDB_VESSEL_INSERT_CONTEXT_H_
#define SDB_VESSEL_INSERT_CONTEXT_H_

#include "vessel/dmlContext.h"
#include "vessel/insertOptions.h"
#include "vessel/freeSpaceMapDef.h"

namespace engine
{
namespace vessel
{
   class insertContext : public dmlContext
   {
      public:
         OSS_INLINE insertContext()
         {}

         virtual ~insertContext(){}
      public:
         OSS_INLINE const insertOptions &getOptions()const
         {
            return _options;
         }
         OSS_INLINE void setOptions(const insertOptions &o)
         {
            _options = o;
         }
         OSS_INLINE fsmCandidate &getCandidate()
         {
            return _candidate;
         }
         OSS_INLINE void setLastFreeSize(UINT32 size)
         {
            _lastFreeSize = size;
         }
         OSS_INLINE UINT32 getLastFreeSize()const
         {
            return _lastFreeSize;
         }
      private:
         insertOptions _options;
         fsmCandidate _candidate;
         UINT32 _lastFreeSize = 0;
   };//class insertContext
}//namespace vessel
}//namespace engine

#endif//SDB_VESSEL_INSERT_CONTEXT_H_