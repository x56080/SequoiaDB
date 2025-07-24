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

   Source File Name = listCollectionsDef.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/listCollectionsDef.h"
#include "vessel/collectionProperties.h"

namespace engine
{
namespace vessel
{
   bson::BSONObj dumpCollectionWhenList(UINT32 csLogicalID,
                                        const collectionProperties *properties)
   {
      SDB_ASSERT(nullptr != properties, "can not be invalid");
      bson::BSONObjBuilder builder;
      builder.append(CL_DUMP_RECORD_FIELD_CS_LOGICAL_ID, csLogicalID)
             .append(CL_DUMP_RECORD_FIELD_MB_ID, properties->clid.getMbId())
             .append(CL_DUMP_RECORD_FIELD_NAME, properties->name)
             .append(CL_DUMP_RECORD_FIELD_CL_LOGICAL_ID, properties->clid.getLid())
             .append(CL_DUMP_RECORD_FIELD_INNER_ID, properties->clid.getInnerId())
             .append(CL_DUMP_RECORD_FIELD_COMPRESSION, properties->compressor)
             .append(CL_DUMP_RECORD_FIELD_MIN_STRIPING, properties->stripingRange.getLow().getValue())
             .append(CL_DUMP_RECORD_FIELD_MAX_STRIPING, properties->stripingRange.getHigh().getValue());
      return builder.obj();
   }
}//namespace vessel
}//namespace engine
