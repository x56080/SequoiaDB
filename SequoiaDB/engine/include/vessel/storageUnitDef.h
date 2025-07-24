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

   Source File Name = storageUnitDef.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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
         return dataArgs.isValid() &&
                indexArgs.isValid() &&
                lobArgs.isValid();
      }

      storageCoreArgs dataArgs;
      storageCoreArgs indexArgs;
      storageCoreArgs lobArgs;
   };//struct createSUOptions

}//namespace vessel
}//namespace engine

#endif//VESSEL_STORAGE_UNIT_DEF_H_
