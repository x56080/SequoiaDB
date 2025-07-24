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

   Source File Name = spt.hpp

   Descriptive Name = Script Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Script component. This file contains structures for javascript
   engine wrapper

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/13/2013  MPQ Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SPT_HPP__
#define SPT_HPP__

#include "core.hpp"
#include "jsapi.h"
#include "oss.hpp"
#include "sptSPDef.hpp"

namespace engine
{

   class Scope : public SDBObject
   {
   public:
      Scope();
      ~Scope();

      BOOLEAN init();

      OSS_INLINE JSContext *context(){ return _context ;}

      INT32 getLastError() ;
      const CHAR* getLastErrMsg() ;

      /**
       * if len > 0, then assume code is a string of length len, which may
       * contain embeded \0;
       * if len = 0, then assume code is a C-style string ending with \0
       *
       * On success, *result points to the result of evalution, otherwise, it is
       * NOT modified.
       */
      INT32 evaluate ( const CHAR *code , UINT32 len ,
                       const CHAR *filename , UINT32 lineno ,
                       CHAR ** result = NULL,
                       INT32 printFlag = SPT_EVAL_FLAG_NONE ) ;

      INT32 evaluate2 ( const CHAR *code, UINT32 len, UINT32 lineno,
                        jsval *rval, CHAR **errMsg,
                        INT32 printFlag = SPT_EVAL_FLAG_NONE ) ;

   private:
      JSContext *_context;
      JSObject *_global;
   };

   class ScriptEngine : public SDBObject
   {
   public:
      ScriptEngine();
      ~ScriptEngine();

      BOOLEAN init();

      Scope *newScope();

      static ScriptEngine *globalScriptEngine();
      static void purgeGlobalScriptEngine();

      friend class Scope;
   private:
      JSRuntime *_runtime;
      JSErrorReporter _errorReporter ;
   };

} // namespace engine

JSBool InitDbClasses( JSContext *cx, JSObject *obj ) ;

#endif

