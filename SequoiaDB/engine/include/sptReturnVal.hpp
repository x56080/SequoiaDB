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

   Source File Name = sptReturnVal.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/03/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SPT_RETURNVAL_HPP_
#define SPT_RETURNVAL_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "sptProperty.hpp"
#include <vector>

namespace engine
{
   typedef std::vector<sptProperty> SPT_PROPERTIES ;

   class _sptReturnVal : public SDBObject
   {
   public:
      _sptReturnVal()
      : _classDef(NULL)
      {}

      virtual ~_sptReturnVal()
      {
         _classDef = NULL ;
      }

      INT32 setNativeVal( const CHAR *name,
                          bson::BSONType type,
                          const void *value ) ;

      INT32 setStringVal( const CHAR *name,
                          const CHAR *value ) ;

      INT32 setUsrObjectVal( const CHAR *name,
                             void *value,
                             const void *classDef ) ;

      INT32 setBSONObj( const CHAR *name,
                        const bson::BSONObj &obj ) ;

      INT32 setBSONArray( const CHAR *name,
                          const std::vector< bson::BSONObj > &vecObj ) ;

      const sptProperty &getVal() const
      {
         return _property ;
      }

      const void *getClassDef()const
      {
         return _classDef ;
      }

      void addReturnValProperty( const sptProperty &property )
      {
         _properties.push_back( property ) ;
      }
    
      const SPT_PROPERTIES &getValProperties()const
      {
         return _properties ;
      }

      void releaseObj()
      {
         _property.releaseObj() ;
      }

   private:
      /// property name in parent.
      /// if this field is assigned,
      /// return value will be set into parent object as a property.
      sptProperty _property ;
      const void *_classDef ;

      /// properties of return val.
      SPT_PROPERTIES _properties ;
   } ;

   typedef class _sptReturnVal sptReturnVal ;
}

#endif

