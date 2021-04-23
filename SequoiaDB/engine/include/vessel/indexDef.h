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

   Source File Name = indexDef.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_INDEX_DEF_H_
#define VESSEL_INDEX_DEF_H_

#include "core.hpp"
#include "oss.hpp"

namespace engine
{
namespace vessel
{
   typedef UINT16 INDEX_TYPE;
   const INDEX_TYPE INVALID_INDEX_TYPE = 255;
   const INDEX_TYPE INDEX_TYPE_LSM = 0;
   const INDEX_TYPE INDEX_TYPE_BTREE = 1;

   static const UINT32 MAX_INDEX_NAME_LEN = 1024;
   static const UINT32 INVALID_LOGICAL_INDEX_ID = UINT32(-1);

   static const UINT32 MAX_INDEX_COUNT_PER_CL = 64;

   static const UINT32 MAX_INDEX_BTREE_PREFIX_COMPRESSION_COLUMNS = 2;
   static const UINT32 MAX_BUILDING_INDEX_SORT_BUF_SIZE = 256;
   static const UINT32 MAX_INDEX_KEY_COUNT = 32;
   static const UINT32 MAX_INDEX_SAVING_SIZE = 4096;
}//namespace vessel
}//namespace engine

#endif//VESSEL_INDEX_DEF_H_