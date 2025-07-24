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

   Source File Name = utilInsertResult.cpp

   Descriptive Name = util insert error info

   When/how to use: N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          02/13/2019  LYB Initial Draft

   Last Changed =

*******************************************************************************/
#include "utilInsertResult.hpp"

using namespace bson ;

namespace engine
{
   static const CHAR *UTIL_INSERT_DUP_ERR          = "DupErrorInfo" ;
   static const CHAR *UTIL_INSERT_DUP_ERR_MATCHER  = "Matcher" ;
   static const CHAR *UTIL_INSERT_DUP_ERR_IDXNAME  = "IndexName" ;
   static const CHAR *UTIL_INSERT_DUP_ERR_IDXVALUE = "IndexValue" ;
   static const CHAR *UTIL_INSERT_DUP_ERR_PATTERN  = "Pattern" ;

   static const BSONObj INEXIST_OP                 = BSON( "$exists" << 0 ) ;

   utilInsertResult::utilInsertResult()
   {
      _isEnableDupErrInfo = FALSE ;
   }

   utilInsertResult::~utilInsertResult()
   {
   }

   void utilInsertResult::enableDupErrInfo()
   {
      _isEnableDupErrInfo = TRUE ;
   }

   void utilInsertResult::disableDupErrInfo()
   {
      _isEnableDupErrInfo = FALSE ;
   }

   BOOLEAN utilInsertResult::isEnaleDupErrInfo()
   {
      return _isEnableDupErrInfo ;
   }

   void utilInsertResult::setDupErrInfo( const CHAR *idxName,
                                         const BSONObj& idxKeyPattern,
                                         const BSONObj& idxValueWithoutKey )
   {
      if ( !isEnaleDupErrInfo() )
      {
         return ;
      }

      {
         BSONObjIterator keyIter( idxKeyPattern ) ;
         BSONObjIterator valueIter( idxValueWithoutKey ) ;
         BSONObjBuilder builder ;
         BSONObjBuilder dupInfoBuilder(
            builder.subarrayStart( UTIL_INSERT_DUP_ERR )) ;

         /*
           idxKeyPattern + idxValueWithoutKey = > matcher
           idxKeyPattern: {"a":1, "b":1}

           idxValueWithoutKey: {"":10, "":10}
           => matcher: {"a":10, "b":10}

           idxValueWithoutKey: {"":10, "":{$undefined:1}}
           => matcher: {"a":10, "b":{$exists:0}}
         */
         BSONObjBuilder matcherBuilder(
            dupInfoBuilder.subobjStart( UTIL_INSERT_DUP_ERR_MATCHER ) ) ;
         while ( keyIter.more() && valueIter.more() )
         {
            BSONElement ke = keyIter.next() ;
            BSONElement ve = valueIter.next() ;
            if ( ve.type() != Undefined )
            {
               matcherBuilder.appendAs( ve, ke.fieldName() );
            }
            else
            {
               matcherBuilder.append( ke.fieldName(), INEXIST_OP ) ;
            }
         }
         matcherBuilder.done() ;
         dupInfoBuilder.done() ;

         builder.append( UTIL_INSERT_DUP_ERR_IDXNAME, idxName ) ;
         builder.append( UTIL_INSERT_DUP_ERR_PATTERN, idxKeyPattern ) ;

         _errInfo = builder.obj() ;
      }
   }

   BSONObj utilInsertResult::getErrorInfo()
   {
      return _errInfo ;
   }

   utilIdxDupErrInfo::utilIdxDupErrInfo( BSONObj errorInfo )
   {
      _errInfo = errorInfo ;
   }

   utilIdxDupErrInfo::~utilIdxDupErrInfo()
   {
   }

   const CHAR *utilIdxDupErrInfo::getIdxName()
   {
      BSONObj dupErrInfo = _errInfo.getObjectField( UTIL_INSERT_DUP_ERR ) ;
      return dupErrInfo.getStringField( UTIL_INSERT_DUP_ERR_IDXNAME ) ;
   }

   BSONObj utilIdxDupErrInfo::getIdxMatcher()
   {
      BSONObj dupErrInfo = _errInfo.getObjectField( UTIL_INSERT_DUP_ERR ) ;
      return dupErrInfo.getObjectField( UTIL_INSERT_DUP_ERR_MATCHER ) ;
   }

   BSONObj utilIdxDupErrInfo::getIdxValue()
   {
      BSONObj dupErrInfo = _errInfo.getObjectField( UTIL_INSERT_DUP_ERR ) ;
      return dupErrInfo.getObjectField( UTIL_INSERT_DUP_ERR_IDXVALUE ) ;
   }

   BSONObj utilIdxDupErrInfo::getIdxPattern()
   {
      BSONObj dupErrInfo = _errInfo.getObjectField( UTIL_INSERT_DUP_ERR ) ;
      return dupErrInfo.getObjectField( UTIL_INSERT_DUP_ERR_PATTERN ) ;
   }
}


