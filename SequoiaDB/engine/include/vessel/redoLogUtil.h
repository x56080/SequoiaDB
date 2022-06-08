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

   Source File Name = redoLogUtil.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_REDO_LOG_UTIL_H_
#define VESSEL_REDO_LOG_UTIL_H_

#include "dpsLogRecord.hpp"
#include "vessel/vesselIdDef.h"
#include "vessel/strSlice.h"
#include "vessel/slice.h"
#include "vessel/vesselFileDef.h"

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

   INT32 commitCreateIndexLog(requestContext *context,
                              const strSlice &fullName,
                              UINT32 indexId,
                              INT32 indexSlot,
                              const slice &indexDef);

   INT32 commitCreateIndexEndLog(requestContext *context,
                                 const strSlice &fullName,
                                 const strSlice &indexName,
                                 UINT32 indexId,
                                 INT32 indexSlot,
                                 INT32 result);

   INT32 commitReleasingPagesLog(requestContext *context,
                                 const ossPoolVector<PAGE_ID> &lpids,
                                 const bson::BSONObj &adjunct);
}//namespace vessel
}//namespace engine

#endif//VESSEL_REDO_LOG_UTIL_H_