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

   Source File Name = rtnSQLCount.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains declare for QGM operators

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/09/2013  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "rtnSQLCount.hpp"

namespace engine
{
   _rtnSQLCount::_rtnSQLCount( const CHAR *pName )
   :_rtnSQLFunc( pName ),
    _count(0)
   {
   }

   _rtnSQLCount::~_rtnSQLCount()
   {

   }

   INT32 _rtnSQLCount::_push( const RTN_FUNC_PARAMS &param  )
   {
      INT32 rc = SDB_OK ;

      if ( !param.begin()->eoo() )
      {
         ++_count ;
      }
      return rc ;
   }

   INT32 _rtnSQLCount::result( BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      try
      {
         builder.append( _alias.toString(), _count ) ;
         _count = 0 ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexcepted err happened:%s",
                 e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }
}
