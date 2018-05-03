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

   Source File Name = rtnSQLSum.cpp

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

#include "rtnSQLSum.hpp"

using namespace bson ;

namespace engine
{
   _rtnSQLSum::_rtnSQLSum( const CHAR *pName )
   :_rtnSQLFunc( pName ),
    _sum(0),
    _decSum(),
    _effective( FALSE )
   {
   }

   _rtnSQLSum::~_rtnSQLSum()
   {

   }

   INT32 _rtnSQLSum::result( BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      try
      {
         if ( !_effective )
         {
            builder.appendNull( _alias.toString() ) ;
         }
         else
         {
            if ( _decSum.isZero() )
            {
               builder.append( _alias.toString(), _sum ) ;
            }
            else
            {
               bsonDecimal tmpDecimal ;

               rc = tmpDecimal.fromDouble( _sum ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "from double failed:double=%f,rc=%d", 
                          _sum, rc ) ;
                  goto error ;
               }

               rc = _decSum.add( tmpDecimal ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "decimal add failed:rc=%d", rc ) ;
                  goto error ;
               }

               builder.append( _alias.toString(), _decSum ) ;
            }

            _effective = FALSE ;
            _sum = 0 ;
            _decSum.setZero() ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexcepted err happened%s",
                 e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnSQLSum::_push( const RTN_FUNC_PARAMS &param )
   {
      INT32 rc = SDB_OK ;
      const BSONElement &ele = *(param.begin()) ;
      if ( ele.isNumber() )
      {
         if ( NumberDecimal == ele.type() )
         {
            _decSum.add( ele.numberDecimal() ) ;
         }
         else
         {
            _sum += ele.Number() ;
         }

         _effective = TRUE ;
      }
      
      return rc ;
   }
}
