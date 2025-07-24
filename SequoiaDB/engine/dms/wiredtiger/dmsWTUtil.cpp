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

   Source File Name = dmsWTUtil.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#include "wiredtiger/dmsWTUtil.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"

using namespace std ;

namespace engine
{
namespace wiredtiger
{

   OSS_THREAD_LOCAL int _lastErrorCode = 0 ;

   int dmsWTGetLastErrorCode()
   {
       return _lastErrorCode ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTRC2DBRCSLOW, "dmsWTRCToDBRCSlow" )
   INT32 dmsWTRCToDBRCSlow( int retCode, WT_SESSION *session, BOOLEAN checkConflict )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTRC2DBRCSLOW ) ;

      if ( 0 == retCode )
      {
         goto done ;
      }

      PD_LOG( PDDEBUG, "Got WiredTiger engine error message %s",
              wiredtiger_strerror( retCode ) ) ;
      _lastErrorCode = retCode ;

      switch ( retCode )
      {
      case EINVAL:
         rc = SDB_INVALIDARG ;
         break ;
      case EMFILE:
         rc = SDB_OSS_UP_TO_LIMIT ;
         break ;
      case WT_NOTFOUND:
         rc = SDB_DMS_EOC ;
         break ;
      case WT_DUPLICATE_KEY:
         rc = pdError( SDB_IXM_DUP_KEY ) ;
         break ;
      case WT_ROLLBACK:
         if ( checkConflict )
         {
            rc = pdError( SDB_IXM_DUP_KEY ) ;
         }
         else
         {
            rc = SDB_SYS ;
         }
         break ;
      default:
         rc = SDB_SYS ;
         break ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTRC2DBRCSLOW, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTBLDDATAIDENT, "dmsWTBuildDataIdent" )
   void dmsWTBuildDataIdent( utilCSUniqueID csUID,
                             utilCLInnerID clInnerID,
                             UINT32 clLID,
                             ossPoolStringStream &ss )
   {
      PD_TRACE_ENTRY( SDB__DMSWTBLDDATAIDENT ) ;

      ss << hex << csUID << "_" << clInnerID << "_" << clLID << "_data" ;

      PD_TRACE_EXIT( SDB__DMSWTBLDDATAIDENT ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTBLDINDEXIDENT, "dmsWTBuildIndexIdent" )
   void dmsWTBuildIndexIdent( utilCSUniqueID csUID,
                              utilCLInnerID clInnerID,
                              UINT32 clLID,
                              utilIdxInnerID idxInnerID,
                              ossPoolStringStream &ss )
   {
      PD_TRACE_ENTRY( SDB__DMSWTBLDINDEXIDENT ) ;

      ss << hex << csUID << "_" << clInnerID << "_" << clLID << "_" << idxInnerID << "_idx" ;

      PD_TRACE_EXIT( SDB__DMSWTBLDINDEXIDENT ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTBLDIDENTPREFIX, "dmsWTBuildIdentPrefix" )
   void dmsWTBuildIdentPrefix( utilCSUniqueID csUID,
                               ossPoolStringStream &ss )
   {
      PD_TRACE_ENTRY( SDB__DMSWTBLDIDENTPREFIX ) ;

      ss << hex << csUID << "_" ;

      PD_TRACE_EXIT( SDB__DMSWTBLDIDENTPREFIX ) ;
   }

}
}
