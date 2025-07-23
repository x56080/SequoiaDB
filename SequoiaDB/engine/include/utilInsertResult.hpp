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

   Source File Name = utilInsertResult.hpp

   Dependencies: N/A

   Restrictions: N/AdmsStorageDataCommon

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          02/13/2019   LYB Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_INSERT_RESULT_HPP_
#define UTIL_INSERT_RESULT_HPP_

#include "oss.hpp"
#include "../bson/bson.hpp"
#include "utilResult.hpp"

using namespace bson ;

namespace engine
{
   class utilIdxDupErrInfo : public SDBObject
   {
   public:
      utilIdxDupErrInfo( BSONObj errorInfo ) ;
      ~utilIdxDupErrInfo() ;

   public:
      const CHAR *getIdxName() ;
      BSONObj getIdxValue() ;
      BSONObj getIdxMatcher() ;
      BSONObj getIdxPattern() ;

   private:
      BSONObj _errInfo ;
   } ;

   class utilInsertResult : public utilWriteResult
   {
   public:
      utilInsertResult() ;
      ~utilInsertResult() ;

   public:
      void enableDupErrInfo() ;
      void disableDupErrInfo() ;
      BOOLEAN isEnaleDupErrInfo() ;
      void setDupErrInfo( const CHAR *idxName, const BSONObj& idxKeyPattern,
                          const BSONObj& idxValueWithoutKey ) ;

      BSONObj getErrorInfo() ;

   private:
      BOOLEAN _isEnableDupErrInfo ;
      BSONObj _errInfo ;
   } ;
}

#endif /* UTIL_INSERT_RESULT_HPP_ */

