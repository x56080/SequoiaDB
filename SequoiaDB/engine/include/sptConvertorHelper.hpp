/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = sptConvertorHelper.hpp

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

#ifndef SPTCONVERTORHELPER_HPP_
#define SPTCONVERTORHELPER_HPP_

#include "core.hpp"
#include "jsapi.h"
#include <string>

INT32 JSObj2BsonRaw( JSContext *cx, JSObject *obj, CHAR **raw ) ;

INT32 JSVal2String( JSContext *cx, const jsval &val, std::string &str ) ;

// caller should free the return pointer using SAFE_JS_FREE
CHAR *convertJsvalToString ( JSContext *cx , jsval val ) ;

BOOLEAN JSObjIsQuery( JSContext *cx, JSObject *obj ) ;

BOOLEAN JSObjIsCursor( JSContext *cx, JSObject *obj ) ;

BOOLEAN JSObjIsCS( JSContext *cx, JSObject *obj ) ;

BOOLEAN JSObjIsCL( JSContext *cx, JSObject *obj ) ;

BOOLEAN JSObjIsRN( JSContext *cx, JSObject *obj ) ;

BOOLEAN JSObjIsRG( JSContext *cx, JSObject *obj ) ;

BOOLEAN JSObjIsSdbObj( JSContext *cx, JSObject *obj ) ;

INT32 cursorNextRaw( void *cursor, CHAR **raw ) ;

INT32 JSObj2Cursor( JSContext *cx, JSObject *obj, void **cursor ) ;

BOOLEAN JSObjIsBsonobj( JSContext *cx, JSObject *obj ) ;

INT32 getBsonRawFromBsonClass( JSContext *cx, JSObject *obj, CHAR **raw ) ;

INT32 getCSNameFromObj( JSContext *cx, JSObject *obj,
                        CHAR **csName ) ;

INT32 getCLNameFromObj( JSContext *cx, JSObject *obj,
                        CHAR **clName ) ;

INT32 getRNNameFromObj( JSContext *cx, JSObject *obj,
                        CHAR **rnName ) ;

INT32 getRGNameFromObj( JSContext *cx, JSObject *obj,
                        CHAR **rgName ) ;

#endif

