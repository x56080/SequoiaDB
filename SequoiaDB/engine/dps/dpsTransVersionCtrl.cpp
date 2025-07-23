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
#include "dpsTrace.hpp"
#include "ixmExtent.hpp" // for _keyCmp

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
                                         const Ordering *order )
   :_keyObj( *key ), _order( order )
   {
      _rid._extent = rid._extent ;
      _rid._offset = rid._offset ;
   }

   preIdxTreeNodeKey::preIdxTreeNodeKey( const preIdxTreeNodeKey &key )
   : _keyObj( key._keyObj ), _order( key._order )
   {
      _rid._extent = key._rid._extent ;
      _rid._offset = key._rid._offset ;
   }

   preIdxTreeNodeKey::~preIdxTreeNodeKey ()
   {
      // We do not want to free the keyData in super class as we don't 
      // own it, simply rid to invalid incase some one continue using it.
      // delete of the lock LRB does the clean up of the key space.
      _rid.reset() ;
   }

   string preIdxTreeNodeKey::toString() const
   {
      std::stringstream ss ;
      ss << "RID(" << _rid._extent << "," << _rid._offset
         << ", Key:" << _keyObj.toString() ;
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

   UINT32 preIdxTreeNodeValue::getOwnnerTID() const
   {
      if ( _pOldVer )
      {
         return _pOldVer->getOwnnerTID() ;
      }
      return 0 ;
   }

   string preIdxTreeNodeValue::toString() const
   {
      dmsRecordID rid = getRecordID() ;
      BSONObj obj = getRecordObj() ;

      std::stringstream ss ;
      ss << "RecordID(" <<  rid._extent << "," << rid._offset << "), " ;
      if ( isRecordDeleted() )
      {
         ss << "(Deleted)" ;
      }
      ss << "Object(" << obj.toString() << ")" ;

      return ss.str() ;
   }

   /*
      preIdxTree implement
   */
   preIdxTree::preIdxTree( const SINT32 idxID, const ixmIndexCB *indexCB )
   {
      _isValid = TRUE ;
      _idxLID = idxID ;
      _keyPattern = indexCB->keyPattern().getOwned() ;
      _order = SDB_OSS_NEW clsCataOrder( Ordering::make( _keyPattern ) ) ;
   }
   
   // copy constructor
   preIdxTree::preIdxTree( const preIdxTree &intree )
   {
      _idxLID = intree._idxLID ;
      _keyPattern = intree._keyPattern ;
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
                                      const dmsRecordID &rid ) const
   {
      return find( preIdxTreeNodeKey( key, rid, getOrdering() ) ) ;
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
      else
      {
         PD_LOG( PDDEBUG, "Inserted key[%s] to index tree  with value[%s]",
                 keyNode.toString().c_str(), value.toString().c_str() ) ;
      }

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
                              BOOLEAN hasLock )
   {
      preIdxTreeNodeKey keyNode( keyData, rid, getOrdering() ) ;
      return insert( keyNode, value, hasLock ) ;
   }

   INT32 preIdxTree::insertWithOldVer( const BSONObj *keyData,
                                       const dmsRecordID &rid,
                                       oldVersionContainer *oldVer,
                                       BOOLEAN hasLock )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( oldVer, "OldVer is NULL" ) ;
      SDB_ASSERT( rid == oldVer->getRecordID(), "Record ID is not the same" ) ;

      BOOLEAN isLocked = FALSE ;

      try
      {
         dpsIdxObj myIdxObj( *keyData, getLID() ) ;
         preIdxTreeNodeKey keyNode( &(myIdxObj.getKeyObj()), rid,
                                    getOrdering() ) ;
         preIdxTreeNodeValue keyValue( oldVer ) ;
         INDEX_TREE_CPOS pos ;

         if ( !hasLock )
         {
            lockX() ;
            isLocked = TRUE ;
         }

         // check if it exist. We might be able to save this check if all
         // callers did the check.
         pos = find( keyNode ) ;
         if ( isPosValid( pos ) )
         {
            /// already exist
            goto done ;
         }

         SDB_ASSERT( oldVer->idxLidExist( getLID() ),
                     "LID is not exist" ) ;

         // insert to both idxset(oldVer) and idxTree
         if( oldVer->insertIdx( myIdxObj ) )
         {
            insert( keyNode, keyValue, TRUE ) ;
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
         rc = SDB_SYS ;
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
         tmpValue = pos->second ;
         ++numDeleted ;
         _tree.erase( pos ) ;
      }

      if ( !hasLock )
      {
         unlockX() ;
      }

      if ( 1 != numDeleted )
      {
         if ( _isValid )
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
                              BOOLEAN hasLock )
   {
      preIdxTreeNodeKey keyNode( keyData, rid, getOrdering() ) ;
      return remove( keyNode, hasLock ) ;
   }

   INT32 preIdxTree::advance( INDEX_TREE_CPOS &pos, INT32 direction ) const
   {
      INT32 rc = SDB_OK ;

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

         /// check is deleted
         if ( !pos->second.isRecordDeleted() )
         {
            break ;
         }
      }

   done:
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
                                      const vector< const BSONElement * > &matchEle,
                                      const vector < BOOLEAN > &matchInclusive,
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
                                const vector< const BSONElement* > &matchEle,
                                const vector< BOOLEAN > &matchInclusive,
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
                                 const vector < const BSONElement *> &matchEle,
                                 const vector < BOOLEAN > &matchInclusive,
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
            if ( startIter->second.isRecordDeleted() )
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

   void preIdxTree::printTree() const
   {
      const UINT32 maxOnceOutSize = 3072 ;
      UINT32 index = 0 ;
      INDEX_TREE_CPOS pos ;

      while( TRUE )
      {
         std::stringstream ss ;

         if ( 0 == index )
         {
            ss << "==> Index tree[Key: " << _keyPattern.toString()
               << ", LID:" << _idxLID
               << ", Size:" << _tree.size()
               << "] nodes:" << std::endl ;
            pos = _tree.begin() ;
         }

         while ( pos != _tree.end() )
         {
            ss << ++index << "Key: " << pos->first.toString()
               << ", Value: " << pos->second.toString()
               << std::endl ;
            ++pos ;

            if ( ss.gcount() >= maxOnceOutSize )
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
      oldVersionCB implement
   */
   oldVersionCB::oldVersionCB()
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
         SDB_ASSERT( it->second->empty(), "Index tree should be empty" ) ;
         ++it ;
      }

      _idxTrees.clear() ;
   }

   // Create an in memory index tree and add to the map
   // Caller must hold _oldVersionCBLatch in X
   // PD_TRACE_DECLARE_FUNCTION ( SDB_OLDVERSIONCB_ADDIDXTREE, "oldVersionCB::addIdxTree" )
   INT32 oldVersionCB::addIdxTree( const globIdxID &gid,
                                   const ixmIndexCB *indexCB,
                                   preIdxTreePtr &treePtr,
                                   BOOLEAN hasLock )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_OLDVERSIONCB_ADDIDXTREE ) ;

      preIdxTreePtr tmpTreePtr ;
      preIdxTree *pTree = NULL ;
      pair<IDXID_TO_TREE_MAP_IT, BOOLEAN> ret ;

      pTree = SDB_OSS_NEW preIdxTree( gid._idxLID, indexCB ) ;
      if ( !pTree || !pTree->isValid() )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      tmpTreePtr = preIdxTreePtr( pTree ) ;
      pTree = NULL ;

      if ( !hasLock )
      {
         latchX() ;
      }

      ret = _idxTrees.insert( IDXID_TO_TREE_MAP_PAIR( gid, tmpTreePtr ) ) ;

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
      if ( rc )
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

   void oldVersionCB::clearIdxTreeByCSID( UINT32 csID, BOOLEAN hasLock )
   {
      preIdxTreePtr treePtr ;
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

               treePtr->clear() ;
               treePtr->setDeleted() ;
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
   }

   void oldVersionCB::clearIdxTreeByCLID( UINT32 csID,
                                          UINT16 clID,
                                          BOOLEAN hasLock )
   {
      preIdxTreePtr treePtr ;
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

            if ( treePtr.get() )
            {
               PD_LOG( PDDEBUG, "Has removed index tree[%s], Key:%s",
                       it->first.toString().c_str(),
                       treePtr->getKeyPattern().toString().c_str() ) ;

               treePtr->clear() ;
               treePtr->setDeleted() ;
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
   }

   /*
      oldVersionContainer implement
   */
   oldVersionContainer::oldVersionContainer( const dmsRecordID &rid,
                                             INT32 csID, UINT16 clID,
                                             UINT32 csLID, UINT32 clLID )
   :_csID( csID ), _clID( clID ), _csLID( csLID ), _clLID( clLID ),
    _rid( rid )
   {
      _isNewRecord   = FALSE ;
      _isDiskDeleting= FALSE ;
      _isDeleted     = FALSE ;
      _isDummyRecord = FALSE ;
      _ownnerTID     = 0 ;
      _prev          = NULL ;
      _next          = NULL ;
      _isOnChain     = FALSE ;
   }

   oldVersionContainer::~oldVersionContainer()
   {
      releaseRecord() ;
   }

   BOOLEAN oldVersionContainer::isRecordEmpty() const
   {
      if ( _isDummyRecord || _isNewRecord || _recordPtr.get() )
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
                                          const BSONObj &obj,
                                          UINT32 ownnerTID )
   {
      INT32 rc = SDB_OK ;
      UINT32 recSize = 0 ;
      dmsRecord *pNewRecord = NULL ;

      SDB_ASSERT( !_recordPtr.get(), "Old record is not NULL" ) ;
      SDB_ASSERT( pRecord, "Record is NULL" ) ;

      if ( _recordPtr.get() )
      {
         goto done ;
      }

      recSize = DMS_RECORD_METADATA_SZ + obj.objsize() ;
      _recordPtr = dpsOldRecordPtr::alloc( recSize ) ;
      if ( !_recordPtr.get() )
      {
         PD_LOG( PDERROR, "Alloc memory(%u) failed, rc: %d",
                 recSize, rc ) ;
         goto error ;
      }

      pNewRecord = ( dmsRecord* )_recordPtr.get() ;
      /// copy header
      ossMemcpy( _recordPtr.get(), (const void*)pRecord,
                 DMS_RECORD_METADATA_SZ ) ;

      pNewRecord->unsetCompressed() ;
      pNewRecord->setSize( recSize ) ;

      /// copy data
      ossMemcpy( _recordPtr.get() + DMS_RECORD_METADATA_SZ,
                 obj.objdata(), obj.objsize() ) ;

#ifdef _DEBUG
      PD_LOG ( PDDEBUG, "Saved old copy for rid(%d,%d) to oldVer(%x)",
               _rid._extent, _rid._offset, this ) ;
#endif //_DEBUG

      _ownnerTID = ownnerTID ;

   done:
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN oldVersionContainer::isIndexObjEmpty() const
   {
      return _oldIdx.empty() ? TRUE : FALSE ;
   }

   void oldVersionContainer::releaseRecord()
   {
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
            SDB_ASSERT( FALSE, "Index[%u] can't found in index set" ) ;
         }
         else
         {
            pTree = (itMap->second).get() ;
            pTree->remove( &(tmpObj.getKeyObj()), _rid, FALSE ) ;
         }
         ++itSet ;
      }

      _oldIdx.clear() ;
      _oldIdxLid.clear() ;

      /// 2. release the record
      _recordPtr = dpsOldRecordPtr() ;
      _isNewRecord = FALSE ;
      _isDiskDeleting = FALSE ;
      _isDeleted = FALSE ;
      _isDummyRecord = FALSE ;
      _ownnerTID = 0 ;
   }

   void oldVersionContainer::setRecordDeleted()
   {
      _isDeleted = TRUE ;
   }

   void oldVersionContainer::setDiskDeleting()
   {
      _isDiskDeleting = TRUE ;
   }

   void oldVersionContainer::setRecordNew( UINT32 ownnerTID )
   {
      _isNewRecord = TRUE ;
      _ownnerTID = ownnerTID ;
   }

   BOOLEAN oldVersionContainer::isRecordNew() const
   {
      return _isNewRecord ;
   }

   void oldVersionContainer::setRecordDummy( UINT32 ownnerTID )
   {
      _isDummyRecord = TRUE ;
      _ownnerTID = ownnerTID ;
   }

   BOOLEAN oldVersionContainer::isRecordDummy() const
   {
      return _isDummyRecord ;
   }

   UINT32 oldVersionContainer::getOwnnerTID() const
   {
      return _ownnerTID ;
   }

   BOOLEAN oldVersionContainer::isRecordDeleted() const
   {
      return _isDeleted ;
   }

   BOOLEAN oldVersionContainer::isDiskDeleting() const
   {
      return _isDiskDeleting ;
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

   BOOLEAN oldVersionContainer::insertIdxTree( preIdxTreePtr treePtr )
   {
      return _oldIdxLid.insert( idxLidMap::value_type( treePtr->getLID(),
                                                       treePtr ) ).second ;
   }

}  // end of namespace

