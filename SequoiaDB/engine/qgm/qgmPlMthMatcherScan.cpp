/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = qgmPIMthMatcherScan.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/1/2014  ly  Initial Draft

   Last Changed =

*******************************************************************************/

#include "qgmPlMthMatcherScan.hpp"
#include "pdTrace.hpp"
#include "qgmTrace.hpp"

using namespace bson;

namespace engine
{
   qgmPlMthMatcherScan::qgmPlMthMatcherScan( const qgmDbAttr &collection,
                                             const qgmOPFieldVec &selector,
                                             const bson::BSONObj &orderby,
                                             const bson::BSONObj &hint,
                                             INT64 numSkip,
                                             INT64 numReturn,
                                             const qgmField &alias,
                                             const bson::BSONObj &matcher )
   : _qgmPlScan( collection, selector, orderby, hint, numSkip, numReturn,
               alias, NULL )
   {
      _condition = matcher.copy();
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMPLMTHMATCHERSCAN__EXEC, "qgmPlMthMatcherScan::_execute" )
   INT32 qgmPlMthMatcherScan::_execute( _pmdEDUCB *eduCB )
   {
      PD_TRACE_ENTRY( SDB__QGMPLMTHMATCHERSCAN__EXEC ) ;
      INT32 rc = SDB_OK;
      SDB_ASSERT ( _input.size() == 0, "impossible" );

      _invalidPredicate = FALSE ;
      _contextID = -1 ;

      if ( SDB_ROLE_COORD == _dbRole )
      {
         rc = _executeOnCoord( eduCB ) ;
      }
      /// not coord or rc is SDB_COORD_UNKNOWN_OP_REQ
      if ( SDB_COORD_UNKNOWN_OP_REQ == rc ||
           SDB_ROLE_COORD != _dbRole )
      {
         rc = _executeOnData( eduCB ) ;
      }

      if ( SDB_RTN_INVALID_PREDICATES == rc )
      {
         rc = SDB_OK ;
         _invalidPredicate = TRUE ;
      }
      else if ( SDB_OK != rc )
      {
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__QGMPLMTHMATCHERSCAN__EXEC, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}
