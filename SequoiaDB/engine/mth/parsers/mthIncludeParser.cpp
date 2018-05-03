/******************************************************************************

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

   Source File Name = mthIncludeParser.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/01/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "mthIncludeParser.hpp"
#include "pdTrace.hpp"
#include "pd.hpp"
#include "mthTrace.hpp"
#include "mthDef.hpp"
#include "mthSActionFunc.hpp"

namespace engine
{
   _mthIncludeParser::_mthIncludeParser()
   {
      _name = MTH_S_INCLUDE ;
   }

   _mthIncludeParser::~_mthIncludeParser()
   {

   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHINCLUDEPARSER_PARSE, "_mthIncludeParser::parse" )
   INT32 _mthIncludeParser::parse( const bson::BSONElement &e,
                                   _mthSAction &action ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHINCLUDEPARSER_PARSE ) ;
      SDB_ASSERT( !e.eoo(), "can not be eoo" ) ;

      if ( !e.isNumber() )
      {
         PD_LOG( PDERROR, "invalid element type[%d]", e.type() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

#if defined (_DEBUG)
      if ( 0 != _name.compare( e.fieldName() ) )
      {
         PD_LOG( PDERROR, "field name[%s] is not valid",
                 e.fieldName() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
#endif

      action.setName( _name.c_str() ) ;
      if ( 0 == e.numberLong() )
      {
         action.setAttribute( MTH_S_ATTR_EXCLUDE ) ;
      }
      else
      {
         action.setAttribute( MTH_S_ATTR_INCLUDE ) ;
         action.setFunc( &mthIncludeBuild,
                         &mthIncludeGet ) ;
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHINCLUDEPARSER_PARSE, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}

