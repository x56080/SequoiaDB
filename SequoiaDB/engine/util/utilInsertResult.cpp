/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

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


