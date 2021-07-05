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

   Source File Name = storageUnitDef.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_STORAGE_UNIT_DEF_H_
#define VESSEL_STORAGE_UNIT_DEF_H_

#include "vessel/vesselIdDef.h"
#include "vessel/storageFileDef.h"
#include "vessel/strSlice.h"

namespace engine
{
namespace vessel
{
   struct createSUOptions
   {
      createSUOptions()
      {}
      ~createSUOptions()
      {}

      BOOLEAN isValid()const
      {
         return INVALID_SPACE_ID != sid &&
                dataArgs.isValid() &&
                indexArgs.isValid() &&
                lobArgs.isValid();
      }

      SPACE_ID sid = INVALID_SPACE_ID;
      storageCoreArgs dataArgs;
      storageCoreArgs indexArgs;
      storageCoreArgs lobArgs;
   };//struct createSUOptions

   struct createLogicalPageSpaceOptions
   {
      OSS_INLINE createLogicalPageSpaceOptions(){}
      OSS_INLINE ~createLogicalPageSpaceOptions(){}

      BOOLEAN isValid()const
      {
         return INVALID_SPACE_ID != sid &&
                !dir.empty() &&
                dataArgs.isValid(); 
      }

      SPACE_ID sid = INVALID_SPACE_ID;
      strSlice dir;
      UINT32 secretValue = 0;
      storageCoreArgs dataArgs;
   };//struct createLogicalPageSpaceOptions

}//namespace vessel
}//namespace engine

#endif//VESSEL_STORAGE_UNIT_DEF_H_
