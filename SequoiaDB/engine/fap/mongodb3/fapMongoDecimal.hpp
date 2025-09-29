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

   Source File Name = fapMongoDecimal.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who        Description
   ====== =========== ========== ==============================================
          2020/06/18  fangjiabin Initial Draft

   Last Changed =

*******************************************************************************/
#include "ossUtil.hpp"
#include "../../bson/bson.hpp"
#include "fapMongodef.hpp"

#ifndef _FAP_MONGO_DECIMAL_HPP_
#define _FAP_MONGO_DECIMAL_HPP_

using namespace bson ;

namespace fap
{
// decimalBID means decimal in binary integer encoding
// It's based on IEEE 754-2008 standards.

#define FAP_MONGO_BSON_DECIMALBID_TYPE   19
// decimal in BID encoding has 128 bits
#define FAP_MONGO_DECIAMLOID_SIZE        16

class _mongoBSONObjIterator
{
public:
   _mongoBSONObjIterator( const BSONObj& jso )
   {
      int sz = jso.objsize() ;
      if ( 0 == sz )
      {
         _pos = _theend = 0 ;
         return ;
      }
      _pos = jso.objdata() + 4 ;
      _theend = jso.objdata() + sz - 1 ;
   }

   _mongoBSONObjIterator()
   {
      _pos    = NULL ;
      _theend = NULL ;
   }

   /** @return true if more elements exist to be enumerated. */
   bool more() { return _pos < _theend ; }

   BSONElement next()
   {
      assert( _pos <= _theend ) ;
      BSONElement e( _pos ) ;
      if ( FAP_MONGO_BSON_DECIMALBID_TYPE == e.type() )
      {
         // e.size = type(1) + fieldName(strlen(fieldName)+1) + dataSize(16)
         _pos +=
         ( 1 + ossStrlen( e.fieldName() ) + 1 + FAP_MONGO_DECIAMLOID_SIZE ) ;
      }
      else
      {
         _pos += e.size() ;
      }
      return e;
   }

   BSONElement operator*()
   {
      assert( _pos <= _theend ) ;
      return BSONElement( _pos ) ;
   }

private:
   const CHAR* _pos ;
   const CHAR* _theend ;
};
typedef _mongoBSONObjIterator mongoBSONObjIterator ;

INT32   sdbDecimal2MongoDecimal( const BSONObj &sdbRecord,
                                 BSONObjBuilder &mongoRecordBob,
                                 BOOLEAN &hasDecimal ) ;

INT32   sdbDecimal2MongoDecimal( const BSONObj &sdbRecord,
                                 BSONArrayBuilder &mongoRecordBab,
                                 BufBuilder &mongoRecordBb ) ;

INT32   mongoDecimal2SdbDecimal( const BSONObj &mongoMsgObj,
                                 BSONObjBuilder &sdbMsgObjBob ) ;

INT32   mongoDecimal2SdbDecimal( const BSONObj &mongoMsgObj,
                                 BSONArrayBuilder &sdbMsgObjBab ) ;

BOOLEAN mongoIsSupportDecimal( const mongoClientInfo &clientInfo ) ;
}

#endif