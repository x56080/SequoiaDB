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

   Source File Name = sptBsonobjArray.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/03/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SPT_BSONOBJ_ARRAY_HPP_
#define SPT_BSONOBJ_ARRAY_HPP_

#include "sptApi.hpp"
#include "../bson/bson.hpp"
#include <vector>

using namespace bson ;
using namespace std ;

namespace engine
{

   /*
      _sptBsonobjArray define
   */
   class _sptBsonobjArray : public SDBObject
   {
   JS_DECLARE_CLASS( _sptBsonobjArray )

   public:
      _sptBsonobjArray() ;
      _sptBsonobjArray( const vector< BSONObj > &vecObjs ) ;
      virtual ~_sptBsonobjArray() ;

      const vector< BSONObj >& getBsonArray() const { return _vecObj ; }

   public:

      static INT32 help( const _sptArguments &arg,
                         _sptReturnVal &rval,
                         bson::BSONObj &detail ) ;

      INT32 construct( const _sptArguments &arg,
                       _sptReturnVal &rval,
                       bson::BSONObj &detail ) ;
      INT32 destruct() ;

      INT32 size( const _sptArguments &arg,
                  _sptReturnVal &rval,
                  bson::BSONObj &detail ) ;

      INT32 more( const _sptArguments &arg,
                  _sptReturnVal &rval,
                  bson::BSONObj &detail ) ;

      INT32 next( const _sptArguments &arg,
                  _sptReturnVal &rval,
                  bson::BSONObj &detail ) ;

      INT32 pos( const _sptArguments &arg,
                 _sptReturnVal &rval,
                 bson::BSONObj &detail ) ;

      INT32 getIndex( const _sptArguments &arg,
                      _sptReturnVal &rval,
                      bson::BSONObj &detail ) ;

      INT32 resolve( const _sptArguments &arg,
                     UINT32 opcode,
                     BOOLEAN &processed,
                     string &callFunc,
                     BOOLEAN &setIDProp,
                     _sptReturnVal &rval,
                     bson::BSONObj &detail ) ;

   private:
      vector< BSONObj >          _vecObj ;
      UINT32                     _curPos ;

   } ;
   typedef _sptBsonobjArray sptBsonobjArray ;
}

#endif // SPT_BSONOBJ_ARRAY_HPP_

