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

   Source File Name = mthElemMatchIterator.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/01/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef MTH_ELEMMATCHITERATOR_HPP_
#define MTH_ELEMMATCHITERATOR_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "../bson/bson.hpp"

namespace engine
{
   class _mthMatchTree ;

   class _mthElemMatchIterator : public SDBObject
   {
   public:
      _mthElemMatchIterator( const bson::BSONObj &obj,
                             _mthMatchTree *matcher,
                             INT32 n = -1,
                             BOOLEAN isArray = TRUE ) ;
      ~_mthElemMatchIterator() ;

   public:
      INT32 next( bson::BSONElement &e ) ;

   private:
      _mthMatchTree *_matcher ;
      bson::BSONObj _obj ;
      bson::BSONObjIterator _i ;
      INT32 _n ;
      INT32 _matched ;
      BOOLEAN _isArray ;
   } ;
   typedef class _mthElemMatchIterator mthElemMatchIterator ;
}

#endif

