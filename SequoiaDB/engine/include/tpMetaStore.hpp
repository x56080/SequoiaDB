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

   Source File Name = tpMetaStore.hpp

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

#ifndef TP_META_STORE_HPP__
#define TP_META_STORE_HPP__

#include "tpCBCommon.hpp"
#include "dpsLogDef.hpp"

namespace engine
{

   /*
      _tpMetaStore define
    */
   class _tpMetaStore : public SDBObject
   {
   public:
      _tpMetaStore() ;
      virtual ~_tpMetaStore() ;

   public:
      INT32 initialize( const CHAR *configPath ) ;
      INT32 save() ;

   public:
      INT32 _readMeta() ;
      INT32 _writeMeta() ;

      OSS_INLINE const CHAR *getMetaFileName() const
      {
         return _metaFileName ;
      }

      OSS_INLINE UINT64 getTime() const
      {
         return _time ;
      }

      OSS_INLINE void setTime( UINT64 time )
      {
         _time = time ;
      }

      OSS_INLINE UINT32 getVersion() const
      {
         return _version ;
      }

      OSS_INLINE void setVersion( UINT32 version )
      {
         _version = version ;
      }

      OSS_INLINE UINT64 getFlushTime() const
      {
         return _flushTime ;
      }

      OSS_INLINE void setFlushTime( UINT64 flushTime )
      {
         _flushTime = flushTime ;
      }

      OSS_INLINE void increaseVersion()
      {
         ++ _version ;
      }

      OSS_INLINE void getLSN( UINT64 &time, UINT32 &version ) const
      {
         time = _time ;
         version = _version ;
      }

      OSS_INLINE void getLSN( DPS_LSN &lsn ) const
      {
         lsn.set( (DPS_LSN_OFFSET)_time, (DPS_LSN_VER)_version ) ;
      }

      OSS_INLINE void setLSN( UINT64 time, UINT32 version )
      {
         _time = time ;
         _version = version ;
      }

      OSS_INLINE void setLSN( const DPS_LSN &lsn )
      {
         _time = (UINT64)( lsn.offset ) ;
         _version = (UINT32)( lsn.version ) ;
      }

   protected:
      CHAR     _metaFileName[ OSS_MAX_PATHSIZE + 1 ] ;
      UINT64   _time ;
      UINT32   _version ;
      UINT64   _flushTime ;
   } ;

   typedef class _tpMetaStore tpMetaStore ;

}

#endif // TP_META_STORE_HPP__
