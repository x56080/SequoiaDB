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

