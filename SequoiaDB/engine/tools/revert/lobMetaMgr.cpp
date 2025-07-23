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

   Source File Name = lobMetaMgr.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/12/2023  Yang Qincheng  Initial Draft

   Last Changed =

*******************************************************************************/
#include "lobMetaMgr.hpp"
#include "ossUtil.hpp"

using namespace std ;

namespace sdbrevert
{
   // lobMeta
   lobMeta::lobMeta( const bson::OID &oid, UINT32 sequence )
   {
      _oid = oid ;
      _sequence = sequence ;
   }

   lobMeta::~lobMeta()
   {
   }

   BOOLEAN lobMeta::operator<( const lobMeta &other ) const
   {
      if ( _oid < other._oid )
         return TRUE ;
      else if ( _oid == other._oid )
         return _sequence < other._sequence ;
      else
         return FALSE ;
   }


   // lobMetaMgr
   lobMetaMgr::lobMetaMgr()
   {
   }

   lobMetaMgr::~lobMetaMgr()
   {
   }

   BOOLEAN lobMetaMgr::checkLSN( const bson::OID &oid, UINT32 sequence, DPS_LSN_OFFSET lsn )
   {
      lobMeta meta( oid, sequence ) ;

      ossScopedLock lock( &_mutex ) ;
      map<lobMeta, DPS_LSN_OFFSET>::iterator it = _lobMetaMap.find( meta ) ;
      if ( it != _lobMetaMap.end() )
      {
         if ( lsn > it->second )
         {
            it->second = lsn ;
            return TRUE ;
         }
         else
         {
            return FALSE ;
         }
      }
      else
      {
         _lobMetaMap.insert( pair<lobMeta, DPS_LSN_OFFSET>( meta, lsn ) ) ;
         return TRUE ;
      }
   }


   // lobLockMap
   lobLockMap::lobLockMap( UINT32 size )
   {
      UINT32 count        = 0 ;
      ossSpinXLatch* lock = NULL ;

      _size = size ;
      while ( count < size )
      {
         lock = new ossSpinXLatch() ;
         _lobLockVec.push_back( lock ) ;
         count++ ;
      }
   }

   lobLockMap::~lobLockMap()
   {
      for ( size_t i = 0 ; i < _lobLockVec.size() ; ++i )
      {
         SAFE_OSS_DELETE( _lobLockVec[i] ) ;
      }
   }

   INT32 lobLockMap::getLobLock( const bson::OID &oid, ossSpinXLatch *&lobLock )
   {
      INT32 rc   = SDB_OK ;
      UINT32 oidHash = ossHash( oid.str().c_str() ) ;
      UINT32 key = oidHash % _size ;

      lobLock = _lobLockVec[key] ;
      if ( NULL == lobLock )
      {
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }
}