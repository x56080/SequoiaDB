/******************************************************************************

   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = utilString.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/01/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef UTIL_STRING_HPP_
#define UTIL_STRING_HPP_

#include "oss.hpp"
#include "core.hpp"
#include <boost/noncopyable.hpp>

#define UTIL_STRING_STAITC_LEN 256 

namespace engine
{
   class _utilString : public SDBObject, boost::noncopyable
   {
   public:
      _utilString() ;
      ~_utilString() ;

   public:
      void  clear()
      {
         _buf[0] = '\0' ;
         _len = 0 ;
      }

      INT32 append( const CHAR *str, UINT32 len ) ;

      INT32 append( CHAR c ) ;

      INT32 resize( UINT32 len ) ;

      OSS_INLINE const CHAR *str() const
      {
         return _buf ;
      }

      /// does not include terminating character
      OSS_INLINE UINT32 len() const
      {
         return _len ;
      }

      OSS_INLINE BOOLEAN enough( UINT32 len ) const
      {
         return _len + len + 1 <= _bufLen ;
      }

      INT32 appendINT32( INT32 v )
      {
         return appendNumber( v, 11, "%d" ) ;
      }

      INT32 appendDouble( FLOAT64 v )
      {
         return appendNumber( v, 25, "%g" ) ;
      }

      INT32 appendINT64( INT64 v )
      {
         return appendNumber( v, 22, "%lld" ) ;
      }

      template <typename T>
      INT32 appendNumber( T v, UINT32 size, const CHAR *macro )
      {
         INT32 rc = SDB_OK ;
         if ( !enough( size ) )
         {
            rc = resize( size + _bufLen ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
         }

         rc = sprintf( &(_buf[_len]), macro, v ) ;
         if ( rc < SDB_OK )
         {
            rc = SDB_SYS ;
            goto error ;
         }

         _len += rc ;
         rc = SDB_OK ;
         _buf[_len] = '\0';
      done:
         return rc ;
      error:
         goto done ;
      }
   private:
      CHAR _static[UTIL_STRING_STAITC_LEN] ;
      CHAR *_dynamic ;
      CHAR *_buf ;
      UINT32 _bufLen ;
      UINT32 _len ; 
   } ;

   typedef class _utilString utilString ;
}

#endif

