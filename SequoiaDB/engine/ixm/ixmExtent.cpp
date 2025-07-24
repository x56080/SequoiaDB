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

   Source File Name = ixmExtent.cpp

   Descriptive Name = Index Manager Extent

   When/how to use: this program may be used on binary and text-formatted
   versions of Index Manager component. This file contains functions for index
   extent implmenetation. This include B tree insert/update/delete.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "ixmExtent.hpp"
#include "dmsStorageIndex.hpp"
#include "pd.hpp"
#include "monCB.hpp"
#include "dmsStorageUnit.hpp"
#include "dmsDump.hpp"
#include "pdTrace.hpp"
#include "ixmTrace.hpp" //include ixmTrace

#include "ixmContext.hpp"

using namespace bson ;

namespace engine
{
   // index tree level starting from 0
   #define IXM_X_LOCK_START_LEVEL ( 2 )

   // create new extent id without parent
   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMEXT2, "_ixmExtent::_ixmExtent" )
   _ixmExtent::_ixmExtent ( dmsExtentID extentID, UINT16 mbID,
                            dmsStorageIndex *pIndexSu )
   {
      SDB_ASSERT ( pIndexSu, "index su can't be NULL" ) ;
      PD_TRACE_ENTRY ( SDB__IXMEXT2 ) ;
      ixmExtentHead *pHeader = NULL ;
      _extRW = pIndexSu->extent2RW( extentID, mbID ) ;
      _pIndexSu = pIndexSu ;
      _pPageMap = _pIndexSu->getPageMap( mbID ) ;
      _pageSize = _pIndexSu->pageSize() ;

      _pOutKeyPageMap = _pIndexSu->getIXMOutsideKeyPageMap() ; 

      pHeader = _extRW.writePtr<ixmExtentHead>( 0, _pageSize ) ;
      _extentHead = (const ixmExtentHead*)pHeader ;
      SDB_ASSERT(_extentHead, "extent can't be NULL" ) ;
      _me = extentID ;
      pHeader->_eyeCatcher [0] = IXM_EXTENT_EYECATCHER0 ;
      pHeader->_eyeCatcher [1] = IXM_EXTENT_EYECATCHER1 ;
      pHeader->_totalKeyNodeNum = 0 ;
      pHeader->_mbID = mbID ;
      // not change flag here
      pHeader->_version = IXM_EXTENT_CURRENT_V ;
      pHeader->_parentExtentID = DMS_INVALID_EXTENT ;
      // set to 65535, indicating end of the page
      pHeader->_beginFreeOffset = _pageSize-1 ;
      pHeader->_right = DMS_INVALID_EXTENT ;
      pHeader->_totalFreeSize = pHeader->_beginFreeOffset -
                        (sizeof(ixmExtentHead) +
                        (pHeader->_totalKeyNodeNum*sizeof(ixmKeyNode))) ;
      pIndexSu->addStatFreeSpace( mbID, pHeader->_totalFreeSize ) ;

      PD_TRACE_EXIT ( SDB__IXMEXT2 );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMEXT4, "_ixmExtent::_ixmExtent" )
   _ixmExtent::_ixmExtent ( dmsExtentID extentID,
                            dmsStorageIndex *pIndexSu )
   {
      SDB_ASSERT ( pIndexSu, "index su can't be NULL" ) ;
      PD_TRACE_ENTRY ( SDB__IXMEXT4 );
      _extRW = pIndexSu->extent2RW( extentID, -1 ) ;
      _pIndexSu = pIndexSu ;
      _pageSize = _pIndexSu->pageSize() ;

      _pOutKeyPageMap = _pIndexSu->getIXMOutsideKeyPageMap() ; 

      _me = extentID ;
      _extentHead = _extRW.readPtr<ixmExtentHead>( 0, _pageSize ) ;
      /// set collection id
      _extRW.setCollectionID( _extentHead->_mbID ) ;
      _pPageMap = _pIndexSu->getPageMap( _extentHead->_mbID ) ;
      PD_TRACE_EXIT ( SDB__IXMEXT4 );
      SDB_ASSERT(_extentHead, "extent can't be NULL" ) ;
   }

   BOOLEAN _ixmExtent::verify () const
   {
      if ( !_extentHead )
      {
         PD_LOG ( PDERROR, "NULL index extent" ) ;
         return FALSE ;
      }
      if ( _extentHead->_eyeCatcher[0] != IXM_EXTENT_EYECATCHER0 ||
           _extentHead->_eyeCatcher[1] != IXM_EXTENT_EYECATCHER1 )
      {
         PD_LOG ( PDERROR, "Invalid index eye-catcher" ) ;
         return FALSE ;
      }
      if ( !(_extentHead->_flag & DMS_MB_FLAG_USED) )
      {
         PD_LOG ( PDERROR, "Unused extent" ) ;
         return FALSE ;
      }
      return TRUE ;
   }

   // Find a given key and RID, returns key position if the caller want to
   // insert a new key, and also return boolean found
   // Each index page is 65536 bytes, with 20 bytes head at beginning, 1 byte
   // reserve at end, we have 65515 bytes. Each ixmKeyNode is 16 bytes, and
   // minimal compact key size is 1 byte, so each index page can maximumly store
   // 65515/17=3853 records, so with binary search we shouldn't spent more than
   // 12 (2^12 = 4096) compares to get the key in the worst case
   // Input:
   //    indexCB : index control block
   //    ixmKey  : index key tries to match
   //    rid     : record ID for the record in collection
   //    order   : order for the index key def
   // Output:
   //    pos     : key position if found, or the expected key position if the
   //              given key is not in the index
   //    keyFound  : whether the key exist in the page
   //    sameFound : whether the key + rid exist in the page.
   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMEXT_FIND, "_ixmExtent::find" )
   INT32 _ixmExtent::find ( const ixmIndexCB *indexCB,
                            const ixmKey &key,
                            const dmsRecordID &rid,
                            const Ordering &order,
                            UINT16 &pos,
                            INT32 &keyFoundPos,
                            BOOLEAN &sameFound ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__IXMEXT_FIND ) ;

      keyFoundPos = -1 ;
      sameFound = FALSE ;

      // use binary search, start from 0 and totalKeyNodeNum-1
      INT32 low = 0 ;
      INT32 high = _extentHead->_totalKeyNodeNum-1 ;
      // get the middle pos
      INT32 middle = (low + high)/2 ;
      // loop until high>=low
      while ( low <= high )
      {
         PD_TRACE3 ( SDB__IXMEXT_FIND,
                     PD_PACK_INT ( low ),
                     PD_PACK_INT ( high ),
                     PD_PACK_INT ( middle ) ) ;
         // get the key for the middle
         const CHAR *keyData = getKeyData( middle ) ;
         // the key supposed to exist, otherwise there's some corruption happen
         if ( !keyData )
         {
            PD_LOG ( PDERROR, "Unable to locate key" ) ;
            dumpIndexExtentIntoLog () ;
            rc = SDB_SYS ;
            goto error ;
         }
         // create ixmKey object and let it compare with the input
         ixmKey keyDisk( keyData ) ;
         INT32 result = key.woCompare ( keyDisk, order ) ;
         PD_TRACE1 ( SDB__IXMEXT_FIND, PD_PACK_INT ( result ) ) ;
         // if the result are the same, let's check whether we allows key
         // duplicate first
         if ( 0 == result )
         {
            keyFoundPos = middle ;
            const ixmKeyNode *M = getKeyNode( middle ) ;
            // let's continue compare the RID
            result = rid.compare( M->_rid ) ;
         }
         // if the compare result shows disk value is smaller, let's set high =
         // middle-1
         if ( result < 0 )
         {
            high = middle -1 ;
         }
         // if the compare result shows disk value is greater, let's set low =
         // middle+1
         else if ( result > 0 )
         {
            low = middle + 1 ;
         }
         // otherwise we have both key+rid identical
         else
         {
            // note it's not possible for an unused key hit this path, because
            // unused key got 1 in least bit, but all normal RID are 4 bytes
            // aligned, that means when result = 0, it always means we found
            // duplicate key + rid for used record
            pos = middle ;
            sameFound = TRUE ;
            goto done ;
         }
         // continue with a new middle
         middle = (low + high)/2 ;
      }
      // if still unable to find
      pos = low ;
      PD_TRACE1 ( SDB__IXMEXT_FIND, PD_PACK_USHORT(pos) ) ;
      // sanity check, this is essential even in release build, just in case
      // index corruption happened on disk
      if ( pos != _extentHead->_totalKeyNodeNum )
      {
         // make sure the requested key is NOT greater than the next key
         {
            const CHAR *keyData = getKeyData (pos) ;
            ixmKey keyDisk( keyData ) ;
            if ( key.woCompare ( keyDisk, order ) > 0 )
            {
               PD_LOG ( PDERROR, "Internal logic error, key compare wrong" ) ;
               dumpIndexExtentIntoLog () ;
               rc = SDB_SYS ;
               goto error ;
            }
         }
         // make sure the previous key is NOT greater than the requested key
         if ( pos > 0 )
         {
            const CHAR *keyData = getKeyData( pos-1 ) ;
            ixmKey keyDisk(keyData) ;
            if ( keyDisk.woCompare ( key, order ) > 0 )
            {
               PD_LOG ( PDERROR, "Internal logic error, key compare wrong" ) ;
               dumpIndexExtentIntoLog () ;
               rc = SDB_SYS ;
               goto error ;
            }
         }
      }
   done :
      PD_TRACE_EXITRC ( SDB__IXMEXT_FIND, rc );
      return rc ;
   error :
      goto done ;
   }

   // syncronized insert, insert a key and rid into index
   INT32 _ixmExtent::insert ( const ixmKey      & key,
                              const dmsRecordID & rid,
                              const Ordering    & order,
                              BOOLEAN             dupAllowed,
                              ixmIndexCB        * indexCB,
                              UINT32            & xLockLevel,
                              _ixmContext       * pixmContext,
                              utilWriteResult   * pResult )
   {
      // make sure current page is locked
      SDB_DASSERT( pixmContext->isLocking( _me ),
                   "Doesn't have lock on index page !" ) ;
      UINT32 depth = 0 ;
      return _insert ( rid, key, order, dupAllowed, DMS_INVALID_EXTENT,
                       DMS_INVALID_EXTENT, indexCB,
                       depth, xLockLevel, pixmContext, pResult ) ;
   }

   // This function is the wrapper for _basicInsert and _split, depends on
   // whether the current extent has sufficient space for a new record
   // If the new record is inserted into current page, it will set left pointer
   // for the new key, and right pointer for the page
   // Input:
   //    pos       : insert position
   //    rid       : data rid
   //    key       : data key
   //    order     : index key ordering
   //    lchild    : left child
   //    rchild    : right child, lchild and rchild represents the dmsExtentID
   //                that should be set to key->_left and extentHead->_right,
   //                these two parameters could be DMS_INVALID_EXTENT for most
   //                new inserts, and may represent things during promoting keys
   //                into parent during split
   //   indexCB    : index control block
   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMEXT_INSERTHERE, "_ixmExtent::insertHere" )
   INT32 _ixmExtent::insertHere ( UINT16              pos,
                                  const dmsRecordID & rid,
                                  const ixmKey      & key,
                                  const Ordering    & order,
                                  dmsExtentID         lchild,
                                  dmsExtentID         rchild,
                                  ixmIndexCB        * indexCB,
                                  _ixmContext       * pixmContext )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__IXMEXT_INSERTHERE ) ;

      _ixmLockInfo currentPage( _me ), parentPage( getParent() ) ; 

      // the caller must have X lock on current page
      SDB_DASSERT( ( pixmContext->getLockHeldInfo( currentPage ) &&
                   ( DPS_TRANSLOCK_X == currentPage.lockMode ) ),
                   "Doesn't have X lock on current page !" ) ;

      // get the child extent id for pos, as _basicInsert
      // calls _reorg in its code path, where pos may be changed
      dmsExtentID ch = getChildExtentID ( pos ) ;

      // attempt to physically insert the key into page
      // if there's no space in the page, it will attempt to reorg the page
      // first, if still not enough space it will return SDB_IXM_NOSPC
      rc = _basicInsert ( pos, rid, key, order, indexCB, FALSE ) ;
      if ( rc )
      {
         if ( SDB_IXM_NOSPC == rc )
         {
            // if _reorg is called in _basicInsert code path
            // the pos might be changed, so return with
            // SDB_IXM_REORG_DONE and retry to avoid inserting
            // into wrong position
            if ( getChildExtentID( pos ) != ch )
            {
               rc = SDB_IXM_REORG_DONE ;
               goto error ;
            }

            // try acquire X lock on parent page
            if ( parentPage.isValid() )
            {
               rc = ixmTryLock( pixmContext, parentPage.page, DPS_TRANSLOCK_X );
               if ( SDB_OK == rc )
               {
                  rc = _split ( pos, rid, key, order, lchild, rchild, indexCB,
                                pixmContext, FALSE ) ;
                  // release lock on parent page
                  ixmUnlock( pixmContext, parentPage.page, TRUE ) ;
                  goto done ;
               }
               else
               {
                  // insert a dummy slot/key in current page
                  rc = _basicInsert ( pos, rid, key, order, indexCB, TRUE ) ;
                  if ( rc )
                  {
                     // dump error message if errCode returned
                     PD_LOG ( PDERROR, "Failed to insert, rc = %d", rc ) ;
                     goto error ;
                  }
               }
            }
            else
            {
               rc = _split ( pos, rid, key, order, lchild, rchild, indexCB,
                             pixmContext, FALSE ) ;
               goto done ;
            }
         }
         else
         {
            if ( SDB_IXM_REORG_DONE != rc )
            { 
               // dump error message if other errCode returned
               PD_LOG ( PDERROR, "Failed to insert, rc = %d", rc ) ;
            }
            goto error ;
         }
      }
      // if insert completed in the current extent, let's reset the left and
      // right pointer
      {
         // get the inserted node
         ixmKeyNode *kn = writeKeyNode( pos ) ;
         // if the node is at end of the page, it means the new value is greater
         // than
         if ( pos+1 == getNumKeyNode() )
         {
            // if we are inserting at the last position, that means we don't
            // have _right for the page (otherwise it will go to _right), and
            // if this is a promoted key from split, we expect _right is
            // pointing to the same extent as lchild
            if ( _extentHead->_right != lchild )
            {
               PD_LOG ( PDERROR, "index logic error[lchild:%d, rchild:%d, "
                        "pos:%u, _extentHead->_right:%d]", lchild, rchild, pos,
                        _extentHead->_right ) ;
               dumpIndexExtentIntoLog () ;
               rc = SDB_SYS ;
               goto error ;
            }
            // let's set the _left for the key to previous extentHead->right,
            // and set extentHead->_right to new rchild
            // this could be the same when the last page in the index tree
            // splits, and promoted to the parent node
            // When this happened, _left will be the original _right, and _right
            // will be the newly created index node
            kn->_left = _extentHead->_right ;
            // no need to set Parent because we are copying inside extent

            _assignRight ( rchild ) ;
         }
         else
         {
            // if we are inserting in the middle, we don't need to worry about
            // _right then
            kn->_left = lchild ;
            // no need to set parent for lchild because it has to be in the
            // same extent. Otherwise we would return SDB_SYS in the following
            // check

            // Since we already shifted all slots to next, let's grab pos+1 as
            // the original key
            ixmKeyNode *kn1 = writeKeyNode( pos + 1 ) ;
            // make sure the original key's _left same as lchild
            if ( kn1->_left != lchild )
            {
               PD_LOG ( PDERROR, "index logic error[lchild:%d, rchild:%d, "
                        "pos:%u, kn1->_left:%d]", lchild, rchild, pos,
                        kn1->_left ) ;
               dumpIndexExtentIntoLog () ;
               rc = SDB_SYS ;
               goto error ;
            }

            // then let's set it's _left to rchild, when rchild !=
            // INVALID_EXTENT, usually it happens during split
            kn1->_left = rchild ;
            // if rchild is not invalid, we should set the parent extent for the
            // rchild to _me
            if ( DMS_INVALID_EXTENT != rchild )
            {
               // since we locked current page with X mode, it is OK to update
               // child page without latching it
               _ixmExtent ( rchild, _pIndexSu ).setParent ( _me ) ;
            }
         }
      }
   done :
      PD_TRACE_EXITRC ( SDB__IXMEXT_INSERTHERE, rc );
      return rc ;
   error :
      goto done ;
   }

   // This function physically insert a key/rid into page. Please note that this
   // function does NOT fix the childs for the adj keys. This operation is
   // performed by insertHere()
   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMEXT__BASICINS, "_ixmExtent::_basicInsert" )
   INT32 _ixmExtent::_basicInsert ( UINT16            & pos,
                                    const dmsRecordID & rid,
                                    const ixmKey      & key,
                                    const Ordering    & order,
                                    ixmIndexCB        * indexCB,
                                    BOOLEAN             bOutsideKey )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__IXMEXT__BASICINS );
      ixmExtentHead *pHeader = _extRW.writePtr<ixmExtentHead>( 0, _pageSize ) ;
      UINT16 bytesNeeded = 0 ;
      imxOutsideKey outKey, dummyKey ;

      // the caller MUST have X lock on current page

      // first let's validate the pos is same or less than the total number of
      // keys in the extent
      if ( pos > getNumKeyNode () )
      {
         PD_LOG ( PDERROR, "insert pos out of range" ) ;
         dumpIndexExtentIntoLog () ;
         rc = SDB_SYS ;
         goto error ;
      }
      // Then let's calculate how many bytes needed
      bytesNeeded = key.dataSize() + sizeof(ixmKeyNode) ;
      // If it's greater than the free size in the page, let's perform reorg and
      // check again
      if ( bytesNeeded > getFreeSize() )
      {
         // before reorg let's get the child extent id for pos
         dmsExtentID ch = getChildExtentID ( pos ) ;
         // note _reorg may change pos
         rc = _reorg ( order, pos ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "index extent reorg failed with rc : %d", rc ) ;
            goto error ;
         }
         SDB_ASSERT ( pos <= getNumKeyNode(), "pos is out of range" ) ;

         if ( FALSE == bOutsideKey )
         {
            // if we still don't have enough space, let's return error
            if ( bytesNeeded > getFreeSize() )
            {
               rc = SDB_IXM_NOSPC ;
               goto error ;
            }
         }
         else
         {
            // if we don't have enough space for a slot, retrun error
            if ( sizeof(ixmKeyNode) > getFreeSize() )
            {
               rc = SDB_IXM_NOSPC ;
               goto error ;
            }
         }
         // after reorg, the pos may points to an element with different lchild,
         // in this case we should be careful and perform find again
         if ( getChildExtentID( pos ) != ch )
         {
            rc = SDB_IXM_REORG_DONE ;
            goto error ;
         }
      }

      if ( bOutsideKey )
      {
         // allocate memory for key obj before insert
         // into outside key page map
         outKey._keyObjPtr = _ixmKeyObjPtr::alloc( key.dataSize(),
                                                   __FILE__, __LINE__ ) ;
         if ( NULL == outKey._keyObjPtr.get() )
         {
            rc = SDB_OOM ;
            PD_LOG ( PDERROR,
                     "Failed to allocate memory, rc = %d", rc ) ;
            goto error ;
         }
      }

      // move getNumKeyNode-pos elements to next slot
      ossMemmove ( (void*)writeKeyNode(pos+1), (void*)getKeyNode(pos),
                   sizeof(ixmKeyNode)*(getNumKeyNode()-pos) ) ;
      // free size minus the size of keynode (16 bytes)
      pHeader->_totalFreeSize -= sizeof(ixmKeyNode) ;
      _pIndexSu->decStatFreeSpace( pHeader->_mbID, sizeof(ixmKeyNode) ) ;
      pHeader->_totalKeyNodeNum ++ ;
      {
         // copy the key into the page
         INT32 datasize = key.dataSize() ;
         ixmKeyNode *kn = writeKeyNode( pos ) ;
         kn->_left = DMS_INVALID_EXTENT ;
         kn->_rid = rid ;

         if ( bOutsideKey )
         {
#if defined (_DEBUG)
            // debug info
            PD_LOG ( PDDEBUG,
                     "Write dummy slot into page:%d, pos:%d, rid(%d,%d)",
                     _me, pos, rid._extent, rid._offset ) ;
#endif
            // prepare data
            kn->SetOutsideKeyFlag() ;
            outKey._rid      = rid ;
            outKey._indexLID = indexCB->getLogicalID() ;
            outKey._mbID     = indexCB->getMBID() ;
            outKey._clLID    = indexCB->getCLLID() ;
            ossMemcpy( outKey._keyObjPtr.get(),
                       key.data(), datasize ) ;
            // insert into outside key page map
            _pOutKeyPageMap->addItem( _me, &outKey ) ;
#if defined (_DEBUG)
            // debug info
            PD_LOG ( PDDEBUG,
                     "Add to outside key map, "
                     "pageId:%d, keyObj addr:%x, size:%d, key value:%s",
                     _me, outKey._keyObjPtr.get(), key.dataSize(),
                     key.toString( FALSE, TRUE ).c_str() ) ;
#endif
            outKey = dummyKey ;

            // start async clean up job to index page
            ixmStartAsyncCleanupIndexPage( _pIndexSu->getDataCSID(),
                                           indexCB->getMBID(),
                                           _pIndexSu->getDatalogicalCSID(),
                                           indexCB->getCLLID(),
                                           indexCB->getLogicalID(),
                                           _me ) ;
         }
         else
         {
            kn->clearOutsideKeyFlag() ;

            // allocate datasize bytes from the page
            rc = _alloc ( datasize, kn->_keyOffset ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to allocate %d bytes in index",
                        key.dataSize()) ;
               goto error ;
            }

            // copy the data into the position
            ossMemcpy ( ((CHAR*)pHeader) + kn->_keyOffset,
                        key.data(), datasize ) ;
         }
      }
#if defined (_DEBUG)
      rc = _validate(MAX, order) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to validate the extent, rc = %d", rc ) ;
         goto error ;
      }
#endif
   done :
      PD_TRACE_EXITRC ( SDB__IXMEXT__BASICINS, rc );
      return rc ;
   error :
      goto done ;
   }

   // split function, this function is private and should not be called outside
   // ixmExtent. It will attempt to split a page in 50/50 mode or 90/10 mode,
   // depends on whether the new record is at end of the page. Once the split
   // position is found, it will allocate a new page and copy all records from
   // split position into new page
   // After that, if the current page is root page, it will allocate another
   // page as root, and prompt the last key from current page into new root
   // If there is parent page, it will also prompt the last key from current
   // page into new root
   // Once the left/right pointer in root are fixed, it will truncate the
   // current page
   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMEXT__SPLIT, "_ixmExtent::_split" )
   INT32 _ixmExtent::_split ( UINT16              pos,
                              const dmsRecordID & rid,
                              const ixmKey      & key,
                              const Ordering    & order,
                              const dmsExtentID   lchild,
                              const dmsExtentID   rchild,
                              ixmIndexCB        * indexCB,
                              _ixmContext       * pixmContext,
                              BOOLEAN             bSplitOnly )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__IXMEXT__SPLIT );
      UINT16 splitPos = 0, newPos = 0 ;
      SDB_ASSERT ( indexCB, "index control block can't be NULL" ) ;
      dmsExtentID newExtentID = DMS_INVALID_EXTENT ;
      const ixmKeyNode *splitKey = NULL ;

      _ixmLockInfo currentPage( _me ), parentPage( getParent() ),
                   newPage, newRootPage ;
      BOOLEAN bHasOutsideKey = FALSE ;
      BOOLEAN bNewPageLocked = FALSE, bNewRootPageLocked = FALSE ;

      // caller MUST hold X lock on current page
      // caller MUST hold X lock on parent page
      if ( ! ( pixmContext->getLockHeldInfo( currentPage ) &&
               ( DPS_TRANSLOCK_X == currentPage.lockMode ) ) )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR,
                 "Error: doesn't have X lock on page:%d, rc:%d",
                 currentPage.page, rc ) ;
         goto error ;
      }
      if ( parentPage.isValid() )
      {
         if ( ! ( pixmContext->getLockHeldInfo( parentPage ) &&
                  ( DPS_TRANSLOCK_X == parentPage.lockMode ) ) )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR,
                    "Error: doesn't have X lock on page:%d, rc:%d",
                    parentPage.page, rc ) ;
            goto error ;
         }
      }

      // find the split position
      rc = _splitPos ( pos, splitPos ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to get split position, rc = %d", rc ) ;
         goto error ;
      }
      PD_TRACE2 ( SDB__IXMEXT__SPLIT, PD_PACK_USHORT(pos),
                  PD_PACK_USHORT(splitPos) ) ;
      // allocate new extent
      rc = indexCB->allocExtent ( newExtentID ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to allocate new extent for index, rc = %d",
                  rc ) ;
         goto error ;
      }
      {
         // X lock on new page
         newPage.setPage( newExtentID ) ;
         rc = ixmLock( pixmContext, newPage.page, DPS_TRANSLOCK_X ) ;
         if ( rc )
         {
            goto error ;
         }
         bNewPageLocked = TRUE ;

         // initialize the header for the new extent
         _ixmExtent newExtent( newExtentID, _extentHead->_mbID, _pIndexSu ) ;

         // copy all keys from the split pos to new extent
         for ( UINT16 i = splitPos + 1 ; i < getNumKeyNode() ; i++ )
         {
            const ixmKeyNode *kn = getKeyNode(i) ;
            rc = newExtent._pushBack( kn->_rid,
                                      // ixmKey(((const CHAR*)_extentHead)
                                      //        +kn->_keyOffset),
                                      ixmKey( getKeyData(i) ),
                                      order, kn->_left, indexCB->getFlag() ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to push back key %d to new extent, "
                        "rc = %d", (INT32)i, rc ) ;
               goto error ;
            }
            if ( kn->isOutsideKey() )
            {
               ixmKeyNode *knW = writeKeyNode(i) ;
               knW->clearOutsideKeyFlag() ;
               bHasOutsideKey = TRUE ;
            }
         }
         // assign the right pointer
         newExtent._assignRight ( _extentHead->_right ) ;

#if defined (_DEBUG)
         rc = newExtent._validate(MAX, order) ;
#else
         rc = newExtent._validate(MIN, order) ;
#endif
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to validate the new extent, rc = %d",
                     rc ) ;
            goto error ;
         }

         // promote the split key into parent
         splitKey = getKeyNode( splitPos ) ;
         _assignRight ( splitKey->_left ) ;

         // is this root page
         if ( DMS_INVALID_EXTENT == getParent() )
         {
            // if this is root page, let's allocate another page
            dmsExtentID rootExtentID = DMS_INVALID_EXTENT ;
            // allocate new extent
            rc = indexCB->allocExtent ( rootExtentID ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to allocate new extent for index, "
                        "rc = %d", rc ) ;
               goto error ;
            }

            // X lock on new root page
            newRootPage.setPage( rootExtentID ) ;
            rc = ixmLock( pixmContext, newRootPage.page, DPS_TRANSLOCK_X ) ;
            if ( rc )
            {
               goto error ;
            }
            bNewRootPageLocked = TRUE ;

            // initialize the header for the new extent
            _ixmExtent rootExtent( rootExtentID, _extentHead->_mbID,
                                   _pIndexSu ) ;
            // promote the split key into parent, key._left point to the current
            // extent
            rc = rootExtent._pushBack ( splitKey->_rid,
                                        // ixmKey(((const CHAR*)_extentHead)+
                                        // splitKey->_keyOffset),
                                        ixmKey( getKeyData( splitPos ) ),
                                        order, _me,
                                        indexCB->getFlag() ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to promote split key into root, "
                        "rc = %d", rc ) ;
               goto error ;
            }
            // the _right is pointing to the splitted node
            rootExtent._assignRight ( newExtentID ) ;
#if defined (_DEBUG)
            rc = rootExtent._validate(MAX, order) ;
#else
            rc = rootExtent._validate(MIN, order) ;
#endif
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to validate the new root, rc = %d",
                        rc ) ;
               goto error ;
            }
            // set new root page
            indexCB->setRoot ( rootExtentID ) ;

            // release lock on new root page
            ixmUnlock( pixmContext, newRootPage.page, TRUE ) ;
            bNewRootPageLocked = FALSE ;
         }
         else
         {
            // when there is parent page exist (so we are not root)
            newExtent.setParent ( getParent(),
                                  IXM_INDEX_FLAG_NORMAL == indexCB->getFlag() );
            // get the parent extent
            _ixmExtent parentExtent( getParent(), _pIndexSu ) ;
            // do physical insert into it
            //
            // When get here, X lock must be held on both parent 
            // and current page. So, it is OK to pass dummy depth
            // and xLockLevel to following _insert()
            UINT32 dummyDepth = 0 ; 
            UINT32 dummyXLockLevel = 0 ;
            rc = parentExtent._insert( splitKey->_rid,
                                       // ixmKey(((const CHAR*)_extentHead)
                                       //        +splitKey->_keyOffset),
                                       ixmKey( getKeyData( splitPos ) ),
                                       order, TRUE, _me,
                                       newExtentID, indexCB,
                                       dummyDepth, dummyXLockLevel,
                                       pixmContext ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to promote into parent, rc = %d",
                        rc ) ;
               goto error ;
            }
         }

         if ( splitKey->isOutsideKey() )
         {
            ixmKeyNode * splitKeyW = writeKeyNode( splitPos ) ;
            splitKeyW->clearOutsideKeyFlag() ;
            bHasOutsideKey = TRUE ;
         }

         // now new page and(or) root page are created, and all keys are copied,
         // so we are safe to truncate
         newPos = pos ;
         // newPos will be the pos that after _reorg
         rc = _truncate ( splitPos, newPos, order ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to truncate index extent, rc = %d",
                     rc ) ;
            goto error ;
         }
         if ( FALSE == bSplitOnly )
         {
            PD_TRACE1 ( SDB__IXMEXT__SPLIT, PD_PACK_USHORT(newPos) ) ;
            // if the insert position is smaller than split position, it will be
            // insert into the original page
            if ( pos <= splitPos )
            {
               SDB_ASSERT ( 0xFFFF != newPos, "Invalid newPos" ) ;
               // insert into newPos since _truncate will call _reorg,
               // which will remove unused keys from original extent,
               // which may change newPos
               rc = insertHere ( newPos, rid, key, order, lchild, rchild,
                                 indexCB, pixmContext ) ;
            }
            else
            {
               // otherwise the insert will be performed in new page
               rc = newExtent.insertHere ( pos-splitPos-1, rid, key, order,
                                           lchild, rchild, indexCB,
                                           pixmContext ) ;
            }
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to insert into splitted page, rc = %d",
                        rc ) ;
               goto error ;
            }
         }

         // release lock on newExtent
         ixmUnlock( pixmContext, newPage.page, TRUE ) ;
         bNewPageLocked = FALSE ;

         // if this page contains outside key, remove it from map
         if ( bHasOutsideKey )
         {
#if defined (_DEBUG)
            PD_LOG ( PDDEBUG,
                     "Remove key from outside key page map while _split, "
                     "pageId:%d", _me ) ;
#endif
            _pOutKeyPageMap->rmItem( _me ) ;
         }
      }
   done :
      if ( bNewPageLocked )
      {
         ixmUnlock( pixmContext, newPage.page, TRUE ) ;
         bNewPageLocked = FALSE ;
      }
      if ( bNewRootPageLocked )
      {
         ixmUnlock( pixmContext, newRootPage.page, TRUE ) ;
         bNewRootPageLocked = FALSE ;
      }

      PD_TRACE_EXITRC ( SDB__IXMEXT__SPLIT, rc );
      return rc ;
   error :
      ixmUnlockAll( pixmContext ) ;
      bNewPageLocked     = FALSE ;
      bNewRootPageLocked = FALSE ;
      goto done ;
   }

   // truncate a page and leave totalNodes. Passin a newPos as
   // input/output, for any interested slot that may move its position.
   // For example the original layout looks like
   // <key1><removed><key2><key3><removed><key4>
   // so <key3> is at position 3.
   // After truncate(4)+reorg, the layout will be like
   // <key1><key2><key3>
   // so <key3> will be at position 2
   INT32 _ixmExtent::_truncate ( UINT16 totalNodes, UINT16 &newPos,
                                 const Ordering &order )
   {
      // caller MUST hold X lock on current page

      if ( totalNodes < getNumKeyNode() )
      {
         ixmExtentHead *pExtent = _extRW.writePtr<ixmExtentHead>() ;
         pExtent->_totalKeyNodeNum = totalNodes ;
         unsetCompact() ;
         return _reorg( order, newPos ) ;
      }
      return SDB_OK ;
   }

   // calculate the split position
   // pos is the position where we are trying to insert new record
   // splitPos is the returned value for where split should starts
   // if the pos is at end of the page, then we do 90/10 split, otherwise we do
   // 50/50 split
   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMEXT__SPLITPOS, "_ixmExtent::_splitPos" )
   INT32 _ixmExtent::_splitPos ( UINT16 pos, UINT16 &splitPos ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__IXMEXT__SPLITPOS );
      UINT16 rightSize = 0 ;
      UINT16 maxRightSize = 0, halfSize = 0 ;
      UINT16 totalKeySize = getTotalKeySize() ;
      PD_TRACE1 ( SDB__IXMEXT__SPLITPOS, PD_PACK_USHORT(totalKeySize) );
      splitPos = 1 ;

      // the caller MUST have X lock on current page

      // we should never call this function when there are less than two keys
      // (if that happen, after split and prompt to parent, we'll have empty
      // page
      if ( getNumKeyNode() < IXM_KEY_NODE_NUM_MIN )
      {
         PD_LOG ( PDERROR, "Only %d elements in the index",
                  (INT32)getNumKeyNode() ) ;
         dumpIndexExtentIntoLog () ;
         rc = SDB_SYS ;
         goto error ;
      }
      halfSize = totalKeySize / 2 ;
      if ( pos == _extentHead->_totalKeyNodeNum )
      {
         // if the new key is at end of the page, we do 90%+10% split
         maxRightSize = totalKeySize / 10 ;
      }
      else
      {
         // otherwise we do half-half split
         maxRightSize = totalKeySize / 2 ;
      }
      // calculate starting from right to left, and calculate the size of each
      // key
      for ( INT32 i = _extentHead->_totalKeyNodeNum-1 ; i >= 0 ; --i )
      {
         const ixmKeyNode *kn = getKeyNode(i) ;
         rightSize += ixmKey(getKeyData(i)).dataSize() ;
         if ( rightSize > maxRightSize )
         {
            splitPos = i ;
            break ;
         }
         // make sure it has enough space to store 'outside' key
         if ( kn->isOutsideKey() )
         {
            if ( rightSize >= halfSize )
            {
               splitPos = i ;
               break ;
            }
         }
      }
      if ( splitPos > getNumKeyNode() - 2 )
      {
         splitPos = getNumKeyNode() - 2 ;
      }
      PD_TRACE1 ( SDB__IXMEXT__SPLITPOS, PD_PACK_USHORT( splitPos ) ) ;
   done :
      PD_TRACE_EXITRC ( SDB__IXMEXT__SPLITPOS, rc );
      return rc ;
   error :
      goto done ;
   }

   // fix parent pointers for all child pages
   // loop through all keynodes, if the child exist, it will go to child and set
   // the parent extent to the current extent id
   // usually this happened during split for the new page
   INT32 _ixmExtent::_fixParentPtrs ( UINT16 startPos, UINT16 stopPos ) const
   {
      for ( UINT16 i = startPos; i < stopPos; i++ )
      {
         const ixmKeyNode *kn = getKeyNode ( i ) ;
         if ( DMS_INVALID_EXTENT != kn->_left )
         {
            /// add to page map
            _pPageMap->addItem( kn->_left, _me ) ;
         }
      }
      return SDB_OK ;
   }

   void _ixmExtent::_assignRight ( const dmsExtentID right )
   {
      ixmExtentHead *pHeader = _extRW.writePtr<ixmExtentHead>() ;
      pHeader->_right = right ;
      if ( DMS_INVALID_EXTENT != right )
      {
         // FIXME:
         // verify if lock child page
         _ixmExtent childExtent ( right, _pIndexSu ) ;
         childExtent.setParent ( _me ) ;
      }
   }

   void _ixmExtent::setChildExtentID ( UINT16 i, dmsExtentID extentID )
   {
      if ( i>_extentHead->_totalKeyNodeNum )
      {
         return ;
      }
      else if ( i == _extentHead->_totalKeyNodeNum )
      {
         _assignRight ( extentID ) ;
      }
      else
      {
         writeKeyNode(i)->_left = extentID ;
         if ( DMS_INVALID_EXTENT != extentID )
         {
            // REVISIT:
            // verify if need to lock child page
            _ixmExtent childExtent ( extentID, _pIndexSu ) ;
            childExtent.setParent ( _me ) ;
         }
      }
   }

   // simply push a key/rid into the page, the caller has to ensure the key+rid
   // is greater than every other nodes and the new keynode will be inserted
   // into the last position
   // the caller should also need to ensure that the bytes required is smaller
   // than free size
   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMEXT__PSHBACK, "_ixmExtent::_pushBack" )
   INT32 _ixmExtent::_pushBack ( const dmsRecordID &rid, const ixmKey &key,
                                 const Ordering &order, const dmsExtentID left,
                                 UINT16 idxState )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__IXMEXT__PSHBACK );

      // the caller MUST have X lock on current page

      UINT16 bytesNeeded = key.dataSize() + sizeof(ixmKeyNode) ;
      ixmExtentHead *pHeader = _extRW.writePtr<ixmExtentHead>( 0, _pageSize ) ;
      ixmKeyNode *kn = NULL ;
      // make sure we are not out of range
      if ( bytesNeeded > _extentHead->_totalFreeSize )
      {
         PD_LOG ( PDERROR, "Bytes needed should never smaller than "
                  "_totalFreeSize" ) ;
         dumpIndexExtentIntoLog () ;
         rc = SDB_SYS ;
         goto error ;
      }
      // if we have something in the page, let's make sure the new key is
      // greater than the last key in the page
      if ( getNumKeyNode() )
      {
         ixmKey lastkey ( getKeyData(getNumKeyNode()-1) ) ;
         if ( lastkey.woCompare(key, order) > 0 )
         {
            PD_LOG ( PDERROR, "New key smaller than the last key during "
                     "push" ) ;
            dumpIndexExtentIntoLog () ;
            rc = SDB_SYS ;
            goto error ;
         }
      }
      // allocate space and copy over key
      pHeader->_totalFreeSize -= sizeof(ixmKeyNode) ;
      _pIndexSu->decStatFreeSpace( pHeader->_mbID, sizeof(ixmKeyNode) ) ;
      kn = writeKeyNode(_extentHead->_totalKeyNodeNum) ;
      pHeader->_totalKeyNodeNum++ ;
      kn->_left = left ;

      if ( DMS_INVALID_EXTENT != kn->_left )
      {
         // Only add to page map if the index is in normal state. Otherwise,
         // set parent for the child in extent directly.
         // This is to avoid one concurrent problem: When multiple indices are
         // being created on the same collection, they may hit the code here
         // at the same time. Because during rebuilding, they only hold shared
         // lock of the collection. But the addItem will do WRITE operation to
         // the map, which is shared by all indices on the collection. This will
         // corrupt the map, and crash the program.
         if ( IXM_INDEX_FLAG_NORMAL == idxState )
         {
            /// add to page map
            _pPageMap->addItem( kn->_left, _me ) ;
         }
         else
         {
            // hold X on current page, it is OK to update child page without
            // latch
            ixmExtent child( kn->_left, _pIndexSu ) ;
            child.setParent( _me, FALSE ) ;
         }
      }

      kn->clearOutsideKeyFlag() ;

      kn->_rid = rid ;
      rc = _alloc ( key.dataSize(), kn->_keyOffset ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to allocate %d bytes in index",
                  key.dataSize()) ;
         goto error ;
      }
      ossMemcpy ( ((CHAR*)pHeader)+kn->_keyOffset,
                  key.data(), key.dataSize()) ;
   done :
      PD_TRACE_EXITRC ( SDB__IXMEXT__PSHBACK, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMEXT__VALIDATE2, "_ixmExtent::_validate" )
   INT32 _ixmExtent::_validate( ixmIndexCB *indexCB, dmsExtentID parent )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__IXMEXT__VALIDATE2 ) ;

      if ( _extentHead->_eyeCatcher[0] != IXM_EXTENT_EYECATCHER0 ||
           _extentHead->_eyeCatcher[1] != IXM_EXTENT_EYECATCHER1 )
      {
         PD_LOG ( PDERROR, "Invalid index extent eye catcher" ) ;
         dumpIndexExtentIntoLog () ;
         rc = SDB_SYS ;
         goto error ;
      }
      if ( _extentHead->_beginFreeOffset - sizeof(ixmExtentHead) -
           _extentHead->_totalKeyNodeNum*sizeof(ixmKeyNode) !=
           _extentHead->_totalFreeSize )
      {
         PD_LOG ( PDERROR, "Inconsistent free size" ) ;
         dumpIndexExtentIntoLog () ;
         rc = SDB_SYS ;
         goto error ;
      }
      if ( !(_extentHead->_flag & DMS_MB_FLAG_USED) )
      {
         PD_LOG ( PDERROR, "Invalid flag" ) ;
         dumpIndexExtentIntoLog () ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( DMS_INVALID_EXTENT != parent
           && getParent() != parent )
      {
         PD_LOG( PDERROR, "Invalid index extent parent" ) ;
         dumpIndexExtentIntoLog() ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( getMBID() != indexCB->getMBID() )
      {
         PD_LOG( PDERROR, "Invalid index extent mb id" ) ;
         dumpIndexExtentIntoLog() ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__IXMEXT__VALIDATE2, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // validate a page, there are 4 levels
   // NONE, MIN, MID, MAX
   // NONE will return SDB_OK right away
   // MIN will validate extent head information
   // MID will compare the first and last keys in the page and return failed if
   // the first key greater than last
   // MAX will compare each key to its next in sequence, and make sure all
   // previous keys are smaller or equal to the next
   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMEXT__VALIDATE, "_ixmExtent::_validate" )
   INT32 _ixmExtent::_validate ( _ixmExtentValidateLevel level,
                                 const Ordering &order ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__IXMEXT__VALIDATE );
      if ( NONE == level )
         goto done ;
      // min/mid/max
      if ( _extentHead->_eyeCatcher[0] != IXM_EXTENT_EYECATCHER0 ||
           _extentHead->_eyeCatcher[1] != IXM_EXTENT_EYECATCHER1 )
      {
         PD_LOG ( PDERROR, "Invalid index extent eye catcher" ) ;
         dumpIndexExtentIntoLog () ;
         rc = SDB_SYS ;
         goto error ;
      }
      if ( _extentHead->_beginFreeOffset - sizeof(ixmExtentHead) -
           _extentHead->_totalKeyNodeNum*sizeof(ixmKeyNode) !=
           _extentHead->_totalFreeSize )
      {
         PD_LOG ( PDERROR, "Inconsistent free size" ) ;
         dumpIndexExtentIntoLog () ;
         rc = SDB_SYS ;
         goto error ;
      }
      if ( !(_extentHead->_flag & DMS_MB_FLAG_USED) )
      {
         PD_LOG ( PDERROR, "Invalid flag" ) ;
         dumpIndexExtentIntoLog () ;
         rc = SDB_SYS ;
         goto error ;
      }

      // mid
      if ( MID == level )
      {
         ixmKey k1 ( getKeyData(0) ) ;
         ixmKey k2 ( getKeyData(_extentHead->_totalKeyNodeNum-1) ) ;
         if ( k1.woCompare(k2, order) > 0 )
         {
            PD_LOG ( PDERROR, "First key is greater than the last" ) ;
            dumpIndexExtentIntoLog () ;
            rc = SDB_SYS ;
            goto error ;
         }
      }
      else if ( MAX == level )
      {
         for ( UINT16 i = 0; i < _extentHead->_totalKeyNodeNum-1; i++ )
         {
            ixmKey k1 ( getKeyData(i)) ;
            ixmKey k2 ( getKeyData(i+1)) ;
            INT32 result = k1.woCompare(k2, order) ;
            if ( result > 0 )
            {
               PD_LOG ( PDERROR, "%d'th key is greater than its next",
                        (INT32)i ) ;
               dumpIndexExtentIntoLog () ;
               rc = SDB_SYS ;
               goto error ;
            }
            else if ( 0 == result )
            {
               dmsRecordID rid1 = getKeyNode(i)->_rid ;
               dmsRecordID rid2 = getKeyNode(i+1)->_rid ;
               if ( rid1.compare(rid2) >=0 )
               {
                  PD_LOG ( PDERROR, "%d'th key's RID is greater or equal to "
                           "the next", (INT32)i ) ;
                  dumpIndexExtentIntoLog () ;
                  rc = SDB_SYS ;
                  goto error ;
               }
            } //else if ( 0 == result )
         } //for (
      } //else if ( MAX == level )
   done :
      PD_TRACE_EXITRC ( SDB__IXMEXT__VALIDATE, rc );
      return rc ;
   error :
      goto done ;
   }
   // inline reorg an index page, wrapper for the other _reorg function
   INT32 _ixmExtent::_reorg (const Ordering &order)
   {
      UINT16 dummy = 0xFFFF ;
      return _reorg ( order, dummy ) ;
   }
   // inline reorg an index page, newPos represent the input/output for a key
   // after reorg happened
   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMEXT__REORG, "_ixmExtent::_reorg" )
   INT32 _ixmExtent::_reorg (const Ordering &order, UINT16 &newPos)
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__IXMEXT__REORG );

      ixmExtentHead *pHeader = NULL ;
      UINT16 beginFreeOffset = _pageSize-1 ;
      UINT16 totalKeyNodeNum = 0 ;
      UINT16 totalFreeSize = beginFreeOffset - sizeof(ixmExtentHead) ;
      CHAR   buffer[DMS_PAGE_SIZE_MAX] ;

      if ( isCompact() )
      {
         goto done ;
      }

      // the caller MUST have X lock on current page

      pHeader = _extRW.writePtr<ixmExtentHead>( 0, _pageSize ) ;

      // loop through all keys in the page
      for ( UINT16 i = 0 ; i < pHeader->_totalKeyNodeNum ; i++ )
      {
         ixmKeyNode *kn = writeKeyNode(i) ;
         INT32 keyDataSize = 0 ;
         // if the slot doesn't same as previous, and that is what we are
         // looking for, then let's set newPos to the new position after reorg
         if ( newPos == i )
         {
            newPos = totalKeyNodeNum ;
         }
         // if there is no child and it's unused, let's skip it ( that means it
         // will not be copied and count, so it's actually deleted)
         if ( kn->isUnused() && DMS_INVALID_EXTENT == kn->_left )
         {
            /// When all node is unused, should keep the one node.
            /// Otherwise the page will has no key node
            if ( totalKeyNodeNum > 0 ||
                 i < pHeader->_totalKeyNodeNum - 1 )
            {
               continue ;
            }
         }
         totalFreeSize -= sizeof(ixmKeyNode) ;
         // copy the key
         // ixmKey key ( ((const CHAR*)pHeader)+kn->_keyOffset) ;
         // ixmKey key ( getKeyData(i) ) ;
         const CHAR *keyData = getKeyData( i ) ;
         ixmKey key ( keyData ) ;
         keyDataSize = key.dataSize() ;
         if ( (INT32)beginFreeOffset - keyDataSize < 0 ||
              (INT32)totalFreeSize - keyDataSize < 0 )
         {
            PD_LOG ( PDERROR, "key is too large" ) ;
            dumpIndexExtentIntoLog () ;
            rc = SDB_SYS ;
            goto error ;
         }
         beginFreeOffset -= keyDataSize ;
         totalFreeSize -= keyDataSize ;
         ossMemcpy ( &buffer[beginFreeOffset],
                     // ((const CHAR*)pHeader)+kn->_keyOffset,
                     // getKeyData( i ),  
                     keyData,  
                     keyDataSize ) ;
         kn->_keyOffset = beginFreeOffset ;
         // if this key/slot is marked as outside key
         // reset the flag
         if ( kn->isOutsideKey() )
         {
            kn->clearOutsideKeyFlag() ;
#if defined (_DEBUG)
            SDB_ASSERT( ( _pOutKeyPageMap->findItem( _me, NULL ) ),
                        "Page doesn't exist in map" ) ;
            PD_LOG ( PDDEBUG,
                     "Remove key from outside key page map while _reorg, "
                     "pageId:%d", _me ) ;
#endif
            _pOutKeyPageMap->rmItem( _me ) ;
         }
         // copy the slot
         if ( totalKeyNodeNum != i )
         {
            ossMemcpy ( ((CHAR*)pHeader) + sizeof(ixmExtentHead) +
                        totalKeyNodeNum*sizeof(ixmKeyNode),
                        ((const CHAR*)pHeader) + sizeof(ixmExtentHead) +
                        i*sizeof(ixmKeyNode),
                        sizeof(ixmKeyNode)) ;
         }
         ++totalKeyNodeNum ;
      }
      // handle the situation where requested pos is right-most
      if ( pHeader->_totalKeyNodeNum == newPos )
      {
         newPos = totalKeyNodeNum ;
      }
      else if ( pHeader->_totalKeyNodeNum < newPos )
      {
         newPos = 0xFFFF ;
      }

      PD_TRACE1 ( SDB__IXMEXT__REORG, PD_PACK_USHORT( newPos ) ) ;

      pHeader->_beginFreeOffset = beginFreeOffset ;
      pHeader->_totalKeyNodeNum = totalKeyNodeNum ;
      _pIndexSu->decStatFreeSpace( pHeader->_mbID,
                                   pHeader->_totalFreeSize ) ;
      pHeader->_totalFreeSize = totalFreeSize ;
      _pIndexSu->addStatFreeSpace( pHeader->_mbID,
                                   pHeader->_totalFreeSize ) ;
      ossMemcpy ( ((CHAR*)pHeader)+beginFreeOffset,
                  &buffer[beginFreeOffset],
                  _pageSize - beginFreeOffset ) ;
#if defined (_DEBUG)
      rc = _validate(MAX, order) ;
#else
      rc = _validate(MIN, order) ;
#endif
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to validate the new extent, rc = %d", rc ) ;
         goto error ;
      }
      setCompact() ;
   done :
      PD_TRACE_EXITRC ( SDB__IXMEXT__REORG, rc );
      return rc ;
   error :
      goto done ;
   }

   INT32 _ixmExtent::_alloc ( INT32 requestSpace, UINT16 &beginOffset )
   {
      if ( requestSpace > (INT32)getFreeSize() )
      {
         return SDB_IXM_NOSPC ;
      }
      ixmExtentHead *pHeader = _extRW.writePtr<ixmExtentHead>() ;
      pHeader->_beginFreeOffset -= requestSpace ;
      pHeader->_totalFreeSize -= requestSpace ;
      _pIndexSu->decStatFreeSpace( pHeader->_mbID, requestSpace ) ;
      beginOffset = pHeader->_beginFreeOffset ;
      return SDB_OK ;
   }


   //
   // . if a page contains outside key, do split, then release all locks
   //   and the caller shall restart the operation from scratch.
   // . if the free space in a page is smaller than the size of ixmKeyNode,
   //   i.e., the minimum free space size, then try reorg. 
   //   If he free space is still smaller than minimum free space size after
   //   rerog, then do split, release alllocks and caller restart the operation
   // Returns
   //   SDB_IXM_HAS_SPLITTED -- split has been done, caller shall restart
   //   SDB_OK               -- . page has no outside key
   //                           . free space is greater than one slot
   //   SDB_IXM_REORG_DONE   -- the pos has been changed after reorg,
   //                           caller may retry find/locate or restart 
   //   SDB_DPS_TRANS_LOCK_INCOMPATIBLE -- failed to upgrade / aquire page lock
   //                                      caller shall release all locks and
   //                                      restart this operation   
   //   SDB_TIMEOUT                     -- failed to upgrade / aquire page lock
   //                                      caller shall release all locks and
   //                                      restart this operation 
   //   other ERRORs                    -- caller falls its error handling logic
   INT32 _ixmExtent::_doSplitIfOutsideKeyExist
   (
      const dmsRecordID  & rid,
      const ixmKey       & key,
      const Ordering     & order,
      UINT16               pos,
      INT8               & currentPageLockMode,
      INT8               & parentPageLockMode,
      ixmIndexCB         * indexCB,
      const UINT32         depth,
      UINT32             & xLockLevel,
      _ixmContext        * pixmContext
   )
   {
      INT32 rc = SDB_OK ;
      const UINT32 minimumBytesNeeded = sizeof(ixmKeyNode) ;
      dmsExtentID ch = DMS_INVALID_EXTENT ;
      UINT16 newPos  = pos ;
      _ixmLockInfo currentPage( _me ), parentPage( getParent() ) ;
      BOOLEAN bAllLockReleased     = FALSE ;
      const UINT32 savedXLockLevel = xLockLevel ;

      // caller shall lock on current page
      if ( FALSE == pixmContext->getLockHeldInfo( currentPage ) )
      {
         rc = SDB_SYS ;
         PD_LOG ( PDERROR, "Error: dosen't have lock on index page:%d, rc=%d",
                  currentPage.page, rc ) ;
         SDB_DASSERT ( FALSE,
                       "_doSplitIfOutsideKeyExist: Current page "
                       "has not been locked !" ) ;
         goto error ;
      }

      ch = getChildExtentID( pos ) ;

      // check if this page has outside key/data.
      // if so do split, then release all locks goto done and restart
      // from scratch
      if ( _pOutKeyPageMap->findItem( currentPage.page, NULL ) )
      {
         if ( ( 0 == xLockLevel ) || ( xLockLevel >= depth ) )
         {
            xLockLevel = depth ;
         }

         // try acquire X lock on parent page
         if ( parentPage.isValid() ) 
         {
            // make sure parent page is locked
            SDB_DASSERT( pixmContext->isLocking( parentPage.page ),
                         "Doesn't have lock on parent page !" ) ;
            SDB_DASSERT( ( depth > 0 ),
                         "depath must be greater than 0 " ) ;
            if ( xLockLevel >= depth - 1 )
            {
               xLockLevel = depth - 1 ;
            }

            if ( DPS_TRANSLOCK_X != parentPageLockMode )
            {
               // unlock current page first
               ixmUnlock( pixmContext, currentPage.page, TRUE ) ;
               currentPageLockMode = DPS_TRANSLOCK_MAX ;

               // update / get X lock on parent page
               rc = ixmLock( pixmContext, parentPage.page, DPS_TRANSLOCK_X ) ;
               if ( rc )
               {
                  // it may fail to upgrade to X due to dead-lock detection,
                  // release locks on all pages and retry from scratch
                  ixmUnlockAll( pixmContext ) ;
                  bAllLockReleased = TRUE ;
                  goto error ;
               }
               parentPageLockMode = DPS_TRANSLOCK_X ;

               // get X lock on current page
               rc = ixmLock( pixmContext, currentPage.page, DPS_TRANSLOCK_X ) ;
               if ( SDB_OK != rc )
               {
                  ixmUnlockAll( pixmContext ) ;
                  bAllLockReleased = TRUE ;
                  goto error ;
               }
               currentPageLockMode = DPS_TRANSLOCK_X ;

               // after release lock on current page, it is possible
               // the current page is changed by another thread.
               // Now return to caller ( _locateForDelete or _insert ) to
               // locate the right position.
               //
               // Here, just reuse the SDB_IXM_REORG_DONE as rc,
               // since caller is check this value for re-finding/locating
               // the position
               rc = SDB_IXM_REORG_DONE ;
               goto done ;
            }
         }

         // upgrade / get X lock on current page
         if ( DPS_TRANSLOCK_X != currentPageLockMode )
         {
            rc = ixmLock( pixmContext, currentPage.page, DPS_TRANSLOCK_X ) ;
            if ( SDB_OK != rc )
            {
               // it may fail to upgrade to X due to dead-lock detection,
               // release locks on all pages and retry from scratch
               ixmUnlockAll( pixmContext ) ;
               bAllLockReleased = TRUE ;
               goto error ;
            }
            currentPageLockMode = DPS_TRANSLOCK_X ;
         }

         // do split(), then release all locks goto done
         // and restart from scratch
         rc = _split ( newPos, rid, key, order,
                       DMS_INVALID_EXTENT, DMS_INVALID_EXTENT,
                       indexCB, pixmContext, TRUE ) ;
         if ( SDB_OK == rc )
         {
            // release locks on all pages and restart from beginning
            ixmUnlockAll( pixmContext ) ;
            bAllLockReleased = TRUE ;
            rc = SDB_IXM_HAS_SPLITTED ;

            // restore previous xLockLevel after _split successfully done
            xLockLevel = savedXLockLevel ;
            goto done ;
         }
         else
         {
            // release locks on all pages
            ixmUnlockAll( pixmContext ) ;
            bAllLockReleased = TRUE ;
            goto error ;
         }
      }
      else if ( getFreeSize() < minimumBytesNeeded )
      {
         if ( ( 0 == xLockLevel ) || ( xLockLevel >= depth ) )
         {
            xLockLevel = depth ;
         }

         // upgrade to X lock on current page
         if ( DPS_TRANSLOCK_X != currentPageLockMode )
         {
            rc = ixmLock( pixmContext, currentPage.page, DPS_TRANSLOCK_X ) ;
            if ( SDB_OK != rc )
            {
               // it may fail to upgrade to X due to dead-lock detection,
               // release locks on all pages and retry from scratch
               ixmUnlockAll( pixmContext ) ;
               bAllLockReleased = TRUE ;
               goto error ;
            }
            currentPageLockMode = DPS_TRANSLOCK_X ;
         }

         // do reorg
         rc = _reorg( order, newPos ) ;
         if ( rc )
         {
            ixmUnlockAll( pixmContext ) ;
            bAllLockReleased = TRUE ;
            PD_LOG( PDERROR, "index extent:%d reorg failed, rc:%d",
                    currentPage.page, rc ) ;
            goto error ;
         }
         else
         {
            // after reorg, the pos may points to an element
            // with different lchild, in this case, do find again
            if ( getChildExtentID( newPos ) != ch )
            {
               rc = SDB_IXM_REORG_DONE ;
               goto done ;
            }
         }

         // check if have mininum space after reorg
         if ( getFreeSize() < minimumBytesNeeded )
         {
            // try acquire X lock on parent page
            if ( parentPage.isValid() )
            {
               // make sure parent page is locked
               SDB_DASSERT( pixmContext->isLocking( parentPage.page ),
                            "Doesn't have lock on parent page !" ) ;
               SDB_DASSERT( ( depth > 0 ),
                            "depath must be greater than 0 " ) ;
               if ( xLockLevel >= depth - 1 )
               {
                  xLockLevel = depth - 1 ;
               }

               if ( DPS_TRANSLOCK_X != parentPageLockMode )
               {
                  // unlock current page first
                  ixmUnlock( pixmContext, currentPage.page, TRUE ) ;
                  currentPageLockMode = DPS_TRANSLOCK_MAX ;

                  // upgrade / get X lock on parent page
                  rc = ixmLock( pixmContext, parentPage.page, DPS_TRANSLOCK_X );
                  if ( rc )
                  {
                     // release locks on all pages and restart from scratch
                     ixmUnlockAll( pixmContext ) ;
                     bAllLockReleased = TRUE ;
                     goto error ;
                  }
                  parentPageLockMode = DPS_TRANSLOCK_X ;

                  // get X lock on current page
                  rc = ixmLock( pixmContext, currentPage.page, DPS_TRANSLOCK_X);
                  if ( SDB_OK != rc )
                  {
                     // it may fail to upgrade to X due to dead-lock detection,
                     // release locks on all pages and retry from scratch
                     ixmUnlockAll( pixmContext ) ;
                     bAllLockReleased = TRUE ;
                     goto error ;
                  }
                  currentPageLockMode = DPS_TRANSLOCK_X ;

                  // after release lock on current page, it is possible
                  // the current page is changed by another thread.
                  // Now return to caller ( _locateForDelete or _insert ) to
                  // locate the right position.
                  //
                  // Here, just reuse the SDB_IXM_REORG_DONE as rc,
                  // since caller is check this value for re-finding/locating
                  // the position
                  rc = SDB_IXM_REORG_DONE ;
                  goto done ;
               }
            }

            // do split(), then release all locks goto done
            // and restart from scratch 
            rc = _split ( newPos, rid, key, order,
                          DMS_INVALID_EXTENT, DMS_INVALID_EXTENT,
                          indexCB, pixmContext, TRUE ) ;
            if ( SDB_OK == rc )
            {
               // release locks on all pages and restart from beginning
               ixmUnlockAll( pixmContext ) ;
               bAllLockReleased = TRUE ;
               rc = SDB_IXM_HAS_SPLITTED ;

               // restore previous xLockLevel after _split successfully done
               xLockLevel = savedXLockLevel ;
               goto done ;
            }
            else
            {
               // release locks on all pages and restart from beginning
               ixmUnlockAll( pixmContext ) ;
               bAllLockReleased = TRUE ;
               goto error ;
            }
         }
      }
   done :
      if ( bAllLockReleased )
      {
         parentPageLockMode  = DPS_TRANSLOCK_MAX ;
         currentPageLockMode = DPS_TRANSLOCK_MAX ;
      }

      PD_TRACE_EXITRC ( SDB__IXMEXT__INSERT, rc ) ;
      return rc ;
   error :
      // if error occurs, release locks on all pages
      if ( ! bAllLockReleased )
      {
         ixmUnlockAll( pixmContext ) ;
         bAllLockReleased = TRUE ;
      }
      goto done ;
   }

   // find + insertHere
   // Internal function, insert an rid/key pair into the current page. This
   // function will perform find() for the given key/rid pair, and recursively
   // call itself if there's child page associate with the keynodes until hit
   // leaf. In leaf it will call insertHere()
   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMEXT__INSERT, "_ixmExtent::_insert" )
   INT32 _ixmExtent::_insert ( const dmsRecordID & rid,
                               const ixmKey      & key,
                               const Ordering    & order,
                               BOOLEAN             dupAllowed,
                               dmsExtentID         lchild,
                               dmsExtentID         rchild,
                               ixmIndexCB        * indexCB,
                               UINT32            & depth,
                               UINT32            & xLockLevel,
                               _ixmContext       * pixmContext,
                               utilWriteResult   * pResult )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__IXMEXT__INSERT ) ;

      INT32 keyFoundPos = -1 ;
      BOOLEAN sameFound = FALSE ;
      UINT16 pos = 0 ;
      dmsExtentID ch = DMS_INVALID_EXTENT ;
      const ixmKeyNode *kn = NULL ;
      INT32 keySize ;

      _ixmLockInfo childPage, currentPage( _me ), parentPage( getParent() ) ;
      BOOLEAN bAllLockReleased = FALSE ;
      INT8 parentPageLockMode  = DPS_TRANSLOCK_MAX,
           currentPageLockMode = DPS_TRANSLOCK_MAX,
           childPageLockMode   = DPS_TRANSLOCK_MAX ;

      // sanity check
      keySize = key.dataSize() ;
      if ( keySize > _pIndexSu->indexKeySizeMax() )
      {
         PD_LOG ( PDERROR, "key size[%d] must be less than or equal to [%d]",
                  keySize, _pIndexSu->indexKeySizeMax() ) ;
         rc = SDB_IXM_KEY_TOO_LARGE ;
         goto error ;
      }
      if ( key.dataSize() <= 0 )
      {
         PD_LOG ( PDERROR, "key size must be greater than 0" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( indexCB->notNull() && key.hasNullOrUndefined() )
      {
         rc = SDB_IXM_KEY_NOTNULL ;
         PD_LOG ( PDERROR, "Any field of index key cannot be null "
                  "or does not exist, rc: %d", rc ) ;
         goto error ;
      }

   retry :
      // make sure current page is locked
      // verify if have lock on current page
      if ( FALSE == pixmContext->getLockHeldInfo( currentPage ) )
      {
         rc = SDB_SYS ;
         PD_LOG ( PDERROR, "Error: dosen't have lock on index page:%d, rc=%d",
                  _me, rc ) ;

         SDB_DASSERT ( FALSE,
                      "_insert: Current page has not been locked !" ) ;
         goto error ;
      }
      currentPageLockMode = currentPage.lockMode ;

      if ( parentPage.isValid() && pixmContext->getLockHeldInfo( parentPage ) )
      {
         parentPageLockMode = parentPage.lockMode ;
      }
      else
      {
         parentPageLockMode  = DPS_TRANSLOCK_MAX ;
      }

      // try to locate where the insert should happen
      rc = find ( indexCB, key, rid, order, pos, keyFoundPos, sameFound ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Error happened during find, rc = %d", rc ) ;
         goto error ;
      }

      if ( sameFound )
      {
         kn = getKeyNode( pos ) ;
         if ( kn->isUnused() )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Page[%d]'s key node[%d] should be used",
                    _me, pos ) ;
            dumpIndexExtentIntoLog() ;
            goto error ;
         }
         PD_LOG ( PDINFO, "same key + rid is already in index" ) ;
         // have same key/rid point to same record
         rc = SDB_IXM_IDENTICAL_KEY ;
         goto error ;
      }
      else if ( !dupAllowed && -1 != keyFoundPos )
      {
         kn = getKeyNode( keyFoundPos ) ;
         // if we find duplicate, let's check whether the key includes all
         // Undefined. If this is the case, it's a special case that user
         // doesn't define those keys, so we should allow it proceed ( which
         // may violate unique definition ). If we restricted this behavior,
         // user cannot insert records that does not contains the keys twice,
         // which is very violating "schemaless"
         if ( kn->isUsed() && ( indexCB->enforced() || !key.isUndefined () ) )
         {
            // this error only returned when dupAllowed == FALSE
            // this error represent duplicate key is not allowed and
            // duplicate key is detected
#ifdef _DEBUG
            PD_LOG ( PDWARNING, "Duplicate key is detected with rid(%d, %d), "
                     "page:%d, keynode:%d, insert rid:(%d, %d)",
                     kn->_rid._extent, kn->_rid._offset, _me, keyFoundPos,
                     rid._extent, rid._offset ) ;
#else
            PD_LOG ( PDINFO, "Duplicate key is detected with rid(%d, %d), "
                     "page:%d, keynode:%d, insert rid:(%d, %d)",
                     kn->_rid._extent, kn->_rid._offset, _me, keyFoundPos,
                     rid._extent, rid._offset ) ;
#endif
            if ( pResult )
            {
               pResult->setCurRID( rid ) ;
               pResult->setPeerRID( kn->_rid ) ;
            }
            rc = SDB_IXM_DUP_KEY ;
            goto error ;
         }
      }

      ch = getChildExtentID( pos ) ;

      // _doSplitIfOutsideKeyExist does following checks
      // 1 if this page has outside key/data,
      //     . acquire X locks on this page and its parent,
      //     . do split,
      //     . release all locks, returns rc as SDB_IXM_HAS_SPLITTED,
      //    -- caller shall restart from scratch for this case
      // 2 check if this page has minimum free space, one slot, i.e., the sizeof
      //   ixmKeyNode.
      //     if so,
      //        return SDB_OK
      //     if not,
      //        . acquire X lock on this page and try reorg
      //        . if pos is changed after rerog,
      //             if so, return SDB_IXM_REORG_DONE
      //             -- the caller may find the pos again and retry, or restart
      //             if not, check if has minimum free space after rerog
      //                if so, return SDB_OK
      //                if not, try X lock on parent,
      //                        do split if acquired X on parent, 
      //                        release all locks if split successfully and
      //                        return rc as SDB_IXM_HAS_SPLITTED 
      rc = _doSplitIfOutsideKeyExist( rid, key, order, pos,
                                      currentPageLockMode,
                                      parentPageLockMode,
                                      indexCB,
                                      depth,
                                      xLockLevel,
                                      pixmContext ) ;
      if ( rc )
      {
         // we have performed reorg and found the position we supposed to
         // insert got a left pointer, so let's reperform find
         if ( SDB_IXM_REORG_DONE == rc )
         {
            rc = SDB_OK ;
            goto retry ;
         }
         goto error ;
      }

      // release lock on parent
      if ( parentPage.isValid() )
      {
         ixmUnlock( pixmContext, parentPage.page, TRUE ) ;
         parentPageLockMode = DPS_TRANSLOCK_MAX ;
      }

      // if there's no child, of course we will insert into the current page
      // and if there is child, but rchild is specified, this means the function
      // is called by the child extent in split (when prompt the last key to the
      // parent, rchild represent the newly created page), in this case we also
      // simply insert it into the page instead of traversing down
      if ( DMS_INVALID_EXTENT == ch || DMS_INVALID_EXTENT != rchild )
      {
         if ( ( 0 == xLockLevel ) || ( xLockLevel >= depth ) )
         {
            xLockLevel = depth ;
         }

         // upgrade to X lock on current index page
         if ( DPS_TRANSLOCK_X != currentPageLockMode )
         {
            rc = ixmLock( pixmContext, currentPage.page, DPS_TRANSLOCK_X ) ;
            if ( SDB_OK != rc )
            {
               // it may fail to upgrade to X due to dead-lock detection,
               // release locks on all pages and restart from scratch 
               ixmUnlockAll( pixmContext ) ;
               bAllLockReleased = TRUE ;
               goto error ;
            }
            currentPageLockMode = DPS_TRANSLOCK_X ;
         }

         rc = insertHere( pos, rid, key, order, lchild, rchild, indexCB,
                          pixmContext ) ;
         if ( rc )
         {
            // we have performed reorg and found the position we supposed to
            // insert got a left pointer, so let's reperform find
            if ( SDB_IXM_REORG_DONE == rc )
            {
               rc = SDB_OK ;
               goto retry ;
            }
            PD_LOG ( PDERROR, "Failed to insert, rc = %d", rc ) ;
            goto error ;
         }
      }
      // otherwise let's traverse down
      else
      {
         depth ++ ;

         // acquire proper lock on child page
         INT8 lockMode = currentPageLockMode ;
         if ( DPS_TRANSLOCK_S == lockMode )
         {
            if ( ( IXM_X_LOCK_START_LEVEL <= depth ) ||
                 ( ( xLockLevel > 0 ) && ( xLockLevel >= depth ) ) )
            {
               lockMode = DPS_TRANSLOCK_X ;
               if ( ( 0 == xLockLevel ) || ( xLockLevel >= depth ) )
               {  
                  xLockLevel = depth ;
               }
            }
         }

         childPage.setPage( ch ) ;
         rc = ixmLock( pixmContext, childPage.page, lockMode ) ;
         if ( rc )
         {
            // in case fails to acquire lock on child page, release locks
            // on all pages and restart from scratch.
            ixmUnlockAll( pixmContext ) ;
            bAllLockReleased = TRUE ;
            goto error ;
         }
         childPageLockMode = lockMode ;

         rc = _ixmExtent(ch, _pIndexSu)._insert( rid, key, order, dupAllowed,
                                                 lchild, rchild, indexCB,
                                                 depth,
                                                 xLockLevel,
                                                 pixmContext, pResult ) ;
         if ( rc )
         {
            if ( ( SDB_IXM_HAS_SPLITTED != rc ) && 
                 ( SDB_TIMEOUT != rc ) &&
                 ( SDB_DPS_TRANS_LOCK_INCOMPATIBLE != rc ) )
            {
               PD_LOG ( PDERROR, "Failed to insert, rc = %d", rc ) ;
               goto error ;
            }
         }
      }
   done :
      if ( bAllLockReleased )
      {
         childPageLockMode   = DPS_TRANSLOCK_MAX ;
         parentPageLockMode  = DPS_TRANSLOCK_MAX ; 
         currentPageLockMode = DPS_TRANSLOCK_MAX ;
      }
      if ( ( DPS_TRANSLOCK_MAX != childPageLockMode ) &&
           childPage.isValid() )
      {
         ixmUnlock( pixmContext, childPage.page, TRUE ) ;
         childPageLockMode = DPS_TRANSLOCK_MAX ;
      }
      if ( DPS_TRANSLOCK_MAX != currentPageLockMode )
      {
         ixmUnlock( pixmContext, currentPage.page, TRUE ) ;
         currentPageLockMode = DPS_TRANSLOCK_MAX ;
      }
      if ( ( DPS_TRANSLOCK_MAX != parentPageLockMode ) &&
           parentPage.isValid() )
      {
         ixmUnlock( pixmContext, parentPage.page, TRUE ) ;
         parentPageLockMode = DPS_TRANSLOCK_MAX ;
      }

      PD_TRACE_EXITRC ( SDB__IXMEXT__INSERT, rc ) ;
      return rc ;
   error :
      // if error occurs, release locks on all pages
      if ( ! bAllLockReleased )
      {
         ixmUnlockAll( pixmContext ) ;
         bAllLockReleased = TRUE ;
      }
      goto done ;
   }


   // clean up the outside key by splitting that index page
   INT32 _ixmExtent::cleanUpOutsideKey ( const ixmKey      & key,
                                         const dmsRecordID & rid,
                                         const Ordering    & order,
                                         dmsExtentID         pageToSplit,
                                         BOOLEAN             dupAllowed,
                                         ixmIndexCB        * indexCB,
                                         UINT32            & xLockLevel,
                                        _ixmContext        * pixmContext )
   {
      // make sure current page is locked
      SDB_DASSERT( pixmContext->isLocking( _me ),
                   "Doesn't have lock on index page !" ) ;
      UINT32 depth = 0 ;
      return _cleanUpOutsideKey ( rid, key, order, pageToSplit,
                                  dupAllowed,
                                  DMS_INVALID_EXTENT, DMS_INVALID_EXTENT,
                                  indexCB,
                                  depth, xLockLevel, pixmContext ) ;
   }


   INT32 _ixmExtent::_cleanUpOutsideKey ( const dmsRecordID & rid,
                                          const ixmKey      & key,
                                          const Ordering    & order,
                                          dmsExtentID         pageToSplit,
                                          BOOLEAN             dupAllowed,
                                          dmsExtentID         lchild,
                                          dmsExtentID         rchild,
                                          ixmIndexCB        * indexCB,
                                          UINT32            & depth,
                                          UINT32            & xLockLevel,
                                          _ixmContext       * pixmContext )
   {
      INT32 rc          = SDB_OK ;
      INT32 keyFoundPos = -1 ;
      BOOLEAN sameFound = FALSE ;
      UINT16 pos        = 0 ;
      dmsExtentID ch    = DMS_INVALID_EXTENT ;

      _ixmLockInfo childPage, currentPage( _me ), parentPage( getParent() ) ;
      BOOLEAN bAllLockReleased = FALSE ;
      INT8 parentPageLockMode  = DPS_TRANSLOCK_MAX,
           currentPageLockMode = DPS_TRANSLOCK_MAX,
           childPageLockMode   = DPS_TRANSLOCK_MAX ;

   retry :
      // make sure current page is locked
      // verify if have lock on current page
      if ( FALSE == pixmContext->getLockHeldInfo( currentPage ) )
      {
         rc = SDB_SYS ;
         PD_LOG ( PDERROR, "Error: dosen't have lock on index page:%d, rc=%d",
                  _me, rc ) ;
         SDB_DASSERT ( FALSE,
                       "_cleanUpOutsideKey: Current page hasn't been locked !");
         goto error ;
      }
      currentPageLockMode = currentPage.lockMode ;

      if ( parentPage.isValid() && pixmContext->getLockHeldInfo( parentPage ) )
      {
         parentPageLockMode = parentPage.lockMode ;
      }
      else
      {
         parentPageLockMode  = DPS_TRANSLOCK_MAX ;
      }

      // try to locate where the insert should happen
      rc = find ( indexCB, key, rid, order, pos, keyFoundPos, sameFound ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Error happened during find, rc = %d", rc ) ;
         goto error ;
      }

      ch = getChildExtentID( pos ) ;

      // _doSplitIfOutsideKeyExist does following checks
      // 1 if this page has outside key/data,
      //     . acquire X locks on this page and its parent,
      //     . do split,
      //     . release all locks, returns rc as SDB_IXM_HAS_SPLITTED,
      // 2 check if this page has minimum free space, one slot, i.e.,
      //   the sizeof ixmKeyNode.
      //     if so,
      //        return SDB_OK
      //     if not,
      //        . acquire X lock on this page and try reorg
      //        . if pos is changed after rerog,
      //             if so, return SDB_IXM_REORG_DONE
      //             -- the caller may find the pos again and retry, or restart
      //             if not, check if has minimum free space after rerog
      //                if so, return SDB_OK
      //                if not, acquire X locks on this page and its parent
      //                        do split if acquired X on parent, 
      //                        release all locks if split successfully and
      //                        return rc as SDB_IXM_HAS_SPLITTED 
      rc = _doSplitIfOutsideKeyExist( rid, key, order, pos,
                                      currentPageLockMode,
                                      parentPageLockMode,
                                      indexCB,
                                      depth,
                                      xLockLevel,
                                      pixmContext ) ;
      if ( rc )
      {
         // we have performed reorg and found the position we supposed to
         // insert got a left pointer, so let's reperform find
         if ( SDB_IXM_REORG_DONE == rc )
         {
            rc = SDB_OK ;
            goto retry ;
         }
         goto error ;
      }

      // release lock on parent
      if ( parentPage.isValid() )
      {
         ixmUnlock( pixmContext, parentPage.page, TRUE ) ;
         parentPageLockMode = DPS_TRANSLOCK_MAX ;
      }

      if ( ( DMS_INVALID_EXTENT == ch ) ||
           ( DMS_INVALID_EXTENT != rchild ) ||
           ( !dupAllowed && -1 != keyFoundPos ) ||
           sameFound ||
           ( SDB_IXM_HAS_SPLITTED == rc ) )
      {
         if ( ( DMS_INVALID_EXTENT != pageToSplit ) && ( pageToSplit == _me ) )
         {
            if ( ( 0 == xLockLevel ) || ( xLockLevel >= depth ) )
            {
               xLockLevel = depth ;
            }
            if ( xLockLevel >= depth - 1 )
            {
               xLockLevel = depth - 1 ;
            }
         }
      }
      // otherwise let's traverse down
      else
      {
         depth ++ ;

         // acquire proper lock on child page
         INT8 lockMode = currentPageLockMode ;
         if ( DPS_TRANSLOCK_S == lockMode )
         {
            if ( ( IXM_X_LOCK_START_LEVEL <= depth ) ||
                 ( ( xLockLevel > 0 ) && ( xLockLevel >= depth ) ) )
            {
               lockMode = DPS_TRANSLOCK_X ;
               if ( ( 0 == xLockLevel ) || ( xLockLevel >= depth ) )
               {  
                  xLockLevel = depth ;
               }
            }
         }

         childPage.setPage( ch ) ;
         rc = ixmLock( pixmContext, childPage.page, lockMode ) ;
         if ( rc )
         {
            // in case fails to acquire lock on child page, release locks
            // on all pages and restart from scratch.
            ixmUnlockAll( pixmContext ) ;
            bAllLockReleased = TRUE ;
            goto error ;
         }
         childPageLockMode = lockMode ;

         rc = _ixmExtent(ch, _pIndexSu)._cleanUpOutsideKey( rid, key, order,
                                                            pageToSplit,
                                                            dupAllowed,
                                                            lchild, rchild,
                                                            indexCB,
                                                            depth,
                                                            xLockLevel,
                                                            pixmContext ) ;
         if ( rc )
         {
            if ( ( SDB_IXM_HAS_SPLITTED != rc ) && 
                 ( SDB_TIMEOUT != rc ) &&
                 ( SDB_DPS_TRANS_LOCK_INCOMPATIBLE != rc ) )
            {
               PD_LOG( PDERROR, "Failed to clean up outside key, rc = %d", rc );
               goto error ;
            }
         }
      }
   done :
      if ( bAllLockReleased )
      {
         childPageLockMode   = DPS_TRANSLOCK_MAX ;
         parentPageLockMode  = DPS_TRANSLOCK_MAX ; 
         currentPageLockMode = DPS_TRANSLOCK_MAX ;
      }
      if ( ( DPS_TRANSLOCK_MAX != childPageLockMode ) &&
           childPage.isValid() )
      {
         ixmUnlock( pixmContext, childPage.page, TRUE ) ;
         childPageLockMode = DPS_TRANSLOCK_MAX ;
      }
      if ( DPS_TRANSLOCK_MAX != currentPageLockMode )
      {
         ixmUnlock( pixmContext, currentPage.page, TRUE ) ;
         currentPageLockMode = DPS_TRANSLOCK_MAX ;
      }
      if ( ( DPS_TRANSLOCK_MAX != parentPageLockMode ) &&
           parentPage.isValid() )
      {
         ixmUnlock( pixmContext, parentPage.page, TRUE ) ;
         parentPageLockMode = DPS_TRANSLOCK_MAX ;
      }

      return rc ;
   error :
      // if error occurs, release locks on all pages
      if ( ! bAllLockReleased )
      {
         ixmUnlockAll( pixmContext ) ;
         bAllLockReleased = TRUE ;
      }
      goto done ;
   }


   // Find and remove specific key from an index
   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMEXT_UNINDEX, "_ixmExtent::unindex" )
   INT32 _ixmExtent::unindex ( const ixmKey      & key,
                               const dmsRecordID & rid,
                               const Ordering    & order,
                               ixmIndexCB        * indexCB,
                               BOOLEAN           & result,
                               UINT32            & xLockLevel,
                               _ixmContext       * pixmContext )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__IXMEXT_UNINDEX );
      BOOLEAN found ;
      ixmRecordID indexrid ;
      result = FALSE ;
      UINT32 depth = 0 ;

      _ixmLockInfo currentPage( _me ), parentPage, newFound ;
      BOOLEAN bAllLocksReleased = FALSE,
              bParentPageLocked = FALSE ;

      // Caller MUST acquire lock on current page
      // make sure current page is locked before _locate
      if ( FALSE == pixmContext->getLockHeldInfo( currentPage ) )
      {
         rc = SDB_SYS ;
         PD_LOG ( PDDEBUG,
                  "Haven't acquire lock on index page:%d before delete."
                  "while unindex, rc: %d.",
                  currentPage.page, rc  );
         SDB_DASSERT ( FALSE, "Page has not been locked !"  );

         goto error ;
      }

      // when _locate returns, it holds S/X lock on the page found,
      // as well as the S/X lock on parent page.
      rc = _locateForDelete ( key, rid, order, indexrid, found, 1, indexCB,
                              depth,       // current index tree level
                              xLockLevel,  // level need to take X lock
                              pixmContext ) ;
      if ( rc )
      {
         if ( ( SDB_DPS_TRANS_LOCK_INCOMPATIBLE != rc ) &&
              ( SDB_TIMEOUT != rc ) &&
              ( SDB_IXM_HAS_SPLITTED != rc ) )
         {
            PD_LOG ( PDERROR, "Failed to locate key and rid" ) ;
         }
         goto error ;
      }

      if ( found )
      {
         newFound.setPage( indexrid._extent ) ;

         SDB_DASSERT ( pixmContext->getLockHeldInfo( newFound ),
                       "Page has not been locked !"  );

         if ( ( 0 == xLockLevel ) || ( xLockLevel >= depth ) )
         { 
            xLockLevel = depth ;
         }

         ixmExtent childExtent( indexrid._extent, _pIndexSu ) ;

         parentPage.setPage( childExtent.getParent() ) ;

         if ( parentPage.isValid() )
         { 
            SDB_DASSERT ( pixmContext->getLockHeldInfo( parentPage ),
                          "Page has not been locked !" ) ;

            // in case the currentPage is still locked,
            // release the lock if currentPage is neither
            // the new found page nor parent of new found page 
            if ( ( currentPage.page != indexrid._extent ) &&
                 ( currentPage.page != childExtent.getParent() ) )
            {
               ixmUnlock( pixmContext, currentPage.page, TRUE ) ;
            }

            // try to get X lock on parent page of the newFound page
            // if there is only one key on the newFound page
            if ( 1 == childExtent.getNumKeyNode() )
            {
               SDB_ASSERT( ( depth > 0 ),
                           "Index tree level must be greater than 0" ) ;

               if ( xLockLevel >= depth - 1 )
               {
                  xLockLevel = depth - 1 ;
               }
               // the index page locking protocol is taking parent first,
               // then child. Currently we hold lock on child, now we want to
               // lock parent lock with X mode. To avoid deadlock, do tryX.
               // If fails release all locks and re-try from scratch. 
               rc = ixmTryLock( pixmContext, parentPage.page, DPS_TRANSLOCK_X );
               if ( SDB_OK != rc )
               {
                  ixmUnlockAll( pixmContext ) ;
                  bAllLocksReleased = TRUE ;
                  goto error ;
               }
               bParentPageLocked = TRUE ;
            }
            else
            {
               // new found page has more than one key note
               // so can we release the lock on its parent
               ixmUnlock( pixmContext, parentPage.page, TRUE ) ;
            }
         }
         else
         {
            // in case the currentPage is still locked,
            // release the lock if currentPage is not
            // the new found page
            if ( currentPage.page != indexrid._extent )
            {
               ixmUnlock( pixmContext, currentPage.page, TRUE ) ;
            }
         }

         // upgrade X lock on new found page
         rc = ixmLock( pixmContext, newFound.page, DPS_TRANSLOCK_X ) ;
         if ( SDB_OK != rc )
         {
            // it may fail to upgrade to X due to dead-lock detection,
            // release all locks and restart from scratch
            ixmUnlockAll( pixmContext ) ;
            bAllLocksReleased = TRUE ;
            goto error ;
         }

         rc = childExtent._delKeyAtPos ( indexrid._slot, order, indexCB,
                                         pixmContext ) ;
         if ( rc )
         {
            ixmUnlockAll( pixmContext ) ;
            bAllLocksReleased = TRUE ;
            if ( ( SDB_DPS_TRANS_LOCK_INCOMPATIBLE != rc ) &&
                 ( SDB_TIMEOUT != rc ) &&
                 ( SDB_IXM_HAS_SPLITTED != rc ) ) 
            {
               PD_LOG ( PDERROR, "failed to delete key, rc = %d" ) ;
            }
            goto error ;
         }
         result = TRUE ;
      }
   done :
      if ( FALSE == bAllLocksReleased )
      {
         ixmUnlockAll( pixmContext ) ;
         bAllLocksReleased = TRUE ;
      }
      PD_TRACE_EXITRC ( SDB__IXMEXT_UNINDEX, rc );
      return rc ;
   error :
      goto done ;
   }

   // delete a key from a given position
   // caller must make sure the pos is smaller than the total number of keys in
   // the page, and there is no left pointer on the key
   // this function will physically remove keynode on the page
   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMEXT__DELKEYATPOS1, "_ixmExtent::_delKeyAtPos" )
   INT32 _ixmExtent::_delKeyAtPos ( UINT16 pos )
   {
      // the caller must have X lock on current page
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__IXMEXT__DELKEYATPOS1 );

      ixmKeyNode *kn = NULL ;
      ixmExtentHead *pHeader = _extRW.writePtr<ixmExtentHead>( 0, _pageSize ) ;
      if ( pos >= getNumKeyNode() )
      {
         PD_LOG ( PDERROR, "pos out of range, pos=%d, totalKey=%d",
                  (INT32)pos, (INT32)getNumKeyNode() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      kn = writeKeyNode( pos ) ;
      if ( DMS_INVALID_EXTENT != kn->_left )
      {
         PD_LOG ( PDERROR, "left pointer must be NULL" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      pHeader->_totalFreeSize += sizeof(ixmKeyNode) ;
      _pIndexSu->addStatFreeSpace( pHeader->_mbID, sizeof(ixmKeyNode) ) ;
      pHeader->_totalKeyNodeNum-- ;

      // if the key is an outside key, then remove it from outside key page map
      if ( kn->isOutsideKey() )
      {
#if defined (_DEBUG)
         PD_LOG ( PDDEBUG,
                  "Remove key from outside key page map while _delKeyAtPos, "
                  "pageId:%d", _me ) ;
#endif
         kn->clearOutsideKeyFlag() ;
         _pOutKeyPageMap->rmItem( _me ) ;
      }

      ossMemmove ( (CHAR*)kn, (const CHAR*)getKeyNode(pos+1),
                   sizeof(ixmKeyNode)*(pHeader->_totalKeyNodeNum-pos) ) ;
      unsetCompact() ;

   done :
      PD_TRACE_EXITRC ( SDB__IXMEXT__DELKEYATPOS1, rc );
      return rc ;
   error :
      goto done ;
   }

   // delete a key from page at pos, caller do NOT need to validate left pointer
   // and root
   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMEXT__DELKEYATPOS2, "_ixmExtent::_delKeyAtPos" )
   INT32 _ixmExtent::_delKeyAtPos ( UINT16           pos,
                                    const Ordering & order,
                                    ixmIndexCB     * indexCB,
                                    _ixmContext    * pixmContext )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__IXMEXT__DELKEYATPOS2 );
      dmsExtentID left = DMS_INVALID_EXTENT ;
      BOOLEAN result = FALSE ;

      _ixmLockInfo currentPage( _me ), parentPage( getParent() ) ;
      BOOLEAN bCurrentPageLocked = FALSE, bParentPageLocked = FALSE ;

      /// verify if current page have been locked
      if ( ! ( pixmContext->getLockHeldInfo( currentPage ) &&
               ( DPS_TRANSLOCK_X == currentPage.lockMode ) ) )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR,
                 "Error: doesn't lock page:%d with X mode, rc:%d",
                 _me, rc ) ;

         SDB_DASSERT( FALSE, "Current page is not locked" ) ;

         goto error ;
      }
      bCurrentPageLocked = TRUE ;

      // caller MUST hold X lock on parent page if current 
      // page only have one key
      if ( parentPage.isValid() && ( 1 == getNumKeyNode() ) )
      {
         if ( ! ( pixmContext->getLockHeldInfo( parentPage ) &&
                  ( DPS_TRANSLOCK_X == parentPage.lockMode ) ) )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR,
                    "Error: doesn't lock page:%d with X mode, rc:%d",
                    parentPage.page, rc ) ;

            SDB_DASSERT( FALSE, "Parent page is not locked" ) ;

            goto error ;
         }
         bParentPageLocked = TRUE ;
      }

      if ( pos >= getNumKeyNode() )
      {
         PD_LOG ( PDERROR, "pos out of range, pos=%d, totalKey=%d",
                 (INT32)pos, (INT32)getNumKeyNode() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      left = getKeyNode( pos )->_left ;

      // All code path within this block will endup jumping to either
      // done or error
      if ( 1 == getNumKeyNode() )
      {
         if ( DMS_INVALID_EXTENT == left &&
              DMS_INVALID_EXTENT == _extentHead->_right )
         {
            // first let's remove the key since we knows the left pointer is
            // NULL
            rc = _delKeyAtPos ( pos ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to delete at pos %d", (INT32) pos ) ;
               goto error ;
            }
            if ( DMS_INVALID_EXTENT != getParent() )
            {
               // if we have only 1 key and there's no left/right children in
               // the page, and we are not root, let's first attempt to share
               // some keys from neighbors
               rc = _mayBalanceWithNeighbors ( order, indexCB, result ) ;
               if ( rc )
               {
                  PD_LOG ( PDERROR, "Failed to balance with neighbors" ) ;
                  goto error ;
               }
               if ( !result )
               {
                  // if we don't have neighbors to share, let's remove the node
                  rc = _delExtent ( indexCB ) ;
                  if ( rc )
                  {
                     PD_LOG ( PDERROR, "Failed to delete extent for the "
                              "index" ) ;
                     goto error ;
                  }
               }
            }

            // when we get here, we already balanced with neighbor or deleted
            // the extent, or it's root page, let's return
            goto done ;
         }

         // when we get here, that means either left pointer is not null or
         // there's right pointer in the page, then let's attempt to do delete
         // internal key
         rc = _deleteInternalKey ( pos, order, indexCB, pixmContext ) ;
         if ( rc )
         {
            if ( ( SDB_DPS_TRANS_LOCK_INCOMPATIBLE != rc ) &&
                 ( SDB_TIMEOUT != rc ) )
            {
               PD_LOG ( PDERROR, "Failed to delete internal key, rc=%d", rc ) ;
            }
            goto error ;
         }

         goto done ;
      }  // end of if ( 1 == getNumKeyNode() )

      // when we get there, that means we have more than 1 key in the extent
      if ( DMS_INVALID_EXTENT == left )
      {
         // No left, we can remove the key from the extent
         rc = _delKeyAtPos ( pos ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to delete at pos %d", (INT32)pos ) ;
            goto error ;
         }
         rc = _mayBalanceWithNeighbors ( order, indexCB, result ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to balance with neighbors" ) ;
            goto error ;
         }
      }
      else
      {
         // if the left pointer is not null, let's do internal delete, this may
         // touch/move the next key
         rc = _deleteInternalKey ( pos, order, indexCB, pixmContext ) ;
         if ( rc )
         {
            if ( ( SDB_DPS_TRANS_LOCK_INCOMPATIBLE != rc ) &&
                 ( SDB_TIMEOUT != rc ) )
            {
               PD_LOG ( PDERROR, "Failed to delete internal key , rc=%d", rc ) ;
            }
            goto error ;
         }
      }
   done :
      if ( bCurrentPageLocked )
      {
         ixmUnlock( pixmContext, currentPage.page, TRUE ) ;
         bCurrentPageLocked = FALSE ;
      }
      if ( parentPage.isValid() && bParentPageLocked )
      {
         ixmUnlock( pixmContext, parentPage.page, TRUE ) ;
         bParentPageLocked = FALSE ;
      }

      PD_TRACE_EXITRC ( SDB__IXMEXT__DELKEYATPOS2, rc );
      return rc ;
   error :
      goto done ;
   }

   // we should do rebalance and merge in this code, but let's leave it for now
   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMEXT__MAYBLCWITHNGB, "_ixmExtent::_mayBalanceWithNeighbors" )
   INT32 _ixmExtent::_mayBalanceWithNeighbors ( const Ordering &order,
                                                ixmIndexCB *indexCB,
                                                BOOLEAN &result )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__IXMEXT__MAYBLCWITHNGB );
      result = FALSE ;
      //UINT16 pos ;
      //BOOLEAN mayBalanceRight ;
      //BOOLEAN mayBalanceLeft ;
      // let's return if it's root

      if ( DMS_INVALID_EXTENT == getParent() )
      {
         goto done ;
      }
      // {
         // get the parent extent
         // ixmExtent parent( getParent(), _pIndexSu ) ;

         // find the key pointing to this extent
         // rc = parent._findChildExtent( _me, pos ) ;

         // if we can't find the key, something really bad happened
         // if ( rc )
         // {
         //    PD_LOG ( PDERROR, "Unable to find the extent in it's parent" ) ;
         //    goto error ;
         // }
      // }

      // if we are not the _right, and our next slot got child, we may do right
      // balance
      /*mayBalanceRight = (pos < parent.getNumKeyNode() &&
                         parent.getChildExtentID(pos+1) !=
                            DMS_INVALID_EXTENT ) ;
      // if we are not the first, and our previous slot got child, we may do
      // left balance
      mayBalanceLeft = (pos>0 && parent.getChildExtentID(pos-1) !=
                            DMS_INVALID_EXTENT ) ;*/
      /*
      // attempt to balance child
      if ( mayBalanceRight )
      {
         // for right balance, we merge pos and pos+1
         rc = parent._tryBalanceChildren ( pos, order, indexCB, result ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to try balance children" ) ;
            goto error ;
         }
         if ( result )
            goto done ;
      }
      if ( mayBalanceLeft )
      {
         // for left balance, we merge pos-1 and pos
         rc = parent._tryBalanceChildren ( pos-1, order, indexCB, result ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to try balance children" ) ;
            goto error ;
         }
         if ( result )
            goto done ;
      }
      // attempt to merge child
      if ( mayBalanceRight )
      {
         // for right balance, we merge pos and pos+1
         rc = parent._doMergeChildren ( pos, order, indexCB, result ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to try balance children" ) ;
            goto error ;
         }
         goto done ;
      }
      if ( mayBalanceLeft )
      {
         // for left balance, we merge pos-1 and pos
         rc = parent._doMergeChildren ( pos-1, order, indexCB, result ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to try balance children" ) ;
            goto error ;
         }
         goto done ;
      } */
   done :
      PD_TRACE_EXITRC ( SDB__IXMEXT__MAYBLCWITHNGB, rc );
      return rc ;
   // error :
   //    goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMEXT__DELEXT, "_ixmExtent::_delExtent" )
   INT32 _ixmExtent::_delExtent ( ixmIndexCB *indexCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__IXMEXT__DELEXT );
      UINT16 pos = 0 ;
      UINT16 mbID = 0 ;
      UINT16 freeSize = 0 ;

      // if we are root, we simply return
      if ( DMS_INVALID_EXTENT != getParent() )
      {
         // get the parent extent
         ixmExtent parent( getParent(), _pIndexSu ) ;

         // find the key pointing to this extent
         rc = parent._findChildExtent ( _me, pos ) ;

         // if we can't find the key, something really bad happened
         if ( rc )
         {
            PD_LOG ( PDERROR, "Unable to find the extent in it's parent" ) ;
            goto error ;
         }
      
         // update child id in parent page 
         parent.setChildExtentID ( pos, DMS_INVALID_EXTENT ) ;
         mbID = _extentHead->_mbID ;
         freeSize = _extentHead->_totalFreeSize ;

         rc = indexCB->freeExtent ( _me ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Unable to free extent" ) ;
            goto error ;
         }
         _pIndexSu->decStatFreeSpace( mbID, freeSize ) ;
         _pPageMap->rmItem( _me ) ;

         // if this page contains outside key, remove it from map
         if ( _pOutKeyPageMap->findItem( _me, NULL ) )
         {
#if defined (_DEBUG)
             PD_LOG ( PDDEBUG,
                      "Remove key from outside key page map while _delExtent, "
                      "pageId:%d", _me ) ;
#endif
            _pOutKeyPageMap->rmItem( _me ) ;
         }
      }

   done :
      PD_TRACE_EXITRC ( SDB__IXMEXT__DELEXT, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMEXT__FNDCHLDEXT, "_ixmExtent::_findChildExtent" )
   INT32 _ixmExtent::_findChildExtent ( dmsExtentID childExtent,
                                        UINT16 &pos ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__IXMEXT__FNDCHLDEXT );
      if ( _extentHead->_right == childExtent )
      {
         pos = getNumKeyNode() ;
         goto done ;
      }
      for ( UINT16 i =0 ; i<getNumKeyNode(); i++ )
      {
         if ( getChildExtentID (i) == childExtent )
         {
            pos = i ;
            goto done ;
         }
      }
      rc = SDB_IXM_KEY_NOTEXIST ;
   done :
      PD_TRACE1 ( SDB__IXMEXT__FNDCHLDEXT, PD_PACK_USHORT( pos ) );
      PD_TRACE_EXITRC ( SDB__IXMEXT__FNDCHLDEXT, rc );
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMEXT__DELITNKEY, "_ixmExtent::_deleteInternalKey" )
   INT32 _ixmExtent::_deleteInternalKey ( UINT16 pos, const Ordering &order,
                                          ixmIndexCB *indexCB,
                                          _ixmContext * pixmContext )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__IXMEXT__DELITNKEY );
      dmsExtentID lchild = getChildExtentID(pos) ;
      dmsExtentID rchild = getChildExtentID(pos+1) ;
      ixmRecordID nextIndexKey ;
      INT32 direction ;

      _ixmLockInfo currentPage( _me ), newPage,
                   parentPage( getParent() ) ;
      BOOLEAN bParentPageLocked  = FALSE,
              bAllLocksReleased  = FALSE ;

      // caller MUST hold X lock on parent page if current
      // page only have one key
      if ( parentPage.isValid() && ( 1 == getNumKeyNode() ) )
      {
         if ( ! ( pixmContext->getLockHeldInfo( parentPage ) &&
                  ( DPS_TRANSLOCK_X == parentPage.lockMode ) ) )
         {
            rc = SDB_SYS ;
            ixmUnlockAll( pixmContext ) ;
            bAllLocksReleased = TRUE ;
            PD_LOG( PDERROR,
                    "Error: doesn't lock page:%d with X mode, rc:%d",
                    parentPage.page, rc ) ;

            SDB_DASSERT ( FALSE, "Parent page hasn't been locked !" ) ;
            goto error ;
         }
      }
      parentPage.reset() ;

      if ( DMS_INVALID_EXTENT == lchild && DMS_INVALID_EXTENT == rchild )
      {
         PD_LOG ( PDERROR, "both left/right child are NULL" ) ;
         dumpIndexExtentIntoLog () ;
         rc = SDB_SYS ;
         goto error ;
      }

      direction = (DMS_INVALID_EXTENT == lchild)?1:-1 ;
      nextIndexKey._extent = _me ;
      nextIndexKey._slot = pos ;

      // when advanceDown returns, the found page shall be locked
      rc = _advanceDown ( nextIndexKey, direction, pixmContext,
                          IXM_ADVANCEDOWN_OP_MODE_DEL ) ;
      if ( rc )
      {
         if ( ( SDB_DPS_TRANS_LOCK_INCOMPATIBLE == rc ) || 
              ( SDB_TIMEOUT == rc ) )
         {
            // _advanceDown fails due to not be able to acquire lock 
            // release all locks and restart from scratch
            ixmUnlockAll( pixmContext ) ;
            bAllLocksReleased = TRUE ;
         }
         else
         {
            ixmUnlockAll( pixmContext ) ;
            bAllLocksReleased = TRUE ;

            PD_LOG ( PDERROR, "Failed to find the next index key, rc:%d", rc ) ;
         }
         goto error ;
      }

      // since we already checked that either lchild or rchild exist,
      // nextIndexKey should never be NULL here
      if ( nextIndexKey.isNull() )
      {
         ixmUnlockAll( pixmContext ) ;
         bAllLocksReleased = TRUE ;

         PD_LOG ( PDERROR, "advance key shouldn't be NULL" ) ;
         dumpIndexExtentIntoLog () ;
         rc = SDB_SYS ;
         goto error ;
      }

      {
         // verify the new found page is locked 
         newPage.setPage( nextIndexKey._extent ) ;
         if ( ! ( pixmContext->getLockHeldInfo( newPage ) &&
                  ( DPS_TRANSLOCK_X == newPage.lockMode ) ) )
         {
            ixmUnlockAll( pixmContext ) ;
            bAllLocksReleased = TRUE ;
            rc = SDB_SYS ;
            PD_LOG ( PDERROR, "Page:%d hasn't been locked, rc = %d",
                     newPage.page, rc ) ;
            SDB_DASSERT ( FALSE, "Page hasn't been locked !" ) ;
            goto error ;
         }

         // now the nextExtent contains the next key
         ixmExtent nextExtent ( nextIndexKey._extent, _pIndexSu ) ;

         // if the new found page has only one key node,
         // its parent must have X lock on it
         parentPage.setPage( nextExtent.getParent() ) ;
         if ( parentPage.isValid() )
         { 
            bParentPageLocked = pixmContext->getLockHeldInfo( parentPage ) ;
            if ( 1 == nextExtent.getNumKeyNode() )
            {
               if ( ! ( bParentPageLocked && 
                        ( DPS_TRANSLOCK_X == parentPage.lockMode ) ) )
               {
                  ixmUnlockAll( pixmContext ) ;
                  bAllLocksReleased = TRUE ;
                  rc = SDB_SYS ;
                  PD_LOG ( PDERROR,
                           "Parent page:%d hasn't been locked, rc = %d",
                           parentPage.page, rc ) ;
                  SDB_DASSERT ( FALSE, "Parent page hasn't been locked !" ) ;
                  goto error ;
               }
            }
         }

         // if the next key do have child or its next (including _right)
         // do have child, let's use a simple way to set the key unused (a
         // better way could be recursively swap+_deleteInternalKey /
         // _delKeyAtPos)
         if ( nextExtent.getChildExtentID ( nextIndexKey._slot ) !=
                    DMS_INVALID_EXTENT ||
              nextExtent.getChildExtentID ( nextIndexKey._slot+1 ) !=
                    DMS_INVALID_EXTENT )
         {
            writeKeyNode(pos)->setUnused() ;

            // if it is possible, release current page lock
            if ( parentPage.isValid() &&
                 ( 1 == nextExtent.getNumKeyNode() ) )
            {
               // if parent of the new found page is locked,
               // then lock on current page can be released
               // only when this page is neither the new found
               // page nor parent of the new found page
               if ( ( currentPage.page != newPage.page ) &&
                    ( currentPage.page != parentPage.page ) )
               {
                  ixmUnlock( pixmContext, currentPage.page, TRUE ) ;
               }
            }
            else
            {
               // if the new found page contains more key nodes
               // then no need to lock its parent. In this case,
               // we may release current page if it is not same
               // as new found page
               if ( currentPage.page != newPage.page )
               {
                  ixmUnlock( pixmContext, currentPage.page, TRUE ) ;
               }
            }
         }
         else
         {
            // if there's no child for the next key, let's replace the next key
            // to the current keynode and remove the next key from its original
            // extent
            const ixmKeyNode *kn = nextExtent.getKeyNode
                  ( nextIndexKey._slot ) ;
            ixmKey nextKey ( nextExtent.getKeyData(nextIndexKey._slot)) ;
            if ( !kn )
            {
               ixmUnlockAll( pixmContext ) ;
               bAllLocksReleased = TRUE ;

               PD_LOG ( PDERROR, "Failed to find key node" ) ;
               dumpIndexExtentIntoLog () ;
               rc = SDB_SYS ;
               goto error ;
            }

            rc = _setInternalKey ( pos, kn->_rid, nextKey, order,
                                   getChildExtentID ( pos ) ,
                                   getChildExtentID ( pos+1 ) ,
                                   indexCB, pixmContext ) ;
            if ( rc )
            {
               ixmUnlockAll( pixmContext ) ;
               bAllLocksReleased = TRUE ;

               PD_LOG ( PDERROR, "failed to set internal key" ) ;
               goto error ;
            }

            // if it is possible, release current page lock
            if ( parentPage.isValid() && 
                 ( 1 == nextExtent.getNumKeyNode() ) )
            {
               // if parent of the new found page is locked,
               // then lock on current page can be released
               // only when this page is neither the new found
               // page nor parent of the new found page
               if ( ( currentPage.page != newPage.page ) &&
                    ( currentPage.page != parentPage.page ) )
               {
                  ixmUnlock( pixmContext, currentPage.page, TRUE ) ;
               }
            }
            else
            {
               // if the new found page contains more key nodes
               // then no need to lock its parent. In this case,
               // we may release current page if it is not same
               // as new found page
               if ( currentPage.page != newPage.page )
               {
                  ixmUnlock( pixmContext, currentPage.page, TRUE ) ;
               }
            }

            rc = nextExtent._delKeyAtPos ( nextIndexKey._slot, order,
                                           indexCB, pixmContext ) ;
            if ( rc )
            {
               ixmUnlockAll( pixmContext ) ;
               bAllLocksReleased = TRUE ;

               if ( ( SDB_DPS_TRANS_LOCK_INCOMPATIBLE != rc ) &&
                    ( SDB_TIMEOUT != rc ) )
               {
                  PD_LOG ( PDERROR, "failed to delete key" ) ;
               }
               goto error ;
            }
         }

         // release new page lock 
         if ( newPage.isValid() )
         {
            ixmUnlock( pixmContext, newPage.page, TRUE ) ;
         }

         // release lock on parent page of new found page
         if ( parentPage.isValid() && bParentPageLocked )
         {
            ixmUnlock( pixmContext, parentPage.page, TRUE ) ;
            bParentPageLocked = FALSE ;
         }
      }
   done :
      if ( FALSE == bAllLocksReleased )
      {
         // release lock on new page
         if ( newPage.isValid() )
         {
            ixmUnlock( pixmContext, newPage.page, TRUE ) ;
         } 
         // release lock on parent page of new found page
         if ( parentPage.isValid() && bParentPageLocked )
         {
            ixmUnlock( pixmContext, parentPage.page, TRUE ) ;
            bParentPageLocked = FALSE ;
         }
      }

      PD_TRACE_EXITRC ( SDB__IXMEXT__DELITNKEY, rc );
      return rc ;
   error :
      goto done ;
   }

   // Move to the next key based on the direction
   // keyRID is for input and output
   // direction=1 means forward, -1 means backward
   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMEXT_ADVANCE, "_ixmExtent::advance" )
   INT32 _ixmExtent::advance ( ixmRecordID &keyRID, INT32 direction,
                               _ixmContext * pixmContext ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__IXMEXT_ADVANCE );
      INT32 adj ;
      dmsExtentID childExtent ;
      dmsExtentID parent ;

      _ixmLockInfo parentPage, childPage ;
      BOOLEAN bParentLocked = FALSE, bChildLocked = FALSE ;

      adj = direction < 0 ? 1:0 ;

      // make sure current page is locked
      childPage.setPage( _me ) ;
      if ( FALSE == pixmContext->getLockHeldInfo( childPage )  )
      {
         rc = ixmLock( pixmContext, childPage.page, DPS_TRANSLOCK_S ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
         bChildLocked = TRUE ;
      }

      rc = _advanceDown( keyRID, direction, pixmContext,
                         IXM_ADVANCEDOWN_OP_MODE_QRY ) ;
      if ( SDB_OK == rc )
      {
         // if the new found page is not same as child page ( _me )
         // release lock on child page
         if (    ( keyRID._extent != childPage.page )
              && ( !keyRID.isNull() ) )
         {
            // release child apge
            ixmUnlock( pixmContext, childPage.page, TRUE ) ;
            bChildLocked = FALSE ;
         }
      }
      // here we are at end of bucket, we should go to parent
      else if ( ( SDB_IXM_EOC == rc ) && ( !keyRID.isNull() ) )
      {
         rc = SDB_OK ;

         childExtent = _me ;
         parent = getParent() ;
         childPage.setPage( childExtent ) ;

         while ( TRUE )
         {
            // we don't continue if getting to root
            if ( DMS_INVALID_EXTENT == parent )
               break ;

            // try lock S on parent
            parentPage.setPage( parent ) ;
            rc = ixmTryLock( pixmContext, parentPage.page, DPS_TRANSLOCK_S ) ;
            if ( SDB_OK != rc )
            {
               // release child apge and retry from beginning
               ixmUnlock( pixmContext, childPage.page, TRUE ) ;
               bChildLocked = FALSE ;
               goto error;
            }
            bParentLocked = TRUE ;

            // release child apge
            ixmUnlock( pixmContext, childPage.page, TRUE ) ;
            bChildLocked = FALSE ;

            ixmExtent parentExtent ( parent, _pIndexSu ) ;

            // switch
            childPage = parentPage ;
            bChildLocked = bParentLocked ;
            parentPage.reset() ;
            bParentLocked = FALSE ;

            // in the parent extent, let's see who's _left pointing to
            // the current extent, then that's what we are looking for
            for ( UINT16 i=0; i<parentExtent.getNumKeyNode(); i++ )
            {
               if ( childExtent == parentExtent.getChildExtentID(i+adj) )
               {
                  keyRID._slot = i ;
                  keyRID._extent = parent ;
                  goto done ;
               }
            }

            // we should never hit here in forward search, because each _left
            // must have a valid keynode, unless it's at _right node
            if ( direction > 0 &&
                 parentExtent._extentHead->_right != childExtent )
            {
               PD_LOG ( PDERROR,"Invalid tree structure" ) ;
               dumpIndexExtentIntoLog () ;
               rc = SDB_SYS ;
               goto error ;
            }

            childExtent = parent ;
            parent = parentExtent.getParent() ;
         }

         // when we get here, it means there's no other keys avaliable
         keyRID.reset() ;
         if ( bChildLocked )
         {
            ixmUnlock( pixmContext, childPage.page, TRUE ) ;
            bChildLocked = FALSE ;
         }
         if ( bParentLocked )
         {
            ixmUnlock( pixmContext, parentPage.page, TRUE ) ;
            bParentLocked = FALSE ;
         }
      }
      else
      {
         goto error ;
      }
   done :
      if ( !keyRID.isNull() )
      {
         if ( ( keyRID._extent != childPage.page ) &&
              ( DMS_INVALID_EXTENT != childPage.page ) )
         {
            ixmUnlock( pixmContext, childPage.page, TRUE ) ;
            bChildLocked = FALSE ;
         }
         if ( ( keyRID._extent != parentPage.page ) &&
              ( DMS_INVALID_EXTENT != parentPage.page ) )
         {
            ixmUnlock( pixmContext, parentPage.page, TRUE ) ;
            bParentLocked = FALSE ;
         }
      }

      PD_TRACE_EXITRC ( SDB__IXMEXT_ADVANCE, rc );
      return rc ;
   error :
      if ( bChildLocked )
      {
         ixmUnlock( pixmContext, childPage.page, TRUE ) ;
         bChildLocked = FALSE ;
      }
      if ( bParentLocked )
      {
         ixmUnlock( pixmContext, parentPage.page, TRUE ) ;
         bParentLocked = FALSE ;
      }
      goto done ;
   }

   // Move to the next key based on the direction
   // keyRID is for input and output
   // direction=1 means forward, -1 means backward
   INT32 _ixmExtent::_advanceDown ( ixmRecordID &keyRID,
                                    INT32 direction,
                                    _ixmContext * pixmContext,
                                    IXM_ADVANCEDOWN_OP_MODE_TYPE opMode ) const
   {
      INT32 rc = SDB_OK ;
      INT32 adj ;
      INT32 ko ;
      dmsExtentID nextDown ;

      _ixmLockInfo curPage( _me ), nextPage ;
      BOOLEAN bCurLocked = FALSE, bNextLocked = FALSE  ;

      SDB_ASSERT ( (( IXM_ADVANCEDOWN_OP_MODE_QRY == opMode ) ||
                    ( IXM_ADVANCEDOWN_OP_MODE_DEL == opMode )),
                   "Invalid operation mode !" ) ;

      INT8 lockMode = ( opMode == IXM_ADVANCEDOWN_OP_MODE_QRY )
                      ? ( DPS_TRANSLOCK_S )
                      : ( DPS_TRANSLOCK_X ) ;

      // make sure current page is locked
      if ( FALSE == pixmContext->getLockHeldInfo( curPage ) )
      {
         rc = ixmLock( pixmContext, curPage.page, lockMode ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
         bCurLocked = TRUE ;
      }

      if ( keyRID._slot >= getNumKeyNode() )
      {
         PD_LOG ( PDERROR, "key slot is out of range" ) ;
         rc = SDB_IXM_KEY_NOTEXIST ;
         goto error ;
      }

      adj = direction < 0 ? 1:0 ;
      ko = keyRID._slot + direction ;
      // for forward, we get _left for the next key
      // for backward, we get _left for the current key
      nextDown = getChildExtentID((UINT16)(ko+adj)) ;
      if ( DMS_INVALID_EXTENT != nextDown )
      {
         // loop until hitting leaf, we always find the _left from next element
         // in forward search, or find the biggest element from the _left for
         // the current element for backward search
         while ( TRUE )
         {
            // lock new page
            nextPage.setPage( nextDown ) ;
            rc = ixmLock( pixmContext, nextPage.page, lockMode ) ;
            if ( rc )
            {
               // unlock all pages
               ixmUnlockAll( pixmContext ) ;
               bCurLocked  = FALSE ;
               bNextLocked = FALSE ;
               goto error ;
            }
            bNextLocked = TRUE ;

            ixmExtent childExtent(nextDown, _pIndexSu) ;

            // unlock current page if we can
            if ( IXM_ADVANCEDOWN_OP_MODE_QRY == lockMode )
            {
               ixmUnlock( pixmContext, curPage.page, TRUE ) ;
               bCurLocked = FALSE ;
            }
            else if ( IXM_ADVANCEDOWN_OP_MODE_DEL == lockMode )
            {
               // free the lock on grandpa page in case it is still locked
               dmsExtentID grandpa
                  = ixmExtent(curPage.page, _pIndexSu).getParent() ;
               if ( DMS_INVALID_EXTENT != grandpa )
               {
                  ixmUnlock( pixmContext, grandpa, TRUE ) ;
               }

               // if child page has more than 1 key release lock on curPage
               if ( childExtent.getNumKeyNode() > 1 )
               {
                  SDB_ASSERT( ( childExtent.getParent() == curPage.page ),
                              "Invalid parent page, corrupted index page" ) ;
                  ixmUnlock( pixmContext, curPage.page, TRUE ) ;
                  bCurLocked = FALSE ;
               }
            }

            // for forward, we get first element in the child, for backward we
            // get the last element in the child
            keyRID._slot = direction>0?0:
                (childExtent.getNumKeyNode()-1) ;
            // get the _left for the child extent
            dmsExtentID child = childExtent.getChildExtentID(keyRID._slot+adj) ;

            if ( DMS_INVALID_EXTENT == child )
               break ;

            nextDown = child ;

            // switch to next page
            curPage     = nextPage ;
            bCurLocked  = bNextLocked ;
            nextPage.reset() ;
            bNextLocked = FALSE ;
         }

         // after loop, the nextDown should represent the element without _left,
         // and keyRID._slot is updated for the target slot. So let's just
         // update keyRID._extent to nextDown
         keyRID._extent = nextDown ;

         goto done ;
      }
      // if we don't have _left, let's just check if we are on the key (instead
      // of end of page)
      if ( ko < getNumKeyNode() && ko >= 0 )
      {
         keyRID._slot = (UINT16)ko ;
         keyRID._extent = _me ;
         goto done ;
      }
      // here we are at end of bucket
      rc = SDB_IXM_EOC ;
      if ( bCurLocked )
      {
         ixmUnlock( pixmContext, curPage.page, TRUE ) ;
         bCurLocked = FALSE ;
      }
      if ( bNextLocked )
      {
         ixmUnlock( pixmContext, nextPage.page, TRUE ) ;
         bNextLocked = FALSE ;
      }
   done :

      return rc ;
   error :
      // unlock current page
      if ( bCurLocked )
      {
         ixmUnlock( pixmContext, curPage.page, TRUE ) ;
         bCurLocked = FALSE ;
      }
      // unlock next page
      if ( bNextLocked )
      {
         ixmUnlock( pixmContext, nextPage.page, TRUE ) ;
         bNextLocked = FALSE ;
      }
      goto done ;
   }

   // caller must make sure there's no _left for pos
   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMEXT__SETITNKEY, "_ixmExtent::_setInternalKey" )
   INT32 _ixmExtent::_setInternalKey (UINT16 pos, const dmsRecordID &rid,
                                      const ixmKey &key,
                                      const Ordering &order, dmsExtentID lchild,
                                      dmsExtentID rchild, ixmIndexCB *indexCB,
                                      _ixmContext * pixmContext )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__IXMEXT__SETITNKEY );
      setChildExtentID ( pos, DMS_INVALID_EXTENT ) ;

      _ixmLockInfo currentPage( _me ) ;

      // the caller must have X lock on current page
      SDB_DASSERT( ( pixmContext->getLockHeldInfo( currentPage ) &&
                     ( DPS_TRANSLOCK_X == currentPage.lockMode ) ),
                   "Doesn't have X lock on current page !" ) ;
      rc = _delKeyAtPos ( pos ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to delete key at pos" ) ;
         goto error ;
      }
      // since _delKeyAtPos moved all following keynodes back to one, we check
      // pos again to get next keynode
      if ( getChildExtentID ( pos ) != rchild )
      {
         PD_LOG ( PDERROR, "rchild doesn't match" ) ;
         dumpIndexExtentIntoLog () ;
         rc = SDB_SYS ;
         goto error ;
      }
      // set child extent for the next to lchild
      setChildExtentID ( pos, lchild ) ;
      rc = insertHere ( pos, rid, key, order, lchild, rchild, indexCB,
                        pixmContext ) ;
      if ( rc )
      {
         // we don't need to worry about SDB_IXM_REORG_DONE because we already
         // set child extent id to lchild, so _reorg should never remove the
         // slot
         PD_LOG ( PDERROR, "Failed to insert here, rc=%d", rc ) ;
         goto error ;
      }
   done :
      PD_TRACE_EXITRC ( SDB__IXMEXT__SETITNKEY, rc );
      return rc ;
   error :
      goto done ;
   }

   INT32 _ixmExtent::_doMergeChildren ( UINT16 pos, const Ordering &order,
                                        ixmIndexCB *indexCB, BOOLEAN &result )
   {
      INT32 rc = SDB_OK ;
      return rc ;
   }

   INT32 _ixmExtent::locate ( const BSONObj &key, const dmsRecordID &rid,
                              const Ordering &order, ixmRecordID &indexrid,
                              BOOLEAN &found, INT32 direction,
                              ixmIndexCB *indexCB,
                              _ixmContext * pixmContext ) const
   {
      INT32 rc = SDB_OK ;
      ixmKeyOwned ixkey ( key ) ;

      _ixmLockInfo currentPage( _me ) ;

      if ( FALSE == pixmContext->getLockHeldInfo( currentPage ) )
      {
         rc = ixmLock( pixmContext, currentPage.page, DPS_TRANSLOCK_S ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }
      rc = _locate ( ixkey, rid, order, indexrid, found, direction,
                     indexCB, pixmContext ) ;
   done :
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMEXT__LOCATE, "_ixmExtent::_locate" )
   INT32 _ixmExtent::_locate ( const ixmKey &key, const dmsRecordID &rid,
                               const Ordering &order, ixmRecordID &indexrid,
                               BOOLEAN &found, INT32 direction,
                               ixmIndexCB *indexCB,
                               _ixmContext * pixmContext ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__IXMEXT__LOCATE );
      SDB_ASSERT ( 1 == direction || -1 == direction, "Invalid direction" ) ;
      UINT16 pos = 0 ;
      dmsExtentID childExtent = DMS_INVALID_EXTENT ;
      INT32 keyFoundPos = -1 ;

      _ixmLockInfo childPage, currentPage( _me ) ;
      BOOLEAN bChildPageLocked = FALSE,
              bCurrentPageLocked = TRUE ;

      //   caller MUST have current page locked 
      SDB_DASSERT( ( pixmContext->getLockHeldInfo( currentPage ) ),
                   "_locate: Page has not been locked !" ) ;

      rc = find ( indexCB, key, rid, order, pos, keyFoundPos, found ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to find in locate" ) ;
         goto error ;
      }

      // if the key and rid exist in this page and not psuedo-deleted
      // then let's just record the extent id and position and return
      if ( found )
      {
         indexrid._extent = _me ;
         indexrid._slot = pos ;
         goto done ;
      }

      // when we get here, that means result == FALSE
      childExtent = getChildExtentID ( pos ) ;
      if ( DMS_INVALID_EXTENT != childExtent )
      {
         // lock child page with S mode
         childPage.setPage( childExtent ) ;
         rc = ixmLock( pixmContext, childPage.page, DPS_TRANSLOCK_S ) ;
         if ( SDB_OK != rc )
         {
            ixmUnlock( pixmContext, currentPage.page, TRUE ) ;
            bCurrentPageLocked = FALSE ;
            goto error ;
         }
         bChildPageLocked = TRUE ;
 
         // release parent page lock before recursively walk through child page
         ixmUnlock( pixmContext, currentPage.page ) ;
         bCurrentPageLocked = FALSE ;

         // if we get left pointer, that means we have child page, then let's do
         // _locate recursively
         rc = ixmExtent(childExtent, _pIndexSu)._locate( key, rid, order,
                                                         indexrid, found,
                                                         direction, indexCB,
                                                         pixmContext ) ;
         if ( rc )
         {
            // release lock on child page when _locate fails
            if ( bChildPageLocked )
            {
               ixmUnlock( pixmContext, childPage.page, TRUE ) ;
               bChildPageLocked = FALSE ;
            }
            // release lock on the new found page if error occurs
            if ( DMS_INVALID_EXTENT != indexrid._extent )
            {
               ixmUnlock( pixmContext, indexrid._extent, TRUE ) ;
            }
            // don't have to repeatedly log in interm pages
            goto error ;
         }

         // if the new found page ( indexrid._extent )
         // is not same as the child page ( childExtent ),
         // try to release lock on child page
         if ( childPage.page != indexrid._extent )
         {
            ixmUnlock( pixmContext, childPage.page, TRUE ) ;
         }

         // if child found the key/rid, or if it find a good place for "next",
         // then we simply return
         // otherwise jump out if and do other checks
         if ( !indexrid.isNull() )
         {
            goto done ;
         }
      }

      // check scan direction
      if ( (direction<0 && 0==pos) || (direction>0 && getNumKeyNode()==pos) )
      {
         if ( DMS_INVALID_EXTENT != indexrid._extent )
         {
            ixmUnlock( pixmContext, indexrid._extent, TRUE ) ;
         }

         indexrid.reset() ;

         if ( bCurrentPageLocked )
         {
            ixmUnlock( pixmContext, currentPage.page, TRUE ) ;
            bCurrentPageLocked = FALSE ;
         }
         if ( bChildPageLocked )
         {
            ixmUnlock( pixmContext, childPage.page, TRUE ) ;
            bChildPageLocked = FALSE ;
         }
      }
      else
      {
         indexrid._extent = _me ;
         indexrid._slot = direction<0?pos-1:pos ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__IXMEXT__LOCATE, rc );
      return rc ;
   error :
      if ( bCurrentPageLocked ) 
      {
         ixmUnlock( pixmContext, currentPage.page, TRUE ) ;
         bCurrentPageLocked = FALSE ;
      }
      if ( bChildPageLocked )
      {
         ixmUnlock( pixmContext, childPage.page, TRUE ) ;
         bChildPageLocked = FALSE ;
      }
      if ( DMS_INVALID_EXTENT != indexrid._extent )
      {
         ixmUnlock( pixmContext, indexrid._extent, TRUE ) ;
      }
      goto done ;
   }


   INT32 _ixmExtent::_locateForDelete
   (
      const ixmKey      & key,
      const dmsRecordID & rid,
      const Ordering    & order,
      ixmRecordID       & indexrid,
      BOOLEAN           & found,
      INT32               direction,
      ixmIndexCB        * indexCB,
      UINT32            & depth,
      UINT32            & xLockLevel,
      _ixmContext       * pixmContext
   )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__IXMEXT__LOCATE ) ;
      SDB_ASSERT ( 1 == direction || -1 == direction, "Invalid direction" ) ;
      UINT16 pos = 0 ;
      dmsExtentID childExtent = DMS_INVALID_EXTENT ;
      INT32 keyFoundPos = -1 ;

      _ixmLockInfo childPage, currentPage( _me ), parentPage( getParent() ) ;
      BOOLEAN bAllLockReleased = FALSE ;
      INT8 parentPageLockMode  = DPS_TRANSLOCK_MAX,
           currentPageLockMode = DPS_TRANSLOCK_MAX,
           childPageLockMode   = DPS_TRANSLOCK_MAX ;

   retry :
      // make sure current page is locked
      // verify if have lock on current page
      if ( FALSE == pixmContext->getLockHeldInfo( currentPage ) )
      {
         rc = SDB_SYS ;
         PD_LOG ( PDERROR, "Error: dosen't have lock on index page:%d, rc=%d",
                  _me, rc ) ;

         SDB_DASSERT ( FALSE,
                       "_locateForDelete: Page has not been locked !" ) ;

         currentPageLockMode = DPS_TRANSLOCK_MAX ;
         goto error ;
      }
      currentPageLockMode = currentPage.lockMode ;

      if ( parentPage.isValid() && pixmContext->getLockHeldInfo( parentPage ) )
      {
         parentPageLockMode = parentPage.lockMode ;
      }
      else
      {
         parentPageLockMode  = DPS_TRANSLOCK_MAX ;
      }

      rc = find ( indexCB, key, rid, order, pos, keyFoundPos, found ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to find in locate" ) ;
         goto error ;
      }

      // if the key and rid exist in this page and not psuedo-deleted
      // then let's just record the extent id and position and return
      if ( found )
      {
         indexrid._extent = _me ;
         indexrid._slot = pos ;
         goto done ;
      }

      // when we get here, that means result == FALSE
      childExtent = getChildExtentID ( pos ) ;

      // when unindex, we may need to check if the page has outside key,
      // if so do split first.
      rc = _doSplitIfOutsideKeyExist( rid, key, order, pos,
                                      currentPageLockMode,
                                      parentPageLockMode,
                                      indexCB,
                                      depth,      // current tree level
                                      xLockLevel, // level need an X lock
                                      pixmContext ) ;
      if ( rc )
      {
         // we have performed reorg and found the position we supposed to
         // insert got a left pointer, so let's reperform find
         if ( SDB_IXM_REORG_DONE == rc )
         {
            rc = SDB_OK ;
            goto retry ;
         }
         goto error ;
      }

      // release lock on parent
      if ( parentPage.isValid() )
      {
         ixmUnlock( pixmContext, parentPage.page, TRUE ) ;
         parentPageLockMode = DPS_TRANSLOCK_MAX ;
      }

      if ( DMS_INVALID_EXTENT != childExtent )
      {
         depth ++ ;

         // lock child page with proper mode
         INT8 lockMode = currentPageLockMode ;
         if ( DPS_TRANSLOCK_S == lockMode )
         {
            if ( ( IXM_X_LOCK_START_LEVEL <= depth ) ||
                 ( ( xLockLevel > 0 ) && ( xLockLevel >= depth ) ) )
            {
               lockMode = DPS_TRANSLOCK_X ; 
               if ( ( 0 == xLockLevel ) || ( xLockLevel >= depth ) )
               {
                  xLockLevel = depth ;
               }
            }
         }

         childPage.setPage( childExtent ) ;
         rc = ixmLock( pixmContext, childPage.page, lockMode ) ;
         if ( SDB_OK != rc )
         {
            // in case fails to acquire lock on child page, release locks
            // on all pages and restart from scratch.
            ixmUnlockAll( pixmContext ) ;
            bAllLockReleased = TRUE ;
            goto error ;
         }
         childPageLockMode = lockMode ;
 
         // if we get left pointer, that means we have child page, then let's do
         // _locate recursively
         rc = ixmExtent(childExtent, _pIndexSu)._locateForDelete( key,
                                                                  rid,
                                                                  order,
                                                                  indexrid,
                                                                  found,
                                                                  direction,
                                                                  indexCB,
                                                                  depth,
                                                                  xLockLevel,
                                                                  pixmContext );
         if ( rc )
         {
            ixmUnlockAll( pixmContext ) ;
            bAllLockReleased = TRUE ;
            goto error ;
         }

         // if child found the key/rid, or if it find a good place for "next",
         // then we simply return
         // otherwise jump out if and do other checks
         if ( !indexrid.isNull() )
         {
            SDB_DASSERT( pixmContext->isLocking( indexrid._extent ),
                         "_locateForDelete: Page has not been locked !" ) ;

            // when _locateForDelete returns it should have lock on
            // the new found page
            ixmExtent newFound( indexrid._extent, _pIndexSu ) ;

            // if the new found page ( indexrid._extent )
            // is not same as the child page ( childExtent ),
            // nor its parent, release lock on child page
            if ( ( childPage.page != indexrid._extent ) &&
                 ( childPage.page != newFound.getParent() ) )
            {
               ixmUnlock( pixmContext, childPage.page, TRUE ) ;
               childPageLockMode = DPS_TRANSLOCK_MAX ;
            }
            // release lock on current page if it is not same
            // as the new found page ( indexrid._extent ) nor
            // its parent page
            if ( ( currentPage.page != indexrid._extent ) &&
                 ( currentPage.page != newFound.getParent() ) )
            {
               ixmUnlock( pixmContext, currentPage.page, TRUE ) ;
               currentPageLockMode = DPS_TRANSLOCK_MAX ;
            }
            goto done ;
         }
      }
      // check scan direction
      if ( (direction<0 && 0==pos) || (direction>0 && getNumKeyNode()==pos) )
      {
         if ( !indexrid.isNull() )
         {
            ixmUnlock( pixmContext, indexrid._extent, TRUE ) ;
         }

         indexrid.reset() ;

         if ( DPS_TRANSLOCK_MAX != currentPageLockMode )
         {
            ixmUnlock( pixmContext, currentPage.page, TRUE ) ;
            currentPageLockMode = DPS_TRANSLOCK_MAX ;
         }
         if ( DPS_TRANSLOCK_MAX != childPageLockMode )
         {
            ixmUnlock( pixmContext, childPage.page, TRUE ) ;
            childPageLockMode = DPS_TRANSLOCK_MAX ;
         }
      }
      else
      {
         indexrid._extent = _me ;
         indexrid._slot = direction<0?pos-1:pos ;
      }
   done :
      PD_TRACE_EXITRC ( SDB__IXMEXT__LOCATE, rc );
      return rc ;
   error :
      // if error occurs, release locks on all pages
      if ( ! bAllLockReleased )
      {
         ixmUnlockAll( pixmContext ) ;
         bAllLockReleased = TRUE ;
         childPageLockMode   = DPS_TRANSLOCK_MAX ;
         parentPageLockMode  = DPS_TRANSLOCK_MAX ;
         currentPageLockMode = DPS_TRANSLOCK_MAX ;
      }
      goto done ;
   }


   // Weather a key exists in the index tree
   // output in result
   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMEXT_EXIST, "_ixmExtent::exists" )
   INT32 _ixmExtent::exists ( const ixmKey &key, const Ordering &order,
                              ixmIndexCB *indexCB, BOOLEAN &result,
                              _ixmContext * pixmContext ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__IXMEXT_EXIST );
      BOOLEAN found ;
      dmsRecordID dummyID ;
      ixmRecordID indexrid ;
      result = FALSE ;

      _ixmLockInfo currentPage( _me ), newPage ;
      BOOLEAN bCurrentPageLocked = FALSE, bNewPageLocked = FALSE ;

      indexrid.reset() ;

      // lock current page( starting page )
      if ( FALSE == pixmContext->getLockHeldInfo( currentPage ) )
      {
         rc = ixmLock( pixmContext, currentPage.page, DPS_TRANSLOCK_S ) ;
         if ( rc )
         {
            goto error ;
         }
         bCurrentPageLocked = TRUE ;
      }

      // try to locate the key and (-1,-1) for rid
      rc = _locate ( key, dummyID, order, indexrid, found, 1, indexCB,
                     pixmContext ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to locate key" ) ;
         goto error ;
      }

      // loop until indexrid is invalid
      while ( TRUE )
      {
         if ( indexrid.isNull() )
            break ;

         // _locate, advance returns the found page locked
         newPage.setPage( indexrid._extent ) ;

         SDB_DASSERT( pixmContext->getLockHeldInfo( newPage ),
                      "Page hasn't been locked !" ) ;

         bNewPageLocked = TRUE ;

         // create extent for indexrid
         ixmExtent extent ( indexrid._extent, _pIndexSu ) ;

         // release lock on starting page if it is not same as new found page
         if ( currentPage.page != newPage.page )
         {
            ixmUnlock( pixmContext, currentPage.page, TRUE ) ;
            bCurrentPageLocked = FALSE ;
         }

         // get the keynode
         const ixmKeyNode *kn = extent.getKeyNode(indexrid._slot) ;
         // skip unused keys (psuedo-deleted)
         if ( kn->isUsed() )
         {
            // compare the on-disk key and the one we are looking for, if they
            // match that means we got exists
            result = ixmKey(extent.getKeyData(indexrid._slot)).woEqual(key) ;
            goto done ;
         }

         // advance to next keynode
         rc = extent.advance ( indexrid, 1, pixmContext ) ;
         if ( rc )
         {
            ixmUnlock( pixmContext, newPage.page, TRUE ) ;
            if ( DMS_INVALID_EXTENT != indexrid._extent )
            {
               ixmUnlock( pixmContext, indexrid._extent, TRUE ) ;
            }
            PD_LOG ( PDERROR, "Failed to advance" ) ;
            goto error ;
         }

         if ( ( DMS_INVALID_EXTENT != indexrid._extent ) &&
              ( newPage.page       != indexrid._extent ) )
         {
            ixmUnlock( pixmContext, newPage.page, TRUE ) ;
            bNewPageLocked = FALSE ;
         }
      }

   done :
      if ( bCurrentPageLocked )
      {
         ixmUnlock( pixmContext, currentPage.page, TRUE ) ;
         bCurrentPageLocked = FALSE ;
      }
      if ( bNewPageLocked )
      {
         ixmUnlock( pixmContext, newPage.page, TRUE ) ;
         bNewPageLocked = FALSE ;
      }
      if ( DMS_INVALID_EXTENT != indexrid._extent )
      {
         ixmUnlock( pixmContext, indexrid._extent, TRUE ) ;
      }
      PD_TRACE_EXITRC ( SDB__IXMEXT_EXIST, rc );
      return rc ;
   error :
      goto done ;
   }

   // in order to avoid parent pointer pointing to itself (from disk
   // corruption), we loop 100 rounds max, usually B tree will never exceed 100
   // levels
   // This function returns the extent id for root of the index page
   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMEXT_GETROOT, "_ixmExtent::getRoot" )
   dmsExtentID _ixmExtent::getRoot() const
   {
      PD_TRACE_ENTRY ( SDB__IXMEXT_GETROOT );
      dmsExtentID extentID = _me ;
      INT32 maxLoop = 100 ;
      while ( DMS_INVALID_EXTENT != extentID &&
              maxLoop > 0 )
      {
         ixmExtent extent ( extentID, _pIndexSu ) ;
         if ( extent.isRoot() )
         {
            PD_TRACE_EXIT ( SDB__IXMEXT_GETROOT );
            return extentID ;
         }
         extentID = extent.getParent() ;
         maxLoop-- ;
      }
      // normally we should never reach here
      PD_LOG ( PDERROR, "loop more than %d times to get root", 100 ) ;
      PD_TRACE_EXIT ( SDB__IXMEXT_GETROOT );
      return DMS_INVALID_EXTENT ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMEXT_FNDSNG, "_ixmExtent::findSingle" )
   INT32 _ixmExtent::findSingle ( const ixmKey   & key,
                                  const Ordering & order,
                                  dmsRecordID    & rid,
                                  ixmIndexCB     * indexCB,
                                  _ixmContext    * pixmContext ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__IXMEXT_FNDSNG );

      BOOLEAN found = FALSE ;
      dmsRecordID dummyID ;
      ixmRecordID indexrid ;

      _ixmLockInfo currentPage( _me ), newPage ;
      BOOLEAN bCurrentPageLocked = FALSE, bNewPageLocked = FALSE ;

      indexrid.reset() ;

      // lock current page( starting page )
      if ( FALSE == pixmContext->getLockHeldInfo( currentPage ) )
      {
         rc = ixmLock( pixmContext, currentPage.page, DPS_TRANSLOCK_S ) ;
         if ( rc )
         {
            goto error ;
         }
         bCurrentPageLocked = TRUE ;
      }

      rc = _locate ( key, dummyID, order, indexrid, found, 1, indexCB,
                     pixmContext ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to locate key" ) ;
         goto error ;
      }
      // loop until indexrid is invalid
      while ( TRUE )
      {
         if ( indexrid.isNull() )
         {
            indexrid.reset() ;
            break ;
         }

         // _locate, advance returns the found page locked
         newPage.setPage( indexrid._extent ) ;

         SDB_DASSERT( pixmContext->getLockHeldInfo( newPage ),
                      "Page hasn't been locked !" ) ;

         bNewPageLocked = TRUE ;

         // create extent for indexrid
         ixmExtent extent ( indexrid._extent, _pIndexSu ) ;

         // release lock on starting page if it is not same as new found page
         if ( currentPage.page != newPage.page )
         {
            ixmUnlock( pixmContext, currentPage.page, TRUE ) ;
            bCurrentPageLocked = FALSE ;
         }

         // get the keynode
         const ixmKeyNode *kn = extent.getKeyNode(indexrid._slot) ;
         // skip unused keys (psuedo-deleted)
         if ( kn->isUsed() )
         {
            // compare the on-disk key and the one we are looking for, if they
            // match that means we got exists
            if ( ixmKey(extent.getKeyData(indexrid._slot)).woCompare (
                        key, order ) != 0 )
            {
               // if the key doesn't match, it means we don't have the key in
               // index, so we reset rid to -1,-1
               rid.reset() ;
               goto done ;
            }
            // if we find the key, let's set rid to kn->_rid
            rid = kn->_rid ;
            goto done ;
         }
         // advance to next keynode
         rc = extent.advance ( indexrid, 1, pixmContext ) ;
         if ( rc )
         {
            ixmUnlock( pixmContext, newPage.page, TRUE ) ;

            if ( DMS_INVALID_EXTENT != indexrid._extent )
            {
               ixmUnlock( pixmContext, indexrid._extent, TRUE ) ;
            }
            PD_LOG ( PDERROR, "Failed to advance" ) ;
            goto error ;
         }

         if ( DMS_INVALID_EXTENT != indexrid._extent )
         {
            ixmUnlock( pixmContext, newPage.page, TRUE ) ;
            bNewPageLocked = FALSE ;
         }
      }
   done :
      if ( bCurrentPageLocked )
      {
         ixmUnlock( pixmContext, currentPage.page, TRUE ) ;
         bCurrentPageLocked = FALSE ;
      }

      if ( bNewPageLocked )
      {
         ixmUnlock( pixmContext, newPage.page, TRUE ) ;
         bNewPageLocked = FALSE ;
      }

      if ( DMS_INVALID_EXTENT != indexrid._extent )
      {
         ixmUnlock( pixmContext, indexrid._extent, TRUE ) ;
      }

      PD_TRACE_EXITRC ( SDB__IXMEXT_FNDSNG, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMEXT_TRUNC, "_ixmExtent::truncate" )
   void _ixmExtent::truncate( ixmIndexCB *indexCB, dmsExtentID parent,
                              BOOLEAN &valid )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__IXMEXT_TRUNC );
      dmsExtentID childExtentID = DMS_INVALID_EXTENT ;
      UINT16 totalFreeSize = _pageSize - 1 - sizeof(ixmExtentHead) ;
      dmsPageMap *pPageMap = _pIndexSu->getPageMap( getMBID() ) ;

      rc = _validate( indexCB, parent ) ;
      if ( rc )
      {
         valid = FALSE ;
         PD_LOG( PDERROR, "Invalid index extent[%d], rc: %d", _me, rc ) ;
         goto done ;
      }
      else
      {
         valid = TRUE ;
         // starting from _right, until first keynode
         for ( INT32 i = (INT32)getNumKeyNode() ; i >= 0; i-- )
         {
            BOOLEAN childValid = TRUE ;
            childExtentID = getChildExtentID ((UINT16)i) ;
            if ( childExtentID != DMS_INVALID_EXTENT )
            {
               // truncated, the page is empty
               // we need to set _totalIndexFreeSpace before freeExtent()
               _pIndexSu->decStatFreeSpace( _extentHead->_mbID,
                                            totalFreeSize ) ;
               try
               {
                  ixmExtent( childExtentID, _pIndexSu ).truncate ( indexCB,
                                                                   _me,
                                                                   childValid) ;
                  // If the child extent is invalid, it's safer not to release
                  // it, and its space will be lost...
                  // It happend that the child extent is the index CB extent,
                  // and finally resulted in crash.
                  // This may happen during recovery after crash.
                  if ( childValid )
                  {
                     indexCB->freeExtent ( childExtentID ) ;
                     pPageMap->rmItem( childExtentID ) ;
                  }
               }
               catch ( std::exception &e )
               {
                  _pIndexSu->addStatFreeSpace( _extentHead->_mbID,
                                               totalFreeSize ) ;
                  PD_LOG( PDWARNING, "Occur exception:%s", e.what() ) ;
               }

               /*
               * To improve performance, not change the page except root
               */
               if ( isRoot() )
               {
                  setChildExtentID ( i, DMS_INVALID_EXTENT ) ;
               }
            }
         }
      }

      _pIndexSu->decStatFreeSpace( _extentHead->_mbID,
                                   _extentHead->_totalFreeSize ) ;

      /*
      * To improve performance, not change the page except root
      */
      if ( isRoot() )
      {
         ixmExtentHead *pHeader = _extRW.writePtr<ixmExtentHead>() ;
         pHeader->_totalKeyNodeNum = 0 ;
         pHeader->_beginFreeOffset = _pageSize - 1 ;
         pHeader->_totalFreeSize = totalFreeSize ;
      }
      _pIndexSu->addStatFreeSpace( _extentHead->_mbID, totalFreeSize ) ;

   done:
      PD_TRACE_EXIT ( SDB__IXMEXT_TRUNC );
   }

   // get the total number of elements in the index node and all children
   // PD_TRACE_DECLARE_FUNCTION ( SDB_IXMEXT_COUNT, "_ixmExtent::count" )
   UINT64 _ixmExtent::count() const
   {
      PD_TRACE_ENTRY ( SDB_IXMEXT_COUNT );
      UINT64 totalCount = 0 ;
      const ixmKeyNode *kn = NULL ;

      for ( INT32 i = (INT32)getNumKeyNode() - 1 ; i >= 0 ; i-- )
      {
         kn = getKeyNode(i) ;
         if ( kn->isUsed() )
         {
            ++totalCount;
         }

         if ( kn->_left != DMS_INVALID_EXTENT )
         {
            totalCount += ixmExtent( kn->_left, _pIndexSu ).count() ;
         }
      }

      if ( DMS_INVALID_EXTENT != _extentHead->_right )
      {
         totalCount += ixmExtent(_extentHead->_right, _pIndexSu).count() ;
      }

      PD_TRACE_EXIT ( SDB_IXMEXT_COUNT ) ;
      return totalCount ;
   }

   BOOLEAN _ixmExtent::isStillValid( UINT16 mbID ) const
   {
      if ( IXM_EXTENT_EYECATCHER0 != _extentHead->_eyeCatcher[0] ||
           IXM_EXTENT_EYECATCHER1 != _extentHead->_eyeCatcher[1] )
      {
         return FALSE ;
      }
      else if ( _extentHead->_mbID != mbID )
      {
         return FALSE ;
      }
      else if ( DMS_EXTENT_FLAG_INUSE != _extentHead->_flag )
      {
         return FALSE ;
      }
      return TRUE ;
   }

   // used in index cursor
   // currentKey is the key from current disk location that trying to be matched
   // prevKey is the key examed from previous run
   // keepFieldsNum is the number of fields from prevKey that should match the
   // currentKey (for example if the prevKey is {c1:1, c2:1}, and keepFieldsNum
   // = 1, that means we want to match c1:1 key for the current location.
   // Depends on if we have skipToNext set, if we do that means we want to skip
   // c1:1 and match whatever the next (for example c1:1.1); otherwise we want
   // to continue match the elements from matchEle )
   // PD_TRACE_DECLARE_FUNCTION ( SDB_IXMEXT__KEYCMP, "_ixmExtent::_keyCmp" )
   INT32 _ixmExtent::_keyCmp ( const BSONObj &currentKey,
                               const BSONObj &prevKey,
                               INT32 keepFieldsNum, BOOLEAN skipToNext,
                               const VEC_ELE_CMP &matchEle,
                               const VEC_BOOLEAN &matchInclusive,
                               const Ordering &o, INT32 direction )
   {
      PD_TRACE_ENTRY ( SDB_IXMEXT__KEYCMP );
      BSONObjIterator ll ( currentKey ) ;
      BSONObjIterator rr ( prevKey ) ;
      VEC_ELE_CMP::const_iterator eleItr = matchEle.begin() ;
      VEC_BOOLEAN ::const_iterator incItr = matchInclusive.begin() ;
      UINT32 mask = 1 ;
      INT32 retCode = 0 ;
      // match keepFieldsNum fields
      for ( INT32 i = 0 ; i < keepFieldsNum; ++i, mask<<=1 )
      {
         BSONElement curEle = ll.next() ;
         BSONElement prevEle = rr.next() ;
         // skip those fields since we don't want to match them from
         // startstopkey iterator
         ++eleItr ;
         ++incItr ;
         INT32 result = curEle.woCompare ( prevEle, FALSE ) ;
         if ( o.descending ( mask ))
            result = -result ;
         if ( result )
         {
            retCode = result ;
            goto done ;
         }
      }
      // if all the keepFieldsNum fields got matched, let's see if we want to
      // simply skip to next key, if so we don't need to match all other
      // elements
      // if that happen, the return value should be -direction, since we want to
      // return -1 if searching forward, otherwise return 1
      if ( skipToNext )
      {
         retCode = -direction ;
         goto done ;
      }
      // if all keepFieldsNum fields got matched, and we want to further match
      // startstopkey iterator, let's move on
      for ( ; ll.more(); mask<<=1 )
      {
         // curEle is always get from current key
         BSONElement curEle = ll.next() ;
         // now let's get the expected element from startstopkey iterator
         BSONElement prevEle = **eleItr ;
         ++eleItr ;
         INT32 result = curEle.woCompare ( prevEle, FALSE ) ;
         if ( o.descending ( mask ))
            result = -result ;
         if ( result )
         {
            retCode = result ;
            goto done ;
         }
         // when getting here, that means the key matches expectation, then
         // let's see if we want inclusive predicate. If not we need to return
         // the negative of direction ( -1 for forward scan, otherwise 1 )
         if ( !*incItr )
         {
            retCode = -direction ;
            goto done ;
         }
         // when get here, it means key match AND inclusive
         ++incItr ;
      }
   done :
      PD_TRACE_EXITRC ( SDB_IXMEXT__KEYCMP, retCode );
      return retCode ;
   }

   // bestIxmRID and resultExtent are the output
   // if rresultExtent != DMS_INVALID_EXTENT, it means there's child extent for
   // the best matched key and we should further dig into that node
   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMEXT__KEYFIND, "_ixmExtent::_keyFind" )
   INT32 _ixmExtent::_keyFind ( UINT16 l, UINT16 h, const BSONObj &prevKey,
                                INT32 keepFieldsNum, BOOLEAN skipToNext,
                                const VEC_ELE_CMP &matchEle,
                                const VEC_BOOLEAN &matchInclusive,
                                const Ordering &o, INT32 direction,
                                ixmRecordID &bestIxmRID,
                                dmsExtentID &resultExtent, _pmdEDUCB *cb ) const
   {
      SINT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__IXMEXT__KEYFIND );
      monAppCB * pMonAppCB = cb ? cb->getMonAppCB() : NULL ;
      SDB_ASSERT ( l <= h, "low must be less than high" ) ;
      DMS_MON_OP_COUNT_INC( pMonAppCB, MON_INDEX_READ, 1 ) ;

      INT32 low = ( INT32 )l ;
      INT32 high = ( INT32 )h ;
      INT32 m = 0 ;
      BufBuilder builder;

      while ( TRUE )
      {
         if ( low > high )
         {
            INT32 tmpSlot = direction > 0 ? low : high ;
            if ( tmpSlot < (INT32)l || tmpSlot > (INT32)h )
            {
               bestIxmRID.reset() ;
            }
            else
            {
               bestIxmRID._extent = _me ;
               bestIxmRID._slot = tmpSlot ;
            }
            resultExtent = getChildExtentID( low ) ;
            goto done ;
         }
         // no need to worry about 16 bit overflow, since each page is only
         // 65536 and each key slot will always > 2 bytes, so h+l won't hit
         // 0xFFFF
         m = ( low + high ) / 2 ;
         const CHAR *data = getKeyData ( m ) ;
         if ( !data )
         {
            PD_LOG ( PDERROR, "slot %d doesn't have matching key", m ) ;
            dumpIndexExtentIntoLog () ;
            rc = SDB_SYS ;
            goto error ;
         }

         builder.reset();
         INT32 r = _keyCmp ( ixmKey(data).toBson(&builder), prevKey, keepFieldsNum,
                             skipToNext, matchEle, matchInclusive, o, direction);
         if ( r < 0 )
         {
            low = m + 1 ;
         }
         else if ( r > 0 )
         {
            high = m - 1 ;
         }
         else
         {
            if ( direction < 0 )
            {
               low = m + 1 ;
            }
            else
            {
               high = m - 1 ;
            }
         }
      } // while ( TRUE )
   done :
      PD_TRACE_EXITRC ( SDB__IXMEXT__KEYFIND, rc );
      return rc ;
   error :
      goto done ;
   }

   // This function locate the key by
   // 1) check if the key is smaller or greater than first or last key in
   // forward and backward search
   // 2) if so it will jump into the child if exist
   // 3) check if the key is greater or smaller than the last or first key in
   // forward and backward search
   // 4) if so it will jump into the child if exist
   // 5) check if the key is in the page, if the key got child it will jump into
   // it
   // the rid may or may not be changed by the following condition
   // A) forward scan
   //   A.1) if first key is greater than prevKey, rid = first key rid and scan
   //   most left pointer
   //   A.2) if last key is smaller than prevKey, rid unchange and scan most
   //   right pointer
   //   A.3) in other condition, do binary search using keyFind and scan child
   // B) backward scan
   //   B.1) if last key is smaller than prevKey, rid = last key rid and scan
   //   most right pointer
   //   B.2) if first key is greater than prevKey, rid unchange and scan most
   //   left pointer
   //   B.3) in other condition, do binary search using keyFind and scan child
   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMEXT_KEYLOCATE, "_ixmExtent::keyLocate" )
   INT32 _ixmExtent::keyLocate ( ixmRecordID       & rid,
                                 const BSONObj     & prevKey,
                                 INT32               keepFieldsNum,
                                 BOOLEAN             skipToNext,
                                 const VEC_ELE_CMP & matchEle,
                                 const VEC_BOOLEAN & matchInclusive,
                                 const Ordering    & o,
                                 INT32               direction,
                                 _pmdEDUCB         * cb,
                                 _ixmContext       * pixmContext )const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__IXMEXT_KEYLOCATE );
      UINT16 l, h, z ;
      const CHAR *data = NULL ;
      INT32 result ;
      dmsExtentID childExtentID ;
      SDB_ASSERT ( direction == 1 || direction == -1, "direction must be "
                   "either 1 or -1" ) ;

      _ixmLockInfo childPage, currentPage( _me ) ;
      BOOLEAN bCurrentPageLocked = FALSE, bChildPageLocked = FALSE ;

      // make sure current page is locked
      if ( FALSE == pixmContext->getLockHeldInfo( currentPage ) )
      {
         rc = ixmLock( pixmContext, currentPage.page, DPS_TRANSLOCK_S ) ;
         if ( rc )
         {
            goto error ;
         }
         bCurrentPageLocked = TRUE ;
      }

      // empty root?
      if ( 0 == getNumKeyNode() )
      {
         rid.reset() ;
         goto done ;
      }

      // keep going until find the smallest/biggest target
      l = 0 ;
      h = getNumKeyNode() - 1 ;
      // when direction = 1, z = 0
      // when direction = -1, z = h
      z = (1-direction)/2*h ;
      data = getKeyData ( z ) ;
      if ( !data )
      {
         PD_LOG ( PDERROR, "slot %d doesn't have matching key", z ) ;
         dumpIndexExtentIntoLog () ;
         rc = SDB_SYS ;
         goto error ;
      }
      // first let's compare the extream condition for first or last record
      // (depends on forward or backward scan)
      result = _keyCmp ( ixmKey(data).toBson(), prevKey, keepFieldsNum,
                         skipToNext, matchEle, matchInclusive, o, direction ) ;
      // if we search forward and first key is greater than expected, or if we
      // search backward and last key is smaller than expected, let's go down
      // a tree level and continue search if possible

      // if we want to exam the one before first ( in forward search ), or the
      // one after last ( in backward search )
      if ( direction * result >= 0 )
      {
         // set best match here. This part should be done here since the first
         // key in this page is less than expected in forward phase ( or greater
         // than expected in backward phase), so this key will be a better match
         rid._extent = _me ;
         rid._slot   = z ;
         if ( direction > 0 )
         {
            // for forward scan, this code path means the requested key is
            // smaller than the lowest
            childExtentID = getChildExtentID(0) ;
         }
         else
         {
            // for backward scan, this code path means the requested key is
            // greater than the last
            childExtentID = _extentHead->_right ;
         }
         // is child exist? if not let's just return the best match
         if ( DMS_INVALID_EXTENT != childExtentID )
         {
            // lock child page
            childPage.setPage( childExtentID ) ;
            rc = ixmLock( pixmContext, childPage.page, DPS_TRANSLOCK_S ) ;
            if ( rc )
            {
               goto error ;
            }
            bChildPageLocked = TRUE ;
 
            // otherwise get the child and recursively call keyLocate
            ixmExtent nextExtent ( childExtentID, _pIndexSu ) ;

            // release lock on current page
            ixmUnlock( pixmContext, currentPage.page, TRUE ) ;
            bCurrentPageLocked = FALSE ;
 
            rc = nextExtent.keyLocate ( rid, prevKey, keepFieldsNum, skipToNext,
                                        matchEle, matchInclusive, o, direction,
                                        cb, pixmContext );
            if ( rc )
            {
               ixmUnlock( pixmContext, childPage.page, TRUE ) ;
               bChildPageLocked = FALSE ;

               if ( DMS_INVALID_EXTENT != rid._extent )
               {
                  ixmUnlock( pixmContext, rid._extent, TRUE ) ;
               }

               PD_LOG ( PDERROR, "Failed to run keyLocate from extent %d",
                        childExtentID ) ;
               goto error ;
            }

            // release lock on child page ( childExtentID ), if the new found
            // page ( rid._extent ) is not same as child page
            if ( ( DMS_INVALID_EXTENT != rid._extent ) &&
                 ( childPage.page != rid._extent ) )
            {
               ixmUnlock( pixmContext, childPage.page, TRUE ) ;
               bChildPageLocked = FALSE ;
            }
         }
         goto done ;
      }

      // now let's check another extream condition, that the last and first key
      // in the page for forward and backward condition
      data = getKeyData( h-z ) ;
      if ( !data )
      {
         PD_LOG ( PDERROR, "slot %d doesn't have matching key", z ) ;
         dumpIndexExtentIntoLog () ;
         rc = SDB_SYS ;
         goto error ;
      }
      // first let's compare the extream condition for first or last record
      // (depends on forward or backward scan)
      result = _keyCmp ( ixmKey(data).toBson(), prevKey, keepFieldsNum,
                         skipToNext, matchEle, matchInclusive, o, direction ) ;
      // if we search forward and last key is less than expected, or if we
      // search backward and first key is greater than expected, let's go down
      // a tree level and continue search if possible
      if ( direction * result < 0 )
      {
         // in this case, be careful we shouldn't overwrite rid as it's less than
         // our expect in forward phase ( or greater than our expect in backward
         // phase ), so we should go into the child if exist. If no child exist
         // let's simply return without touching rid
         // get child
         if ( direction > 0 )
         {
            // for forward scan, this code path means the requested key is
            // greater than the largest
            childExtentID = _extentHead->_right ;
         }
         else
         {
            // for backward scan, this code path means the requested key is
            // smaller than the lowest
            childExtentID = getChildExtentID(0) ;
         }
         // is child exist? if not let's just return the best match
         if ( DMS_INVALID_EXTENT != childExtentID )
         {
            // lock child page
            childPage.setPage( childExtentID );
            rc = ixmLock( pixmContext, childPage.page, DPS_TRANSLOCK_S ) ;
            if ( rc )
            {
               goto error ;
            }
            bChildPageLocked = TRUE ;

            // otherwise get the child and recursively call keyLocate
            ixmExtent nextExtent ( childExtentID, _pIndexSu ) ;

            // release lock on current page
            ixmUnlock( pixmContext, currentPage.page, TRUE ) ;
            bCurrentPageLocked = FALSE ;

            rc = nextExtent.keyLocate ( rid, prevKey, keepFieldsNum, skipToNext,
                                        matchEle, matchInclusive, o, direction,
                                        cb, pixmContext );
            if ( rc )
            {
               ixmUnlock( pixmContext, childPage.page, TRUE ) ;
               bChildPageLocked = FALSE ;

               if ( DMS_INVALID_EXTENT != rid._extent )
               {
                  ixmUnlock( pixmContext, rid._extent, TRUE ) ;
               }

               PD_LOG ( PDERROR, "Failed to run keyLocate from extent %d",
                        childExtentID ) ;
               goto error ;
            }

            // release lock on child page ( childExtentID ), if the new found
            // page ( rid._extent ) is not same as child page
            if ( ( DMS_INVALID_EXTENT != rid._extent ) &&
                 ( childPage.page != rid._extent ) )
            {
               ixmUnlock( pixmContext, childPage.page, TRUE ) ;
               bChildPageLocked = FALSE ;
            }
         }
         goto done ;
      }

      // otherwise the key must fall in this page
      rc = _keyFind ( l, h, prevKey, keepFieldsNum, skipToNext, matchEle,
                      matchInclusive, o, direction, rid, childExtentID, cb ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to run keyFind from extent %d", _me ) ;
         goto error ;
      }
      if ( DMS_INVALID_EXTENT != childExtentID )
      {
         // lock child page
         childPage.setPage( childExtentID );
         rc = ixmLock( pixmContext, childPage.page, DPS_TRANSLOCK_S ) ;
         if ( rc )
         {
            goto error ;
         }
         bChildPageLocked = TRUE ;

         ixmExtent nextExtent ( childExtentID, _pIndexSu ) ;

         // release lock on current page
         ixmUnlock( pixmContext, currentPage.page, TRUE ) ;
         bCurrentPageLocked = FALSE ;

         rc = nextExtent.keyLocate ( rid, prevKey, keepFieldsNum, skipToNext,
                                     matchEle, matchInclusive, o, direction,
                                     cb, pixmContext ) ;
         if ( rc )
         {
            ixmUnlock( pixmContext, childPage.page, TRUE ) ;
            bChildPageLocked = FALSE ;

            if ( DMS_INVALID_EXTENT != rid._extent )
            {
               ixmUnlock( pixmContext, rid._extent, TRUE ) ;
            }

            PD_LOG ( PDERROR, "Failed to run keyLocate from extent %d",
                     childExtentID ) ;
            goto error ;
         }

         // release lock on child page ( childExtentID ), if the new found
         // page ( rid._extent ) is not same as child page
         if ( ( DMS_INVALID_EXTENT != rid._extent ) &&
              ( childPage.page != rid._extent ) )
         {
            ixmUnlock( pixmContext, childPage.page, TRUE ) ;
            bChildPageLocked = FALSE ;
         }
      }

      // release lock on current page ( _me ), if the new found
      // page ( rid._extent ) is not same as child page
      if ( ( DMS_INVALID_EXTENT != rid._extent ) &&
           ( currentPage.page != rid._extent ) )
      {
         ixmUnlock( pixmContext, currentPage.page, TRUE ) ;
         bCurrentPageLocked = FALSE ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__IXMEXT_KEYLOCATE, rc );
      return rc ;
   error :
      if ( bCurrentPageLocked )
      {
         // release lock on current page
         ixmUnlock( pixmContext, currentPage.page, TRUE ) ;
         bCurrentPageLocked = FALSE ;
      }
      if ( bChildPageLocked )
      {
         // release lock on child page
         ixmUnlock( pixmContext, childPage.page, TRUE ) ;
         bChildPageLocked = FALSE ;
      }
      goto done ;
   }

   // get the rid for the smallest/greatest key matching prevKey (forward and
   // backward scan), if there's no such thing exist, rid is reset.
   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMEXT_KEYADVANCE, "_ixmExtent::keyAdvance" )
   INT32 _ixmExtent::keyAdvance ( ixmRecordID       & rid,
                                  const BSONObj     & prevKey,
                                  INT32               keepFieldsNum,
                                  BOOLEAN             skipToNext,
                                  const VEC_ELE_CMP & matchEle,
                                  const VEC_BOOLEAN & matchInclusive,
                                  const Ordering    & o,
                                  INT32               direction,
                                  _pmdEDUCB         * cb,
                                  _ixmContext       * pixmContext ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__IXMEXT_KEYADVANCE );
      UINT16 l = 0, h = 0 ;
      BOOLEAN currentLevel = FALSE ;
      dmsExtentID childExtentID = DMS_INVALID_EXTENT ;
      dmsExtentID parentExtentID = DMS_INVALID_EXTENT ;
      const CHAR *data = NULL ;

      _ixmLockInfo nextPage, currentPage( _me ) ;
      BOOLEAN bCurrentPageLocked = FALSE, bNextPageLocked = FALSE ;

      // make sure current page is locked
      if ( FALSE == pixmContext->getLockHeldInfo( currentPage ) )
      {
         rc = ixmLock( pixmContext, currentPage.page, DPS_TRANSLOCK_S ) ;
         if ( rc )
         {
            goto error ; 
         }
         bCurrentPageLocked = TRUE ;
      }

      // first let's compare the last/first item (forward and backward scan)
      // with the expect key
      if ( direction > 0 )
      {
         // for forward scan, compare if the latest key is greater than the
         // target
         l = rid.isNull()?(0):rid._slot ;
         h = getNumKeyNode() - 1 ;
         data = getKeyData( h ) ;
         if ( !data )
         {
            PD_LOG ( PDERROR, "slot %d doesn't have matching key", h ) ;
            dumpIndexExtentIntoLog () ;
            rc = SDB_SYS ;
            goto error ;
         }
         currentLevel = ( _keyCmp ( ixmKey(data).toBson(), prevKey,
                                    keepFieldsNum, skipToNext, matchEle,
                                    matchInclusive, o, direction) >= 0 ) ;
      }
      else
      {
         // for backward scan, compare if the first key is smaller than the
         // target
         l = 0 ;
         h = rid.isNull()?(getNumKeyNode()-1):rid._slot ;
         data = getKeyData ( l ) ;
         if ( !data )
         {
            PD_LOG ( PDERROR, "slot %d doesn't have matching key", l ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         currentLevel = ( _keyCmp ( ixmKey(data).toBson(), prevKey,
                                    keepFieldsNum, skipToNext, matchEle,
                                    matchInclusive, o, direction) <= 0 ) ;
      }

      // if the latest/first key is greater/smaller than the target, that means
      // we don't need to traversal up. So let's simply call keyFind in the
      // current level
      // if keyFind get us a slot without child, that's our target then.
      // Otherwise we have to keep going to drill down
      if ( currentLevel )
      {
         rc = _keyFind ( l, h, prevKey, keepFieldsNum, skipToNext,
                         matchEle, matchInclusive, o, direction,
                         rid, childExtentID, cb ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to keyFind in extent %d", _me ) ;
            goto error ;
         }

         if ( DMS_INVALID_EXTENT == childExtentID )
         {
            goto done ;
         }
         else
         {
            // acquire lock on child page
            nextPage.setPage( childExtentID ) ;
            rc = ixmLock( pixmContext, nextPage.page, DPS_TRANSLOCK_S );
            if ( SDB_OK != rc )
            {
               goto error ;
            }
            bNextPageLocked = TRUE ;

            ixmExtent childExtent ( childExtentID, _pIndexSu ) ;

            // release lock on curren page
            ixmUnlock( pixmContext, currentPage.page, TRUE ) ;
            bCurrentPageLocked = FALSE ;

            rc = childExtent.keyLocate ( rid, prevKey, keepFieldsNum,
                                         skipToNext, matchEle,
                                         matchInclusive, o, direction, cb,
                                         pixmContext ) ;
            if ( rc )
            {
               ixmUnlock( pixmContext, nextPage.page, TRUE ) ;
               bNextPageLocked = FALSE ;

               if ( DMS_INVALID_EXTENT != rid._extent )
               {
                  ixmUnlock( pixmContext, rid._extent, TRUE ) ;
               }

               PD_LOG ( PDERROR, "Failed to keyLocate in extent %d",
                        childExtentID ) ;
               goto error ;
            }

            // release lock on nextPage page ( childExtentID ), if the new found
            // page ( rid._extent ) is not same as child page
            if ( ( DMS_INVALID_EXTENT != rid._extent ) &&
                 ( nextPage.page != rid._extent ) )
            {
               ixmUnlock( pixmContext, nextPage.page, TRUE ) ;
               bNextPageLocked = FALSE ;
            }
         }
      }
      else if ( (parentExtentID = getParent()) != DMS_INVALID_EXTENT )
      {
         // we need to go up, so let's first reset rid
         rid.reset() ;
         // if we get here, that means the target is greater or smaller than
         // latest/first key (forward and backward). That means the key is not
         // within the current node, and we should traversal up

         // acquire lock on parent page
         nextPage.setPage( parentExtentID ) ;
         rc = ixmTryLock( pixmContext, nextPage.page, DPS_TRANSLOCK_S );
         if ( SDB_OK != rc )
         {
            // release lock on current page
            ixmUnlock( pixmContext, currentPage.page, TRUE ) ;
            bCurrentPageLocked = FALSE ;
            goto error ;
         }
         bNextPageLocked = TRUE ;

         // release lock on current page
         ixmUnlock( pixmContext, currentPage.page, TRUE ) ;
         bCurrentPageLocked = FALSE ;

         ixmExtent parentExtent ( parentExtentID, _pIndexSu ) ;
         rc = parentExtent.keyAdvance ( rid, prevKey, keepFieldsNum,
                                        skipToNext, matchEle, matchInclusive,
                                        o, direction, cb, pixmContext ) ;
         if ( rc )
         {
            ixmUnlock( pixmContext, nextPage.page, TRUE ) ;
            bNextPageLocked = FALSE ;

            if ( DMS_INVALID_EXTENT != rid._extent )
            {
               ixmUnlock( pixmContext, rid._extent, TRUE ) ;
            }

            PD_LOG ( PDERROR, "Failed to keyAdvance in extent %d",
                     parentExtentID ) ;
            goto error ;
         }

         // release lock on nextPage page ( parentExtentID ), if the new found
         // page ( rid._extent ) is not same as child page
         if ( ( DMS_INVALID_EXTENT != rid._extent ) &&
              ( nextPage.page != rid._extent ) )
         {
            ixmUnlock( pixmContext, nextPage.page, TRUE ) ;
            bNextPageLocked = FALSE ;
         }
      }
      else
      {
         // we have to reset rid here, so that if there's no further keys
         // in index scan let's return NULL
         rid.reset() ;

         // if we are root?
         rc = keyLocate ( rid, prevKey, keepFieldsNum, skipToNext, matchEle,
                          matchInclusive, o, direction, cb, pixmContext ) ;
         if ( rc )
         {
            if ( bCurrentPageLocked )
            {
               ixmUnlock( pixmContext, currentPage.page, TRUE ) ;
               bCurrentPageLocked = FALSE ;
            }

            if ( DMS_INVALID_EXTENT != rid._extent )
            {
               ixmUnlock( pixmContext, rid._extent, TRUE ) ;
            }

            PD_LOG ( PDERROR, "Failed to keyLocate in extent %d", _me ) ;
            goto error ;
         }

         // release lock on CurrentPage ( _me ), if the new found
         // page ( rid._extent ) is not same
         if ( ( DMS_INVALID_EXTENT != rid._extent ) &&
              ( currentPage.page != rid._extent ) )
         {
            ixmUnlock( pixmContext, currentPage.page, TRUE ) ;
            bCurrentPageLocked = FALSE ;
         }
      }

      // release lock on CurrentPage ( _me ), if the new found
      // page ( rid._extent ) is not same
      if ( bCurrentPageLocked &&
           ( DMS_INVALID_EXTENT != rid._extent ) &&
           ( currentPage.page != rid._extent ) )
      {
         ixmUnlock( pixmContext, currentPage.page, TRUE ) ;
         bCurrentPageLocked = FALSE ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__IXMEXT_KEYADVANCE, rc );
      return rc ;
   error :
      if ( bNextPageLocked )
      {
         ixmUnlock( pixmContext, nextPage.page, TRUE ) ;
         bNextPageLocked = FALSE ;
      }
      if ( bCurrentPageLocked )
      {
         ixmUnlock( pixmContext, currentPage.page, TRUE ) ;
         bCurrentPageLocked = FALSE ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMEXT_DMPINXEXT2LOG, "_ixmExtent::dumpIndexExtentIntoLog" )
   INT32 _ixmExtent::dumpIndexExtentIntoLog () const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__IXMEXT_DMPINXEXT2LOG );
      // 1MB buffer should be enough for output
      INT32 indexExtentDumpBufferSize = 1024 * 1024 ;
      std::deque<dmsExtentID> childExtents ;
      CHAR *pBuffer = (CHAR*)SDB_OSS_MALLOC ( indexExtentDumpBufferSize ) ;
      PD_CHECK ( pBuffer, SDB_OOM, error, PDERROR,
                 "Failed to allocate memory for dump buffer" ) ;
      rc = dmsDump::dumpIndexExtent ( (CHAR*)_extentHead,
                                       _pageSize,
                                       pBuffer, indexExtentDumpBufferSize,
                                       NULL,
                                       DMS_SU_DMP_OPT_HEX |
                                       DMS_SU_DMP_OPT_HEX_WITH_ASCII |
                                       DMS_SU_DMP_OPT_HEX_PREFIX_AS_ADDR |
                                       DMS_SU_DMP_OPT_FORMATTED,
                                       childExtents,
                                       TRUE ) ;
      if ( rc > 0 )
      {
         PD_LOG ( PDERROR, "Index Page Dump:\n%s", pBuffer ) ;
      }
      rc = SDB_OK ;

   done :
      if ( pBuffer )
      {
         SDB_OSS_FREE ( pBuffer ) ;
      }
      PD_TRACE_EXITRC ( SDB__IXMEXT_DMPINXEXT2LOG, rc );
      return rc ;
   error :
      goto done ;
   }

}
