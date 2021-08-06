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

   Source File Name = lsmIndex.hpp

   Descriptive Name = LSM Index APIs, vessel::Index implementation

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          02/12/2021  JT  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef LSMINDEX_HPP_
#define LSMINDEX_HPP_

#include "vessel/lsm/lsmIdxKey.hpp"
#include "vessel/lsm/lsmIndexMeta.hpp"
#include "vessel/lsm/lsmDB.hpp"
#include "rtnPredicate.hpp"     // VEC_ELE_CMP
#include "rocksdb/rocksdb_namespace.h"

using namespace bson ;
using namespace rocksdb ;

namespace engine
{
namespace vessel
{
/*
   SDB LSM Tree Index is inspired by RocksDB key-value storage.
   It serves as a LSM Tree type index manager with MVCC support.

   As implemented with a K-V store, each SDB LSM entry( K-V pair ) may serve
   for different purpose, so it requires specific encoding regarding its type.
   There are two types of entry in current desgin:

   1. Normal index entry, EntryType = LSM_ENTRY_TYPE_DATA
         Key:   EntryType_IndexID_Ordering_EncodedKey_RowID_LSN_TransID
         Value: NULL           --  a normal data entry
                 -or-
                Flag_RBSOffset --  a data entry is marked as deleted

   2. Checkpoint entry,  EntryType = LSM_ENTRY_TYPE_CKPT
         Key:   EntryType_IndexID_LSN
         Value: NULL
      Note: the IndexID for checkpoing entry is just padding(filled with all
            0xFF), try to make the two types of Key with same length prefix

   Here
   * EntryType:   UINT8, entry type,
                     0x0 index key
                     0x1 checkpoint
   * '_':         nothing, is just for readability
   * IndexID:     sdbIndexID, unique index id, { csID, clID, indexLLID }
   * Ordering:    Ordering, key object ordering information
   * EncodedKey:  ixmKey, key object, the raw data stream
   * RowID:       vessel::recordID, record id, rid
   * LSN:         UINT64, corresponding data LSN, sorted in descending order
   * TransID:     SDB global transaction number, sorted in descending order

   * Flag:        UINT8, entry flag,
                     0x0 normal
                     0x1 marked as deleted
   * RBSOffset:   offset/position in roll back segment ( RBS )
*/


class lsmIndex: public SDBObject
{
public:
  lsmIndex() {  _initalized = FALSE ; _it = NULL ; _reUseIter = FALSE; }
  virtual ~lsmIndex() { }

  // No copying allowed
  lsmIndex( const lsmIndex & ) = delete;
  void operator= ( const lsmIndex & ) = delete;

  // initialization
  INT32 init( LSMDB * lsmdb, const indexMeta & idxMeta ) ;
  BOOLEAN isInitialized() const { return _initalized; }

  /*
    locate an index key entry stored in rocksdb that matches the passed
    in parameter searchKey( ixmKey ) regarding the searching direction.
    Input:
      direction: searching direction, 1 forward, -1 backward
      keyObj:    ixmKey, searching key obj
      rid:       vessel::recordID, recordID ( a.k.a row id )
    Output:
      keyEntry:  lsmKeyEntry, the actuall key and value stored in rocksdb.
      pFound:    whether find the exact match the searching key
    Return:
      SDB_OK: normal return
      otherwise any popped error code

    PLEASE NOTE :
      There could be multiple key entries stored in rocksdb match the searching
      criteria, i.e., a key entry having same < keyObj, rid >. This is different
      comparing BTree index entry, while < keyObj, rid > can uniquely identify
      a key entry.  As we known a LSM key entry stored in rocksdb is packed as
      following:
         EntryType_IndexID_Ordering_EncodedKey_RowID_LSN_TransID
      When an index key having same < keyObj, rid > was deleted and inserted
      multiple times, so exist multiple entires having same < keyObj, rid > in
      rocksdb. Therefore, the returned key has following different sematics:
      . when direction > 0(forward scan), the key entry returned is the newest
        one( or the smallest one from rocksb perspective ), i.e., it has
        same < keyObj, rid > with the largest LSN, because a key entry
        internally sorted on LSN field in descending order.
      . when direction < 0(backward scan), the key entry returned is the oldest
        one( or the largest one from rocksb perspective ), i.e., the key has
        same key< keyObj, rid > with the smallest LSN, because a key entry
        internally sorted on LSN field in descending order.

      For example, we have following index entries saved in rocksdb.
      Symbol K21 is first key entry of index 2, K36 is 6th key entry of index 3.
      They( K21, K36, and etc on ) are just symbols for easy notation, these
      symbols are not stored in rocksdb.

      example1:
      locate: index={1,2,3}, keyObj={1,'t9',1}, rid={pg:1,slt:1}, dir=-1
      return: K33, i.e., 0_{1,2,3}_{1,"t9",1}_{pg:1,slt:1}_1_{sn:1,nd:1}

      example2:
      locate: index={1,2,3}, keyObj={1,'t9',1}, rid={pg:1,slt:1}, dir=1
      return: K31, i.e., 0_{1,2,3}_{1,"t9",1}_{pg:1,slt:1}_3_{sn:1,nd:3}

      K     IndexID           KeyObj       RowID      LSN TransID
      K21 0_{cs:1,cl:1,idx:2}_{1,"t9",  1}_{pg:1,slt:1}_2_{sn:1,nd:2}
      K22 0_{cs:1,cl:1,idx:2}_{1,"t9",  1}_{pg:1,slt:1}_1_{sn:1,nd:1}
      K23 0_{cs:1,cl:1,idx:2}_{1,"test",1}_{pg:1,slt:1}_2_{sn:1,nd:2}
      K24 0_{cs:1,cl:1,idx:2}_{1,"test",1}_{pg:1,slt:1}_1_{sn:1,nd:1}
      K31 0_{cs:1,cl:2,idx:3}_{1,"t9",  1}_{pg:1,slt:1}_3_{sn:1,nd:3}
      K32 0_{cs:1,cl:2,idx:3}_{1,"t9",  1}_{pg:1,slt:1}_2_{sn:1,nd:2}
      K33 0_{cs:1,cl:2,idx:3}_{1,"t9",  1}_{pg:1,slt:1}_1_{sn:1,nd:1}
      K34 0_{cs:1,cl:2,idx:3}_{1,"test",1}_{pg:1,slt:1}_3_{sn:1,nd:3}
      K35 0_{cs:1,cl:2,idx:3}_{1,"test",1}_{pg:1,slt:1}_2_{sn:1,nd:2}
      K36 0_{cs:1,cl:2,idx:3}_{1,"test",1}_{pg:1,slt:1}_1_{sn:1,nd:1}
      K37 0_{cs:1,cl:2,idx:3}_{2,"t0",  1}_{pg:1,slt:2}_3_{sn:1,nd:3}
      K38 0_{cs:1,cl:2,idx:3}_{2,"test",1}_{pg:1,slt:2}_3_{sn:1,nd:3}
      K39 0_{cs:1,cl:2,idx:3}_{4,"t0",  1}_{pg:2,slt:1}_3_{sn:1,nd:3}
      K3A 0_{cs:1,cl:2,idx:3}_{5,"t0",  1}_{pg:2,slt:1}_3_{sn:1,nd:3}
      K3B 0_{cs:1,cl:2,idx:3}_{5,"t0",  2}_{pg:1,slt:1}_3_{sn:1,nd:3}
      K3C 0_{cs:1,cl:2,idx:3}_{5,"t1",  1}_{pg:2,slt:1}_3_{sn:1,nd:3}
  */
  INT32 locate( const INT32              direction,
                const ixmKey           & keyObj,
                const vessel::recordID & rid,
                lsmKeyEntry            & keyEntry,
                BOOLEAN                * pFound ) ;

  /*
     advance to next index key entry that matches the passed in parameter
     searchKey regarding the searching direction.
     Input:
       direction: searching direction, 1 forward, -1 backward
       searchKey: lsmKeyEntry, searching key entry
     Output:
       keyEntry:  lsmKeyEntry, the actuall key entry stored in rocksdb.
     Return:
       SDB_OK: normal return
       otherwise any popped error code

     PLEASE NOTE :
     There could be multiple key entries stored in rocksdb match the searching
     criteria, f.g., a key entry having same < keyObj, rid >.  As we known a key
     entry stored in rocksdb is packed as following:
        EntryType_IndexID_Ordering_EncodedKey_RowID_LSN_TransID
     When an index key having same < keyObj,rid > was deleted and inserted
     multiple times, so exist multiple entires having same < keyObj, rid > in
     rocksdb. The locate function is be used to after locate an index entry
     statisfies the "keyObj + rid" searching criteria, and get the next/previous
     index entry of a specific index entry( the one returned from locate
     function ).

     For example, we have following index entries saved in rocksdb
     K21 is first key entry of index 2, K36 is 6th key entry of index 3.

     example1:
     advance: dir=-1,K33,i.e., 0_{1,2,3}_{1,"t9",1}_{pg:1,slt:1}_1_{sn:1,nd:1}
     return: K32, i.e., 0_{1,2,3}_{1,"t9",1}_{pg:1,slt:1}_2_{sn:1,nd:2}

     example2:
     advance: dir=1,K31,i.e., 0_{1,2,3}_{1,'t9',1}_{pg:1,slt:1}_3_{sn:1,nd:3}
     return: K33, i.e., 0_{1,2,3}_{1,"t9",1}_{pg:1,slt:1}_2_{sn:1,nd:2}

     K     IndexID           KeyObj       RowID      LSN TransID
     K21 0_{cs:1,cl:1,idx:2}_{1,"t9",  1}_{pg:1,slt:1}_2_{sn:1,nd:2}
     K22 0_{cs:1,cl:1,idx:2}_{1,"t9",  1}_{pg:1,slt:1}_1_{sn:1,nd:1}
     K23 0_{cs:1,cl:1,idx:2}_{1,"test",1}_{pg:1,slt:1}_2_{sn:1,nd:2}
     K24 0_{cs:1,cl:1,idx:2}_{1,"test",1}_{pg:1,slt:1}_1_{sn:1,nd:1}
     K31 0_{cs:1,cl:2,idx:3}_{1,"t9",  1}_{pg:1,slt:1}_3_{sn:1,nd:3}
     K32 0_{cs:1,cl:2,idx:3}_{1,"t9",  1}_{pg:1,slt:1}_2_{sn:1,nd:2}
     K33 0_{cs:1,cl:2,idx:3}_{1,"t9",  1}_{pg:1,slt:1}_1_{sn:1,nd:1}
     K34 0_{cs:1,cl:2,idx:3}_{1,"test",1}_{pg:1,slt:1}_3_{sn:1,nd:3}
     K35 0_{cs:1,cl:2,idx:3}_{1,"test",1}_{pg:1,slt:1}_2_{sn:1,nd:2}
     K36 0_{cs:1,cl:2,idx:3}_{1,"test",1}_{pg:1,slt:1}_1_{sn:1,nd:1}
     K37 0_{cs:1,cl:2,idx:3}_{2,"t0",  1}_{pg:1,slt:2}_3_{sn:1,nd:3}
     K38 0_{cs:1,cl:2,idx:3}_{2,"test",1}_{pg:1,slt:2}_3_{sn:1,nd:3}
     K39 0_{cs:1,cl:2,idx:3}_{4,"t0",  1}_{pg:2,slt:1}_3_{sn:1,nd:3}
     K3A 0_{cs:1,cl:2,idx:3}_{5,"t0",  1}_{pg:2,slt:1}_3_{sn:1,nd:3}
     K3B 0_{cs:1,cl:2,idx:3}_{5,"t0",  2}_{pg:1,slt:1}_3_{sn:1,nd:3}
     K3C 0_{cs:1,cl:2,idx:3}_{5,"t1",  1}_{pg:2,slt:1}_3_{sn:1,nd:3}
  */
  INT32 advance( const INT32        direction,
                 const lsmKeyEntry & searchKey,
                 lsmKeyEntry       & keyEntry ) ;


  /*
     find the smallest/greatest key entry matches the pushed down verb and
     searching keyObj, prevKey, regarding the scan direction.
     Input:
       direction: searching direction, 1 forward, -1 backward
       prevKey:   ixmKey, previous index key obj to start with the searching
       order:     key order information
       keepFieldsNum and skipToNext:
         keepFieldsNum is the number of fields from prevKey that should match
         the currentKey (for example if the prevKey is {c1:1, c2:1}, and
         keepFieldsNum = 1, that means we want to match c1:1 key for the
         current location. Depends on if we have skipToNext set, if we do
         that it means we want to skip c1:1 and match whatever the next
         (for example c1:1.1); otherwise we want to continue match the
         elements from matchElement
       matchElement and matchInclusive:
         push down matching criteria from access plan
     Output:
       keyEntry: the actuall key entry and value stored in rocksdb.
     Return:
       SDB_OK: normal return
       otherwise any popped error code

     For example, we have following index entries saved in rocksdb
     K21 is first key entry of index 2, K36 is 6th key entry of index 3.

     example1:
     keyLocate: index={1,2,3}, keyObj={1,'t9',1}, dir=1, numFields=3, skip=0
     return: K31 (i.e., 0_{1,2,3}_{1,"t9",1}_{pg:1,slt:1}_3_{sn:1,nd:3}

     example2:
     keyLocate: index={1,2,3}, keyObj={1,'t9',1}, dir:-1, numFields:3, skip=0
     return: K33, i.e., 0_{1,2,3}_{1,"t9",1}_{pg:1,slt:1}_1_{sn:1,nd:1}

     example3:
     keyLocate: index={1,2,3}, keyObj={1,'t9',2}, dir:1, numFields:3, skip:0
     return: K34, i.e., 0_{1,2,3}_{1,"test",1}_{pg:1,slt:1}_3_{sn:1,nd:3}

     K     IndexID           KeyObj       RowID      LSN TransID
     K21 0_{cs:1,cl:1,idx:2}_{1,"t9",  1}_{pg:1,slt:1}_2_{sn:1,nd:2}
     K22 0_{cs:1,cl:1,idx:2}_{1,"t9",  1}_{pg:1,slt:1}_1_{sn:1,nd:1}
     K23 0_{cs:1,cl:1,idx:2}_{1,"test",1}_{pg:1,slt:1}_2_{sn:1,nd:2}
     K24 0_{cs:1,cl:1,idx:2}_{1,"test",1}_{pg:1,slt:1}_1_{sn:1,nd:1}
     K31 0_{cs:1,cl:2,idx:3}_{1,"t9",  1}_{pg:1,slt:1}_3_{sn:1,nd:3}
     K32 0_{cs:1,cl:2,idx:3}_{1,"t9",  1}_{pg:1,slt:1}_2_{sn:1,nd:2}
     K33 0_{cs:1,cl:2,idx:3}_{1,"t9",  1}_{pg:1,slt:1}_1_{sn:1,nd:1}
     K34 0_{cs:1,cl:2,idx:3}_{1,"test",1}_{pg:1,slt:1}_3_{sn:1,nd:3}
     K35 0_{cs:1,cl:2,idx:3}_{1,"test",1}_{pg:1,slt:1}_2_{sn:1,nd:2}
     K36 0_{cs:1,cl:2,idx:3}_{1,"test",1}_{pg:1,slt:1}_1_{sn:1,nd:1}
     K37 0_{cs:1,cl:2,idx:3}_{2,"t0",  1}_{pg:1,slt:2}_3_{sn:1,nd:3}
     K38 0_{cs:1,cl:2,idx:3}_{2,"test",1}_{pg:1,slt:2}_3_{sn:1,nd:3}
     K39 0_{cs:1,cl:2,idx:3}_{4,"t0",  1}_{pg:2,slt:1}_3_{sn:1,nd:3}
     K3A 0_{cs:1,cl:2,idx:3}_{5,"t0",  1}_{pg:2,slt:1}_3_{sn:1,nd:3}
     K3B 0_{cs:1,cl:2,idx:3}_{5,"t0",  2}_{pg:1,slt:1}_3_{sn:1,nd:3}
     K3C 0_{cs:1,cl:2,idx:3}_{5,"t1",  1}_{pg:2,slt:1}_3_{sn:1,nd:3}
  */
  INT32 keyLocate( const INT32              direction,
                   const ixmKey           & prevKey,
                   const INT32              keepFieldsNum,
                   const BOOLEAN            skipToNext,
                   const VEC_ELE_CMP      & matchElement,
                   const VEC_BOOLEAN      & matchInclusive,
                   lsmKeyEntry            & keyEntry );

  /*
     advance to the next smallest/greatest key entry matches the pushed down
     verb and searching keyObj, prevKey, regarding the scan direction.
     Input:
       direction: searching direction, 1 forward, -1 backward
       prevKey: previous index key obj to start with the searching
       order: key order information
       keepFieldsNum and skipToNext:
         keepFieldsNum is the number of fields from prevKey that should match
         the currentKey (for example if the prevKey is {c1:1, c2:1}, and
         keepFieldsNum = 1, that means we want to match c1:1 key for the
         current location. Depends on if we have skipToNext set, if we do
         that it means we want to skip c1:1 and match whatever the next
         (for example c1:1.1); otherwise we want to continue match the
         elements from matchElement
       matchElement and matchInclusive:
         push down matching criteria from access plan
     Output:
       keyEntry: the actuall key entry and value stored in rocksdb.
       Return:
        SDB_OK: normal return
        otherwise any popped error code

     For example, we have following index entries saved in rocksdb
     K21 is first key entry of index 2, K36 is 6th key entry of index 3.

     example1:
     keyAdvance: index={1,2,3}, keyObj={1,'t9',2}, dir=-1, numFields=3, skip=0
     return: K33, i.e., 0_{1,2,3}_{1,"t9",1}_{pg:1,slt:1}_1_{sn:1,nd:1}

     example2:
     keyAdvance, index={1,2,3}, keyObj={1,'t9',2}, dir=1, numFields=3, skip=0
     return: K34, i.e., 0_{1,2,3}_{1,"test",1}_{pg:1,slt:1}_3_{sn:1,nd:3}

     example3:
     keyAdvance, index={1,2,3}, keyObj={5,'t0'}, dir=1, numFields=2, skip=1
     return: K3C, i.e., 0_{1,2,3}_{5,"t1",1}_{pg:2,slt:1}_3_{sn:1,nd:3}

     example4:
     keyAdvance, index={1,2,3}, keyObj={1,'t9',1}, dir:-1, numFields=3, skip=0
     return, No key found

     example5:
     keyAdvance, index={1,2,3}, keyObj={1,'t9'}, dir=1, numFields=2, skip=1
     return, K34, i.e., 0_{1,2,3}_{1,"test",1}_{pg:1,slt:1}_3_{sn:1,nd:3}

     K     IndexID           KeyObj       RowID      LSN TransID
     K21 0_{cs:1,cl:1,idx:2}_{1,"t9",  1}_{pg:1,slt:1}_2_{sn:1,nd:2}
     K22 0_{cs:1,cl:1,idx:2}_{1,"t9",  1}_{pg:1,slt:1}_1_{sn:1,nd:1}
     K23 0_{cs:1,cl:1,idx:2}_{1,"test",1}_{pg:1,slt:1}_2_{sn:1,nd:2}
     K24 0_{cs:1,cl:1,idx:2}_{1,"test",1}_{pg:1,slt:1}_1_{sn:1,nd:1}
     K31 0_{cs:1,cl:2,idx:3}_{1,"t9",  1}_{pg:1,slt:1}_3_{sn:1,nd:3}
     K32 0_{cs:1,cl:2,idx:3}_{1,"t9",  1}_{pg:1,slt:1}_2_{sn:1,nd:2}
     K33 0_{cs:1,cl:2,idx:3}_{1,"t9",  1}_{pg:1,slt:1}_1_{sn:1,nd:1}
     K34 0_{cs:1,cl:2,idx:3}_{1,"test",1}_{pg:1,slt:1}_3_{sn:1,nd:3}
     K35 0_{cs:1,cl:2,idx:3}_{1,"test",1}_{pg:1,slt:1}_2_{sn:1,nd:2}
     K36 0_{cs:1,cl:2,idx:3}_{1,"test",1}_{pg:1,slt:1}_1_{sn:1,nd:1}
     K37 0_{cs:1,cl:2,idx:3}_{2,"t0",  1}_{pg:1,slt:2}_3_{sn:1,nd:3}
     K38 0_{cs:1,cl:2,idx:3}_{2,"test",1}_{pg:1,slt:2}_3_{sn:1,nd:3}
     K39 0_{cs:1,cl:2,idx:3}_{4,"t0",  1}_{pg:2,slt:1}_3_{sn:1,nd:3}
     K3A 0_{cs:1,cl:2,idx:3}_{5,"t0",  1}_{pg:2,slt:1}_3_{sn:1,nd:3}
     K3B 0_{cs:1,cl:2,idx:3}_{5,"t0",  2}_{pg:1,slt:1}_3_{sn:1,nd:3}
     K3C 0_{cs:1,cl:2,idx:3}_{5,"t1",  1}_{pg:2,slt:1}_3_{sn:1,nd:3}
  */
  INT32 keyAdvance( const INT32              direction,
                    const ixmKey           & prevKey,
                    const INT32              keepFieldsNum,
                    const BOOLEAN            skipToNext,
                    const VEC_ELE_CMP      & matchElement,
                    const VEC_BOOLEAN      & matchInclusive,
                    lsmKeyEntry            & keyEntry ) ;
  /*
     insert a key entry into rocksdb
     Note:
       No old version will be saved for insert operation.

     For LSM( rocksdb ) index entry, it is packed with following format :
        Key:   EntryType_IndexID_Ordering_EncodedKey_RowID_DataLSN_TransID
        Value: NULL           -- for new entry
               Flag_RBSOffset -- for entry marked as deleted
     Here
      * EntryType:   UINT8, entry type, LSM_ENTRY_TYPE_DATA = 0x0, index key
      * '_':         nothing, is just for readability
      * IndexID:     sdbIndexID, unique index id, { csID, clID, indexLLID }
      * Ordering:    Ordering, key object( BSON object ) ordering
      * EncodedKey:  ixmKey, key object, the raw data of a ixmKey
      * RowID:       vessel::recordID, record id
      * DataLSN:     UINT64, corresponding data LSN, sorted in descending order
      * TransID:     SDB global transaction number, sorted in descending order

     Input:
       keyEntry: the key entry and value to be inserted into rocksdb
       logLSN:   the LSN for this insert operation
       pBatch:   pointer to rocksdb::WriteBatch, if inserting is going to
                 be done via a writeBatch. It is the caller to decide to
                 write/commit the batch or abort the operations.
                 If this pointer is NULL, the inserting will be treated
                 as a single operation.
     Return:
       SDB_OK: normal return
       otherwise any popped error code
  */
  INT32 keyInsert( const lsmKeyEntry    & keyEntry,
                   const UINT64           logLSN,
                   rocksdb::WriteBatch  * pBatch = NULL ) ;


  /*
     Mark a key entry as deleted and save the old version

     For lsm( rocksdb ) index
         the delete operation would be simply 'insert'/save a record for
         the old version index key; the original index key would be remain
         as it was.
         old version index key:
           K: EntryType_IndexID_Ordering_EncodedKey_RowID_NewDataLSN_NewTransID
           V: Flag_RBSOffset
     For btree index
         the index item would removed from btree directly and 'insert'/save
         rocksdb with two entries, one is old version index key and the other
         one is origin index key.
         old version index key:
           K: EntryType_IndexID_Ordering_EncodedKey_RowID_NewDataLSN_NewTxID
           V: Flag_RBSOffset
         original index key:
           K: EntryType_IndexID_Ordering_EncodedKey_RowID_OrigDataLSN_OrigTxID
           V: NULL
     Input:
       blsmIdx:       whether delete a rocksdb key entry
       origKeyEntry:  original key entry
       keyEntry:      the key entry( old version ) to write into rocksdb to
                      mark the original key as deleted
       pBatch:        pointer to rocksdb::WriteBatch, if deleting is going to
                      be done via a writeBatch. It is the caller to decide to
                      write/commit the batch or abort the operations.
                      If this pointer is NULL, the deleting will be processed
                      as a single operation.
     Return:
       SDB_OK: normal return
       otherwise any popped error code
  */
   INT32 keyDelete( const BOOLEAN            blsmIdx,
                    const lsmKeyEntry      & origKeyEntry,
                    const lsmKeyEntry      & keyEntry,
                    const UINT64             logLSN,
                    rocksdb::WriteBatch    * pBatch = NULL ) ;

  /*
     delete an index entry without saving its old version
     Input:
       keyEntry: the key entry to be removed
       logLSN:   the LSN for this delete operation
       pBatch:   pointer to rocksdb::WriteBatch, if deleting is going to
                 be done via a writeBatch. It is the caller to decide to
                 write/commit the batch or abort the operations.
                 If this pointer is NULL, the deleting will be treated
                 as a single operation.
     Return:
       SDB_OK: normal return
       otherwise any popped error code
  */
  INT32 keyRemove( const lsmKeyEntry   & keyEntry,
                   const UINT64          logLSN,
                   rocksdb::WriteBatch * pBatch = NULL );

  /*
     truncate an index
     Input:
       logLSN:  the LSN for this operation
     Return:
       SDB_OK: normal return
       otherwise any popped error code
  */
  INT32 truncateIndex( UINT64 logLSN ) ;

  /*
     drop an index
     Input:
       pReqCtx: pointer to vessel::requestContext
       logLSN:  the LSN for this operation
     Return:
       SDB_OK: normal return
       otherwise any popped error code
  */
  INT32 dropIndex( UINT64 logLSN ) ;

protected:
  BSONObj _buildKeyObj( const BSONObj &prevKey,
                        INT32 keepFieldsNum,
                        BOOLEAN skipToNext,
                        const VEC_ELE_CMP &matchEle,
                        const VEC_BOOLEAN &matchInclusive,
                        INT32 direction ) const ;

  // free the rocksdb::Slice buffer, and clear that Slice
  void _freeAndClear( rocksdb::Slice & aSlice );

  // allocate and copy the content into a rocksdb::Slice
  INT32 _allocAndCopy( const CHAR * keyAddr,
                      const UINT32 keySize,
                      rocksdb::Slice & K ) ;

  // allocate buffer to construct a rocksdb::Slice,
  // and copy the content of a keyEntry into that buffer
  INT32 _allocAndCopy( const BOOLEAN       bPackLogLSN,
                       const UINT64        logLSN,
                       const lsmKeyEntry & keyEntry,
                       rocksdb::Slice    & K ) ;

  INT32 _advance( const INT32 direction, lsmKeyEntry & keyEntry ) ;

  // locate to or advance to the next of the smallest/greatest key entry matches
  // the pushed down verb and searching key object, prevKey, regarding the scan
  // direction.
  INT32 _keySearch( const BOOLEAN            bNextOnly,
                    const INT32              direction,
                    const ixmKey           & prevKey,
                    const INT32              keepFieldsNum,
                    const BOOLEAN            skipToNext,
                    const VEC_ELE_CMP      & matchElement,
                    const VEC_BOOLEAN      & matchInclusive,
                    lsmKeyEntry            & keyEntry ) ;

  // destory/close an iterator
  void _closeIter()
  {
     if ( _it )
     {
        delete _it ;
        _it = NULL ;
     }
  }

  // create an iterator
  void _openIter()
  {
     if ( NULL == _it )
     {
        _it = _lsmdb->NewIterator( _rOpt, LSM_CF_INDEX );
        SDB_ASSERT( ( NULL != _it ), "Iterator hasn't been opened !" );
     }
  }

private:
  // an internal helper function, locate an index entry matches the passed in
  // parameter, searchKey( rocksdb::Slice ), regarding the searching direction.
  INT32 _locate( const INT32              direction,
                 const rocksdb::Slice   & searchKey,
                 lsmKeyEntry            & keyEntry ) ;

  // insert a key entry packed with following format into rocksdb:
  //   Key:   EntryType_IndexID_Ordering_EncodedKey_RowID_DataLSN_TransID
  //   Value: NULL or Flag_RBSOffset
  INT32 _keyInsert( const rocksdb::Slice & key,
                    const rocksdb::Slice & value,
                    rocksdb::WriteBatch  * pBatch );

  // mark a key entry as deleted and save the old version
  // blsmIdx : whether delete a rocksdb index entry
  INT32 _keyDelete( const BOOLEAN blsmIdx,
                    const rocksdb::Slice & origKey,
                    const rocksdb::Slice & key,
                    const rocksdb::Slice & value,
                    rocksdb::WriteBatch  * pBatch ) ;

  // delete an entry from rocksdb without saving its old version
  INT32 _keyRemove( const rocksdb::Slice & key,
                    rocksdb::WriteBatch  * pBatch ) ;

protected:
   LSMDB *          _lsmdb ;
   indexMeta        _idxMeta ;
   sdbIndexID       _idxId ;
   const Ordering * _pOrdering;
   CHAR             _uBuf[ lsmMinDataKeySz ] ;
   CHAR             _lBuf[ lsmMinDataKeySz ] ;
   rocksdb::Slice   _uKey;
   rocksdb::Slice   _lKey;
   rocksdb::ReadOptions _rOpt;
   BOOLEAN              _initalized = FALSE ;
   rocksdb::Iterator *  _it = NULL ;
   BOOLEAN              _reUseIter  = FALSE ;  // whether iterator can be reused
};


/*
   LSM ( rocksdb ) index read-only operation API for isolation Repeatable Read

   Rocksdb iterator will read from an implicit snapshot as of the time the
   iterator is created, so it will not see the latest update. It works well
   for read-only operation when isolation mode is set to Repetable-Read,
   but it may cause 'lost update' when scan for non-readonly operation.
   However, an iterator has some creation costs, in some cases( especially,
   read-only operation with RR isolation mode ), we want to avoid the creation
   costs of iterators by reusing iterators. When we are doing it, be aware that
   in case an iterator getting stale, it can block resource from being released.
   So make sure destroy or refresh them if they are not used after some time,
   for example, when pauseScan / resumeScan.
   The lsmIdxRepeatableReadOnly class is created solely for read-only operation
   with RR isolation mode. It reuses the iterator to avoid creation costs.
   It also provides closeCursor() and openCursor() methods to destroy or refresh
   the iterator.
*/
class lsmIdxRepeatableReadOnly : public lsmIndex
{
public:
   lsmIdxRepeatableReadOnly() : lsmIndex() { _reUseIter = TRUE; }

   virtual ~lsmIdxRepeatableReadOnly() { closeCursor(); }

   // No copying allowed
   lsmIdxRepeatableReadOnly( const lsmIdxRepeatableReadOnly & ) = delete;
   void operator= ( const lsmIdxRepeatableReadOnly & ) = delete;

   // create iterator after initialization
   INT32 init( LSMDB * lsmdb, const indexMeta & idxMeta )
   {
      INT32 rc = lsmIndex::init( lsmdb, idxMeta );
      if ( SDB_OK == rc )
      {
         openCursor();
      }
      else
      {
         _initalized = FALSE ;
      }
      return rc ;
   }

   // destory iterator
   OSS_INLINE void closeCursor()
   {
      _closeIter() ;
   }

   // create iterator
   OSS_INLINE void openCursor()
   {
      SDB_ASSERT( ( _initalized ),
                  "LSM Index for repetable read-only is not initialized !" );
      _openIter() ;
   }

   // reuse iterator instead of destroy / create each time
   INT32 advance( const INT32         direction,
                  const lsmKeyEntry & searchKey,
                  lsmKeyEntry       & keyEntry ) ;

   // this class is designed solely for read-only with isolation RR mode
   // so disable all update operations.
   INT32 keyInsert( const lsmKeyEntry & keyEntry, const UINT64 logLSN )
   {
      // FIX ME:
      // Create a proper error code
      return SDB_OPTION_NOT_SUPPORT ;
   }

   INT32 keyDelete( const BOOLEAN            blsmIdx,
                    const lsmKeyEntry      & origKeyEntry,
                    const lsmKeyEntry      & keyEntry,
                    const UINT64             logLSN )
   {
      // FIX ME:
      // Create a proper error code
      return SDB_OPTION_NOT_SUPPORT ;
   }

   INT32 keyRemove( const lsmKeyEntry & keyEntry, const UINT64 logLSN )
   {
      // FIX ME:
      // Create a proper error code
      return SDB_OPTION_NOT_SUPPORT ;
   }

   INT32 truncateIndex( UINT64 logLSN )
   {
      // FIX ME:
      // Create a proper error code
      return SDB_OPTION_NOT_SUPPORT ;
   }

   INT32 dropIndex( UINT64 logLSN )
   {
      // FIX ME:
      // Create a proper error code
      return SDB_OPTION_NOT_SUPPORT ;
   }
};


} //namespace vessel
} // namespace engine
#endif  // LSMINDEX_HPP_
