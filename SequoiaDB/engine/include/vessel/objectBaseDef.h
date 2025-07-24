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

   Source File Name = objectBaseDef.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_OBJECT_BASE_DEF_H_
#define VESSEL_OBJECT_BASE_DEF_H_

namespace engine
{
namespace vessel
{
   enum CS_STATUS : UINT16
   {  
      CS_STATUS_INVALID = 0,
      CS_STATUS_ONLINE = 1,
   };//enum CS_STATUS

   enum CS_TYPE : UINT16
   {
      CS_TYPE_INVALID = 0,
      CS_TYPE_NORMAL = 1,
   };//enum CS_TYPE

   enum CL_TYPE : UINT16
   {
      CL_TYPE_INVALID = 0,
      CL_TYPE_NORMAL = 1,
      CL_TYPE_MAX = 65535,
   };//enum CL_TYPE
} // namespace vessel

} // namespace engine


#endif//VESSEL_OBJECT_BASE_DEF_H_