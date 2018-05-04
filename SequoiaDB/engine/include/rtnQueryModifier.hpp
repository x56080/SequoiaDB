/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

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

using std::vector ;

namespace engine
{
   class _rtnQueryModifier: public SDBObject
   {
   public:
      _rtnQueryModifier( BOOLEAN isUpdate,
                         BOOLEAN isRemove,
                         BOOLEAN returnNew = FALSE ) ;

      ~_rtnQueryModifier()
      {
      }

      INT32 loadUpdator( const bson::BSONObj &updator ) ;

      inline BOOLEAN isUpdate() const { return _isUpdate ; }
      inline BOOLEAN isRemove() const { return _isRemove ; }
      inline BOOLEAN returnNew() const { return _returnNew ; }
      inline mthModifier& getModifier() { return _modifier ; }
      inline vector<INT64>* getDollarList() { return &_dollarList ; }

   private:
      // disallow copy and assign
      _rtnQueryModifier( const _rtnQueryModifier& ) ;
      void operator=( const _rtnQueryModifier& ) ;

   private:
      BOOLEAN        _isUpdate ;
      BOOLEAN        _isRemove ;
      BOOLEAN        _returnNew ;
      bson::BSONObj  _updator ;
      mthModifier    _modifier ;
      vector<INT64>  _dollarList ;
   } ;
   typedef class _rtnQueryModifier rtnQueryModifier ;
}

#endif /* RTN_QUERYMODIFIER_HPP_ */
