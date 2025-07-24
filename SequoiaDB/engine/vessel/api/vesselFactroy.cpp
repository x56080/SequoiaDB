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

   Source File Name = vesselFactory.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/api/vesselFactory.h"
#include "vessel/vesselImpl.h"


namespace engine
{
namespace vessel
{
   IVessel *vesselFactroy::createInstance()const
   {
      return SDB_OSS_NEW vesselImpl();
   }

   void vesselFactroy::releaseInstance(IVessel *o)const
   {
      SAFE_OSS_DELETE(o);
   }
} // namespace vessel

} // namespace engine
