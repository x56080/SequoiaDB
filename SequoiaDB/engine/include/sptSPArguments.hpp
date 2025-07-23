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

   Source File Name = sptSPArguments.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/03/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SPT_SPARGUMENTS_HPP_
#define SPT_SPARGUMENTS_HPP_

#include "sptArguments.hpp"
#include "jsapi.h"

namespace engine
{
   /*
      _sptSPArguments define
   */
   class _sptSPArguments : public _sptArguments
   {
   public:
      _sptSPArguments( JSContext *context, uintN argc, jsval *vp ) ;
      virtual ~_sptSPArguments() ;

   public:
      virtual INT32 getNative( UINT32 pos, void *value,
                               SPT_NATIVE_TYPE type ) const ;

      virtual INT32 getString( UINT32 pos, std::string &value,
                               BOOLEAN strict = TRUE ) const ;

      virtual INT32 getBsonobj( UINT32 pos, bson::BSONObj &value,
                                BOOLEAN strict = TRUE,
                                BOOLEAN allowNull = FALSE )
                                const ;

      virtual INT32 getArray( UINT32 pos, vector< bson::BSONObj > &value,
                              BOOLEAN strict = TRUE ) const ;

      virtual INT32 getArray( UINT32 pos, vector< string > &value,
                              BOOLEAN strict = TRUE ) const ;

      virtual INT32 getUserObj( UINT32 pos, const _sptObjDesc &objDesc,
                                const void** value ) const ;

      virtual sptPrivateData* getPrivateData() const ;

      virtual UINT32 argc() const
      {
         return _argc ;
      }

      virtual BOOLEAN isString( UINT32 pos ) const ;
      virtual BOOLEAN isInt( UINT32 pos ) const ;
      virtual BOOLEAN isBoolean( UINT32 pos ) const ;
      virtual BOOLEAN isDouble( UINT32 pos ) const ;
      virtual BOOLEAN isNumber( UINT32 pos ) const ;
      virtual BOOLEAN isObject( UINT32 pos ) const ;
      virtual BOOLEAN isNull( UINT32 pos ) const ;
      virtual BOOLEAN isVoid( UINT32 pos ) const ;
      virtual BOOLEAN isUserObj( UINT32 pos,
                                 const _sptObjDesc &objDesc ) const ;
      virtual BOOLEAN isArray( UINT32 pos ) const ;
      virtual string  getUserObjClassName( UINT32 pos ) const ;

      virtual string  getErrMsg() const ;
      virtual BOOLEAN hasErrMsg() const ;

   private:
      jsval *_getValAtPos( UINT32 pos ) const ;

   private:
      JSContext         *_context ;
      uintN             _argc ;
      jsval             *_vp ;
      mutable string    _errMsg ;
   } ;
}

#endif

