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

   Source File Name = lsmIdxKey.hpp

   Descriptive Name = LSM Index Key Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/04/2021  JT  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef LSMIDXKEY_HPP_
#define LSMIDXKEY_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "ossUtil.h"
#include "vessel/lsm/lsmIdxID.hpp"
#include "../bson/bson.h"
#include "../bson/ordering.h"
#include "vessel/recordID.h"
#include "ixmKey.hpp"
#include "dpsTransID.hpp"
#include "dmsRBSSUMgr.hpp"  // dmsRBSOffset
#include "rocksdb/rocksdb_namespace.h"
#include "rocksdb/slice.h"
#include "rocksdb/comparator.h"

using namespace bson ;
using namespace rocksdb ;
namespace engine
{
namespace vessel
{
// SDB LSM Tree Index is inspired by RocksDB key-value storage.
// It serves as a LSM Tree type index manager with MVCC support.
//
// As implemented with a K-V store, each SDB LSM entry( K-V pair ) may serve
// for different purpose, so it requires specific encoding regarding its type.
// There are two types of entry in current desgin:
//
// 1. Normal index entry, EntryType = LSM_ENTRY_TYPE_DATA
//       Key:   EntryType_IndexID_Ordering_EncodedKey_RowID_LSN_TransID
//       Value: Flag_RBSOffset
//
// 2. Checkpoint entry,  EntryType = LSM_ENTRY_TYPE_CKPT
//       Key:   EntryType_IndexID_LSN
//       Value: NULL
//    Note: the IndexID here is just padding(filled with all 0xFF),
//          try to make the two types of Key with same length prefix
//
// Here
// * EntryType:   UINT8, entry type,
//                   0x0 index key
//                   0x1 checkpoint
// * '_':         nothing, is just for readability
// * IndexID:     sdbIndexID, unique index id, { csID, clID, indexLLID }
// * Ordering:    Ordering, key object( BSON object ) ordering
// * EncodedKey:  ixmKey, key object, the raw data stream
// * RowID:       vessel::recordID, record id
// * LSN:         UINT64, sdb log sequence number
// * TransID:     SDB global transaction number
//
// * Flag:        UINT8, entry flag,
//                   0x0 normal
//                   0x1 marked as deleted
// * RBSOffset:   offset/position in roll back segment ( RBS )

// LSM entry type
#define LSM_ENTRY_TYPE_DATA    ((UINT8)0x0)  /* normal index entry */
#define LSM_ENTRY_TYPE_CKPT    ((UINT8)0x1)  /* checkpoint */
#define LSM_ENTRY_TYPE_INVALID ((UINT8)0xFF)

// LSM entry flag
#define LSM_ENTRY_FLAG_NORMAL  ((UINT8)0x0)
#define LSM_ENTRY_FLAG_DELETED  ((UINT8)0x1)
#define LSM_ENTRY_FLAG_INVALID ((UINT8)0xFF)


// calculate the length of full data key( i.e., with all fields/elements )
extern UINT32 lsmCalFullDataKeyLen(BOOLEAN bAppOpLSN, const ixmKey &keyObj );

// Estimate the total size of memory when construct a slice from the input.
// Return vaule, the total number of bytes needed to construct the slice
// Note:
//   if a pointer of a field is NULL, this field and subsequent fields
//   will not be counted in.
extern UINT32 lsmEstDataKeyLen
(
   BOOLEAN                  bAppendOpLSN,       // if append log operation LSN
   const sdbIndexID       * pIdxID,             // indexID
   const Ordering         * pOrdering  = NULL,  // ordering
   const ixmKey           * pKeyObj    = NULL,  // keyObj
   const vessel::recordID * pRid       = NULL,  // rowid
   const UINT64           * pDataLSN   = NULL,  // dataLSN
   const DPS_TRANS_ID     * pTransID   = NULL   // transID
) ;

// Pack data entry fields into raw memory buffer.
// Return value, the total number of bytes written into the buffer.
// Note:
//   if a field pointer is NULL, this field and the subsequent fields
//   will be discarded.
extern UINT32 lsmPackDataKey
(
   CHAR                   * buf,                // buffer address
   UINT32                   bufSz,              // buffer size
   BOOLEAN                  bAppendOpLSN,       // if append log operation LSN
   UINT64                   opLSN,              // log operation LSN
   const sdbIndexID       * pIdxID,             // indexID
   const Ordering         * pOrdering  = NULL,  // ordering
   const ixmKey           * pKeyObj    = NULL,  // keyObj
   const vessel::recordID * pRid       = NULL,  // rowid
   const UINT64           * pLSN       = NULL,  // lsn
   const DPS_TRANS_ID     * pTransID   = NULL   // transID
) ;


extern UINT32 lsmPackDataValue
(
   CHAR                   * buf,                // buffer address
   UINT32                   bufSz,              // buffer size
   const UINT8              LSN,                // flag
   const dmsRBSOffset     & rbsPos              // RBS position
) ;


// Pack checkpoint entry into raw memory buffer
extern UINT32 lsmPackCKPTEntry
(
   CHAR         * buf,          // buffer address
   UINT32         bufSz,        // buffer size
   BOOLEAN        bAppendOpLSN, // if log operation LSN
   UINT64         opLSN,        // log operation LSN
   UINT64         LSN           // lsn
) ;


// Extract/unpack data key fields from a slice to the destinations
// NOTE:
//  . when bAllocBuf is true, it will ALLOCATE( SDB_THREAD_ALLOC ) memory
//    in order to decode the keyObj and copy to the destination.
//    caller shall free( SDB_THREAD_FREE ) this memory at the proper time to
//    avoid memory leaking.
//    when bAllocBuf is false, it will NOT allocate memory, the raw
//    buffer address will be saved in pObjdata. In this case, caller shall not
//    try to free the memory.
//  . except the keyObj field, it will NOT allocate memory when copy
//    the extracted data from slice to the destination. The pointer to each
//    field passed in must be a valid address
//  . pass NULL for an uninterested field, so it will not unpack that field
extern void lsmUnpackDataKey
(
   const rocksdb::Slice & aSlice,           // slice
   const BOOLEAN          bAllocBuf,        // if allocate buffer for keyObj
   sdbIndexID           * pIdxID,           // indexID
   Ordering             * pOrdering,        // ordering
   CHAR               * * pObjdata,         // keyObj raw data
   UINT32               * pObjSz,           // keyObj objsize
   vessel::recordID     * pRid,             // rowid
   UINT64               * pLSN,             // lsn
   DPS_TRANS_ID         * pTransID,         // transID
   UINT64               * pOpLSN    = NULL  // log operation LSN
) ;


// unpack the value part of a data entry
//  . The pointer to each field passed in must be a valid address
//  . pass NULL for an uninterested field, so it will not unpack that field
extern void lsmUnpackDataValue
(
   const rocksdb::Slice & aSlice,
   UINT8                * valueFlag,
   dmsRBSOffset         * rbsPos
) ;


// unpack checkpoint entry
extern void lsmUnpackCKPTEntry
(
   const rocksdb::Slice & aSlice,        // slice
   UINT64               * pLSN,          // lsn
   UINT64               * pOpLSN  = NULL // log operation LSN
) ;


// validate whether a packed Slice has same IndexID
extern BOOLEAN lsmIsSameIdxId
(
   const rocksdb::Slice & aSlice,
   const sdbIndexID     & idxId
) ;


// validate whether a packed Slice satisfies all checking criteria.
// If a passed in pointer is NULL that field checking will be skipped
extern BOOLEAN lsmIsSameIndexKey
(
   const rocksdb::Slice   & aSlice,
   const ixmKey           * pKeyObj = NULL,
   const vessel::recordID * pRid    = NULL,
   const sdbIndexID       * pIdxId  = NULL
) ;

// update the packed data entry to the most adjacent one,
// say increase/decrease LSN by 1
extern void lsmUpdateDataEntryToMostAdjacent(rocksdb::Slice a, INT32 direction);

const UINT32 lsmEntryTypeSz    = sizeof( UINT8 ) ;
const UINT32 lsmOrdSz          = sizeof( Ordering ) ;
const UINT32 lsmRidSz          = sizeof(recordID);
const UINT32 lsmLsnSz          = sizeof( UINT64 ) ;
const UINT32 lsmTxIDSz         = sizeof( DPS_TRANS_ID ) ;
const UINT32 lsmFlagSz         = sizeof( UINT8 );
const UINT32 lsmRBSPosSz       = sizeof( dmsRBSOffset ) ;
const UINT32 lsmDummyKeyObjSz  = 6 ; // ixmKeyOwned(BSONObj()).dataSize();
const UINT32 lsmMinDataKeySz   = lsmEntryTypeSz + lsmIdxIDSz
                                 + lsmOrdSz + lsmDummyKeyObjSz + lsmRidSz
                                 + lsmLsnSz + lsmTxIDSz ;

// Return SDB LSM key comparator
extern const rocksdb::Comparator* lsmKeyComparator();

class lsmKeyEntry : public SDBObject
{
public:
   lsmKeyEntry()
   {
      _dataLsn = 0 ;
      _flag    = LSM_ENTRY_FLAG_NORMAL ;
      _rid     = vessel::recordID(0,0);
   }

   // Input parameter
   //   K:  packed key, normally it is readed from rocksdb
   //   V:  packed value associated with key
   void init( const rocksdb::Slice & K, const rocksdb::Slice & V );

   void init( const BSONObj          & keyObj,
              const vessel::recordID & rid,
              const UINT64             dataLsn,
              const DPS_TRANS_ID     & transID,
              const UINT8              flag,
              const dmsRBSOffset     & rbsPos )
   {
      _keyObj  = keyObj.getOwned() ;
      _rid     = rid;
      _dataLsn = dataLsn;
      _transID = transID;
      _flag    = flag;
      _rbsPos  = rbsPos;
   }

   lsmKeyEntry( const rocksdb::Slice & K, const rocksdb::Slice & V )
   {
      _dataLsn = 0 ;
      _flag    = LSM_ENTRY_FLAG_NORMAL ;
      _rid     = vessel::recordID(0,0);
      init( K,V ) ;
   }

   lsmKeyEntry( const lsmKeyEntry & rhs )
   {
      _keyObj  = rhs._keyObj.getOwned() ;
      _rid     = rhs._rid;
      _dataLsn = rhs._dataLsn;
      _transID = rhs._transID;
      _flag    = rhs._flag;
      _rbsPos  = rhs._rbsPos;
   }

   lsmKeyEntry & operator= ( const lsmKeyEntry &rhs )
   {
      _keyObj  = rhs._keyObj.getOwned() ;
      _rid     = rhs._rid;
      _dataLsn = rhs._dataLsn;
      _transID = rhs._transID;
      _flag    = rhs._flag;
      _rbsPos  = rhs._rbsPos;
      return *this ;
   }

   void shallowCopy( const BSONObj          & keyObj,
                     const vessel::recordID & rid,
                     const UINT64             dataLsn,
                     const DPS_TRANS_ID     & transID,
                     const UINT8              flag,
                     const dmsRBSOffset     & rbsPos )
   {
      _keyObj  = keyObj ;
      _rid     = rid;
      _dataLsn = dataLsn;
      _transID = transID;
      _flag    = flag;
      _rbsPos  = rbsPos;
   }

   void shallowCopy( const lsmKeyEntry &rhs )
   {
      _keyObj  = rhs._keyObj ;
      _rid     = rhs._rid;
      _dataLsn = rhs._dataLsn;
      _transID = rhs._transID;
      _flag    = rhs._flag;
      _rbsPos  = rhs._rbsPos;
   }

   void reset()
   {
      _keyObj  = BSONObj() ;
      _rid     = vessel::recordID(0,0);
      _dataLsn = 0 ;
      _transID = DPS_TRANS_ID();
      _flag    = LSM_ENTRY_FLAG_NORMAL;
      _rbsPos  = dmsRBSOffset();
   }

   virtual ~lsmKeyEntry() {}

   OSS_INLINE BSONObj getKeyObj() const { return _keyObj; }
   OSS_INLINE vessel::recordID getRid() const { return _rid; }
   OSS_INLINE UINT64 getDataLsn() const { return _dataLsn; }
   OSS_INLINE DPS_TRANS_ID getTransID() const { return _transID; }
   OSS_INLINE UINT8 getFlag() const { return _flag; }
   OSS_INLINE dmsRBSOffset getRBSPos() const { return _rbsPos; }

protected:
   BSONObj          _keyObj;
   vessel::recordID _rid;
   UINT64           _dataLsn;
   DPS_TRANS_ID     _transID;

   UINT8            _flag;     // delete flag
   dmsRBSOffset     _rbsPos;
} ;


} //namespace vessel
} // namespace engine
#endif  // LSMIDXKEY_HPP_
