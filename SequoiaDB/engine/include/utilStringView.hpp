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

   Source File Name = utilStringView.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/20/2022  ZHY Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef UTIL_STRING_VIEW_HPP__
#define UTIL_STRING_VIEW_HPP__

#include "ossTypes.h"
#include "ossUtil.hpp"
#include <cstddef>
#include <cstring>
#include <functional>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
namespace engine
{
class utilStringView
{
public:
   using value_type = CHAR;
   using traits_type = std::char_traits< CHAR >;
   using pointer = CHAR *;
   using const_pointer = const CHAR *;
   using reference = CHAR &;
   using const_reference = const CHAR &;
   using const_iterator = const CHAR *;
   using iterator = const_iterator;
   using const_reverse_iterator = std::reverse_iterator< const_iterator >;
   using reverse_iterator = const_reverse_iterator;
   using size_type = size_t;
   using difference_type = std::ptrdiff_t;
   static constexpr size_type npos = static_cast< size_type >( -1 );
   utilStringView() noexcept : _ptr( nullptr ), _length( 0 )
   {
   }
   utilStringView( const CHAR *str )
   : _ptr( str ), _length( str ? ossStrlen( str ) : 0 )
   {
   }
   utilStringView( const CHAR *data, size_type len ) : _ptr( data ), _length( len )
   {
   }
   utilStringView( const std::string &str ) : utilStringView( str.data(), str.size() )
   {
   }

public:
   const_iterator begin() const noexcept
   {
      return _ptr;
   }
   const_iterator end() const noexcept
   {
      return _ptr + _length;
   }
   const_iterator cbegin() const noexcept
   {
      return begin();
   }
   const_iterator cend() const noexcept
   {
      return end();
   }
   const_reverse_iterator rbegin() const noexcept
   {
      return const_reverse_iterator( end() );
   }
   const_reverse_iterator rend() const noexcept
   {
      return const_reverse_iterator( begin() );
   }
   const_reverse_iterator crbegin() const noexcept
   {
      return rbegin();
   }
   const_reverse_iterator crend() const noexcept
   {
      return rend();
   }
   size_type size() const noexcept
   {
      return _length;
   }
   size_type length() const noexcept
   {
      return size();
   }
   BOOLEAN empty() const noexcept
   {
      return _length == 0;
   }
   const_reference operator[]( size_type i ) const
   {
      return _ptr[ i ];
   }
   const_reference at( size_type i ) const
   {
      if ( i > size() )
      {
         throw std::out_of_range( "stringView::at" );
      }
      return _ptr[ i ];
   }
   const_pointer data() const noexcept
   {
      return _ptr;
   }
   utilStringView substr( size_type pos = 0, size_type n = npos ) const
   {
      if ( pos + n > size() )
      {
         throw std::out_of_range( "stringView::substr" );
         return utilStringView();
      }
      else
      {
         return utilStringView( _ptr + pos, std::min( n, _length - pos ) );
      }
   }

   size_type find( CHAR ch, size_type pos = 0 ) const noexcept
   {
      if ( empty() || pos >= _length )
      {
         return npos;
      }
      const CHAR *result = static_cast< const CHAR * >(
          memchr( _ptr + pos, ch, _length - pos ) );
      return result != nullptr ? static_cast< size_type >( result - _ptr )
                               : npos;
   }

   INT32 compare( utilStringView x ) const noexcept
   {
      UINT32 n = std::min( _length, x._length );
      INT32 res = ossMemcmp( _ptr, x._ptr, n );
      if ( 0 == res )
      {
         if ( _length < x._length )
         {
            res = -1;
         }
         else if ( _length > x._length )
         {
            res = 1;
         }
      }
      return res;
   }

   INT32 compare( size_type pos1, size_type count1, utilStringView v ) const
   {
      return substr( pos1, count1 ).compare( v );
   }

   INT32 compare( const CHAR *s ) const
   {
      return compare( utilStringView( s ) );
   }

private:
   const_pointer _ptr = nullptr;
   size_type _length = 0;
};
OSS_INLINE BOOLEAN operator==( utilStringView x, utilStringView y )
{
   return x.compare( y ) == 0;
}

OSS_INLINE BOOLEAN operator!=( utilStringView x, utilStringView y )
{
   return !( x == y );
}

OSS_INLINE BOOLEAN operator<( utilStringView x, utilStringView y )
{
   return x.compare( y ) < 0;
}

OSS_INLINE BOOLEAN operator>( utilStringView x, utilStringView y )
{
   return y < x;
}

OSS_INLINE BOOLEAN operator<=( utilStringView x, utilStringView y )
{
   return !( y < x );
}

OSS_INLINE BOOLEAN operator>=( utilStringView x, utilStringView y )
{
   return !( x < y );
}
} // namespace engine

#endif