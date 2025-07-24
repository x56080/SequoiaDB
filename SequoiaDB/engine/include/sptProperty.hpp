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

   Source File Name = sptProperty.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/03/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SPT_PROPERTY_HPP_
#define SPT_PROPERTY_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "../bson/bson.hpp"
#include <vector>

namespace engine
{
   typedef void (*SPT_RELEASE_OBJ_FUNC)(void *instance) ;

   /*
      _sptProperty define
   */
   class _sptProperty : public SDBObject
   {
   public:
      _sptProperty() ;
      _sptProperty( const _sptProperty &other ) ;
      _sptProperty &operator=(const _sptProperty &other) ;
      virtual ~_sptProperty() ;

   public:
      /// BOOLEAN, INT32, FLOAT64
      INT32 assignNative( const CHAR *name,
                          bson::BSONType type,
                          const void *value ) ;

      /// value should be base64 coded when
      /// it is a binary data.
      INT32 assignString( const CHAR *name,
                          const CHAR *value ) ;

      INT32 assignBsonobj( const CHAR *name,
                           const bson::BSONObj &value ) ;

      INT32 assignBsonArray( const CHAR *name,
                             const std::vector< bson::BSONObj > &vecObj ) ;

      /// WARNING: value will be registered in
      /// engine and released in JS_Destructor.
      INT32 assignUsrObject( const CHAR *name,
                             void *value ) ;

      INT32 getNative( bson::BSONType type,
                       void *value ) const ;

      /// copy value if u want to modify or keep it.
      const CHAR *getString() const ;

      inline bson::BSONType getType() const
      {
         return _type ;
      }

      inline void *getValue() const
      {
         return ( void * )_value ;
      }

      inline const std::string &getName() const
      {
         return _name ;
      }

      inline void releaseObj()
      {
         if ( bson::Object == _type && 0 != _value && _pReleaseFunc )
         {
            _pReleaseFunc( (void*)_value ) ;
            _value = 0 ;
            _pReleaseFunc = NULL ;
            _type = bson::EOO ;
         }
      }

   private:
      std::string _name ;
      UINT64 _value ;
      bson::BSONType _type ;
      SPT_RELEASE_OBJ_FUNC _pReleaseFunc ;
      
   } ;
   typedef class _sptProperty sptProperty ;
}

#endif // SPT_PROPERTY_HPP_

