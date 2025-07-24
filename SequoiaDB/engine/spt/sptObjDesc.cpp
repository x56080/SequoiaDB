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

   Source File Name = sptObjDesc.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/06/2016  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "sptObjDesc.hpp"
#include "ossUtil.hpp"
#include "sptCommon.hpp"
#include "pd.hpp"

using namespace std ;

namespace engine
{

   /*
      _sptObjFactory implement
   */
   _sptObjFactory::_sptObjFactory()
   {
   }

   _sptObjFactory::~_sptObjFactory()
   {
   }

   BOOLEAN _sptObjFactory::_isExist( const sptObjDesc *desc ) const
   {
      return -1 != _find( desc, _vecObjs ) ? TRUE : FALSE ;
   }

   INT32 _sptObjFactory::_find( const string &objName,
                                const SPT_VEC_OBJDESC &vecObjs ) const
   {
      UINT32 size = vecObjs.size() ;
      UINT32 i = 0 ;

      for ( ; i < size ; ++i )
      {
         if ( !vecObjs[ i ] )
         {
            continue ;
         }
         if ( 0 == ossStrcmp( vecObjs[ i ]->getJSClassName(),
                                   objName.c_str() ) )
         {
            /// find
            return ( INT32 )i ;
         }
      }

      return -1 ;
   }

   INT32 _sptObjFactory::_find( const sptObjDesc *desc,
                                const SPT_VEC_OBJDESC &vecObjs ) const
   {
      UINT32 size = vecObjs.size() ;
      UINT32 i = 0 ;

      for ( ; i < size ; ++i )
      {
         if ( !vecObjs[ i ] )
         {
            continue ;
         }
         if ( desc == vecObjs[ i ] )
         {
            /// find
            return ( INT32 )i ;
         }
      }

      return -1 ;
   }

   void _sptObjFactory::registerObj( const sptObjDesc *desc )
   {
      SDB_ASSERT( desc, "desc can't be NULL" ) ;
      SDB_ASSERT( !_isExist( desc ), "desc already exist" ) ;

      if ( !desc || _isExist( desc ) )
      {
         return ;
      }

      _vecObjs.push_back( desc ) ;
   }

   const sptObjDesc* _sptObjFactory::findObj( const string &objName ) const
   {
      INT32 pos = _find( objName, _vecObjs ) ;
      if ( -1 != pos )
      {
         return _vecObjs[ pos ] ;
      }
      return NULL ;
   }

   UINT32 _sptObjFactory::getObjDescs( SPT_VEC_OBJDESC &vecObjDesc ) const
   {
      vecObjDesc = _vecObjs ;
      return vecObjDesc.size() ;
   }

   BOOLEAN _sptObjFactory::isInstanceOf( JSContext *cx,
                                         JSObject *obj,
                                         const string &objName )
   {
      sptObjDesc *desc = ( sptObjDesc* )findObj( objName ) ;
      if ( desc )
      {
         return desc->isInstanceOf( cx, obj ) ;
      }
      return FALSE ;
   }

   string _sptObjFactory::getClassName( JSContext *cx, JSObject *obj )
   {
      jsval val = JSVAL_VOID ;
      string name ;

      if ( JS_GetProperty( cx, obj, SPT_OBJ_CNAME_PROPNAME, &val ) &&
           JSVAL_IS_STRING( val ) )
      {
         JSString *jsStr = JSVAL_TO_STRING( val ) ;
         CHAR* str = JS_EncodeString ( cx , jsStr ) ;
         if ( str )
         {
            name = str ;
            JS_free( cx, str ) ;
         }
      }

      return name ;
   }

   void _sptObjFactory::_getClassMemFuncNamesByNative( const string &className,
                                                       set<string> &setFuns,
                                                       BOOLEAN showHide )
   {
      const _sptObjDesc *desc = findObj( className ) ;
      while ( desc )
      {
         desc->getFuncMap().getMemberFuncNames( setFuns, showHide ) ;
         desc = desc->getParent() ;
      }
   }

   void _sptObjFactory::_getClassStaticFuncNamesByNative( const string &className,
                                                          set < string > &setFuns,
                                                          BOOLEAN showHide )
   {
      const _sptObjDesc *desc = findObj( className ) ;
      if ( desc )
      {
         desc->getFuncMap().getStaticFuncNames( setFuns, showHide ) ;
      }
   }

   void _sptObjFactory::getObjFuncNames( JSContext *cx,
                                         JSObject *obj,
                                         set< string > &setFuns,
                                         BOOLEAN showHide )
   {
      /// 1. to get native funcs
      string className = getClassName( cx, obj ) ;
      _getClassMemFuncNamesByNative( className, setFuns, showHide ) ;

      /// 2. to get self func2
      _getObjFuncNames( cx, obj, setFuns ) ;
      /// 3. get prototype's funcs
      JSObject *prototype = JS_GetPrototype( cx, obj ) ;
      while( prototype )
      {
         _getObjFuncNames( cx, prototype, setFuns ) ;
         prototype = JS_GetPrototype( cx, prototype ) ;
      }
   }

   void _sptObjFactory::getObjStaticFunNames( JSContext *cx,
                                              JSObject *obj,
                                              set < string > &setFuns,
                                              BOOLEAN showHide )
   {
      /// 1. to get native static funcs
      string className = getClassName( cx, obj ) ;
      _getClassStaticFuncNamesByNative( className, setFuns, showHide ) ;

      /// 2. to get constructor's static functions
      JSObject *constructor = JS_GetConstructor( cx, obj ) ;
      if ( constructor )
      {
         _getObjFuncNames( cx, constructor, setFuns ) ;
      }
   }

   void _sptObjFactory::getClassFuncNames( JSContext *cx,
                                           const string &className,
                                           set< string > &setFuns,
                                           BOOLEAN showHide )
   {
      JSObject *jsObj = _newObjectByName( cx, className ) ; ;
      if ( jsObj )
      {
         getObjFuncNames( cx, jsObj, setFuns, showHide ) ;
         JS_MaybeGC( cx ) ;
      }
   }

   void _sptObjFactory::getClassStaticFuncNames( JSContext *cx,
                                                 const string &className,
                                                 set < string > &setFuns,
                                                 BOOLEAN showHide )
   {
      JSObject *jsObj = _newObjectByName( cx, className ) ;
      if ( jsObj )
      {
         getObjStaticFunNames( cx, jsObj, setFuns, showHide ) ;
         JS_MaybeGC( cx ) ;
      }
   }

   void _sptObjFactory::getObjPropNames( JSContext *cx,
                                         JSObject *obj,
                                         set< string > &setProp )
   {
      /// first get self prop
      _getObjPropNames( cx, obj, setProp ) ;
      /// then get prototype's prop
      JSObject *prototype = NULL ;
      prototype = JS_GetPrototype( cx, obj ) ;
      while( prototype )
      {
         _getObjPropNames( cx, prototype, setProp ) ;
         prototype = JS_GetPrototype( cx, prototype ) ;
      }
   }

   void _sptObjFactory::getObjStaticPropNames( JSContext *cx,
                                               JSObject *obj,
                                               set < string > &setProp )
   {
      JSObject *constructor = JS_GetConstructor( cx, obj ) ;
      if ( constructor )
      {
         _getObjPropNames( cx, constructor, setProp ) ;
      }
   }

   void _sptObjFactory::getClassStaticPropNames( JSContext *cx,
                                                 const string &className,
                                                 set < string > &setProp )
   {
      JSObject *jsObj = _newObjectByName( cx, className ) ;
      if ( jsObj )
      {
         getObjStaticPropNames( cx, jsObj, setProp ) ;
         JS_MaybeGC( cx ) ;
      }
   }

   void _sptObjFactory::_getObjFuncNames( JSContext *cx,
                                          JSObject *obj,
                                          set< string > &setFuns )
   {
      JSIdArray *properties = NULL ;
      properties = JS_Enumerate( cx, obj ) ;
      if ( !properties )
      {
         goto done ;
      }

      for ( jsint i = 0; i < properties->length; i++ )
      {
         jsid id = properties->vector[i] ;
         jsval fieldName = JSVAL_VOID ;
         jsval fieldValue = JSVAL_VOID ;

         if ( !JS_GetPropertyById( cx, obj, id, &fieldValue ) )
         {
            continue ;
         }
         if ( !JSVAL_IS_OBJECT( fieldValue ) )
         {
            continue ;
         }
         JSObject *objvalue = JSVAL_TO_OBJECT( fieldValue ) ;
         if ( !objvalue )
         {
            continue ;
         }
         if ( !JS_ObjectIsFunction( cx, objvalue ) )
         {
            continue ;
         }
         if ( !JS_IdToValue( cx, id, &fieldName ) )
         {
            continue ;
         }
         JSString *jsStr = JS_ValueToString( cx, fieldName ) ;
         if ( !jsStr )
         {
            continue ;
         }
         CHAR *str = JS_EncodeString( cx, jsStr ) ;
         if ( !str )
         {
            continue ;
         }
         setFuns.insert( str ) ;
         JS_free( cx, str ) ;
      }

      /// free
      JS_DestroyIdArray( cx, properties ) ;

   done:
      return ;
   }

   void _sptObjFactory::_getObjPropNames( JSContext *cx,
                                          JSObject *obj,
                                          set< string > &setProp )
   {
      JSIdArray *properties = NULL ;
      properties = JS_Enumerate( cx, obj ) ;
      if ( !properties )
      {
         goto done ;
      }

      for ( jsint i = 0; i < properties->length; i++ )
      {
         jsid id = properties->vector[i] ;
         jsval fieldName = JSVAL_VOID ;
         jsval fieldValue = JSVAL_VOID ;

         if ( !JS_GetPropertyById( cx, obj, id, &fieldValue ) )
         {
            continue ;
         }
         JSObject *objvalue = NULL ;
         if ( JSVAL_IS_OBJECT( fieldValue ) &&
              NULL != ( objvalue = JSVAL_TO_OBJECT( fieldValue ) ) &&
              JS_ObjectIsFunction( cx, objvalue ) )
         {
            continue ;
         }
         if ( !JS_IdToValue( cx, id, &fieldName ) )
         {
            continue ;
         }
         JSString *jsStr = JS_ValueToString( cx, fieldName ) ;
         if ( !jsStr )
         {
            continue ;
         }
         CHAR *str = JS_EncodeString( cx, jsStr ) ;
         if ( !str )
         {
            continue ;
         }
         setProp.insert( str ) ;
         JS_free( cx, str ) ;
      }
      /// free
      JS_DestroyIdArray( cx, properties ) ;

   done:
      return ;
   }

   void _sptObjFactory::_sortAndAssert( SPT_VEC_OBJDESC &vecObj,
                                         const sptObjDesc *desc )
   {
      /// no name
      if ( desc->isIgnoredName() )
      {
         vecObj.push_back( desc ) ;
      }
      /// no parent
      else if ( desc->isIgnoredParent() )
      {
         vecObj.push_back( desc ) ;
      }
      /// parent existed
      else if ( -1 != _find( desc->getParent(), vecObj ) )
      {
         vecObj.push_back( desc ) ;
      }
      /// parent is not existed
      else
      {
         INT32 tmpPos = _find( desc->getParent(), _vecObjs ) ;
         if ( -1 == tmpPos )
         {
            SDB_ASSERT( FALSE, "Parent is not existed" ) ;
         }
         else
         {
            const sptObjDesc *parent = _vecObjs[ tmpPos ] ;
            _vecObjs[ tmpPos ] = NULL ;
            _sortAndAssert( vecObj, parent ) ;
         }
         vecObj.push_back( desc ) ;
      }
   }

   void _sptObjFactory::sortAndAssert()
   {
      SPT_VEC_OBJDESC vecTmp ;
      UINT32 size = _vecObjs.size() ;
      UINT32 i = 0 ;
      const sptObjDesc *desc = NULL ;

      for ( ; i < size ; ++i )
      {
         desc = _vecObjs[ i ] ;

         if ( !desc )
         {
            continue ;
         }

         _sortAndAssert( vecTmp, desc ) ;
      }

      _vecObjs = vecTmp ;
   }

   void _sptObjFactory::getClassNames( set< string > &setClass,
                                       BOOLEAN showHide )
   {
      const sptObjDesc *desc = NULL ;
      UINT32 i = 0 ;
      UINT32 size = _vecObjs.size() ;

      for ( ; i < size ; ++i )
      {
         desc = _vecObjs[ i ] ;

         if ( !desc || desc->isIgnoredName() )
         {
            continue ;
         }
         if ( showHide || !desc->isHide() )
         {
            setClass.insert( desc->getJSClassName() ) ;
         }
      }
   }

   JSObject* _sptObjFactory::_newObjectByName( JSContext *cx,
                                               const string &className )
   {
      JSObject *jsObj = NULL ;

      if ( className.empty() )
      {
         jsObj = (JSObject*)sdbGetThreadGlobal() ;
      }
      else
      {
         const _sptObjDesc *desc = findObj( className ) ;
         if ( desc )
         {
            jsObj = JS_NewObject ( cx, (JSClass *)(desc->getClassDef()),
                                   0 , 0 ) ;
         }
         if ( jsObj )
         {
            jsval val = JSVAL_VOID ;
            JSString *jsstr = JS_NewStringCopyN( cx, className.c_str(),
                                                 className.length() ) ;
            if ( jsstr )
            {
               val = STRING_TO_JSVAL( jsstr ) ;
               JS_DefineProperty( cx, jsObj, SPT_OBJ_CNAME_PROPNAME,
                                  val, 0, 0,
                                  SPT_PROP_READONLY|SPT_PROP_PERMANENT ) ;
            }
         }
      }

      return jsObj ;
   }

   sptObjFactory* sptGetObjFactory()
   {
      static sptObjFactory s_objFactory ;
      return &s_objFactory ;
   }

   BOOLEAN sptIsInstanceOf( JSContext *cx,
                            JSObject *obj,
                            const string &objName )
   {
      return sptGetObjFactory()->isInstanceOf( cx, obj, objName ) ;
   }

   /*
      _sptObjAssist implement
   */
   _sptObjAssist::_sptObjAssist( const sptObjDesc *desc )
   {
      sptGetObjFactory()->registerObj( desc ) ;
   }

   _sptObjAssist::~_sptObjAssist()
   {
   }

}


