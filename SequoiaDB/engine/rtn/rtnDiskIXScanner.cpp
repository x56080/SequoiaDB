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

   Source File Name = rtnIXScanner.cpp

   Descriptive Name = Runtime Index Scanner

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains code for index traversal,
   including advance, pause, resume operations.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "rtnIXScanner.hpp"
#include "rtnDiskIXScanner.hpp"
#include "dmsStorageUnit.hpp"
#include "keystring/utilKeyString.hpp"
#include "keystring/utilKeyStringBuilder.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"
<<<<<<< HEAD
=======
#include "optAccessPlanRuntime.hpp"
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
#include "pdSecure.hpp"

using namespace std ;
using namespace bson ;
using namespace engine::keystring ;

namespace engine
{

   _rtnDiskIXScanner::_rtnDiskIXScanner ( ixmIndexCB *indexCB,
                                          optAccessPlanRuntime * planRuntime,
                                          _dmsStorageUnit *su,
                                          _dmsMBContext *mbContext,
                                          BOOLEAN isAsync,
                                          _pmdEDUCB *cb,
                                          BOOLEAN indexCBOwnned )
<<<<<<< HEAD
   :_rtnIXScanner( indexCB, predList, su, mbContext, isAsync, cb, indexCBOwnned ),
     _listIterator( *predList ),
=======
   :_rtnIXScanner( indexCB, planRuntime, su, cb, indexCBOwnned ),
     _listIterator( *_pPredList ),
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
     _pMonCtxCB(NULL)
   {
      _reset() ;
   }

   _rtnDiskIXScanner::~_rtnDiskIXScanner ()
   {
   }

   rtnScannerType _rtnDiskIXScanner::getType() const
   {
      return SCANNER_TYPE_DISK ;
   }

   rtnScannerType _rtnDiskIXScanner::getCurScanType() const
   {
      return SCANNER_TYPE_DISK ;
   }

   void _rtnDiskIXScanner::disableByType( rtnScannerType type )
   {
      /// do nothing
   }

   BOOLEAN _rtnDiskIXScanner::isTypeEnabled( rtnScannerType type ) const
   {
      return type == SCANNER_TYPE_DISK ? TRUE : FALSE ;
   }

   INT32 _rtnDiskIXScanner::getIdxLockModeByType( rtnScannerType type ) const
   {
      return -1 ;
   }

   BOOLEAN _rtnDiskIXScanner::isAvailable() const
   {
      return TRUE ;
   }

   void _rtnDiskIXScanner::setMonCtxCB( _monContextCB *monCtxCB )
   {
      _pMonCtxCB = monCtxCB ;
   }

   void _rtnDiskIXScanner::_reset()
   {
      _savedObj = BSONObj() ;
      _savedRID.reset() ;
      _listIterator.reset() ;

      if ( _pInfo )
      {
         _pInfo->clear() ;
      }
      _init = FALSE ;
   }

   // change the scanner's current location to a given key and rid
   // User can indicate if they want to reset _savedObj/_savedRID using
   // selected index RID position (_curIndexRID)
   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNDISKIXSCAN_RELORID1, "_rtnDiskIXScanner::_relocateRID" )
   INT32 _rtnDiskIXScanner::_relocateRID( const BSONObj &keyObj,
                                          const dmsRecordID &rid,
<<<<<<< HEAD
                                          INT32 direction,
                                          BOOLEAN &isFound )
=======
                                          INT32 direction )
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB__RTNDISKIXSCAN_RELORID1 ) ;

      monAppCB *pMonAppCB = _cb ? _cb->getMonAppCB() : NULL ;

      isFound = FALSE ;

      PD_CHECK ( _indexCB, SDB_OOM, error, PDERROR,
                 "Failed to allocate memory for indexCB" ) ;

      // sanity check, make sure we are on valid index
      PD_CHECK ( _indexCB->isInitialized(), SDB_DMS_INVALID_INDEXCB, error,
                 PDERROR, "Index does not exist" ) ;

      PD_CHECK ( _indexCB->getFlag() == IXM_INDEX_FLAG_NORMAL,
                 SDB_IXM_UNEXPECTED_STATUS, error, PDERROR,
                 "Unexpected index status: %d", _indexCB->getFlag() ) ;

      if ( !_init )
      {
<<<<<<< HEAD
         rc = _firstInit() ;
         if ( SDB_IXM_EOC == rc )
=======
         monAppCB * pMonAppCB   = _cb ? _cb->getMonAppCB() : NULL ;
         // get root
         dmsExtentID rootExtent = _indexCB->getRoot() ;
         ixmExtent root ( rootExtent, _su->index() ) ;
         BOOLEAN found          = FALSE ;

         // locate the new key, the returned RID is stored in _curIndexRID
         rc = root.locate ( keyObj, rid, _order, _curIndexRID,
                            found, direction, _indexCB ) ;
         PD_RC_CHECK ( rc, PDERROR, "Failed to locate from new keyobj(%s) "
                       "and rid(%d, %d), rc: %d", PD_SECURE_OBJ( keyObj ),
                       rid._extent, rid._offset, rc ) ;

         _savedObj = keyObj.getOwned() ;
         _savedRID = rid ;

         if ( found && !isReadonly() )
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
         {
            _isEOF = TRUE ;
            goto done ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to init scanner, rc: %d", rc ) ;

<<<<<<< HEAD
         _init = TRUE ;
=======
         DMS_MON_OP_COUNT_INC( pMonAppCB, MON_INDEX_READ, 1 ) ;
#ifdef _DEBUG
         PD_LOG ( PDDEBUG,
                  "relocateRID to saved obj(%s) and rid(%d, %d), found(%d)",
                  _savedObj.toString().c_str(),
                  _savedRID._extent, _savedRID._offset, found ) ;
#endif
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
      }

      PD_CHECK( _cursorPtr, SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to relocate record, cursor is clsoed" ) ;

      rc = _cursorPtr->locate( keyObj, rid, FALSE, _cb, isFound ) ;
      if ( SDB_IXM_EOC == rc )
      {
         _isEOF = TRUE ;
         goto done ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to locate key, rc: %d", rc ) ;
      DMS_MON_OP_COUNT_INC( pMonAppCB, MON_INDEX_READ, 1 ) ;

      // mark _init to true so that advance won't call keyLocate again
      _init = TRUE ;
      _isEOF = FALSE ;

   done :
      PD_TRACE_EXITRC ( SDB__RTNDISKIXSCAN_RELORID1, rc ) ;
      return rc ;
   error :
      goto done ;
   }

<<<<<<< HEAD
   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNDISKIXSCAN_RELORID, "_rtnDiskIXScanner::relocateRID" )
   INT32 _rtnDiskIXScanner::relocateRID( const BSONObj &keyObj,
                                         const dmsRecordID &rid )
=======
   INT32 _rtnDiskIXScanner::relocateRID( const BSONObj &keyObj,
                                         const dmsRecordID &rid )
   {
      return _relocateRID( keyObj, rid, _direction ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNDISKIXSCAN_RELORID2, "_rtnDiskIXScanner::relocateRID" )
   INT32 _rtnDiskIXScanner::relocateRID( BOOLEAN &found )
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNDISKIXSCAN_RELORID ) ;

      BOOLEAN isFound = FALSE ;
      rc = _relocateRID( keyObj, rid, _direction, isFound ) ;
      if ( SDB_IXM_EOC == rc )
      {
<<<<<<< HEAD
         _isEOF = TRUE ;
         rc = SDB_OK ;
         goto done ;
=======
         PD_LOG ( PDERROR, "Failed to locate from saved obj(%s) and "
                  "rid(%d, %d), rc: %d", PD_SECURE_OBJ( _savedObj ),
                  _savedRID._extent, _savedRID._offset, rc ) ;
         goto error ;
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to relocate RID, rc: %d", rc ) ;

      rc = _cursorPtr->getCurrentKey( _savedObj ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to save key, rc: %d", rc ) ;

      rc = _cursorPtr->getCurrentRecordID( _savedRID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to save record ID, rc: %d", rc ) ;

      _savedObj = _savedObj.getOwned() ;
      _relocatedRID = _savedRID ;

   done:
      PD_TRACE_EXITRC( SDB__RTNDISKIXSCAN_RELORID, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   INT32 _rtnDiskIXScanner::_relocateRID( BOOLEAN &found )
   {
      return _relocateRID( _savedObj, _savedRID, _direction, found ) ;
   }

   // return SDB_IXM_EOC for end of index
   // return SDB_IXM_DEDUP_BUF_MAX if hit max dedup buffer
   // otherwise rid is set to dmsRecordID
   // advance() must be called between resumeScan() and pauseScan()
   // caller must make sure the table is locked before calling resumeScan, and
   // must release the table lock right after pauseScan
   //PD_TRACE_DECLARE_FUNCTION ( SDB__RTNDISKIXSCAN_ADVANCE, "_rtnDiskIXScanner::advance" )
   INT32 _rtnDiskIXScanner::advance ( dmsRecordID &rid )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNDISKIXSCAN_ADVANCE ) ;

      SDB_ASSERT ( _indexCB, "_indexCB can't be NULL" ) ;

      if ( _isEOF )
      {
         rc = SDB_IXM_EOC ;
         goto error ;
      }
      while ( TRUE )
      {
         BOOLEAN needAdvance = TRUE ;
         // first time run after reset, we need to locate the first key
         if ( !_init )
         {
            rc = _firstInit() ;
            if ( SDB_IXM_EOC == rc )
            {
               _init = TRUE ;
               goto done ;
            }
            PD_RC_CHECK( rc, PDERROR, "Failed to init scanner, rc: %d", rc ) ;

            _init = TRUE ;
         }
         else if ( !_savedRID.isValid() )
         {
            rc = _advance() ;
            if ( SDB_IXM_EOC == rc )
            {
               goto done ;
            }
            PD_RC_CHECK( rc, PDERROR, "Failed to advance scanner, rc: %d", rc ) ;
         }
         else
         {
<<<<<<< HEAD
            _savedRID.reset() ;
            _relocatedRID.reset() ;
            _savedObj = BSONObj() ;
         }

         rc = _fetchNext( rid, needAdvance ) ;
         if ( SDB_IXM_EOC == rc )
=======
            PD_LOG ( PDERROR, "Failed to get buffer from current rid(%d, %d)",
                     _curIndexRID._extent, _curIndexRID._slot ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         try
         {
            // get the key from index rid
            try
            {
               _builder.reset();
               _curKeyObj = ixmKey(dataBuffer).toBson( &_builder ) ;
               DMS_MON_OP_COUNT_INC( pMonAppCB, MON_INDEX_READ, 1 ) ;
            }
            catch ( std::exception &e )
            {
               PD_RC_CHECK ( SDB_SYS, PDERROR,
                             "Failed to convert from buffer "
                             "to bson, rid:(%d, %d): %s",
                             _curIndexRID._extent,
                             _curIndexRID._slot, e.what() ) ;
            }
            // compare the key in list iterator
            rc = _listIterator.advance ( _curKeyObj ) ;
            // if -2, that means we hit end of iterator, so all other keys in
            // index are not within our select range
            if ( -2 == rc )
            {
               rc = SDB_IXM_EOC ;
               goto done ;
            }
            // if >=0, that means the key is not selected and we want to
            // further advance the key in index
            else if ( rc >= 0 )
            {
               lastRID = _curIndexRID ;
               rc = indexExtent.keyAdvance ( _curIndexRID, _curKeyObj, rc,
                                             _listIterator.after(),
                                             _listIterator.cmp(),
                                             _listIterator.inc(),
                                             _order, _direction, _cb ) ;
               PD_RC_CHECK ( rc, PDERROR,
                             "Failed to advance, rc = %d", rc ) ;
               if ( lastRID == _curIndexRID )
               {
                  _curIndexRID.reset() ;
               }
               continue ;
            }
            // otherwise let's attempt to get dms rid
            else
            {
               _savedRID = indexExtent.getRID( _curIndexRID._slot ) ;
               // make sure the RID we read is not psuedo-deleted
               if ( _savedRID.isNull() || !_insert2Dup( _savedRID ) )
               {
                  // usually this means a psuedo-deleted rid, we should jump
                  // back to beginning of the function and advance to next
                  // key
                  // if we are able to find the recordid in dupBuffer, that
                  // means we've already processed the record, so let's also
                  // jump back to begin
                  _savedRID.reset() ;
                  goto begin ;
               }
               // make sure we don't hit maximum size of dedup buffer
               /*if ( _pInfo && _pInfo->isUpToLimit() )
               {
                  rc = SDB_IXM_DEDUP_BUF_MAX ;
                  goto error ;
               }*/

               // ready to return to caller
               rid = _savedRID ;

               // if we are write mode, let's record the _savedObj as well
               if ( !isReadonly() )
               {
                  _savedObj = _curKeyObj.getOwned() ;
               }
               // otherwise if we are read mode, let's reset _savedRID
               else
               {
                  // in readonly scenario, _savedRID should always be null
                  // unless pauseScan() is called
                  _savedRID.reset() ;
               }
               rc = SDB_OK ;
               break ;
            }
         } // try
         catch( std::bad_alloc &e )
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
         {
            goto done ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to fetch record ID from scanner, "
                      "rc: %d", rc ) ;
         if ( needAdvance )
         {
            continue ;
         }
         break ;
      }

   done :
      if ( SDB_IXM_EOC == rc && !_isEOF )
      {
         _isEOF = TRUE ;
         rid.reset() ;

         PD_LOG( PDDEBUG, "Hit end with last obj(%s)",
                 PD_SECURE_OBJ( _curKeyObj ) ) ;
      }
      PD_TRACE_EXITRC( SDB__RTNDISKIXSCAN_ADVANCE, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // save the bson key + rid for the current index rid, before releasing
   // X latch on the collection
   // we have to call resumeScan after getting X latch again just in case
   // other sessions changed tree structure
   // this is used for query scan only
   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNDISKIXSCAN_PAUSESCAN, "_rtnDiskIXScanner::pauseScan" )
   INT32 _rtnDiskIXScanner::pauseScan()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNDISKIXSCAN_PAUSESCAN ) ;

      if ( !_init || _isEOF )
      {
         goto done ;
      }

      if ( !_savedRID.isValid() )
      {
<<<<<<< HEAD
         rc = _cursorPtr->getCurrentKey( _savedObj ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to save key, rc: %d", rc ) ;
         _savedObj = _savedObj.getOwned() ;

         rc = _cursorPtr->getCurrentRecordID( _savedRID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to save record ID, rc: %d", rc ) ;
=======
         const CHAR *dataBuffer = NULL ;
         // for read mode, let's copy savedobj then
         ixmExtent indexExtent( _curIndexRID._extent, _su->index() ) ;
         dataBuffer = indexExtent.getKeyData( _curIndexRID._slot ) ;
         if ( !dataBuffer )
         {
            PD_LOG ( PDERROR, "Failed to get buffer from current rid(%d, %d)",
                     _curIndexRID._extent, _curIndexRID._slot ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         try
         {
            _savedObj = ixmKey(dataBuffer).toBson().getOwned() ;
         }
         catch ( std::exception &e )
         {
            PD_LOG ( PDERROR, "Failed to convert buffer to bson from current "
                     "rid(%d, %d): %s", _curIndexRID._extent,
                     _curIndexRID._slot, e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         _savedRID = indexExtent.getRID( _curIndexRID._slot ) ;

         PD_LOG( PDDEBUG, "Paused in obj(%s) with rid(%d, %d)",
                 PD_SECURE_OBJ( _savedObj ),
                 _savedRID._extent, _savedRID._offset ) ;
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
      }

      rc = _cursorPtr->pause( _cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to pause cursor, rc: %d", rc ) ;

      PD_LOG( PDDEBUG, "Paused in obj(%s) with rid(%d,%d)",
              PD_SECURE_OBJ( _savedObj ),
              _savedRID._extent, _savedRID._offset ) ;
      _relocatedRID.reset() ;

   done:
      PD_TRACE_EXITRC ( SDB__RTNDISKIXSCAN_PAUSESCAN, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // restoring the bson key and rid for the current index rid. This is done by
   // comparing the current key/rid pointed by _curIndexRID still same as
   // previous saved one. If so it means there's no change in this key, and we
   // can move on the from the current index rid. Otherwise we have to locate
   // the new position for the saved key+rid
   // this is used in query scan only
   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNDISKIXSCAN_RESUMESCAN, "_rtnDiskIXScanner::resumeScan" )
   INT32 _rtnDiskIXScanner::resumeScan( BOOLEAN &isCursorSame )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNDISKIXSCAN_RESUMESCAN ) ;

      isCursorSame = TRUE ;
      _curKeyObj = BSONObj() ;

      if ( !_indexCB )
      {
         PD_LOG ( PDERROR, "Failed to allocate memory for indexCB" ) ;
         rc = SDB_OOM ;
         goto error ;
      }
      if ( !_indexCB->isInitialized() )
      {
         rc = SDB_DMS_INIT_INDEX ;
         goto done ;
      }

      if ( _indexCB->getFlag() != IXM_INDEX_FLAG_NORMAL )
      {
         rc = SDB_IXM_UNEXPECTED_STATUS ;
         goto done ;
      }

      if ( !_init || _savedObj.isEmpty() )
      {
         rc = SDB_OK ;
         goto done ;
      }

      // compare the historical index OID with the current index oid, to make
      // sure the index is not changed during the time
      if ( !_indexCB->isStillValid ( _indexOID ) ||
           _indexLID != _indexCB->getLogicalID() )
      {
         rc = SDB_DMS_INVALID_INDEXCB ;
         goto done ;
      }

      if ( !_cursorPtr || !_savedRID.isValid() )
      {
         isCursorSame = TRUE ;
         goto done ;
      }

      rc = _relocateRID( isCursorSame ) ;
      if ( SDB_IXM_EOC == rc )
      {
         _isEOF = TRUE ;
         rc = SDB_OK ;
         goto done ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to relocate record, rc: %d", rc ) ;

      PD_LOG( PDDEBUG, "Relocate in obj(%s) with rid(%d,%d), found(%d)",
              PD_SECURE_OBJ( _savedObj ), _savedRID._extent,
              _savedRID._offset, isCursorSame ) ;

      if ( isCursorSame && _relocatedRID != _savedRID )
      {
         _savedRID.reset() ;
         _curKeyObj = _savedObj ;
      }
<<<<<<< HEAD
      _relocatedRID.reset() ;
=======
      else
      {
         // when we get here, it means something changed and we need to
         // relocateRID
         // note relocateRID may relocate to the index that already read.
         // However after advance() returning the RID we'll check if the
         // index already has been read, so we should be safe to not
         // reset _savedRID
         rc = relocateRID( isSame ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to relocate RID, rc: %d", rc ) ;
            goto error ;
         }

         PD_LOG( PDDEBUG, "Relocate in obj(%s) with rid(%d, %d), found(%d)",
                 PD_SECURE_OBJ( _savedObj ), _savedRID._extent,
                 _savedRID._offset, isSame ) ;

         if ( isSame )
         {
            _savedRID.reset() ;
            _curKeyObj = _savedObj ;
         }
      }
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2

   done:
      PD_TRACE_EXITRC ( SDB__RTNDISKIXSCAN_RESUMESCAN, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNDISKIXTREESCAN_CHECKSNAPSHOTID, "_rtnDiskIXScanner::checkSnapshotID" )
   INT32 _rtnDiskIXScanner::checkSnapshotID( BOOLEAN &isCursorSame )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNDISKIXTREESCAN_CHECKSNAPSHOTID ) ;

      isCursorSame = _mbContext->mbStat()->_snapshotID.compare(
                                                _cursorPtr->getSnapshotID() ) ;

      PD_TRACE_EXITRC( SDB__RTNDISKIXTREESCAN_CHECKSNAPSHOTID, rc ) ;

      return rc ;
   }

   rtnPredicateListIterator* _rtnDiskIXScanner::_getPredicateListInterator()
   {
      return &_listIterator ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNDISKIXSCAN__FIRSTINIT, "_rtnDiskIXScanner::_firstInit" )
   INT32 _rtnDiskIXScanner::_firstInit()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNDISKIXSCAN__FIRSTINIT ) ;

      keystring::keyStringStackBuilder builder ;
      shared_ptr<IIndex> idxPtr ;

      rc = _su->index()->getIndex( _mbContext, _indexCB, _cb, idxPtr ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get index, rc: %d", rc ) ;

      rc = builder.buildPredicate( _listIterator.cmp(),
                                   _order,
                                   _listIterator.inc(),
                                   _direction > 0 ? TRUE : FALSE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build key string, rc: %d", rc ) ;

      rc = idxPtr->createIndexCursor( _cursorPtr,
                                      builder.getShallowKeyString(),
                                      _isAsync,
                                      _direction > 0 ? TRUE : FALSE,
                                      _cb ) ;
      if ( SDB_IXM_EOC == rc )
      {
         goto done ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to create index cursor on "
                   "collection [%s.%s] index [%s], rc: %d",
                   _su->getSUName(), _mbContext->clName(), _indexCB->getName(),
                   rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNDISKIXSCAN__FIRSTINIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNDISKIXSCAN__ADVANCE, "_rtnDiskIXScanner::_advance" )
   INT32 _rtnDiskIXScanner::_advance()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNDISKIXSCAN__ADVANCE ) ;

      rc = _cursorPtr->advance( _cb ) ;
      if ( SDB_OK != rc )
      {
         if ( SDB_IXM_EOC == rc )
         {
            goto done ;
         }
         else
         {
            PD_LOG( PDERROR, "Failed to advance cursor, rc: %d", rc ) ;
         }
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNDISKIXSCAN__ADVANCE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

<<<<<<< HEAD
   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNDISKIXSCAN__FETCHNEXT, "_rtnDiskIXScanner::_fetchNext" )
   INT32 _rtnDiskIXScanner::_fetchNext( dmsRecordID &rid, BOOLEAN &needAdvance )
=======
   void _rtnDiskIXScanner::getOwnerTransID( DPS_TRANS_ID &transID )
   {
      SDB_ASSERT( FALSE, "Owner not provided in disk scan.") ;
      transID.reset() ;
   }

   INT32 _rtnDiskIXScanner::_isCursorSame( ixmExtent *pExtent,
                                           const BSONObj &saveObj,
                                           const dmsRecordID &saveRID,
                                           BOOLEAN &isSame,
                                           BOOLEAN *hasRead )
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNDISKIXSCAN__FETCHNEXT ) ;

      monAppCB *pMonAppCB = _cb ? _cb->getMonAppCB() : NULL ;
      needAdvance = FALSE ;

      while ( TRUE )
      {
         INT32 iterRes = 0 ;

         rc = _cursorPtr->getCurrentKey( _curKeyObj ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get key object, rc: %d", rc ) ;

         DMS_MON_OP_COUNT_INC( pMonAppCB, MON_INDEX_READ, 1 ) ;

         // compare the key in list iterator
         iterRes = _listIterator.advance( _curKeyObj ) ;
         // if -2, that means we hit end of iterator, so all other keys in
         // index are not within our select range
         if ( -2 == iterRes )
         {
            rc = SDB_IXM_EOC ;
            goto done ;
         }
         // if >=0, that means the key is not selected and we want to
         // further advance the key in index
         else if ( iterRes >= 0 )
         {
            keystring::keyStringStackBuilder builder ;
            BOOLEAN isFound = FALSE ;

            if ( iterRes > 0 )
            {
               rc = builder.buildPredicate( _curKeyObj,
                                          (UINT32)iterRes,
                                          _order,
                                          _direction > 0 ? TRUE : FALSE ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to build key string, rc: %d", rc ) ;
            }
            else
            {
               rc = builder.buildPredicate( _listIterator.cmp(),
                                            _order,
                                            _listIterator.inc(),
                                            _direction > 0 ? TRUE : FALSE ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to build key string, rc: %d", rc ) ;
            }

            rc = _cursorPtr->locate( builder.getShallowKeyString(), FALSE, _cb, isFound ) ;
            if ( SDB_IXM_EOC == rc )
            {
               goto done ;
            }
            PD_RC_CHECK( rc, PDERROR, "Failed to locate key, rc: %d", rc ) ;
         }
         // otherwise let's attempt to get dms rid
         else
         {
<<<<<<< HEAD
            rc = _cursorPtr->getCurrentRecordID( rid ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get record ID, rc: %d", rc ) ;

            // make sure the RID we read is not psuedo-deleted
            if ( rid.isNull() || !_insert2Dup( rid ) )
            {
               // usually this means a psuedo-deleted rid, we should jump
               // back to beginning of the function and advance to next
               // key
               // if we are able to find the recordid in dupBuffer, that
               // means we've already processed the record, so let's also
               // jump back to begin
               rid.reset() ;
               needAdvance = TRUE ;
               goto done ;
            }
            rc = SDB_OK ;
            break ;
=======
            PD_LOG ( PDERROR, "Failed to convert buffer to bson from "
                     "current rid(%d, %d): %s", _curIndexRID._extent,
                     _curIndexRID._slot, e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNDISKIXSCAN__FETCHNEXT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   void _rtnDiskIXScanner::getRBSPositions( dmsRBSOffset & startPos,
                                            dmsRBSOffset & endPos,
                                            dmsRecordID  & rid,
                                            preIdxTreePtr  memTree )
   {
      preIdxTreePtr  tree ;
      // native diskIXScan should not get to here. We can only
      // call this function through merge scan, and the tree
      // should have been set up. One speical case was during merge
      // scan, the MemIXscanner hasn't touch the tree yet,
      // if we come from disk ixscanner, we might need to search
      // RBS, but the stopping position would be the first position
      // pointed by mem tree
      startPos.reset() ;
      endPos.reset() ;

      if ( !memTree.get() )
      {
         globIdxID gid( getSu()->CSID(),
                        getIndexCB()->getMBID(),
                        getIndexCB()->getLogicalID() ) ;
         tree = pmdGetKRCB()->getTransCB()->getOldVCB()
                ->getIdxTree( gid, FALSE ) ;
      }
      else
      {
         tree = memTree ;
      }
      if ( tree.get() )
      {
         // End position should be the newest index tree value for
         // this RID. If there is no keynode for this RID in memTree, that
         // means we didn't change the index before, we should search the
         // whole RBS
         INDEX_TREE_POS it = memTree->getKeyNodeFromRidTree( rid ) ;
         if ( it != memTree->getTree()->end() )
         {
            endPos = it->second.getRBSOffset() ;
         }
      }
      return ;
   }

}

