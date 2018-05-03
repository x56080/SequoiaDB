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

   Source File Name = rtnSQLLast.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains declare for QGM operators

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/05/2013  JHL  Initial Draft

   Last Changed =

*******************************************************************************/
#include "rtnSQLLast.hpp"

using namespace bson;

namespace engine
{
   rtnSQLLast::rtnSQLLast( const CHAR *pName )
   :_rtnSQLFunc( pName )
   {
   }

   rtnSQLLast::~rtnSQLLast()
   {
   }

   INT32 rtnSQLLast::result( BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK;
      try
      {
         if ( _ele.eoo() )
         {
            builder.appendNull( _alias.toString() );
         }
         else if ( !_alias.empty() )
         {
            builder.appendAs( _ele, _alias.toString() );
         }
         else
         {
            builder.append( _ele );
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "received unexpected error:%s", e.what() );
         rc = SDB_SYS;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 rtnSQLLast::_push( const RTN_FUNC_PARAMS &param )
   {
      INT32 rc = SDB_OK;
      try
      {
         const BSONElement &ele = *(param.begin());
         if ( !ele.eoo() )
         {
            BSONObjBuilder builder;
            builder.append( ele );
            _obj = builder.obj();
            _ele = _obj.firstElement();
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "received unexpected error:%s", e.what() );
         rc = SDB_SYS;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }
}
