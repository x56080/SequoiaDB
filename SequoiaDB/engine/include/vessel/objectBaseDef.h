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

   Source File Name = objectBaseDef.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_OBJECT_BASE_DEF_H_
#define VESSEL_OBJECT_BASE_DEF_H_

namespace engine
{
namespace vessel
{
   enum CS_STATUS
   {  
      CS_STATUS_INVALID = 0,
      CS_STATUS_ONLINE = 1,
   };//enum CS_STATUS

   enum CS_TYPE
   {
      CS_TYPE_INVALID = 0,
      CS_TYPE_NORMAL = 1,
   };
} // namespace vessel

} // namespace engine


#endif//VESSEL_OBJECT_BASE_DEF_H_