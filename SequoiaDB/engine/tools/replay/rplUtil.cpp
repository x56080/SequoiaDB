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

   Source File Name = rplUtil.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          22/9/2016  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "rplUtil.hpp"
#include "dpsDef.hpp"
#include "pd.hpp"

namespace replay
{
   CHAR* getOPName(UINT16 type)
   {
      switch(type)
      {
      case LOG_TYPE_DATA_INSERT:
         return RPL_LOG_OP_INSERT;
      case LOG_TYPE_DATA_UPDATE:
         return RPL_LOG_OP_UPDATE;
      case LOG_TYPE_DATA_DELETE:
         return RPL_LOG_OP_DELETE;
      case LOG_TYPE_CL_TRUNC:
         return RPL_LOG_OP_TRUNCATE_CL;
      case LOG_TYPE_CS_CRT:
         return RPL_LOG_OP_CREATE_CS;
      case LOG_TYPE_CS_DELETE:
         return RPL_LOG_OP_DELETE_CS;
      case LOG_TYPE_CL_CRT:
         return RPL_LOG_OP_CREATE_CL;
      case LOG_TYPE_CL_DELETE:
         return RPL_LOG_OP_DELETE_CL;
      case LOG_TYPE_IX_CRT:
         return RPL_LOG_OP_CREATE_IX;
      case LOG_TYPE_IX_DELETE:
         return RPL_LOG_OP_DELETE_IX;
      case LOG_TYPE_LOB_WRITE:
         return RPL_LOG_OP_LOB_WRITE;
      case LOG_TYPE_LOB_REMOVE:
         return RPL_LOG_OP_LOB_REMOVE;
      case LOG_TYPE_LOB_UPDATE:
         return RPL_LOG_OP_LOB_UPDATE;
      case LOG_TYPE_LOB_TRUNCATE:
         return RPL_LOG_OP_LOB_TRUNCATE;
      case LOG_TYPE_DUMMY:
      case LOG_TYPE_CL_RENAME:
      case LOG_TYPE_TS_COMMIT:
      case LOG_TYPE_TS_ROLLBACK:
      case LOG_TYPE_INVALIDATE_CATA:
      case LOG_TYPE_CS_RENAME:
         return "unsupported";
      default:
         SDB_ASSERT(FALSE, "unknown log type");
         return "unknown";
      }
   }
}

