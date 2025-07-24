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
#include "monLatch.hpp"
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
   // class forward declaration
   class dmsTransLockCallback ;
   class _dmsRBSMgr ;

   // number of slots in RBS hash bucket, a prime number less than 32K
   //#define  DMS_RBS_HASH_BKT_SLOTS   ( (UINT32) 32749 )
   // number of slots in RBS hash bucket, a prime number less than 256K
   #define  DMS_RBS_HASH_BKT_SLOTS   ( (UINT32) 262139 )

   #define DMS_BUILD_RBS_CL_NAME( clName, cl )             \
               ossSnprintf ( clName, sizeof(clName),       \
                             DMS_RBS_NAME_PATTERN,         \
                             SDB_DMSRBS_NAME, cl )

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

      OSS_INLINE void reset()
      {
         _clID = DMS_INVALID_CLID ;
         _logicalID = -1 ;
      }
   } ;

   class _dmsRBSHashBkt
   {
   private:
      // we may have different implementation of how to store and access
      // the old versions. Eventually, we may want to cache the newest 
      // old "version" in memory, which could be hanging off the record
      // lock. 
      // Full size is 256k* (12+56)B = 16MB
      dmsRBSOffset   _offset[ DMS_RBS_HASH_BKT_SLOTS ] ;  // offset on disk
      monSpinXLatch  _latch[ DMS_RBS_HASH_BKT_SLOTS ] ;   // latch to protect the bucket
      // Full size is 32k* (12+40)B = 1.6MB
      //ossSpinXLatch  _latch[ DMS_RBS_HASH_BKT_SLOTS ] ;   // latch to protect the bucket

   public: 
      _dmsRBSHashBkt()
      {
         for ( UINT32 i = 0; i < DMS_RBS_HASH_BKT_SLOTS; i++ )
         {
            _latch[i] = monSpinXLatch( MON_LATCH_RBSHASHBKT_BUCKETLATCH ) ;
         }
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

      CHAR * getObj()
      {
         return (CHAR *)_offset ;
      }

      SINT32 getObjSize()
      {
         return sizeof(_offset) ;
      }

      void reset()
      {
         for ( UINT32 i = 0 ; i < DMS_RBS_HASH_BKT_SLOTS ; ++ i )
         {
            _offset[ i ].reset() ;
         }
      }
   } ;

   class _dmsRBSSUMgr : public _dmsSysSUMgr
   {
   private :
      UINT32          _index ;
      _dmsRBSMgr *    _rbsMgr ;
      // Keep track of 
      // - the collection currently in use;
      // - the collection previously freed;
      // - the collection prepared by async thread;
      //
      // They are protected by _latch. Protocol as following:
      // -  Read of any of the three fields require _latch in S;
      //    Update of them require X. 
      // -  Creation on new RBSCL require the latch in X so that there
      //    is only one guy creating new RBSCL.
      // -  Drop expired CLs only need holding mbLock of the CL. So caller
      //    need to check if the CL exist and try the mbLock in X.
      // -  Background job (GC) will try its best to prepare one new CL ahead
      //    of team so the writer does not have to create CL synchronously.
      //    Since we may have more than one GC thread, we use _prepInProgress
      //    to multiple guys from preparing the collections at same time.
      UINT16          _currentCollection ;
      UINT16          _lastFreeCollection ;
      UINT16          _preparedCollection ;
      monSpinSLatch   _latch ;
      BOOLEAN         _prepInProgress ;

      // The max size of each collection
      UINT32          _maxCollectionSize ;

      // Number of time the add CL was not performed by GC
      ossAtomic32     _numSyncAddCL ;

      CHAR            _metaCLName[30] ;

      // The hash bucket to point to the head of the record. 
      _dmsRBSHashBkt  _rbsRecordBkt ;

      CHAR            _metaCSName[ 30 ] ;
 
   public :
      _dmsRBSSUMgr () ;

      // this function verify whether RBS collection space exist. If it
      // is not exist then create one. And then reset all temp collections
      SINT32 init( _dmsRBSMgr *rbsMgr, UINT32 index ) ;
      SINT32 fini() ;

      SINT32 rbsAppendRecord ( dmsStorageUnitID   csid,
                               UINT16             clid,
                               UINT32             clLID,
                               const dmsRecordID &rid,
                               UINT32             bucketID,
                               DPS_TRANS_ID      &recordTransid,
                               DPS_TRANS_ID      &ownerTransid,
                               const BSONObj     &obj,
                               dmsTransLockCallback * callback = NULL );

      SINT32 rbsGetRecord ( dmsStorageUnitID  csid,
                            UINT16            clid,
                            UINT32            clLID,
                            dmsRecordID      &rid,
                            UINT32             bucketID,
                            DPS_TRANS_ID     &transid,
                            BOOLEAN          &found,
                            dmsRecordData    &record,
                            dmsRBSOffset     &startPos,
                            dmsRBSOffset     &endPos,
                            const DPS_TRANS_ID &diskRecordTransID ) ;


      void gcRBS ( ) ;
      UINT32 getNumSyncAddCL() { return _numSyncAddCL.fetch() ; }

      UINT32 getCLSize() { return DMS_DFT_RBSCL_SIZE ; }
      UINT32 getNumTotalCL() { return DMS_MAX_RBS_CL ; }
      UINT32 getNumFreeCL()
      {
         if ( _lastFreeCollection > _currentCollection )
         {
            return (_lastFreeCollection - _currentCollection) ;
         }
         else
         {
            return getNumTotalCL() - _currentCollection + _lastFreeCollection ;
         }
      }

   private:

      SINT32 _initRBSCS( pmdEDUCB *eduCB, SDB_DPSCB * dpsCB ) ;

      void  _latchX() { _latch.get() ; }

      void  _releaseX() { _latch.release() ; }

      void  _latchS() { _latch.get_shared() ; }

      void  _releaseS() { _latch.release_shared() ; }

      SINT32 _prepareRBSCLForRecord( UINT32        recordSize,
                                     pmdEDUCB     *eduCB,
                                     SDB_DPSCB    *dpsCB,
                                     dmsMBContext *& clContext,
                                     UINT16        & rbsclID ) ;

      SINT32 _allocRBSRecordSpace( UINT32        size,
                                   dmsRBSOffset &newOffset,
                                   pmdEDUCB     *eduCB,
                                   SDB_DPSCB    *dpsCB,
                                   dmsMBContext *metaContext,
                                   dmsMBContext *&clContext ) ;

      BOOLEAN _needPrepareRBSCL( BOOLEAN latched ) ;

      SINT32 _prepareRBSCL( pmdEDUCB   * eduCB,
                            SDB_DPSCB  * dpsCB,
                            BOOLEAN      updateCurCL ) ;

      SINT32 _gcRBS ( UINT16 position, SDB_DPSCB *dpsCB ) ;

      BOOLEAN _rbsPositionExpired( dmsRBSOffset &pos ) ;
      BOOLEAN _rbsCLExpired( UINT16 cl ) ;
   } ;
   typedef class _dmsRBSSUMgr dmsRBSSUMgr ;

}
#endif //DMSRBSSUMGR_HPP__

