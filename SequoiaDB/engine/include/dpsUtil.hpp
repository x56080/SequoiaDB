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

   Source File Name = dpsUtil.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains declare for data types used in
   SequoiaDB.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/08/2019  Linyoub  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DPSUTIL_HPP_
#define DPSUTIL_HPP_

#include "ossTypes.h"
#include "dpsDef.hpp"
#include "ossMemPool.hpp"

namespace engine
{

   #define DPS_TRANS_STR_LEN           ( 64 )
   #define DPS_RECORD_FLAGS_STATUS_LEN ( 127 )

   #define DPS_LOG_OP_NAME_DUMMY            "dummy"
   #define DPS_LOG_OP_NAME_INSERT           "insert"
   #define DPS_LOG_OP_NAME_UPDATE           "update"
   #define DPS_LOG_OP_NAME_DELETE           "delete"
   #define DPS_LOG_OP_NAME_CREATE_CS        "createcs"
   #define DPS_LOG_OP_NAME_DELETE_CS        "deletecs"
   #define DPS_LOG_OP_NAME_CREATE_CL        "createcl"
   #define DPS_LOG_OP_NAME_DELETE_CL        "deletecl"
   #define DPS_LOG_OP_NAME_CREATE_IX        "createix"
   #define DPS_LOG_OP_NAME_DELETE_IX        "deleteix"
   #define DPS_LOG_OP_NAME_CL_RENAME        "renamecl"
   #define DPS_LOG_OP_NAME_TRUNCATE_CL      "truncatecl"
   #define DPS_LOG_OP_NAME_TS_COMMIT        "commit"
   #define DPS_LOG_OP_NAME_TS_ROLLBACK      "rollback"
   #define DPS_LOG_OP_NAME_INVALIDATE_CATA  "invalidatecata"
   #define DPS_LOG_OP_NAME_LOB_WRITE        "lobwrite"
   #define DPS_LOG_OP_NAME_LOB_REMOVE       "lobremove"
   #define DPS_LOG_OP_NAME_LOB_UPDATE       "lobupdate"
   #define DPS_LOG_OP_NAME_LOB_TRUNCATE     "lobtruncate"
   #define DPS_LOG_OP_NAME_CS_RENAME        "renamecs"
   #define DPS_LOG_OP_NAME_POP              "pop"
   #define DPS_LOG_OP_NAME_ALTER            "alter"
   #define DPS_LOG_OP_NAME_ADDUNIQUEID      "adduniqueid"
   #define DPS_LOG_OP_NAME_RETURN           "return"
   #define DPS_LOG_OP_NAME_SEC_KEY_CRT      "seckeycrt"

   dpsLogConfig &dpsGetGlobalLogConfig() ;

   const CHAR* dpsTransStatusToString( INT32 status ) ;

   INT32 dpsGetTransIDFromString( const CHAR *pStr, DPS_TRANS_ID &transID ) ;

   const CHAR* dpsTransIDToString( const DPS_TRANS_ID &transID,
                                   CHAR *pBuff,
                                   UINT32 bufSize ) ;

   ossPoolString dpsTransIDToString( const DPS_TRANS_ID &transID ) ;

   const CHAR* dpsTransIDAttrToString( const DPS_TRANS_ID &transID,
                                       CHAR *pBuff,
                                       UINT32 bufSize ) ;

   ossPoolString dpsTransIDAttrToString( const DPS_TRANS_ID &transID ) ;

   void dpsFlags2String( UINT16 flags, CHAR * pBuffer, INT32 bufSize ) ;

   void dpsAppendFlagString( CHAR * pBuffer, INT32 bufSize,
                             const CHAR *flagStr ) ;

   void dpsInvalidateCataTypeToString( UINT8 type, CHAR *pBuffer, INT32 bufSize ) ;

   const CHAR *dpsGetOPName(UINT16 type) ;

   typedef ossPoolSet< DPS_TRANS_ID > DPS_TRANS_ID_SET ;

   // downgrade transaction ID from v1 to v0
   // WARNING: will lose high 16 bits of timestamp
   DPS_TRANS_ID dpsTransIDDowngrade( const dpsTransID_v1 &transID ) ;

}

#endif // DPSUTIL_HPP_


