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

   Source File Name = rtnSQLPush.cpp

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
#include "rtnSQLPush.hpp"
#include "ossMem.hpp"
#include "ossTypes.h"
#include "ossErr.h"

using namespace bson;

namespace engine
{
   rtnSQLPush::rtnSQLPush( const CHAR *pName )
   :_rtnSQLFunc( pName )
   {
      _pArrBuilder = SDB_OSS_NEW BSONArrayBuilder();
   }

   rtnSQLPush::~rtnSQLPush()
   {
      if ( _pArrBuilder )
      {
         SDB_OSS_DEL _pArrBuilder;
      }
   }

   INT32 rtnSQLPush::result( BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK;
      PD_CHECK( !_alias.empty(), SDB_INVALIDARG, error, PDERROR,
               "no aliases for function!" );
      try
      {
         builder.appendArray( _alias.toString(), _pArrBuilder->arr() );
         SDB_OSS_DEL _pArrBuilder;
         _pArrBuilder = SDB_OSS_NEW BSONArrayBuilder();
         PD_CHECK( _pArrBuilder != NULL, SDB_OOM, error, PDERROR,
                  "malloc failed" );
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

   INT32 rtnSQLPush::_push( const RTN_FUNC_PARAMS &param )
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT( _pArrBuilder != NULL, "_pArrBuilder can't be NULL!" );
      try
      {
         const BSONElement &ele = *(param.begin());
         if ( !ele.eoo() && !ele.isNull() )
         {
            _pArrBuilder->append( ele );
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
