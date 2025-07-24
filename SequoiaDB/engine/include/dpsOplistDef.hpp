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

   Source File Name = dpsOplistDef.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DPS_OPLIST_DEF_HPP__
#define DPS_OPLIST_DEF_HPP__

#include "dpsDef.hpp"

namespace engine
{
   enum DPS_OPLIST_STATUS : INT32
   {
      START = 0x00,
      BUILDING = 0x01,
      ROLLING_BACK = 0x02,
      COMPLETED = 0x03,
   };

} // namespace engine


#endif//DPS_OPLIST_DEF_HPP__