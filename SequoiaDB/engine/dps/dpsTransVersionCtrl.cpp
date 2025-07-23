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

   Source File Name = dpsTransVersionCtrl.cpp

   Descriptive Name = dps transaction version control

   When/how to use: this program may be used on binary and text-formatted
   versions of Data Protection component. This file contains functions for
   transaction isolation control through version control implmenetation.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/05/2018  YC  Initial Draft

   Last Changed =

*******************************************************************************/
#include "dpsTransVersionCtrl.hpp"
#include <sstream>
#include "ossMem.hpp"
#include "pdTrace.hpp"
#include "dmsCB.hpp"
#include "dpsTrace.hpp"
#include "dmsRBSSUMgr.hpp"
#include "dpsTransCB.hpp"
#include "dmsTransLockCallback.hpp"
#include "ixmExtent.hpp" // for _keyCmp
#include "dpsUtil.hpp"
#include "pdSecure.hpp"

using namespace bson ;

namespace engine
{
   string globIdxID::toString() const
   {
      std::stringstream ss ;
      ss << "CSID:" << _csID << ", CLID:" << _clID
         << ", IDXLID:" << _idxLID ;
      return ss.str() ;
   }

   /*
      preIdxTreeNodeKey implement
   */
   preIdxTreeNodeKey::preIdxTreeNodeKey( const BSONObj* key,
                                         const dmsRecordID &rid,
                                         const Ordering *order,
                                         const DPS_TRANS_ID &transID )
   :_keyObj( *key ), _transID( transID ), _order( order )
   {
      _rid._extent = rid._extent ;
      _rid._offset = rid._offset ;
   }

   preIdxTreeNodeKey::preIdxTreeNodeKey( const preIdxTreeNodeKey &key )
   : _keyObj( key._keyObj ), _transID( key._transID ), _order( key._order )
   {
      _rid._extent = key._rid._extent ;
      _rid._offset = key._rid._offset ;
   }

   preIdxTreeNodeKey::~preIdxTreeNodeKey ()
   {
      // simply rid to invalid in case some one continue using it.
      _rid.reset() ;
   }

   string preIdxTreeNodeKey::toString( BOOLEAN encryptKey ) const
   {
      std::stringstream ss ;
      ss << "RID(" << _rid._extent << "," << _rid._offset
         << ", Key:"
         << ( encryptKey ? PD_SECURE_OBJ( _keyObj ) : _keyObj.toString() )
         << ", TransID:" << dpsTransIDToString( _transID ).c_str() ;
      return ss.str() ;
   }

   /*
      preIdxTreeNodeValue implement
   */
   BOOLEAN preIdxTreeNodeValue::isRecordDeleted() const
   {
      if ( _pOldVer )
      {
         return _pOldVer->isRecordDeleted() ;
      }
      return FALSE ;
   }

   BOOLEAN preIdxTreeNodeValue::isRecordNew() const
   {
      if ( _pOldVer )
      {
         return _pOldVer->isRecordNew() ;
      }
      return FALSE ;
   }

   dpsOldRecordPtr preIdxTreeNodeValue::getRecordPtr() const
   {
      if ( _pOldVer )
      {
         return _pOldVer->getRecordPtr() ;
      }
      return dpsOldRecordPtr() ;
   }

   const dmsRecord* preIdxTreeNodeValue::getRecord() const
   {
      return ( const dmsRecord* )getRecordPtr().get() ;
   }

   const dmsRecordID& preIdxTreeNodeValue::getRecordID() const
   {
      static dmsRecordID _dummyID ;
      if ( _pOldVer )
      {
         return _pOldVer->getRecordID() ;
      }
      return _dummyID ;
   }

   BSONObj preIdxTreeNodeValue::getRecordObj() const
   {
      if ( _pOldVer )
      {
         return _pOldVer->getRecordObj() ;
      }
      return BSONObj() ;
   }

   UINT32 preIdxTreeNodeValue::getOwnerTID() const
   {
      if ( _pOldVer )
      {
         return _pOldVer->getOwnerTID() ;
      }
      return 0 ;
   }

   string preIdxTreeNodeValue::toString( BOOLEAN encryptObj ) const
   {
      dmsRecordID rid = getRecordID() ;
      BSONObj obj = getRecordObj() ;

      std::stringstream ss ;
      ss << "RecordID(" <<  rid._extent << ", " << rid._offset << "), " ;
      if ( isRecordDeleted() )
      {
         ss << "(Deleted)" ;
      }

      ss << "Object("
         << ( encryptObj ? PD_SECURE_OBJ( obj ) : obj.toString() )
         << ")" ;

      ss << "rbsOffset(" << _rbsOffset._clID << ", "
         << _rbsOffset._logicalID << ")" ;

      return ss.str() ;
   }

   void preIdxTreeNodeValue::setRBSOffset( const dmsRBSOffset& offset )
   {
      _rbsOffset = offset ;
   }

   DPS_PREIDXTREENODEVALUE_STATUS preIdxTreeNodeValue::getStatus() const
   {
      DPS_PREIDXTREENODEVALUE_STATUS result = DPS_PREIDXTREENODEVALUE_INVALID ;
      // When _pOldVer is NULL or recordPtr is NULL,
      // it will be treated invalid. As for new record or dummy record,
      // they are not in the tree, the recordPtr will be NULL,
      // thus they will be treated as invalid.
      if ( ( NULL != _pOldVer ) && ( NULL != getRecord() ) )
      {
         if ( isRecordDeleted() )
         {
            result = DPS_PREIDXTREENODEVALUE_DELETED ;
         }
         else
         {
            result = DPS_PREIDXTREENODEVALUE_VALID ;
         }
      }
      return result ;
   }


   /*
      preIdxTree implement
   */
   preIdxTree::preIdxTree( const SINT32 idxID, const ixmIndexCB *indexCB )
    : _latch(MON_LATCH_PREIDXTREE_LATCH) ,
      _sizeHWM(0) ,
      _preSize(0)
   {
      _isValid = TRUE ;
      _lastLowTranID = DPS_INVALID_TRANSID_SN ;
      _idxLID = idxID ;
      _keyPattern = indexCB->keyPattern().getOwned() ;
      _order = SDB_OSS_NEW clsCataOrder( Ordering::make( _keyPattern ) ) ;
   }

   // copy constructor
   preIdxTree::preIdxTree( const preIdxTree &intree )
    : _latch(MON_LATCH_PREIDXTREE_LATCH) ,
      _sizeHWM( intree.getSizeHWM() ) ,
      _preSize( intree.getPreSize() )
   {
      _idxLID = intree._idxLID ;
      _keyPattern = intree._keyPattern ;
      _lastLowTranID = intree._lastLowTranID ;
      _tree = intree._tree ;
      _isValid = intree._isValid ;
      _order = SDB_OSS_NEW clsCataOrder( Ordering::make( _keyPattern ) ) ;
   }

   // destructor
   preIdxTree::~preIdxTree()
   {
      if ( NULL != _order )
      {
         SDB_OSS_DEL _order ;
      }
   }

   INDEX_TREE_CPOS preIdxTree::find( const preIdxTreeNodeKey &key ) const
   {
      return _tree.find( key ) ;
   }

   INDEX_TREE_CPOS preIdxTree::find ( const BSONObj *key,
                                      const dmsRecordID &rid,
                                      const DPS_TRANS_ID &transID ) const
   {
      return find( preIdxTreeNodeKey( key, rid, getOrdering(), transID ) ) ;
   }

   BOOLEAN preIdxTree::isPosValid( INDEX_TREE_CPOS pos ) const
   {
      return pos != _tree.end() ? TRUE : FALSE ;
   }

   void preIdxTree::resetPos( INDEX_TREE_CPOS & pos ) const
   {
      pos = _tree.end() ;
   }

   INDEX_TREE_CPOS preIdxTree::beginPos() const
   {
      return _tree.begin() ;
   }

   const preIdxTreeNodeKey& preIdxTree::getNodeKey( INDEX_TREE_CPOS pos ) const
   {
      SDB_ASSERT( pos != _tree.end(), "Pos is invalid" ) ;
      return pos->first ;
   }

   const preIdxTreeNodeValue& preIdxTree::getNodeData( INDEX_TREE_CPOS pos ) const
   {
      SDB_ASSERT( pos != _tree.end(), "Pos is invalid" ) ;
      return pos->second ;
   }

   void preIdxTree::clear( BOOLEAN hasLock )
   {
      if ( !hasLock )
      {
         lockX() ;
      }

      _tree.clear() ;
      _sizeHWM.init(0) ;
      _preSize.init(0) ;

      if ( !hasLock )
      {
         unlockX() ;
      }
   }

   // Description:
   //   insert a node to map. Latch is held within the function
   // Input:
   //   keyNode: key to insert
   //   value:   value to insert
   //   lockHeld: if the tree lock is already held or not
   // Return:
   //   SDB_OK if success.  Error code on any failure
   // PD_TRACE_DECLARE_FUNCTION ( SDB_PREIDXTREE_INSERT, "preIdxTree::insert" )
   INT32 preIdxTree::insert ( const preIdxTreeNodeKey &keyNode,
                              const preIdxTreeNodeValue &value,
                              BOOLEAN hasLock )
   {
      PD_TRACE_ENTRY( SDB_PREIDXTREE_INSERT );

      INT32 rc = SDB_OK ;
      std::pair< INDEX_TREE_POS, BOOLEAN > ret ;
      preIdxTreeNodeValue tmpValue ;

      //input check
      SDB_ASSERT( keyNode.isValid(), "key is invalid" ) ;
      SDB_ASSERT( value.isValid() , "value is invalid" ) ;

      if ( !keyNode.isValid() || !value.isValid() )
      {
         rc = SDB_SYS ;
         goto error ;
      }

      // insert the pair into the map(tree)
      if( !hasLock )
      {
         lockX() ;
      }

      try
      {
         ret = _tree.insert( INDEX_BINARY_TREE::value_type( keyNode, value ) ) ;
         if ( !ret.second )
         {
            tmpValue = ret.first->second ;
         }
      }
      catch ( std::exception &e )
      {
         if( !hasLock )
         {
            unlockX();
         }

         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      _sizeHWM.swapGreaterThan( _tree.size() ) ;

      // if mvccOn, we need to modify the _ridTree and _ridPre/_ridNext
      if ( ret.second && pmdGetOptionCB()->mvccOn() )
      {
         INDEX_RID_TREE::iterator pre = _ridTree.find(keyNode.getRID()) ;

         try
         {
            if ( pre == _ridTree.end() )
            {
               // rid not exist, add it
               ret.first->second.setRidPre( _tree.end() ) ;
               ret.first->second.setRidNext( _tree.end() ) ;
               _ridTree.insert( INDEX_RID_TREE::value_type( keyNode.getRID(),
                                                            ret.first) ) ;
#if SDB_INTERNAL_DEBUG
               PD_LOG( PDDEBUG,
                       "Inserted rid[%d, %d] version(%s) to rid tree[%d]",
                       keyNode.getRID()._extent, keyNode.getRID()._offset,
                       dpsTransIDToString(keyNode.getNodeTransID()).c_str(),
                       _idxLID ) ;
#endif
            }
            else
            {
               // rid already exist. Note that newly inserted one
               // should always be the newest version, thus be directly
               // pointed by ridTree node.
               pre->second->second.setRidNext( ret.first );
               ret.first->second.setRidPre( pre->second ) ;
               ret.first->second.setRidNext( _tree.end() ) ;
               _ridTree[keyNode.getRID()] = ret.first ;

#if SDB_INTERNAL_DEBUG
               PD_LOG( PDDEBUG,
                       "Added new rid[%d, %d] version(%s) to rid tree[%d]",
                       keyNode.getRID()._extent, keyNode.getRID()._offset,
                       dpsTransIDToString(keyNode.getNodeTransID()).c_str(),
                       _idxLID ) ;
#endif
            }
         }
         catch ( std::exception &e )
         {
            if( !hasLock )
            {
               unlockX();
            }

            PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
            rc = SDB_OOM ;
            goto error ;
         }
      }


      if( !hasLock )
      {
         unlockX();
      }

      // Insert failed due to identical key(key+rid). This should not happen.
      // Instead of panic, let's return err and leave caller to handle
      if ( !ret.second )
      {
         rc = SDB_IXM_IDENTICAL_KEY ;

         PD_LOG( PDWARNING, "Trying to insert identical keys into the memory"
                 "tree, Key[%s], Value[%s], ConflictValue[%s]",
                 keyNode.toString().c_str(),
                 value.toString().c_str(),
                 tmpValue.toString().c_str() ) ;

         goto error ;
      }
#ifdef _DEBUG
      else
      {
         PD_LOG( PDDEBUG, "Inserted key[%s] to index tree(%d) with value[%s]",
                 keyNode.toString().c_str(), _idxLID,
                 value.toString().c_str() ) ;
      }
#endif

   done:
      PD_TRACE_EXITRC( SDB_PREIDXTREE_INSERT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // Create a node from key and rid, insert the node to map(tree)
   // Dependency:
   //   Caller has to held the tree latch in X
   INT32 preIdxTree::insert ( const BSONObj *keyData,
                              const dmsRecordID &rid,
                              const preIdxTreeNodeValue &value,
                              BOOLEAN hasLock,
                              const DPS_TRANS_ID &transID )
   {
      preIdxTreeNodeKey keyNode( keyData, rid, getOrdering(), transID ) ;
      return insert( keyNode, value, hasLock ) ;
   }

   INT32 preIdxTree::insertWithOldVer( const BSONObj *keyData,
                                       const dmsRecordID &rid,
                                       oldVersionContainer *oldVer,
                                       BOOLEAN hasLock,
                                       const DPS_TRANS_ID &transID,
                                       dmsTransLockCallback *callback,
                                       INT32 indexID )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( oldVer, "OldVer is NULL" ) ;
      SDB_ASSERT( rid == oldVer->getRecordID(), "Record ID is not the same" ) ;

      BOOLEAN isLocked = FALSE ;

      try
      {
         dpsIdxObj myIdxObj( *keyData, getLID() ) ;
         preIdxTreeNodeKey keyNode( &(myIdxObj.getKeyObj()), rid,
                                    getOrdering(), transID ) ;
         preIdxTreeNodeValue keyValue( oldVer ) ;
         INDEX_TREE_POS pos ;

         // when insert node, we need to add the RBS offset and pre/next
         // if mvccon
         if( pmdGetOptionCB()->mvccOn() )
         {
            SDB_ASSERT( callback, "Callback should not be NULL" ) ;
            keyValue.setRBSOffset( callback->getRBSRecordOffset() ) ;
         }

         if ( !hasLock )
         {
            lockX() ;
            isLocked = TRUE ;
         }

         // check if it exist. We might be able to save this check if all
         // callers did the check.
         pos = _tree.find( keyNode ) ;
         if ( isPosValid( pos ) )
         {
            if ( pos->second.getOldVer() != oldVer )
            {
               SDB_ASSERT( pos->second.isRecordDeleted(),
                           "The record must be Deleted" ) ;
               /// found deleted, should remove it
               _tree.erase( pos ) ;
            }
            else
            {
               /// already exist
               goto done ;
            }
         }

         SDB_ASSERT( oldVer->idxLidExist( getLID() ),
                     "LID is not exist" ) ;

         // insert to both idxset(oldVer) and idxTree
         if( oldVer->insertIdx( myIdxObj ) )
         {
            insert( keyNode, keyValue, TRUE ) ;
            if ( NULL != callback )
            {
               callback->setIndexUpdated( indexID ) ;
            }
         }
         else
         {
            rc = SDB_SYS ;
            SDB_ASSERT( SDB_OK == rc, "Index tree's node is inconsistency with "
                        "OldVer" ) ;
            goto error ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done :
      if ( isLocked )
      {
         unlockX() ;
      }
      return rc ;
   error :
      goto done ;
   }

   // Description:
   //   delete a node from map. Latch is held within the function
   // Input:
   //   keyNode: key to delete
   // Return:
   //   Number of node deleted from the map
   // PD_TRACE_DECLARE_FUNCTION ( SDB_PREIDXTREE_REMOVE, "preIdxTree::remove" )
   UINT32 preIdxTree::remove( const preIdxTreeNodeKey &keyNode,
                              const oldVersionContainer *pOldVer,
                              BOOLEAN hasLock )
   {
      PD_TRACE_ENTRY( SDB_PREIDXTREE_REMOVE ) ;

      INDEX_TREE_POS pos ;
      UINT32 numDeleted = 0 ;
      preIdxTreeNodeValue tmpValue ;


      SDB_ASSERT( keyNode.isValid(), "KeyNode is invalid" ) ;

      if ( !hasLock )
      {
         lockX() ;
      }

      pos = _tree.find( keyNode ) ;

      if ( pos != _tree.end() )
      {
         if ( !pOldVer || pOldVer == pos->second.getOldVer() )
         {
            tmpValue = pos->second ;
            ++numDeleted ;

            // if mvccOn  deal with _ridTree and _ridPre/_ridNext
            if ( pmdGetOptionCB()->mvccOn() )
            {
               _adjustRidChainForErase( pos ) ;
            }

            _tree.erase( pos ) ;
         }
      }

      if ( !hasLock )
      {
         unlockX() ;
      }

      if ( 1 != numDeleted )
      {
         if ( _isValid && !pOldVer )
         {
            PD_LOG( PDWARNING,
                    "Did not find records in index tree(%d) with key[%s]",
                    _idxLID,
                    keyNode.toString().c_str() ) ;
#ifdef _DEBUG
            printTree() ;
#endif
            SDB_ASSERT( ( 1 == numDeleted ),
                        "Delete record number must be 1" ) ;
         }
      }
      else
      {
         PD_LOG( PDDEBUG, "Has removed one record from index tree(%d), "
                 "Key[%s], Value[%s]", _idxLID, keyNode.toString().c_str(),
                 tmpValue.toString().c_str() ) ;
      }

      PD_TRACE_EXITRC( SDB_PREIDXTREE_REMOVE, numDeleted ) ;
      return numDeleted ;
   }

   UINT32 preIdxTree::remove( const BSONObj *keyData,
                              const dmsRecordID &rid,
                              const oldVersionContainer *pOldVer,
                              BOOLEAN hasLock )
   {
      // In MVCC, without the transID, we might delete unexpected version
      SDB_ASSERT( !pmdGetOptionCB()->mvccOn(),
                  "Should not use this interface when MVCC is enabled" ) ;
      preIdxTreeNodeKey keyNode( keyData, rid, getOrdering() ) ;
      return remove( keyNode, pOldVer, hasLock ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_PREIDXTREE_REMOVEFORRECORD, "preIdxTree::removeForRecord" )
   void preIdxTree::removeForRecord( SINT32 extID, SINT32 offset )
   {
      dmsRecordID              rid( extID, offset ) ;
      INDEX_TREE_POS           curPos, nextPos ;
      INDEX_RID_TREE::iterator it ;

      PD_TRACE_ENTRY( SDB_PREIDXTREE_REMOVEFORRECORD ) ;

      lockX() ;

      it = _ridTree.find( rid ) ;

      // check if rid exist in the _ridTree
      if ( it != _ridTree.end() )
      {
         // get the first(newest) tree nodes of the record
         curPos = it->second ;
         do
         {
            nextPos = curPos->second.getRidPre() ;
#ifdef _DEBUG
            PD_LOG( PDDEBUG,
                    "Erasing node in index tree(%d) with key[%s]",
                    _idxLID,
                    curPos->first.toString().c_str() ) ;
#endif
            _tree.erase( curPos ) ;
            curPos = nextPos ;
         }
         while ( curPos != _tree.end() ) ;
         // make sure rid is removed
         _ridTree.erase( rid ) ;
      }

      unlockX() ;
      PD_TRACE_EXIT( SDB_PREIDXTREE_REMOVEFORRECORD ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_PREIDXTREE_RESETVALUE, "preIdxTree::resetValue" )
   void preIdxTree::resetValue( const preIdxTreeNodeKey &keyNode,
                                DPS_TRANSID_SN           ownerTransID,
                                BOOLEAN                  hasLock )
   {
      PD_TRACE_ENTRY( SDB_PREIDXTREE_RESETVALUE ) ;

      INDEX_TREE_POS pos ;
      UINT32 numChanged = 0 ;

      SDB_ASSERT( keyNode.isValid(), "KeyNode is invalid" ) ;

      if ( !hasLock )
      {
         lockX() ;
      }

      pos = _tree.find( keyNode ) ;
      if ( pos != _tree.end() )
      {
         ++numChanged ;
         pos->second.reset() ;
      }

#ifdef _DEBUG
      if ( 1 != numChanged )
      {
         // this node could be GCed
         if ( _isValid && ( ownerTransID > _lastLowTranID ) )
         {
            PD_LOG( PDWARNING,
                    "Find %d records in index tree(%d) with key[%s], "
                    "ownerTransID(%llu), _lastLowTranID(%llu).\n",
                    numChanged, _idxLID,
                    keyNode.toString().c_str(),
                    ownerTransID, _lastLowTranID ) ;
            printTree() ;
            SDB_ASSERT( ( 1 == numChanged ),
                        "Change record number must be 1" ) ;
         }
      }
      else
      {
         PD_LOG( PDDEBUG, "Has reset one record from index tree(%d), "
                 "Key[%s], Value[%s]", _idxLID, keyNode.toString().c_str(),
                 pos->second.toString().c_str() ) ;
      }
#endif

      if ( !hasLock )
      {
         unlockX() ;
      }

      PD_TRACE_EXIT( SDB_PREIDXTREE_RESETVALUE ) ;
      return ;
   }


   // PD_TRACE_DECLARE_FUNCTION ( SDB_PREIDXTREE_ADVANCE, "preIdxTree::advance" )
   INT32 preIdxTree::advance( INDEX_TREE_CPOS &pos, INT32 direction ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_PREIDXTREE_ADVANCE ) ;

      while( TRUE )
      {
         if ( direction > 0 )
         {
            if ( pos == _tree.end() || ++pos == _tree.end() )
            {
               rc = SDB_IXM_EOC ;
               goto error ;
            }
         }
         else
         {
            INDEX_TREE_CRPOS rtempIter( pos ) ;
            if ( rtempIter == _tree.rend() )
            {
               rc = SDB_IXM_EOC ;
               goto error ;
            }
            pos = (++rtempIter).base() ;
         }

         // check if deleted. The record could have been marked deleted
         // but index haven't been physically removed from the tree due
         // to async release.
         // OR in MVCC, the in memory record could be deleted after written
         // to RBS, but the old version index is still in the tree which
         // the caller should evaluate and make decision on how to
         // use the record
         if ( !pos->second.isRecordDeleted() || pmdGetOptionCB()->mvccOn() )
         {
            break ;
         }
      }

   done:
      PD_TRACE_EXIT( SDB_PREIDXTREE_ADVANCE );
      return rc ;
   error:
      goto done ;
   }

   // Description:
   //   Locate the best matching location based on the passed in idxkey obj
   //   and search criteria
   // Input:
   //   keyObj+rid: indexkey objct (with rid) to look for
   //   direction: search direction
   // Output:
   //   pos: The iterator pointing to the best location.
   //   found: find the exact match or not
   // Return:
   //   SDB_OK:  normal return
   //   otherwise any popped error code
   // Dependency:
   //   caller must hold the tree latch in S
   // PD_TRACE_DECLARE_FUNCTION ( SDB_PREIDXTREE_LOCATE, "preIdxTree::locate" )
   INT32 preIdxTree::locate ( const BSONObj      &keyObj,
                              const dmsRecordID  &rid,
                              INDEX_TREE_CPOS    &pos,
                              BOOLEAN            &found,
                              INT32              direction ) const
   {
      PD_TRACE_ENTRY( SDB_PREIDXTREE_LOCATE ) ;
      INT32  rc = SDB_IXM_EOC ;
      preIdxTreeNodeKey myKey( &keyObj, rid, getOrdering() ) ;

      found = FALSE ;
      pos = _tree.end() ;

      // quick check for exact match
      INDEX_TREE_CPOS tempIter = _tree.find( myKey ) ;
      if( tempIter != _tree.end() )
      {
         pos = tempIter ;
         found = TRUE ;
         rc = SDB_OK ;
         goto done ;
      }

      tempIter = _tree.lower_bound( myKey ) ;
      if ( direction > 0 )
      {
         if ( tempIter != _tree.end() )
         {
            pos = tempIter ;
            rc = SDB_OK ;
            goto done ;
         }
      }
      else
      {
         INDEX_TREE_CRPOS rtempIter( tempIter ) ;
         if ( rtempIter != _tree.rend() )
         {
            pos = (++rtempIter).base() ;
            rc = SDB_OK ;
            goto done ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB_PREIDXTREE_LOCATE, rc ) ;
      return rc ;
   }

   BSONObj preIdxTree::_buildPredObj( const BSONObj &prevKey,
                                      INT32 keepFieldsNum,
                                      BOOLEAN skipToNext,
                                      const VEC_ELE_CMP &matchEle,
                                      const VEC_BOOLEAN &matchInclusive,
                                      INT32 direction ) const
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

   // Description:
   //   Locate the best matching key location based on the provided key value
   //   and search criteria. The key value is likely saved from the last cycle
   //   before the pause. Since the tree structure can be changed due to
   //   insertion and deletion, we don't bother to try and verify the saved
   //   iterator. Directly use the key value to find the best match key and
   //   start the new round from there.
   // Input:
   //   prevKey: previous indexkey to start with the search
   //   keepFieldsNum & skipToNext:
   //   keepFieldsNum is the number of fields from prevKey that should match
   //   the currentKey (for example if the prevKey is {c1:1, c2:1}, and
   //   keepFieldsNum = 1, that means we want to match c1:1 key for the
   //   current location. Depends on if we have skipToNext set, if we do
   //   that it means we want to skip c1:1 and match whatever the next
   //   (for example c1:1.1); otherwise we want to continue match the
   //   elements from matchEle
   //   matchEle matchInclusive: push down matching criteria from access plan
   //   o: key order information
   //   direction: search direction
   // Output:
   //   iter: The iterator pointing to the best location.
   // Return:
   //   SDB_IXM_EOC: end of index search
   //   Any error coming from the function
   // Dependency:
   //   Caller must hold tree latch in S/X
   // PD_TRACE_DECLARE_FUNCTION ( SDB_PREIDXTREE_KEYLOCATE, "preIdxTree::keyLocate" )
   INT32 preIdxTree::keyLocate( INDEX_TREE_CPOS &pos,
                                const BSONObj &prevKey,
                                INT32 keepFieldsNum,
                                BOOLEAN skipToNext,
                                const VEC_ELE_CMP &matchEle,
                                const VEC_BOOLEAN &matchInclusive,
                                INT32 direction ) const
   {
      PD_TRACE_ENTRY( SDB_PREIDXTREE_KEYLOCATE ) ;

      INT32               rc           = SDB_OK ;
      BOOLEAN             found        = FALSE ;
      INT32               result       = 0 ;
      BSONObj             data ;
      BSONObj             locateObj ;
      dmsRecordID dummyRid ;

      if ( empty() )
      {
         pos = _tree.end() ;
         rc = SDB_IXM_EOC ;
         goto done;
      }

      if ( direction > 0 )
      {
         dummyRid.resetMin() ;
      }
      else
      {
         dummyRid.resetMax() ;
      }

      try
      {
         locateObj = _buildPredObj( prevKey, keepFieldsNum,
                                    skipToNext, matchEle,
                                    matchInclusive, direction ) ;

         rc = locate( locateObj, dummyRid, pos, found, direction ) ;
         if ( rc )
         {
            goto error ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDWARNING, "Build pred object failed: %s", e.what() ) ;

         pos = direction > 0 ? _tree.begin() : --( _tree.end() ) ;
      }

      data = getNodeKey( pos ).getKeyObj() ;
      while ( TRUE )
      {
         result = _ixmExtent::_keyCmp( data, prevKey, keepFieldsNum,
                                       skipToNext, matchEle,
                                       matchInclusive,
                                       *(this->getOrdering()),
                                       direction ) ;
         if ( result * direction >= 0 )
         {
            break ;
         }

         rc = advance( pos, direction ) ;
         if ( rc )
         {
            goto error ;
         }
         data = getNodeKey( pos ).getKeyObj() ;
      }

   done :
      PD_TRACE_EXITRC( SDB_PREIDXTREE_KEYLOCATE, rc ) ;
      return rc;
   error :
      goto done ;
   }

   // Description:
   //   After the listIterators have changed, we will advance the key to the
   //   best matching key location based on the provided cur location, prevkey
   //   and search criteria. The key value is likely saved from the last cycle
   //   before the pause. Since the tree structure can be changed due to
   //   insertion and deletion, we don't bother to try and verify the saved
   //   iterator. Directly use the key value to find the best match key and
   //   start the new round from there.
   // Input:
   //   iter: The iterator pointing to the current searching location.
   //   prevKey: previous indexkey to start with the search
   //   keepFieldsNum & skipToNext:
   //   keepFieldsNum is the number of fields from prevKey that should match
   //   the currentKey (for example if the prevKey is {c1:1, c2:1}, and
   //   keepFieldsNum = 1, that means we want to match c1:1 key for the
   //   current location. Depends on if we have skipToNext set, if we do
   //   that it means we want to skip c1:1 and match whatever the next
   //   (for example c1:1.1); otherwise we want to continue match the
   //   elements from matchEle
   //   matchEle matchInclusive: push down matching criteria from access plan
   //   direction: search direction
   // Output:
   //   iter: The iterator pointing to the best location.
   //   prevKey:
   // Return:
   //   SDB_IXM_EOC: end of index search
   //   Any error coming from the function
   // Dependency:
   //   Caller must hold tree latch, the iter must not be tree end
   // PD_TRACE_DECLARE_FUNCTION ( SDB_PREIDXTREE_KEYADVANCE, "preIdxTree::keyAdvance" )
   INT32 preIdxTree::keyAdvance( INDEX_TREE_CPOS &pos,
                                 const BSONObj &prevKey,
                                 INT32 keepFieldsNum, BOOLEAN skipToNext,
                                 const VEC_ELE_CMP &matchEle,
                                 const VEC_BOOLEAN &matchInclusive,
                                 INT32 direction ) const
   {
      return keyLocate( pos, prevKey, keepFieldsNum, skipToNext,
                        matchEle, matchInclusive, direction ) ;
   }

   // Traverse the tree to see if the key exist, caller need to hold
   // the latch otherwise the iterator can change underneath
   // PD_TRACE_DECLARE_FUNCTION ( SDB_PREIDXTREE_ISKEYEXIST, "preIdxTree::isKeyExist" )
   BOOLEAN preIdxTree::isKeyExist( const BSONObj &key,
                                   preIdxTreeNodeValue &value ) const
   {
      PD_TRACE_ENTRY( SDB_PREIDXTREE_ISKEYEXIST );
      BOOLEAN found = FALSE ;

      if ( !_tree.empty() )
      {
         dmsRecordID rid ;
         rid.resetMin() ;
         preIdxTreeNodeKey lowKey( &key, rid, getOrdering() ) ;
         rid.resetMax();
         preIdxTreeNodeKey highKey( &key, rid, getOrdering() ) ;

         INDEX_TREE_CPOS startIter ;
         INDEX_TREE_CPOS endIter ;

         startIter = _tree.lower_bound( lowKey ) ;
         endIter = _tree.upper_bound( highKey ) ;

         while ( startIter != _tree.end() && startIter != endIter )
         {
            if ( !startIter->second.isValid() ||
                 startIter->second.isRecordDeleted() )
            {
               ++startIter ;
            }
            else
            {
               found = TRUE ;
               value = startIter->second ;
               break ;
            }
         }
      }

      PD_TRACE1( SDB_PREIDXTREE_ISKEYEXIST, PD_PACK_UINT( found ) ) ;
      PD_TRACE_EXIT( SDB_PREIDXTREE_ISKEYEXIST );
      return found ;
   }

   // Run garbage collection on a tree, erase all nodes older than lowtran.
   // PD_TRACE_DECLARE_FUNCTION ( SDB_PREIDXTREE_GC, "preIdxTree::gc" )
   DPS_TRANSID_SN preIdxTree::gc( DPS_TRANSID_SN lowTran )
   {
      INDEX_TREE_POS pos ;

      // Lowest transID in the tree
      DPS_TRANSID_SN idxTreeLowTran = DPS_MAX_TRANSID_SN ;

      PD_TRACE_ENTRY( SDB_PREIDXTREE_GC );
#ifdef _DEBUG
      PD_LOG ( PDDEBUG,
               "gc memixtree(%d) to lowTran %llu), lastGCtime(%llu)",
               _idxLID, lowTran, _lastLowTranID );
#endif
      SDB_ASSERT( pmdGetOptionCB()->mvccOn(),
                  "should only be called if mvcc is enabled." ) ;
      lockX();

      // Only gc if lowtran moved up
      if ( lowTran > _lastLowTranID )
      {
         _preSize.poke( _tree.size() ) ;

         pos = _tree.begin() ;

         // go through each node and check its transID, remove the node
         // if it's oldver than lowTran
         while ( pos != _tree.end() )
         {
             // Note that this lowtran passed in should has expired version
             // and max error considered. so we can do the simple sn
             // comparison with global transaction tag
            if ( pos->first.getNodeTransID().getGlobSN() < lowTran )
            {
               // only remove the node if old version container
               // pointer is NULL, otherwise, the thread running
               // releaseRecord() may encounter assertion failure
               // when it removes this node.
               if ( FALSE == pos->second.isValid() )
               {
                  INDEX_TREE_POS temp = pos ;
                  //preIdxTreeNodeValue tmpValue = pos->second ;
#ifdef _DEBUG
                  PD_LOG ( PDDEBUG,
                           "Remove node(%s) from ixtree(%d),lowTran(%llu)",
                           pos->first.toString().c_str(), _idxLID,
                           lowTran );
#endif
                  _adjustRidChainForErase( pos ) ;
                  pos++ ;
                  _tree.erase(temp) ;
               }
               else
               {
                  // skip the node if old version container is still there
                  pos++ ;
               }
            }
            else
            {
               // Node's transID is greater than lowTran, now lets compute
               // the lowest transID above lowTran for this tree
               if ( idxTreeLowTran >
                    pos->first.getNodeTransID().getGlobSN() )
               {
                  idxTreeLowTran = pos->first.getNodeTransID().getGlobSN() ;
               }

               pos++ ;
            }
         }

         // All the nodes are older than lowTran
         if ( idxTreeLowTran == DPS_MAX_TRANSID_SN )
         {
            idxTreeLowTran = lowTran ;
         }
         _lastLowTranID = idxTreeLowTran ;
      }

      if ( idxTreeLowTran == DPS_MAX_TRANSID_SN )
      {
         idxTreeLowTran = _lastLowTranID ;
      }

      unlockX() ;

      PD_TRACE_EXIT( SDB_PREIDXTREE_GC );
      return idxTreeLowTran ;
   }

   // adjust pre and next node in the rid chain when removing a node from
   // the in memory index tree. tree latch must be held in X
   void preIdxTree::_adjustRidChainForErase( INDEX_TREE_POS & pos )
   {
      preIdxTreeNodeValue tmpValue = pos->second ;
      dmsRecordID         rid = pos->first.getRID() ;

      SDB_ASSERT( ( _ridTree.find(rid) != _ridTree.end() ),
                    "Ridtree node is not valid." ) ;
      // this is the last version of index for this rid
      if ( tmpValue.getRidPre() == _tree.end() &&
           tmpValue.getRidNext() == _tree.end() )
      {
#ifdef _DEBUG
         PD_LOG ( PDDEBUG, "Removing rid(%d, %d) from ridtree(%d)",
                  rid._extent, rid._offset, _idxLID ) ;
#endif
         SDB_ASSERT( ( _ridTree[rid] == pos ),
                     "Ridtree node is not valid." ) ;
         _ridTree.erase( rid ) ;
      }
      else
      {
         // removing the first node(newest version) in rid chain.
         // this can happen if all the old versions of this rid
         // are good candidates for GC, but the newest version has the
         // "front" order in idx tree. So it is cleaned up first.
         if ( _ridTree[rid] == pos )
         {
#ifdef _DEBUG
            PD_LOG ( PDDEBUG,
                     "Remove first rid(%d, %d) from ridtree(%d)",
                     rid._extent, rid._offset, _idxLID ) ;
#endif
            _ridTree[rid] = tmpValue.getRidPre() ;
         }

         if ( tmpValue.getRidPre() != _tree.end() )
         {
            tmpValue.getRidPre()->
               second.setRidNext( tmpValue.getRidNext() ) ;
         }
         if ( tmpValue.getRidNext() != _tree.end() )
         {
            tmpValue.getRidNext()->
               second.setRidPre( tmpValue.getRidPre() ) ;
         }
      }

   }

   // search _ridTree, find the first version of keynode in memtree
   // for the given rid. Caller should hold the tree latch
   INDEX_TREE_POS preIdxTree::getKeyNodeFromRidTree( dmsRecordID rid )
   {
      INDEX_TREE_POS pos = _tree.end() ;
      INDEX_RID_TREE::iterator it = _ridTree.find( rid ) ;
      if ( it != _ridTree.end() )
      {
         pos = it->second ;
      }

      return pos ;
   }

   void preIdxTree::printTree( BOOLEAN detailed ) const
   {
      const UINT32 maxOnceCount = 20 ;
      UINT32 index = 0 ;
      INDEX_TREE_CPOS pos ;

      while( TRUE )
      {
         std::stringstream ss ;
         UINT32 count = 0 ;
         if ( 0 == index )
         {
            ss << "==> Index tree[Key: " << _keyPattern.toString()
               << ", LID:" << _idxLID
               << ", lastLowTranID:" << _lastLowTranID
               << ", Size:" << _tree.size()
               << ", preSize:" << getPreSize()
               << ", SizeHWM:" << getSizeHWM()
               << " nodes:" << std::endl ;
            // only print each node if asked for detailed info
            if ( detailed )
            {
               pos = _tree.begin() ;
            }
            else
            {
               pos = _tree.end() ;
            }
         }

         while ( pos != _tree.end() )
         {
            ss << ++index << " Key: " << pos->first.toString()
               << ", Value: " << pos->second.toString()
               << std::endl ;
            ++pos ;
            ++count ;

            if ( count >= maxOnceCount )
            {
               break ;
            }
         }

         if ( pos == _tree.end() )
         {
            ss << "<== End index tree" ;
            PD_LOG( PDEVENT, ss.str().c_str() ) ;
            break ;
         }
         else
         {
            PD_LOG( PDEVENT, ss.str().c_str() ) ;
         }
      }
   }

   /*
      oldVersionUnit implement
   */
   oldVersionUnit::iterator& oldVersionUnit::iterator::operator= ( const oldVersionUnit::iterator &rhs )
   {
      release() ;

      _pUnit = rhs._pUnit ;
      _cur = rhs._cur ;
      _init = rhs._init ;
      _stepCnt = rhs._stepCnt ;
      _getCnt = rhs._getCnt ;
      _interval = rhs._interval ;

      if ( rhs._locked )
      {
         _pUnit->lockS() ;
         _locked = TRUE ;
      }

      if ( rhs._lockOnChain )
      {
         _cur->lockOnChain() ;
         _lockOnChain = TRUE ;
      }

      return *this ;
   }

   void oldVersionUnit::iterator::release()
   {
      if ( _lockOnChain )
      {
         _cur->unlockOnChain() ;
         _pUnit->_event.signalAll() ;
         _lockOnChain = FALSE ;
      }

      if ( _locked )
      {
         _pUnit->unlockS() ;
         _locked = FALSE ;
      }

      _pUnit = NULL ;
      _cur = NULL ;
   }

   oldVersionContainer* oldVersionUnit::iterator::next()
   {
      if ( _pUnit )
      {
         ++_getCnt ;

         if ( !_locked )
         {
            resume() ;
         }
         else if ( _stepCnt > 0 && _getCnt % _stepCnt == 0 )
         {
            pause() ;
            ossSleep( _interval ) ;
            resume() ;
         }

         if ( !_init )
         {
            _cur = _pUnit->_pChain ;
            _init = TRUE ;
         }
         else if ( _cur )
         {
            _cur = _cur->getNext() ;
         }

         if ( !_cur )
         {
            pause() ;
         }
      }
      return _cur ;
   }

   void oldVersionUnit::iterator::pause()
   {
      if ( _pUnit && _locked )
      {
         if ( _cur && !_lockOnChain )
         {
            _cur->lockOnChain() ;
            _pUnit->_event.reset() ;
            _lockOnChain = TRUE ;
         }
         _pUnit->unlockS() ;
         _locked = FALSE ;
      }
   }

   void oldVersionUnit::iterator::resume()
   {
      if ( _pUnit && !_locked )
      {
         _pUnit->lockS() ;
         _locked = TRUE ;

         if ( _lockOnChain )
         {
            _cur->unlockOnChain() ;
            _pUnit->_event.signalAll() ;
            _lockOnChain = FALSE ;
         }
      }
   }

   oldVersionUnit::oldVersionUnit()
   {
      _pChain = NULL ;
   }

   oldVersionUnit::~oldVersionUnit()
   {
      clearChain() ;
   }

   void oldVersionUnit::addToChain( oldVersionContainer *pOldVer,
                                    BOOLEAN hasLock )
   {
      BOOLEAN locked = FALSE ;

      if ( !hasLock )
      {
         lockX() ;
         locked = TRUE ;
      }

      pOldVer->setPrev( NULL ) ;
      pOldVer->setNext( _pChain ) ;
      if ( _pChain )
      {
         _pChain->setPrev( pOldVer ) ;
      }
      _pChain = pOldVer ;
      _pChain->setOnChain() ;

      if ( locked )
      {
         unlockX() ;
         locked = FALSE ;
      }
   }

   void oldVersionUnit::removeFromChain( oldVersionContainer *pOldVer,
                                         BOOLEAN hasLock )
   {
      BOOLEAN locked = FALSE ;

      if ( !hasLock )
      {
         lockX() ;
         locked = TRUE ;
      }

   retry:
      if ( pOldVer->isOnChain() )
      {
         oldVersionContainer *prev = pOldVer->getPrev() ;
         oldVersionContainer *next = pOldVer->getNext() ;

         if ( pOldVer->isLockOnChain() )
         {
            unlockX() ;
            while( pOldVer->isLockOnChain() )
            {
               _event.wait( OSS_ONE_SEC ) ;
            }
            lockX() ;
            goto retry ;
         }

         // prev is not NULL, it could be not the head of a connected chain
         if ( prev )
         {
            prev->setNext( next ) ;
            if ( next )
            {
               next->setPrev( prev ) ;
            }
         }
         else
         {
            SDB_ASSERT( _pChain == pOldVer, "Not the same" ) ;
            _pChain = next ;

            if ( next )
            {
               next->setPrev( NULL );
            }
         }

         pOldVer->setPrev( NULL ) ;
         pOldVer->setNext( NULL ) ;
         pOldVer->unsetOnChain() ;
      }

      if ( locked )
      {
         unlockX() ;
         locked = FALSE ;
      }
   }

   void oldVersionUnit::clearChain( BOOLEAN hasLock )
   {
      BOOLEAN locked = FALSE ;

      if ( !hasLock )
      {
         lockX() ;
         locked = TRUE ;
      }

      oldVersionContainer *oldVer = _pChain ;
      while ( NULL != oldVer )
      {
         _pChain = oldVer->getNext() ;

         oldVer->setPrev( NULL ) ;
         oldVer->setNext( NULL ) ;
         oldVer->unsetOnChain() ;

         oldVer = _pChain ;
      }

      if ( locked )
      {
         unlockX() ;
         locked = FALSE ;
      }
   }

   oldVersionUnit::iterator oldVersionUnit::itr( INT64 stepCnt, INT32 interval )
   {
      iterator it( this, stepCnt, interval ) ;
      return it ;
   }

   /*
      oldVersionCB implement
   */
   oldVersionCB::oldVersionCB()
      : _minTransIDSN( DPS_INVALID_TRANSID_SN ),
      _treeSizeHWM(0)
   {
   }

   oldVersionCB::~oldVersionCB()
   {
      fini() ;
   }

   INT32 oldVersionCB::init()
   {
      return SDB_OK ;
   }

   void oldVersionCB::fini()
   {
      /// check tree nodes
      IDXID_TO_TREE_MAP_IT it = _idxTrees.begin() ;
      while( it != _idxTrees.end() )
      {

         SDB_ASSERT( pmdGetOptionCB()->mvccOn() || it->second->empty(),
                     "Index tree should be empty" ) ;
         if ( !it->second->empty() )
         {
            PD_LOG( PDDEBUG, "clean up index tree[%s]",
                    it->first.toString().c_str() ) ;
            it->second.get()->clear() ;
         }
         ++it ;
      }
      _idxTrees.clear() ;

      /// check old version unit map
      MAP_OLDVERION_UNIT_IT itUnit = _mapOldVersionUnit.begin() ;
      while ( itUnit != _mapOldVersionUnit.end() )
      {
         SDB_ASSERT( itUnit->second->empty(),
                     "Old version unit should be empty" ) ;
         ++itUnit ;
      }
      _mapOldVersionUnit.clear() ;
   }

   // Create an in memory index tree and add to the map
   // PD_TRACE_DECLARE_FUNCTION ( SDB_OLDVERSIONCB_ADDIDXTREE, "oldVersionCB::addIdxTree" )
   INT32 oldVersionCB::addIdxTree( const globIdxID &gid,
                                   const ixmIndexCB *indexCB,
                                   preIdxTreePtr &treePtr,
                                   BOOLEAN hasLock )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_OLDVERSIONCB_ADDIDXTREE ) ;

      preIdxTreePtr tmpTreePtr ;
      pair<IDXID_TO_TREE_MAP_IT, BOOLEAN> ret ;

      tmpTreePtr = preIdxTreePtr::allocRaw( __FILE__, __LINE__, ALLOC_OSS ) ;
      if ( !tmpTreePtr.get() )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      new ( (void*)tmpTreePtr.get() ) preIdxTree( gid._idxLID, indexCB ) ;

      if ( !tmpTreePtr->isValid() )
      {
         rc = SDB_OOM ;
         goto error ;
      }

      if ( !hasLock )
      {
         latchX() ;
      }

      try
      {
         ret = _idxTrees.insert( IDXID_TO_TREE_MAP_PAIR( gid, tmpTreePtr ) ) ;
      }
      catch ( exception &e )
      {
         if ( !hasLock )
         {
            releaseX() ;
         }
         PD_LOG( PDERROR, "Failed to add index tree, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

      if ( !hasLock )
      {
         releaseX() ;
      }

      if ( ret.second )
      {
         PD_LOG( PDDEBUG, "Create index tree[%s], Key:%s",
                 gid.toString().c_str(),
                 indexCB->keyPattern().toString().c_str() ) ;
      }
      else
      {
         treePtr = ret.first->second ;
         rc = SDB_IXM_EXIST ;
         goto error ;
      }

      treePtr = ret.first->second ;

   done:
      PD_TRACE_EXIT( SDB_OLDVERSIONCB_ADDIDXTREE ) ;
      return rc ;
   error:
      goto done ;
   }

   // based on global logic index id, get the in memory idx tree
   preIdxTreePtr oldVersionCB::getIdxTree( const globIdxID &gid,
                                           BOOLEAN hasLock )
   {
      preIdxTreePtr treePtr ;
      IDXID_TO_TREE_MAP_IT it ;

      if ( !hasLock )
      {
         latchS() ;
      }

      it = _idxTrees.find( gid ) ;

      if ( it != _idxTrees.end() )
      {
         treePtr = it->second ;
      }

      if ( !hasLock )
      {
         releaseS() ;
      }

      return treePtr ;
   }

   INT32 oldVersionCB::getOrCreateIdxTree( const globIdxID &gid,
                                           const ixmIndexCB *indexCB,
                                           preIdxTreePtr &treePtr,
                                           BOOLEAN hasLock )
   {
      INT32 rc = SDB_OK ;

      treePtr = getIdxTree( gid, hasLock ) ;
      if ( treePtr.get() )
      {
         goto done ;
      }

      rc = addIdxTree( gid, indexCB, treePtr, hasLock ) ;
      if ( SDB_IXM_EXIST == rc )
      {
         rc = SDB_OK ;
      }
      else if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // Delete an in memory index tree and remove it from the map
   // PD_TRACE_DECLARE_FUNCTION ( SDB_OLDVERSIONCB_DELIDXTREE, "oldVersionCB::delIdxTree" )
   void oldVersionCB::delIdxTree( const globIdxID &gid, BOOLEAN hasLock )
   {
      preIdxTreePtr treePtr ;
      IDXID_TO_TREE_MAP_IT it ;

      PD_TRACE_ENTRY( SDB_OLDVERSIONCB_DELIDXTREE );

#if SDB_INTERNAL_DEBUG
      PD_LOG( PDDEBUG, "Going to delete in memory Index tree for (%d,%d,%d)",
              gid._csID,
              gid._clID,
              gid._idxLID );
#endif
      if ( !hasLock )
      {
         latchX() ;
      }

      it = _idxTrees.find( gid ) ;
      if ( it != _idxTrees.end() )
      {
         treePtr = it->second ;
         _idxTrees.erase( it ) ;
      }

      if ( !hasLock )
      {
         releaseX() ;
      }

      if ( treePtr.get() )
      {
         PD_LOG( PDDEBUG, "Has removed index tree[%s], Key:%s",
                 gid.toString().c_str(),
                 treePtr->getKeyPattern().toString().c_str() ) ;
         treePtr->clear() ;
         treePtr->setDeleted() ;
      }

      PD_TRACE_EXIT ( SDB_OLDVERSIONCB_DELIDXTREE ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OLDVERSIONCB_GCIDXTREES, "oldVersionCB::gcIdxTrees" )
   void oldVersionCB::gcIdxTrees( )
   {
      PD_TRACE_ENTRY ( SDB_OLDVERSIONCB_GCIDXTREES ) ;
      preIdxTreePtr treePtr ;
      IDXID_TO_TREE_MAP_IT it ;
      DPS_TRANSID_SN  expiredVersion = DPS_INVALID_TRANSID_SN ;
      DPS_TRANSID_SN  minTreeLowTran = DPS_MAX_TRANSID_SN ;

      latchS() ;
      it = _idxTrees.begin() ;

      // loop through trees
      while ( it != _idxTrees.end() )
      {
         // handle one tree
         // Will release the latch of old version container, save the global
         // index ID and tree pointer before we release the latch.
         globIdxID curGIID = it->first ;
         treePtr = it->second ;

         releaseS() ;

         // get current expired version for each tree. Max time error is
         // already considered
         expiredVersion = sdbGetTransCB()->getExpiredVersion() ;

         if ( treePtr.get() &&
              DPS_INVALID_TRANSID_SN != expiredVersion )
         {
            DPS_TRANSID_SN treeLowTran ;
#ifdef _DEBUG
            PD_LOG( PDDEBUG, "gc index tree[%s], Key:%s, expired version[%s]",
                    it->first.toString().c_str(),
                    treePtr->getKeyPattern().toString().c_str(),
                    dpsTransSNToString( expiredVersion ).c_str() ) ;
#endif
            // NOTE: we need global transaction tag with SN
            treeLowTran = treePtr->gc( expiredVersion ) ;

            if ( treeLowTran < minTreeLowTran )
            {
               minTreeLowTran = treeLowTran ;
            }
            _treeSizeHWM.swapGreaterThan( treePtr->getSizeHWM() ) ;

         }

         latchS() ;
         // We have released latch of oldVerContainer after we got
         // the tree pointer from this iterator. But during we gc the current
         // tree, the iterator could be erased by drop index, so the iterator
         // will not be safe to move to next. We need to find the next
         // tree by upper bound of the current global index ID
         it = _idxTrees.upper_bound( curGIID ) ;
      }

      if ( minTreeLowTran != DPS_MAX_TRANSID_SN )
      {
         updateMinLowTranSN( minTreeLowTran ) ;
      }

      releaseS() ;

      PD_TRACE_EXIT ( SDB_OLDVERSIONCB_GCIDXTREES ) ;
   }

   void oldVersionCB::clearIdxTreeByCSID( UINT32 csID, BOOLEAN hasLock )
   {
      preIdxTreePtr treePtr ;
      IDXID_TO_TREE_MAP mapTmpTree ;
      IDXID_TO_TREE_MAP_IT it ;

      if ( !hasLock )
      {
         latchX() ;
      }

      it = _idxTrees.begin() ;
      while ( it != _idxTrees.end() )
      {
         if ( it->first._csID == csID )
         {
            treePtr = it->second ;

            if ( treePtr.get() )
            {
               PD_LOG( PDDEBUG, "Has removed index tree[%s], Key:%s",
                       it->first.toString().c_str(),
                       treePtr->getKeyPattern().toString().c_str() ) ;

               treePtr->setDeleted() ;
               try
               {
                  mapTmpTree[ it->first ] = it->second ;
               }
               catch( std::exception &e )
               {
                  PD_LOG( PDWARNING, "Occur exception: %s", e.what() ) ;
                  treePtr->clear() ;
               }
            }
            _idxTrees.erase( it++ ) ;
            continue ;
         }
         ++it ;
      }

      if ( !hasLock )
      {
         releaseX() ;
      }

      /// clear trees's node out of latch mutex
      it = mapTmpTree.begin() ;
      while( it != mapTmpTree.end() )
      {
         it->second->clear() ;
         ++it ;
      }
      mapTmpTree.clear() ;
   }

   void oldVersionCB::clearIdxTreeByCLID( UINT32 csID,
                                          UINT16 clID,
                                          BOOLEAN hasLock )
   {
      preIdxTreePtr treePtr ;
      IDXID_TO_TREE_MAP mapTmpTree ;
      IDXID_TO_TREE_MAP_IT it ;

      if ( !hasLock )
      {
         latchX() ;
      }

      it = _idxTrees.begin() ;
      while ( it != _idxTrees.end() )
      {
         if ( it->first._csID == csID &&
              it->first._clID == clID )
         {
            treePtr = it->second ;
            PD_LOG( PDDEBUG, "Removing index tree[%s], Key:%s",
                    it->first.toString().c_str(),
                    treePtr->getKeyPattern().toString().c_str() ) ;

            if ( treePtr.get() )
            {
               treePtr->setDeleted() ;
               try
               {
                  mapTmpTree[ it->first ] = it->second ;
               }
               catch( std::exception &e )
               {
                  PD_LOG( PDWARNING, "Occur exception: %s", e.what() ) ;
                  treePtr->clear() ;
               }
            }
            _idxTrees.erase( it++ ) ;
            continue ;
         }
         ++it ;
      }

      if ( !hasLock )
      {
         releaseX() ;
      }

      // clear trees's node out of latch mutex, it's safe to do so because
      // scanner would have found the tree invalid after having the lock again
      it = mapTmpTree.begin() ;
      while( it != mapTmpTree.end() )
      {
         it->second->clear() ;
         ++it ;
      }
      mapTmpTree.clear() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OLDVERSIONCB_ADDOLDVERSIONUNIT, "oldVersionCB::addOldVersionUnit" )
   INT32 oldVersionCB::addOldVersionUnit( UINT32 csID,
                                          UINT32 clID,
                                          oldVersionUnitPtr &unitPtr,
                                          BOOLEAN hasLock )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_OLDVERSIONCB_ADDOLDVERSIONUNIT ) ;

      oldVersionUnitPtr tmpUnitPtr ;
      pair<MAP_OLDVERION_UNIT_IT, BOOLEAN> ret ;
      UINT64 keyID = ossPack32To64( csID, clID ) ;

      tmpUnitPtr = oldVersionUnitPtr::alloc( __FILE__, __LINE__, ALLOC_OSS ) ;
      if ( !tmpUnitPtr.get() )
      {
         rc = SDB_OOM ;
         goto error ;
      }

      if ( !hasLock )
      {
         latchX() ;
      }

      try
      {
         ret = _mapOldVersionUnit.insert(
                              MAP_OLDVERION_UNIT_PAIR( keyID,
                                                       tmpUnitPtr ) ) ;
      }
      catch ( exception &e )
      {
         if ( !hasLock )
         {
            releaseX() ;
         }
         PD_LOG( PDERROR, "Failed to add old version unit, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

      if ( !hasLock )
      {
         releaseX() ;
      }

      if ( ret.second )
      {
         PD_LOG( PDDEBUG, "Create old version unit[CSID:%u, CLID:%u] succeed",
                 csID, clID ) ;
      }
      else
      {
         unitPtr = ret.first->second ;
         rc = SDB_DMS_EXIST ;
         goto error ;
      }

      unitPtr = ret.first->second ;

   done:
      PD_TRACE_EXIT( SDB_OLDVERSIONCB_ADDOLDVERSIONUNIT ) ;
      return rc ;
   error:
      goto done ;
   }

   oldVersionUnitPtr oldVersionCB::getOldVersionUnit( UINT32 csID,
                                                      UINT32 clID,
                                                      BOOLEAN hasLock )
   {
      oldVersionUnitPtr unitPtr ;
      MAP_OLDVERION_UNIT_IT it ;

      if ( !hasLock )
      {
         latchS() ;
      }

      it = _mapOldVersionUnit.find( ossPack32To64( csID, clID ) ) ;

      if ( it != _mapOldVersionUnit.end() )
      {
         unitPtr = it->second ;
      }

      if ( !hasLock )
      {
         releaseS() ;
      }

      return unitPtr ;
   }

   INT32 oldVersionCB::getOrCreateOldVersionUnit( UINT32 csID,
                                                  UINT32 clID,
                                                  oldVersionUnitPtr &unitPtr,
                                                  BOOLEAN hasLock )
   {
      INT32 rc = SDB_OK ;

      unitPtr = getOldVersionUnit( csID, clID, hasLock ) ;
      if ( unitPtr.get() )
      {
         goto done ;
      }

      rc = addOldVersionUnit( csID, clID, unitPtr, hasLock ) ;
      if ( SDB_DMS_EXIST == rc )
      {
         rc = SDB_OK ;
      }
      else if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OLDVERSIONCB_DELOLDVERSIONUNIT, "oldVersionCB::delOldVersionUnit" )
   void oldVersionCB::delOldVersionUnit( UINT32 csID,
                                         UINT32 clID,
                                         BOOLEAN hasLock )
   {
      oldVersionUnitPtr unitPtr ;
      MAP_OLDVERION_UNIT_IT it ;

      PD_TRACE_ENTRY( SDB_OLDVERSIONCB_DELOLDVERSIONUNIT );

      if ( !hasLock )
      {
         latchX() ;
      }

      it = _mapOldVersionUnit.find( ossPack32To64( csID, clID ) ) ;
      if ( it != _mapOldVersionUnit.end() )
      {
         unitPtr = it->second ;
         _mapOldVersionUnit.erase( it ) ;
      }

      if ( !hasLock )
      {
         releaseX() ;
      }

      if ( unitPtr.get() )
      {
         unitPtr->clearChain() ;
         PD_LOG( PDDEBUG, "Has removed old version unit[CSID:%u, CLID:%u]",
                 csID, clID ) ;
      }

      PD_TRACE_EXIT ( SDB_OLDVERSIONCB_DELOLDVERSIONUNIT ) ;
   }

   void oldVersionCB::clearOldVersionUnitByCS( UINT32 csID, BOOLEAN hasLock )
   {
      MAP_OLDVERION_UNIT tmpMapUnit ;
      MAP_OLDVERION_UNIT_IT it ;

      UINT32 keyCSID = ~0 ;
      UINT32 keyCLID = ~0 ;

      if ( !hasLock )
      {
         latchX() ;
      }

      it = _mapOldVersionUnit.begin() ;
      while ( it != _mapOldVersionUnit.end() )
      {
         ossUnpack32From64( it->first, keyCSID, keyCLID ) ;
         if ( keyCSID == csID )
         {
            try
            {
               tmpMapUnit[ it->first ] = it->second ;
            }
            catch( std::exception &e )
            {
               PD_LOG( PDWARNING, "Occur exception: %s", e.what() ) ;
               /// ignore
            }

            PD_LOG( PDDEBUG, "Has removed old version unit[CSID:%u, CLID:%u]",
                    keyCSID, keyCLID ) ;

            _mapOldVersionUnit.erase( it++ ) ;
            continue ;
         }
         ++it ;
      }

      if ( !hasLock )
      {
         releaseX() ;
      }

      /// clear old version unit's chain out of latch mutex
      tmpMapUnit.clear() ;
   }

   // This function will go through each index tree for the given collection,
   // remove all nodes for the given record by following the ridTree chain.
   // Proper latch are taken within this function. The caller suppose to hold
   // the record lock of the provided record. Currently mblatch of the
   // collection is also held.
   // PD_TRACE_DECLARE_FUNCTION ( SDB_OLDVERSIONCB_CLEANIDXNODESFORRECORD, "oldVersionCB::cleanIdxNodesForRecord" )
   void oldVersionCB::cleanIdxNodesForRecord( UINT32 csID,
                                              UINT16 clID,
                                              SINT32 extID,
                                              SINT32 offset )
   {
      PD_TRACE_ENTRY ( SDB_OLDVERSIONCB_CLEANIDXNODESFORRECORD ) ;
      preIdxTreePtr treePtr ;
      IDXID_TO_TREE_MAP_IT it ;
      globIdxID   gid(csID, clID, 0);

      latchS() ;
      // find the first index tree of this collection under the latch
      // _idxTrees is ordered by globIdxID, where csID/clID take privillage
      // over idxLid on comparison. And idxLid is in ascending order.
      it = _idxTrees.lower_bound(gid) ;

      while ( it != _idxTrees.end()     &&
              (it->first._csID == csID) &&
              (it->first._clID == clID) )
      {
         // this index can't be dropped as we hold the record lock
         // (and mblatch), we can do the work without latch
         releaseS() ;
         treePtr = it->second ;

         treePtr->removeForRecord( extID, offset );

         latchS() ;
         ++it ;
      }

      releaseS() ;
      PD_TRACE_EXIT ( SDB_OLDVERSIONCB_CLEANIDXNODESFORRECORD ) ;
   }

   /*
      oldVersionContainer implement
   */
   oldVersionContainer::oldVersionContainer( const dmsRecordID &rid,
                                             INT32 csID, UINT16 clID,
                                             UINT32 csLID, UINT32 clLID )
   :_csID( csID ), _clID( clID ), _csLID( csLID ), _clLID( clLID ),
    _rid( rid ), _recordTransID(), _ownerTransID()
   {
      _statMask      = 0 ;
      _ownerTID      = 0 ;
      _prev          = NULL ;
      _next          = NULL ;
      _isOnChain     = FALSE ;
      _lockCnt       = 0 ;
      //_refCount      = 0 ;
   }

   oldVersionContainer::~oldVersionContainer()
   {
      releaseRecord() ;
      //SDB_ASSERT( 0 == _refCount, "RefCount is invalid" ) ;
   }

   BOOLEAN oldVersionContainer::isRecordEmpty() const
   {
      if ( isRecordDummy() || isRecordNew() || _recordPtr.get() )
      {
         return FALSE ;
      }
      return TRUE ;
   }

   const dmsRecord* oldVersionContainer::getRecord() const
   {
      return ( const dmsRecord* )_recordPtr.get() ;
   }

   BSONObj oldVersionContainer::getRecordObj() const
   {
      const dmsRecord *pRecord = getRecord() ;
      if ( pRecord && !pRecord->isCompressed() )
      {
         return BSONObj( pRecord->getData() ) ;
      }
      return BSONObj() ;
   }

   INT32 oldVersionContainer::saveRecord( const dmsRecord *pRecord,
                                          const BSONObj    &obj,
                                          UINT32           ownerTID,
                                          DPS_TRANS_ID     ownerTransID )
   {
      INT32 rc = SDB_OK ;
      UINT32 recSize = 0 ;
      dmsRecord *pNewRecord = NULL ;
      DPS_TRANS_ID recordTransID ;

      SDB_ASSERT( !_recordPtr.get(), "Old record is not NULL" ) ;
      SDB_ASSERT( pRecord, "Record is NULL" ) ;

      if ( _recordPtr.get() )
      {
         goto done ;
      }

      if ( pRecord->hasGlobTransID() )
      {
         recordTransID = pRecord->getGlobTransID() ;
      }

      recSize = DMS_RECORD_METADATA_SZ + obj.objsize() ;
      _recordPtr = dpsOldRecordPtr::alloc( recSize, __FILE__, __LINE__,
                                           ALLOC_POOL ) ;
      if ( !_recordPtr.get() )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Alloc memory(%u) failed, rc: %d",
                 recSize, rc ) ;
         goto error ;
      }

      pNewRecord = ( dmsRecord* )_recordPtr.get() ;
      /// copy header
      ossMemcpy( _recordPtr.get(), (const void*)pRecord,
                 pRecord->hasGlobTransID() ? DMS_RECORD_METADATA_SZ :
                                             DMS_RECORD_V0_METADATA_SZ ) ;

      pNewRecord->unsetCompressed() ;
      pNewRecord->setSize( recSize ) ;
      pNewRecord->setHasGlobTransID() ;
      pNewRecord->setGlobTransID( recordTransID ) ;

      /// copy data
      ossMemcpy( _recordPtr.get() + DMS_RECORD_METADATA_SZ,
                 obj.objdata(), obj.objsize() ) ;

      _ownerTID = ownerTID ;
      _recordTransID = recordTransID ;
      _ownerTransID = ownerTransID ;

#ifdef _DEBUG
      PD_LOG ( PDDEBUG,
               "Thread(%d) Saved old copy for rid(%d, %d) to oldVer(%x) "
               "through transaction(%s), recordTransID(%s)",
               ownerTID, _rid._extent, _rid._offset,
               this, dpsTransIDToString( _ownerTransID ).c_str(),
               dpsTransIDToString( _recordTransID ).c_str() ) ;
#endif //_DEBUG

   done:
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN oldVersionContainer::isIndexObjEmpty() const
   {
      return _oldIdx.empty() ? TRUE : FALSE ;
   }

   void oldVersionContainer::_releaseRecord( preIdxTree *pTree,
                                             const preIdxTreeNodeKey &keyNode,
                                             BOOLEAN treeLatchHeld )
   {
      SDB_ASSERT( NULL != pTree, "tree is invalid" ) ;

      // remove the index from mem tree if mvcc is off or the
      // transaction had been rolledback
      if ( !pmdGetOptionCB()->mvccOn() || isRolledback() )
      {
#if SDB_INTERNAL_DEBUG
   PD_LOG( PDDEBUG, "Removing index from mem tree, latchHeld(%d): "
           "rid(%d, %d), ownertransid(%s), recordtransID(%s), obj(%s)",
            treeLatchHeld, _rid._extent, _rid._offset,
            dpsTransIDToString( _ownerTransID ).c_str(),
            dpsTransIDToString( _recordTransID ).c_str(),
            this->getRecordObj().toString().c_str() ) ;
#endif
         pTree->remove( keyNode, this, treeLatchHeld ) ;
      }
      else
      {
#if SDB_INTERNAL_DEBUG
   PD_LOG( PDDEBUG, "Resetting index in mem tree, latchHeld(%d): "
           "rid(%d, %d), ownertransid(%s), recordtransID(%s), obj(%s)",
            treeLatchHeld, _rid._extent, _rid._offset,
            dpsTransIDToString( _ownerTransID ).c_str(),
            dpsTransIDToString( _recordTransID ).c_str(),
            this->getRecordObj().toString().c_str() ) ;
#endif
         // reset the tree node value if mvcc is on
         pTree->resetValue( keyNode,
                            getOwnerTransID().getGlobSN(),
                            treeLatchHeld ) ;
      }
   }

   // free up all the storage for this old version record
   void oldVersionContainer::releaseRecord( dmsTransLockCallback* callback )
   {
      preIdxTree *pTree = NULL ;
      idxObjSet::iterator itSet ;
      idxLidMap::iterator itMap ;

      /// 1. save the version to RBS when MVCC is turned on
      /// NOTE:  maybe we can skip this write out if this is lock
      ///        release due to rollback. But it won't hurt much
      ///        if we just write it out. There could be an identical
      ///        version in RBS, waste one disk read in the future
/*    //FIXME: potential perf improvement to only write RBS during commit(5505)
      if ( pmdGetOptionCB()->mvccOn()  && this->hasRecord() )
      {
         // FIXME, decide on the API during review
         SINT32 rc = pmdGetKRCB()->getDMSCB()->getRBSSUMgr()
                      ->appendRecord( _csID, _clID, _rid,
                                      _recordTransID,
                                      _ownerTransID,
                                      this->getRecordObj() ) ;  // use insertRecord API
                   //  this->getRecord() ) ;  // use my own interface
         if ( rc )
         {
            PD_LOG( PDERROR,
                    "Failed to write record (%d, %d, %d, %d) to RBS "
                    "(rc=%d), leave the record!",
                    _csID, _clID, _rid._extent, _rid._offset, rc ) ;
            SDB_ASSERT( FALSE,
                        "Failed to write in memory old version to RBS") ;
            goto done ;
         }
#ifdef _DEBUG
         PD_LOG( PDDEBUG, "Successfully saved record to RBS: "
                 "rid(%d, %d), ownertransid(%s), recordtransID(%s), obj(%s)",
                  _rid._extent, _rid._offset,
                  dpsTransIDToString( _ownerTransID ).c_str(),
                  dpsTransIDToString( _recordTransID ).c_str(),
                  this->getRecordObj().toString().c_str() ) ;
#endif

      }
*/
      /// 2. release the tree node if mvcc is not turned on
      itSet = _oldIdx.begin() ;
      while( itSet != _oldIdx.end() )
      {
         const dpsIdxObj &tmpObj = *itSet ;

         itMap = _oldIdxLid.find( tmpObj.getIdxLID() ) ;
         if ( itMap == _oldIdxLid.end() )
         {
#if defined ( _DEBUG )
            PD_LOG( PDDEBUG, "Index[%u] is not found in idxLidMap",
                    tmpObj.getIdxLID() );
            SDB_ASSERT( FALSE, "Index is not found in idxLidMap, _oldIdxLid" ) ;
#endif
         }
         else
         {
            BOOLEAN treeLatchHeld = FALSE ;
            pTree = (itMap->second).get() ;
            preIdxTreeNodeKey keyNode( &(tmpObj.getKeyObj()),
                                       _rid,
                                       pTree->getOrdering(),
                                       _ownerTransID ) ;
            INT32 idxLID = pTree->getLID() ;

            // For merge scanner, make sure the scanner is X latched ;
            // as for non-transactional IUD, it could be disk scan.
            if ( callback &&
                 callback->isIndexProtectionRequired() &&
                 callback->isIndexProtected( idxLID ) )
            {
               SDB_ASSERT( callback->isIndexProtected( idxLID, EXCLUSIVE ),
                           "Index tree must be held exclusively" ) ;
               treeLatchHeld = TRUE ;
            }

            _releaseRecord( pTree, keyNode, treeLatchHeld ) ;
         }
         ++itSet ;
      }

      _oldIdx.clear() ;
      _oldIdxLid.clear() ;

      /// 3. release the record
      _recordPtr = dpsOldRecordPtr() ;
      _statMask = 0 ;
      _ownerTID = 0 ;

      _ownerTransID.reset() ;
      _recordTransID.reset() ;

      return ;
   }

   BOOLEAN oldVersionContainer::tryReleaseRecord( dmsTransLockCallback* callback )
   {
      BOOLEAN succeed = FALSE ;
      preIdxTree *pTree = NULL ;
      idxObjSet::iterator itSet ;
      idxLidMap::iterator itMap ;

      /// 1. release the tree node
      itSet = _oldIdx.begin() ;
      while( itSet != _oldIdx.end() )
      {
         const dpsIdxObj &tmpObj = *itSet ;

         itMap = _oldIdxLid.find( tmpObj.getIdxLID() ) ;
         if ( itMap == _oldIdxLid.end() )
         {
#if defined ( _DEBUG )
            PD_LOG( PDDEBUG, "Index[%u] is not found in idxLidMap",
                    tmpObj.getIdxLID() );
            SDB_ASSERT( FALSE, "Index is not found in idxLidMap, _oldIdxLid" ) ;
#endif
         }
         else
         {
            BOOLEAN treeLatchHeld = FALSE ;
            pTree = (itMap->second).get() ;
            preIdxTreeNodeKey keyNode( &(tmpObj.getKeyObj()),
                                       _rid,
                                       pTree->getOrdering(),
                                       _ownerTransID ) ;
            INT32 idxLID = pTree->getLID() ;

            // For merge scanner, make sure the scanner is X latched ;
            // as for non-transactional IUD, it could be disk scan.
            if ( callback &&
                 callback->isIndexProtectionRequired() &&
                 callback->isIndexProtected( idxLID ) )
            {
               SDB_ASSERT( callback->isIndexProtected( idxLID, EXCLUSIVE ),
                           "Index tree must be held exclusively" ) ;
               treeLatchHeld = TRUE ;
            }

            if ( idxLID == tmpObj.getIdxLID() && treeLatchHeld )
            {
               _releaseRecord( pTree, keyNode, treeLatchHeld ) ;
               _oldIdx.erase( itSet++ ) ;
               continue ;
            }
            else if ( pTree->tryLockX() )
            {
               _releaseRecord( pTree, keyNode, TRUE ) ;
               pTree->unlockX() ;
               _oldIdx.erase( itSet++ ) ;
               continue ;
            }
            else
            {
               goto done ;
            }
         }
         ++itSet ;
      }

      releaseRecord() ;
      succeed = TRUE ;

   done:
      return succeed ;
   }

   BOOLEAN oldVersionContainer::releaseIndex( dmsTransLockCallback *callback,
                                              preIdxTreePtr &treePtr,
                                              BOOLEAN hasLocked )
   {
      BOOLEAN removed = FALSE ;

      SDB_ASSERT( NULL != treePtr.get(), "index tree is invalid" ) ;

      INT32 indexLID = treePtr->getLID() ;

      // check all index items to match given index LID
      // NOTE: dpsIdxObj compared by pair ( indexLID, key )
      dpsIdxObj tempObj( BSONObj(), indexLID ) ;
      idxObjSet::iterator iter = _oldIdx.upper_bound( tempObj ) ;
      while ( iter != _oldIdx.end() )
      {
         if ( iter->getIdxLID() == indexLID )
         {
            const dpsIdxObj &idxObj = ( *iter ) ;
            preIdxTreeNodeKey keyNode( &( idxObj.getKeyObj() ),
                                       _rid,
                                       treePtr->getOrdering(),
                                       _ownerTransID ) ;
            treePtr->remove( keyNode, this, hasLocked ) ;
            _oldIdx.erase( iter ++ ) ;
            removed = TRUE ;
            continue ;
         }
         else if ( iter->getIdxLID() > indexLID )
         {
            break ;
         }
         ++ iter ;
      }

      if ( removed )
      {
         _oldIdxLid.erase( indexLID ) ;
      }

      return removed ;
   }

   void oldVersionContainer::setRecordDeleted()
   {
      OSS_BIT_SET( _statMask, OLDVER_MASK_DELETED ) ;
   }

   void oldVersionContainer::setDiskDeleting()
   {
      OSS_BIT_SET( _statMask, OLDVER_MASK_DISK_DELETING ) ;
   }

   void oldVersionContainer::setRecordNew( UINT32 ownerTID )
   {
      OSS_BIT_SET( _statMask, OLDVER_MASK_NEW_RECORD ) ;
      _ownerTID = ownerTID ;
   }

   BOOLEAN oldVersionContainer::isRecordNew() const
   {
      return OSS_BIT_TEST( _statMask, OLDVER_MASK_NEW_RECORD ) ? TRUE : FALSE ;
   }

   void oldVersionContainer::setRecordDummy( UINT32 ownerTID )
   {
      OSS_BIT_SET( _statMask, OLDVER_MASK_DUMMY ) ;
      _ownerTID = ownerTID ;
   }

   BOOLEAN oldVersionContainer::isRecordDummy() const
   {
      return OSS_BIT_TEST( _statMask, OLDVER_MASK_DUMMY ) ? TRUE : FALSE ;
   }

   void oldVersionContainer::setRolledback()
   {
      OSS_BIT_SET( _statMask, OLDVER_MASK_ROLLED_BACK ) ;
   }

   BOOLEAN oldVersionContainer::isRolledback() const
   {
      return OSS_BIT_TEST( _statMask, OLDVER_MASK_ROLLED_BACK ) ? TRUE : FALSE ;
   }


   UINT32 oldVersionContainer::getOwnerTID() const
   {
      return _ownerTID ;
   }

   BOOLEAN oldVersionContainer::isRecordDeleted() const
   {
      return OSS_BIT_TEST( _statMask, OLDVER_MASK_DELETED ) ? TRUE : FALSE ;
   }

   BOOLEAN oldVersionContainer::hasRecord() const
   {
      if (NULL != _recordPtr.get())
      {
         SDB_ASSERT( !isRecordDummy() && !isRecordNew(),
                     "Can't be dummy or new when has record" ) ;
         return TRUE ;
      }
      else
      {
         return FALSE ;
      }
   }

   BOOLEAN oldVersionContainer::isDiskDeleting() const
   {
      return OSS_BIT_TEST( _statMask, OLDVER_MASK_DISK_DELETING ) ?
             TRUE : FALSE ;
   }

   // check if the index lid already exists in the set
   BOOLEAN oldVersionContainer::idxLidExist( SINT32 idxLID ) const
   {
      return ( _oldIdxLid.find( idxLID ) != _oldIdxLid.end() ) ;
   }

   const dpsIdxObj* oldVersionContainer::getIdxObj( SINT32 idxLID ) const
   {
      const dpsIdxObj *pObj = NULL ;
      for ( idxObjSet::const_iterator i = _oldIdx.begin() ;
            i != _oldIdx.end() ;
            ++i )
      {
         if ( i->getIdxLID() == idxLID )
         {
            pObj = &( *i ) ;
            break ;
         }
      }

      return pObj ;
   }

   BOOLEAN oldVersionContainer::isIdxObjExist( const dpsIdxObj &obj ) const
   {
      return _oldIdx.find( obj ) != _oldIdx.end() ? TRUE : FALSE ;
   }

   // given an index object, insert into the idxObjSet. Return false
   // if the same index for the record already exist. In this case,
   // the object was not inserted
   BOOLEAN oldVersionContainer::insertIdx( const dpsIdxObj &i )
   {
      return _oldIdx.insert( i ).second ;
   }

   INT32 oldVersionContainer::insertIdxTree( preIdxTreePtr treePtr,
                                             BOOLEAN *pInserted )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN succeed = FALSE ;
      try
      {
         succeed = _oldIdxLid.insert( idxLidMap::value_type( treePtr->getLID(),
                                                       treePtr ) ).second ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_OOM ;
      }
      if ( pInserted )
      {
         *pInserted = succeed ;
      }
      return rc ;
   }

   BOOLEAN oldVersionContainer::isLockOnChain() const
   {
      return 0 == _lockCnt ? FALSE : TRUE ;
   }

   void oldVersionContainer::lockOnChain()
   {
      ossFetchAndIncrement32( &_lockCnt ) ;
   }

   void oldVersionContainer::unlockOnChain()
   {
      ossFetchAndDecrement32( &_lockCnt ) ;
   }

   /*
   oldVersionContainer* oldVersionContainer::newThis( const dmsRecordID &rid,
                                                      INT32 csID,
                                                      UINT16 clID,
                                                      UINT32 csLID,
                                                      UINT32 clLID )
   {
      oldVersionContainer *pOldVer = NULL ;
      pOldVer = SDB_OSS_NEW oldVersionContainer( rid, csID, clID,
                                                 csLID, clLID ) ;
      if ( pOldVer )
      {
         ossFetchAndIncrement32( &(pOldVer->_refCount) ) ;
      }

      return pOldVer ;
   }

   oldVersionContainer* oldVersionContainer::copyThis()
   {
      INT32 oldRef = ossFetchAndIncrement32( &_refCount ) ;
      if ( oldRef > 0 )
      {
         OSS_BIT_SET( _statMask, OLDVER_MASK_HAS_COPED ) ;
         return this ;
      }
      else
      {
         SDB_ASSERT( oldRef > 0, "OldRef is invalid" ) ;
      }
      return NULL ;
   }

   void oldVersionContainer::releaseThis()
   {
      INT32 oldRef = ossFetchAndDecrement32( &_refCount ) ;
      if ( oldRef <= 1 )
      {
         SDB_ASSERT( 1 == oldRef, "OldRef is invalid" ) ;
         SDB_OSS_DEL this ;
      }
   }

   BOOLEAN oldVersionContainer::hasCoped() const
   {
      return OSS_BIT_TEST( _statMask, OLDVER_MASK_HAS_COPED ) ? TRUE : FALSE ;
   }
   */

}  // end of namespace

