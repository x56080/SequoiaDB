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

   Source File Name = rtnStatMCVSet.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/19/2022  ZHY Initial Draft
          11/25/2022  ZHY Move to rtn module
   Last Changed =

*******************************************************************************/
#include "rtnStatMCVSet.hpp"
#include "rtnTrace.hpp"
#include "pdTrace.hpp"

namespace engine {
   #define cls_STAT_CHECKHOLE_THRESHOLD       ( 10 )
   /*
      _clsStatValues implement
    */
   _rtnStatValues::~_rtnStatValues ()
   {
      _clear() ;
   }

   INT32 _rtnStatValues::init ( UINT32 size, UINT32 allocSize )
   {
      INT32 rc = SDB_OK ;

      if ( allocSize < size )
      {
         allocSize = size ;
      }

      if ( allocSize == 0 )
      {
         goto done ;
      }

      _pValues = new(std::nothrow) BSONObj[ allocSize ] ;
      PD_CHECK( _pValues, SDB_OOM, error, PDWARNING,
                "Failed to allocate %u values", allocSize ) ;

      _size = size ;
      _allocSize = allocSize ;

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _rtnStatValues::pushBack ( const BSONObj &boValue )
   {
      if ( _size == _allocSize )
      {
         return SDB_OOM ;
      }
      _pValues[ _size ] = boValue.getOwned() ;
      _size ++ ;
      return SDB_OK ;
   }

   INT32 _rtnStatValues::binarySearch ( rtnStatKey &keyValue, INT32 cmpFlag,
                                        INT32 keyIncFlag, BOOLEAN &isEqual ) const
   {
      isEqual = FALSE ;

      INT32 low = 0, high = _size - 1, mid = 0 ;
      INT32 index = -1 ;
      BSONObj boEmpty ;

      // The output idx means:
      // 1. idx is 0
      //    1. isEqual is true, value == _pValues[0]
      //    2. isEqual is false, value < _pValues[0]
      // 2. idx is 1 to _size - 1
      //    1. isEqual is true, value == _pValues[idx]
      //    2. isEqual is false, _pValues[idx-1]< value < _pValues[idx]
      // 3. idx is _size: _pValues[_size - 1] < value

      if ( 0 == _size )
      {
         return -1 ;
      }

      while ( low <= high )
      {
         mid = ( low + high ) / 2 ;

         INT32 res = keyValue.compareValue( cmpFlag, keyIncFlag,
                                            _pValues[ mid ] ) ;

         if ( 0 == res )
         {
            index = mid ;
            isEqual = TRUE ;
            break ;
         }
         else if ( res > 0 )
         {
            index = mid + 1 ;
            low = mid + 1 ;
         }
         else
         {
            high = mid - 1 ;
            index = mid ;
         }
      }

      return index ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNSTATVALUES_CHKVALS, "_rtnStatValues::checkValues" )
   INT32 _rtnStatValues::checkValues ( UINT32 numKeys, const BSONObj &keyPattern )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNSTATVALUES_CHKVALS ) ;

      for ( UINT32 idx = 0 ; idx < _size ; idx ++ )
      {
         PD_CHECK( numKeys >= (UINT32)_pValues[idx].nFields(),
                   SDB_INVALIDARG, error, PDWARNING, "Number of keys are not "
                   "matched, index: %u, expected: %u, actual: %d",
                   idx, numKeys, _pValues[idx].nFields() ) ;
         if ( numKeys == (UINT32)_pValues[idx].nFields() )
         {
            BSONObjIterator iterKey( keyPattern ) ;
            BSONObjIterator iterValue( _pValues[idx] ) ;
            while( iterKey.more() && iterValue.more() )
            {
               const BSONElement beKey = iterKey.next() ;
               const BSONElement beValue = iterValue.next() ;

               PD_CHECK( 0 == ossStrcmp( beKey.fieldName(), beValue.fieldName() ),
                         SDB_INVALIDARG, error, PDWARNING,
                         "Key names are not matched, expected: %s, actual: %s",
                         beKey.fieldName(), beValue.fieldName() ) ;
            }
            PD_CHECK( !iterKey.more() && !iterValue.more(),
                      SDB_INVALIDARG, error, PDWARNING,
                      "Number of keys are not matched" ) ;
         }
         else
         {
            BSONObjIterator iterKey( keyPattern ) ;
            BSONObjIterator iterValue( _pValues[idx] ) ;
            BSONObjBuilder valueBuilder ;
            while( iterKey.more() )
            {
               const BSONElement beKey = iterKey.next() ;
               if ( iterValue.more() )
               {
                  const BSONElement beValue = iterValue.next() ;

                  PD_CHECK( 0 == ossStrcmp( beKey.fieldName(), beValue.fieldName() ),
                            SDB_INVALIDARG, error, PDWARNING,
                            "Key names are not matched, expected: %s, actual: %s",
                            beKey.fieldName(), beValue.fieldName() ) ;

                  valueBuilder.append( beValue ) ;
               }
               else
               {
                  valueBuilder.appendUndefined( beKey.fieldName() ) ;
               }
            }
            PD_CHECK( !iterValue.more() && !iterKey.more(),
                      SDB_INVALIDARG, error, PDWARNING,
                      "Number of keys are not matched" ) ;
            _pValues[idx] = valueBuilder.obj() ;
         }
      }

      _numKeys = numKeys ;

   done :
      PD_TRACE_EXIT( SDB__RTNSTATVALUES_CHKVALS ) ;
      return rc ;
   error :
      goto done ;
   }

   void _rtnStatValues::_clear ()
   {
      if ( _pValues )
      {
         delete [] _pValues ;
         _pValues = NULL ;
      }
      _size = 0 ;
   }

   BOOLEAN _rtnStatValues::_inRange ( UINT32 idx, rtnStatKey *pStartKey,
                                      rtnStatKey *pStopKey ) const
   {
      if ( idx > _size )
      {
         return FALSE ;
      }

      if ( idx == _size )
      {
         return pStopKey ? FALSE : TRUE ;
      }

      if ( pStartKey )
      {
         // Expect start key < value[idx]
         // The first element is compared in earlier range comparison
         if ( !pStartKey->compareAllValues( 1, -1, _pValues[idx] ) )
         {
            return FALSE ;
         }
      }
      if ( pStopKey )
      {
         // Expect stop key > value[idx]
         // The first element is compared in earlier range comparison
         if ( !pStopKey->compareAllValues( 1, 1, _pValues[idx] ) )
         {
            return FALSE ;
         }
      }

      return TRUE ;
   }

   /*
      _rtnStatMCVSet implement
    */
   _rtnStatMCVSet::~_rtnStatMCVSet ()
   {
      if ( _pFractions )
      {
         delete [] _pFractions ;
         _pFractions = NULL ;
      }
   }

   INT32 _rtnStatMCVSet::init ( UINT32 size, UINT32 allocSize )
   {
      INT32 rc = SDB_OK ;

      if ( allocSize < size )
      {
         allocSize = size ;
      }

      if ( allocSize == 0 )
      {
         goto done ;
      }

      rc = _rtnStatValues::init( size, allocSize ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to init values, rc: %d", rc ) ;

      _pFractions = new(std::nothrow)UINT16[ allocSize ] ;
      PD_CHECK( _pFractions, SDB_OOM, error, PDWARNING,
                "Failed to allocate memory for %u fractions",
                allocSize ) ;

      _totalFrac = 0 ;

   done :
      return rc ;
   error :
      _clear() ;
      goto done ;
   }

   INT32 _rtnStatMCVSet::pushBack ( const BSONObj &boValue, UINT16 fraction )
   {
      INT32 rc = _rtnStatValues::pushBack( boValue ) ;

      if ( SDB_OK == rc )
      {
         SDB_ASSERT( _size > 0, "_size is invalid" ) ;
         _pFractions[ _size - 1 ] = fraction ;
      }

      return rc ;
   }

   void _rtnStatMCVSet::clear ()
   {
      if ( _pFractions )
      {
         delete [] _pFractions ;
         _pFractions = NULL ;
      }
      _totalFrac = 0 ;
      _rtnStatValues::_clear() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNSTATMCVSET_EVALOPTR, "_rtnStatMCVSet::evalOperator" )
   INT32 _rtnStatMCVSet::evalOperator ( rtnStatKey *pStartKey,
                                        rtnStatKey *pStopKey,
                                        BOOLEAN &hitMCV,
                                        double &predSelectivity,
                                        double &scanSelectivity ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNSTATMCVSET_EVALOPTR ) ;

      BOOLEAN startIncluded = FALSE, stopIncluded = FALSE ;
      BOOLEAN startEqual = FALSE, stopEqual = FALSE ;
      INT32 startFlag = 0, stopFlag = 0 ;
      INT32 startIdx = -1, stopIdx = -1 ;
      UINT32 rangeCount = 0 ;
      UINT16 tmpPredSel = 0, tmpScanSel = 0 ;

      BOOLEAN checkHoles = FALSE ;

      PD_CHECK( getSize() > 0, SDB_INVALIDARG, error, PDWARNING,
                "No MCV set is available" ) ;

      PD_CHECK( ( pStartKey && pStopKey && pStartKey->size() == pStopKey->size() ) ||
                  pStartKey || pStopKey, SDB_INVALIDARG, error, PDWARNING,
                  "Numbers of keys are not matched" ) ;

      // Only check holes when number of keys are larger than 1
      if ( _numKeys > 1 && ( ( pStartKey && pStartKey->size() > 1 ) ||
                             ( pStopKey && pStopKey->size() > 1 ) ) )
      {
         checkHoles = TRUE ;
      }

      // Get the index of start key
      if ( pStartKey )
      {
         startIncluded = pStartKey->isIncluded() ;
         startFlag = startIncluded ? -1 : 1 ;
         startIdx = binarySearch( *pStartKey, -1, startFlag, startEqual ) ;
         PD_CHECK( startIdx >= 0, SDB_INVALIDARG, error, PDWARNING,
                   "Failed to locate start key %s in MCV set",
                   pStartKey->toString().c_str() ) ;
      }
      else
      {
         // $lt operator, include all smaller MCV items
         startIdx = 0 ;
         startIncluded = TRUE ;
         startEqual = FALSE ;
      }

      // Get the index of stop key
      if ( pStopKey )
      {
         stopIncluded = pStopKey->isIncluded() ;
         stopFlag = stopIncluded ? 1 : -1 ;
         stopIdx = binarySearch( *pStopKey, 1, stopFlag, stopEqual ) ;
         PD_CHECK( stopIdx >= 0, SDB_INVALIDARG, error, PDWARNING,
                   "Failed to locate stop key %s in MCV set",
                   pStopKey->toString().c_str() ) ;
      }
      else
      {
         // $gt operator, include all greater MCV items
         stopIdx = getSize() ;
         stopIncluded = TRUE ;
         stopEqual = FALSE ;
      }

      // Don't need to check holes if the range is too large
      if ( checkHoles && ( stopIdx - startIdx > cls_STAT_CHECKHOLE_THRESHOLD ) )
      {
         checkHoles = FALSE ;
      }

      if ( startIdx != stopIdx )
      {
         // Check startIdx
         if ( startIncluded || !startEqual )
         {
            // The start key <= value case, add the first selected MCV item
            tmpScanSel += getFracInt( startIdx ) ;
            if ( checkHoles && ( ( startIncluded && startEqual ) ||
                                 _inRange( (UINT32)startIdx, pStartKey, pStopKey ) ) )
            {
               tmpPredSel += getFracInt( startIdx ) ;
            }
            rangeCount ++ ;
         }

         // Check startIdx + 1 to stopIdx - 1
         for ( INT32 idx = startIdx + 1 ; idx < stopIdx ; idx ++ )
         {
            // Every ranges between selected MCV items are needed
            tmpScanSel += getFracInt( idx ) ;
            if ( checkHoles && _inRange( idx, pStartKey, pStopKey ) )
            {
               tmpPredSel += getFracInt( idx ) ;
            }
            rangeCount ++ ;
         }

         // Check stopIdx
         if ( stopIncluded && stopEqual && stopIdx < (INT32)getSize() )
         {
            // The stop key is equal to the last selected MCV item, add
            // the last selected MCV item
            tmpScanSel += getFracInt( stopIdx ) ;
            if ( checkHoles )
            {
               tmpPredSel += getFracInt( stopIdx ) ;
            }
            rangeCount ++ ;
         }
      }
      else
      {
         // start idx is equal to stop idx, which means the start key and stop
         // key are equal to the same MCV item
         if ( startIncluded && startEqual && stopIncluded && stopEqual &&
              startIdx < (INT32)getSize() )
         {
            // The stop key is equal to the selected MCV item, which should
            // be included
            tmpScanSel = getFracInt( startIdx ) ;
            if ( checkHoles )
            {
               tmpPredSel += getFracInt( startIdx ) ;
            }
            rangeCount ++ ;
         }
      }

      scanSelectivity = (double)tmpScanSel / (double)RTN_STAT_FRACTION_SCALE  ;
      scanSelectivity = RTN_STAT_ROUND_SELECTIVITY( scanSelectivity ) ;

      if ( checkHoles )
      {
         predSelectivity = (double)tmpPredSel / (double)RTN_STAT_FRACTION_SCALE ;
         predSelectivity = RTN_STAT_ROUND_SELECTIVITY( predSelectivity ) ;
      }
      else
      {
         predSelectivity = scanSelectivity ;
      }
      hitMCV = rangeCount > 0 ;

   done :
      PD_TRACE_EXITRC( SDB__RTNSTATMCVSET_EVALOPTR, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNSTATMCVSET_EVALETOPTR, "_rtnStatMCVSet::evalETOperator" )
   INT32 _rtnStatMCVSet::evalETOperator ( rtnStatKey &key,
                                          BOOLEAN &hitMCV,
                                          double &predSelectivity,
                                          double &scanSelectivity ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNSTATMCVSET_EVALETOPTR );

      BOOLEAN equal = FALSE ;
      INT32 idx = -1 ;
      double tmpPredSel = 0.0 ;

      PD_CHECK( getSize() > 0, SDB_INVALIDARG, error, PDWARNING,
                "No MCV set is available" ) ;

      idx = binarySearch( key, 0, 0, equal ) ;
      PD_CHECK( idx >= 0, SDB_INVALIDARG, error, PDWARNING,
                "Failed to locate start key %s in MCV set",
                key.toString().c_str() ) ;

      if ( idx < (INT32)getSize() && equal )
      {
         tmpPredSel = getFrac( idx ) ;
         hitMCV = TRUE ;
      }
      else
      {
         hitMCV = FALSE ;
      }

      predSelectivity = RTN_STAT_ROUND_SELECTIVITY( tmpPredSel ) ;
      scanSelectivity = predSelectivity ;

   done :
      PD_TRACE_EXITRC( SDB__RTNSTATMCVSET_EVALETOPTR, rc ) ;
      return rc ;
   error :
      goto done ;
   }
}