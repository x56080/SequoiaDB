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

   Source File Name = utilChangeStreamWatchInfo.hpp

   Descriptive Name = Change Stream Watch Info

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/01/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_CHANGE_STREAM_WATCH_INFO_HPP__
#define UTIL_CHANGE_STREAM_WATCH_INFO_HPP__

#include "dpsLogDef.hpp"
#include "oss.hpp"
#include "ossMemPool.hpp"
#include "ossTypes.h"
#include "utilBitmap.hpp"
#include "utilDataExInfo.hpp"
#include "utilPooledObject.hpp"
#include "dms.hpp"
#include "dpsDef.hpp"
#include "utilUniqueID.hpp"
#include "utilChangeStreamOptions.hpp"

namespace engine
{

   /*
      utilWatchType define
    */
   // watch type
   typedef enum _utilWatchType
   {
      // watch collection
      UTIL_WATCH_COLLECTION = 0,
      // watch collection space
      UTIL_WATCH_COLLECTION_SPACE,
      // watch all
      UTIL_WATCH_ALL
   } utilWatchType ;

   // get name of watch type
   const CHAR *utilGetWatchTypeName( utilWatchType watchType ) ;

   /*
      utilLogTypeBitmap define
    */
   // mask log types to bitmap
   typedef _utilStackBitmap< LOG_TYPE_NUM > utilLogTypeBitmap ;

   /*
      utilWatchCSUIDSet define
    */
   // watch set fo collection space unique ID
   typedef ossPoolSet< utilCSUniqueID > utilWatchCSUIDSet ;
   typedef utilWatchCSUIDSet::iterator utilWatchCSUIDSetIter ;
   typedef utilWatchCSUIDSet::const_iterator utilWatchCSUIDSetCIter ;

   /*
      utilWatchCLUIDSet define
    */
   // watch set fo collection unique ID
   typedef ossPoolSet< utilCLUniqueID > utilWatchCLUIDSet ;
   typedef utilWatchCLUIDSet::iterator utilWatchCLUIDSetIter ;
   typedef utilWatchCLUIDSet::const_iterator utilWatchCLUIDSetCIter ;

   /*
      _utilWatchCSBitmap define
    */
   // watch bitmap for collection spaces
   // based on hash of collection space unique ID
   #define UTIL_WATCH_BITMAP_SIZE ( 128 )
   #define UTIL_WATCH_BITMAP_MODULO ( UTIL_WATCH_BITMAP_SIZE - 1 )

   class _utilWatchCSBitmap : public _utilStackBitmap< UTIL_WATCH_BITMAP_SIZE >
   {
   public:
      _utilWatchCSBitmap() = default ;
      ~_utilWatchCSBitmap() = default ;

   public:
      // check if watching collection space
      BOOLEAN isWatching( utilCSUniqueID csUniqueID ) const
      {
         return testBit( csUniqueID & UTIL_WATCH_BITMAP_MODULO ) ;
      }

      // check if watching collection
      // ( by check if watching its collection space )
      BOOLEAN isWatching( utilCLUniqueID clUniqueID ) const
      {
         return isWatching( utilGetCSUniqueID( clUniqueID ) ) ;
      }

      // watch collection space
      void watch( utilCSUniqueID csUniqueID )
      {
         setBit( csUniqueID & UTIL_WATCH_BITMAP_MODULO ) ;
      }

      // watch collection
      // ( by mark watching its collection space )
      void watch( utilCLUniqueID clUniqueID )
      {
         watch( utilGetCSUniqueID( clUniqueID ) ) ;
      }
   } ;
   typedef class _utilWatchCSBitmap utilWatchCSBitmap ;

   /*
      _utilChangeStreamWatchInfo define
    */
   // watch information for change stream
   class _utilChangeStreamWatchInfo : public _utilPooledObject
   {
   public:
      _utilChangeStreamWatchInfo() = default ;
      _utilChangeStreamWatchInfo( const _utilChangeStreamWatchInfo &other ) = default ;
      ~_utilChangeStreamWatchInfo() = default ;

      // get highest level of watch type
      utilWatchType getWatchLevel() const
      {
         return _watchLevel ;
      }

      // watch collection space
      INT32 watchCS( const CHAR *csName, utilCSUniqueID csUnqiueID ) ;
      // watch collection
      INT32 watchCL( const CHAR *clName, utilCLUniqueID clUniqueID ) ;
      // watch log types
      INT32 watchLogTypes( UINT8 changeTypeMask ) ;

      // complete setting watch information
      void complete() ;

      // check if watching collection space by unique ID
      // if we can not comfirm by unique ID, need check name
      BOOLEAN isWatchingCS( utilCSUniqueID csUniqueID, BOOLEAN &needCheckName ) const ;
      // check if watching collection by unique ID
      // if we can not comfirm by unique ID, need check name
      BOOLEAN isWatchingCL( utilCLUniqueID clUniqueID, BOOLEAN &needCheckName ) const ;

      // check if watching collection space by name
      BOOLEAN isWatchingCS( const CHAR *csName ) const ;
      // check if watching collection by name
      BOOLEAN isWatchingCL( const CHAR *clName ) const ;
      // check if watching collection space by name explicitly
      // not implicitly enable by watching its collection
      BOOLEAN isWatchingCSExplicitly( const CHAR *clName ) const ;
      // check if watching collection by name explicitly
      // not implicitly enable by watching its collection space
      BOOLEAN isWatchingCLExplicitly( const CHAR *clName ) const ;
      // check if watching collection space by name implicitly
      // enable by watching its collection
      BOOLEAN isWatchingCSImplicitly( const CHAR *csName ) const ;
      // check if watching collection by name implicitly
      // enable by watching its collection space
      BOOLEAN isWatchingCLImplicitly( const CHAR *csName ) const ;

      // check if watching log type
      BOOLEAN isWatchingLogType( DPS_LOG_TYPE logType ) const
      {
         return _watchedLogTypeBitmap.testBit( logType ) ;
      }

      // check if error log type which should report error
      BOOLEAN isErrorLogType( DPS_LOG_TYPE logType ) const
      {
         return _errorLogTypeBitmap.testBit( logType ) ;
      }

      // get watched collection space unique ID bitmap
      const utilWatchCSBitmap &getWatchedCSBitmap() const
      {
         return _watchedCSBitmap ;
      }

      // get watched log type bitmap
      const utilLogTypeBitmap &getWatchedLogTypeBitmap() const
      {
         return _watchedLogTypeBitmap ;
      }

      // get error log type bitmap
      const utilLogTypeBitmap &getErrorLogTypeBitmap() const
      {
         return _errorLogTypeBitmap ;
      }

   protected:
      void _setWatchedCSBitmap( utilCSUniqueID csUniqueID ) ;
      void _setWatchedCSBitmap( utilCLUniqueID clUniqueID ) ;

   protected:
      // highest watch level
      utilWatchType _watchLevel = UTIL_WATCH_ALL ;
      // watched collection spaces by unique ID bitmap
      utilWatchCSBitmap _watchedCSBitmap ;
      // watched collection spaces by name
      utilWatchCSNameSet _watchedCSByName ;
      // watched system collection spaces by name
      utilWatchCSNameSet _watchedSysCSByName ;
      // watched collections by name
      utilWatchCLNameSet _watchedCLByName ;
      // watched system collections by name
      utilWatchCLNameSet _watchedSysCLByName ;
      // watched collections space by unique ID
      utilWatchCSUIDSet _watchedCSByUID ;
      // watched collections by unique ID
      utilWatchCLUIDSet _watchedCLByUID ;
      // watched log types in bitmap format
      utilLogTypeBitmap _watchedLogTypeBitmap ;
      // error log types in bitmap format
      utilLogTypeBitmap _errorLogTypeBitmap ;
   } ;

   typedef class _utilChangeStreamWatchInfo utilChangeStreamWatchInfo ;

   /*
      _utilChangeStreamLogInfo define
    */
   // log information for change stream
   class _utilChangeStreamLogInfo : public _utilLogExInfo
   {
   public:
      _utilChangeStreamLogInfo() = default ;
      _utilChangeStreamLogInfo( const _utilChangeStreamLogInfo &other ) = default ;
      ~_utilChangeStreamLogInfo() = default ;

      _utilChangeStreamLogInfo &operator =( const _utilChangeStreamLogInfo &other ) = default ;

      _utilChangeStreamLogInfo( const _utilLogExInfo &logInfo,
                                DPS_LSN_OFFSET offset,
                                DPS_LSN_VER version,
                                UINT32 length,
                                DPS_LOG_TYPE logType )
      : _utilLogExInfo( logInfo ),
        _length( length ),
        _logType( logType )
      {
         _lsn.set( offset, version ) ;
      }

      // get offset of log record
      DPS_LSN_OFFSET getOffset() const
      {
         return _lsn.offset ;
      }

      // get version of log record
      DPS_LSN_VER getVersion() const
      {
         return _lsn.version ;
      }

      // get next offset of log record
      DPS_LSN_OFFSET getNextOffset() const
      {
         return _lsn.offset + _length ;
      }

      // get LSN of log record
      const DPS_LSN &getLSN() const
      {
         return _lsn ;
      }

      // get length of log record
      UINT32 getLength() const
      {
         return _length ;
      }

      // get type of log record
      DPS_LOG_TYPE getLogType() const
      {
         return _logType ;
      }

      // compare by LSN
      bool operator <( const _utilChangeStreamLogInfo &other ) const
      {
         return _lsn.offset < other._lsn.offset ;
      }

   protected:
      void _resetInfo()
      {
         _utilLogExInfo::_resetInfo() ;
         _lsn.reset() ;
         _length = 0 ;
         _logType = LOG_TYPE_DUMMY ;
      }

   protected:
      // LSN of log record
      DPS_LSN _lsn ;
      // length of log record
      UINT32 _length = 0 ;
      // type of log record
      DPS_LOG_TYPE _logType = LOG_TYPE_DUMMY ;
   } ;

   typedef class _utilChangeStreamLogInfo utilChangeStreamLogInfo ;

}

#endif // UTIL_CHANGE_STREAM_OPTIONS_HPP__
