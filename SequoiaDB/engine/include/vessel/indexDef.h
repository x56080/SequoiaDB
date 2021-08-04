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
   static const INDEX_TYPE INVALID_INDEX_TYPE = 65535;
   static const INDEX_TYPE INDEX_TYPE_LSM = 0;
   static const INDEX_TYPE INDEX_TYPE_BTREE = 1;

   static const UINT32 INVALID_LOGICAL_INDEX_ID = (UINT32)(-1);

   static const UINT32 MAX_INDEX_COUNT_PER_CL = 64;

   OSS_INLINE BOOLEAN isValidIndexSlot(INT32 slot)
   {
      return 0 <= slot && slot < (INT32)MAX_INDEX_COUNT_PER_CL;
   }

   static const UINT32 MAX_INDEX_BTREE_PREFIX_COMPRESSION_COLUMNS = 2;
   static const UINT32 MAX_INDEX_KEY_COLUMNS = 32;

   static const UINT32 DIRECT_MAPPING_INDEX_COUNT_PER_CL = 4;

   enum INDEX_STATUS
   {
      INDEX_STATUS_INVALID = 0,
      INDEX_STATUS_CREATING = 1,
      INDEX_STATUS_REBUIDING = 2,
      INDEX_STATUS_ONLINE = 3,
      INDEX_STATUS_REMOVING = 4,
   };// enum INDEX_STATUS

   static const CHAR * const VESSEL_INDEX_FIELD_NAME_TYPE = "type";
   static const CHAR * const VESSEL_INDEX_FIELD_NAME_BTREE_OPTIONS = "btree";
   static const CHAR * const VESSEL_INDEX_FIELD_NAME_LSM_OPTIONS = "lsm";
   static const CHAR * const VESSEL_INDEX_FIELD_NAME_PREFIX_COMPRESSION = "PrefixCompression";
   static const CHAR * const VESSEL_INDEX_FIELD_NAME_COLUMN_FAMILY = "ColumnFamily";

   static const CHAR * const VESSEL_INDEX_FIELD_NAME_INDEX_ID = "LogicalIndexId";
   static const CHAR * const VESSEL_INDEX_FIELD_NAME_STATUS = "status";
   static const CHAR * const VESSEL_INDEX_FIELD_NAME_CREATED_TIME = "CreatedTime";
   static const CHAR * const VESSEL_INDEX_FIELD_NAME_ALTERED_TIME = "AlteredTime";
}//namespace vessel
}//namespace engine

#endif//VESSEL_INDEX_DEF_H_