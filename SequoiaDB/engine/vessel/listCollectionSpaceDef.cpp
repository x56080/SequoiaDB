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

   Source File Name = listCollectionSpaceDef.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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