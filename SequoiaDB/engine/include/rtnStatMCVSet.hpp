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

   Source File Name = rtnStatMCVSet.hpp

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
#ifndef CLS_STAT_MCV_SET_HPP__
#define CLS_STAT_MCV_SET_HPP__

#include "rtnStatisticsDef.hpp"
#include "../bson/bson.h"

using namespace bson;

namespace engine
{
   /*
         _clsStatKey define
       */
   class _rtnStatKey
   {
   public:
      _rtnStatKey( BOOLEAN included = TRUE ) : _included( included ) {}

      virtual ~_rtnStatKey() {}

      virtual INT32 compareValue( INT32 cmpFlag, INT32 incFlag, const BSONObj &rValue ) = 0;

      virtual BOOLEAN compareAllValues( UINT32 startIdx, INT32 cmpFlag, const BSONObj &rValue ) = 0;

      virtual string toString() = 0;

      virtual UINT32 size() = 0;

      virtual const BSONElement &firstElement() = 0;

      OSS_INLINE BOOLEAN isIncluded() const
      {
         return _included;
      }

      OSS_INLINE void setIncluded( BOOLEAN included )
      {
         _included = included;
      }

   protected:
      OSS_INLINE INT32 _equalButLeftMore( INT32 incFlag )
      {
         // The compared elements are equal, but left has more elements,
         // normally it is left > right
         // But if incFlag is -1 which means a virtual $minKey is appended
         // left, so right > left
         return incFlag < 0 ? -1 : 1;
      }

      OSS_INLINE INT32 _equalButRightMore( INT32 incFlag )
      {
         // The compared elements are equal, but right has more elements,
         // normally it is left < right
         // But if incFlag is 1 which means a virtual $maxKey is appended to
         // left, so left > right
         return incFlag > 0 ? 1 : -1;
      }

      OSS_INLINE INT32 _equalDefault( INT32 incFlag )
      {
         // The compared elements are equal
         // If $maxKey is appended to left, left > right
         // If $minKey is appended to left, left < right
         return incFlag > 0 ? 1 : ( incFlag < 0 ? -1 : 0 );
      }

   protected:
      BOOLEAN _included;
   };

   typedef class _rtnStatKey rtnStatKey;

   /*
         _clsStatValues define
       */
   class _rtnStatValues : public SDBObject
   {
   public:
      _rtnStatValues() = default;

      virtual ~_rtnStatValues();

      INT32 init( UINT32 size, UINT32 allocSize );

      INT32 pushBack( const BSONObj &boValue );

      INT32 binarySearch( rtnStatKey &keyValue,
                          INT32 cmpFlag,
                          INT32 keyIncFlag,
                          BOOLEAN &isEqual ) const;

      OSS_INLINE UINT32 getSize() const
      {
         return _size;
      }

      OSS_INLINE void setValue( UINT32 idx, const BSONObj &boValue )
      {
         if ( idx < _size )
         {
            _pValues[ idx ] = boValue.getOwned();
         }
      }

      OSS_INLINE const BSONObj &getValue( UINT32 idx ) const
      {
         SDB_ASSERT( idx < _size, "Wrong index" );
         return _pValues[ idx ];
      }

      INT32 checkValues( UINT32 numKeys, const BSONObj &keyPattern );

   protected:
      void _clear();

      BOOLEAN _inRange( UINT32 idx, rtnStatKey *pStartKey, rtnStatKey *pStopKey ) const;

   protected:
      UINT32 _numKeys = 0;
      UINT32 _size = 0;
      UINT32 _allocSize = 0;
      BSONObj *_pValues = nullptr;
   };

   /*
      _clsStatMCVSet define
    */
   class _rtnStatMCVSet : public _rtnStatValues
   {
   public:
      _rtnStatMCVSet() = default;

      virtual ~_rtnStatMCVSet();

      INT32 init( UINT32 size, UINT32 allocSize );

      INT32 pushBack( const BSONObj &boValue, UINT16 fraction );

      OSS_INLINE void setFrac( UINT32 idx, UINT16 fraction )
      {
         if ( idx < _size )
         {
            _pFractions[ idx ] = fraction;
         }
      }

      OSS_INLINE double getFrac( UINT32 idx ) const
      {
         if ( idx < _size )
         {
            return (double)_pFractions[ idx ] / (double)RTN_STAT_FRACTION_SCALE;
         }
         return 0.0;
      }

      OSS_INLINE UINT16 getFracInt( UINT32 idx ) const
      {
         if ( idx < _size )
         {
            return _pFractions[ idx ];
         }
         return 0;
      }

      OSS_INLINE void setTotalFrac()
      {
         UINT16 totalFrac = 0;
         for ( UINT32 i = 0; i < _size; i++ )
         {
            totalFrac += _pFractions[ i ];
         }
         totalFrac = RTN_STAT_ROUND( totalFrac, 0, RTN_STAT_FRACTION_SCALE );
         _totalFrac = totalFrac;
      }

      OSS_INLINE double getTotalFrac() const
      {
         return (double)_totalFrac / (double)RTN_STAT_FRACTION_SCALE;
      }

      void clear();

      INT32 evalOperator( rtnStatKey *pStartKey,
                          rtnStatKey *pStopKey,
                          BOOLEAN &hitMCV,
                          double &predSelectivity,
                          double &scanSelectivity ) const;

      INT32 evalETOperator( rtnStatKey &key,
                            BOOLEAN &hitMCV,
                            double &predSelectivity,
                            double &scanSelectivity ) const;

   protected:
      UINT16 *_pFractions = nullptr ;
      UINT16 _totalFrac = 0;
   };

   typedef class _rtnStatMCVSet rtnStatMCVSet;
} // namespace engine

#endif