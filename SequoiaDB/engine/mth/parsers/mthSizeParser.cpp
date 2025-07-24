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

   Source File Name = mthSizeParser.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== ==============================================
          07/21/2021  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#include "mthSizeParser.hpp"
#include "pdTrace.hpp"
#include "pd.hpp"
#include "mthTrace.hpp"
#include "mthDef.hpp"
#include "mthSActionFunc.hpp"

using namespace bson ;

namespace engine
{
   _mthSizeParser::_mthSizeParser()
   {
      _name = MTH_S_SIZE ;
   }

   _mthSizeParser::~_mthSizeParser() {}

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSIZEPARSER_PARSE, "_mthSizeParser::parse" )
   INT32 _mthSizeParser::parse( const bson::BSONElement &e,
                                _mthSAction &action ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY(SDB__MTHSIZEPARSER_PARSE ) ;

      if ( !mthIsNumber1( e ) )
      {
         rc = SDB_INVALIDARG ;
         PD_RC_CHECK( rc, PDERROR, "placeholder must be 1" ) ;
      }

      action.setAttribute( MTH_S_ATTR_PROJECTION ) ;
      action.setFunc( &mthSizeBuild, &mthSizeGet ) ;
      action.setName( _name.c_str() ) ;

   done:
      PD_TRACE_EXITRC( SDB__MTHSIZEPARSER_PARSE, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}

