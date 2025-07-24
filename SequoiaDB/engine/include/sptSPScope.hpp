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

namespace engine
{
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
      virtual INT32 start() ;

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
                         bson::BSONObj &rval,
                         bson::BSONObj &detail ) ;

   private:
      virtual INT32 _loadUsrDefObj( _sptObjDesc *desc ) ;

      INT32 _loadUsrClass( _sptObjDesc *desc ) ;

      INT32 _loadGlobal( _sptObjDesc *desc ) ;

      INT32 _rval2obj( JSContext *cx,
                       const jsval &jsrval,
                       bson::BSONObj &rval ) ;

   private:
      JSRuntime *_runtime ;
      JSContext *_context ;
      JSObject *_global ;
      JSErrorReporter _errReporter ;
   } ;
   typedef class _sptSPScope sptSPScope ;
}

#endif

