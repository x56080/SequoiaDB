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

   Source File Name = stpMetaManager.hpp

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
#ifndef STP_META_MANAGER_HPP__
#define STP_META_MANAGER_HPP__

#include "stpCBCommon.hpp"
#include "stpModule.hpp"
#include "utilSHMBuffer.hpp"
#include "stpMetaStore.hpp"
#include "stpMsg.hpp"
#include "stpTimeMapManager.hpp"

namespace engine
{

   /*
      _stpMetaManager define
    */
   // _stpMetaManager manages meta data including shared memory and meta LSN
   class _stpMetaManager : public stpManagerBase
   {
      DECLARE_OBJ_MSG_MAP()

   public:
      // constructor and destructor
      _stpMetaManager( STPCB *stpCB ) ;
      virtual ~_stpMetaManager () ;

   public:
      // get name of module
      OSS_INLINE virtual const CHAR *getModuleName() const
      {
         return STP_META_MANAGER_NAME ;
      }

      // on timer callback
      virtual void onTimer( UINT64 timerID, UINT32 interval ) ;

      // process message callback
      virtual INT32 processMessage( NET_HANDLE handle,
                                    MsgHeader *message ) ;

   protected:
      // internal initialize
      virtual INT32 _initialize() ;
      // internal finalize
      virtual INT32 _finalize() ;
      // on event after activated
      virtual INT32 _postActivate() ;

      // on event of before primary change
      virtual INT32 _beforeChangePrimary( BOOLEAN primaryIsMe ) ;

   public:
      // get key of shared memory ( service name )
      OSS_INLINE const CHAR *getSHMKey() const
      {
         return _buffer.getKeyString() ;
      }

      OSS_INLINE stpTimeMapManager *getTimeMapManager()
      {
         return &_timeMapMgr ;
      }

   protected:
      // handle meta notify ( need update meta LSN )
      INT32 _handleMetaNotify( NET_HANDLE handle,
                               const stpMetaNotify *notify ) ;
      // handle meta synchronize request
      INT32 _handleMetaSyncReq( NET_HANDLE handle,
                               const stpMetaSyncReq *request ) ;
      // handle meta synchronize response
      INT32 _handleMetaSyncRsp( NET_HANDLE handle,
                                const stpMetaSyncRsp *response ) ;

      // send meta notify
      INT32 _sendMetaNotify( const MsgRouteID &routeID ) ;
      // send meta synchronize request
      INT32 _sendMetaSyncReq( const MsgRouteID &routeID ) ;
      // send meta synchronize response
      INT32 _sendMetaSyncRsp( NET_HANDLE handle,
                              const stpMetaSyncReq *request,
                              UINT64 time,
                              UINT32 version,
                              INT32 returnCode ) ;

   public:
      // launch meta synchronize
      INT32 launchMetaSync() ;
      // broadcast meta notify
      INT32 broadcastMetaNotify() ;
      // get meta LSN
      INT32 getMetaLSN( UINT64 &time, UINT32 &version ) ;
      // get meta LSN in DPS_LSN format
      INT32 getMetaLSN( DPS_LSN &lsn ) ;
      // update meta LSN
      INT32 updateMetaLSN( const DPS_LSN &metaLSN ) ;
      // update meta LSN by getting logical time
      INT32 updateMetaLSN() ;

      // convert real time to logical time
      INT32 convRTimeToLTime( const stpHPTime &realTime,
                              stpHPTime &logicalTime ) ;
      // convert logical time to real time
      INT32 convLTimeToRTime( const stpHPTime &logicalTime,
                              stpHPTime &realTime ) ;

   protected:
      // set meta LSN
      INT32 _setMetaLSN( UINT64 time, UINT32 version ) ;
      // update meta LSN
      INT32 _updateMetaLSN( UINT64 time,
                            BOOLEAN increaseVersion,
                            BOOLEAN &updated ) ;
      // update meta LSN
      INT32 _updateMetaLSN( const DPS_LSN &metaLSN, BOOLEAN &updated ) ;

      // get synchronize interval of meta data
      UINT64 _getMetaSyncInterval() ;

   protected:
      // lock to protect meta
      ossRWMutex     _mutex ;
      // shared memory buffer to meta data
      utilSHMBuffer  _buffer ;
      // meta LSN store
      stpMetaStore   _store ;
      // timeout to synchronize meta
      UINT64         _metaSyncTimeout ;

      ossRWMutex        _timeMapMutex ;
      stpHPTime         _lastRealTime ;
      stpLogicalTimeNS  _lastLogicalTime ;

      stpTimeMapManager _timeMapMgr ;
   } ;

}

#endif // STP_META_MANAGER_HPP__
