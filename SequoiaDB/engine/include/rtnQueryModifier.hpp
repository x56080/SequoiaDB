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

   Source File Name = rtnQueryModifier.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          24/3/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_QUERYMODIFIER_HPP_
#define RTN_QUERYMODIFIER_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "../bson/bson.hpp"
#include "mthModifier.hpp"
#include "rtnHintModifier.hpp"

using std::vector ;

namespace engine
{
   class _rtnQueryModifier: public rtnHintModifier
   {
   public:
      _rtnQueryModifier() ;

      ~_rtnQueryModifier()
      {
      }

      inline BOOLEAN isUpdate() const
      {
         return RTN_MODIFY_UPDATE == _modifyOp ;
      }

      inline BOOLEAN isRemove() const
      {
         return RTN_MODIFY_REMOVE == _modifyOp ;
      }

      inline BOOLEAN returnNew() const { return _returnNew ; }
      inline mthModifier& getModifier() { return _modifier ; }
      inline vector<INT64>* getDollarList() { return &_dollarList ; }

   private:
      // disallow copy and assign
      _rtnQueryModifier( const _rtnQueryModifier& ) ;
      void operator=( const _rtnQueryModifier& ) ;

      INT32 _parseModifyEle( const BSONElement &ele ) ;
      INT32 _onInit() ;

   private:
      BOOLEAN        _returnNew ;
      mthModifier    _modifier ;
      vector<INT64>  _dollarList ;
   } ;
   typedef class _rtnQueryModifier rtnQueryModifier ;
}

#endif /* RTN_QUERYMODIFIER_HPP_ */
