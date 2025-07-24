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

   Source File Name = mthSliceIterator.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/01/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "mthSliceIterator.hpp"

using namespace bson ;

namespace engine
{
   _mthSliceIterator::_mthSliceIterator( const bson::BSONObj &obj,
                                         INT32 begin,
                                         INT32 limit )
   :_obj( obj ),
    _where( 0 ),
    _limit( limit ),
    _itr( _obj )
   {
      INT32 total = obj.nFields() ;
      _where = begin < 0 ? begin + total : begin ;
      if ( _where < 0 )
      {
         _where = 0 ;
      }

      while ( 0 != _where )
      {
         if ( _itr.more() )
         {
            _itr.next() ;
            --_where ;
         }
         else
         {
            _limit = 0 ;
            break ;
         }
      }
   }

   _mthSliceIterator::~_mthSliceIterator()
   {

   }

   BOOLEAN _mthSliceIterator::more()
   {
      return _limit != 0 &&
             _itr.more() ;
   }

   bson::BSONElement _mthSliceIterator::next()
   {
      if ( more() )
      {
         if ( 0 < _limit )
         {
            --_limit ;
         }
         return _itr.next() ;
      }
      else
      {
         return BSONElement() ;
      }
   }
}

