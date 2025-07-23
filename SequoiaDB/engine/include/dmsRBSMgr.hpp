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

   Source File Name = dmsRBSMgr.hpp

   Descriptive Name = DMS Roll Back Segment Management Header

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS RollBack Segment Storage Unit Management.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/10/2019  CYX Initial Draft
          01/06/2020  HGM Copy from dmsRBSSUMgr.hpp

   Last Changed =

*******************************************************************************/
#ifndef DMSRBSMGR_HPP__
#define DMSRBSMGR_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "dmsRBSSUMgr.hpp"

namespace engine
{

   // Max allowed RBS GC tasks
   #define MAX_RBS_GC_TASK 3

   /*
       _dmsRBSMgr define
    */
   // _dmsRBSMgr is manager for RBS storage units
   class _dmsRBSMgr
   {
   public:
      _dmsRBSMgr() ;

      // this function verify whether RBS collection space exist. If it
      // is not exist then create one. And then reset all temp collections
      INT32 init( UINT32 rbsNum ) ;
      INT32 fini() ;

      INT32 rbsAppendRecord( dmsStorageUnitID   csid,
                             UINT16             clid,
                             UINT32             clLID,
                             const dmsRecordID &rid,
                             DPS_TRANS_ID      &recordTransid,
                             DPS_TRANS_ID      &ownerTransid,
                             const BSONObj     &obj,
                             dmsTransLockCallback * callback = NULL ) ;

      INT32 rbsGetRecord( dmsStorageUnitID  csid,
                          UINT16            clid,
                          UINT32            clLID,
                          dmsRecordID      &rid,
                          DPS_TRANS_ID     &transid,
                          BOOLEAN          &found,
                          dmsRecordData    &record,
                          dmsRBSOffset     &startPos,
                          dmsRBSOffset     &endPos,
                          const DPS_TRANS_ID &diskRecordTransID ) ;


      void gcRBS() ;
      void incActiveGC() { _numActiveGC.inc() ; }
      void decActiveGC() { _numActiveGC.dec() ; }
      UINT32 getNumActiveGC() { return _numActiveGC.fetch() ; }
      BOOLEAN allowGC() { return _numActiveGC.fetch() < MAX_RBS_GC_TASK ; }

      UINT32 getNumSyncAddCL() ;
      UINT32 getCLSize() { return DMS_DFT_RBSCL_SIZE ; }
      UINT32 getNumTotalCL() { return DMS_MAX_RBS_CL * _rbsNum ; }
      UINT32 getNumFreeCL() ;

   protected:
      OSS_INLINE UINT32 _hash ( dmsStorageUnitID   _csID ,
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

      OSS_INLINE UINT32 _hash ( dmsStorageUnitID  _csID ,
                                UINT16            _clID ,
                                DPS_LSN_OFFSET   &_lsn )
      {
         // NOTE: hash with LSN is not used yet
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

      INT32 _initRBSSUMgrs( UINT32 rbsNum ) ;

   protected:
      // Number of active GC thread
      ossAtomic32 _numActiveGC ;
      // number of RBS storage units
      // NOTE: should be power of 2
      UINT32      _rbsNum ;
      // _rbsNum - 1, used for quick modulus
      UINT32      _rbsModulo ;
      // power number of _rbsNum
      UINT32      _rbsPowerIndex ;
      // array for RBS storage units
      dmsRBSSUMgr *_rbsSUMgrs ;
   } ;

   typedef class _dmsRBSMgr dmsRBSMgr ;

}

#endif //DMSRBSMGR_HPP__
