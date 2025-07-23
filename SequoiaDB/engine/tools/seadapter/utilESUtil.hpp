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

   Source File Name = utilESUtil.hpp

   Descriptive Name = Elasticsearch utility.

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for sdbcm,
   which is used to do cluster managing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/03/2018  YSD  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_ESUTIL_HPP__
#define UTIL_ESUTIL_HPP__

#include "core.hpp"
#include "../bson/bson.hpp"
#include <string>
#include <vector>

using namespace bson ;

namespace seadapter
{
   // Elasticsearch field datatypes.
   // Note: Any change here should also change getTypeStr.
   enum ES_DATA_TYPE
   {
      ES_TEXT,
      ES_KEYWORD,
      ES_WILDCARD,
      ES_MULTI_FIELDS,
      ES_DATE,
      ES_LONG,
      ES_DOUBLE,
      ES_BOOLEAN,
      ES_IP,

      ES_OBJECT,
      ES_NESTED,

      ES_GEO_POINT,
      ES_GEO_SHAPE,
      ES_COMPLETION
   } ;

   class _utilESMapping
   {
      public:
         _utilESMapping() ;
         ~_utilESMapping() ;

         INT32 toObj( BSONObj &mapObj ) const ;

         INT32 generateIndexMapping( const BSONObj &mappings ) ;

      private:
         INT32 _processField( const BSONElement &eField ) ;

         INT32 _generateDefaultIndexTemplate() ;
         INT32 _generateStringTemplate() ;
         INT32 _generateDoubleTemplate() ;

      private:
         ossPoolVector< BSONObj > _properties ;
         ossPoolVector< BSONObj > _templates ;
   } ;
   typedef _utilESMapping utilESMapping ;

   void encodeID( const BSONElement &idEle, string &id ) ;
   INT32 decodeID( const CHAR *id, CHAR *raw, UINT32 &len, BSONType &type ) ;

   INT32 seGetStringElement ( const BSONObj &obj, const CHAR *fieldName,
                              const CHAR **value ) ;

   INT32 seGetObjElement ( const BSONObj &obj, const CHAR *fieldName,
                           BSONObj &value ) ;
}

#endif /* UTIL_ESUTIL_HPP__ */

