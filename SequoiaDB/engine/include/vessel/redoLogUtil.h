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

   Source File Name = redoLogUtil.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_REDO_LOG_UTIL_H_
#define VESSEL_REDO_LOG_UTIL_H_

#include "dpsLogRecord.hpp"
#include "vessel/vesselIdDef.h"
#include "vessel/strSlice.h"
#include "vessel/slice.h"
#include "vessel/vesselFileDef.h"

#include "../../bson/bson.hpp"

namespace engine
{
namespace vessel
{
   class requestContext;

   UINT32 packSidAndType(SPACE_ID sid,
                         SPACE_TYPE spaceType,
                         FILE_TYPE fileType);

   void unpackSidAndType(UINT32 value,
                         SPACE_ID &sid,
                         SPACE_TYPE &spaceType,
                         FILE_TYPE &fileType);

   INT32 commitCreateIndexLog(const ossPoolString &fullName,
                              UINT32 indexLogicalId,
                              const bson::BSONObj &obj,
                              DPS_LSN_OFFSET &lsn);

   INT32 commitCreateIndexEndLog(const ossPoolString &fullName,
                                 const std::string &indexName,
                                 UINT32 indexId,
                                 INT32 result,
                                 DPS_LSN_OFFSET *lsn = nullptr);

   INT32 commitRemoveIndexLog(const ossPoolString &fullName,
                              const std::string &indexName,
                              UINT32 indexId,
                              DPS_LSN_OFFSET &lsn);

   INT32 commitReleasingPagesLog(requestContext *context,
                                 const ossPoolVector<PAGE_ID> &lpids,
                                 const bson::BSONObj &adjunct);

   INT32 commitTruncateCLLog(const ossPoolString &fullName,
                             DPS_LSN_OFFSET &lsn);

   INT32 commitRemoveCLLog(const ossPoolString &fullName,
                           DPS_LSN_OFFSET &lsn);
   
}//namespace vessel
}//namespace engine

#endif//VESSEL_REDO_LOG_UTIL_H_