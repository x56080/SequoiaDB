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

   Source File Name = utilChangeStreamOptions.hpp

   Descriptive Name = Change Stream Options

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/01/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_CHANGE_STREAM_OPTIONS_HPP__
#define UTIL_CHANGE_STREAM_OPTIONS_HPP__

#include "oss.hpp"
#include "ossMemPool.hpp"
#include "ossTypes.h"
#include "utilMap.hpp"
#include "utilPooledAutoPtr.hpp"
#include "utilPooledObject.hpp"
#include "utilStreamToken.hpp"
#include "../bson/bson.hpp"

namespace engine
{

   /*
      UTIL_CHANGE_TYPE define
    */
   #define UTIL_CHANGE_TYPE_NONE       ( 0x00 )
   #define UTIL_CHANGE_TYPE_DML_RECORD ( 0x01 )
   #define UTIL_CHANGE_TYPE_DML_LOB    ( 0x02 )
   #define UTIL_CHANGE_TYPE_DDL        ( 0x04 )
   #define UTIL_CHANGE_TYPE_TRANS      ( 0x08 )
   #define UTIL_CHANGE_TYPE_ALL        ( 0xFF )
   #define UTIL_CHANGE_TYPE_DFT        ( ( UTIL_CHANGE_TYPE_DML_RECORD ) | \
                                         ( UTIL_CHANGE_TYPE_DDL ) )

   #define UTIL_CHANGE_TYPE_NAME_NONE        "none"
   #define UTIL_CHANGE_TYPE_NAME_DML_RECORD  "record"
   #define UTIL_CHANGE_TYPE_NAME_DML_LOB     "lob"
   #define UTIL_CHANGE_TYPE_NAME_DDL         "ddl"
   #define UTIL_CHANGE_TYPE_NAME_TRANS       "trans"
   #define UTIL_CHANGE_TYPE_NAME_ALL         "all"


   // default value of max wait time in seconds
   #define UTIL_CHANGE_STREAM_MAX_WAIT_TIME_DEF ( 1 )
   // default value of max wait time in milliseconds
   #define UTIL_CHANGE_STREAM_MAX_WAIT_TIME_MS_DEF \
                     ( ( UTIL_CHANGE_STREAM_MAX_WAIT_TIME_DEF ) * OSS_ONE_SEC )

   // default value of cache size in megabytes
   #define UTIL_CHANGE_STREAM_CACHE_SIZE_DEF    ( 32 )
   #define UTIL_CHANGE_STREAM_CACHE_SIZE_MIN    ( 0 )
   #define UTIL_CHANGE_STREAM_CACHE_SIZE_MAX    ( 2048 )
   // default value of cache size in bytes
   #define UTIL_CHANGE_STREAM_CACHE_SIZE_B_DEF \
                     ( ( UTIL_CHANGE_STREAM_CACHE_SIZE_DEF ) * 1024 * 1024 )

   /*
      _utilWatchCSNameSet define
    */
   // name set of watching collection spaces
   class _utilWatchCSNameSet : public ossPoolSet< _utilMapStringKey >
   {
   public:
      _utilWatchCSNameSet() = default ;
      ~_utilWatchCSNameSet() = default ;

      // check if collection space is watched
      BOOLEAN isCSWatched( const CHAR *csName ) const
      {
         return NULL != csName && count( csName ) > 0 ;
      }

      // check if collection is watched by watching its collection spaces
      BOOLEAN isCLWatched( const CHAR *clName ) const
      {
         if ( NULL != clName )
         {
            if ( size() == 1 )
            {
               return _isCLInCS( begin()->_pString, clName ) ;
            }
            else if ( size() > 1 )
            {
               const_iterator iter = upper_bound( clName ) ;
               if ( iter != end() && iter != begin() )
               {
                  -- iter ;
                  return _isCLInCS( iter->_pString, clName ) ;
               }
               else if ( iter == end() )
               {
                  // last one, reverse search the last one
                  return _isCLInCS( rbegin()->_pString, clName ) ;
               }
            }
         }
         return FALSE ;
      }

   protected:
      // check if collection is in collection space
      static BOOLEAN _isCLInCS( const CHAR *csName, const CHAR *clName )
      {
         SDB_ASSERT( NULL != csName, "collection space name should be valid" ) ;
         SDB_ASSERT( NULL != csName, "collection name should be valid" ) ;
         UINT32 nameLength = ossStrlen( csName ) ;
         return ( 0 == ossStrncmp( csName, clName, nameLength ) ) &&
                ( '.' == clName[ nameLength ] ) ;
      }
   } ;

   typedef _utilWatchCSNameSet utilWatchCSNameSet ;
   typedef utilWatchCSNameSet::iterator utilWatchCSNameSetIter ;
   typedef utilWatchCSNameSet::const_iterator utilWatchCSNameSetCIter ;

   /*
      _utilWatchCLNameSet define
    */
   // name set of watching collections
   class _utilWatchCLNameSet : public ossPoolSet< _utilMapStringKey >
   {
   public:
      _utilWatchCLNameSet() = default ;
      ~_utilWatchCLNameSet() = default ;

      // check if collection space is watched by watching its collection
      BOOLEAN isCSWatched( const CHAR *csName ) const
      {
         if ( NULL != csName )
         {
            UINT32 nameLength = ossStrlen( csName ) ;
            const_iterator iter = lower_bound( csName ) ;
            return ( iter != end() ) &&
                   ( 0 == ossStrncmp( iter->_pString, csName, nameLength ) ) &&
                   ( '.' == iter->_pString[ nameLength ] ) ;
         }
         return FALSE ;
      }

      // check if collection is watched
      BOOLEAN isCLWatched( const CHAR *clName ) const
      {
         return NULL != clName && count( clName ) > 0 ;
      }
   } ;

   typedef _utilWatchCLNameSet utilWatchCLNameSet ;
   typedef utilWatchCLNameSet::iterator utilWatchCLNameSetIter ;
   typedef utilWatchCLNameSet::const_iterator utilWatchCLNameSetCIter ;

   /*
      _utilChangeStreamOptions define
    */
   // options of change stream
   class _utilChangeStreamOptions : public _utilPooledObject
   {
   public:
      _utilChangeStreamOptions() = default ;
      _utilChangeStreamOptions( const _utilChangeStreamOptions &other ) = default ;
      ~_utilChangeStreamOptions() = default ;

      _utilChangeStreamOptions &operator =(
                           const _utilChangeStreamOptions &other ) = default ;

      // reset options
      void reset() ;

      // parse options from BSON format
      INT32 fromBSON( const bson::BSONObj &options ) ;

      const bson::BSONObj &getOptions() const
      {
         return _boOptions ;
      }

      const utilChangeStreamToken &getToken() const
      {
         return _token ;
      }

      const utilWatchCSNameSet &getCollectionSpaces() const
      {
         return _collectionSpaces ;
      }

      const utilWatchCLNameSet &getCollections() const
      {
         return _collections ;
      }

      UINT8 getChangeTypeMask() const
      {
         return _changeTypeMask ;
      }

      UINT32 getMaxWaitTimeMS() const
      {
         return _maxWaitTimeMS ;
      }

      UINT32 getCacheSizeB() const
      {
         return _cacheSizeB ;
      }

   protected:
      // parse change types from string format
      INT32 _parseChangeTypeMask( const CHAR *changeTypes ) ;

   protected:
      // options in BSON format
      bson::BSONObj _boOptions ;
      // token to resume
      utilChangeStreamToken _token ;
      // watching collection spaces
      utilWatchCSNameSet _collectionSpaces ;
      // watching collections
      utilWatchCLNameSet _collections ;
      // mask of watched change types
      UINT8  _changeTypeMask = UTIL_CHANGE_TYPE_DFT ;
      // max wait time in milliseconds
      UINT32 _maxWaitTimeMS = UTIL_CHANGE_STREAM_MAX_WAIT_TIME_MS_DEF ;
      // cache size in bytes
      UINT32 _cacheSizeB = UTIL_CHANGE_STREAM_CACHE_SIZE_B_DEF ;
      // allow none change types
      BOOLEAN _allowNoneChangeTypes = FALSE ;
   } ;

   typedef class _utilChangeStreamOptions utilChangeStreamOptions ;

}

#endif // UTIL_CHANGE_STREAM_OPTIONS_HPP__
