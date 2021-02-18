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

   Source File Name = IRedoLogger.h

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_I_REDO_LOGGER_H_
#define VESSEL_I_REDO_LOGGER_H_

#include "dpsDef.hpp"

namespace engine
{
   class _dpsLogRecord;
namespace vessel
{
   class ISession;
   class logRecordContext;

   class IRedoLogger : public SDBObject
   {
      public:
         IRedoLogger(){}
         virtual ~IRedoLogger(){}

      public:
         /// normal api.
         /// prepare and commit.
         virtual INT32 log(ISession *session,
                           const _dpsLogRecord *record,
                           DPS_LSN_OFFSET *lsn) = 0;

         /// allocate lsn and log buffer.
         virtual INT32 prepare(ISession *session,
                               logRecordContext *context) = 0;

         virtual INT32 pushLogRecordElement(ISession *session,
                                            logRecordContext *context,
                                            DPS_TAG tag,
                                            UINT32 len,
                                            const void *value) = 0;

         virtual INT32 commit(ISession *session,
                              logRecordContext *context) = 0;

         /// do not commit log after commit.
         virtual INT32 abort(ISession *session,
                             logRecordContext *context) = 0;

         virtual INT32 pushMaxFileLSN(ISession *session,
                                      DPS_LSN_OFFSET lsn) = 0;
   };//class IRedoLogger
}//namespace vessel
}//namespace engine

#endif//VESSEL_I_REDO_LOGGER_H_