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

   Source File Name = dpsTransVersionCtrl.hpp

   Descriptive Name = Data Protection Services Types Header

   When/how to use: this program may be used on binary and text-formatted
   versions of dps component. This file contains declare for data types used in
   SequoiaDB.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/05/2018  YXC Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DPSTRANSVERSIONCTRL_HPP_
#define DPSTRANSVERSIONCTRL_HPP_

#include "dms.hpp"
#include "ixm.hpp"
#include "ossLatch.hpp"
#include "dmsRecord.hpp"
#include "dpsTransID.hpp"
#include "utilPooledObject.hpp"
#include "utilPooledAutoPtr.hpp"
#include "clsCatalogAgent.hpp"
#include "../bson/ordering.h"
#include "ossMemPool.hpp"
#include "rtnPredicate.hpp"
#include "dmsRBSMgr.hpp"
#include <boost/shared_ptr.hpp>

using namespace bson ;

namespace engine
{
   // class forward declare
   class preIdxTree ;
   class dpsTransCB ;
   class dpsTransLRBHeader ;
   class oldVersionContainer ;
   class dmsTransLockCallback ;
   class preIdxTreeNodeKey ;
   class preIdxTreeNodeValue ;

   // globIdxID uniquely define an index globally
   class globIdxID : public SDBObject
   {
   public:
      UINT32  _csID;   // collectionspace id
      UINT16  _clID;   // collection id
      SINT32  _idxLID; // index logic id

      globIdxID()
      {
         _csID = DMS_INVALID_SUID ;
         _clID = DMS_INVALID_MBID ;
         _idxLID = -1 ;
      }

      globIdxID( UINT32 csID, UINT16 clID, SINT32 idxLID )
      {
         _csID = csID ;
         _clID = clID ;
         _idxLID = idxLID ;
      }

      bool operator< ( const globIdxID &rhs ) const
      {
         bool rv = false ;

         if ( _csID < rhs._csID )
         {
            rv = true ;
         }
         else if ( _csID == rhs._csID )
         {
            if ( _clID < rhs._clID )
            {
               rv = true ;
            }
            else if ( _clID == rhs._clID )
            {
               if ( _idxLID < rhs._idxLID )
               {
                  rv = true ;
               }
            }
         }

         return rv ;
      }

      string toString() const ;
   } ;

   typedef _utilPooledAutoPtr dpsOldRecordPtr ;

   // use map which is implemented using red-black tree to hold
   // old version index value
   typedef  ossPoolMap<preIdxTreeNodeKey,
                       preIdxTreeNodeValue
                       > INDEX_BINARY_TREE ;

   typedef INDEX_BINARY_TREE::iterator                   INDEX_TREE_POS ;
   typedef INDEX_BINARY_TREE::const_iterator             INDEX_TREE_CPOS ;
   typedef INDEX_BINARY_TREE::reverse_iterator           INDEX_TREE_RPOS ;
   typedef INDEX_BINARY_TREE::const_reverse_iterator     INDEX_TREE_CRPOS ;

   typedef  ossPoolMap<dmsRecordID,
                       INDEX_TREE_POS
                       > INDEX_RID_TREE ;

   /** definition of preIdxTreeNodeKey
    *  preIdxTreeNodeKey is the key for node in preIdxTree.
    *  Note that the raw key is stored in _keyData.
    *  
    **/
   class preIdxTreeNodeKey : public SDBObject
   {
   // public interfaces:
   public: 
      // constructors:
      // use super class to construct key portion
      preIdxTreeNodeKey( const BSONObj* key,
                         const dmsRecordID &rid,
                         const Ordering *order,
                         const DPS_TRANS_ID &transID = DPS_TRANS_ID() ) ;

      // copy constructor
      preIdxTreeNodeKey( const preIdxTreeNodeKey &key ) ;

      ~preIdxTreeNodeKey () ;

      const Ordering* getOrdering() const
      {
         return _order ;
      }

      const dmsRecordID& getRID() const
      {
         return _rid;
      }

      BOOLEAN isValid() const
      {
         return _rid.isValid() && _order ;
      }

      const BSONObj& getKeyObj() const
      {
         return _keyObj ;
      }


      const DPS_TRANS_ID getNodeTransID() const
      {
         return _transID ;
      }

      INT32 woCompare( const preIdxTreeNodeKey &right ) const
      {
         const Ordering *pOrder = _order ? _order : right._order ;
         return _keyObj.woCompare( right._keyObj, *pOrder, FALSE ) ;
      }

      bool operator< ( const preIdxTreeNodeKey &right ) const
      {
         bool rv = false ;

         INT32 rt = woCompare( right ) ;
         if ( rt < 0 )
         {
            rv = true;
         }
         else if ( rt == 0 )// compare RID when key equals
         {
            // compare return -1 when lhk is before rhk
            if ( _rid._extent < right._rid._extent )
            {
               rv = true ;
            }
            else if ( _rid._extent == right._rid._extent )
            {
               if ( _rid._offset < right._rid._offset )
               {
                  rv = true ;
               }
               else if ( _rid._offset == right._rid._offset ) 
               {
                  if ( right._transID.isValid() && _transID.isValid() )
                  {
                     // evaluate transID if both have valid transID
                     // note that the tranID is in decending order so that 
                     // we will scan and evaluate newer version first. This
                     // way we won't mistakenly use too old version which 
                     // could be "visiable" to the newer transaction.
                     rv = _transID.getGlobSN() > right._transID.getGlobSN() ;
                  }
                  // if either side has INVALID_TRANS_ID, we treat two key
                  // equal, which we will return false
               }
            }
         }
         return rv ;
      }

      bool operator== ( const preIdxTreeNodeKey &right ) const
      {
         bool rv = false ;
         if ( 0 == woCompare( right ) && _rid == right._rid )
         {
            // evaluate transID if both nodes has valid transID
            if ( _transID.isInvalid() || right._transID.isInvalid() )
            {
               rv = true ;
            }
            else
            {
               rv = ( _transID == right._transID ) ;
            }
         }
         return rv ;
      }

      string toString() const ;

      const UINT32 size() const
      {
         return ( _keyObj.objsize() + sizeof(dmsRecordID) + 
                  sizeof(DPS_TRANS_ID) + sizeof(Ordering *) ) ;
      }
      // private attributes:
   private:
      BSONObj           _keyObj ;
      // Original rid. This is used to differ duplicated keys when index is 
      // not unique
      dmsRecordID       _rid ;
      // We use the transID as the version of the index in the tree
      // This is NOT reflecting the record transID, but the owner transID, 
      // which is the transaction adding the version into the tree. We will 
      // use this to decide when to recycle the node from the mem tree, i.e.
      // we can delete the tree node if the transID is older than lowTran.
      // This is also used for visiability check as the "owner" transID.
      DPS_TRANS_ID      _transID ;
      // it's shared from the tree. Check clsCataOrder()
      const Ordering    *_order ;
      
   } ;

   enum DPS_PREIDXTREENODEVALUE_STATUS
   {
      DPS_PREIDXTREENODEVALUE_NONE = 0,
      // preIdxTree node value is valid
      DPS_PREIDXTREENODEVALUE_VALID,
      // preIdxTree node value is deleted
      DPS_PREIDXTREENODEVALUE_DELETED,
      // _pOldVer is NULL, or recordPtr is NULL.
      // new record or dummy record are not in
      // the tree, so will be invalid.
      DPS_PREIDXTREENODEVALUE_INVALID
   } ;

   // the index tree node value is the index into the lrbHdr which contain
   // both old version index and record
   class preIdxTreeNodeValue : SDBObject
   {
   public:
      // constructor
      preIdxTreeNodeValue( oldVersionContainer *pOldVer = NULL )
      {
         _pOldVer = pOldVer ;
      }

      preIdxTreeNodeValue( const preIdxTreeNodeValue &rhs )
      {
         _pOldVer = rhs._pOldVer ;
         _ridPre = rhs._ridPre ;
         _ridNext = rhs._ridNext ;
         _rbsOffset = rhs._rbsOffset ;
      }

      ~preIdxTreeNodeValue()
      {
         _pOldVer = NULL ;
      }

      preIdxTreeNodeValue& operator=( const preIdxTreeNodeValue &rhs )
      {
         _pOldVer = rhs._pOldVer ;
         _ridPre = rhs._ridPre ;
         _ridNext = rhs._ridNext ;
         _rbsOffset = rhs._rbsOffset ;
         return *this ;
      }

      BOOLEAN isValid() const
      {
         return _pOldVer ? TRUE : FALSE ;
      }

      void  reset()
      {
         _pOldVer = NULL ;
      }

      BOOLEAN isRecordDeleted() const ;

      BOOLEAN isRecordNew() const ;

      const oldVersionContainer* getOldVer() const { return _pOldVer ; }
      const INDEX_TREE_POS   getRidPre() const  { return _ridPre ; }
      const INDEX_TREE_POS   getRidNext() const { return _ridNext ; }
      dmsRBSOffset     getRBSOffset() const { return _rbsOffset ; }

      void setRidPre(INDEX_TREE_POS pre) { _ridPre = pre ; }
      void setRidNext(INDEX_TREE_POS next) { _ridNext = next ; }


      dpsOldRecordPtr      getRecordPtr() const ;
      const dmsRecord*     getRecord() const ;
      const dmsRecordID&   getRecordID() const ;
      BSONObj              getRecordObj() const ;
      UINT32               getOwnerTID() const ;

      void setRBSOffset( const dmsRBSOffset& offset ) ;

      string toString() const ;

      DPS_PREIDXTREENODEVALUE_STATUS getStatus() const ;

      const UINT32 size() const
      {
         return ( sizeof(INDEX_TREE_POS) * 2 + sizeof(dmsRBSOffset) +
                  sizeof(oldVersionContainer *) ) ;
      }
   // private member
   private:
      oldVersionContainer    *_pOldVer ;
      // Following three fields are only used to support MVCC index scan.
      // pre and next pointer for the node with same record(RID)
      INDEX_TREE_POS          _ridPre ;   // This is previous version
      INDEX_TREE_POS          _ridNext ;  // This is newer version
      // Corresponding old record version location stored in RBS 
      dmsRBSOffset            _rbsOffset ;
   } ;


   /** definition of preIdxTree
    *  preIdxTree is a red-black tree which holds all old key values of a 
    *  specific index during runtime. Index scanner will merge this in-memory
    *  tree with the index on disk during runtime based on its isolation level.
    *  The rule of thumb is: on-disk index contain latest value with could be 
    *  uncommitted. in-memory preIdxTree holds the last committed value.
    **/
   class preIdxTree : public SDBObject
   {
      friend class oldVersionCB ;
      friend class oldVersionContainer ;

   // public interfaces:
   public: 
      // constructors & destructors
      preIdxTree( const SINT32 idxID, const ixmIndexCB *indexCB ) ;

      // copy constructor
      preIdxTree( const preIdxTree &intree ) ;

      // destructor
      ~preIdxTree() ;

      BOOLEAN isValid() const
      {
         return ( _isValid && _order ) ? TRUE : FALSE ;
      }

      const INDEX_BINARY_TREE* getTree() const
      {
         return &_tree ;
      }

      // get index logic ID
      SINT32 getLID() const
      {
         return _idxLID ;
      }

      const Ordering * getOrdering() const
      {
         return _order->getOrdering() ;
      }

      const BSONObj& getKeyPattern() const
      {
         return _keyPattern ;
      }

      // find a node based on the key, caller need to hold the latch
      // otherwise the iterator can change underneath
      INDEX_TREE_CPOS   find( const preIdxTreeNodeKey &key ) const ;
      INDEX_TREE_CPOS   find ( const BSONObj *key,
                               const dmsRecordID &rid,
                               const DPS_TRANS_ID &transID = DPS_TRANS_ID()
                                                                     ) const ;
      BOOLEAN           isPosValid( INDEX_TREE_CPOS pos ) const ;

      void              resetPos( INDEX_TREE_CPOS &pos ) const ;
      INDEX_TREE_CPOS   beginPos() const ;

      // caller need to hold the latch otherwise the iterator can
      // change underneath
      const preIdxTreeNodeKey&   getNodeKey( INDEX_TREE_CPOS pos ) const ;
      const preIdxTreeNodeValue& getNodeData( INDEX_TREE_CPOS pos ) const ;

      // Traverse the tree to see if the key exist, caller need to hold 
      // the latch otherwise the iterator can change underneath
      BOOLEAN isKeyExist( const BSONObj &key,
                          preIdxTreeNodeValue &value ) const ;

      INT32 advance( INDEX_TREE_CPOS &pos, INT32 direction ) const ;

      // locate the best match key in the tree based on the order and direction
      INT32 locate ( const BSONObj      &keyObj,
                     const dmsRecordID  &rid,
                     INDEX_TREE_CPOS    &pos,
                     BOOLEAN            &found,
                     INT32              direction ) const ;

      // move the iterator to the proper key location
      INT32 keyLocate( INDEX_TREE_CPOS &pos, const BSONObj &prevKey,
                       INT32 keepFieldsNum, BOOLEAN skipToNext,
                       const VEC_ELE_CMP &matchEle,
                       const VEC_BOOLEAN &matchInclusive,
                       INT32 direction ) const ;

      // Advance the pushed down verb and locate the key
      INT32 keyAdvance( INDEX_TREE_CPOS &pos,
                        const BSONObj &prevKey,
                        INT32 keepFieldsNum, BOOLEAN skipToNext,
                        const VEC_ELE_CMP &matchEle,
                        const VEC_BOOLEAN &matchInclusive,
                        INT32 direction ) const ;

      void  setDeleted() { _isValid = FALSE ; }

      INT32 insertWithOldVer( const BSONObj *keyData,
                              const dmsRecordID &rid,
                              oldVersionContainer *oldVer,
                              BOOLEAN hasLock,
                              const DPS_TRANS_ID &transID = DPS_TRANS_ID(),
                              dmsTransLockCallback * callback = NULL,
                              INT32 indexID = -1 ) ;

      void lockX()
      {
         _latch.get() ;
      }

      void lockS()
      {
         _latch.get_shared() ;
      }

      void unlockX()
      {
         _latch.release() ;
      }

      void unlockS()
      {
         _latch.release_shared() ;
      }

      BOOLEAN tryLockX()
      {
         return _latch.try_get() ;
      }

      BOOLEAN tryLockS()
      {
         return _latch.try_get_shared() ;
      }

      BOOLEAN empty() const
      {
         return _tree.empty() ;
      }

      INT32 size() const
      {
         return _tree.size() ;
      }

      BOOLEAN hasRidPre( INDEX_TREE_CPOS & pos ) 
      {
         return ( this->getNodeData(pos).getRidPre() != _tree.end() ) ;
      }

      BOOLEAN hasRidNext( INDEX_TREE_CPOS & pos ) 
      {
         return ( this->getNodeData(pos).getRidNext() != _tree.end() ) ;
      }

      INDEX_TREE_POS getKeyNodeFromRidTree( dmsRecordID rid ) ; 

      // assistant function to print out the whole tree.
      void printTree( BOOLEAN detailed = TRUE) const ;

      SINT64 getSizeHWM ( ) const
      {
         return _sizeHWM.peek() ;
      }

      SINT64 getPreSize ( ) const
      {
         return _preSize.peek() ;
      }

   protected:
      // insert a node to map
      INT32 insert ( const preIdxTreeNodeKey &keyNode,
                     const preIdxTreeNodeValue &value,
                     BOOLEAN hasLock = FALSE ) ;

      INT32 insert ( const BSONObj *keyData,
                     const dmsRecordID &rid,
                     const preIdxTreeNodeValue &value,
                     BOOLEAN hasLock = FALSE,
                     const DPS_TRANS_ID &transID = DPS_TRANS_ID() ) ;

      // delete a node
      UINT32 remove( const preIdxTreeNodeKey &keyNode,
                     const oldVersionContainer *pOldVer,
                     BOOLEAN hasLock = FALSE ) ;

      UINT32 remove( const BSONObj *keyData,
                     const dmsRecordID &rid,
                     const oldVersionContainer *pOldVer,
                     BOOLEAN hasLock = FALSE ) ;

      // delete all nodes for a record
      void  removeForRecord( SINT32 extID, SINT32 offset ) ;

      void  resetValue( const preIdxTreeNodeKey &keyNode,
                        DPS_TRANSID_SN           ownerTransID,
                        BOOLEAN                  hasLock = FALSE ) ;

      void  clear( BOOLEAN hasLock = FALSE ) ;
      DPS_TRANSID_SN gc( DPS_TRANSID_SN lowTran ) ;

      BSONObj     _buildPredObj( const BSONObj &prevKey,
                                 INT32 keepFieldsNum,
                                 BOOLEAN skipToNext,
                                 const VEC_ELE_CMP &matchEle,
                                 const VEC_BOOLEAN &matchInclusive,
                                 INT32 direction ) const ;

   private:
      void  _adjustRidChainForErase( INDEX_TREE_POS & pos ) ;

   // private attributes:
   private:
      BOOLEAN             _isValid ;
      SINT32              _idxLID ; // index logic id
      DPS_TRANSID_SN      _lastLowTranID ; // The lowTran in the tree from last GC
      // Latching protocal
      // 1. preIdxTree latch must be held in X to insert/delete node in the tree
      //    oldVersionCB(_oldVersionCBLatch) need to be held in S before
      //    taking individual preIdxTree latch.
      // 2. Should never request LRB hash bkt latch while holding preIdxTree 
      //    latch, reverse order is OK. Keep in mind we store the _lrbHdrIdx
      //    in the tree so that we have direct access to lrbHdr without need
      //    to go through lrbhash bkt.
      monSpinSLatch       _latch ;  // latch for concurrency control, 
                                    // adding/removing node need latch in X
                                    // find/travers need latch in S
      INDEX_BINARY_TREE   _tree ;   // tree to hold all old index key value
      clsCataOrder       *_order ;  // wrap class to hold the shared ordering
      BSONObj             _keyPattern ;
      // a separate tree ordered by RID, the value points to the latest
      // index value(iterator) in the above _tree. This tree only exists
      // if MVCC and Globtrans are enabled. This is also protected by the 
      // _latch. Note that we normally update the _ridTree the same time we
      // touch _tree, under the same _latch at the same time.
      INDEX_RID_TREE      _ridTree ;
      // tree statistic data
      ossAtomic64  _sizeHWM ; // max number of elements ever existed in the map
      ossAtomic64  _preSize ; // previous number of elements before GC/cleanup

   } ;

   typedef utilSharePtr<preIdxTree>       preIdxTreePtr ;

   // global map from an index to its own tree
   typedef  ossPoolMap< const globIdxID,
                        preIdxTreePtr
                       > IDXID_TO_TREE_MAP ;

   typedef IDXID_TO_TREE_MAP::iterator       IDXID_TO_TREE_MAP_IT ;

   typedef  std::pair< const globIdxID,
                       preIdxTreePtr
                     > IDXID_TO_TREE_MAP_PAIR ;

   #define OLD_VERUNIT_ITR_STEP_DFT             ( 200 )
   #define OLD_VERUNIT_ITR_INTERVAL_DFT         ( 100 )

   /*
      oldVersionUnit define
   */
   class oldVersionUnit : public utilPooledObject
   {
      public:
         class iterator
         {
            public:
               iterator()
               {
                  _pUnit = NULL ;
                  _cur = NULL ;
                  _lockOnChain = FALSE ;
                  _locked = FALSE ;
                  _init = FALSE ;

                  _stepCnt = OLD_VERUNIT_ITR_STEP_DFT ;
                  _getCnt  = 0 ;
                  _interval = OLD_VERUNIT_ITR_INTERVAL_DFT ;
               }

               iterator( oldVersionUnit *pUnit,
                         INT64 stepCnt = OLD_VERUNIT_ITR_STEP_DFT,
                         INT32 interval = OLD_VERUNIT_ITR_INTERVAL_DFT )
               {
                  _pUnit = pUnit ;
                  _cur = NULL ;
                  _lockOnChain = FALSE ;
                  _locked = FALSE ;
                  _init = FALSE ;

                  _stepCnt = stepCnt ;
                  _getCnt  = 0 ;
                  _interval = interval ;

                  if ( _interval < 0 )
                  {
                     _interval = OLD_VERUNIT_ITR_INTERVAL_DFT ;
                  }
               }

               iterator( const iterator &rhs )
               {
                  _pUnit = NULL ;
                  _cur = NULL ;
                  _lockOnChain = FALSE ;
                  _locked = FALSE ;
                  _init = FALSE ;

                  _stepCnt = OLD_VERUNIT_ITR_STEP_DFT ;
                  _getCnt  = 0 ;
                  _interval = OLD_VERUNIT_ITR_INTERVAL_DFT ;

                  this->operator= ( rhs ) ;
               }

               ~iterator()
               {
                  release() ;
               }

               iterator& operator= ( const iterator &rhs ) ;
               void release() ;
               oldVersionContainer* next() ;

            protected:
               void pause() ;
               void resume() ;

            private:
               oldVersionUnit             *_pUnit ;
               oldVersionContainer        *_cur ;
               BOOLEAN                    _lockOnChain ;
               BOOLEAN                    _locked ;
               BOOLEAN                    _init ;
               INT64                      _stepCnt ;
               INT64                      _getCnt ;
               INT32                      _interval ;
         } ;

      public:
         oldVersionUnit() ;
         ~oldVersionUnit() ;

         void lockX()
         {
            _latch.get() ;
         }

         void lockS()
         {
            _latch.get_shared() ;
         }

         void unlockX()
         {
            _latch.release() ;
         }

         void unlockS()
         {
            _latch.release_shared() ;
         }

         BOOLEAN empty() const { return !_pChain ? TRUE : FALSE ;  }

      public:

         void           addToChain( oldVersionContainer *pOldVer,
                                    BOOLEAN hasLock = FALSE ) ;
         void           removeFromChain( oldVersionContainer *pOldVer,
                                         BOOLEAN hasLock = FALSE ) ;
         void           clearChain( BOOLEAN hasLock = FALSE ) ;

         iterator       itr( INT64 stepCnt = OLD_VERUNIT_ITR_STEP_DFT,
                             INT32 interval = OLD_VERUNIT_ITR_INTERVAL_DFT ) ;

      private:
         oldVersionContainer        *_pChain ;
         ossSpinSLatch              _latch ;
         ossEvent                   _event ;
   } ;

   typedef utilSharePtr<oldVersionUnit>               oldVersionUnitPtr ;
   typedef ossPoolMap< UINT64, oldVersionUnitPtr >    MAP_OLDVERION_UNIT ;
   typedef MAP_OLDVERION_UNIT::iterator               MAP_OLDVERION_UNIT_IT ;
   typedef std::pair< UINT64, oldVersionUnitPtr>      MAP_OLDVERION_UNIT_PAIR ;

   /** definition of oldVersionCB
    *  Control block holding all resources and structures for old version 
    *  records and index keys. It's globally hanging of dpsTransCB
    **/
   class oldVersionCB : public SDBObject
   {
   // public interfaces
   public:
      // constructor
      oldVersionCB() ;

      // destructor
      ~oldVersionCB() ;

      INT32 init() ;
      void  fini() ;

      void latchS()
      {
         _oldVersionCBLatch.get_shared() ;
      }

      void latchX()
      {
         _oldVersionCBLatch.get() ;
      }

      void releaseX()
      {
         _oldVersionCBLatch.release() ;
      }

      void releaseS()
      {
         _oldVersionCBLatch.release_shared() ;
      }

      INT32             addIdxTree( const globIdxID &gid,
                                    const ixmIndexCB *indexCB,
                                    preIdxTreePtr &treePtr,
                                    BOOLEAN hasLock ) ;

      preIdxTreePtr     getIdxTree( const globIdxID &gid,
                                    BOOLEAN hasLock ) ;

      INT32             getOrCreateIdxTree( const globIdxID &gid,
                                            const ixmIndexCB *indexCB,
                                            preIdxTreePtr &treePtr,
                                            BOOLEAN hasLock ) ;

      void              delIdxTree( const globIdxID &gid,
                                    BOOLEAN hasLock ) ;

      void              gcIdxTrees( ) ;

      void              clearIdxTreeByCSID( UINT32 csID,
                                            BOOLEAN hasLock ) ;
      void              clearIdxTreeByCLID( UINT32 csID,
                                            UINT16 clID,
                                            BOOLEAN hasLock ) ;
      void              cleanIdxNodesForRecord( UINT32 csID,
                                                UINT16 clID,
                                                SINT32 extID,
                                                SINT32 offset ) ;

      INT32             addOldVersionUnit( UINT32 csID,
                                           UINT32 clID,
                                           oldVersionUnitPtr &unitPtr,
                                           BOOLEAN hasLock = FALSE ) ;

      oldVersionUnitPtr getOldVersionUnit( UINT32 csID,
                                           UINT32 clID,
                                           BOOLEAN hasLock = FALSE ) ;

      INT32             getOrCreateOldVersionUnit( UINT32 csID,
                                                   UINT32 clID,
                                                   oldVersionUnitPtr &unitPtr,
                                                   BOOLEAN hasLock = FALSE ) ;

      void              delOldVersionUnit( UINT32 csID,
                                           UINT32 clID,
                                           BOOLEAN hasLock = FALSE ) ;

      void              clearOldVersionUnitByCS( UINT32 csID,
                                                 BOOLEAN hasLock = FALSE ) ;

      void              updateMinLowTranSN( DPS_TRANSID_SN lowTran )
      {
         _minTransIDSN.swapGreaterThan( lowTran ) ;
      }

      DPS_TRANSID_SN    getMinLowTranSN( )
      {
         return _minTransIDSN.fetch() ;
      }

      UINT64            getTreeSizeHWM( )
      {
         return _treeSizeHWM.peek() ;
      }

   // private attributes
   private:
      // latch to protect the fields. Should hold it in X to initialize and 
      // destroy _memBlockPool, otherwise hold in S
      ossSpinSLatch       _oldVersionCBLatch ;
      IDXID_TO_TREE_MAP   _idxTrees ;     // in memory trees holding older 
                                          // version of indexes
      MAP_OLDVERION_UNIT  _mapOldVersionUnit ;
      // The smallest transID among all trees
      DPS_TRANSID_SN_ATOMIC _minTransIDSN ;
      // statistic data
      // max number of elements ever existed in any tree
      ossAtomic64  _treeSizeHWM ;
   } ;

   /*
      dpsIdxObj define
   */
   class dpsIdxObj : public SDBObject
   {
   public:
      dpsIdxObj()
      {
         _idxLID = -1 ;
      }

      dpsIdxObj( const BSONObj &key, SINT32 idxLID )
      :_idxLID( idxLID ), _idxObj( key.getOwned() )
      {
      }

      dpsIdxObj( const dpsIdxObj &rhs )
      {
         _idxLID = rhs._idxLID ;
         _idxObj = rhs._idxObj ;
      }

      dpsIdxObj& operator= ( const dpsIdxObj &rhs )
      {
         _idxLID = rhs._idxLID ;
         _idxObj = rhs._idxObj ;
         return *this ;
      }

      SINT32   getIdxLID() const { return _idxLID ; }
      const BSONObj& getKeyObj() const { return _idxObj ; }

      void     setIdxLID( SINT32 idxLID ) { _idxLID = idxLID ; }
      void     setKeyObj( const BSONObj &obj ) { _idxObj = obj.getOwned() ; }

      bool     operator< ( const dpsIdxObj &rhs ) const
      {
         bool rv = false ;

         if ( _idxLID < rhs._idxLID )
         {
            rv = true ;
         }
         else if( _idxLID == rhs._idxLID )
         {
            // same LID, do bson compare based on order
            rv = ( _idxObj.woCompare( rhs._idxObj, BSONObj(), FALSE ) < 0 ) ;
         }

         return rv ;
      }

   private:
      SINT32            _idxLID ; // index unique logical ID
      BSONObj           _idxObj ; // BSON Object
   } ;

   // Use set of idx lid to figure out if the first version of an index value 
   // was already stored or not. 
   typedef ossPoolMap< SINT32, preIdxTreePtr >     idxLidMap ;
   // use set of idxObj to store all index key values
   typedef ossPoolSet< dpsIdxObj >                 idxObjSet ;

   // indicate it is a newly created record
   #define OLDVER_MASK_NEW_RECORD            0x00000001
   // indicate it is a dummy record when not use rollback segment
   #define OLDVER_MASK_DUMMY                 0x00000002
   // indicate when trans is committed, in mem older record off lrbhdr is deleted
   #define OLDVER_MASK_DELETED               0x00000004
   // indicate part of delete operation created this oldVer.
   // We could delete the record with light job during gc
   #define OLDVER_MASK_DISK_DELETING         0x00000008
 //#define OLDVER_MASK_HAS_COPED             0x00000010
   #define OLDVER_MASK_ROLLED_BACK           0x00000020

   // Class to store all information for old version record/indexes. This 
   // container is currently hanging off LRBHdr
   class oldVersionContainer : public _utilPooledObject
   {
      friend class preIdxTree ;
      friend class oldVersionUnit ;

   public:
      oldVersionContainer( const dmsRecordID &rid,
                           INT32 csID, UINT16 clID,
                           UINT32 csLID, UINT32 clLID ) ;
      ~oldVersionContainer() ;

      BOOLEAN              isRecordEmpty() const ;

      SINT32               getCSID() const { return _csID ; }
      UINT16               getCLID() const { return _clID ; }
      UINT32               getCSLID() const { return _csLID ; }
      UINT32               getCLLID() const { return _clLID ; }

      const dmsRecordID&   getRecordID() const { return _rid ; }
      dpsOldRecordPtr      getRecordPtr() const { return _recordPtr ; }
      DPS_TRANS_ID         getRecordTransID() const { return _recordTransID ; }
      DPS_TRANS_ID         getOwnerTransID() const { return _ownerTransID; }
      BSONObj              getRecordObj() const ;
      const dmsRecord*     getRecord() const ;

      INT32                saveRecord( const dmsRecord *pRecord,
                                       const BSONObj &obj,
                                       UINT32 ownnerTID,
                                       DPS_TRANS_ID recordTransID ) ;
      void                 releaseRecord( dmsTransLockCallback* callback = NULL ) ;
      BOOLEAN              tryReleaseRecord( dmsTransLockCallback* callback = NULL ) ;
      // release all items created by this old version from given index tree
      BOOLEAN              releaseIndex( dmsTransLockCallback *callback,
                                         preIdxTreePtr &treePtr,
                                         BOOLEAN hasLocked ) ;

      void                 setRecordDeleted() ;
      void                 setOwnerTransID( DPS_TRANS_ID const & ownerTransID ) 
                           { _ownerTransID = ownerTransID ; }

      BOOLEAN              isRecordDeleted() const ;
      BOOLEAN              hasRecord() const ;

      void                 setDiskDeleting() ;
      BOOLEAN              isDiskDeleting() const ;

      void                 setRecordNew( UINT32 ownerTID ) ;
      BOOLEAN              isRecordNew() const ;

      void                 setRolledback( ) ;
      BOOLEAN              isRolledback() const ;

      void                 setRecordDummy( UINT32 ownerTID ) ;
      BOOLEAN              isRecordDummy() const ;

      UINT32               getOwnerTID() const ;

      BOOLEAN              isIndexObjEmpty() const ;

      // check if the index lid already exists in the set
      BOOLEAN              idxLidExist( SINT32 idxLID ) const ;
      INT32                insertIdxTree( preIdxTreePtr treePtr,
                                          BOOLEAN *pInserted = NULL ) ;

      // based on index LID passed in, retrieve the index value
      const dpsIdxObj*     getIdxObj( SINT32 idxLID ) const ;
      BOOLEAN              isIdxObjExist( const dpsIdxObj &obj ) const ;

      BOOLEAN              isOnChain() const { return _isOnChain ; }
      BOOLEAN              isLockOnChain() const ;

      /*
      static oldVersionContainer*   newThis( const dmsRecordID &rid,
                                             INT32 csID, UINT16 clID,
                                             UINT32 csLID, UINT32 clLID) ;
      oldVersionContainer*          copyThis() ;
      void                          releaseThis() ;
      BOOLEAN                       hasCoped() const ;
      */

   protected:
      // given an index object, insert into the idxObjSet. Return false
      // if the same index for the record already exist. In this case,
      // the object was not inserted
      BOOLEAN              insertIdx( const dpsIdxObj &i ) ;

      oldVersionContainer *getNext() const { return _next ; }
      oldVersionContainer *getPrev() const { return _prev ; }
      void setNext( oldVersionContainer * ptr ) { _next = ptr ; }
      void setPrev( oldVersionContainer * ptr ) { _prev = ptr ; }
      void setOnChain() { _isOnChain = TRUE ; }
      void unsetOnChain() { _isOnChain = FALSE ; }
      void lockOnChain() ;
      void unlockOnChain() ;

      void _releaseRecord( preIdxTree *pTree,
                           const preIdxTreeNodeKey &keyNode,
                           BOOLEAN treeLatchHeld ) ;

   private:
      INT32             _csID ;
      UINT16            _clID ;
      UINT32            _csLID ;
      UINT32            _clLID ;
      dmsRecordID       _rid ;
      dpsOldRecordPtr   _recordPtr ;   // pointer to copy of old record
      UINT32            _ownerTID ;   // owner thread ID
      DPS_TRANS_ID      _recordTransID ; // record transID as the version
      DPS_TRANS_ID      _ownerTransID ; // transaction ID from owner
      UINT32            _statMask ;
      // A set of index Lids (up to 64) associated to this record.
      // We use this to figure out if the idx was already stored in the tree
      // Keep in mind that RC require us read the last committed version which
      // will be the version before first update in the transaction
      idxLidMap         _oldIdxLid ; 
      // point to a set containing all key sets.
      idxObjSet         _oldIdx ;
      oldVersionContainer * _prev ;
      oldVersionContainer * _next ;
      BOOLEAN               _isOnChain ;
      UINT32               _lockCnt ;
      /*
      INT32                _refCount ;
      */
   } ;

}

#endif //DPSTRANSVERSIONCTRL_HPP_

