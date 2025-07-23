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

   Source File Name = utilDataExInfo.hpp

   Descriptive Name = Data Ex Info

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/01/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_DATA_EX_INFO_HPP__
#define UTIL_DATA_EX_INFO_HPP__

#include "oss.hpp"
#include "ossTypes.hpp"
#include "dms.hpp"
#include "utilPooledAutoPtr.hpp"
#include "utilUniqueID.hpp"

namespace engine
{

   /*
      _utilDataExInfo define
    */
   // data extra information for DPS log record
   class _utilDataExInfo : public SDBObject
   {
   public:
      _utilDataExInfo() = default ;
      _utilDataExInfo( const _utilDataExInfo &info ) = default ;
      ~_utilDataExInfo() = default ;

      _utilDataExInfo &operator =( const _utilDataExInfo &info ) = default ;

      utilCSUniqueID getCSUID() const
      {
         return _csUID ;
      }

      utilCLUniqueID getCLUID() const
      {
         return _clUID ;
      }

      UINT32  getCSLID() const
      {
         return _csLID ;
      }

      UINT32  getCLLID() const
      {
         return _clLID ;
      }

      dmsExtentID getExtentLID() const
      {
         return _extLID ;
      }

      BOOLEAN isValid() const
      {
         return _isValid ;
      }

   protected:
      void _setInfo( utilCSUniqueID csUID,
                     UINT32 csLID,
                     utilCLUniqueID clUID,
                     UINT32 clLID,
                     dmsExtentID extLID )
      {
         _csUID = csUID ;
         _clUID = clUID ;
         _csLID = csLID ;
         _clLID = clLID ;
         _extLID = extLID ;
         _isValid = TRUE ;
      }

      void _resetInfo()
      {
         _csUID = UTIL_UNIQUEID_NULL ;
         _clUID = UTIL_UNIQUEID_NULL ;
         _csLID = ~0 ;
         _clLID = ~0 ;
         _extLID = DMS_INVALID_EXTENT ;
         _isValid = FALSE ;
      }

   protected:
      // unique ID of collection space
      utilCSUniqueID _csUID = UTIL_UNIQUEID_NULL ;
      // unique ID of collection
      utilCLUniqueID _clUID = UTIL_UNIQUEID_NULL ;
      // logical ID of collection space
      UINT32      _csLID = ~0 ;
      // logical ID of collection
      UINT32      _clLID = ~0 ;
      // logical extent ID
      dmsExtentID _extLID = DMS_INVALID_EXTENT ;
      // indicate whether the information is valid
      BOOLEAN _isValid  = FALSE ;
   } ;

   typedef class _utilDataExInfo utilDataExInfo ;

   /*
      _utilLogRecordCache define
    */
   // cache for log record
   #define UTIL_CACHE_FLAG_EMPTY  ( 0x00000000 )
   #define UTIL_CACHE_FLAG_FILLED ( 0x00000001 )

   class _utilLogRecordCache : public SDBObject
   {
   public:
      _utilLogRecordCache() = default ;
      _utilLogRecordCache( const _utilLogRecordCache &cache ) = default ;
      ~_utilLogRecordCache() = default ;

      _utilLogRecordCache &operator =( const _utilLogRecordCache &cache ) = default ;

      // get buffer pointer
      const CHAR *getBuffer() const
      {
         return ( NULL != _cachePtr.get() ) ? ( _getBuffer() ) : ( NULL ) ;
      }

      // get buffer pointer
      CHAR *getBuffer()
      {
         return ( NULL != _cachePtr.get() ) ? ( _getBuffer() ) : ( NULL ) ;
      }

      UINT32 getCapacity() const
      {
         return _capacity ;
      }

      // check whether cache is ready ( just allocated, not filled )
      BOOLEAN isReady() const
      {
         return NULL != _cachePtr.get() ;
      }

      // check whether cache is enough for size
      BOOLEAN canCache( UINT32 size ) const
      {
         return NULL != _cachePtr.get() && _capacity >= size ;
      }

      // check whether cache is filled
      BOOLEAN isFilled() const
      {
         return ( ( NULL != _cachePtr.get() ) &&
                  ( OSS_BIT_TEST( _getFlag(), UTIL_CACHE_FLAG_FILLED ) ) ) ? TRUE : FALSE ;
      }

      // mark cache is filled
      void doneFill()
      {
         if ( NULL != _cachePtr.get() )
         {
            OSS_BIT_SET( _getFlag(), UTIL_CACHE_FLAG_FILLED ) ;
         }
      }

      // allocate cache
      void alloc( UINT32 size )
      {
         _cachePtr = utilPooledAutoPtr::alloc( size + sizeof( UINT32 ), ALLOC_POOL ) ;
         if ( NULL != _cachePtr.get() )
         {
            _getFlag() = UTIL_CACHE_FLAG_EMPTY ;
            _capacity = size ;
         }
      }

      // release cache
      void release()
      {
         _capacity = 0 ;
         _cachePtr.release() ;
      }

   protected:
      CHAR *_getBuffer()
      {
         return _cachePtr.get() + sizeof( UINT32 ) ;
      }

      UINT32 &_getFlag()
      {
         return *(UINT32 *)( _cachePtr.get() ) ;
      }

      const CHAR *_getBuffer() const
      {
         return _cachePtr.get() + sizeof( UINT32 ) ;
      }

      const UINT32 &_getFlag() const
      {
         return *(UINT32 *)( _cachePtr.get() ) ;
      }

   protected:
      // capacity of cache
      UINT32 _capacity = 0 ;
      // cache buffer
      // NOTE: first 4 bytes (UINT32) is flag
      utilPooledAutoPtr _cachePtr ;
   } ;

   typedef class _utilLogRecordCache utilLogRecordCache ;

   /*
      _utilLogExInfo define
    */
   // log information with data extra information
   class _utilLogExInfo : public _utilDataExInfo
   {
   public:
      _utilLogExInfo() = default ;
      _utilLogExInfo( const _utilLogExInfo &info ) = default ;
      ~_utilLogExInfo() = default ;

      _utilLogExInfo &operator =( const _utilLogExInfo &info ) = default ;

      _utilLogExInfo( const _utilDataExInfo &info )
      : _utilDataExInfo( info )
      {
      }

      // get cache for log record
      utilLogRecordCache &getLogRecordCache()
      {
         return _cache ;
      }

      // get cache for log record
      const utilLogRecordCache &getLogRecordCache() const
      {
         return _cache ;
      }

      // get cache buffer for log record
      const CHAR *getLogRecordCacheBuffer() const
      {
         return _cache.getBuffer() ;
      }

      // check whether log record cache is enough for size
      BOOLEAN canUseLogRecordCache( UINT32 size ) const
      {
         return _cache.canCache( size ) ;
      }

      // check whether log record cache is ready ( just allocated, not filled )
      BOOLEAN isLogRecordCacheReady() const
      {
         return _cache.isReady() ;
      }

      // check whether log record cache is filled
      BOOLEAN isLogRecordCacheFilled() const
      {
         return _cache.isFilled() ;
      }

      // allocate log record cache
      void allocLogRecordCache( UINT32 size )
      {
         _cache.alloc( size ) ;
      }

   protected:
      void _resetInfo()
      {
         _utilDataExInfo::_resetInfo() ;
         _cache.release() ;
      }

   protected:
      // cache for log record
      utilLogRecordCache _cache ;
   } ;

   typedef class _utilLogExInfo utilLogExInfo ;

}

#endif // UTIL_DATA_EX_INFO_HPP__
