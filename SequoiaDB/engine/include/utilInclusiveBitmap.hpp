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

   Source File Name = utilInclusiveBitmap.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef UTIL_INCLUSIVE_BITMAP_HPP_
#define UTIL_INCLUSIVE_BITMAP_HPP_

#include "ossUtil.hpp"
#include "utilBitmap.hpp"

namespace engine
{

   /*
      _utilInclusiveBitmap define
    */
   /// the bit at the position is 0 means inclusive
   class _utilInclusiveBitmap : public _utilStackBitmap< 32 >
   {
   public:
      _utilInclusiveBitmap() = default ;
      ~_utilInclusiveBitmap() = default ;

      explicit _utilInclusiveBitmap( UINT32 size, BOOLEAN inclusive )
      : _utilStackBitmap()
      {
         SDB_ASSERT( size <= getSize(), "out of bound" ) ;
         if ( !inclusive )
         {
            setAllBits() ;
         }
      }

      void reset()
      {
         resetBitmap() ;
      }

      BOOLEAN operator []( UINT32 pos ) const
      {
         return isInclusive( pos ) ;
      }

      void setAll( UINT32 size, BOOLEAN inclusive )
      {
         SDB_ASSERT( size <= getSize(), "out of bound" ) ;
         if ( inclusive )
         {
            resetBitmap() ;
         }
         else
         {
            setAllBits() ;
         }
      }

      void setInclusive( UINT32 pos )
      {
         SDB_ASSERT( pos < getSize(), "out of bound" ) ;
         clearBit( pos ) ;
      }

      void setExclusive( UINT32 pos )
      {
         SDB_ASSERT( pos < getSize(), "out of bound" ) ;
         setBit( pos ) ;
      }

      void set( UINT32 pos, BOOLEAN inclusive )
      {
         if ( inclusive )
         {
            setInclusive( pos ) ;
         }
         else
         {
            setExclusive( pos ) ;
         }
      }

      void setBatch( UINT32 begin, UINT32 end, BOOLEAN inclusive )
      {
         for ( UINT32 i = begin ; i <= end ; ++ i )
         {
            set( i, inclusive ) ;
         }
      }

      BOOLEAN allInclusive() const
      {
         return isEmpty() ;
      }

      BOOLEAN isInclusive( UINT32 pos ) const
      {
         SDB_ASSERT( pos < getSize(), "out of bound" ) ;
         return !testBit( pos ) ;
      }

      BOOLEAN isExclusive( UINT32 pos ) const
      {
         SDB_ASSERT( pos < getSize(), "out of bound" ) ;
         return testBit( pos ) ;
      }
   } ;

   typedef class _utilInclusiveBitmap utilInclusiveBitmap ;

}

#endif // UTIL_INCLUSIVE_BITMAP_HPP_
