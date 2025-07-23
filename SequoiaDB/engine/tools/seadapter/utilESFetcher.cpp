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

   Source File Name = utilESFetcher.cpp

   Descriptive Name = Elasticsearch fetcher.

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for sdbcm,
   which is used to do cluster managing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/16/2018  YSD  Initial Draft

   Last Changed =

*******************************************************************************/
#include "utilESFetcher.hpp"
#include "pd.hpp"
#include "pmdEDU.hpp"

using namespace engine ;

#define ES_SCROLL_FIX_SIZE          10000

namespace seadapter
{
   _utilESFetcher::_utilESFetcher( const CHAR *index, const CHAR *type )
   {
      SDB_ASSERT( index, "Index is NULL" ) ;
      SDB_ASSERT( type, "type is NULL" ) ;
      _clt = NULL ;
      _index = std::string( index ) ;
      _type = std::string( type ) ;
      _size = 0 ;
   }

   _utilESFetcher::~_utilESFetcher()
   {
   }

   INT32 _utilESFetcher::setClt( utilESClt *esClt )
   {
      INT32 rc = SDB_OK ;

      if ( !esClt )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "esClt is NULL" ) ;
         goto error ;
      }
      _clt = esClt ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilESFetcher::setCondition( const BSONObj &condObj )
   {
      INT32 rc = SDB_OK ;
      try
      {
         _query = condObj.copy() ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _utilESFetcher::setSize( INT64 size )
   {
      _size = size ;
   }

   void _utilESFetcher::setFilterPath( const CHAR *filterPath )
   {
      SDB_ASSERT( filterPath, "filter path is NULL" ) ;
      _filterPath = filterPath ;
   }

   _utilESPageFetcher::_utilESPageFetcher( const CHAR *index, const CHAR *type )
   : _utilESFetcher( index, type )
   {
      _from = 0 ;
      _fetchDone =  FALSE ;
   }

   _utilESPageFetcher::~_utilESPageFetcher()
   {
   }

   void _utilESPageFetcher::setFrom( INT64 from )
   {
      _from = from ;
   }

   INT32 _utilESPageFetcher::fetch( utilCommObjBuff &result )
   {
      INT32 rc = SDB_OK ;
      UINT32 origNum = result.getObjNum() ;

      if ( !_fetchDone )
      {
         rc = _clt->getDocument( _index.c_str(), _type.c_str(),
                                 _query.toString( FALSE, TRUE ).c_str(),
                                 result, FALSE ) ;
         if ( rc )
         {
            const CHAR *errMsg = _clt->getLastErrMsg() ;
            if ( errMsg )
            {
               PD_LOG_MSG( PDERROR, "Get document of index[ %s ] and "
                           "type[ %s ] failed[ %d ], query string: %s. "
                           "Error message: %s",
                           _index.c_str(), _type.c_str(), rc,
                           _query.toString( FALSE, TRUE ).c_str(),
                           _clt->getLastErrMsg() ) ;
            }
            else
            {
               PD_LOG_MSG( PDERROR, "Get document of index[ %s ] and "
                           "type[ %s ] failed[ %d ], query string: %s.",
                           _index.c_str(), _type.c_str(), rc,
                           _query.toString( FALSE, TRUE ).c_str() ) ;
            }
            goto error ;
         }
         _fetchDone = TRUE ;

         if ( result.getObjNum() - origNum == 0 )
         {
            rc = SDB_DMS_EOC ;
            goto error ;
         }
      }
      else
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   _utilESScrollFetcher::_utilESScrollFetcher( const CHAR *index,
                                               const CHAR *type )
   : _utilESFetcher( index, type )
   {
   }

   _utilESScrollFetcher::~_utilESScrollFetcher()
   {
      // Clean the scroll on es to free resource.
      if ( !_scrollID.empty() )
      {
         _clt->clearScroll( _scrollID ) ;
      }
   }

   INT32 _utilESScrollFetcher::fetch( utilCommObjBuff &result )
   {
      INT32 rc = SDB_OK ;
      UINT32 origNum = result.getObjNum() ;

      if ( _scrollID.empty() )
      {
         // We just want the scroll id and document id. So use the filter path.
         rc = _clt->initScroll( _scrollID, _index.c_str(), _type.c_str(),
                                _query.toString( FALSE, TRUE ), result,
                                ES_SCROLL_FIX_SIZE, _filterPath.c_str() ) ;
         if ( rc )
         {
            const CHAR *errMsg = _clt->getLastErrMsg() ;
            if ( errMsg )
            {
               PD_LOG_MSG( PDERROR, "Initialize scroll for index[ %s ] and "
                           "type[ %s ] failed[ %d ], query string: %s. "
                           "Error message: %s",
                           _index.c_str(), _type.c_str(), rc,
                           _query.toString( FALSE, TRUE ).c_str(),
                           _clt->getLastErrMsg() ) ;
            }
            else
            {
               PD_LOG_MSG( PDERROR, "Initialize scroll for index[ %s ] and "
                           "type[ %s ] failed[ %d ], query string: %s.",
                           _index.c_str(), _type.c_str(), rc,
                           _query.toString( FALSE, TRUE ).c_str() ) ;
            }
            goto error ;
         }
      }
      else
      {
         rc = _clt->scrollNext( _scrollID, result, _filterPath.c_str() ) ;
         if ( rc )
         {
            const CHAR *errMsg = _clt->getLastErrMsg() ;
            if ( errMsg )
            {
               PD_LOG_MSG( PDERROR, "Scroll with id[ %s ] for index[ %s ] and "
                           "type[ %s ] failed[ %d ]. Error message: %s",
                           _scrollID.c_str(), _index.c_str(), _type.c_str(),
                           rc, _clt->getLastErrMsg() ) ;
            }
            else
            {
               PD_LOG_MSG( PDERROR, "Scroll with id[ %s ] for index[ %s ] and "
                           "type[ %s ] failed[ %d ].",
                           _scrollID.c_str(), _index.c_str(),
                           _type.c_str(), rc ) ;
            }
            goto error ;
         }
      }

      if ( result.getObjNum() - origNum == 0 )
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }
}

