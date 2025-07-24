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
#include <string>

namespace engine
{
   typedef SPT_PROP_ARRAY  SPT_PROPERTIES ;

   /*
      _sptReturnVal define
   */
   class _sptReturnVal : public SDBObject
   {
   public:
      _sptReturnVal() ;
      ~_sptReturnVal() ;

      sptProperty& getReturnVal()
      {
         return _val ;
      }

      void setReturnValAttr( UINT32 attr ) ;
      void setReturnValName( const string &name ) ;

      sptProperty* addReturnValProperty( const std::string &name,
                                         UINT32 attr = SPT_PROP_DEFAULT ) ;
      sptProperty* addSelfProperty( const std::string &name,
                                    UINT32 attr = SPT_PROP_DEFAULT ) ;

      void addSelfToReturnValProperty( const std::string &name,
                                       UINT32 attr = SPT_PROP_DEFAULT ) ;

      const SPT_PROPERTIES& getReturnValProperties() const
      {
         return _valProperties ;
      }

      const SPT_PROPERTIES& getSelfProperties() const
      {
         return _selfProperties ;
      }

      template< typename T >
      INT32 setUsrObjectVal( void *value )
      {
         return _val.assignUsrObject< T >( value ) ;
      }

   private:
      /// Return val. If the val's field name is not empty, will
      /// add this val to self as property with the name
      sptProperty       _val ;

      /// properties of return val. In used only when _val is Object.
      /// Will ignored the item its name is empty.
      SPT_PROPERTIES    _valProperties ;

      /// properties of self. Will ignored the item its name is empty.
      SPT_PROPERTIES    _selfProperties ;

   } ;

   typedef class _sptReturnVal sptReturnVal ;
}

#endif // SPT_RETURNVAL_HPP_

