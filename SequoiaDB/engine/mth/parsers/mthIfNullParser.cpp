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

   Source File Name = mthIfNullParser.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== ==============================================
          04/13/2023  youjiangfeng Initial Draft

   Last Changed =

*******************************************************************************/
#include "mthIfNullParser.hpp"
#include "pdTrace.hpp"
#include "pd.hpp"
#include "mthTrace.hpp"
#include "mthDef.hpp"
#include "mthSActionFunc.hpp"

using namespace bson ;

namespace engine
{
   _mthIfNullParser::_mthIfNullParser()
   {
      _name = MTH_S_IFNULL ;
   }

   _mthIfNullParser::~_mthIfNullParser() {}

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHTIFNULLPARSER_PARSE, "_mthIfNullParser::parse" )
   INT32 _mthIfNullParser::parse( const bson::BSONElement &e,
                                  _mthSAction &action ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHTIFNULLPARSER_PARSE ) ;

      action.setAttribute( MTH_S_ATTR_PROJECTION ) ;
      action.setFunc( &mthIfNullBuild, &mthIfNullGet ) ;
      action.setName( _name.c_str() ) ;

      try
      {
         action.setArg( BSON( "arg1" << e ) ) ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "An exception occurred when set the arg of "
                 "$ifnull: %s, rc: %d", e.what(), rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHTIFNULLPARSER_PARSE, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}


