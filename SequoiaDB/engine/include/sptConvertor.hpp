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

   Source File Name = sptConvertor.hpp

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
#ifndef SPTCONVERTOR2_HPP_
#define SPTCONVERTOR2_HPP_

#include "core.hpp"
#include "jsapi.h"
#include "../bson/bson.hpp"
#include <string>
#include "sptObjDesc.hpp"
#include "sptSPVal.hpp"

namespace engine
{
   /*
      sptConvertor define
   */
   class sptConvertor
   {
   public:
      sptConvertor( JSContext *cx, BOOLEAN strict = TRUE )
      :_cx( cx ),_hasSetErrMsg( FALSE ), _strict( strict )
      {
      }

      ~sptConvertor()
      {
         _cx = NULL ;
      }

   public:
      static INT32 toString( JSContext *cx,
                             const jsval &val,
                             std::string &str ) ;

   public:
      INT32 toBson( JSObject *obj,
                    bson::BSONObj &bsobj,
                    BOOLEAN *pIsArray = NULL ) ;

      INT32 toBson( const sptSPVal *pVal,
                    bson::BSONObj &obj,
                    BOOLEAN *pIsArray = NULL ) ;

      INT32 appendToBson( const std::string &key,
                          const sptSPVal &val,
                          bson::BSONObjBuilder &builder ) ;

      INT32 toObjArray( JSObject *obj, vector< bson::BSONObj > &bsArray ) ;
      INT32 toStrArray( JSObject *obj, vector< string > &bsArray ) ;

      const string& getErrMsg() const ;

   private:
      INT32 _traverse( JSObject *obj,
                       bson::BSONObjBuilder &builder ) ;

      INT32 _appendToBson( const std::string &name,
                           const sptSPVal &val,
                           bson::BSONObjBuilder &builder ) ;

      void _setErrMsg( const string &msg, BOOLEAN isReplace ) ;

   private:
      JSContext   *_cx ;
      BOOLEAN     _hasSetErrMsg ;
      string      _errMsg ;
      BOOLEAN     _strict ;
   } ;
}
#endif // SPTCONVERTOR2_HPP_

