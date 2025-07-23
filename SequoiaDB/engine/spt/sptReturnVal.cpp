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

   Source File Name = sptReturnVal.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/03/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "sptReturnVal.hpp"
#include "sptBsonobj.hpp"
#include "sptBsonobjArray.hpp"
#include "ossUtil.hpp"
#include "pd.hpp"

using namespace bson ;

namespace engine
{

   _sptReturnVal::_sptReturnVal()
   {
   }

   _sptReturnVal::~_sptReturnVal()
   {
      /// release
      UINT32 i = 0 ;
      for ( i = 0 ; i < _valProperties.size() ; ++i )
      {
         SDB_OSS_DEL _valProperties[ i ] ;
      }
      _valProperties.clear() ;

      for ( i = 0 ; i < _selfProperties.size() ; ++i )
      {
         SDB_OSS_DEL _selfProperties[ i ] ;
      }
      _selfProperties.clear() ;
   }

   sptProperty* _sptReturnVal::addReturnValProperty( const std::string &name,
                                                     UINT32 attr )
   {
      sptProperty *add = SDB_OSS_NEW sptProperty() ;
      if ( add )
      {
         add->setName( name ) ;
         add->setAttr( attr ) ;
         _valProperties.push_back( add ) ;
      }
      return add ;
   }

   sptProperty* _sptReturnVal::addSelfProperty( const std::string &name,
                                                UINT32 attr )
   {
      sptProperty *add = SDB_OSS_NEW sptProperty() ;
      if ( add )
      {
         add->setName( name ) ;
         add->setAttr( attr ) ;
         _selfProperties.push_back( add ) ;
      }
      return add ;
   }

   void _sptReturnVal::setReturnValName( const string &name )
   {
      _val.setName( name ) ;
   }

   void _sptReturnVal::setReturnValAttr( UINT32 attr )
   {
      _val.setAttr( attr ) ;
   }

   void _sptReturnVal::addSelfToReturnValProperty( const string &name,
                                                   UINT32 attr )
   {
      _val.addBackwardProp( name, attr ) ;
   }
}

