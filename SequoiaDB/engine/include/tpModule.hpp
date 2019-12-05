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

   Source File Name = tpModule.hpp

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

#ifndef TP_MODULE_HPP__
#define TP_MODULE_HPP__

#include "oss.hpp"
#include "tpCBCommon.hpp"
#include "dpsLogDef.hpp"
#include "netRouteAgent.hpp"
#include "pmdPipeManager.hpp"
#include "tpNode.hpp"
#include "tpOptions.hpp"
#include "pmdObjBase.hpp"
#include "ossMemPool.hpp"
#include "tpServerSession.hpp"
#include "tpMetaData.hpp"

namespace engine
{

   /*
      _tpModule define
    */
   class _tpModule : public SDBObject
   {
   public:
      _tpModule( SDB_TPCB *tpCB ) ;
      virtual ~_tpModule() ;

   public:
      OSS_INLINE BOOLEAN isInitialized() const
      {
         return _initialized ;
      }

      OSS_INLINE BOOLEAN isActivated() const
      {
         return _activated ;
      }

   public:
      virtual INT32 initialize() ;
      virtual INT32 finalize() ;
      virtual INT32 activate() ;
      virtual INT32 deactivate() ;

      virtual INT32 onChangePrimary( const MsgRouteID &primaryRID,
                                     BOOLEAN isLocalPrimary ) ;
      virtual INT32 onChangeServers( UINT32 version,
                                     const TP_SERVER_LIST &servers ) ;

      OSS_INLINE virtual const CHAR *getModuleName() const
      {
         return TP_MODULE_NAME ;
      }

      OSS_INLINE virtual UINT32 getModuleRoleMask() const
      {
         return TP_ROLE_MASK_ALL ;
      }

   protected :
      OSS_INLINE virtual INT32 _initialize()
      {
         return SDB_OK ;
      }

      OSS_INLINE virtual INT32 _finalize()
      {
         return SDB_OK ;
      }

      OSS_INLINE virtual INT32 _preActivate()
      {
         return SDB_OK ;
      }

      OSS_INLINE virtual INT32 _postActivate()
      {
         return SDB_OK ;
      }

      OSS_INLINE virtual INT32 _preDeactivate()
      {
         return SDB_OK ;
      }

      OSS_INLINE virtual INT32 _postDeactivate()
      {
         return SDB_OK ;
      }

      OSS_INLINE virtual INT32 _onChangePrimary( const MsgRouteID &primaryRID,
                                                 BOOLEAN isLocalPrimary )
      {
         return SDB_OK ;
      }

      OSS_INLINE virtual INT32 _onChangeServers( UINT32 version,
                                                 const TP_SERVER_LIST &servers )
      {
         return SDB_OK ;
      }

   protected:
      SDB_TPCB *           _tpCB ;
      tpOptions *          _options ;
      netRouteAgent *      _netAgent ;
      pmdPipeManager *     _pipeManager ;
      BOOLEAN              _initialized ;
      BOOLEAN              _activated ;
   } ;

   typedef class _tpModule tpModule ;
   typedef ossPoolList< tpModule * > TP_MODULE_LIST ;


   /*
      _tpHandlerBase define
    */
   class _tpHandlerBase : public tpModule
   {
   public:
      _tpHandlerBase( SDB_TPCB *tpCB ) ;
      virtual ~_tpHandlerBase() ;

   protected:
      tpServiceManager *   _serviceManager ;
      tpCatalogManager *   _catalogManager ;
      tpMetaManager *      _metaManager ;
      tpSourceManager *    _sourceManager ;
      tpSyncManager *      _syncManager ;
      tpReplManager *      _replManager ;
   } ;

   typedef class _tpHandlerBase tpHandlerBase ;

   /*
      _tpManagerBase define
    */
   class _tpManagerBase : public _pmdObjBase,
                          public tpHandlerBase,
                          public tpMetaHolder,
                          public netTimeoutHandler
   {
   public:
      _tpManagerBase( SDB_TPCB *tpCB ) ;
      virtual ~_tpManagerBase() ;

   public:
      virtual INT32 activate() ;
      virtual INT32 deactivate() ;

      virtual void attachCB( pmdEDUCB *cb ) ;
      virtual void detachCB( pmdEDUCB *cb ) ;

      virtual void handleTimeout( const UINT32 &millisec,
                                  const UINT32 &timerID ) ;

   public:
      OSS_INLINE virtual BOOLEAN activeEDU() const
      {
         return TRUE ;
      }

      OSS_INLINE virtual BOOLEAN activeTimer() const
      {
         return TRUE ;
      }

      virtual INT32 handleMessage( NET_HANDLE handle,
                                   const MsgHeader *message ) ;
      virtual INT32 processMessage( NET_HANDLE handle,
                                    MsgHeader *message ) ;

   protected:
      INT32 _startEDU() ;
      INT32 _stopEDU() ;

      INT32 _registerTimer() ;
      INT32 _unregisterTimer() ;

      BOOLEAN _asyncHandleTimeout( const UINT32 &timerID,
                                   const UINT32 &millisec ) ;
      BOOLEAN _asyncHandleMessage( NET_HANDLE handle,
                                   const MsgHeader *message ) ;

      void _fillRequestHeader( MsgHeader &request,
                               UINT32 requestSize,
                               INT32 opCode ) ;

      void _fillReplyHeader( const MsgHeader &request,
                             MsgOpReply &reply,
                             UINT32 replySize,
                             INT32 returnCode ) ;

      void _fillReplyHeader( const MsgHeader &request,
                             MsgInternalReplyHeader &reply,
                             UINT32 replySize,
                             INT32 returnCode ) ;

   protected:
      ossSpinSLatch           _eduLatch ;
      EDUID                   _eduID ;
      pmdEDUCB *              _eduCB ;
      tpServerSession         _session ;
      tpNetMsgHandler *       _netMsgHandler ;
      tpPipeMsgHandler *      _pipeMsgHandler ;
      UINT64                  _timerID ;
      ossEvent                _attachEvent ;
   } ;

   typedef class _tpManagerBase tpManagerBase ;

}

#endif // TP_MODULE_HPP__
