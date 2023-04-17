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


