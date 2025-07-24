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

   Source File Name = sptConvertor2.hpp

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

class sptConvertor2
{
public:
   sptConvertor2( JSContext *cx )
   :_cx( cx )
   {

   }

   ~sptConvertor2()
   {
      _cx = NULL ;
   }

public:
   INT32 toBson( JSObject *obj , bson::BSONObj &bsobj ) ;

   static INT32 toString( JSContext *cx,
                          const jsval &val,
                          std::string &str ) ;

private:
   INT32 _traverse( JSObject *obj , bson::BSONObjBuilder &builder ) ;

   INT32 _appendToBson( const std::string &name,
                        const jsval &val,
                        bson::BSONObjBuilder &builder ) ;

   INT32 _getDecimalPrecision( const CHAR *precisionStr, 
                               INT32 *precision, INT32 *scale ) ;
   BOOLEAN _addSpecialObj( JSObject *obj,
                           const CHAR *key,
                           bson::BSONObjBuilder &builder ) ;

   BOOLEAN _getProperty( JSObject *obj,
                         const CHAR *fieldName,
                         JSType type,
                         jsval &val ) ;

   INT32 _toString( const jsval &val, std::string &str ) ;

   INT32 _toInt( const jsval &val, INT32 &iN ) ;

   INT32 _toDouble( const jsval &val, FLOAT64 &fV ) ;

   INT32 _toBoolean( const jsval &val, BOOLEAN &bL ) ;

   BOOLEAN _isValidNumberLong( const CHAR *value ) ;

private:
   JSContext *_cx ;
} ;

#endif

