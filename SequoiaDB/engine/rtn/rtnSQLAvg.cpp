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

   Source File Name = rtnSQLAvg.cpp

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

#include "rtnSQLAvg.hpp"

using namespace bson ;

namespace engine
{
   _rtnSQLAvg::_rtnSQLAvg( const CHAR *pName )
   :_rtnSQLFunc( pName ),
    _decTotal(),
    _total(0),
    _count(0)
   {
   }

   _rtnSQLAvg::~_rtnSQLAvg()
   {

   }

   INT32 _rtnSQLAvg::result( BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      try
      {
         if ( 0 == _count )
         {
            builder.appendNull( _alias.toString() ) ;
         }
         else
         {
            if ( _decTotal.isZero() )
            {
               FLOAT64 avg = _total / _count ;
               builder.append( _alias.toString(), avg ) ;
            }
            else
            {
               bsonDecimal tmpTotal ;
               bsonDecimal result ;

               rc = tmpTotal.fromDouble( _total ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "from double failed:double=%f,rc=%d", 
                          _total, rc ) ;
                  goto error ;
               }

               rc = _decTotal.add( tmpTotal ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "decimal add failed:rc=%d", rc ) ;
                  goto error ;
               }

               rc = _decTotal.div( _count, result ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "decimal div failed:rc=%d", rc ) ;
                  goto error ;
               }
               builder.append( _alias.toString(), result ) ;
            }

            _decTotal.setZero() ;
            _total = 0 ;
            _count = 0 ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected err happened:%s",
                 e.what() ) ;
         rc = SDB_OK ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnSQLAvg::_push( const RTN_FUNC_PARAMS &param )
   {
      INT32 rc = SDB_OK ;

      try
      {
         const BSONElement &ele = *( param.begin() ) ;
         if ( ele.isNumber() )
         {
            if ( NumberDecimal == ele.type() )
            {
               _decTotal.add( ele.numberDecimal() ) ;
            }
            else 
            {
               _total += ele.Number() ;
            }

            ++_count ;
         }
         
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected err happened:%s",
                 e.what() ) ;
         rc = SDB_OK ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }


}
