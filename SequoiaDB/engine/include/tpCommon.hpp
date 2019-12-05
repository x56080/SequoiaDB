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

   Source File Name = tpCommon.hpp

   Descriptive Name = SequoiaDB Time Protocol Service

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for SequoiaDB
   Time Protocol Service.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/30/2019  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef TP_COMMON_HPP__
#define TP_COMMON_HPP__

namespace engine
{

   #define TP_PIPE_SERVICE_NAME     "TP"

   #define TP_VERSION_V1            ( 1 )
   #define TP_VERSION               ( TP_VERSION_V1 )

   // in seconds
   #define TP_MIN_SYNC_INTERVAL     ( 10 )
   #define TP_MAX_SYNC_INTERVAL     ( 600 )
   #define TP_DEF_SYNC_INTERVAL     ( 60 )

   #define TP_MIN_TIME_ERROR_US     ( 1000 )
   #define TP_MAX_TIME_ERROR_US     ( 50000 )
   #define TP_DEF_TIME_ERROR_US     ( TP_MIN_TIME_ERROR_US )

   #define TP_SEC_TO_MILLISEC( x )        ( ( x ) * OSS_ONE_SEC )
   #define TP_SEC_TO_MICROSEC( x )        ( (UINT64)( x ) * 1000000LL )
   #define TP_SEC_TO_NANOSEC( x )         ( (UINT64)( x ) * 1000000000LL )

   #define TP_MICROSEC_TO_NANOSEC( x )    ( (UINT64)( x ) * 1000LL )
   #define TP_MILLISEC_TO_NANOSEC( x )    ( (UINT64)( x ) * 1000000LL )

   #define TP_NANOSEC_TO_MICROSEC( x )    ( (UINT64)( x ) / 1000LL )

   // in nanosecond
   #define TP_MIN_TIME_ERROR  ( TP_MICROSEC_TO_NANOSEC( TP_MIN_TIME_ERROR_US ) )
   #define TP_MAX_TIME_ERROR  ( TP_MICROSEC_TO_NANOSEC( TP_MAX_TIME_ERROR_US ) )
   #define TP_DEF_TIME_ERROR  ( TP_MICROSEC_TO_NANOSEC( TP_DEF_TIME_ERROR_US ) )

   #define TP_DEF_SLEWRATE    ( 10000LL )

}

#endif // TP_COMMON_HPP__
