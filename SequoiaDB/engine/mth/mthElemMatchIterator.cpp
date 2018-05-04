/******************************************************************************

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

   Source File Name = mthElemMatchIterator.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/01/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "mthElemMatchIterator.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "mthTrace.hpp"
#include "mthMatchTree.hpp"

using namespace bson ;

namespace engine
{
   _mthElemMatchIterator::_mthElemMatchIterator( const bson::BSONObj &obj,
                                                 _mthMatchTree *matcher,
                                                 INT32 n,
                                                 BOOLEAN isArray )
   :_matcher( matcher ),
    _obj( obj ),
    _i( _obj ),
    _n( n ),
    _matched( 0 ),
    _isArray( isArray )
   {
      SDB_ASSERT( NULL != _matcher, "can not be null" ) ;
   }

   _mthElemMatchIterator::~_mthElemMatchIterator()
   {

   }

   INT32 _mthElemMatchIterator::next( bson::BSONElement &e )
   {
      INT32 rc = SDB_OK ;
      e = BSONElement() ;

      while ( _i.more() && ( _n < 0 || _matched < _n ) )
      {
         BSONElement ele = _i.next() ;
         BSONObj matchTarget ;
         if ( _isArray && Object == ele.type() )
         {
            matchTarget = ele.embeddedObject() ;
         }
         else if ( !_isArray )
         {
            matchTarget = ele.wrap() ;
         }
         else
         {
            continue ;
         }

         BOOLEAN res = FALSE ;
         rc = _matcher->matches( matchTarget, res ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to match obj:%d", rc ) ;
            goto error ;
         }
         else if ( res )
         {
            e = ele ;
            ++_matched ;
            break ;
         }
      }

      if ( e.eoo() )
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }
   done:
      return rc ;
   error:
      e = BSONElement() ;
      goto done ;
   }
}

