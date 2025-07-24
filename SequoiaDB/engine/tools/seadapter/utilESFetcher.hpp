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

   Source File Name = utilESFetcher.hpp

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
#ifndef UTIL_ESFETCHER_HPP__
#define UTIL_ESFETCHER_HPP__

#include "oss.hpp"
#include "../bson/bsonobj.h"
#include "utilCommObjBuff.hpp"
#include "utilESClt.hpp"
#include <string>
#include "utilESCltMgr.hpp"

using bson::BSONObj ;

namespace seadapter
{
   // ES fetcher is used to fetch data from Elasticsearch. Two options are
   // provided:
   // 1. Do pagination by using from + size
   // 2. Scrolling.

   class _utilESFetcher : public SDBObject
   {
      public:
         _utilESFetcher( const CHAR *index, const CHAR *type ) ;
         virtual ~_utilESFetcher() ;

         INT32 setCondition( const BSONObj &condObj ) ;
         void setSize( INT64 size ) ;
         void setFilterPath( const CHAR *filterPath ) ;

         virtual INT32 fetch( utilCommObjBuff &result ) = 0 ;

      protected:
         std::string    _index ;
         std::string    _type ;
         BSONObj        _query ;
         INT64          _size ;
         std::string    _filterPath ;
         utilESCltMgr  *_esCltMgr ;
   } ;
   typedef _utilESFetcher utilESFetcher ;

   // Do pagination with from + size.
   class _utilESPageFetcher : public utilESFetcher
   {
      public:
         _utilESPageFetcher( const CHAR *index, const CHAR *type ) ;
         virtual ~_utilESPageFetcher() ;

         void setFrom( INT64 from ) ;
         virtual INT32 fetch( utilCommObjBuff &result ) ;
      private:
         INT64       _from ;
         BOOLEAN     _fetchDone ;
   } ;
   typedef _utilESPageFetcher utilESPageFetcher ;

   // Do scrolling.
   class _utilESScrollFetcher : public utilESFetcher
   {
      public:
         _utilESScrollFetcher( const CHAR *index, const CHAR *type ) ;
         virtual ~_utilESScrollFetcher() ;
         virtual INT32 fetch( utilCommObjBuff &result ) ;

      private:
         std::string   _scrollID ;
   } ;
   typedef _utilESScrollFetcher utilESScrollFetcher ;
}

#endif /* UTIL_ESFETCHER_HPP__ */

