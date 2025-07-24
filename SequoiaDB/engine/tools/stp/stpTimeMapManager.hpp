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

   Source File Name = stpTimeMapManager.hpp

   Descriptive Name = Serial Time Protocol

   When/how to use: this program may be used on binary and text-formatted
   versions of STP component. This file contains structure for Serial Time
   Protocol.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/20/2020  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef STP_TIME_MAP_MANAGER_HPP__
#define STP_TIME_MAP_MANAGER_HPP__

#include "core.hpp"
#include "ossUtil.hpp"
#include "utilPooledObject.hpp"
#include "ossMemPool.hpp"
#include "ossRWMutex.hpp"
#include "utilSQLiteDB.hpp"
#include "stpLogicalTime.hpp"
#include "stpOptions.hpp"
#include "stpMetaData.hpp"

namespace engine
{

   // time difference between logical time and real time is
   // tolerable in -/+ 0.5 seconds
   #define STP_TIME_MAP_MAX_TOLERANCE ( 500000LL )
   #define STP_TIME_MAP_MIX_TOLERANCE ( -500000LL )

   /*
      _stpTimeMapRecord define
    */
   // record for time mapping
   class _stpTimeMapRecord : public utilPooledObject
   {
   public:
      _stpTimeMapRecord()
      : _logicalTime( 0LL ),
        _realTime( 0LL )
      {
      }

      _stpTimeMapRecord( UINT64 logicalTime, UINT64 realTime )
      : _logicalTime( logicalTime ),
        _realTime( realTime )
      {
      }

      _stpTimeMapRecord( const _stpTimeMapRecord &record )
      : _logicalTime( record._logicalTime ),
        _realTime( record._realTime )
      {
      }

      ~_stpTimeMapRecord()
      {
      }

      // assign operator
      _stpTimeMapRecord &operator =( const _stpTimeMapRecord &record )
      {
         _logicalTime = record._logicalTime ;
         _realTime = record._realTime ;
         return (*this) ;
      }

      // get logical time
      UINT64 getLogicalTime() const
      {
         return _logicalTime ;
      }

      // get real time
      UINT64 getRealTime() const
      {
         return _realTime ;
      }

      // set logical time
      void setLogicalTime( UINT64 logicalTime )
      {
         _logicalTime = logicalTime ;
      }

      // set real time
      void setRealTime( UINT64 realTime )
      {
         _realTime = realTime ;
      }

      // set mapping
      void setMap( UINT64 logicalTime, UINT64 realTime )
      {
         _logicalTime = logicalTime ;
         _realTime = realTime ;
      }

      // check if logical time and real time are the same
      BOOLEAN isSameTime() const
      {
         return _logicalTime == _realTime ;
      }

      // check if the record is valid
      BOOLEAN isValid() const
      {
         return 0 != _logicalTime && 0 != _realTime ;
      }

      // get time shift between logical time and real time
      INT64 getTimeShift() const
      {
         return (INT64)( _logicalTime ) - (INT64)( _realTime ) ;
      }

      // check if the time shift is tolerable
      BOOLEAN isTolerable( const _stpTimeMapRecord &record ) const
      {
         INT64 inputTimeShift = record.getTimeShift() ;
         INT64 curTimeShift = getTimeShift() ;
         INT64 changed = inputTimeShift - curTimeShift ;

         return ( changed < STP_TIME_MAP_MAX_TOLERANCE &&
                  changed > STP_TIME_MAP_MIX_TOLERANCE ) ? TRUE : FALSE ;
      }

      // calculate real time by time shift
      UINT64 calcRealTime( UINT64 logicalTime )
      {
         // shift = L' - R'
         // R = L - ( shift )
         INT64 timeShift = getTimeShift() ;
         if ( timeShift > 0 &&
              (INT64)logicalTime < timeShift )
         {
            return 0LL ;
         }
         return (UINT64)( (INT64)logicalTime - timeShift ) ;
      }

      // calculate logical time by time shift
      UINT64 calcLogicalTime( UINT64 realTime )
      {
         // shift = L' - R'
         // L = R + ( shift )
         INT64 timeShift = getTimeShift() ;
         if ( timeShift < 0 &&
              ( -timeShift ) > (INT64)realTime )
         {
            return 0LL ;
         }
         return (UINT64)( (INT64)realTime + timeShift ) ;
      }

   protected:
      // logical time
      UINT64 _logicalTime ;
      // real time
      UINT64 _realTime ;
   } ;

   typedef class _stpTimeMapRecord stpTimeMapRecord ;
   typedef ossPoolList< stpTimeMapRecord > STP_TIMEMAP_RECLIST ;

   /*
      _stpTimeMapStore define
    */
   // time mapping store interface
   class _stpTimeMapStore : public SDBObject
   {
   public:
      _stpTimeMapStore()
      : _maxTimeMapSize( 0 )
      {
      }

      virtual ~_stpTimeMapStore() {}

   public:
      // initialize
      virtual INT32 initialize( const stpOptions *options ) = 0 ;
      // finalize
      virtual INT32 finalize() = 0 ;

      // save time mapping record
      virtual INT32 saveRecord( const stpTimeMapRecord &record ) = 0 ;
      // get nearest record before given logical time
      virtual INT32 getRecordBeforeLTime( UINT64 logicalTime,
                                          BOOLEAN included,
                                          stpTimeMapRecord &record ) = 0 ;
      // get nearest record before given real time
      virtual INT32 getRecordBeforeRTime( UINT64 realTime,
                                          BOOLEAN included,
                                          stpTimeMapRecord &record ) = 0 ;

      // get records after given logical time
      virtual INT32 getRecordsAfterLTime( UINT64 logicalTime,
                                          BOOLEAN included,
                                          UINT32 count,
                                          STP_TIMEMAP_RECLIST &recordList ) = 0 ;

      // get records after given real time
      virtual INT32 getRecordsAfterRTime( UINT64 realTime,
                                          BOOLEAN included,
                                          UINT32 count,
                                          STP_TIMEMAP_RECLIST &recordList ) = 0 ;

      // get latest record
      virtual INT32 getLastRecord( stpTimeMapRecord &record ) = 0 ;

      // get latest records
      virtual INT32 getLastRecords( UINT32 count,
                                    STP_TIMEMAP_RECLIST &recordList ) = 0 ;

      // get first record
      virtual INT32 getFirstRecord( stpTimeMapRecord &record ) = 0 ;

      // get first records
      virtual INT32 getFirstRecords( UINT32 count,
                                     STP_TIMEMAP_RECLIST &recordList ) = 0 ;

      // clear expired records
      virtual void clearExpiredRecords() = 0 ;

      // clear all records
      virtual void clearAllRecords() = 0 ;

      // get number of records
      virtual UINT32 getTimeMapSize() = 0 ;

      // set max size of time map
      virtual void setMaxTimeMapSize( INT32 maxTimeMapSize )
      {
         _maxTimeMapSize = maxTimeMapSize ;
         if ( _isDisabled() )
         {
            clearAllRecords() ;
         }
      }

   protected:
      OSS_INLINE BOOLEAN _isEnabled() const
      {
         return 0 != _maxTimeMapSize ;
      }

      OSS_INLINE BOOLEAN _isDisabled() const
      {
         return 0 == _maxTimeMapSize ;
      }

      OSS_INLINE BOOLEAN _isUnlimited() const
      {
         return _maxTimeMapSize < 0 ;
      }

   protected:
      // max size of time map
      INT32 _maxTimeMapSize ;
   } ;

   typedef class _stpTimeMapStore stpTimeMapStore ;

   /*
      _stpTimeMapMemStore define
    */
   // time mapping store in memory ( used for cache )
   class _stpTimeMapMemStore : public _stpTimeMapStore
   {
   public:
      _stpTimeMapMemStore() ;
      virtual ~_stpTimeMapMemStore() ;

      // initialize
      virtual INT32 initialize( const stpOptions *options ) ;
      // finalize
      virtual INT32 finalize() ;

      // save time mapping record
      virtual INT32 saveRecord( const stpTimeMapRecord &record ) ;

      // get nearest record before given logical time
      virtual INT32 getRecordBeforeLTime( UINT64 logicalTime,
                                          BOOLEAN included,
                                          stpTimeMapRecord &record ) ;

      // get nearest record before given real time
      virtual INT32 getRecordBeforeRTime( UINT64 realTime,
                                          BOOLEAN included,
                                          stpTimeMapRecord &record ) ;

      // get records after given logical time
      virtual INT32 getRecordsAfterLTime( UINT64 logicalTime,
                                          BOOLEAN included,
                                          UINT32 count,
                                          STP_TIMEMAP_RECLIST &recordList ) ;

      // get records after given real time
      virtual INT32 getRecordsAfterRTime( UINT64 realTime,
                                          BOOLEAN included,
                                          UINT32 count,
                                          STP_TIMEMAP_RECLIST &recordList ) ;

      // get latest record
      virtual INT32 getLastRecord( stpTimeMapRecord &record ) ;

      // get latest records
      virtual INT32 getLastRecords( UINT32 count,
                                    STP_TIMEMAP_RECLIST &recordList ) ;

      // get first record
      virtual INT32 getFirstRecord( stpTimeMapRecord &record ) ;

      // get first records
      virtual INT32 getFirstRecords( UINT32 count,
                                     STP_TIMEMAP_RECLIST &recordList ) ;

      // clear expired records
      virtual void clearExpiredRecords() ;

      // clear all records
      virtual void clearAllRecords() ;

      // get size of map
      OSS_INLINE virtual UINT32 getTimeMapSize()
      {
         return _logicalTimeMap.size() ;
      }

      OSS_INLINE BOOLEAN isEmpty()
      {
         return _logicalTimeMap.empty() ;
      }

   protected:
      typedef ossPoolMap< UINT64, UINT64 > STP_TIME_MAP ;

      // get the nearest mapping times from time map before the given time
      // input:
      // - timeMap: time map either from real time to logical time or
      //            from logical time to real time
      // - timeBefore: time mapping is before this time
      // - include: whether the time mapping can equal to the target time
      // output:
      // - mappingTime: time key in time mapping just before the target time
      // - mappedTime: time value in time mapping just before the target time
      // WARNING: must in lock
      void _getRecordFromMap( const STP_TIME_MAP &timeMap,
                              UINT64 timeBefore,
                              BOOLEAN included,
                              UINT64 &mappingTime,
                              UINT64 &mappedTime ) ;

      // get the mapping list from time map after the given time
      // input:
      // - timeMap: time map either from real time to logical time or
      //            from logical time to real time
      // - afterTime: time mapping list is after this target time
      // - include: whether the time mapping can equal to the target time
      // - maxNumReturn: max number of records will be returned
      // - fromRTimeToLTime: mapping direction, from real time to logical time,
      //                     or from logical time to real time
      // output:
      // - recordList: list of time mapping records after the given time
      // WARNING: must in lock
      INT32 _getRecordListFromMap( const STP_TIME_MAP &timeMap,
                                   UINT64 afterTime,
                                   BOOLEAN included,
                                   UINT32 maxNumReturn,
                                   BOOLEAN fromRTimeToLTime,
                                   STP_TIMEMAP_RECLIST &recordList ) ;

      // save time mapping record
      INT32 _saveRecord( const stpTimeMapRecord &record ) ;

      // clear expired records
      void _clearExpiredRecords() ;

      // clear records from maps
      void _clearMaps( STP_TIME_MAP &mapping,
                       STP_TIME_MAP &mapped,
                       INT32 maxMapSize ) ;

   protected:
      // mapping key: logical time, mapping value: real time
      STP_TIME_MAP _logicalTimeMap ;
      // mapping key: real time, mapping value: logical time
      STP_TIME_MAP _realTimeMap ;
   } ;

   typedef class _stpTimeMapMemStore stpTimeMapMemStore ;

   /*
      _stpTimeMapDBStore define
    */
   // time mapping store in database
   class _stpTimeMapDBStore : public _stpTimeMapStore
   {
   public:
      _stpTimeMapDBStore() ;
      virtual ~_stpTimeMapDBStore() ;

      // initialize
      virtual INT32 initialize( const stpOptions *options ) ;
      // finalize
      virtual INT32 finalize() ;

      // save time mapping record
      virtual INT32 saveRecord( const stpTimeMapRecord &record ) ;

      // get nearest record before given logical time
      virtual INT32 getRecordBeforeLTime( UINT64 logicalTime,
                                          BOOLEAN included,
                                          stpTimeMapRecord &record ) ;
      // get nearest record before given real time
      virtual INT32 getRecordBeforeRTime( UINT64 realTime,
                                          BOOLEAN included,
                                          stpTimeMapRecord &record ) ;

      // get records after given logical time
      virtual INT32 getRecordsAfterLTime( UINT64 logicalTime,
                                          BOOLEAN included,
                                          UINT32 count,
                                          STP_TIMEMAP_RECLIST &recordList ) ;

      // get records after given real time
      virtual INT32 getRecordsAfterRTime( UINT64 realTime,
                                          BOOLEAN included,
                                          UINT32 count,
                                          STP_TIMEMAP_RECLIST &recordList ) ;

      // get latest record
      virtual INT32 getLastRecord( stpTimeMapRecord &record ) ;

      // get latest records
      virtual INT32 getLastRecords( UINT32 count,
                                    STP_TIMEMAP_RECLIST &recordList ) ;

      // get first record
      virtual INT32 getFirstRecord( stpTimeMapRecord &record ) ;

      // get first records
      virtual INT32 getFirstRecords( UINT32 count,
                                     STP_TIMEMAP_RECLIST &recordList ) ;

      // clear expired records
      virtual void clearExpiredRecords() ;

      // clear all records
      virtual void clearAllRecords() ;

      // get number of records
      virtual UINT32 getTimeMapSize() ;

      // set max size of time map
      virtual void setMaxTimeMapSize( INT32 maxTimeMapSize ) ;

   protected:
      // ensure time map table and indexes are created
      INT32 _ensureTable() ;
      // prepare statements
      INT32 _prepareStatements() ;
      // save record into database
      INT32 _saveRecord( const stpTimeMapRecord &record ) ;
      // load cache from database
      INT32 _loadCache() ;
      // get size of time map
      INT32 _getTimeMapSize( UINT32 &timeMapSize ) ;
      // merge record lists
      INT32 _mergeRecordList( STP_TIMEMAP_RECLIST &recordList,
                              const STP_TIMEMAP_RECLIST &mergeList,
                              BOOLEAN isMergeInDescOrder,
                              BOOLEAN isMergeToEnd ) ;

      // get nearest record by given time from database
      // input:
      // - timeColumnName: column name of given time
      // - targetTime: given time
      // - isIncluded: whether to include target time
      // - isBefore: whether to get result before target time
      // - isDescOrder: in descent order
      // output:
      // - record: result time map record
      INT32 _getRecordByTime( const CHAR *timeColumnName,
                              UINT64 targetTime,
                              BOOLEAN isIncluded,
                              BOOLEAN isBefore,
                              BOOLEAN isDescOrder,
                              stpTimeMapRecord &record ) ;

      // get records by given time
      // input:
      // - timeColumnName: column name of given time
      // - targetTime: given time
      // - isIncluded: whether to include target time
      // - isBefore: whether to get result before target time
      // - isDescOrder: in descent order
      // - count: number of results
      // output:
      // - recordList: result time map records
      INT32 _getRecordsByTime( const CHAR *timeColumnName,
                               UINT64 targetTime,
                               BOOLEAN isIncluded,
                               BOOLEAN isBefore,
                               BOOLEAN isDescOrder,
                               UINT32 count,
                               STP_TIMEMAP_RECLIST &recordList ) ;

      // get latest or first record
      // input:
      // - formattedQuery: SQL query to get time map records
      // - isDescOrder: in descent order
      // output:
      // - record: result time map record
      INT32 _getRecord( BOOLEAN isDescOrder,
                        stpTimeMapRecord &record ) ;

      // get latest or first records
      // input:
      // - isDescOrder: in descent order
      // - count: number of results
      // output:
      // - recordList: result time map records
      INT32 _getRecords( BOOLEAN isDescOrder,
                         UINT32 count,
                         STP_TIMEMAP_RECLIST &recordList ) ;

      BOOLEAN _isLTimeInCache( UINT64 logicalTime ) ;
      BOOLEAN _isRTimeInCache( UINT64 realTime ) ;

   protected:
      // memory cache for latest records
      stpTimeMapMemStore   _cache ;

      // max size of cache
      UINT32               _maxCacheSize ;

      // current size of time map
      UINT32               _currentMapSize ;

      // SQLite database
      utilSQLiteDB         _database ;

      // prepared save record statement
      utilSQLiteStatement  _saveRecordStmt ;
   } ;

   typedef class _stpTimeMapDBStore stpTimeMapDBStore ;

   /*
      _stpTimeMapManager define
    */
   class _stpTimeMapManager : public SDBObject
   {
   public:
      _stpTimeMapManager() ;
      ~_stpTimeMapManager() ;

   public:
      // initialize
      INT32 initialize( const stpOptions *options ) ;
      // finalize
      INT32 finalize() ;

      // acquire logical time and real time and save as a time mapping record
      // in store
      // NOTE: save time mapping when
      // - after primary STP server has updated LSN meta data
      // - after synchronize client has synchronized time with a INTERVALCHECK
      //   status
      // TODO:
      // - client will synchronize the records from primary server to
      //   keep the same mapping result in one STP cluster
      // - currently, only save in memory, we need to save in a database file
      //   in the future
      INT32 saveTimeMapping( stpMetaData *metaData ) ;
      // convert logical time to real time
      INT32 convLTimeToRTime( const stpHPTime &logicalTime,
                              stpHPTime &realTime ) ;
      // convert real time to logical time
      INT32 convRTimeToLTime( const stpHPTime &realTime,
                              stpHPTime &logicalTime ) ;

      // get records after given logical time
      INT32 getRecordsAfterLTime( const stpHPTime &logicalTime,
                                  BOOLEAN included,
                                  UINT32 count,
                                  STP_TIMEMAP_RECLIST &recordList ) ;

      // get records after given real time
      INT32 getRecordsAfterRTime( const stpHPTime &realTime,
                                  BOOLEAN included,
                                  UINT32 count,
                                  STP_TIMEMAP_RECLIST &recordList ) ;

      // get latest records
      INT32 getLastRecords( UINT32 count,
                            STP_TIMEMAP_RECLIST &recordList ) ;

      // indicate if we need to save time mapping
      OSS_INLINE BOOLEAN isNeedSaveTimeMapping()
      {
         return _needSaveTimeMapping ;
      }

      // set if we need to save time mapping
      OSS_INLINE void setNeedSaveTimeMapping( BOOLEAN needSave )
      {
         _needSaveTimeMapping = needSave ;
      }

      // set max size of time map
      OSS_INLINE void setMaxTimeMapSize( INT32 maxTimeMapSize,
                                         BOOLEAN hasLock )
      {
         if ( !hasLock )
         {
            _storeLock.lock_w() ;
         }

         if ( NULL != _store )
         {
            _store->setMaxTimeMapSize( maxTimeMapSize ) ;
         }

         if ( !hasLock )
         {
            _storeLock.release_w() ;
         }
      }

   protected:
      // lock to protected time map store
      ossRWMutex        _storeLock ;
      // indicate whether we need to save time mapping record
      volatile BOOLEAN  _needSaveTimeMapping ;
      // last time mapping record saved in store
      stpTimeMapRecord  _lastRecord ;
      // store of time mapping ( can be in memory or in database file )
      stpTimeMapStore * _store ;
   } ;

   typedef class _stpTimeMapManager stpTimeMapManager ;

}

#endif // STP_TIME_MAP_MANAGER_HPP__
