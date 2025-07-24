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

   Source File Name = sptSPObject.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          25/01/2018  WJM  Initial Draft

   Last Changed =

*******************************************************************************/
#include "sptObject.hpp"
#include "jsapi.h"

namespace engine
{
   /*
      sptSPObject define
   */
   class sptSPObject: public sptObject
   {
   public:
      sptSPObject( JSContext *cx, JSObject *obj ) ;

      virtual ~sptSPObject() ;

      INT32 getObjectField( const std::string &fieldName,
                            sptObjectPtr &objPtr ) const ;

      INT32 getBoolField( const std::string &fieldName, BOOLEAN &rval,
                          INT32 mask = SPT_CVT_FLAGS_FROM_BOOL ) const ;

      INT32 getIntField( const std::string &fieldName, INT32 &rval,
                         INT32 mask = SPT_CVT_FLAGS_FROM_INT ) const ;

      INT32 getDoubleField( const std::string &fieldName, FLOAT64 &rval,
                            INT32 mask = SPT_CVT_FLAGS_FROM_DOUBLE ) const ;

      INT32 getStringField( const std::string &fieldName, string &rval,
                            INT32 mask = SPT_CVT_FLAGS_FROM_STRING ) const ;

      INT32 getUserObj( const sptObjDesc &objDesc, const void** value ) const ;

      INT32 toBSON( bson::BSONObj &rval, BOOLEAN strict = TRUE ) const ;

      INT32 toString( std::string &rval ) const ;

      INT32 getFieldType( const std::string &fieldName,
                          SPT_JS_TYPE &type ) const ;

      BOOLEAN isFieldExist( const std::string &fieldName ) const ;

      INT32 getFieldNumber( UINT32 &number ) const ;

      INT32 getDesc( const sptObjDesc **pDesc ) const ;

   protected:
      INT32 _getTypeOfVal( jsval val, SPT_JS_TYPE &type ) const ;

      INT32 _getObjectDesc( JSObject* obj, BOOLEAN &isSpecialObj,
                            const sptObjDesc **pDesc ) const ;
   private:
      JSContext *_cx ;
      JSObject *_obj ;
   } ;
}