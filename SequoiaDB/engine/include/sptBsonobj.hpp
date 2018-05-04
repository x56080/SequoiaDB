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

   Source File Name = sptBsonobj.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/03/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SPT_BSONOBJ_HPP_
#define SPT_BSONOBJ_HPP_

#include "sptApi.hpp"
#include "../bson/bson.hpp"

namespace engine
{
   /*
      _sptBsonobj define
   */
   class _sptBsonobj : public SDBObject
   {
   JS_DECLARE_CLASS( _sptBsonobj )

   public:
      _sptBsonobj() ;
      _sptBsonobj( const bson::BSONObj &obj ) ;
      virtual ~_sptBsonobj() ;

      bson::BSONObj getBson() { return _obj ; }

   public:
      INT32 construct( const _sptArguments &arg,
                       _sptReturnVal &rval,
                       bson::BSONObj &detail) ;

      INT32 toJson( const _sptArguments &arg,
                    _sptReturnVal &rval,
                    bson::BSONObj &detail ) ;

      INT32 destruct() ;

   private:
      bson::BSONObj _obj ;
   } ;
   typedef _sptBsonobj sptBsonobj ;

}

#endif // SPT_BSONOBJ_HPP_

