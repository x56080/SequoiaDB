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

   Source File Name = sptSPScope.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/03/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SPT_SPSCOPE_HPP_
#define SPT_SPSCOPE_HPP_

#include "sptScope.hpp"
#include "jsapi.h"
#include <map>

namespace engine
{

   /*
      _sptSPResultVal define
   */
   class _sptSPResultVal : public _sptResultVal
   {
      public:
         _sptSPResultVal() ;
         virtual ~_sptSPResultVal() ;

         virtual const void*     rawPtr() const ;
         virtual bson::BSONObj   toBSON() const ;

         void    reset( JSContext *ctx ) ;

      protected:
         INT32  _rval2obj( JSContext *cx,
                           const jsval &jsrval,
                           bson::BSONObj &rval ) const ;

      protected:
         jsval             _value ;
         JSContext         *_ctx ;

   } ;
   typedef _sptSPResultVal sptSPResultVal ;

   typedef map< string, const JSObject* >       MAP_NAME_2_PROTOTYPE ;
   /*
      _sptSPScope define
   */
   class _sptSPScope : public _sptScope
   {
   public:
      _sptSPScope() ;
      virtual ~_sptSPScope() ;

      virtual SPT_SCOPE_TYPE getType() const { return SPT_SCOPE_TYPE_SP ; }

      template<typename T>
      BOOLEAN isInstanceOf( JSContext *cx, JSObject *obj )
      {
         return T::__desc.isInstanceOf( cx, obj ) ;
      }

   public:
      virtual INT32 start( UINT32 loadMask = SPT_OBJ_MASK_ALL ) ;

      virtual void shutdown() ;

      JSContext *getContext()
      {
         return _context ;
      }

      JSObject *getGlobalObj()
      {
         return _global ;
      }

   public:
      virtual INT32 eval(const CHAR *code, UINT32 len,
                         const CHAR *filename,
                         UINT32 lineno,
                         INT32 flag,
                         const sptResultVal **ppRval ) ;

      virtual void   getGlobalFunNames( set<string> &setFunc,
                                        BOOLEAN showHide = FALSE ) ;

      virtual void   getObjStaticFunNames( const string &objName,
                                           set<string> &setFunc,
                                           BOOLEAN showHide = FALSE ) ;

      virtual void   getObjFunNames( const void *pObj,
                                     set<string> &setFunc,
                                     BOOLEAN showHide = FALSE ) ;

      virtual void   getObjPropNames( const void *pObj,
                                      set<string> &setProp ) ;

      virtual BOOLEAN   isInstanceOf( const void *pObj,
                                      const string &objName ) ;

      virtual string getObjClassName( const void *pObj ) ;

   private:
      virtual INT32 _loadUsrDefObj( _sptObjDesc *desc ) ;

      INT32 _loadObj( UINT32 loadMask ) ;

      INT32 _loadUsrClass( _sptObjDesc *desc ) ;

      INT32 _loadGlobal( _sptObjDesc *desc ) ;

      void  _addPrototype( const string &name,
                           const JSObject *obj ) ;

      const JSObject*   _getPrototype( const string &name ) const ;

      BOOLEAN           _hasPrototype( const string &name ) const ;

   private:
      JSRuntime *_runtime ;
      JSContext *_context ;
      JSObject *_global ;
      sptSPResultVal _rval ;
      MAP_NAME_2_PROTOTYPE _mapName2Proto ;

   } ;
   typedef class _sptSPScope sptSPScope ;
}

#endif

