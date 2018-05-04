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

   Source File Name = rtnSQLMin.cpp

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

#include "rtnSQLMin.hpp"

namespace engine
{
   _rtnSQLMin::_rtnSQLMin( const CHAR *pName )
   :_rtnSQLFunc( pName )
   {
   }

   _rtnSQLMin::~_rtnSQLMin()
   {

   }

   INT32 _rtnSQLMin::result( BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;

      try
      {
         if ( _ele.eoo() )
         {
            builder.appendNull( _alias.toString() ) ;
         }
         else
         {
            builder.appendAs( _ele, _alias.toString() ) ;
         }

         _obj = BSONObj() ;
         _ele = BSONElement() ;
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

   INT32 _rtnSQLMin::_push( const RTN_FUNC_PARAMS &param )
   {
      INT32 rc = SDB_OK ;
      try
      {
         const BSONElement &ele = *(param.begin()) ;
         if ( ele.eoo() || ele.isNull() )
         {
            goto done ;
         }
         else if ( _ele.eoo() )
         {
            BSONObjBuilder builder ;
            builder.append( ele ) ;
            _obj = builder.obj() ;
            _ele = _obj.firstElement() ;
         }
         else if ( 0 < _ele.woCompare( ele, FALSE ) )
         {
            BSONObjBuilder builder ;
            builder.append( ele ) ;
            _obj = builder.obj() ;
            _ele = _obj.firstElement() ;
         }
         else
         {
            /// do nothing.
         }
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
