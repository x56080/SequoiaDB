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

   Source File Name = utilBsonHash.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/3/2015   YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_BSONHASH_HPP_
#define UTIL_BSONHASH_HPP_

#include "oss.hpp"
#include "core.hpp"
#include "../bson/bson.hpp"

namespace engine
{
   class _utilBSONHasherObsolete : public SDBObject
   {
   private:
      _utilBSONHasherObsolete() {}
      ~_utilBSONHasherObsolete() {}

   public:
      static UINT32 hash( const bson::BSONObj &obj,
                          UINT32 partitionBit = 0 ) ;

      static UINT32 hash( const bson::BSONElement &e ) ;

      static UINT32 hash( const bson::OID &oid ) ;

      static UINT32 hash( const void *v, UINT32 size ) ;

      static UINT32 hash( const CHAR *str ) ;
   } ;

   typedef class _utilBSONHasherObsolete BSON_HASHER_OBSOLETE ;

   class _utilBSONHasher : public SDBObject
   {
   private:
      _utilBSONHasher() {}
      ~_utilBSONHasher() {}

      static UINT32 _hashDecimal( UINT32 hashCode,
                                  const bson::bsonDecimal &decimal ) ;

      static BOOLEAN _isInDoubleRange( const bson::bsonDecimal &decimal ) ;

   public:
      static UINT32 hashObj( const bson::BSONObj &obj,
                             UINT32 partitionBit = 0 ) ;

      static UINT32 hashElement( const bson::BSONElement &e ) ;

      static UINT32 hashOid( const bson::OID &oid ) ;

      static UINT32 hash( const void *v, UINT32 size ) ;

      static UINT32 hashStr( const CHAR *str ) ;

      static UINT32 hashFLoat64( UINT32 hashCode, FLOAT64 value ) ;

      static UINT32 hashDecimal( UINT32 hashCode,
                                 const bson::bsonDecimal &decimal ) ;

      static UINT32 hashCombine( UINT32 x, UINT32 y ) ;

      static UINT32 hashObjV3( const bson::BSONObj &obj,
                               UINT32 partitionBit = 0 ) ;

      static UINT32 hashElementV3( const bson::BSONElement &e ) ;

      static UINT32 hashFLoat64V3( UINT32 hashCode, FLOAT64 value ) ;

      static UINT32 hashDecimalV3( UINT32 hashCode,
                                   const bson::bsonDecimal &decimal ) ;
   } ;

   typedef class _utilBSONHasher BSON_HASHER ;
}

#endif

