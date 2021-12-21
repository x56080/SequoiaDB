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

   Source File Name = dmsEngineDef.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SDB_DMS_ENGINE_DEF_HPP_
#define SDB_DMS_ENGINE_DEF_HPP_

#include "dmsStripingId.hpp"

namespace engine
{
   enum DMS_SCAN_FOR
   {
      DMS_SCAN_FOR_NONE = 0,
      DMS_SCAN_FOR_SHARE = 1,
      DMS_SCAN_FOR_UPDATE = 2,
   };//enum DMS_SCAN_FOR

   

} // namespace engine


#endif//SDB_DMS_ENGINE_HPP_