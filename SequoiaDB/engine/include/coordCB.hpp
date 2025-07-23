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

   Source File Name = coordCB.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/28/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef COORDCB_HPP__
#define COORDCB_HPP__

#include "netRouteAgent.hpp"
#include "ossUtil.h"
#include "coordRemoteSession.hpp"
#include "pmdRemoteMsgEventHandler.hpp"
#include "sdbInterface.hpp"
#include "dmsCB.hpp"
#include "pmdEDU.hpp"
#include "rtnCB.hpp"
#include "dpsLogWrapper.hpp"
#include "msgCatalog.hpp"
#include "pmdObjBase.hpp"
#include "rtn.hpp"
#include "clsRegAssit.hpp"
#include "ossMemPool.hpp"
#include "coordDataSource.hpp"

using namespace std ;

namespace engine
{
   /*
      _CoordCB define
   */
   class _CoordCB : public _pmdObjBase, public _IControlBlock, public _IExecutorEventHandler
   {
      DECLARE_OBJ_MSG_MAP()

      typedef ossPoolMap< SINT64, UINT64>    CONTEXT_LIST ;

      typedef struct _eventInfo
      {
         UINT64      _requestID ;
         UINT64      _dbTick ;
         SINT32      _eventType ;

         _eventInfo()
         {
            _requestID = 0 ;
            _dbTick    = 0 ;
            _eventType = 0 ;
         }
      } eventInfo ;

      //  Key: UINT64( Handle, TID) -> eventInfo
      typedef ossPoolMap< UINT64, eventInfo> EVENT_MAP ;

      public:
         _CoordCB() ;
         virtual ~_CoordCB() ;

         virtual SDB_CB_TYPE cbType() const { return SDB_CB_COORD ; }
         virtual const CHAR* cbName() const { return "COORDCB" ; }

         virtual INT32  init () ;
         virtual INT32  active () ;
         virtual INT32  deactive () ;
         virtual INT32  fini () ;
         virtual void   onConfigChange() ;

         virtual void   attachCB( _pmdEDUCB *cb ) ;
         virtual void   detachCB( _pmdEDUCB *cb ) ;

         virtual INT32 getMaxProcMsgTime() const { return 2 ; }
         virtual INT32 getMaxProcEventTime() const { return 2 ; }

         UINT32         setTimer( UINT32 milliSec ) ;
         void           killTimer( UINT32 timerID ) ;

         coordResource* getResource() ;
         netRouteAgent* getRouteAgent() ;
         pmdRemoteSessionMgr* getRSManager() ;
         coordDataSourceMgr*  getDSManager() ;

      public:
         virtual INT32  onRecieve( const UINT32 &handle,
                                   const MsgHeader *header,
                                   const CHAR *pMsg ) ;

      protected:
         virtual void onTimer ( UINT64 timerID, UINT32 interval ) ;
         INT32 _reply ( const NET_HANDLE &handle,
                        MsgOpReply *pReply,
                        const CHAR *pReplyData = NULL,
                        UINT32 replyDataLen = 0 ) ;
         INT32 _sendRegisterMsg () ;
         INT32 _onCatRegisterRes ( NET_HANDLE handle, MsgHeader *pMsg ) ;
         INT32 _defaultMsgFunc( NET_HANDLE handle, MsgHeader *pMsg ) ;
         void  _onMsgBegin( NET_HANDLE handle, MsgHeader *pMsg ) ;
         void  _onMsgEnd() ;
         INT32 _processMsg( const NET_HANDLE &handle, MsgHeader *pMsg ) ;
         INT32 _processGetMoreMsg ( const NET_HANDLE &handle,
                                    MsgHeader *pMsg,
                                    rtnContextBuf &buffObj,
                                    INT64 &contextID ) ;
         INT32 _processAdvanceMsg ( const NET_HANDLE &handle,
                                    MsgHeader *pMsg,
                                    rtnContextBuf &buffObj,
                                    INT64 &contextID ) ;
         INT32 _processKillContext( MsgHeader *pMsg ) ;
         INT32 _processRemoteDisc( const NET_HANDLE &handle,
                                   MsgHeader *pMsg ) ;
         INT32 _processMsgReq( MsgHeader *pMsg ) ;
         INT32 _filterQueryCmd( _rtnCommand *pCommand,
                                MsgHeader *pMsg ) ;
         INT32 _processQueryMsg( const NET_HANDLE &handle,
                                 MsgHeader *pMsg,
                                 rtnContextBuf &buffObj,
                                 INT64 &contextID ) ;
         INT32 _processSessionInit( MsgHeader *pMsg ) ;
         INT32 _processInterruptMsg( const NET_HANDLE & handle,
                                     MsgHeader * header ) ;
         INT32 _processDisconnectMsg( const NET_HANDLE & handle,
                                      MsgHeader * header ) ;
         INT32 _processPacketMsg( const NET_HANDLE & handle,
                                  MsgHeader * header,
                                  INT64 &contextID,
                                  rtnContextBuf &buf ) ;
         void _delContextByHandle( const UINT32 &handle ) ;
         void _delContext( const UINT32 &handle, UINT32 tid ) ;
         void _delContextByID( INT64 contextID, BOOLEAN rtnDel ) ;
         void _addContext( const UINT32 &handle, UINT32 tid, INT64 contextID ) ;
         INT32 _sendToCatlog ( MsgHeader *pMsg, NET_HANDLE *pHandle = NULL ) ;
         INT32 _processUpdateGrpInfo () ;
         INT32 _processCatGrpChgNty () ;

         void  _delEventByHandle( const UINT32 &handle ) ;
         void  _delEventExpired() ;
         INT32 _checkEvent( const UINT32 &handle, MsgHeader *pMsg ) ;

      private:

         coordResource                 _resource ;
         pmdRemoteSessionMgr           _remoteSessionMgr ;
         coordSessionPropMgr           _sitePropMgr ;
         coordDataSourceMgr            _dsMgr ;

         pmdRemoteMsgHandler           *_pMsgHandler ;
         pmdRemoteTimerHandler         *_pTimerHandler ;
         netRouteAgent                 *_pAgent ;

         UINT16                        _shardServiceID ;
         CHAR                          _shdServiceName[OSS_MAX_SERVICENAME+1] ;
         _MsgRouteID                   _selfNodeID ;

         UINT64                        _regTimerID ;
         UINT64                        _clearEventTimerID ;

         ossEvent                      _attachEvent ;

         const CHAR                    *_pCollectionName ;
         std::string                   _cmdCollectionName ;

         _SDB_DMSCB                    *_pDmsCB;
         _dpsLogWrapper                *_pDpsCB;
         _SDB_RTNCB                    *_pRtnCB;
         pmdEDUCB                      *_pEDUCB;

         ossSpinXLatch                 _contextLatch ;
         CONTEXT_LIST                  _contextLst;

         EVENT_MAP                     _eventMap ;
         UINT32                        _curHandle ;
         UINT32                        _curTID ;
         UINT64                        _curReqID ;

         MsgOpReply                    _replyHeader ;
         BOOLEAN                       _needReply ;
         BSONObj                       _errorInfo ;

         UINT32                        _inPacketLevel ;
         INT64                         _pendingContextID ;
         rtnContextBuf                 _pendingBuff ;
   } ;
   typedef _CoordCB CoordCB ;

   /*
      get global coord cb
   */
   CoordCB* sdbGetCoordCB() ;

}

#endif // COORDCB_HPP__

