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

   Source File Name = rtnSQLBuildObj.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains declare for QGM operators

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/16/2013  JHL  Initial Draft

   Last Changed =

*******************************************************************************/
#include "rtnSQLBuildObj.hpp"
#include "ossMem.hpp"
#include "ossTypes.h"
#include "ossErr.h"

using namespace bson;

namespace engine
{
   rtnSQLBuildObj::rtnSQLBuildObj( const CHAR *pName )
   :_rtnSQLFunc( pName )
   {
      _hasData = FALSE;
   }

   rtnSQLBuildObj::~rtnSQLBuildObj()
   {
   }

   INT32 rtnSQLBuildObj::result( bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK;
      PD_CHECK( !_alias.empty(), SDB_INVALIDARG, error, PDERROR,
               "no aliases for function!" );
      try
      {
         builder.append( _alias.toString(), _obj ) ;
         _hasData = FALSE;
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

   INT32 rtnSQLBuildObj::_push( const RTN_FUNC_PARAMS &param )
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT( param.size() == _param.size(), "invalid size!" );

      if ( _hasData )
      {
         goto done ;
      }

      try
      {
         BSONObjBuilder objBuilder;
         UINT32 i = 0 ;
         for ( i = 0; i < param.size(); i++ )
         {
            if ( !param[i].eoo() )
            {
               objBuilder.appendAs( param[i], _param[i].alias.toString() );
            }
         }
         _obj = objBuilder.obj() ;
         _hasData = TRUE ;
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