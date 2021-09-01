#include "vessel/lsm/lsmIdxKey.hpp"
#include "utilMemListPool.hpp"
#include "pd.hpp"
#include <string>

using namespace bson ;
using namespace std ;
using namespace rocksdb ;

namespace engine
{
namespace vessel
{

// LSM Key rocksdb::Comparator implementation
class LsmKeyComparatorImpl : public rocksdb::Comparator
{
public:
  LsmKeyComparatorImpl() {}

  // SDB LSM Tree Index is inspired by RocksDB key-value storage.
  // It serves as a LSM Tree type index manager with MVCC support.
  //
  // As implemented with a K-V store, each SDB LSM entry( K-V pair ) may serve
  // for different purpose, so requires specific encoding regarding its type.
  // There are two types of entry in current desgin:
  //
  // 1. Normal index entry, EntryType = LSM_ENTRY_TYPE_DATA
  //       Key:   EntryType_IndexID_Ordering_EncodedKey_RowID_LSN_TransID
  //       Value: Flag_RBSOffset
  //
  // 2. Checkpoint entry, EntryType = LSM_ENTRY_TYPE_CKPT
  //       Key:   EntryType_IndexID_LSN
  //       Value: NULL
  //    Note: the IndexID here is just padding( all bytes filled with 0xFF ),
  //          try to make the two types of Key with same length prefix
  //
  // Here
  //
  // * EntryType:   UINT8, sdb lsm entry type,
  //                  0x0 index key
  //                  0x1 checkpoint
  // * '_':         nothing, is just for readability
  // * IndexID:     globalIndexID, unique index id, { csID, clID, indexLLID }
  // * Ordering:    Ordering, key object ordering information
  // * EncodedKey:  ixmKey, key object, the raw data stream
  // * RowID:       vessel::recordID, record id
  // * LSN:         UINT64, sdb log sequence number
  // * TransID:     SDB global transaction number
  //
  // * Flag:        UINT8, entry flag,
  //                  0x0 normal
  //                  0x1 marked as deleted
  // * RBSOffset:   offset/position in roll back segment ( RBS )

  // Three-way comparison.  Returns value:
  //   < 0 iff "a" < "b",
  //   == 0 iff "a" == "b",
  //   > 0 iff "a" > "b"
  #define _gt_ ((int)1)
  #define _lt_ ((int)-1)
  #define _eq_ ((int)0)
  #define LSM_COMP_CHECK_SLICE_LENGTH_FOR_NEXT_FIELD( _Len_ )  \
  if ( aSize < ( aOffset + _Len_ ) )                           \
  {                                                            \
     if ( bSize < ( bOffset + _Len_ ) )                        \
     {                                                         \
        return _eq_ ;                                          \
     }                                                         \
     else                                                      \
     {                                                         \
        return _lt_ ;                                          \
     }                                                         \
  }                                                            \
  else if ( bSize < ( bOffset + _Len_ ) )                      \
  {                                                            \
     return _gt_ ;                                             \
  }

  int Compare
  (
     const rocksdb::Slice & a,
     const rocksdb::Slice & b
  ) const override
  {
     const CHAR * aStart = a.data();       // address of Slice/Key a
     const CHAR * bStart = b.data();       // address of Slice/Key b
     const UINT32 aSize  = a.size(),       // the length of Slice a
                  bSize  = b.size();       // the legnth of Slice b
     UINT32       aOffset = 0,             // field offset in a
                  bOffset = 0;             // field offset in b

     // validate the length of slice a and b is long enough for EntryType field
     LSM_COMP_CHECK_SLICE_LENGTH_FOR_NEXT_FIELD( lsmEntryTypeSz )

     // extract EntryType field
     UINT8 aType = static_cast<UINT8>(a[aOffset]),
           bType = static_cast<UINT8>(b[bOffset]);
     // move offset to next field
     aOffset += lsmEntryTypeSz;
     bOffset += lsmEntryTypeSz;

     // compare EntryType field
     if ( aType == bType )
     {
        int result = _eq_ ;

        if ( LSM_ENTRY_TYPE_DATA == aType )
        {
           // validate the length of a and b for IndexID field
           LSM_COMP_CHECK_SLICE_LENGTH_FOR_NEXT_FIELD( lsmIdxIDSz )

           // extract IndexID field
           globalIndexID *a_pIdxID = (globalIndexID*)(aStart + aOffset),
                      *b_pIdxID = (globalIndexID*)(bStart + bOffset);
           // move offset to next field
           aOffset += lsmIdxIDSz;
           bOffset += lsmIdxIDSz;

           // compare indexID filed
           if ( *a_pIdxID < *b_pIdxID )
           {
              return _lt_ ;
           }
           else if ( !( *a_pIdxID == *b_pIdxID ) ) // aIdxID > bIdxID
           {
              return _gt_ ;
           }

           SDB_ASSERT( ( _eq_ == result ), "Invalid result" ) ;

           // check the length of a and b is long enough to contain
           // ordering and keyObj
           LSM_COMP_CHECK_SLICE_LENGTH_FOR_NEXT_FIELD( lsmOrdSz + 2 )

           // extract ordering field
           Ordering *a_pOrder = (Ordering*)(aStart + aOffset);

           // move offset to next field
           aOffset += lsmOrdSz;
           bOffset += lsmOrdSz;

           // extract keyObj field
           ixmKey aKeyObj( (const char*)(aStart + aOffset) ),
                  bKeyObj( (const char*)(bStart + bOffset) );
           LSM_COMP_CHECK_SLICE_LENGTH_FOR_NEXT_FIELD( aKeyObj.dataSize() )
           // move offset to next field
           aOffset += aKeyObj.dataSize();
           bOffset += bKeyObj.dataSize();

           // compare keyObj field
           result = aKeyObj.woCompare( bKeyObj, *a_pOrder );
           if ( _eq_ != result )
           {
              return result ;
           }

           // check the length of a and b is long enough to contain RowID
           LSM_COMP_CHECK_SLICE_LENGTH_FOR_NEXT_FIELD( lsmRidSz )

           // extract RowID field
           vessel::recordID *a_pRid = (vessel::recordID*)(aStart + aOffset),
                            *b_pRid = (vessel::recordID*)(bStart + bOffset);
           // move offset to next field
           aOffset += lsmRidSz;
           bOffset += lsmRidSz;

           // compare RowID
           if ( *a_pRid < *b_pRid )
           {
              return _lt_ ;
           }
           else if ( !( *a_pRid == *b_pRid ) )
           {
              return _gt_ ;
           }

           // check the length of a and b is long enough to contain LSN
           LSM_COMP_CHECK_SLICE_LENGTH_FOR_NEXT_FIELD( lsmLsnSz )

           // extract LSN field
           UINT64 aLSN = *((const UINT64 *)(aStart + aOffset));
           UINT64 bLSN = *((const UINT64 *)(bStart + bOffset));
           // move to next field
           aOffset += lsmLsnSz;
           bOffset += lsmLsnSz;

           // compare LSN
           // key is sorted on LSN with decending order
           if ( aLSN > bLSN )
           {
              return _lt_ ;
           }
           else if ( aLSN < bLSN )
           {
              return _gt_ ;
           }

           // check the length of a and b is long enough to contain transID
           LSM_COMP_CHECK_SLICE_LENGTH_FOR_NEXT_FIELD( lsmTxIDSz )

           // extract transID field
           DPS_TRANS_ID *a_pTransID = (DPS_TRANS_ID*)(aStart + aOffset),
                        *b_pTransID = (DPS_TRANS_ID*)(bStart + bOffset);
           // move to next field
           aOffset += lsmTxIDSz;
           bOffset += lsmTxIDSz;

           // compare transID
           // key is sorted on tranID with decending order
           if ( *a_pTransID < *b_pTransID )
           {
              return _gt_ ;
           }
           else if ( *a_pTransID != *b_pTransID )
           {
              return _lt_ ;
           }

           SDB_ASSERT( ( _eq_ == result ), "Invalid result" ) ;

           return result ;
        }
        else if ( LSM_ENTRY_TYPE_CKPT == aType )
        {
           LSM_COMP_CHECK_SLICE_LENGTH_FOR_NEXT_FIELD( lsmIdxIDSz + lsmLsnSz )
           // skip the padding field ( dummy IndexId )
           aOffset += lsmIdxIDSz;
           bOffset += lsmIdxIDSz;
           // extract LSN field
           UINT64 aLSN = static_cast<UINT64>(a[aOffset]),
                  bLSN = static_cast<UINT64>(b[bOffset]);
           // move offset to next field
           aOffset += lsmLsnSz;
           bOffset += lsmLsnSz;
           // compare LSN
           if ( aLSN < bLSN )
           {
              return  _lt_ ;
           }
           else if ( aLSN == bLSN )
           {
              return _eq_ ;
           }
           else
           {
              return _gt_ ;
           }
        }
        else
        {
           return a.compare( b ) ;
        }
     }
     else if ( aType < bType )
     {
        return _lt_ ;
     }
     else
     {
        return _gt_ ;
     }
  }

  const char* Name() const override { return "sdb.LsmKeyComparator"; }

  void FindShortestSeparator(std::string*,const rocksdb::Slice&)const override{}
  void FindShortSuccessor(std::string*) const override {}

} ; // class LsmKeyComparatorImpl


const rocksdb::Comparator* lsmKeyComparator()
{
  static LsmKeyComparatorImpl _lsmKeyComparator;
  return & _lsmKeyComparator;
}


//
// Helper functions
//

// calculate the length of full data key( i.e., with all fields/elements )
UINT32 lsmCalFullDataKeyLen(UINT32 ixmKeySize)
{
   UINT32 result = 0;
   result += ixmKeySize +
             lsmEntryTypeSz +        // EntryType
             lsmIdxIDSz +            // IndexID
             lsmOrdSz +              // Ordering
             lsmRidSz +              // RowID
             lsmLsnSz +              // LSN
             lsmTxIDSz ;             // TransID
   return result;
}

// Estimate the total size of memory when construct a slice from the input.
// Return vaule, the total number of bytes needed to construct the slice
// Note:
//   if a pointer of a field is NULL, this field and the subsequent fields
//   will not be counted in.
#define LSM_EST_CHECK_DATA_ADDR_EXIT_FUNC_IF_NULL( _ptr_ )  \
if ( NULL == _ptr_ )                                        \
{                                                           \
   return offset ;                                          \
}

/*
UINT32 lsmEstDataKeyLen
(
   const globalIndexID       * pIdxID,             // indexID
   const Ordering         * pOrdering,          // ordering
   const ixmKey           * pKeyObj,            // keyObj
   const recordID      * pRid,               // rowid
   const UINT64           * pLSN,               // lsn
   const DPS_TRANS_ID     * pTransID            // transID
)
{
   UINT32 offset = 0 ;

   // EntryType
   offset += lsmEntryTypeSz ;

   // IndexID
   LSM_EST_CHECK_DATA_ADDR_EXIT_FUNC_IF_NULL( pIdxID )
   offset += lsmIdxIDSz ;

   // Ordering
   LSM_EST_CHECK_DATA_ADDR_EXIT_FUNC_IF_NULL( pOrdering )
   offset += lsmOrdSz ;

   // KeyObj
   LSM_EST_CHECK_DATA_ADDR_EXIT_FUNC_IF_NULL( pKeyObj )
   offset += pKeyObj->dataSize() ;

   // RowID
   LSM_EST_CHECK_DATA_ADDR_EXIT_FUNC_IF_NULL( pRid )
   offset += lsmRidSz ;

   // LSN
   LSM_EST_CHECK_DATA_ADDR_EXIT_FUNC_IF_NULL( pLSN )
   offset += lsmLsnSz ;

   // TransID
   LSM_EST_CHECK_DATA_ADDR_EXIT_FUNC_IF_NULL( pTransID )
   offset += lsmTxIDSz ;

   return offset ;
}*/


// Encode data entry fields into raw memory buffer.
// Return value, the total number of bytes written into the buffer.
// Note:
//   while encoding, if a field pointer is NULL, this field and subsequent
//   fields will be discarded.
#define LSM_PACK_CHECK_ADDR_EXIT_FUNC_IF_NULL( _ptr_, _bAppendLsn_ ) \
if ( NULL == _ptr_ )                                                 \
{                                                                    \
   if ( _bAppendLsn_ )                                               \
   {                                                                 \
      ossMemcpy( ( buf + offset ), &opLSN, lsmLsnSz ) ;              \
      offset += lsmLsnSz ;                                           \
   }                                                                 \
   return offset ;                                                   \
}

#define LSM_PACK_COLUMN_OR_GOTO_ERROR(_buf_, _bufSz_, _data_, _dataSz_) \
do\
{\
   if ((_bufSz_) < (offset + (_dataSz_)))\
   {\
      rc = SDB_INVALIDARG;\
      goto error;\
   }\
   ossMemcpy((CHAR*)(_buf_) + offset, (_data_), (_dataSz_));\
   offset += (_dataSz_);\
} while(FALSE)

#define LSM_UNPACK_COLUMN_OR_GOTO_ERROR(_buf_, _bufSz_, _data_, _dataSz_) \
do\
{\
   if ((_bufSz_) < (offset + (_dataSz_)))\
   {\
      rc = SDB_INVALIDARG;\
      goto error;\
   }\
   ossMemcpy((_data_), ((CHAR*)(_buf_) + offset), (_dataSz_));\
   offset += (_dataSz_);\
} while(FALSE)

/*
UINT32 lsmPackDataKey
(
   CHAR                   * buf,                // buffer address
   UINT32                   bufSz,              // buffer size
   const globalIndexID       * pIdxID,             // indexID
   const Ordering         * pOrdering,          // ordering
   const ixmKey           * pKeyObj,            // keyObj
   const recordID      * pRid,               // rowid
   const UINT64           * pLSN,               // lsn
   const DPS_TRANS_ID     * pTransID            // transID
)
{
   UINT32 offset = 0 ;
   UINT8  entryType = LSM_ENTRY_TYPE_DATA ;

   // verify if the buffer size is big enough
   if ( ( NULL == buf ) ||
        ( bufSz < lsmEstDataKeyLen( pIdxID,
                                    pOrdering,
                                    pKeyObj,
                                    pRid,
                                    pLSN,
                                    pTransID ) ) )
   {
      return offset ;
   }

   // EntryType
   LSM_PACK_CHECK_ADDR_EXIT_FUNC_IF_NULL( &entryType, bAppendOpLSN )
   buf[offset] = entryType ;
   offset += lsmEntryTypeSz ;

   // IndexID
   LSM_PACK_CHECK_ADDR_EXIT_FUNC_IF_NULL( pIdxID, bAppendOpLSN )
   ossMemcpy( ( buf + offset ), pIdxID, lsmIdxIDSz ) ;
   offset += lsmIdxIDSz;

   // Ordering
   LSM_PACK_CHECK_ADDR_EXIT_FUNC_IF_NULL( pOrdering, bAppendOpLSN )
   ossMemcpy( ( buf + offset ), pOrdering, lsmOrdSz );
   offset += lsmOrdSz;

   // KeyObj
   LSM_PACK_CHECK_ADDR_EXIT_FUNC_IF_NULL( pKeyObj, bAppendOpLSN )
   ossMemcpy( ( buf + offset ), pKeyObj->data(), pKeyObj->dataSize() );
   offset += pKeyObj->dataSize() ;

   // RowID
   LSM_PACK_CHECK_ADDR_EXIT_FUNC_IF_NULL( pRid, bAppendOpLSN )
   ossMemcpy( ( buf + offset ), pRid, lsmRidSz );
   offset += lsmRidSz ;

   // LSN
   LSM_PACK_CHECK_ADDR_EXIT_FUNC_IF_NULL( pLSN, bAppendOpLSN )
   ossMemcpy( ( buf + offset ), pLSN, lsmLsnSz );
   offset += lsmLsnSz ;

   // TransID
   LSM_PACK_CHECK_ADDR_EXIT_FUNC_IF_NULL( pTransID, bAppendOpLSN )
   ossMemcpy( ( buf + offset ), pTransID, lsmTxIDSz );
   offset += lsmTxIDSz ;

   // append memtable operation LSN
   if ( bAppendOpLSN )
   {
      ossMemcpy( ( buf + offset ), &opLSN, lsmLsnSz );
      offset += lsmLsnSz ;
   }

   return offset ;
}*/

INT32 lsmUnpackIndexFullKey
(
   const CHAR             * buf,        // buffer address
   UINT32                 bufSz,      // buffer size
   globalIndexID          &idxID,       // indexID
   orderingWrapper        &ordering,  // ordering
   ixmKey                 &key,  // key
   recordID            &rid,  // rowid
   UINT64                 &lsn,  // lsn
   DPS_TRANS_ID           &transID   // transID
)
{
   INT32 rc = SDB_OK;
   UINT8  entryType = LSM_ENTRY_TYPE_DATA ;
   UINT32 offset = 0;
   if (NULL == buf || 0 == bufSz)
   {
      rc = SDB_INVALIDARG;
      goto error;
   }

   LSM_UNPACK_COLUMN_OR_GOTO_ERROR(buf, bufSz, &entryType, sizeof(entryType));

   LSM_UNPACK_COLUMN_OR_GOTO_ERROR(buf, bufSz, &idxID, sizeof(globalIndexID));

   LSM_UNPACK_COLUMN_OR_GOTO_ERROR(buf, bufSz, &ordering, sizeof(orderingWrapper));

   key.assign(ixmKey(buf + offset));
   offset += key.dataSize();

   LSM_UNPACK_COLUMN_OR_GOTO_ERROR(buf, bufSz, &rid, sizeof(recordID));

   LSM_UNPACK_COLUMN_OR_GOTO_ERROR(buf, bufSz, &lsn, sizeof(UINT64));

   LSM_UNPACK_COLUMN_OR_GOTO_ERROR(buf, bufSz, &transID, sizeof(DPS_TRANS_ID));
done:
   return rc;
error:
   goto done;
}

INT32 lsmPackIndexFullKey
(
   CHAR                   * buf,        // buffer address
   UINT32                   bufSz,      // buffer size
   const globalIndexID    &idxID,       // indexID
   const orderingWrapper  &ordering,  // ordering
   const ixmKey           &key,  // keyObj
   const recordID         &rid,  // rowid
   UINT64                 lsn,  // lsn
   const DPS_TRANS_ID     &transID   // transID
)
{
   INT32 rc = SDB_OK;
   UINT8  entryType = LSM_ENTRY_TYPE_DATA ;
   UINT32 offset = 0;
   UINT32 keyDataSize = (UINT32)(key.dataSize());
   
   if (NULL == buf || !key.isValid())
   {
      rc = SDB_INVALIDARG;
      goto error;
   }

   LSM_PACK_COLUMN_OR_GOTO_ERROR(buf, bufSz, &entryType, sizeof(entryType));

   LSM_PACK_COLUMN_OR_GOTO_ERROR(buf, bufSz, &idxID, sizeof(globalIndexID));

   LSM_PACK_COLUMN_OR_GOTO_ERROR(buf, bufSz, &ordering, sizeof(orderingWrapper));

   LSM_PACK_COLUMN_OR_GOTO_ERROR(buf, bufSz, key.data(), keyDataSize);

   LSM_PACK_COLUMN_OR_GOTO_ERROR(buf, bufSz, &rid, sizeof(recordID));

   LSM_PACK_COLUMN_OR_GOTO_ERROR(buf, bufSz, &lsn, sizeof(UINT64));

   LSM_PACK_COLUMN_OR_GOTO_ERROR(buf, bufSz, &transID, sizeof(DPS_TRANS_ID));


done:
   return rc;
error:
   goto done;
}

UINT32 lsmPackDataValue
(
   CHAR                   * buf,                // buffer address
   UINT32                   bufSz,              // buffer size
   const UINT8              flag,               // flag
   const dmsRBSOffset     & rbsPos              // RBS position
)
{
   UINT32 offset = 0 ;
   // verify if the buffer size is big enough
   if ( ( NULL == buf ) || ( bufSz < ( lsmFlagSz + lsmRBSPosSz ) ) )
   {
      return offset ;
   }
   buf[offset] = flag ;
   offset += lsmFlagSz ;

   ossMemcpy( ( buf + offset ), &rbsPos, lsmRBSPosSz ) ;
   offset += lsmRBSPosSz;
   return offset ;
}


UINT32 lsmPackCKPTEntry
(
   CHAR         * buf,          // buffer address
   UINT32         bufSz,        // buffer size
   BOOLEAN        bAppendOpLSN, // if append memtable operation LSN
   UINT64         opLSN,        // memtable operation LSN
   UINT64         LSN           // lsn
)
{
   UINT32 offset = 0 ;
   UINT64 lsn    = LSN ;
   // verify if the buffer size is big enough
   if ( ( NULL == buf ) ||
        ( bufSz < ( bAppendOpLSN ? (lsmEntryTypeSz + lsmIdxIDSz + lsmLsnSz * 2)
                                 : (lsmEntryTypeSz + lsmIdxIDSz + lsmLsnSz))) )
   {
      return offset ;
   }
   // Entrype
   buf[0] = LSM_ENTRY_TYPE_CKPT ;
   offset += lsmEntryTypeSz;
   // Padding
   ossMemset( ( buf + offset ), 0xFF, lsmIdxIDSz );
   offset += lsmIdxIDSz;
   // LSN
   ossMemcpy( ( buf + offset ), &lsn, lsmLsnSz );
   offset += lsmLsnSz ;
   // append memtable operation LSN if it is needed
   if ( bAppendOpLSN )
   {
      lsn = opLSN ;
      ossMemcpy( ( buf + offset ), &lsn, lsmLsnSz );
      offset += lsmLsnSz ;
   }
   return offset ;
}


// Extract/unpack data entry fields from a slice to the destinations
// NOTE:
//  . when bAllocBuf is true, it will ALLOCATE( SDB_THREAD_ALLOC ) memory
//    in order to decode the keyObj ( ixmKey ) and copy to the destination.
//    caller shall free( SDB_THREAD_FREE ) this memory at the proper time to
//    avoid memory leaking.
//    when bAllocBuf is false, it will NOT allocate memory, the raw
//    buffer address will be saved in pObjdata. In this case, caller shall not
//    try to free this memory.
//  . except the keyObj field, it will NOT allocate memory when copy
//    the extracted data from slice to the destination. The pointer to each
//    field passed in must be a valid address
//  . pass NULL for an uninterested field, so it will not unpack that field

#define LSM_UNPACK_CHECK_IF_EXCEED_SLICE_LEN( _len_ )   \
if ( ( aSize - offset ) < (( UINT32 )( _len_ )) )       \
{                                                       \
   return ;                                             \
}

void lsmUnpackDataKey
(
   const rocksdb::Slice & aSlice,           // slice
   const BOOLEAN          bAllocBuf,        // if allocate buffer for keyObj
   globalIndexID           * pIdxID,           // indexID
   Ordering             * pOrdering,        // ordering
   CHAR               * * pObjdata,         // keyObj raw data
   UINT32               * pObjSz,           // keyObj objsize
   recordID          * pRid,             // rowid
   UINT64               * pLSN,             // lsn
   DPS_TRANS_ID         * pTransID,         // transID
   UINT64               * pOpLSN            // log operation LSN
)
{
   const char * aStart = aSlice.data();       // address of Slice/Key a
   const UINT32 aSize  = aSlice.size();       // the length of Slice a
   UINT32       offset = 0;                   // field offset in Slice

   // EntryType
   // validate the length of slice for EntryType field
   LSM_UNPACK_CHECK_IF_EXCEED_SLICE_LEN( lsmEntryTypeSz )
   // move offset to next field
   offset += lsmEntryTypeSz;

   // IndexID
   LSM_UNPACK_CHECK_IF_EXCEED_SLICE_LEN( lsmIdxIDSz )
   if ( pIdxID )
   {
      ossMemcpy( pIdxID, (aStart + offset), lsmIdxIDSz ) ;
   }
   offset += lsmIdxIDSz;

   // Ordering
   LSM_UNPACK_CHECK_IF_EXCEED_SLICE_LEN( lsmOrdSz )
   if ( pOrdering )
   {
      ossMemcpy( pOrdering, (aStart + offset), lsmOrdSz ) ;
   }
   offset += lsmOrdSz;

   // KeyObj
   // The actual size of keyObj remains unknown before decoding.
   // Try to construct a keyObj from the slice, so can we know the exact
   // size of the keyObj
   LSM_UNPACK_CHECK_IF_EXCEED_SLICE_LEN( 2 )
   ixmKey aKeyObj( (const CHAR*)(aStart + offset) ) ;
   LSM_UNPACK_CHECK_IF_EXCEED_SLICE_LEN( aKeyObj.dataSize() )
   if ( pObjdata )
   {
      if ( bAllocBuf )
      {
         // allocate memory before copying keyObj's raw data
         *pObjdata = (CHAR*)SDB_THREAD_ALLOC(sizeof(CHAR) * aKeyObj.dataSize());
         if ( *pObjdata )
         {
            ossMemcpy( *pObjdata, aKeyObj.data(), aKeyObj.dataSize() ) ;
         }
      }
      else
      {
         *pObjdata = (CHAR*)(aStart + offset);
      }
      if ( pObjSz )
      {
         *pObjSz = aKeyObj.dataSize();
      }
   }
   offset += aKeyObj.dataSize() ;

   // RowID
   LSM_UNPACK_CHECK_IF_EXCEED_SLICE_LEN( lsmRidSz )
   if ( pRid )
   {
      ossMemcpy( pRid, (aStart + offset), lsmRidSz ) ;
   }
   offset += lsmRidSz ;

   // LSN
   LSM_UNPACK_CHECK_IF_EXCEED_SLICE_LEN( lsmLsnSz )
   if ( pLSN )
   {
      *pLSN = static_cast<UINT64>(aSlice[offset]) ;
   }
   offset += lsmLsnSz;

   // TransID
   LSM_UNPACK_CHECK_IF_EXCEED_SLICE_LEN( lsmTxIDSz )
   if ( pTransID )
   {
      ossMemcpy( pTransID, (aStart + offset), lsmTxIDSz );
   }
   offset += lsmTxIDSz ;

   // opLSN, memtable operation LSN
   LSM_UNPACK_CHECK_IF_EXCEED_SLICE_LEN( lsmLsnSz )
   if ( pOpLSN )
   {
      *pOpLSN = static_cast<UINT64>( aSlice[offset] );
   }
}


void lsmUnpackDataValue
(
   const rocksdb::Slice & aSlice,
   UINT8                * valueFlag,
   dmsRBSOffset         * rbsPos
)
{
   const CHAR * aStart = aSlice.data();
   const UINT32 aSz    = aSlice.size();
   if ( aStart && ( aSz > 0 ) )
   {
      if ( valueFlag )
      {
         *valueFlag = static_cast<UINT8>(aStart[0]) ;
      }
      if ( rbsPos )
      {
         if ( aSz >= ( lsmFlagSz + lsmRBSPosSz ) )
         {
            ossMemcpy( rbsPos, (aStart + lsmFlagSz), lsmRBSPosSz );
         }
      }
   }
   return ;
}


// unpack checkpoint entry
void lsmUnpackCKPTEntry
(
   const rocksdb::Slice & aSlice,        // slice
   UINT64       * pLSN,                  // lsn
   UINT64       * pOpLSN                 // memtable operation LSN
)
{
   const UINT32 aSize  = aSlice.size();       // the length of Slice a
   UINT32       offset = 0;                   // field offset in Slice

   // EntryType
   // validate the length of slice for EntryType field
   LSM_UNPACK_CHECK_IF_EXCEED_SLICE_LEN( lsmEntryTypeSz )
   // move offset to next field
   offset += lsmEntryTypeSz;

   // padding filed ( IndexID )
   LSM_UNPACK_CHECK_IF_EXCEED_SLICE_LEN( lsmIdxIDSz )
   offset += lsmIdxIDSz;

   // LSN
   LSM_UNPACK_CHECK_IF_EXCEED_SLICE_LEN( lsmLsnSz )
   if ( pLSN )
   {
      *pLSN = static_cast<UINT64>(aSlice[offset]) ;
   }
   offset += lsmLsnSz;

   // opLSN, memtable operation LSN
   LSM_UNPACK_CHECK_IF_EXCEED_SLICE_LEN( lsmLsnSz )
   if ( pOpLSN )
   {
      *pOpLSN = static_cast<UINT64>( aSlice[offset] );
   }
}


// validate whether a packed Slice has same IndexID
BOOLEAN lsmIsSameIdxId
(
   const rocksdb::Slice & aSlice,
   const globalIndexID & idxId
)
{
   const UINT32 aSize  = aSlice.size();       // the length of aSlice
   UINT32       offset = 0;                   // field offset in aSlice
   UINT8        myType ;

   // validate the length of slice
   if ( ( aSize - offset ) < ( lsmEntryTypeSz + lsmIdxIDSz ) )
   {
      return FALSE ;
   }

   // EntryType
   myType = static_cast<UINT8>(aSlice[offset]);
   if ( LSM_ENTRY_TYPE_DATA != myType )
   {
      return FALSE ;
   }
   // move offset to next field
   offset += lsmEntryTypeSz;

   // IndexID
   globalIndexID * pIdxId = (globalIndexID*)(aSlice.data() + offset) ;
   if ( idxId == *pIdxId )
   {
      return TRUE ;
   }
   return FALSE;
}


// validate whether a packed Slice satisfies all checking criteria.
// If a passed in pointer is NULL that field checking will be skipped
BOOLEAN lsmIsSameIndexKey
(
   const rocksdb::Slice   & aSlice,
   const ixmKey           * pKeyObj,
   const recordID      * pRid,
   const globalIndexID       * pIdxId
)
{
   const CHAR * aStart = aSlice.data();       // the address of aSlice
   const UINT32 aSize  = aSlice.size();       // the length of aSlice
   UINT32       offset = 0;                   // field offset in aSlice

   // validate minimum Slice size
   if ( aSize < ( lsmEntryTypeSz + lsmIdxIDSz ) )
   {
      return FALSE ;
   }

   // EntryType
   UINT8 myType = static_cast<UINT8>(aSlice[offset]) ;
   if ( LSM_ENTRY_TYPE_DATA != myType )
   {
      return FALSE ;
   }
   // move offset to next field
   offset += lsmEntryTypeSz;

   // IndexID
   if ( pIdxId )
   {
      globalIndexID * pMyIdxId = (globalIndexID*)(aSlice.data() + offset) ;
      if ( !( *pIdxId == *pMyIdxId ) )
      {
         return FALSE ;
      }
   }
   // move offset to next field
   offset += lsmIdxIDSz;

   // Ordering
   if ( ( aSize - offset ) < lsmOrdSz )
   {
      return FALSE ;
   }
   Ordering * pMyOrder = (Ordering*)(aStart + offset) ;
   offset += lsmOrdSz;

   // KeyObj
   UINT32 keyObjSz = 0 ;
   if ( pKeyObj || pRid )
   {
      if ( ( aSize - offset ) < 2 )
      {
         return FALSE ;
      }
      ixmKey myKeyObj( (const CHAR*)(aStart + offset) ) ;
      keyObjSz = myKeyObj.dataSize() ;
      if ( ( aSize - offset ) < keyObjSz )
      {
         return FALSE ;
      }
      if ( pKeyObj )
      {
         if ( 0 != myKeyObj.woCompare( *pKeyObj, *pMyOrder ) )
         {
            return FALSE ;
         }
      }
   }
   // move offset to next field
   offset += keyObjSz ;

   // Rid
   if ( pRid )
   {
      if ( ( aSize - offset ) < lsmRidSz )
      {
         return FALSE ;
      }
      recordID *pMyRid = (recordID*)(aStart + offset) ;
      if ( !( *pRid == *pMyRid ) )
      {
         return FALSE;
      }
   }
   return TRUE ;
}


void lsmUpdateDataEntryToMostAdjacent( rocksdb::Slice a, INT32 direction )
{
   const CHAR * aStart = a.data(); // the address of aSlice
   const UINT32 aSize  = a.size(); // the length of aSlice
   UINT32       offset ;           // field offset in aSlice

   offset = lsmEntryTypeSz + lsmIdxIDSz + lsmOrdSz ;
   if ( aSize > ( offset + 2 ) )
   {
      ixmKey myKeyObj( (const CHAR*)(aStart + offset) ) ;
      offset += myKeyObj.dataSize() ;
      if ( aSize >= ( offset + lsmRidSz + lsmLsnSz ) )
      {
         vessel::recordID *pRid = (vessel::recordID*)(aStart + offset);
         UINT64 * aLsn = (UINT64*)(aStart + offset + lsmRidSz );
         PAGE_ID        pageId = pRid->getPageID();
         RECORD_SLOT_ID slotId = pRid->getSlotID();
         // key sorted on LSN field in descending order
         if ( direction > 0 )
         {
            if ( *aLsn > 0 )
            {
               (*aLsn) -= 1;
            }
            else
            {
               slotId++;
               pRid->setSlotID( slotId ) ;
            }
         }
         else
         {
            if ( *aLsn != ((UINT64)(-1)) )
            {
               (*aLsn) += 1;
            }
            else
            {
               if ( slotId > 0 )
               {
                  slotId--;
                  pRid->setSlotID( slotId ) ;
               }
               else if ( pageId > 0 )
               {
                  pageId--;
                  pRid->setPageID( pageId ) ;
               }
            }
         }
      }
   }
}

INT32 lsmKeyEntry::shallowCopy(const rocksdb::Slice &fullEntry)
{
   INT32 rc = SDB_OK;
   globalIndexID indexId;
   orderingWrapper ordering;

   if (fullEntry.size() < LSM_MIN_FULL_KEY_SIZE)
   {
      rc = SDB_INVALIDARG;
      goto error;
   }

   rc = lsmUnpackIndexFullKey(fullEntry.data(),
                              fullEntry.size(),
                              indexId,
                              ordering,
                              _key, _rid, _dataLsn, _transID);
   if (SDB_OK != rc)
   {
      PD_LOG(PDERROR, "failed to unpack full key:%d", rc);
      goto error;
   }
done:
   return rc;
error:
   reset();
   goto done;
}
} // namespace vessel
} // namespace engine
