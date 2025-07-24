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

   Source File Name = mthSAction.cpp

   Descriptive Name = mth selector action

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/01/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "mthSAction.hpp"
#include "mthTrace.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"

namespace engine
{
   _mthSAction::_mthSAction()
   : _mthMatchTreeHolder(),
     _buildFunc( NULL ),
     _getFunc( NULL ),
     _name( NULL ),
     _attribute( MTH_S_ATTR_NONE ),
     _strictDataMode( FALSE )
   {
      _matchTargetBob.reset() ;
   }

   _mthSAction::~_mthSAction()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSACTION_BUILD, "_mthSAction::build" )
   INT32 _mthSAction::build( const CHAR *fieldName,
                             const bson::BSONElement &e,
                             bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHSACTION_BUILD ) ;
      if ( NULL == _buildFunc )
      {
         goto done ;
      }

      rc = ( *_buildFunc )( fieldName, e, this, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to build column:%d", rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHSACTION_BUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSACTION_GET, "_mthSAction::get" )
   INT32 _mthSAction::get( const CHAR *fieldName,
                           const bson::BSONElement &in,
                           bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHSACTION_GET ) ;
      if ( NULL == _getFunc )
      {
         goto done ;
      }

      rc = ( *_getFunc )( fieldName, in, this, out ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to get column:%d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHSACTION_GET, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}

