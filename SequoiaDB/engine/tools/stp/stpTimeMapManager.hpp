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
#include "stpLogicalTime.hpp"

namespace engine
{

   // -/+ 0.5 seconds
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
   typedef ossPoolVector< stpTimeMapRecord > STP_TIME_MAP_RECORD_LIST ;

   /*
      _stpTimeMapStore define
    */
   // time mapping store interface
   class _stpTimeMapStore : public SDBObject
   {
   public:
      _stpTimeMapStore() {}
      virtual ~_stpTimeMapStore() {}

   public:
      // initialize
      virtual INT32 initialize() = 0 ;
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
      virtual INT32 getRecordAfterLTime( UINT64 logicalTime,
                                           BOOLEAN included,
                                           UINT32 count,
                                           STP_TIME_MAP_RECORD_LIST &recordList ) = 0 ;

      // get records after given real time
      virtual INT32 getRecordAfterRTime( UINT64 realTime,
                                         BOOLEAN included,
                                         UINT32 count,
                                         STP_TIME_MAP_RECORD_LIST &recordList ) = 0 ;

      // clear expired records
      virtual INT32 clearExpiredRecords() = 0 ;
   } ;

   typedef class _stpTimeMapStore stpTimeMapStore ;

   /*
      _stpTimeMapMemStore define
    */
   // time mapping store in memory
   class _stpTimeMapMemStore : public _stpTimeMapStore
   {
   public:
      _stpTimeMapMemStore() ;
      virtual ~_stpTimeMapMemStore() ;

      // initialize
      virtual INT32 initialize() ;
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
      virtual INT32 getRecordAfterLTime( UINT64 logicalTime,
                                         BOOLEAN included,
                                         UINT32 count,
                                         STP_TIME_MAP_RECORD_LIST &recordList ) ;

      // get records after given real time
      virtual INT32 getRecordAfterRTime( UINT64 realTime,
                                         BOOLEAN included,
                                         UINT32 count,
                                         STP_TIME_MAP_RECORD_LIST &recordList ) ;

      // clear expired records
      virtual INT32 clearExpiredRecords() ;

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
                                   STP_TIME_MAP_RECORD_LIST &recordList ) ;

   protected:
      // lock to protect maps
      ossRWMutex _mapLock ;
      // mapping key: logical time, mapping value: real time
      STP_TIME_MAP _logicalTimeMap ;
      // mapping key: real time, mapping value: logical time
      STP_TIME_MAP _realTimeMap ;
   } ;

   typedef class _stpTimeMapMemStore stpTimeMapMemStore ;

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
      INT32 initialize() ;
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
      INT32 saveTimeMapping() ;
      // convert logical time to real time
      INT32 convLTimeToRTime( const stpHPTime &logicalTime,
                              stpHPTime &realTime ) ;
      // convert real time to logical time
      INT32 convRTimeToLTime( const stpHPTime &realTime,
                              stpHPTime &logicalTime ) ;

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

   protected:
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
