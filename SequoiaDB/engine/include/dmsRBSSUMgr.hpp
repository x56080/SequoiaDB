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

   Source File Name = dmsRBSSUMgr.hpp

   Descriptive Name = DMS Roll Back Segment Storage Unit Management Header

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS RollBack Segment Storage Unit Management.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/10/2019  CYX Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMSRBSSUMGR_HPP__
#define DMSRBSSUMGR_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "ossLatch.hpp"
#include "ossUtil.hpp"
#include "dms.hpp"
#include "dmsSysSUMgr.hpp"
#include "dmsRecord.hpp"
#include "dmsStorageDataCommon.hpp"
#include "dmsStorageDataCapped.hpp"
#include "dpsDef.hpp"

using namespace std ;

namespace engine
{
   // number of slots in RBS hash bucket, a prime number less than 32K
   #define  DMS_RBS_HASH_BKT_SLOTS   ( (UINT32) 32749 )

   #define DMS_BUILD_RBS_CL_NAME( clName, cl )             \
               ossSnprintf ( clName, sizeof(clName),       \
                             DMS_RBS_NAME_PATTERN,         \
                             SDB_DMSRBS_NAME, cl )

   #define DMS_RBS_FLUSH_OPTION_MASK         ( (UINT16)0x0F )
   #define DMS_RBS_FLUSH_OPTION_COLLECTIONS  ( (UINT16)0x01 )
   #define DMS_RBS_FLUSH_OPTION_HASHBKT      ( (UINT16)0x02 )
   
   // total number of meta record SYSRBS0000 has
   // currently has 2:
   // first holds _currentCollection and _lastFreeCollection
   // second holds the whole in memory bucket
   #define DMS_RBS_NUM_META_RECORDS    2

   // record offset within RBS
   class dmsRBSOffset
   {
   public:
      UINT16  _clID ;           // RBS collection id, it's the same as the
                                // one in rbs cl name (1-4095)
                                // not the collection logical id
      INT64   _logicalID ;      // offset within the collection
   public:
      dmsRBSOffset()
      {
         _clID = DMS_INVALID_CLID ;
         _logicalID = -1 ;
      }
 
      BOOLEAN  operator==(const dmsRBSOffset &rhs) const
      {
         return ((_clID == rhs._clID) && (_logicalID == rhs._logicalID) ) ;
      }

      dmsRBSOffset&  operator=(const dmsRBSOffset &rhs)
      {
         _clID = rhs._clID ;
         _logicalID = rhs._logicalID ;
         return *this ;
      }

      OSS_INLINE BOOLEAN isValid() const
      {
         BOOLEAN rv = TRUE ;
         if ( _clID == DMS_INVALID_CLID || _logicalID == -1 )
         { 
            rv = FALSE ;
         }
         return rv ;
      }
   } ;

   class _dmsRBSHashBkt
   {
   private:
      // we may have different implementation of how to store and access
      // the old versions. Eventually, we may want to cache the newest 
      // old "version" in memory, which could be hanging off the record
      // lock. 
      // Full size is 32k* (40+12)B = 1.6MB
      dmsRBSOffset   _offset[ DMS_RBS_HASH_BKT_SLOTS ] ;  // offset on disk
      ossSpinXLatch  _latch[ DMS_RBS_HASH_BKT_SLOTS ] ;   // latch to protect the bucket
      SINT64         _recordLogicalID ; // the logicalID of meta record which
                                        // stores hashbkt on disk

   public: 
      _dmsRBSHashBkt()
      {
         _recordLogicalID = DMS_INVALID_REC_LOGICALID ;
      }

      void   lock( UINT32 bkt )
      {
         _latch[bkt].get() ;
      }
      void   release( UINT32 bkt )
      {
         _latch[bkt].release() ;
      }

      void   setOffset( dmsRBSOffset & o, UINT32 bkt ) 
      {
         _offset[bkt] = o ;
      }
      dmsRBSOffset & getOffset ( UINT32 bkt )
      {
         return _offset[bkt] ;
      }

      void setLogicalID( SINT64 id )
      {
         _recordLogicalID = id ;
      }
      SINT64 getLogicalID()
      {
         return _recordLogicalID ;
      }

      CHAR * getObj()
      {
         return (CHAR *)_offset ;
      }

      SINT32 getObjSize()
      {
         return sizeof(_offset) ;
      }
   } ;

   class _dmsRBSSUMgr : public _dmsSysSUMgr
   {
   private :

      // The collection currently in use and the previously freed collecion
      // These are basically in memory version of the meta record stored
      // in SYSRBS0000. it's for quick look up of current value so we 
      // can directly use the collection.
      // They are protected by metaCL's mbLatch
      UINT16  _currentCollection ;
      UINT16  _lastFreeCollection ;

      // The max size of each collection
      UINT32  _maxCollectionSize ;

      CHAR _metaCLName[30] ;

      // The hash bucket to point to the head of the record. 
      _dmsRBSHashBkt    _rbsRecordBkt ;
 
   public :
      _dmsRBSSUMgr ( _SDB_DMSCB *dmsCB ) ;

      // this function verify whether RBS collection space exist. If it
      // is not exist then create one. And then reset all temp collections
      SINT32 init() ;
      SINT32 fini() ;

      SINT32 release ( _dmsMBContext *&context ) ;

      SINT32 reserve ( _dmsMBContext **ppContext, UINT64 eduID ) ;

      SINT32 loadMeta () ;
      SINT32 loadHashBkt( dmsMBContext *context ) ;
      SINT32 flushMeta( UINT16        curCL,
                        UINT16        lastFreeCL,
                        SDB_DPSCB    *dpsCB,
                        UINT16        flushOption,
                        dmsMBContext *context = NULL ) ;

      SINT32 appendRecord ( dmsStorageUnitID  csid,
                            UINT16            clid,
                            DPS_LSN_OFFSET    lsn,
                            const dmsRecordID &rid,
                            DPS_TRANS_ID      recordTransid,
                            DPS_TRANS_ID      ownerTransid,
                            const BSONObj     &obj );

      SINT32 getRecord ( dmsStorageUnitID  csid,
                         UINT16            clid,
                         DPS_LSN_OFFSET    &lsn,
                         dmsRecordID      &rid,
                         DPS_TRANS_ID      transid,
                         BOOLEAN          &found,
                         dmsRecordData    &record ) ; 
      void gcRBS ( ) ;

   private:

      SINT32 _getMeta ( UINT16 &curCL, 
                        UINT16 &lastFreeCL, 
                        dmsMBContext *context  ) ;

      SINT32 _initRBSCS( pmdEDUCB *eduCB, SDB_DPSCB * dpsCB ) ;

      // release a collection
      SINT32 _release ( ) ;

      SINT32 _rebuildHashBktFromCL( dmsMBContext *context ) ;

      // insert the meta record during create
      SINT32 _insertMeta ( UINT16 curCL, 
                           UINT16 lastFreeCL,
                           dmsMBContext *context,
                           SDB_DPSCB * dpsCB ) ;

      // based on csId, clID and rid to hash to a bucket
      OSS_INLINE UINT32 _hash ( dmsStorageUnitID   _csID ,
                                UINT16             _clID ,
                                const dmsRecordID &_rid ) ;

      // based on csId, clID and lsn to hash to a bucket
      OSS_INLINE UINT32 _hash ( dmsStorageUnitID  _csID ,
                                UINT16            _clID ,
                                DPS_LSN_OFFSET   &_lsn ) ;

      SINT32 _prepareRBSCLForRecord( UINT32        recordSize,
                                     pmdEDUCB     *eduCB,
                                     SDB_DPSCB    *dpsCB,
                                     dmsMBContext *& clContext ) ;

      SINT32 _allocRBSRecordSpace( UINT32        size,
                                   dmsRBSOffset &newOffset,
                                   pmdEDUCB     *eduCB,
                                   SDB_DPSCB    *dpsCB,
                                   dmsMBContext *metaContext,
                                   dmsMBContext *&clContext ) ;
      SINT32 _gcRBS ( UINT16 &position, SDB_DPSCB *dpsCB ) ;

   } ;
   typedef class _dmsRBSSUMgr dmsRBSSUMgr ;

   OSS_INLINE UINT32 _dmsRBSSUMgr::_hash ( dmsStorageUnitID   _csID ,
                                           UINT16             _clID ,
                                           const dmsRecordID &_rid ) 
   {
      UINT64 b = 0 ;
      b |= (UINT64)(_csID & 0xFFF) << 52 ;
      b |= (UINT64)(_rid._extent & 0xFFFFFF) << 28 ;
      b |= (_rid._offset & 0xFFFFFFF) ;

      // ossHash use DJB Hash ( Daniel J. Bernstein ) algorithm :
      //   h(i) = h(i-1) * 33 + str[i]
      // bitwise multiplication x << 5 + x it equivalent to x * 33,
      // where the magic 5 comes. However, there is no adequate
      // explaination on why 33 is choosed as multiplier
      return ( ossHash( (CHAR*)&( b ), (sizeof( b )), 5 ) ) % 
               DMS_RBS_HASH_BKT_SLOTS ;
   }

   OSS_INLINE UINT32 _dmsRBSSUMgr::_hash ( dmsStorageUnitID  _csID ,
                                           UINT16            _clID ,
                                           DPS_LSN_OFFSET   &_lsn ) 
   {
      UINT64 b = 0 ;
      // Use 12 bits out of 32 for CSID ( cover 4096 CSs ),
      // Use 8 bits out of 16 for CLID ( cover 256 CLs),
      // Use 44 bits out 64 for lsn offset ( which can cover years
      // of logs for busy system )
      b |= (UINT64)(_csID & 0xFFF) << 52 ;
      b |= (UINT64)(_clID & 0xFF) << 44 ;
      b |= (_lsn & 0xFFFFFFFFFFF) ;

      // ossHash use DJB Hash ( Daniel J. Bernstein ) algorithm :
      //   h(i) = h(i-1) * 33 + str[i]
      // bitwise multiplication x << 5 + x it equivalent to x * 33,
      // where the magic 5 comes. However, there is no adequate
      // explaination on why 33 is choosed as multiplier
      return ( ossHash( (CHAR*)&( b ), (sizeof( b )), 5 ) ) % 
               DMS_RBS_HASH_BKT_SLOTS ;
   }

   /*
      _dmsRBSGCJob define
      Class to trigger RBS GC work in the background
   */
   class _dmsRBSGCJob : public _utilLightJob
   {
      public:
         _dmsRBSGCJob( _dmsRBSSUMgr  *rbsSUMgr ) ;
         virtual ~_dmsRBSGCJob() ;
         virtual const CHAR*     name() const ;
         virtual INT32        doit( IExecutor *pExe,
                                       UTIL_LJOB_DO_RESULT &result,
                                       UINT64 &sleepTime ) ;
      private:
         _dmsRBSSUMgr * _rbsSUMgr ;
   } ;
   typedef _dmsRBSGCJob dmsRBSGCJob ;

   void  dmsStartAsyncRBSGC() ;
}
#endif //DMSRBSSUMGR_HPP__

