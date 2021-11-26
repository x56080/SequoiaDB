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

   Source File Name = IRecordUpdater.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_I_RECORD_UPDATER_H_
#define VESSEL_I_RECORD_UPDATER_H_

#include "vessel/slice.h"
#include "vessel/strSlice.h"

namespace engine
{
namespace vessel
{
   class IRecordUpdater : public SDBObject
   {
      public:
         IRecordUpdater(){}
         virtual ~IRecordUpdater(){}
         IRecordUpdater(const IRecordUpdater &) = delete;
         IRecordUpdater &operator=(const IRecordUpdater &) = delete;

      public:
         virtual INT32 modify(const slice &record) = 0;

      public:/// result
         virtual slice getModifiedRecord()const = 0;

         virtual UINT32 getFieldsModified()const = 0;

         virtual strSlice getModifiedFieldName(UINT32 pos)const = 0;
   };//class IRecordUpdater
} // namespace vessel

} // namespace engine


#endif//VESSEL_I_RECORD_UPDATER_H_