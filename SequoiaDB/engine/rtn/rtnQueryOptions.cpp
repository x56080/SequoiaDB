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

   Source File Name = rtnQueryOptions.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   user command processing on coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          27/05/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "rtnQueryOptions.hpp"
#include "ossUtil.hpp"
#include "msgMessage.hpp"
#include <sstream>

using namespace bson ;

namespace engine
{
   _rtnQueryOptions::~_rtnQueryOptions()
   {
      if ( NULL != _fullNameBuf )
      {
         SDB_OSS_FREE( _fullNameBuf ) ;
      }

      _fullName = NULL ;
      _fullNameBuf = NULL ;
   }

   INT32 _rtnQueryOptions::getOwned()
   {
      INT32 rc = SDB_OK ;
      if ( NULL != _fullNameBuf )
      {
         SDB_OSS_FREE( _fullNameBuf ) ;
         _fullNameBuf = NULL ;
      }

      if ( NULL != _fullName )
      {
         _fullNameBuf = ossStrdup( _fullName ) ;
         if ( NULL == _fullNameBuf )
         {
            rc = SDB_OOM ;
            goto error ;
         }
      }

      _fullName = _fullNameBuf ;
      _query = _query.getOwned() ;
      _selector = _selector.getOwned() ;
      _orderBy = _orderBy.getOwned() ;
      _hint = _hint.getOwned() ;
   done:
      return rc ;
   error:
      goto done ;
   }

   _rtnQueryOptions &_rtnQueryOptions::operator=( const _rtnQueryOptions &o )
   {
      _query = o._query ;
      _selector = o._selector ;
      _orderBy = o._orderBy ;
      _hint = o._hint ;
      _fullName = o._fullName ;
      if ( NULL != _fullNameBuf )
      {
         SDB_OSS_FREE( _fullNameBuf ) ;
         _fullNameBuf = NULL ;
      }
      _skip = o._skip ;
      _limit = o._limit ;
      _flag = o._flag ;
      _enablePrefetch = o._enablePrefetch ;
      return *this ;
   }

   string _rtnQueryOptions::toString() const
   {
      stringstream ss ;
      if ( _fullName )
      {
         ss << "Name: " << _fullName ;
         ss << ", Query: " << _query.toString() ;
         ss << ", Selector: " << _selector.toString() ;
         ss << ", OrderBy: " << _orderBy.toString() ;
         ss << ", Hint: " << _hint.toString() ;
         ss << ", Skip: " << _skip ;
         ss << ", Limit: " << _limit ;
         ss << ", Flags: " << _flag ;
      }
      return ss.str() ;
   }

   INT32 _rtnQueryOptions::fromQueryMsg( CHAR *pMsg )
   {
      INT32 rc = SDB_OK ;
      CHAR *pQuery = NULL ;
      CHAR *pSelector = NULL ;
      CHAR *pOrderBy = NULL ;
      CHAR *pHint = NULL ;

      rc = msgExtractQuery( pMsg, &_flag, (CHAR**)&_fullName, &_skip, &_limit,
                            &pQuery, &pSelector, &pOrderBy, &pHint ) ;
      PD_RC_CHECK( rc, PDERROR, "Extrace query msg failed, rc: %d", rc ) ;

      if ( NULL != _fullNameBuf )
      {
         SDB_OSS_FREE( _fullNameBuf ) ;
         _fullNameBuf = NULL ;
      }

      try
      {
         _query = BSONObj( pQuery ) ;
         _selector = BSONObj( pSelector ) ;
         _orderBy = BSONObj( pOrderBy ) ;
         _hint = BSONObj( pHint ) ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Extrace query msg occur exception: %s",
                 e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnQueryOptions::toQueryMsg( CHAR **ppMsg, INT32 &buffSize ) const
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( ppMsg, "ppMsg can't be NULL" ) ;

      rc = msgBuildQueryMsg( ppMsg, &buffSize, _fullName, _flag, 0,
                             _skip, _limit, &_query, &_selector, &_orderBy,
                             &_hint ) ;
      PD_RC_CHECK( rc, PDERROR, "Build query msg failed, rc: %d", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

}

