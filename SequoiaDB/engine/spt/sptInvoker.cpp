/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = sptInvoker.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/03/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "sptInvoker.hpp"
#include "ossUtil.hpp"
#include "sptCommon.hpp"

using namespace bson ;

namespace engine
{

   /*
      _sptInvoker implement
   */
   INT32 _sptInvoker::_getValFromProperty( JSContext *cx,
                                           const sptProperty &pro,
                                           jsval &val )
   {
      INT32 rc = SDB_OK ;
      if ( String == pro.getType() )
      {
         JSString *jsstr = JS_NewStringCopyN( cx, pro.getString(),
                                              ossStrlen( pro.getString() ) ) ;
         if ( NULL == jsstr )
         {
            ossPrintf( "%s\n", pro.getString() ) ;
            PD_LOG( PDERROR, "failed to create a js string" ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         val = STRING_TO_JSVAL( jsstr ) ;
      }
      else if ( Bool == pro.getType() )
      {
         BOOLEAN v = TRUE ;
         rc = pro.getNative( Bool, &v ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }

         val = BOOLEAN_TO_JSVAL( v ) ;
      }
      else if ( NumberInt == pro.getType() )
      {
         INT32 v = 0 ;
         rc = pro.getNative( NumberInt, &v ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }

         val = INT_TO_JSVAL( v ) ;
      }
      else if ( NumberDouble == pro.getType() )
      {
         FLOAT64 v = 0 ;
         rc = pro.getNative( NumberDouble, &v ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
         val = DOUBLE_TO_JSVAL( v ) ;
      }
      else
      {
         PD_LOG( PDERROR, "the type %d is not surpported yet.",
                 pro.getType() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptInvoker::_callbackDone( JSContext *cx, JSObject *obj,
                                     _sptReturnVal &rval,
                                     const bson::BSONObj &detail,
                                     jsval *rvp )
   {
      INT32 rc = SDB_OK ;
      const sptProperty &rpro = rval.getVal() ;
      jsval val = JSVAL_VOID ;

      if ( EOO == rpro.getType() )
      {
         *rvp = JSVAL_VOID ;
         goto done ;
      }
      else if ( Object == rpro.getType() )
      {
         JSObject *jsObj = JS_NewObject ( cx, (JSClass *)(rval.getClassDef()),
                                          0 , 0 ) ;
         if ( NULL == jsObj )
         {
            PD_LOG( PDERROR, "faile to new js object" ) ;
            rc = SDB_OOM ;
            /// need to release object instance
            rval.releaseObj() ;
            goto error ;
         }

         JS_SetPrivate( cx, jsObj, rpro.getValue() ) ;

         if ( !rval.getValProperties().empty() )
         {
            rc = _sptInvoker::setProperty( cx, jsObj,
                                           rval.getValProperties() ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
         }

         val = OBJECT_TO_JSVAL( jsObj ) ;
      }
      else
      {
         rc = _getValFromProperty( cx, rpro, val ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }

      if ( !rpro.getName().empty() && NULL != obj )
      {
         if ( !JS_SetProperty( cx, obj,
                               rpro.getName().c_str(),
                               &val ))
         {
            PD_LOG( PDERROR, "failed to set obj to parent obj" ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }

      *rvp = val ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptInvoker::setProperty( JSContext *cx,
                                   JSObject *obj,
                                   const SPT_PROPERTIES &properties )
   {
      INT32 rc = SDB_OK ;
      SPT_PROPERTIES::const_iterator itr = properties.begin() ;
      for ( ; itr != properties.end(); itr++ )
      {
         SDB_ASSERT( !itr->getName().empty(), "name can not be empty" ) ;
         jsval val = JSVAL_VOID ;
         rc = _getValFromProperty( cx, *itr, val ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }

         if ( !JS_SetProperty( cx, obj, itr->getName().c_str(), &val ) )
         {
            PD_LOG( PDERROR, "failed to set property of obj" ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   void _sptInvoker::_reportError( JSContext *cx,
                                   INT32 rc,
                                   const bson::BSONObj &detail )
   {
      sdbSetErrno( rc ) ;

      if ( SDB_OK != rc )
      {
         stringstream ss ;
         BSONObjIterator itr( detail) ;
         INT32 fieldNum = detail.nFields() ;
         INT32 count = 0 ;
         while ( itr.more() )
         {
            if ( count > 0 )
            {
               ss << ", " ;
            }
            BSONElement e = itr.next() ;
            if ( fieldNum > 1 ||
                 0 != ossStrcmp( SPT_ERR, e.fieldName() ) )
            {
               ss << e.fieldName() << ": " ;
            }

            if ( String == e.type() )
            {
               ss << e.valuestr() ;
            }
            else if ( NumberInt == e.type() )
            {
               ss << e.numberInt() ;
            }
            else if ( NumberLong == e.type() )
            {
               ss << e.numberLong() ;
            }
            else if ( NumberDouble == e.type() )
            {
               ss << e.numberDouble() ;
            }
            else if ( Bool == e.type() )
            {
               ss << ( e.boolean() ? "true" : "false" ) ;
            }
            else
            {
               ss << e.toString( false, false ) ;
            }
            ++count ;
         }
         sdbSetErrMsg( ss.str().c_str() ) ;

         if ( sdbIsErrMsgEmpty() )
         {
            sdbSetErrMsg( getErrDesp( rc ) ) ;
         }

         JS_SetPendingException( cx , INT_TO_JSVAL( rc ) ) ;
      }
      else
      {
         sdbSetErrMsg( NULL ) ;
      }

      return ;
   }

}

