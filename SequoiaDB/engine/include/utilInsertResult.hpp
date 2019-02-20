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

