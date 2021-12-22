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

   Source File Name = lsmIndex.cpp

   Descriptive Name = LSM Index API

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          02/12/2021  JT  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/lsm/lsmIndex.hpp"
#include "ixmExtent.hpp"   // _keyCmp
#include "rocksdb/rocksdb_namespace.h"

using namespace bson ;
using namespace rocksdb ;
using namespace std ;

namespace engine
{
namespace vessel
{

// initialization
INT32 lsmIndex::init( LSMDB * lsmdb, const lsmIndexMeta & idxMeta )
{
   INT32 rc = SDB_OK;

   _initalized = FALSE;

   SDB_ASSERT( ( NULL != lsmdb ),   "Invalid argument" ) ;
   SDB_ASSERT( (idxMeta.isValid()), "Invalid index metadata" ) ;
   if ( ( NULL == lsmdb ) || ( ! idxMeta.isValid() ) )
   {
      return ( rc = SDB_INVALIDARG );
   }
   _lsmdb     = lsmdb ;
   _idxMeta   = idxMeta ;

   // construct upper_bound, lower_bound key and ReadOptions
   globalIndexID upIdxId(_idxMeta.getIdxId().getLogicalCSID(),
                         _idxMeta.getIdxId().getLogicalCSID(),
                         _idxMeta.getIdxId().getLogicalIndexID() + 1);

   recordID dummyRid(0,0);
   UINT64 dummyLsn = ((UINT64)(-1));
   DPS_TRANS_ID dummyTxID;
   BSONObj dummyObj ;
   ixmKeyOwned dummyKey( dummyObj );

   rc = lsmPackIndexFullKey(_uBuf, lsmMinDataKeySz,
                            upIdxId, _idxMeta.getOrdering(),
                            dummyKey, dummyRid,
                            dummyLsn, dummyTxID);
   if (SDB_OK != rc)
   {
      PD_LOG(PDERROR, "failed to build ukey:%d", rc);
      goto error;
   }

   rc = lsmPackIndexFullKey(_lBuf, lsmMinDataKeySz,
                            _idxMeta.getIdxId(),
                            _idxMeta.getOrdering(),
                            dummyKey, dummyRid,
                            dummyLsn, dummyTxID);
   if (SDB_OK != rc)
   {
      PD_LOG(PDERROR, "failed to build lkey:%d", rc);
      goto error;
   }

   _uKey = rocksdb::Slice( _uBuf, lsmMinDataKeySz ) ;
   _lKey = rocksdb::Slice( _lBuf, lsmMinDataKeySz ) ;

   // set ReadOptions
   _rOpt = _lsmdb->getReadOpt();
   _rOpt.iterate_lower_bound  = &_lKey;
   _rOpt.iterate_upper_bound  = &_uKey;
   _rOpt.auto_prefix_mode     = true;

   _initalized                = TRUE;

done:
   return rc ;
error:
   fini();
   goto done;
}

void lsmIndex::fini()
{
   _lsmdb = NULL;
   _idxMeta = lsmIndexMeta();
   _uKey.clear();
   _lKey.clear();
   _rOpt = rocksdb::ReadOptions();
   _initalized = FALSE;
   _it = NULL;
   _reUseIter = FALSE;
}


void lsmIndex::_freeAndClear( rocksdb::Slice & aSlice )
{
   if ( aSlice.data() )
   {
      CHAR * bufPtr = (CHAR*) aSlice.data() ;
      SDB_THREAD_FREE( bufPtr ) ;
      aSlice.clear();
   }
}


INT32 lsmIndex::_allocAndCopy( const CHAR * keyAddr,
                               const UINT32 keySize,
                               rocksdb::Slice & K )
{
   INT32 rc   = SDB_OK;
   CHAR * buf = NULL;
   if ( ( 0 != keySize ) && ( NULL != keyAddr ) )
   {
      buf = (CHAR*)SDB_THREAD_ALLOC( keySize ) ;
      if ( buf )
      {
         ossMemcpy( buf, keyAddr, keySize );
         K = rocksdb::Slice( buf, keySize );
      }
      else
      {
         rc = SDB_OOM;
      }
   }
   else
   {
      rc = SDB_INVALIDARG ;
   }
   return rc ;
}


INT32 lsmIndex::packFullIndexKey(const lsmKeyEntry &ke,
                                 rocksdb::Slice &out)
{
   INT32 rc = SDB_OK;
   SDB_ASSERT(ke.isValid(), "must be valid");
   SDB_ASSERT(out.empty(), "must be empty");
   UINT32 keyDataSize = (UINT32)(ke.getKey().dataSize());
   UINT32 bufSize = lsmCalFullDataKeyLen(keyDataSize);
   CHAR * buf = (CHAR*)SDB_THREAD_ALLOC( bufSize );
   if ( NULL == buf )
   {
      return ( rc = SDB_OOM );
   }
   ossMemset(buf, 0, bufSize) ;
   
   rc = lsmPackIndexFullKey(buf, bufSize,
                            _idxMeta.getIdxId(),
                            _idxMeta.getOrdering(),
                            ke.getKey(),
                            ke.getRid(),
                            ke.getDataLsn(),
                            ke.getTransID());
   if (SDB_OK != rc)
   {
      PD_LOG(PDERROR, "failed to pack full lsm index key:%d", rc);
      goto error;
   }

   out = rocksdb::Slice(buf, bufSize);

done:
   return rc;
error:
   goto done;
}


/*
   locate an index key entry stored in rocksdb that matches the passed
   in parameter, searchKey, regarding the searching direction.
   Input:
     direction: searching direction, 1 forward, -1 backward
     keyObj:    searching key obj
     rid:       vessel::recordID, recordID ( a.k.a row id )
   Output:
     keyEntry: the actuall key and value stored in rocksdb.
     pFound: whether find the exact match the searching key
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
   rocksdb. Therefore the returned key has following different sematics:
     . when direction > 0( forward ), the key entry returned is the newest
       one( or the smallest one from rocksb perspective ), i.e., it has
       same < keyObj, rid > with the largest LSN, because a key entry
       internally sorted on LSN field in descending order.
     . when direction < 0( backward ), the key entry returned is the oldest
       one( or the largest one from rocksb perspective ), i.e., the key has
       same key< keyObj, rid > with the smallest LSN, because a key entry
       internally sorted on LSN field in descending order.

   For example, we have following index entries saved in rocksdb
   K21 is first key entry of index 2, K36 is 6th key entry of index 3.

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
INT32 lsmIndex::locate( const INT32            direction,
                        const ixmKey           & key,
                        const recordID      & rid,
                        lsmOwnedRecord         & out,
                        BOOLEAN                & exactlyMatched )

{
   SDB_ASSERT( ( _initalized ), "LSM Index is not initialized !" );
   INT32 rc = SDB_IXM_EOC ;
   // key entry saved in rocksdb is sorted on lsn field in descending order
   UINT64 dataLsn = ( direction > 0 ) ? ((UINT64)(-1)) : 0 ;
   rocksdb::Slice searchSlice;
   lsmKeyEntry entry;
   exactlyMatched = FALSE;

   out.fini();

   if (!key.isValid())
   {
      rc = SDB_INVALIDARG;
      goto error;
   }

   entry.shallowCopy(key, rid, dataLsn, DPS_TRANS_ID());
   rc = packFullIndexKey(entry, searchSlice);
   if (SDB_OK != rc)
   {
      PD_LOG(PDERROR, "failed to pack full search key:%d", rc);
      goto error;
   }

   // create a DB iterator
   _openIter();

   if ( direction > 0 )
   {
      _it->Seek( searchSlice ) ;
   }
   else
   {
      _it->SeekForPrev( searchSlice ) ;
   }
   if ( _it->Valid() && lsmIsSameIdxId( _it->key(), _idxMeta.getIdxId() ) )
   {
      rc = SDB_OK ;
      rc = out.init( _it->key(), _it->value() ) ;
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init owned index record");
         goto error;
      }
      exactlyMatched = lsmIsSameIndexKey( _it->key(), &key, &rid );
   }

   if ( FALSE == _reUseIter )
   {
      _closeIter();
   }

   

done:
   if (!searchSlice.empty())
   {
      _freeAndClear(searchSlice);
   }
   return rc ;
error:
   goto done;
}


// an internal helper function, locate an index entry matches the passed in
// parameter, searchKey( rocksdb::Slice ), regarding the searching direction.
INT32 lsmIndex::_locate( const INT32            direction,
                         const rocksdb::Slice   & searchKey,
                         lsmOwnedRecord         & out )
{
   SDB_ASSERT( ( _initalized ), "LSM Index is not initialized !" );

   INT32 rc = SDB_IXM_EOC ;

   rocksdb::Iterator * it = _lsmdb->NewIterator( _rOpt, LSM_CF_INDEX );

   SDB_ASSERT( ( NULL != it ), "Iterator hasn't been opened !" );

   if ( direction > 0 )
   {
      it->Seek( searchKey ) ;
   }
   else
   {
      it->SeekForPrev( searchKey ) ;
   }
   if ( it->Valid() && lsmIsSameIdxId( it->key(), _idxMeta.getIdxId() ) )
   {
      rc = out.init( it->key(), it->value() ) ;
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init owned index record");
         goto error;
      }
   }

done:
   delete it ;
   return rc ;
error:
   goto done;
}


/*
   advance to next index key entry that matches the passed in parameter
   searchKey regarding the searching direction.
   Input:
     direction: searching direction, 1 forward, -1 backward
     searchKey: searching key entry
   Output:
     keyEntry: the actuall key entry stored in rocksdb.
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
   rocksdb. The locate function is be used to locate an index entry statisfies
   the "keyObj + rid" searching criteria, and advance function is used to get
   the next/previous index entry of a specific index entry( the one returned
   from locate function ).

   For example, we have following index entries saved in rocksdb
   K21 is first key entry of index 2, K36 is 6th key entry of index 3.

   example1:
   advance: dir=-1,K33( i.e., 0_{1,2,3}_{1,"t9",1}_{pg:1,slt:1}_1_{sn:1,nd:1} )
   return: K32, i.e., 0_{1,2,3}_{1,"t9",1}_{pg:1,slt:1}_2_{sn:1,nd:2}

   example2:
   advance: dir=1,K31( i.e., 0_{1,2,3}_{1,'t9',1}_{pg:1,slt:1}_3_{sn:1,nd:3} )
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
INT32 lsmIndex::advance( const INT32        direction,
                         const lsmKeyEntry & searchKey,
                         lsmOwnedRecord    & out )
{
   SDB_ASSERT( ( _initalized ), "LSM Index is not initialized !" );
   INT32 rc = SDB_OK ;
   // construct rocksdb::Slice searchSlice
   rocksdb::Slice searchSlice ;

   rc = packFullIndexKey(searchKey, searchSlice);
   if (SDB_OK != rc)
   {
      goto error;
   }

   // create DB iterator and seek
   _openIter() ;

   if ( direction > 0 )
   {
      _it->Seek( searchSlice ) ;
   }
   else
   {
      _it->SeekForPrev( searchSlice ) ;
   }
   if ( _it->Valid() && lsmIsSameIdxId( _it->key(), _idxMeta.getIdxId() ) )
   {
      // if it is same as the searchKey, move to next
      if ( 0 == lsmKeyComparator()->Compare( _it->key(), searchSlice ) )
      {
         rocksdb::Slice tmpKey ;
         rc = _allocAndCopy( _it->key().data(), _it->key().size(), tmpKey );
         if ( SDB_OK == rc )
         {
            lsmUpdateDataEntryToMostAdjacent( tmpKey, direction );
            rc = _locate( direction, tmpKey, out ) ;
            _freeAndClear( tmpKey );
         }
      }
      else
      {
         rc = out.init(_it->key(), _it->value());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init owned record:%d", rc);
            goto error;
         }         
      }
   }

   if ( FALSE == _reUseIter )
   {
      _closeIter();
   }

done:
   _freeAndClear( searchSlice );
   return rc ;
error:
   goto done;
}


INT32 lsmIndex::_advance( const INT32 direction, lsmOwnedRecord & out )
{
   SDB_ASSERT( ( NULL != _it ), "Iterator hasn't been opened !" );
   INT32 rc = SDB_IXM_EOC ;

   if ( _it->Valid() )
   {
      if ( direction > 0 )
      {
         _it->Next() ;
      }
      else
      {
         _it->Prev() ;
      }
      if ( _it->Valid() && lsmIsSameIdxId( _it->key(), _idxMeta.getIdxId() ) )
      {
         rc = out.init(_it->key(), _it->value());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init owned record:%d", rc);
            goto error;
         }  
      }
   }
done:
   return rc ;
error:
   goto done;
}


// copied from preIdxTree::_buildPredObj
BSONObj lsmIndex::_buildKeyObj( const BSONObj     & prevKey,
                                INT32               keepFieldsNum,
                                BOOLEAN             skipToNext,
                                const VEC_ELE_CMP & matchEle,
                                const inclusiveVec & matchInclusive,
                                INT32               direction ) const
{
   UINT32 index = 0 ;
   BSONObjBuilder builder ;
   BSONObjIterator itr( prevKey ) ;

   for ( ; (INT32)index < keepFieldsNum ; ++index )
   {
      BSONElement e = itr.next() ;
      builder.appendAs( e, "" ) ;
   }

   while ( index < matchEle.size() )
   {
      builder.appendAs( *matchEle[ index ], "" ) ;
      ++index ;
   }

   return builder.obj() ;
}


// locate to or advance to the next of the smallest/greatest key entry matches
// the pushed down verb and searching key object, prevKey, regarding the scan
// direction.
INT32 lsmIndex::_keySearch( const BOOLEAN            bNextOnly,
                            const INT32              direction,
                            const ixmKey           & prevKey,
                            const INT32              keepFieldsNum,
                            const BOOLEAN            skipToNext,
                            const VEC_ELE_CMP      & matchElement,
                            const inclusiveVec      & matchInclusive,
                            lsmOwnedRecord         & out )
{
   INT32  rc = SDB_IXM_EOC, result  = 0 ;
   BufBuilder builder;
   BSONObj prevKeyBson, locateBson, curKeyBson ;
   recordID rid;
   lsmOwnedRecord entryRecord;
   INT32 nFields = 0 ;
   BOOLEAN bLocateObjMatch = FALSE ;
   BOOLEAN exactlyMatched = FALSE;
   out.fini();

   if ( direction > 0 )
   {
      // set to Min
      rid = recordID(0,0);
   }
   else
   {
      // set to Max
      //invalid rid is max
   }

   // prepare key( ixmKey ) for pre-search/locate
   try
   {
      builder.reset();
      prevKeyBson = prevKey.toBson( &builder ) ;
      // get total number of fields of prevKey
      nFields = prevKeyBson.nFields();
      // build the locate BSON obj for locate operation
      locateBson = _buildKeyObj( prevKeyBson,
                                 keepFieldsNum, skipToNext,
                                 matchElement, matchInclusive,
                                 direction ) ;
   }
   catch( std::exception &e )
   {
      PD_LOG( PDWARNING, "Build pred object failed: %s", e.what() ) ;
   }

   // locate the first position regarding the pervKey and direction
   ixmKeyOwned locateIxmKey( locateBson ) ;
   rc = locate( direction, locateIxmKey, rid, entryRecord, exactlyMatched );
   if ( rc )
   {
      return rc ;
   }

   rc = SDB_IXM_EOC;
   // if is same key obj move to next when perform advance operation
   while( TRUE )
   {
      lsmKeyEntry tmpKeyEntry;
      try
      {
         curKeyBson = _buildKeyObj( entryRecord.getKey().getKey().toBson(),
                                    keepFieldsNum, skipToNext,
                                    matchElement, matchInclusive,
                                    direction ) ;
      }
      catch( std::exception &e )
      {
         PD_LOG(PDWARNING, "Build current key obj failed: %s", e.what());
         break ;
      }
      // compare the keyObj extracted from current key entry with
      // the keyObj built for locate operation, if they are same,
      // then move to next key entry when perform advance operation
      if ( 0 == curKeyBson.woCompare( locateBson, _idxMeta.getBsonOrdering() ) )
      {
         if ( ! bNextOnly )
         {
            bLocateObjMatch = TRUE ;
            break ;
         }

         tmpKeyEntry.shallowCopy( entryRecord.getKey() ) ;

         if ( FALSE == _reUseIter )
         {
            rc = advance( direction, tmpKeyEntry, entryRecord ) ;
         }
         else
         {
            rc = _advance( direction, entryRecord ) ;
         }
         if ( rc )
         {
            return rc ;
         }
      }
      else
      {
         break ;
      }
   }

   rc = SDB_IXM_EOC;
   while ( TRUE )
   {
      lsmKeyEntry tmpKeyEntry;
      result = _ixmExtent::_keyCmp( entryRecord.getKey().getKey().toBson(),
                                    prevKeyBson,
                                    keepFieldsNum, skipToNext,
                                    matchElement, matchInclusive,
                                    _idxMeta.getBsonOrdering(), direction ) ;

      /*
         _ixmExtent::_keyCmp may 'confuse' keyLocate operation. For example,
         while process entry {5,t0,1}, when direction=1, keyObj={5},
         keepFieldsNum=1 and skipToNext=1, _keyCmp returns -1 instead of 0.
           {1,tA,3}
           {2,t0,1}
           {4,t0,1}
         * {5,t0,1}
           {5,t0,2}
           {5,t1,1}
      */
      if ( ( ! bNextOnly ) && ( direction > 0 ) && skipToNext &&
             bLocateObjMatch && ( keepFieldsNum < nFields ) )
      {
         result = result * ( -1 );
      }

      if ( result * direction >= 0 )
      {
         rc = out.copy(entryRecord);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to copy entry record:%d", rc);
            goto error;
         }
         
         goto done;
      }

      tmpKeyEntry.shallowCopy( entryRecord.getKey() ) ;
      if ( FALSE == _reUseIter )
      {
         rc = advance( direction, tmpKeyEntry, entryRecord ) ;
      }
      else
      {
         rc = _advance( direction, entryRecord ) ;
      }

      if ( rc )
      {
         break ;
      }
   }

done:
   return rc ;
error:
   goto done;
}

/*
   find the smallest/greatest key entry matches the pushed down verb and
   searching keyObj, prevKey, regarding the scan direction.
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
INT32 lsmIndex::keyLocate( const INT32              direction,
                           const ixmKey           & prevKey,
                           const INT32              keepFieldsNum,
                           const BOOLEAN            skipToNext,
                           const VEC_ELE_CMP      & matchElement,
                           const inclusiveVec      & matchInclusive,
                           lsmOwnedRecord         & out )
{
   SDB_ASSERT( ( _initalized ), "LSM Index is not initialized !" );
   return _keySearch( FALSE,
                      direction,
                      prevKey,
                      keepFieldsNum,
                      skipToNext,
                      matchElement,
                      matchInclusive,
                      out );
}

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
INT32 lsmIndex::keyAdvance( const INT32              direction,
                            const ixmKey           & prevKey,
                            const INT32              keepFieldsNum,
                            const BOOLEAN            skipToNext,
                            const VEC_ELE_CMP      & matchElement,
                            const inclusiveVec      & matchInclusive,
                            lsmOwnedRecord            & out )
{
   SDB_ASSERT( ( _initalized ), "LSM Index is not initialized !" );

   return _keySearch( TRUE,
                      direction,
                      prevKey,
                      keepFieldsNum,
                      skipToNext,
                      matchElement,
                      matchInclusive,
                      out );
}


// insert a key entry packed with following format into rocksdb:
//   Key:   EntryType_IndexID_Ordering_EncodedKey_RowID_DataLSN_TransID
//   Value: Flag_RBSOffset
INT32 lsmIndex::_keyInsert( const rocksdb::Slice & key,
                            const rocksdb::Slice & value,
                            rocksdb::WriteBatch  * pBatch )
{
   INT32 rc = SDB_OK ;
   rocksdb::Status s ;
   if ( NULL == pBatch )
   {
      rocksdb::WriteOptions wrtOpt = _lsmdb->getWrtOpt() ;
      s = _lsmdb->Put( wrtOpt, LSM_CF_INDEX, key, value ) ;
   }
   else
   {
      s = pBatch->Put( _lsmdb->getCFHdl( LSM_CF_INDEX ), key, value ) ;
   }
   if ( !s.ok() )
   {
      rc = SDB_IO ;
      // log error msg
   }
   return rc ;
}

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
     logLSN:  the LSN for this insert operation
   Return:
     SDB_OK: normal return
     otherwise any popped error code
*/
INT32 lsmIndex::keyInsert(const lsmKeyEntry &key)
{
   SDB_ASSERT( ( _initalized ), "LSM Index is not initialized !" );
   INT32 rc   = SDB_OK;
   rocksdb::Slice ks, vs;
   lsmIndexValue vl;
   vl.reset(LSM_VALUE_TYPE_INSERT);
   vs = vl.getSlice();

   if (!key.isValid())
   {
      rc = SDB_INVALIDARG;
      goto error;
   }

   rc = packFullIndexKey(key, ks);
   if (SDB_OK != rc)
   {
      PD_LOG(PDERROR, "failed to pack full index key:%d", rc);
      goto error;
   }

   rc = _keyInsert(ks, vs, NULL);
   if (SDB_OK != rc)
   {
      PD_LOG(PDERROR, "failed to insert key:%d", rc);
      goto error;
   }

done:
   if (!ks.empty())
   {
      _freeAndClear(ks);
   }
   return rc ;
error:
   goto done;
}



// Mark a key entry as deleted and save the old version
// blsmIdx : whether delete a rocksdb index entry
INT32 lsmIndex::_keyDelete( const BOOLEAN blsmIdx,
                            const rocksdb::Slice & origKey,
                            const rocksdb::Slice & key,
                            const rocksdb::Slice & value,
                            rocksdb::WriteBatch  * pBatch )
{
   INT32 rc = SDB_OK ;
   rocksdb::Status s ;
   if ( NULL == pBatch )
   {
      rocksdb::WriteBatch batch ;
      rocksdb::WriteOptions wrtOpt = _lsmdb->getWrtOpt() ;
      s = batch.Put( _lsmdb->getCFHdl( LSM_CF_INDEX ), key, value ) ;
      if ( ! s.ok() )
      {
         rc = SDB_IO ;
         // log error msg
         return rc ;
      }
      if ( ! blsmIdx )
      {
         s = batch.Put( _lsmdb->getCFHdl( LSM_CF_INDEX ),
                        origKey,
                        rocksdb::Slice() );
      }
      if ( ! s.ok() )
      {
         rc = SDB_IO ;
         // log error msg
         return rc ;
      }
      s = _lsmdb->Write( wrtOpt, &batch ) ;
   }
   else
   {
      s = pBatch->Put( _lsmdb->getCFHdl( LSM_CF_INDEX ), key, value ) ;
      if ( ! s.ok() )
      {
         rc = SDB_IO ;
         // log error msg
         return rc ;
      }
      if ( ! blsmIdx )
      {
         s = pBatch->Put( _lsmdb->getCFHdl( LSM_CF_INDEX ),
                          origKey,
                          rocksdb::Slice() );
      }
   }
   if ( ! s.ok() )
   {
      rc = SDB_IO ;
      // log error msg
   }
   return rc ;
}


/*
   Mark a key entry as deleted and save the old version

   For lsm( rocksdb ) index
       the delete operation would be simply 'insert'/save a record for
       the old version index key ( a.k.a new record ); the original index key
       would be remain as it was.
       old version index key ( a.k.a new record ):
        K: EntryType_IndexID_Ordering_EncodedKey_RowID_NewDataLSN_NewTransID
        V: Flag_RBSOffset
   For btree index
       the index item would removed from btree directly and 'insert'/save
       rocksdb with two entries, one is old version index key( a.k.a new
       record ) and the other one is origin index key.
       old version index key ( a.k.a new record ):
        K: EntryType_IndexID_Ordering_EncodedKey_RowID_NewDataLSN_NewTransID
        V: Flag_RBSOffset
       original index key:
        K: EntryType_IndexID_Ordering_EncodedKey_RowID_OrigDataLSN_OrigTransID
        V: NULL
   Input:
     blsmIdx: whether delete a rocksdb key entry
     origKeyEntry:  original key entry
     keyEntry:      the key entry( old version ) to write into rocksdb to
                    mark the original key as deleted
   Return:
     SDB_OK: normal return
     otherwise any popped error code
*/

/*
INT32 lsmIndex::keyDelete( const BOOLEAN            blsmIdx,
                           const lsmKeyEntry      & origKeyEntry,
                           const lsmKeyEntry      & keyEntry,
                           const UINT64             logLSN,
                           rocksdb::WriteBatch    * pBatch )
{
   SDB_ASSERT( ( _initalized ), "LSM Index is not initialized !" );

   INT32 rc = SDB_OK;
   rocksdb::Slice origKey, key, value ;

   rc = _allocAndCopy( TRUE, logLSN, keyEntry, key ) ;
   if ( SDB_OK == rc )
   {
      CHAR buf[ lsmFlagSz + lsmRBSPosSz ];
      lsmPackDataValue( buf, (lsmFlagSz + lsmRBSPosSz),
                        keyEntry.getFlag(), keyEntry.getRBSPos() );
      value = rocksdb::Slice( buf, (lsmFlagSz + lsmRBSPosSz) );

      if ( ! blsmIdx )
      {
         rc = _allocAndCopy( TRUE, logLSN, origKeyEntry, origKey ) ;
         if ( SDB_OK == rc )
         {
            rc = _keyDelete( blsmIdx, origKey, key, value, pBatch );
            _freeAndClear( origKey );
         }
      }
      else
      {
         rc = _keyDelete( blsmIdx, origKey, key, value, pBatch );
      }
      _freeAndClear( key );
   }
   return rc ;
}
*/


// delete an entry from rocksdb without saving its old version
INT32 lsmIndex::_keyRemove( const rocksdb::Slice & key,
                            rocksdb::WriteBatch  * pBatch )
{
   INT32 rc = SDB_OK ;
   rocksdb::Status s ;
   if ( NULL == pBatch )
   {
      rocksdb::WriteOptions wrtOpt = _lsmdb->getWrtOpt() ;
      s = _lsmdb->Delete( wrtOpt, LSM_CF_INDEX, key ) ;
   }
   else
   {
      s = pBatch->Delete( _lsmdb->getCFHdl( LSM_CF_INDEX ), key ) ;
   }
   if ( ! s.ok() )
   {
      rc = SDB_IO ;
      // log error msg
   }
   return rc ;
}


/*
   delete an index entry without saving its old version
   Input:
     keyEntry: the key entry to be removed
     logLSN:   the LSN for this delete operation
   Return:
     SDB_OK: normal return
     otherwise any popped error code
*/
INT32 lsmIndex::keyRemove( const lsmKeyEntry   & keyEntry,
                           const UINT64          logLSN )
{
   SDB_ASSERT( ( _initalized ), "LSM Index is not initialized !" );

   rocksdb::Slice key;
   INT32 rc = packFullIndexKey(keyEntry, key);
   if (SDB_OK != rc)
   {
      PD_LOG(PDERROR, "failed to pack full key:%d", rc);
      goto error;
   }
   
   rc = _keyRemove( key, NULL ) ;
   if (SDB_OK != rc)
   {
      PD_LOG(PDERROR, "failed to remove index key:%d", rc);
      goto error;
   }

done:
   if (!key.empty())
   {
      _freeAndClear(key);
   }
   return rc ;
error:
   goto done;
}


/*
   truncate an index
*/
INT32 lsmIndex::truncateIndex( UINT64 logLSN )
{
   SDB_ASSERT( ( _initalized ), "LSM Index is not initialized !" );

   INT32 rc = SDB_OK ;
   // construct upper-bound and lower-bound key for DeleteRange
   rocksdb::WriteOptions wrtOpt = _lsmdb->getWrtOpt() ;
   rocksdb::Status s = _lsmdb->DeleteRange(wrtOpt, LSM_CF_INDEX, _lKey, _uKey);
   if ( ! s.ok() )
   {
      rc = SDB_IO ;
      // log error msg
   }
   return rc ;
}


/*
    drop an index
*/
INT32 lsmIndex::dropIndex( UINT64 logLSN )
{
   SDB_ASSERT( ( _initalized ), "LSM Index is not initialized !" );

   return truncateIndex( logLSN ) ;
}


/*
  LSM ( rocksdb ) index read-only operation API for isolation Repeatable Read
*/

INT32 lsmIdxRepeatableReadOnly::advance( const INT32        direction,
                                         const lsmKeyEntry & searchKey,
                                         lsmOwnedRecord       & out )
{
   SDB_ASSERT( ( _initalized ), "LSM Index is not initialized !" );
   INT32 rc = SDB_IXM_EOC ;
   // construct rocksdb::Slice searchSlice
   rocksdb::Slice searchSlice ;
   out.fini();

   rc = packFullIndexKey(searchKey, searchSlice);
   if (SDB_OK != rc)
   {
      goto error;
   }

   _openIter() ;

   if ( direction > 0 )
   {
      _it->Seek( searchSlice ) ;
   }
   else
   {
      _it->SeekForPrev( searchSlice ) ;
   }
   if ( _it->Valid() && lsmIsSameIdxId( _it->key(), _idxMeta.getIdxId() ) )
   {
      // if it is same as the searchKey, move to next
      if ( 0 == lsmKeyComparator()->Compare( _it->key(), searchSlice ) )
      {
         if ( direction > 0 )
         {
            _it->Next() ;
         }
         else
         {
            _it->Prev() ;
         }
         if ( _it->Valid() && lsmIsSameIdxId( _it->key(), _idxMeta.getIdxId() ) )
         {
            rc = out.init(_it->key(), _it->value());
            if (SDB_OK != rc)
            {
               goto error;
            }
         }
      }
      else
      {
         rc = out.init(_it->key(), _it->value());
         if (SDB_OK != rc)
         {
            goto error;
         }
      }
   }
  

done:
   if (!searchSlice.empty())
   {
       _freeAndClear( searchSlice );
   }
   return rc ;
error:
   goto done;
}


} //namespace vessel
} // namespace engine
