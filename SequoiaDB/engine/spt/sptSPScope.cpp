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

   Source File Name = sptSPScope.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/03/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "sptSPScope.hpp"
#include "sptObjDesc.hpp"
#include "pd.hpp"
#include "ossUtil.hpp"
#include "sptSPDef.hpp"
#include "sptBsonobj.hpp"
#include "sptBsonobjArray.hpp"
#include "sptGlobalFunc.hpp"
#include "sptConvertor2.hpp"
#include "sptConvertorHelper.hpp"
#include "sptCommon.hpp"
#include "spt.hpp"
#include "../spt/js_in_cpp.hpp"

namespace engine
{
   /*
      Local function define
   */
   static JSClass global_class = {
   "Global",                     // class name
   JSCLASS_GLOBAL_FLAGS,         // flags
   JS_PropertyStub,              // addProperty
   JS_PropertyStub,              // delProperty
   JS_PropertyStub,              // getProperty
   JS_StrictPropertyStub,        // setProperty
   JS_EnumerateStub,             // enumerate
   JS_ResolveStub,               // resolve
   JS_ConvertStub,               // convert
   JS_FinalizeStub,              // finalize
   JSCLASS_NO_OPTIONAL_MEMBERS   // optional members
   } ;

   #define SPT_RVAL_KEY          ""
   const UINT32 RUNTIME_SIZE = 64 * 1024 * 1024 ;

   /*
      _sptSPResultVal implement
   */
   _sptSPResultVal::_sptSPResultVal()
   :_value( JSVAL_VOID )
   {
      _ctx = NULL ;
   }

   _sptSPResultVal::~_sptSPResultVal()
   {
   }

   const void* _sptSPResultVal::rawPtr() const
   {
      return (void*)&_value ;
   }

   bson::BSONObj _sptSPResultVal::toBSON() const
   {
      bson::BSONObj obj ;
      _rval2obj( _ctx, _value, obj ) ;
      return obj ;
   }

   void _sptSPResultVal::reset( JSContext *ctx )
   {
      _ctx = ctx ;
      _errStr.clear() ;
      _value = JSVAL_VOID ;
   }

   INT32 _sptSPResultVal::_rval2obj( JSContext *cx,
                                     const jsval &jsrval,
                                     bson::BSONObj &rval ) const
   {
      INT32 rc = SDB_OK ;
      bson::BSONObjBuilder builder ;
      string errMsg ;

      if ( JSVAL_IS_VOID( jsrval ) )
      {
      }
      else if ( JSVAL_IS_STRING( jsrval ) )
      {
         std::string v ;
         rc = sptConvertor2::toString( cx, jsrval, v ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
         builder.append( SPT_RVAL_KEY, v ) ;
      }
      else if ( JSVAL_IS_INT( jsrval ) )
      {
         int32 v = 0 ;
         if ( !JS_ValueToInt32( cx, jsrval, &v ) )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         builder.append( SPT_RVAL_KEY, v ) ;
      }
      else if ( JSVAL_IS_DOUBLE( jsrval ) )
      {
         jsdouble v ;
         if ( !JS_ValueToNumber( cx, jsrval, &v ))
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         builder.appendNumber( SPT_RVAL_KEY, v ) ;
      }
      else if ( JSVAL_IS_BOOLEAN( jsrval ) )
      {
         JSBool v ;
         if ( !JS_ValueToBoolean( cx, jsrval, &v ) )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         builder.appendBool( SPT_RVAL_KEY, v ) ;
      }
      else if ( JSVAL_IS_OBJECT( jsrval ) )
      {
         JSObject *obj = JSVAL_TO_OBJECT( jsrval ) ;
         if ( JSObjIsBsonobj( cx, obj ) )
         {
            CHAR *rawData = NULL ;
            rc = getBsonRawFromBsonClass( cx, obj, &rawData ) ;
            if ( rc )
            {
               goto error ;
            }
            else if ( !rawData )
            {
               rc = SDB_SYS ;
               goto error ;
            }
            builder.append( SPT_RVAL_KEY, bson::BSONObj( rawData ) ) ;
         }
         else if ( _sptBsonobj::__desc.isInstanceOf( cx, obj ) )
         {
            _sptBsonobj *p = (_sptBsonobj*)JS_GetPrivate( cx, obj ) ;
            if ( NULL == p )
            {
               rc = SDB_SYS ;
               goto error ;
            }
            builder.append( SPT_RVAL_KEY, p->getBson() ) ;
         }
         else if ( _sptBsonobjArray::__desc.isInstanceOf( cx, obj ) )
         {
            _sptBsonobjArray *p = (_sptBsonobjArray*)JS_GetPrivate( cx, obj ) ;
            const vector<bson::BSONObj> vecObjs = p->getBsonArray() ;
            bson::BSONArrayBuilder sub( builder.subarrayStart( SPT_RVAL_KEY ) ) ;
            for ( UINT32 i = 0 ; i < vecObjs.size() ; ++i )
            {
               sub.append( vecObjs[ i ] ) ;
            }
            sub.done() ;
         }
         else if ( !JSObjIsSdbObj( cx, JSVAL_TO_OBJECT( jsrval ) ) )
         {
            sptConvertor2 c( cx ) ;
            bson::BSONObj v ;
            rc = c.toBson( JSVAL_TO_OBJECT( jsrval ), v, errMsg ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
            builder.append( SPT_RVAL_KEY, v ) ;
         }
      }
      else
      {
         ossPrintf( "the type[%d] is not supported yet"OSS_NEWLINE,
                    JS_TypeOfValue( cx, jsrval ) ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rval = builder.obj() ;
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _sptSPScope define
   */
   _sptSPScope::_sptSPScope()
   :_runtime( NULL ),
    _context( NULL )
   {
   }

   _sptSPScope::~_sptSPScope()
   {
      shutdown() ;
   }

   INT32 _sptSPScope::start( UINT32 loadMask )
   {
      INT32 rc = SDB_OK ;
      if ( NULL != _runtime )
      {
         ossPrintf( "scope has already been started up"OSS_NEWLINE) ;
         rc = SDB_SYS ;
         goto error ;
      }

      _runtime = JS_NewRuntime( RUNTIME_SIZE );
      if ( NULL == _runtime )
      {
         ossPrintf( "failed to init js runtime"OSS_NEWLINE ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      _context = JS_NewContext( _runtime, RUNTIME_SIZE / 8 );
      if ( NULL == _context )
      {
         ossPrintf( "failed to init js context"OSS_NEWLINE ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      JS_SetOptions( _context, JSOPTION_VAROBJFIX );
      JS_SetVersion( _context, JSVERSION_LATEST );
      JS_SetErrorReporter( _context, sdbReportError ) ;

      _global = JS_NewCompartmentAndGlobalObject( _context, &global_class,
                                                  NULL );
      if ( NULL == _global )
      {
         ossPrintf( "failed to init js global object"OSS_NEWLINE ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( !JS_InitStandardClasses( _context, _global ) )
      {
         ossPrintf( "failed to init standard class"OSS_NEWLINE ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = _loadObj( loadMask ) ;
      if ( rc )
      {
         ossPrintf( "Failed to load object: %d"OSS_NEWLINE, rc ) ;
         goto error ;
      }
      _loadMask = loadMask ;
      sdbDeclareThreadContext( _context ) ;
      sdbDeclareThreadGlobal( _global ) ;

   done:
      return rc ;
   error:
      shutdown() ;
      goto done ;
   }

   INT32 _sptSPScope::_loadObj( UINT32 loadMask )
   {
      INT32 rc = SDB_OK ;

      if ( loadMask & SPT_OBJ_MASK_STANDARD )
      {
         if ( !InitDbClasses( _context, _global ) )
         {
            ossPrintf( "Failed to init dbclass"OSS_NEWLINE ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }

      if ( loadMask & SPT_OBJ_MASK_USR )
      {
         SPT_VEC_OBJDESC vecObjs ;
         sptGetObjFactory()->getObjDescs( vecObjs ) ;
         for ( UINT32 i = 0 ; i < vecObjs.size() ; ++i )
         {
            sptObjDesc *desc = (sptObjDesc*)vecObjs[ i ] ;

            rc = loadUsrDefObj( desc ) ;
            if ( rc )
            {
               ossPrintf( "Load object[%s] failed, rc: %d"OSS_NEWLINE,
                          desc->getJSClassName(), rc ) ;
               goto error ;
            }
         }
      }

      if ( loadMask & SPT_OBJ_MASK_INNER_JS )
      {
         rc = evalInitScripts( this ) ;
         if ( rc )
         {
            ossPrintf ( "Failed to init spt scope, rc = %d"OSS_NEWLINE, rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _sptSPScope::shutdown()
   {
      sdbUndeclareThreadContext( _context ) ;
      sdbUndeclareThreadGlobal( _global ) ;

      if ( NULL != _context )
      {
         void *p = JS_GetContextPrivate( _context ) ;
         if ( NULL != p )
         {
            SDB_OSS_FREE( p ) ;
         }

         JS_SetContextPrivate( _context, NULL ) ;

         JS_EndRequest(_context) ;
         JS_DestroyContext( _context ) ;
         _context = NULL ;
      }

      if ( NULL != _runtime )
      {
         JS_DestroyRuntime( _runtime ) ;
         _runtime = NULL ;
         JS_ShutDown() ;
      }

      _global = NULL ;
   }

   INT32 _sptSPScope::_loadUsrDefObj( _sptObjDesc *desc )
   {
      INT32 rc = SDB_OK ;
      if ( !desc->isIgnoredName() )
      {
         rc = _loadUsrClass( desc ) ;
      }
      else
      {
         rc = _loadGlobal( desc ) ;
      }
      if ( SDB_OK != rc )
      {
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptSPScope::_loadGlobal( _sptObjDesc *desc )
   {
      INT32 rc = SDB_OK ;
      const _sptFuncMap &fMap = desc->getFuncMap() ;
      const sptFuncMap::NORMAL_FUNCS &funcs = fMap.getStaticFuncs() ;
      JSFunctionSpec *specs = new JSFunctionSpec[funcs.size() + 1] ;
      if ( NULL == specs )
      {
         ossPrintf( "failed to allocate mem."OSS_NEWLINE ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      {
      UINT32 i = 0 ;
      sptFuncMap::NORMAL_FUNCS::const_iterator itr = funcs.begin() ;
      for ( ; i < funcs.size() ; i++, itr++ )
      {
         specs[i].name = itr->first.c_str() ;
         specs[i].call = itr->second._pFunc ;
         specs[i].nargs = 0 ;
         specs[i].flags = itr->second._attr ;
      }
      specs[i].name = NULL ;
      specs[i].call = NULL ;
      specs[i].nargs = 0 ;
      specs[i].flags = 0 ;

      if ( !JS_DefineFunctions( _context, _global, specs ) )
      {
         ossPrintf( "failed to define global functions"OSS_NEWLINE ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      }
   done:
      delete []specs ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptSPScope::_loadUsrClass( _sptObjDesc *desc )
   {
      INT32 rc = SDB_OK ;
      JSObject *prototype = NULL ;
      const sptObjDesc *parentDesc = NULL ;
      JSObject *parent_proto = NULL ;
      const CHAR *objName = desc->getJSClassName() ;
      const _sptFuncMap &fMap = desc->getFuncMap() ;
      JS_INVOKER::MEMBER_FUNC construct = fMap.getConstructor() ;
      JS_INVOKER::DESTRUCT_FUNC destruct = fMap.getDestructor() ;
      JS_INVOKER::RESLOVE_FUNC resolve = fMap.getResolver() ;

      uint32 flags = NULL == resolve ?
                     JSCLASS_HAS_PRIVATE :
                     JSCLASS_HAS_PRIVATE | JSCLASS_NEW_RESOLVE ;

      JSResolveOp resolveOp = NULL == resolve ?
                              JS_ResolveStub : (JSResolveOp)resolve ;

      JSClass cDef = { ( CHAR * )objName,
                    flags,
                    JS_PropertyStub,
                    JS_PropertyStub,
                    JS_PropertyStub,
                    JS_StrictPropertyStub,
                    JS_EnumerateStub,
                    resolveOp,
                    JS_ConvertStub,
                    destruct,
                    JSCLASS_NO_OPTIONAL_MEMBERS } ;

      desc->setClassDef( cDef ) ;

      const sptFuncMap::NORMAL_FUNCS &memberFuncs = fMap.getMemberFuncs() ;
      const sptFuncMap::NORMAL_FUNCS &staticFuncs = fMap.getStaticFuncs() ;

      JSFunctionSpec *fSpecs = NULL ;
      JSFunctionSpec *sfSpecs = NULL ;

      if ( _hasPrototype( objName ) )
      {
         goto done ;
      }

      if ( !desc->isIgnoredParent() )
      {
         parentDesc = desc->getParent() ;
         if ( !parentDesc )
         {
            ossPrintf( "Get object[%s]'s parent object failed"OSS_NEWLINE,
                       desc->getJSClassName() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         parent_proto = (JSObject*)_getPrototype( parentDesc->getJSClassName() ) ;
      }

      /// +1 for FS_END
      fSpecs = new JSFunctionSpec[memberFuncs.size() + 1] ;
      if ( NULL == fSpecs )
      {
         ossPrintf( "failed to allocate mem."OSS_NEWLINE ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      sfSpecs = new JSFunctionSpec[staticFuncs.size() + 1] ;
      if ( NULL == sfSpecs )
      {
         ossPrintf( "failed to allocate mem."OSS_NEWLINE ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      {
         UINT32 i = 0 ;
         sptFuncMap::NORMAL_FUNCS::const_iterator itr = memberFuncs.begin() ;
         for ( ; i < memberFuncs.size() ; i++, itr++ )
         {
            fSpecs[i].name = itr->first.c_str() ;
            fSpecs[i].call = itr->second._pFunc ;
            fSpecs[i].nargs = 0 ;
            fSpecs[i].flags = itr->second._attr ;
         }
         fSpecs[i].name = NULL ;
         fSpecs[i].call = NULL ;
         fSpecs[i].nargs = 0 ;
         fSpecs[i].flags = 0 ;

         i = 0 ;
         itr = staticFuncs.begin() ;
         for ( ; i < staticFuncs.size() ; i++, itr++ )
         {
            sfSpecs[i].name = itr->first.c_str() ;
            sfSpecs[i].call = itr->second._pFunc ;
            sfSpecs[i].nargs = 0 ;
            sfSpecs[i].flags = itr->second._attr ;
         }
         sfSpecs[i].name = NULL ;
         sfSpecs[i].call = NULL ;
         sfSpecs[i].nargs = 0 ;
         sfSpecs[i].flags = 0 ;

         prototype = JS_InitClass( _context, /// context
                                   _global,  /// object
                                   parent_proto,  /// parent_proto
                                   (JSClass*)desc->getClassDef(), /// class
                                   construct, /// constructor
                                   0, /// nargs
                                   0, /// ps
                                   fSpecs, /// fs
                                   0, /// static_ps
                                   sfSpecs /// static_fs
                                   ) ;

         if ( !prototype )
         {
            ossPrintf( "failed to call js_initclass"OSS_NEWLINE ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         _addPrototype( objName, prototype ) ;
      }

   done:
      if ( NULL != fSpecs )
      {
         delete []fSpecs ;
      }
      if ( NULL != sfSpecs )
      {
         delete []sfSpecs ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptSPScope::eval( const CHAR *code, UINT32 len,
                            const CHAR *filename,
                            UINT32 lineno,
                            INT32 flag,
                            const sptResultVal **ppRval )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT ( _context && _global, "this scope has not been initilized" ) ;
      SDB_ASSERT( NULL != code || 0 < len, "code can not be empty" ) ;
      jsval exception = JSVAL_VOID ;
      CHAR *print = NULL ;

      _rval.reset( _context ) ;
      jsval *pRval = ( jsval* )_rval.rawPtr() ;

      // set error report
      sdbSetPrintError( ( flag & SPT_EVAL_FLAG_PRINT ) ? TRUE : FALSE ) ;
      sdbSetNeedClearErrorInfo( TRUE ) ;

      if ( !JS_EvaluateScript( _context, _global, code,
                               len, filename, lineno,
                               pRval ) )
      {
         rc = sdbGetErrno() ? sdbGetErrno() : SDB_SPT_EVAL_FAIL ;
         goto error ;
      }

      if ( flag & SPT_EVAL_FLAG_PRINT )
      {
         if ( !JSVAL_IS_VOID ( *pRval ) )
         {
            print = convertJsvalToString ( _context , *pRval ) ;
            if ( !print )
            {
               rc = SDB_SYS ;
               goto error ;
            }
         }

         if ( NULL != print && print[0] != '\0' )
         {
            ossPrintf( "%s"OSS_NEWLINE, print ) ;
         }
      }

      // clear return error
      if ( sdbIsNeedClearErrorInfo() &&
           !JS_IsExceptionPending( _context ) )
      {
         sdbClearErrorInfo() ;
      }

   done:
      if ( ppRval )
      {
         *ppRval = &_rval ;
      }
      SAFE_JS_FREE ( _context , print ) ;
      return rc ;
   error:
      if ( JS_IsExceptionPending( _context ) &&
           JS_GetPendingException ( _context , &exception ) )
      {
         CHAR *strException = NULL ;
         JSString *jsstr = JS_ValueToString( _context, exception ) ;
         if ( NULL != jsstr )
         {
            strException = JS_EncodeString ( _context, jsstr ) ;
         }

         if ( NULL != strException )
         {
            std::stringstream ss ;
            ss << "Uncaught exception:" ;
            ss << strException ;
            std::string errInfo = ss.str() ;
            _rval.setError( errInfo ) ;
            sdbReportError( NULL, 0, errInfo.c_str(), TRUE ) ;
            SAFE_JS_FREE( _context, strException ) ;
         }
         JS_ClearPendingException ( _context ) ;
      }
      goto done ;
   }

   BOOLEAN _sptSPScope::isInstanceOf( const void *pObj,
                                      const string &objName )
   {
      JSObject *pJSObj = ( JSObject* )pObj ;
      return sptGetObjFactory()->isInstanceOf( _context, pJSObj, objName ) ;
   }

   string _sptSPScope::getObjClassName( const void *pObj )
   {
      JSObject *pJSObj = ( JSObject* )pObj ;
      return sptGetObjFactory()->getClassName( _context, pJSObj ) ;
   }

   void _sptSPScope::getGlobalFunNames( set< string > &setFunc,
                                        BOOLEAN showHide )
   {
      sptGetObjFactory()->getObjStaticFunNames( _context, _global,
                                                setFunc, showHide ) ;
   }

   void _sptSPScope::getObjStaticFunNames( const string &objName,
                                           set< string > &setFunc,
                                           BOOLEAN showHide )
   {
      sptGetObjFactory()->getClassStaticFuncNames( _context, objName,
                                                   setFunc, showHide ) ;
   }

   void _sptSPScope::getObjFunNames( const void *pObj,
                                     set< string > &setFunc,
                                     BOOLEAN showHide )
   {
      JSObject *pJSObj = ( JSObject* )pObj ;
      return sptGetObjFactory()->getObjFuncNames( _context, pJSObj,
                                                  setFunc, showHide ) ;
   }

   void _sptSPScope::getObjPropNames( const void *pObj,
                                      set< string > &setProp )
   {
      JSObject *pJSObj = ( JSObject* )pObj ;
      return sptGetObjFactory()->getObjPropNames( _context, pJSObj, setProp ) ;
   }

   void _sptSPScope::_addPrototype( const string &name,
                                    const JSObject *obj )
   {
      _mapName2Proto[name] = obj ;
   }

   const JSObject* _sptSPScope::_getPrototype( const string &name ) const
   {
      MAP_NAME_2_PROTOTYPE::const_iterator it = _mapName2Proto.find( name ) ;
      if ( it != _mapName2Proto.end() )
      {
         return it->second ;
      }
      return NULL ;
   }

   BOOLEAN _sptSPScope::_hasPrototype( const string &name ) const
   {
      MAP_NAME_2_PROTOTYPE::const_iterator it = _mapName2Proto.find( name ) ;
      if ( it != _mapName2Proto.end() )
      {
         return TRUE ;
      }
      return FALSE ;
   }
}

