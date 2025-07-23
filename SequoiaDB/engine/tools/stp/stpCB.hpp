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

   Source File Name = stpCB.hpp

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
#ifndef STP_CB_HPP__
#define STP_CB_HPP__

#include "stpCBCommon.hpp"
#include "stpMsg.hpp"
#include "stpOptions.hpp"
#include "stpMsgHandler.hpp"
#include "stpNetManager.hpp"
#include "stpServiceManager.hpp"
#include "stpNodeManager.hpp"
#include "stpMetaManager.hpp"
#include "stpSyncSourceManager.hpp"
#include "stpSyncClientManager.hpp"
#include "stpReplManager.hpp"
#include "stpLogicalTime.hpp"
#include "ossMemPool.hpp"
#include "netRouteAgent.hpp"
#include "pmdPipeManager.hpp"
#include "pmdObjBase.hpp"

namespace engine
{

   /*
      _stpConfigHandle define
    */
   class _stpConfigHandle : public _IConfigHandle
   {
   public:
      _stpConfigHandle( IControlBlock *cb )
      : _cb( cb )
      {
      }

      virtual ~_stpConfigHandle()
      {
      }

      virtual void onConfigChange( UINT32 changeID )
      {
         _cb->onConfigChange() ;
      }

      virtual void onConfigSave()
      {
         _cb->onConfigSave() ;
      }

      virtual INT32 onConfigInit()
      {
         return SDB_OK ;
      }

   protected:
      IControlBlock * _cb ;
   } ;

   typedef class _stpConfigHandle stpConfigHandle ;

   /*
      _stpCB define
    */
   // _stpCB is control block of STP
   class _stpCB : public _pmdObjBase,
                  public IControlBlock
   {
   public:
      // constructor and destructor
      _stpCB() ;
      virtual ~_stpCB() ;

   public:
      // override functions of control block
      // get type control block
      OSS_INLINE virtual SDB_CB_TYPE cbType() const
      {
         return SDB_CB_STP ;
      }

      // get name of control block
      OSS_INLINE virtual const CHAR *cbName() const
      {
         return "STPCB" ;
      }

      // initialize control block
      virtual INT32 init() ;
      // active control block
      virtual INT32 active() ;
      // deactive control block
      virtual INT32 deactive() ;
      // finalize control block
      virtual INT32 fini() ;

      // on event of config change
      virtual void  onConfigChange() ;

   public:
      // get options
      OSS_INLINE stpOptions *getOptions()
      {
         return ( &_options ) ;
      }

      // get net route agent
      OSS_INLINE netRouteAgent *getNetAgent()
      {
         return _netManager.getNetAgent() ;
      }

      // get net agent
      OSS_INLINE stpNetManager *getNetManager()
      {
         return ( &_netManager ) ;
      }

      // get pipe manager
      OSS_INLINE pmdPipeManager *getPipeManager()
      {
         return ( &_pipeManager ) ;
      }

      // get net message handler
      OSS_INLINE stpNetMsgHandler *getNetMsgHandler()
      {
         return ( &_netMsgHandler ) ;
      }

      // get pipe message handler
      OSS_INLINE stpPipeMsgHandler *getPipeMsgHandler()
      {
         return ( &_pipeMsgHandler ) ;
      }

      // get service manager
      OSS_INLINE stpServiceManager *getServiceManager()
      {
         return ( &_serviceManager ) ;
      }

      // get node manager
      OSS_INLINE stpNodeManager *getNodeManager()
      {
         return ( &_nodeManager ) ;
      }

      // get meta manager
      OSS_INLINE stpMetaManager *getMetaManager()
      {
         return ( &_metaManager ) ;
      }

      // get synchronize source manager
      OSS_INLINE stpSyncSourceManager *getSyncSourceManager()
      {
         return ( &_syncSourceManager ) ;
      }

      // get synchronize client manager
      OSS_INLINE stpSyncClientManager *getSyncClientManager()
      {
         return ( &_syncClientManager ) ;
      }

      // get replica manager
      OSS_INLINE stpReplManager *getReplManager()
      {
         return ( &_replManager ) ;
      }

      // get meta data
      OSS_INLINE stpMetaData *getMetaData()
      {
         return _metaManager.getMetaData() ;
      }

      // check if local is a primary server, it will wait for reelection
      OSS_INLINE BOOLEAN checkPrimaryServer( pmdEDUCB *cb )
      {
         return ( _nodeManager.isServer() &&
                  ( SDB_OK == _replManager.primaryCheck( cb ) ) ) ;
      }

      // check if local is a primary server, it will not wait for reelection
      OSS_INLINE BOOLEAN isPrimaryServer()
      {
         return ( pmdIsPrimary() &&
                  _nodeManager.isPrimaryServer() ) ;
      }

      // check if local is primary
      // NOTE: could be test mode
      OSS_INLINE BOOLEAN isPrimary()
      {
         return _nodeManager.isPrimary() ;
      }

      // check if local is a secondary server
      OSS_INLINE BOOLEAN isSecondaryServer()
      {
         return _nodeManager.isSecondaryServer() ;
      }

      // callback before primary change
      INT32 beforeChangePrimary( BOOLEAN isLocalPrimary ) ;

      // callback after primary change
      INT32 afterChangePrimary( const MsgRouteID &primaryRID,
                                BOOLEAN isLocalPrimary ) ;
      // callback on servers change
      INT32 onChangeServers() ;
      // callback on role change
      INT32 onChangeRole( STP_ROLE role ) ;

      virtual void onTimer( UINT64 timerID, UINT32 interval ) ;

   protected:
      // register module
      INT32 _registerModule( stpModule *module ) ;
      // unregister all modules
      void  _unregisterAllModules() ;
      // initialize modules
      INT32 _initializeModules() ;

   protected:
      // check time extra info
      void _checkTimeExInfo() ;
      // initialize net agent
      INT32 _initNetAgent() ;
      // initialize pipe manager
      INT32 _initPipeManager() ;
      // activate system clock ( calculate time interval with CPU tick )
      INT32 _activeSystemClock() ;
      // activate net agent
      INT32 _activeNetAgent() ;
      // activate pipe manager
      INT32 _activePipeManager() ;

   protected:
      // options from config file
      stpOptions           _options ;
      // config handler
      stpConfigHandle      _configHandler ;
      // net manager
      stpNetManager        _netManager ;
      // pipe manager ( owned by STP )
      pmdPipeManager       _pipeManager ;
      // net message handler processes network message
      stpNetMsgHandler     _netMsgHandler ;
      // pipe message handler processes pipe message
      stpPipeMsgHandler    _pipeMsgHandler ;
      // service manager handles sessions from client ( sdbshell )
      stpServiceManager    _serviceManager ;
      // node manager handles node information, including servers,
      // primary, etc.
      stpNodeManager       _nodeManager ;
      // meta manager handles meta data, including shared memory and meta
      // LSN synchronize
      stpMetaManager       _metaManager ;
      // synchronize source manager handles time synchronize as source
      stpSyncSourceManager _syncSourceManager ;
      // synchronize client manager handles time synchronize as client
      stpSyncClientManager _syncClientManager ;
      // replica manager handles replica vote between servers
      stpReplManager       _replManager ;
      // module list registered to STP
      // register all modules ( node manager, etc), into module list, and
      // call initialize, active functions by iterating module list
      STP_MODULE_LIST      _moduleList ;

      UINT64               _checkTimeout ;
   } ;

}

#endif // STP_CB_HPP__
