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

   Source File Name = qgmSelectorExpr.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

******************************************************************************/

#include "qgmSelectorExpr.hpp"
#include "qgmTrace.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"

namespace engine
{
   _qgmSelectorExpr::_qgmSelectorExpr()
   {

   }

   _qgmSelectorExpr::~_qgmSelectorExpr()
   {

   }

   std::string _qgmSelectorExpr::toString() const
   {
      std::stringstream ss ;
      if ( NULL != _exprRoot )
      {
         _exprRoot->toString( ss ) ;
      }
      return ss.str() ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMSELECTOREXPR_GETVALUE, "_qgmSelectorExpr::getValue" )
   INT32 _qgmSelectorExpr::getValue( const bson::BSONElement &e,
                                     _qgmValueTuple &v ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__QGMSELECTOREXPR_GETVALUE ) ;
      if ( NULL == _exprRoot )
      {
         PD_LOG( PDERROR, "expr root is null" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = _exprRoot->getValue( e, &v ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to get value from expr:%d", rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__QGMSELECTOREXPR_GETVALUE, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}

