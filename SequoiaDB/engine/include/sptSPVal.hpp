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

   Source File Name = sptSPVal.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of Script component. This file contains structures for javascript
   engine wrapper

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/27/2018  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SPT_SP_VAL_HPP__
#define SPT_SP_VAL_HPP__

#include "core.hpp"
#include "jsapi.h"
#include <string>
#include "sptObjDesc.hpp"

namespace engine
{

   /*
      _sptSPVal define
   */
   class _sptSPVal : public SDBObject
   {
      public:
         _sptSPVal( JSContext *pContext = NULL,
                    const jsval &val = JSVAL_VOID ) ;
         virtual ~_sptSPVal() ;

         const jsval*   valuePtr() const ;

         void           reset( JSContext *pContext = NULL,
                               const jsval &val = JSVAL_VOID ) ;

      public:

         BOOLEAN     isNull() const ;
         BOOLEAN     isVoid() const ;
         BOOLEAN     isInt() const ;
         BOOLEAN     isDouble() const ;
         BOOLEAN     isNumber() const ;
         BOOLEAN     isString() const ;
         BOOLEAN     isBoolean() const ;

         BOOLEAN     isObject() const ;
         BOOLEAN     isFunctionObj() const ;
         BOOLEAN     isArrayObj() const ;

         /*
            Special Object
         */
         BOOLEAN     isSPTObject( BOOLEAN *pIsSpecial = NULL,
                                  string *pClassName = NULL,
                                  const sptObjDesc **ppDesc = NULL ) const ;

         INT32       toInt( INT32 &value ) const ;
         INT32       toDouble( FLOAT64 &value ) const ;
         INT32       toString( string &value ) const ;
         INT32       toBoolean( BOOLEAN &value ) const ;

      private:
         JSContext      *_pContext ;
         jsval          _value ;

   } ;
   typedef _sptSPVal sptSPVal ;

}
#endif // SPT_SP_VAL_HPP__

