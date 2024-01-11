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

   Source File Name = dmsWTUtil.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMS_WT_UTIL_HPP_
#define DMS_WT_UTIL_HPP_

#include "dmsDef.hpp"
#include "ossLikely.hpp"
#include "utilUniqueID.hpp"

#include <wiredtiger.h>

namespace engine
{
namespace wiredtiger
{

   int dmsWTGetLastErrorCode() ;

   // convert WiredTiger error code to SequoiaDB error code
   INT32 dmsWTRCToDBRCSlow( int retCode, WT_SESSION *session, BOOLEAN checkConflict ) ;

   // quick convert WiredTiger error code to SequoiaDB error code
   OSS_INLINE INT32 dmsWTRCToDBRC( int retCode,
                                   WT_SESSION *session,
                                   BOOLEAN checkConflict = FALSE )
   {
      if ( OSS_LIKELY( 0 == retCode ) )
      {
         return SDB_OK ;
      }
      return dmsWTRCToDBRCSlow( retCode, session, checkConflict ) ;
   }

   // WiredTiger call wrapper with error code convert
   #define WT_CALL( func, session ) ( dmsWTRCToDBRC( ( func ), session ) )
   #define WT_CONFLICT_CALL( func, session ) ( dmsWTRCToDBRC( ( func ), session, TRUE ) )

   void dmsWTBuildDataIdent( utilCSUniqueID csUID,
                             utilCLInnerID clInnerID,
                             UINT32 clLID,
                             ossPoolStringStream &ss ) ;

   void dmsWTBuildIndexIdent( utilCSUniqueID csUID,
                              utilCLInnerID clInnerID,
                              UINT32 clLID,
                              utilIdxInnerID idxInnerID,
                              ossPoolStringStream &ss ) ;

   void dmsWTBuildLobIdent( utilCSUniqueID csUID,
                            utilCLInnerID clInnerID,
                            UINT32 clLID,
                            ossPoolStringStream &ss ) ;

   void dmsWTBuildIdentPrefix( utilCSUniqueID csUID,
                               ossPoolStringStream &ss ) ;

}
}

#endif // DMS_WT_UTIL_HPP_
