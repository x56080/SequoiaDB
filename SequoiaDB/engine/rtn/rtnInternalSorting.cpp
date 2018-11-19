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

   Source File Name = rtnInternalSotring.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains declare for runtime
   functions.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "rtnInternalSorting.hpp"
#include "pd.hpp"
#include "ossUtil.hpp"
#include "pmdEDU.hpp"
#include "ossUtil.hpp"
#include "ixm_common.hpp"

#define RTN_SORT_USE_INSERTSORT        64
#define RTN_SORT_SAME_SWAP_THRESHOLD   0.1

#define RTN_SORT_SWAP( a, b ) \
        do\
        {\
           _rtnSortTuple *tmp = *(a) ;\
           *(a) = *(b) ;\
           *(b) = tmp ;\
        } while(0)

namespace engine
{
   ///_rtnInternalSorting
   _rtnInternalSorting::_rtnInternalSorting( const BSONObj &orderby,
                                             rtnSortArea &sortArea,
                                             INT64 limit )
   :_order( Ordering::make( orderby ) ),
    _sortArea( sortArea ),
    _tupleDirBlock( NULL ),
    _maxTupleBlock( NULL ),
    _workTupleBlock( NULL ),
    _objNum( 0 ),
    _fetched( 0 ),
    _recursion( 0 ),
    _limit( limit ),
    _maxRecordSize( 0 )
   {
   }

   _rtnInternalSorting::~_rtnInternalSorting()
   {
      /// _begin is not allcated here.
      /// it will be freed in rtnSorting.
   }

   INT32 _rtnInternalSorting::push( const BSONObj& keyObj, const CHAR* obj,
                                    INT32 objLen, BSONElement* arrEle )
   {
      INT32 rc = SDB_OK ;
      const CHAR* key = keyObj.objdata() ;
      INT32 keyLen = keyObj.objsize() ;
      UINT32 tupleSize = keyLen + objLen + sizeof(_rtnSortTuple) ;

      SDB_ASSERT( NULL != key, "key can't be NULL" ) ;
      SDB_ASSERT( NULL != obj, "obj can't be NULL" ) ;
      SDB_ASSERT( keyLen > 0, "keyLen must be greater than 0") ;
      SDB_ASSERT( objLen > 0, "objLen must be greater than 0") ;

      if ( (UINT32)objLen > _maxRecordSize )
      {
         _maxRecordSize = (UINT32)objLen ;
      }

      if ( !_tupleDirBlock ||
           _tupleDirBlock->freeSize() < sizeof( _rtnSortTuple *) )
      {
         rc = _extendTupleDirectory() ;
         if ( rc )
         {
            if ( SDB_HIT_HIGH_WATERMARK != rc )
            {
               PD_LOG( PDERROR, "Extend tuple directory space failed[%d]",
                       rc ) ;
            }
            goto error ;
         }
      }

      if ( !_workTupleBlock || _workTupleBlock->freeSize() < tupleSize )
      {
         rc = _extendTupleSpace( tupleSize ) ;
         if ( rc )
         {
            if ( SDB_HIT_HIGH_WATERMARK != rc )
            {
               PD_LOG( PDERROR, "Extend tuple space failed[%d]",
                       rc ) ;
            }
            goto error ;
         }
      }

      rc = _appendTuple( key, (UINT32)keyLen, obj, (UINT32)objLen, arrEle ) ;
      PD_RC_CHECK( rc, PDERROR, "Append tuple to sort area failed[%d]", rc ) ;

      ++_objNum ;

   done:
      return rc ;
   error:
      goto done ;
   }

   void _rtnInternalSorting::clearBuf( BOOLEAN tryExtend )
   {
      if ( !_tupleDirBlock || !_workTupleBlock )
      {
         goto done ;
      }

      _tupleDirBlock->reset() ;

      // If there are more than 1 block, free all tuple blocks except the
      // biggest one.
      if ( _tupleBlocks.size() > 1 )
      {
         BLOCK_LIST_ITR itr = _tupleBlocks.begin() ;
         while ( itr != _tupleBlocks.end() )
         {
            if ( *itr != _maxTupleBlock )
            {
               _sortArea.releaseBlock( *itr ) ;
               itr = _tupleBlocks.erase( itr ) ;
            }
            else
            {
               ++itr ;
            }
         }

         _workTupleBlock = _maxTupleBlock ;
      }

      if ( tryExtend )
      {
         FLOAT64 extendRatio = 0.0 ;
         const FLOAT64 minExtendRatio = 1.1 ;
         extendRatio =
               (FLOAT64)_sortArea.capacity() / (FLOAT64)_sortArea.usedSpace() ;

         if ( extendRatio >= minExtendRatio )
         {
            size_t newDirectorySize =
                  (size_t)( _tupleDirBlock->capacity() * extendRatio ) ;
            size_t newTupleBlockSize =
                  (size_t)( _workTupleBlock->capacity() * extendRatio ) ;

            // Reallocation may fail, just ignore, the current buffer should have
            // no change.
            _sortArea.reallocBlock( _tupleDirBlock, newDirectorySize ) ;
            _sortArea.reallocBlock( _workTupleBlock, newTupleBlockSize ) ;
         }
      }

      _workTupleBlock->reset() ;

      _objNum = 0 ;
      _fetched = 0 ;

   done:
      return ;
   }

   INT32 _rtnInternalSorting::next( _rtnSortTuple **tuple )
   {
      INT32 rc = SDB_OK ;

      if ( !more() )
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      *tuple = *( ( _rtnSortTuple **)
         (_tupleDirBlock->offset2Addr(_fetched * sizeof(_rtnSortTuple *))) ) ;
      SDB_ASSERT( NULL != *tuple, "can not be NULL" ) ;

      ++_fetched ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnInternalSorting::sort( _pmdEDUCB *cb )
   {
      PD_LOG( PDDEBUG, "begin to do internal sort. number of"
                       " obj:%d", _objNum ) ;
      INT32 rc = SDB_OK ;
      _rtnSortTuple **firstTupleAddr = NULL ;
      _rtnSortTuple **lastTupleAddr = NULL ;

      if ( 0 == _objNum )
      {
         goto done ;
      }

      firstTupleAddr = (_rtnSortTuple **)_tupleDirBlock->offset2Addr( 0 ) ;
      lastTupleAddr = (_rtnSortTuple **)
            _tupleDirBlock->offset2Addr( sizeof(_rtnSortTuple *) *
                                          ( _objNum - 1 ) ) ;

      _recursion = 0 ;
      rc = _quickSort( firstTupleAddr, lastTupleAddr, cb ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      PD_LOG( PDDEBUG, "quick sorting recursion:%lld", _recursion ) ;
   done:
      return rc ;
   error:
      goto done ;
   }


   INT32 _rtnInternalSorting::_partition( _rtnSortTuple **left,
                                          _rtnSortTuple **right,
                                          _rtnSortTuple **&leftAxis,
                                          _rtnSortTuple **&rightAxis )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( left < right, "impossible" ) ;
      SDB_ASSERT( NULL != *left && NULL != *right, "can not be NULL" ) ;

      _rtnSortTuple **mid = left + (( right - left ) >> 1 ) ;
      _rtnSortTuple **randPtr = NULL ;

      /// woCompare 's cost may be expensive. think about use it
      /// only when range is large.
      try
      {
         if ( 0 < (*left)->compare( *mid, _order ) )
         {
            RTN_SORT_SWAP( mid, left ) ;
         }
         if ( 0 < (*left)->compare( *right, _order ) )
         {
            RTN_SORT_SWAP( left, right ) ;
         }
         if ( 0 < (*right)->compare( *mid, _order ) )
         {
            RTN_SORT_SWAP( mid, right ) ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected err happened:%s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      {
      _rtnSortTuple **pivot = right ;
      _rtnSortTuple **i = left ;
      _rtnSortTuple **j = right - 1 ;
      FLOAT64 sameNum = 0 ;

      while ( i < j )
      {
         try
         {
            INT32 compare = (*pivot)->compare( *j, _order ) ;
            if ( 0 > compare )
            {
               --j ;
               continue;
            }
            else if ( 0 == compare )
            {
               ++sameNum ;
               --j ;
               continue;
            }
            else
            {
               /// do nothing.
            }
         }
         catch ( std::exception &e )
         {
            PD_LOG( PDERROR, "unexpected err happened:%s", e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         while ( i < j )
         {
            try
            {
               INT32 compare = (*pivot)->compare( *i, _order) ;
               if ( 0 < compare )
               {
                  ++i ;
               }
               else if ( 0 == compare )
               {
                  ++sameNum ;
                  break ;
               }
               else
               {
                  break ;
               }
            }
            catch ( std::exception &e )
            {
               PD_LOG( PDERROR, "unexpected err happened:%s", e.what() ) ;
               rc = SDB_SYS ;
               goto error ;
            }
         }

         if ( i < j )
         {
            RTN_SORT_SWAP( i, j ) ;
            ++i ;
            --j ;
         }
         else
         {
            break ;
         }
      }

      if ( i == j )
      {
         try
         {
            if ( 0 > (*pivot)->compare( *j, _order))
            {
               RTN_SORT_SWAP( pivot, j ) ;
            }
            else if ( j + 1 < pivot )
            {
               ++j ;
               RTN_SORT_SWAP( pivot, j ) ;
            }
            else
            {
               j = pivot ;
            }
         }
         catch ( std::exception &e )
         {
            PD_LOG( PDERROR, "unexpected err happened:%s", e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }
      else
      {
         j = i;
         RTN_SORT_SWAP( pivot, j ) ;
      }

      /// the right maybe the pivot( near to left), so we change the right
      /// to rand pos
      if ( j + 1 < right )
      {
         randPtr = j + 1 + ossRand() % ( right - j - 1 ) ;
         RTN_SORT_SWAP( right, randPtr ) ;
      }

      leftAxis = j ;
      rightAxis = j ;

      /// collect all the obj which is the same to pivot.
      /// it can reduce the number of recursive.
      if ( RTN_SORT_SAME_SWAP_THRESHOLD < ( sameNum / (right - left + 1) ))
      {
         rc = _swapLeftSameKey( left, j, leftAxis ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to swap same key:%d", rc ) ;
            goto error ;
         }

         rc = _swapRightSameKey( j, right, rightAxis ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to swap same key:%d", rc ) ;
            goto error ;
         }
      }
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnInternalSorting::_swapLeftSameKey( _rtnSortTuple **left,
                                                _rtnSortTuple **right,
                                                _rtnSortTuple **&axis )
   {
      INT32 rc = SDB_OK ;
      _rtnSortTuple *pivot = *right ;
      axis = right ;
      _rtnSortTuple **i = left ;
      _rtnSortTuple **j = right - 1 ;
      while ( i < j )
      {
         if ( 0 != pivot->compare( *i, _order ) )
         {
            ++i ;
            continue ;
         }

         while ( i < j &&
                 0 == pivot->compare( *j, _order ))
         {
            axis = j-- ;
         }

         if ( i < j )
         {
            RTN_SORT_SWAP( i, j ) ;
            ++i ;
            axis = j-- ;
         }
         else
         {
            break ;
         }
      }
      return rc ;
   }

   INT32 _rtnInternalSorting::_swapRightSameKey( _rtnSortTuple **left,
                                                _rtnSortTuple **right,
                                                _rtnSortTuple **&axis )
   {
      INT32 rc = SDB_OK ;
      _rtnSortTuple *pivot = *left ;
      axis = left ;
      _rtnSortTuple **i = left + 1 ;
      _rtnSortTuple **j = right ;
      while ( i < j )
      {
         if ( 0 != pivot->compare( *j, _order ) )
         {
            --j ;
            continue ;
         }

         while ( i < j &&
                 0 == pivot->compare( *i, _order ))
         {
            axis = i++ ;
         }

         if ( i < j )
         {
            RTN_SORT_SWAP( i, j ) ;
            --j ;
            axis = i++ ;
         }
         else
         {
            break ;
         }
      }
      return rc ;
   }

   INT32 _rtnInternalSorting::_extendTupleDirectory()
   {
      INT32 rc = SDB_OK ;

      // Double the tuple directory space.
      UINT32 targetSize = ( NULL == _tupleDirBlock ) ?
         RTN_SORT_TUPLE_DIR_MIN_SZ : (_tupleDirBlock->capacity() * 2) ;
      UINT32 extendSize = ( NULL == _tupleDirBlock ) ?
            RTN_SORT_TUPLE_MIN_SZ : _tupleDirBlock->capacity() ;

      // Sort area exhausted.
      if ( _sortArea.freeSpace() < extendSize )
      {
         rc = SDB_HIT_HIGH_WATERMARK ;
         goto error ;
      }

      if ( !_tupleDirBlock )
      {
         rc = _sortArea.allocBlock( targetSize, _tupleDirBlock, FALSE ) ;
         PD_RC_CHECK( rc, PDERROR, "Allocate memory for tuple directory "
                                   "failed[%d]. Size: %u", rc, targetSize ) ;
      }
      else
      {
         rc = _sortArea.reallocBlock( _tupleDirBlock, targetSize ) ;
         if ( rc )
         {
            // Resize failed, maybe out of memory. Only log in debug level.
            // The caller should decide whether to log error or not.
            PD_LOG( PDDEBUG, "Resize tuple directory block from %llu to %u "
                             "failed[%d]", _tupleDirBlock->capacity(),
                             targetSize, rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnInternalSorting::_extendTupleSpace( UINT32 tupleSize )
   {
      INT32 rc = SDB_OK ;

      rtnSortAreaBlock *newBlock = NULL ;
      size_t newBlockSize = 0 ;

      if ( _sortArea.freeSpace() < tupleSize )
      {
         rc = SDB_HIT_HIGH_WATERMARK ;
         goto error ;
      }

      newBlockSize = _maxTupleBlock ?
            ( _maxTupleBlock->capacity() * 2 ) : RTN_SORT_TUPLE_MIN_SZ ;
      if ( tupleSize > newBlockSize )
      {
         newBlockSize = tupleSize ;
      }

      // Try to get as much memory as possible.
      if ( newBlockSize > _sortArea.freeSpace() )
      {
         newBlockSize = _sortArea.freeSpace() ;
      }

      rc = _sortArea.allocBlock( newBlockSize, newBlock, FALSE ) ;
      if ( rc )
      {
         // Resize failed, only log in debug level. The caller should decide
         // whether to log error or not.
         PD_LOG( PDDEBUG, "Extend sort area block space failed[%d]. "
                          "Requested size: %llu", rc, newBlockSize ) ;
         goto error ;
      }

      if ( !_maxTupleBlock ||
           ( newBlock->capacity() > _maxTupleBlock->capacity() ) )
      {
         _maxTupleBlock = newBlock ;
      }
      _tupleBlocks.push_back( newBlock ) ;
      _workTupleBlock = newBlock ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnInternalSorting::_appendTuple( const CHAR *key, UINT32 keyLen,
                                            const CHAR *obj, UINT32 objLen,
                                            BSONElement *arrEle )
   {
      INT32 rc = SDB_OK ;
      _rtnSortTuple tupleTmp ;
      _rtnSortTuple *tuplePtr = (_rtnSortTuple *)
            _workTupleBlock->offset2Addr( _workTupleBlock->length() ) ;

      tupleTmp.setLen( keyLen, objLen ) ;
      if ( !arrEle || arrEle->eoo() )
      {
         tupleTmp.setHash( 0, 0 ) ;
      }
      else
      {
         ixmMakeHashValue( *arrEle, tupleTmp.hashValue() ) ;
      }

     // 1. Append the tuple in the working block.
     rc = _workTupleBlock->append( (CHAR *)&tupleTmp, sizeof( tupleTmp ) ) ;
     PD_RC_CHECK( rc, PDERROR, "Append tuple to sort area block failed[%d]",
                  rc ) ;
     rc = _workTupleBlock->append( key, keyLen ) ;
     PD_RC_CHECK( rc, PDERROR, "Append key to sort area block failed[%d]",
                  rc ) ;
     rc = _workTupleBlock->append( obj, objLen ) ;
     PD_RC_CHECK( rc, PDERROR, "Append object to sort area block failed[%d]",
                  rc ) ;

     // 2. Append the tuple pointer in the directory block.
     rc = _tupleDirBlock->append( (CHAR *)&tuplePtr,
                                  sizeof( _rtnSortTuple *) ) ;
     PD_RC_CHECK( rc, PDERROR, "Append tuple directory failed[%d]", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnInternalSorting::_quickSort( _rtnSortTuple **left,
                                          _rtnSortTuple **right,
                                          _pmdEDUCB *cb )
   {
      SDB_ASSERT( left <= right, "impossible" ) ;

      INT32 rc = SDB_OK ;
      _rtnSortTuple **leftAxis = NULL ;
      _rtnSortTuple **rightAxis = NULL ;
      ++_recursion ;

      /// can stop when recuresive call this func.
      if ( cb->isInterrupted() )
      {
         rc = SDB_APP_INTERRUPT ;
         goto error ;
      }

      if ( left == right )
      {
         goto done ;
      }

      if ( right - left < RTN_SORT_USE_INSERTSORT )
      {
         rc = _insertSort( left, right ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }

         goto done ;
      }

      rc = _partition( left, right, leftAxis, rightAxis ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      if ( left < leftAxis - 1 )
      {
         rc = _quickSort( left, leftAxis - 1, cb ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }

      /// if left is more than limit, don't to sort the right
      if ( _limit > 0 && rightAxis - left + 1 >= _limit )
      {
         goto done ;
      }

      if ( rightAxis + 1 < right )
      {
         rc = _quickSort( rightAxis + 1, right, cb ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnInternalSorting::_insertSort( _rtnSortTuple **left,
                                           _rtnSortTuple **right )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( left < right, "impossible" ) ;
      for ( _rtnSortTuple **i = left + 1;
            i <= right;
            i++ )
      {
         try
         {
            for ( _rtnSortTuple **j = i;
                  j > left;
                  j-- )
            {
               if ( 0 > (*j)->compare( *(j - 1), _order ))
               {
                  RTN_SORT_SWAP( j, j - 1 ) ;
               }
               else
               {
                  break ;
               }
            }
         }
         catch ( std::exception &e )
         {
            PD_LOG( PDERROR, "unexcepted err happened:%s", e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }
}

