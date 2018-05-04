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

   Source File Name = sptConvertor2.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of Script component. This file contains structures for javascript
   engine wrapper

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/13/2013  YW Initial Draft

   Last Changed =

*******************************************************************************/

#include "ossUtil.hpp"
#include "sptConvertor2.hpp"
#include "pd.hpp"
#include "ossMem.hpp"
#include "sptConvertorHelper.hpp"

using namespace bson ;

INT32 sptConvertor2::toBson( JSObject *obj , bson::BSONObj &bsobj )
{
   INT32 rc = SDB_OK ;
   SDB_ASSERT( NULL != _cx, "can not be NULL" ) ;

   CHAR *pData = NULL ;

   rc = JSObj2BsonRaw( _cx, obj, &pData ) ;
   if ( rc )
   {
      goto error ;
   }

   try
   {
      bsobj = BSONObj( pData ).getOwned() ;
   }
   catch( std::exception & )
   {
      rc = SDB_SYS ;
      goto error ;
   }

done:
   if ( pData )
   {
      free( pData ) ;
      pData = NULL ;
   }
   return rc ;
error:
   goto done ;
}

INT32 sptConvertor2::toString( JSContext *cx,
                               const jsval &val,
                               std::string &str )
{
   SDB_ASSERT( NULL != cx, "impossible" ) ;
   return JSVal2String( cx, val, str ) ;
}

