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

   Source File Name = sptSPArguments.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/03/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "sptSPArguments.hpp"
#include "sptSPDef.hpp"
#include "pd.hpp"
#include "sptConvertor2.hpp"

using namespace bson ;

namespace engine
{
   _sptSPArguments::_sptSPArguments( JSContext *context, uintN argc, jsval *vp )
   :_context(context),
    _argc(argc),
    _vp(vp)
   {
      SDB_ASSERT( NULL != _context && NULL != _vp, "can not be NULL" ) ;
   }

   _sptSPArguments::~_sptSPArguments()
   {
      _context = NULL ;
      _vp = NULL ;
   }

   INT32 _sptSPArguments::getString( UINT32 pos,
                                     std::string &value,
                                     BOOLEAN strict ) const
   {
      INT32 rc = SDB_OK ;
      JSString *jsStr = NULL ;
      CHAR *str = NULL ;
      jsval *val = NULL ;

      if ( _argc <= pos )
      {
         rc = SDB_OUT_OF_BOUND ;
         goto error ;
      }

      val = _getValAtPos( pos ) ;
      if ( NULL == val )
      {
         PD_LOG( PDERROR, "failed to get val at pos" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      /// strict for String
      if ( strict )
      {
         if ( !JSVAL_IS_STRING( *val ) )
         {
            PD_LOG( PDERROR, "jsval is not a string." ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         jsStr = JSVAL_TO_STRING( *val ) ;
      }
      /// transfer other to string
      else
      {
         jsStr = JS_ValueToString( _context, *val ) ;
      }
      /// Check the result
      if ( NULL == jsStr )
      {
         PD_LOG( PDERROR, "failed to convert jsval to jsstr" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      str = JS_EncodeString ( _context , jsStr ) ;
      if ( NULL == str )
      {
         PD_LOG( PDERROR, "failed to convert a js str to a normal str" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      value.assign( str ) ;

   done:
      SAFE_JS_FREE( _context, str ) ;
      return rc ;
   error:
      goto done ;
   }
   

   jsval *_sptSPArguments::_getValAtPos( UINT32 pos ) const
   {
      return JS_ARGV( _context, _vp ) + pos ;
   }

   INT32 _sptSPArguments::getBsonobj( UINT32 pos,
                                      bson::BSONObj &value ) const
   {
      INT32 rc = SDB_OK ;
      JSObject *jsObj = NULL ;
      jsval *val = NULL ;
      sptConvertor2 convertor( _context ) ;

      _errMsg.clear() ;

      if ( _argc <= pos )
      {
         rc = SDB_OUT_OF_BOUND ;
         goto error ;
      }

      val = _getValAtPos( pos ) ;
      if ( NULL == val )
      {
         _errMsg = "Failed to get val at pos" ;
         PD_LOG( PDERROR, _errMsg ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( !JSVAL_IS_OBJECT( *val ) )
      {
         _errMsg = "jsval is not a object" ;
         PD_LOG( PDERROR, _errMsg ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      jsObj = JSVAL_TO_OBJECT( *val ) ;
      if ( NULL == jsObj )
      {
         _errMsg = "failed to convert jsval to object" ;
         PD_LOG( PDERROR, _errMsg ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = convertor.toBson( jsObj, value, _errMsg ) ;
      if ( SDB_OK != rc )
      {
         if ( _errMsg.empty() )
         {
             PD_LOG( PDERROR, "failed to convert jsobj to bsonobj:%d", rc ) ;
         }
         else
         {
            PD_LOG( PDERROR, _errMsg.c_str(), rc ) ;
         }
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _sptSPArguments::isString( UINT32 pos ) const
   {
      jsval *val = NULL ;
      if ( _argc > pos && NULL != ( val = _getValAtPos( pos ) ) &&
           JSVAL_IS_STRING( *val ) )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   BOOLEAN _sptSPArguments::isNull( UINT32 pos ) const
   {
      jsval *val = NULL ;
      if ( _argc > pos && NULL != ( val = _getValAtPos( pos ) ) &&
           JSVAL_IS_NULL( *val ) )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   BOOLEAN _sptSPArguments::isVoid( UINT32 pos ) const
   {
      jsval *val = NULL ;
      if ( _argc > pos && NULL != ( val = _getValAtPos( pos ) ) &&
           JSVAL_IS_VOID( *val ) )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   BOOLEAN _sptSPArguments::isInt( UINT32 pos ) const
   {
      jsval *val = NULL ;
      if ( _argc > pos && NULL != ( val = _getValAtPos( pos ) ) &&
           JSVAL_IS_INT( *val ) )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   BOOLEAN _sptSPArguments::isDouble( UINT32 pos ) const
   {
      jsval *val = NULL ;
      if ( _argc > pos && NULL != ( val = _getValAtPos( pos ) ) &&
           JSVAL_TO_DOUBLE( *val ) )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   BOOLEAN _sptSPArguments::isNumber( UINT32 pos ) const
   {
      jsval *val = NULL ;
      if ( _argc > pos && NULL != ( val = _getValAtPos( pos ) ) &&
           JSVAL_IS_NUMBER( *val ) )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   BOOLEAN _sptSPArguments::isObject( UINT32 pos ) const
   {
      jsval *val = NULL ;
      if ( _argc > pos && NULL != ( val = _getValAtPos( pos ) ) &&
           JSVAL_IS_OBJECT( *val ) )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   BOOLEAN _sptSPArguments::isBoolean( UINT32 pos ) const
   {
      jsval *val = NULL ;
      if ( _argc > pos && NULL != ( val = _getValAtPos( pos ) ) &&
           JSVAL_IS_BOOLEAN( *val ) )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   string _sptSPArguments::getErrMsg() const
   {
      return _errMsg ;
   }

   BOOLEAN _sptSPArguments::hasErrMsg() const
   {
      return _errMsg.empty() ? FALSE : TRUE ;
   }

   #define NATIVE_VALUE_EQ( pData, type, value ) \
      do \
      { \
         switch( type ) \
         { \
            case SPT_NATIVE_CHAR : \
               *(CHAR*)pData = ( CHAR )( value ) ; \
               break ; \
            case SPT_NATIVE_INT16 : \
               *(INT16*)pData = ( INT16 )( value ) ; \
               break ; \
            case SPT_NATIVE_INT32 : \
               *(INT32*)pData = ( INT32 )( value ) ; \
               break ; \
            case SPT_NATIVE_INT64 : \
               *(INT64*)pData = ( INT64 )( value ) ; \
               break ; \
            case SPT_NATIVE_FLOAT32 : \
               *(FLOAT32*)pData = ( FLOAT32 )( value ) ; \
               break ; \
            case SPT_NATIVE_FLOAT64 : \
               *(FLOAT64*)pData = ( FLOAT64 )( value ) ; \
               break ; \
            default : \
               PD_LOG( PDERROR, "type[%d] is error", type ) ; \
               rc = SDB_INVALIDARG ; \
               goto error ; \
         } \
      } while ( 0 )


   INT32 _sptSPArguments::getNative( UINT32 pos, void *value,
                                     SPT_NATIVE_TYPE type ) const
   {
      INT32 rc = SDB_OK ;
      jsval *val = NULL ;

      if ( _argc <= pos )
      {
         rc = SDB_OUT_OF_BOUND ;
         goto error ;
      }

      val = _getValAtPos( pos ) ;
      if ( NULL == val )
      {
         PD_LOG( PDERROR, "failed to get val at pos" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( JSVAL_IS_INT( *val ) )
      {
         NATIVE_VALUE_EQ( value, type, JSVAL_TO_INT( *val ) ) ;
      }
      else if ( JSVAL_IS_BOOLEAN( *val ) )
      {
         NATIVE_VALUE_EQ( value, type, JSVAL_TO_BOOLEAN( *val ) ) ;
      }
      else if ( JSVAL_IS_DOUBLE( *val ) )
      {
         NATIVE_VALUE_EQ( value, type, JSVAL_TO_DOUBLE( *val ) ) ;
      }
      else
      {
         PD_LOG( PDERROR, "jsval is not a native value" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }
}

