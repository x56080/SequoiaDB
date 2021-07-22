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

   Source File Name = listCollectionSpaceDef.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/listCollectionSpaceDef.h"

namespace engine
{
namespace vessel
{
   bson::BSONObj dumpCollectionSpace(SPACE_ID sid,
                                     const CHAR *name,
                                     utilCSUniqueID uniqueId,
                                     UINT32 logicalId,
                                     UINT16 status,
                                     UINT32 dataPageSize,
                                     UINT32 dataSegSize,
                                     UINT32 idxPageSize,
                                     UINT32 idxSegSize,
                                     UINT32 lobPageSize,
                                     UINT32 lobSegSize)
   {
      bson::BSONObjBuilder builder;
      builder.append(CS_DUMP_RECORD_FIELD_SPACE_ID, sid)
             .append(CS_DUMP_RECORD_FIELD_NAME, name)
             .append(CS_DUMP_RECORD_FIELD_UNIQUE_ID, uniqueId)
             .append(CS_DUMP_RECORD_FIELD_LOGICAL_ID, logicalId)
             .append(CS_DUMP_RECORD_FIELD_STATUS, status)
             .append(CS_DUMP_RECORD_FIELD_DATA_PAGE_SIZE, dataPageSize)
             .append(CS_DUMP_RECORD_FIELD_DATA_SEG_SIZE, dataSegSize)
             .append(CS_DUMP_RECORD_FIELD_IDX_PAGE_SIZE, idxPageSize)
             .append(CS_DUMP_RECORD_FIELD_IDX_SEG_SIZE, idxSegSize)
             .append(CS_DUMP_RECORD_FIELD_LOB_PAGE_SIZE, lobPageSize)
             .append(CS_DUMP_RECORD_FIELD_LOB_SEG_SIZE, lobSegSize);
      return builder.obj();
   }
}//namespace vessel
}//namespace engine