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

   Source File Name = mthDefaultParser.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/01/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "mthDefaultParser.hpp"
#include "pdTrace.hpp"
#include "pd.hpp"
#include "mthTrace.hpp"
#include "mthDef.hpp"
#include "mthSActionFunc.hpp"

namespace engine
{
   _mthDefaultParser::_mthDefaultParser()
   {
      _name = MTH_S_DEFAULT ;
   }

   _mthDefaultParser::~_mthDefaultParser()
   {

   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHDEFAULTPARSER_PARSE, "_mthDefaultParser::parse" )
   INT32 _mthDefaultParser::parse( const bson::BSONElement &e,
                                   _mthSAction &action ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHDEFAULTPARSER_PARSE ) ;
      SDB_ASSERT( !e.eoo(), "can not be eoo" ) ;

      action.setAttribute( MTH_S_ATTR_DEFAULT ) ;
      action.setFunc( &mthDefaultBuild,
                      &mthDefaultGet ) ;
      action.setName( _name.c_str() ) ;
      action.setValue( e ) ;
      PD_TRACE_EXITRC( SDB__MTHDEFAULTPARSER_PARSE, rc ) ;
      return rc ;
   }
}

