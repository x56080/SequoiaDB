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

   Source File Name = sptObjDesc.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/03/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SPT_OBJDESC_HPP_
#define SPT_OBJDESC_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "sptFuncMap.hpp"
#include "jsapi.h"

namespace engine
{
   class _sptObjDesc : public SDBObject
   {
   public:
      _sptObjDesc()
      :_init(FALSE)
      {}

      virtual ~_sptObjDesc(){}
   public:
      const CHAR *getJSClassName() const
      {
         return _jsClassName.c_str() ;
      }

      const _sptFuncMap &getFuncMap()const
      {
         return _funcMap ;
      }

      const JSClass *getClassDef() const
      {
         return _init ? &_classDef : NULL ;
      }

      void setClassName( const CHAR *name )
      {
         _jsClassName.assign( name ) ;
      }

      void setClassDef( const JSClass &def )
      {
         _classDef = def ;
         _init = TRUE ;
      }

      BOOLEAN getIgnore() const
      {
         return _jsClassName.empty() ;
      }

      BOOLEAN isInstanceOf( JSContext *cx, JSObject *obj )
      {
         if ( !_init )
         {
            return FALSE ;
         }
         return JS_InstanceOf( cx, obj, &_classDef, NULL ) ;
      }

   protected:
      std::string _jsClassName ;
      _sptFuncMap _funcMap ;
      JSClass _classDef ;
      BOOLEAN _init ;
   } ;
   typedef class _sptObjDesc sptObjDesc ;
}

#endif

