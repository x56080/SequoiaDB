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

   Source File Name = stpModule.hpp

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
#ifndef STP_MODULE_HPP__
#define STP_MODULE_HPP__

#include "oss.hpp"
#include "stpCBCommon.hpp"
#include "dpsLogDef.hpp"
#include "netRouteAgent.hpp"
#include "pmdPipeManager.hpp"
#include "stpNode.hpp"
#include "stpOptions.hpp"
#include "pmdObjBase.hpp"
#include "ossMemPool.hpp"
#include "stpServerSession.hpp"
#include "stpMetaData.hpp"
#include "stpNetManager.hpp"

namespace engine
{

   /*
      _stpModule define
    */
   // _stpModule is base class for STPCB's sub modules
   class _stpModule : public SDBObject
   {
   public:
      // constructor and destructor
      _stpModule( STPCB *stpCB ) ;
      virtual ~_stpModule() ;

   public:
      // check if module is initialized
      OSS_INLINE BOOLEAN isInitialized() const
      {
         return _initialized ;
      }

      // check if module is activated
      OSS_INLINE BOOLEAN isActivated() const
      {
         return _activated ;
      }

   public:
      // initialize module
      virtual INT32 initialize() ;
      // finalize module
      virtual INT32 finalize() ;
      // active module
      virtual INT32 activate() ;
      // deactive module
      virtual INT32 deactivate() ;

      // callback on before change primary event
      virtual INT32 beforeChangePrimary( BOOLEAN primaryIsMe ) ;

      // callback on after change primary event
      virtual INT32 afterChangePrimary( const MsgRouteID &primaryRID,
                                        BOOLEAN primaryIsMe ) ;
      // callback on change servers event
      // NOTE: version is group version
      virtual INT32 onChangeServers( UINT32 version,
                                     const STP_SERVER_LIST &servers ) ;

      // get name of module
      OSS_INLINE virtual const CHAR *getModuleName() const
      {
         return STP_MODULE_NAME ;
      }

      // get role mask of module
      // NOTE: role mask indicates if this module is used in a role
      OSS_INLINE virtual UINT32 getModuleRoleMask() const
      {
         return STP_ROLE_MASK_ALL ;
      }

   protected:
      // functions for modules to implement
      // internal call of initialize
      OSS_INLINE virtual INT32 _initialize()
      {
         return SDB_OK ;
      }

      // internal call of finalize
      OSS_INLINE virtual INT32 _finalize()
      {
         return SDB_OK ;
      }

      // internal call on event before activate
      OSS_INLINE virtual INT32 _preActivate()
      {
         return SDB_OK ;
      }

      // internal call on event after activate
      OSS_INLINE virtual INT32 _postActivate()
      {
         return SDB_OK ;
      }

      // internal call on event before deactivate
      OSS_INLINE virtual INT32 _preDeactivate()
      {
         return SDB_OK ;
      }
      // internal call on event after deactivate
      OSS_INLINE virtual INT32 _postDeactivate()
      {
         return SDB_OK ;
      }

      // internal call on event of primary change
      OSS_INLINE virtual INT32 _beforeChangePrimary( BOOLEAN primaryIsMe )
      {
         return SDB_OK ;
      }

      // internal call on event of primary change
      OSS_INLINE virtual INT32 _afterChangePrimary( const MsgRouteID &primaryRID,
                                                    BOOLEAN primaryIsMe )
      {
         return SDB_OK ;
      }

      // internal call on event of server change
      OSS_INLINE virtual INT32 _onChangeServers(
                                             UINT32 version,
                                             const STP_SERVER_LIST &servers )
      {
         return SDB_OK ;
      }

   protected:
      // pointer to STPCB
      STPCB *           _stpCB ;
      // pointer to STP options
      stpOptions *      _options ;
      // pointer to net agent
      netRouteAgent *   _netAgent ;
      // pointer to pipe manager
      pmdPipeManager *  _pipeManager ;
      // indicates module is initialized
      BOOLEAN           _initialized ;
      // indicates module is activated
      BOOLEAN           _activated ;
   } ;

   typedef class _stpModule stpModule ;
   typedef ossPoolList< stpModule * > STP_MODULE_LIST ;

   /*
      _stpHandlerBase define
    */
   // _stpHandlerBase is based class for STPCB's message or event handlers
   class _stpHandlerBase : public stpModule
   {
   public:
      // constructor and destructor
      _stpHandlerBase( STPCB *stpCB ) ;
      virtual ~_stpHandlerBase() ;

   public:
      // get functions
      OSS_INLINE stpServiceManager *getServiceManager()
      {
         return _serviceManager ;
      }

      OSS_INLINE stpNodeManager *getNodeManager()
      {
         return _nodeManager ;
      }

      OSS_INLINE stpMetaManager *getMetaManager()
      {
         return _metaManager ;
      }

      OSS_INLINE stpSyncSourceManager *getSyncSourceManager()
      {
         return _syncSourceManager ;
      }

      OSS_INLINE stpSyncClientManager *getSyncClientManager()
      {
         return _syncClientManager ;
      }

      OSS_INLINE stpReplManager *getReplManager()
      {
         return _replManager ;
      }

   protected:
      // pointer to service manager
      stpServiceManager *     _serviceManager ;
      // pointer to node manager
      stpNodeManager *        _nodeManager ;
      // pointer to meta manager
      stpMetaManager *        _metaManager ;
      // pointer to synchronize source manager
      stpSyncSourceManager *  _syncSourceManager ;
      // pointer to synchronize client manager
      stpSyncClientManager *  _syncClientManager ;
      // pointer to replica manager
      stpReplManager *        _replManager ;
   } ;

   typedef class _stpHandlerBase stpHandlerBase ;

   /*
      _stpManagerBase define
    */
   // _stpManagerBase is based class for STPCB's sub-managers
   class _stpManagerBase : public _pmdObjBase,
                           public stpHandlerBase,
                           public stpMetaHolder,
                           public netTimeoutHandler
   {
   public:
      // constructor and destructor
      _stpManagerBase( STPCB *stpCB ) ;
      virtual ~_stpManagerBase() ;

   public:
      // override functions of STP module
      // activate and deactivate
      virtual INT32 activate() ;
      virtual INT32 deactivate() ;

      // override functions of PMD object
      // attach and detach CB
      virtual void attachCB( pmdEDUCB *cb ) ;
      virtual void detachCB( pmdEDUCB *cb ) ;

      // override functions for net timeout handler
      // handler timeout
      virtual void handleTimeout( const UINT32 &millisec,
                                  const UINT32 &timerID ) ;

   public:
      // indicate if another EDU is needed to run this module
      // NOTE: FALSE means use the main thread of net agent
      OSS_INLINE virtual BOOLEAN activeEDU() const
      {
         return TRUE ;
      }

      // indicate if another timer is needed to run this module
      // NOTE: FALSE means use the main timer of net agent
      OSS_INLINE virtual BOOLEAN activeTimer() const
      {
         return TRUE ;
      }

      // handle message callback
      // NOTE: will redirect message into running EDU if needed
      virtual INT32 handleMessage( NET_HANDLE handle,
                                   const MsgHeader *message ) ;
      // process message callback
      virtual INT32 processMessage( NET_HANDLE handle,
                                    MsgHeader *message ) ;

   protected:
      // start EDU
      INT32 _startEDU() ;
      // stop EDU
      INT32 _stopEDU() ;

      // register timer
      INT32 _registerTimer() ;
      // unregister timer
      INT32 _unregisterTimer() ;

      // handle timeout in asynchronous ( redirect to running EDU )
      BOOLEAN _asyncHandleTimeout( const UINT32 &timerID,
                                   const UINT32 &millisec ) ;
      // handle message in asynchronous ( redirect to running EDU )
      BOOLEAN _asyncHandleMessage( NET_HANDLE handle,
                                   const MsgHeader *message ) ;

   protected:
      // EDU latch to protect EDU info
      // NOTE: we could create another EDU ( thread ) to handle messages for
      // this sub-manager, need latch to avoid re-start EDU
      ossSpinSLatch        _eduLatch ;
      // ID to running EDU
      EDUID                _eduID ;
      // control block of running EDU
      pmdEDUCB *           _eduCB ;
      // session to server ( used to choose server to send message )
      stpServerSession     _session ;
      // STP net agent
      stpNetManager *      _netManager ;
      // pointer to net message handler
      stpNetMsgHandler *   _netMsgHandler ;
      // pointer to pipe message handler
      stpPipeMsgHandler *  _pipeMsgHandler ;
      // timer ID for this sub-manager
      UINT64               _timerID ;
      // attach event to indicate the EDU is attached
      ossEvent             _attachEvent ;
   } ;

   typedef class _stpManagerBase stpManagerBase ;

}

#endif // STP_MODULE_HPP__
