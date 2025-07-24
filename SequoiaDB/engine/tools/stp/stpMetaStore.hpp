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

   Source File Name = stpMetaStore.hpp

   Descriptive Name = Serial Time Protocol

   When/how to use: this program may be used on binary and text-formatted
   versions of STP component. This file contains structure for Serial Time
   Protocol.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/30/2019  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef STP_META_STORE_HPP__
#define STP_META_STORE_HPP__

#include "stpCBCommon.hpp"
#include "dpsLogDef.hpp"

namespace engine
{

   /*
      _stpMetaStore define
    */
   // _stpMetaStore stores the meta LSN into meta file
   class _stpMetaStore : public SDBObject
   {
   public:
      // constructor and destructor
      _stpMetaStore() ;
      virtual ~_stpMetaStore() ;

   public:
      // initialize meta LSN store
      INT32 initialize( const CHAR *configPath ) ;
      // save meta LSN
      INT32 save() ;

   public:
      // get name of meta file
      OSS_INLINE const CHAR *getMetaFileName() const
      {
         return _metaFileName ;
      }

      // get time of meta LSN ( as offset )
      OSS_INLINE UINT64 getTime() const
      {
         return (UINT64)( _metaLSN.offset ) ;
      }

      // set time of meta LSN ( as offset )
      OSS_INLINE void setTime( UINT64 time )
      {
         _metaLSN.offset = (DPS_LSN_OFFSET)time ;
      }

      // get version of meta LSN
      OSS_INLINE UINT32 getVersion() const
      {
         return (UINT32)( _metaLSN.version ) ;
      }

      // set version of meta LSN
      OSS_INLINE void setVersion( UINT32 version )
      {
         _metaLSN.version = (DPS_LSN_VER)version ;
      }

      // get flush time of meta file
      OSS_INLINE UINT64 getFlushTime() const
      {
         return _flushTime ;
      }

      // set flush time of meta file
      OSS_INLINE void setFlushTime( UINT64 flushTime )
      {
         _flushTime = flushTime ;
      }

      // increase version
      OSS_INLINE void increaseVersion()
      {
         ++ _metaLSN.version ;
      }

      // get meta LSN in time and version
      OSS_INLINE void getLSN( UINT64 &time, UINT32 &version ) const
      {
         time = (UINT64)( _metaLSN.offset ) ;
         version = (UINT32)( _metaLSN.version ) ;
      }

      // get meta LSN in LSN format
      OSS_INLINE void getLSN( DPS_LSN &lsn ) const
      {
         lsn = _metaLSN ;
      }

      // set meta LSN by time and version
      OSS_INLINE void setLSN( UINT64 time, UINT32 version )
      {
         _metaLSN.set( time, version ) ;
      }

      // set meta LSN by LSN format
      OSS_INLINE void setLSN( const DPS_LSN &lsn )
      {
         _metaLSN = lsn ;
      }

   protected:
      // read meta LSN from meta file
      INT32 _readMeta() ;
      // write meta LSN to meta file
      INT32 _writeMeta() ;

   protected:
      // file name of meta file
      CHAR     _metaFileName[ OSS_MAX_PATHSIZE + 1 ] ;

      // below is cache of the content of meta file

      // meta LSN includes time of primary server and version of
      // replica server group, is used for replica vote
      // - time ( as offset ) is updated by current logical time of primary
      //   server
      // - version is updated by replica vote ( primary switch to increase )
      DPS_LSN  _metaLSN ;

      // time to flush meta file
      UINT64   _flushTime ;
   } ;

   typedef class _stpMetaStore stpMetaStore ;

}

#endif // STP_META_STORE_HPP__
