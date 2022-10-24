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

   Source File Name = clsStatMCVSet.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/19/2022  ZHY Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CLS_STAT_MCV_SET_HPP__
#define CLS_STAT_MCV_SET_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "../bson/bson.h"
using namespace bson ;

namespace engine
{
   #define CLS_STAT_FRACTION_SCALE             ( 10000 )

   #define CLS_STAT_ROUND( x, min, max ) \
           ( OSS_MIN( OSS_MAX( ( x ), ( min ) ), ( max ) ) )

   #define CLS_STAT_ROUND_SELECTIVITY( x ) \
           CLS_STAT_ROUND( ( x ), ( 0.0 ), ( 1.0 ) )
/*
      _clsStatKey define
    */
   class _clsStatKey
   {
      public :
         _clsStatKey ( BOOLEAN included = TRUE )
         : _included( included )
         {
         }

         virtual ~_clsStatKey () {}

         virtual INT32 compareValue ( INT32 cmpFlag, INT32 incFlag,
                                      const BSONObj &rValue ) = 0 ;

         virtual BOOLEAN compareAllValues ( UINT32 startIdx, INT32 cmpFlag,
                                            const BSONObj &rValue ) = 0 ;

         virtual string toString () = 0 ;

         virtual UINT32 size () = 0 ;

         virtual const BSONElement &firstElement () = 0 ;

         OSS_INLINE BOOLEAN isIncluded () const
         {
            return _included ;
         }

         OSS_INLINE void setIncluded ( BOOLEAN included )
         {
            _included = included ;
         }

      protected :
         OSS_INLINE INT32 _equalButLeftMore ( INT32 incFlag )
         {
            // The compared elements are equal, but left has more elements,
            // normally it is left > right
            // But if incFlag is -1 which means a virtual $minKey is appended
            // left, so right > left
            return incFlag < 0 ? -1 : 1 ;
         }

         OSS_INLINE INT32 _equalButRightMore ( INT32 incFlag )
         {
            // The compared elements are equal, but right has more elements,
            // normally it is left < right
            // But if incFlag is 1 which means a virtual $maxKey is appended to
            // left, so left > right
            return incFlag > 0 ? 1 : -1 ;
         }

         OSS_INLINE INT32 _equalDefault ( INT32 incFlag )
         {
            // The compared elements are equal
            // If $maxKey is appended to left, left > right
            // If $minKey is appended to left, left < right
            return incFlag > 0 ? 1 : ( incFlag < 0 ? -1 : 0 ) ;
         }

      protected :
         BOOLEAN _included ;
   } ;

   typedef class _clsStatKey clsStatKey ;

/*
      _clsStatValues define
    */
   class _clsStatValues : public SDBObject
   {
      public :
         _clsStatValues () ;

         virtual ~_clsStatValues () ;

         INT32 init ( UINT32 size, UINT32 allocSize ) ;

         INT32 pushBack ( const BSONObj &boValue ) ;

         INT32 binarySearch ( clsStatKey &keyValue, INT32 cmpFlag,
                              INT32 keyIncFlag, BOOLEAN &isEqual ) const ;

         OSS_INLINE UINT32 getSize () const
         {
            return _size ;
         }

         OSS_INLINE void setValue ( UINT32 idx, const BSONObj &boValue )
         {
            if ( idx < _size )
            {
               _pValues[ idx ] = boValue.getOwned() ;
            }
         }

         OSS_INLINE const BSONObj &getValue ( UINT32 idx ) const
         {
            SDB_ASSERT( idx < _size, "Wrong index" ) ;
            return _pValues[ idx ] ;
         }

         INT32 checkValues ( UINT32 numKeys, const BSONObj &keyPattern ) ;

      protected :

         void _clear () ;

         BOOLEAN _inRange ( UINT32 idx, clsStatKey *pStartKey,
                            clsStatKey *pStopKey ) const ;

      protected :
         UINT32            _numKeys ;
         UINT32            _size ;
         UINT32            _allocSize ;
         BSONObj *         _pValues ;
   } ;

   /*
      _clsStatMCVSet define
    */
   class _clsStatMCVSet : public _clsStatValues
   {
      public :
         _clsStatMCVSet () ;

         virtual ~_clsStatMCVSet () ;

         INT32 init ( UINT32 size, UINT32 allocSize ) ;

         INT32 pushBack ( const BSONObj &boValue, UINT16 fraction ) ;

         OSS_INLINE void setFrac ( UINT32 idx, UINT16 fraction )
         {
            if ( idx < _size )
            {
               _pFractions[ idx ] = fraction ;
            }
         }

         OSS_INLINE double getFrac ( UINT32 idx ) const
         {
            if ( idx < _size )
            {
               return (double)_pFractions[ idx ] / (double)CLS_STAT_FRACTION_SCALE ;
            }
            return 0.0 ;
         }

         OSS_INLINE UINT16 getFracInt ( UINT32 idx ) const
         {
            if ( idx < _size )
            {
               return _pFractions[ idx ] ;
            }
            return 0 ;
         }

         OSS_INLINE void setTotalFrac ()
         {
            UINT16 totalFrac = 0 ;
            for ( UINT32 i = 0 ; i < _size ; i++ )
            {
               totalFrac += _pFractions[ i ] ;
            }
            totalFrac = CLS_STAT_ROUND( totalFrac, 0, CLS_STAT_FRACTION_SCALE ) ;
            _totalFrac = totalFrac ;
         }

         OSS_INLINE double getTotalFrac () const
         {
            return (double)_totalFrac / (double)CLS_STAT_FRACTION_SCALE ;
         }

         void clear () ;

         INT32 evalOperator ( clsStatKey *pStartKey, clsStatKey *pStopKey,
                              BOOLEAN &hitMCV,
                              double &predSelectivity,
                              double &scanSelectivity ) const ;

         INT32 evalETOperator ( clsStatKey &key, BOOLEAN &hitMCV,
                                double &predSelectivity,
                                double &scanSelectivity ) const ;

      protected :
         UINT16 *          _pFractions ;
         UINT16            _totalFrac ;
   } ;

   typedef class _clsStatMCVSet clsStatMCVSet ;
}

#endif