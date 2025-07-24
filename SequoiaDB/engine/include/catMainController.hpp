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

   Source File Name = catMainController.hpp

   Descriptive Name = Process MoDel Header

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains structure kernel control block,
   which is the most critical data structure in the engine process.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef CAT_MAIN_CONTROLLER_HPP__
#define CAT_MAIN_CONTROLLER_HPP__

#include "pmdObjBase.hpp"
#include "netMsgHandler.hpp"
#include "netTimer.hpp"
#include "msgCatalog.hpp"
#include "pmdEDU.hpp"
#include "pmd.hpp"
#include "ossEvent.hpp"
#include "catCatalogManager.hpp"
#include "catNodeManager.hpp"
#include "utilMap.hpp"

#include <vector>

namespace engine
{

   class _SDB_DMSCB ;
   class _SDB_RTNCB ;
   class _authCB ;
   class sdbCatalogueCB ;

   /*
      catMainController define
   */
   class catMainController : public _pmdObjBase, public _netMsgHandler,
                             public _netTimeoutHandler
   {
   typedef _utilMap< SINT64, UINT64, 20 >    CONTEXT_LIST ;
   typedef std::vector< pmdEDUEvent >        VEC_EVENT ;

   DECLARE_OBJ_MSG_MAP()

   public:
      catMainController() ;
      virtual ~catMainController() ;
      INT32 init() ;

      virtual void   attachCB( _pmdEDUCB *cb ) ;
      virtual void   detachCB( _pmdEDUCB *cb ) ;

      virtual void   onTimer ( UINT64 timerID, UINT32 interval ) ;

      ossEvent* getAttachEvent() { return &_attachEvent ; }
      ossEvent* getChangeEvent() { return &_changeEvent ; }

      BOOLEAN   delayCurOperation() ;
      BOOLEAN   isDelayed() const { return _isDelayed ; }

   public:
      INT32 handleMsg( const NET_HANDLE &handle,
                       const _MsgHeader *header,
                       const CHAR *msg ) ;
      void  handleClose( const NET_HANDLE &handle, _MsgRouteID id ) ;

      void  handleTimeout( const UINT32 &millisec, const UINT32 &id ) ;

   protected:
      virtual INT32 _defaultMsgFunc ( NET_HANDLE handle,
                                      MsgHeader* msg ) ;

      INT32 _processMsg( const NET_HANDLE &handle, MsgHeader *pMsg ) ;

      void  _dispatchDelayedOperation( BOOLEAN dispatch ) ;

   // event process functions
   protected:
      INT32 _onActiveEvent( pmdEDUEvent *event ) ;
      INT32 _onDeactiveEvent( pmdEDUEvent *event ) ;

   // msg process functions
   protected :
      INT32 _processGetMoreMsg ( const NET_HANDLE &handle, MsgHeader *pMsg ) ;
      INT32 _processQueryDataGrp( const NET_HANDLE &handle, MsgHeader *pMsg ) ;
      INT32 _processQueryCollections( const NET_HANDLE &handle,
                                      MsgHeader *pMsg ) ;
      INT32 _processQueryCollectionSpaces ( const NET_HANDLE &handle,
                                            MsgHeader *pMsg ) ;
      INT32 _processQueryMsg( const NET_HANDLE &handle, MsgHeader *pMsg ) ;
      INT32 _processKillContext(const NET_HANDLE &handle, MsgHeader *pMsg ) ;
      INT32 _processAuthenticate( const NET_HANDLE &handle, MsgHeader *pMsg ) ;
      INT32 _processAuthCrt( const NET_HANDLE &handle, MsgHeader *pMsg ) ;
      INT32 _processAuthDel( const NET_HANDLE &handle, MsgHeader *pMsg ) ;
      INT32 _processSessionInit( const NET_HANDLE &handle, MsgHeader *pMsg ) ;
      INT32 _processRemoteDisc ( const NET_HANDLE &handle, MsgHeader *pMsg ) ;
      INT32 _processInterruptMsg( const NET_HANDLE &handle,
                                  MsgHeader *header ) ;
      INT32 _processDisconnectMsg( const NET_HANDLE &handle,
                                   MsgHeader *header ) ;
      INT32 _processQueryRequest ( const NET_HANDLE &handle,
                                   MsgHeader *pMsg,
                                   const CHAR *pCollectionName ) ;

   protected:
      INT32 _postMsg( const NET_HANDLE &handle, const MsgHeader *pHead ) ;
      INT32 _catBuildMsgEvent ( const NET_HANDLE &handle,
                                const MsgHeader *pMsg,
                                pmdEDUEvent &event ) ;
      INT32 _ensureMetadata() ;
      INT32 _createSysIndex ( const CHAR *pCollection,
                              const CHAR *pIndex,
                              pmdEDUCB *cb ) ;
      INT32 _createSysCollection ( const CHAR *pCollection,
                                   pmdEDUCB *cb ) ;
      void _addContext( const UINT32 &handle, UINT32 tid, INT64 contextID ) ;
      void _delContextByHandle( const UINT32 &handle ) ;
      void _delContext( const UINT32 &handle, UINT32 tid ) ;
      void _delContextByID( INT64 contextID, BOOLEAN rtnDel ) ;

   private :
      pmdEDUMgr         *_pEduMgr;
      sdbCatalogueCB    *_pCatCB;
      _SDB_DMSCB        *_pDmsCB;
      pmdEDUCB          *_pEDUCB;
      _SDB_RTNCB        *_pRtnCB;
      _authCB           *_pAuthCB;
      CONTEXT_LIST      _contextLst;
      ossSpinXLatch     _contextLatch ;

      ossEvent          _attachEvent ;

      ossEvent          _changeEvent ;

      // for cata lock
      VEC_EVENT         _vecEvent ;
      UINT32            _checkEventTimerID ;
      pmdEDUEvent       _lastDelayEvent ;
      BOOLEAN           _isDelayed ;

   } ;

}

#endif // CAT_MAIN_CONTROLLER_HPP__

