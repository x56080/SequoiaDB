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
#include "logRecordContext.h"
#include "vessel/deltaLogRecord.h"
#include "vessel/slice.h"

namespace engine
{
namespace vessel
{
   class IRedoLogger;
   class ISession;
   class requestContext;

/*
   INT32 initCreateCSLogRecord(const CHAR *name,
                               const SPACE_ID *sid,
                               const utilCSUniqueID *uniqueID,
                               const createCSOptions *options,
                               dpsLogRecord &lr);*/

   INT32 pushFullNameElement(IRedoLogger *logger,
                             ISession *session,
                             logRecordContext *lrc,
                             const strSlice &csName,
                             const strSlice &clName);

   UINT32 packSidAndType(SPACE_ID sid,
                         SPACE_TYPE spaceType,
                         FILE_TYPE fileType);

   void unpackSidAndType(UINT32 value,
                         SPACE_ID &sid,
                         SPACE_TYPE &spaceType,
                         FILE_TYPE &fileType);

   class lpsLogUtil : public SDBObject
   {
      public:
         lpsLogUtil() = delete;
         ~lpsLogUtil() = delete;

      public:
         static INT32 prepare(requestContext *context,
                              const slice &initer,
                              const deltaLogRecord &dlr,
                              logRecordContext &lrc,
                              BOOLEAN isOplistHead = FALSE,
                              DPS_LSN_OFFSET oplist = DPS_INVALID_LSN_OFFSET,
                              BOOLEAN isOplistTail = FALSE);

         static INT32 commit(requestContext *context,
                             logRecordContext &lrc,
                             SPACE_ID sid,
                             SPACE_TYPE spaceType,
                             FILE_TYPE fileType,
                             const deltaLogRecord &dlr,
                             const slice &initer);

         static INT32 abort(requestContext *context,
                            logRecordContext &lrc);
   };//class lpsLogUtil

}//namespace vessel
}//namespace engine

#endif//VESSEL_REDO_LOG_UTIL_H_