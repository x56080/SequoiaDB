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

   Source File Name = omManager.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/15/2014  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OM_MANAGER_HPP__
#define OM_MANAGER_HPP__

#include "omDef.hpp"
#include "ossLatch.hpp"
#include "pmdObjBase.hpp"
#include "sdbInterface.hpp"
#include "pmdEDU.hpp"
#include "pmd.hpp"
#include "dmsCB.hpp"
#include "rtnCB.hpp"
#include "netRouteAgent.hpp"
#include "pmdRemoteSession.hpp"
#include "omMsgEventHandler.hpp"

#include <vector>
#include <string>
#include <map>

using namespace std ;
using namespace bson ;

namespace engine
{
   class _pmdEDUCB ;

   /*
      omAgentInfo define
   */
   struct omAgentInfo
   {
      UINT64   _id ;
      string   _host ;
      string   _service ;
   } ;

   class omHostVersion ;
   class omTaskManager ;
   /*
      _omManager define
   */
   class _omManager : public _pmdObjBase, public _IControlBlock, 
                      public IEventHander 
   {
      DECLARE_OBJ_MSG_MAP()

      typedef map< UINT64, omAgentInfo* >    MAP_ID2HOSTPTR ;
      typedef MAP_ID2HOSTPTR::iterator       MAP_ID2HOSTPTR_IT ;

      typedef map< string, omAgentInfo >     MAP_HOST2ID ;
      typedef MAP_HOST2ID::iterator          MAP_HOST2ID_IT ;

      public:
         _omManager() ;
         virtual ~_omManager() ;

         virtual SDB_CB_TYPE cbType() const { return SDB_CB_OMSVC ; }
         virtual const CHAR* cbName() const { return "OMSVC" ; }

         virtual INT32  init () ;
         virtual INT32  active () ;
         virtual INT32  deactive () ;
         virtual INT32  fini () ;
         virtual void   onConfigChange() {}

         virtual void   attachCB( _pmdEDUCB *cb ) ;
         virtual void   detachCB( _pmdEDUCB *cb ) ;

         UINT32      setTimer( UINT32 milliSec ) ;
         void        killTimer( UINT32 timerID ) ;

         // comm interface
         netRouteAgent* getRouteAgent() ;
         MsgRouteID     updateAgentInfo( const string &host,
                                         const string &service ) ;
         MsgRouteID     getAgentIDByHost( const string &host ) ;
         INT32          getHostInfoByID( MsgRouteID routeID,
                                         string &host,
                                         string &service ) ;
         void           delAgent( MsgRouteID routeID ) ;
         void           delAgent( const string &host ) ;

         pmdRemoteSessionMgr* getRSManager() { return &_rsManager ; }

         INT32             authenticate( BSONObj &obj, _pmdEDUCB *cb ) ;
         INT32             authUpdatePasswd( string user, string oldPasswd,
                                             string newPasswd, pmdEDUCB *cb ) ;

         INT32             getBizHostInfo( const string &businessName, 
                                           list <string> &hostsList ) ;
         INT32             appendBizHostInfo( const string &businessName, 
                                              list <string> &hostsList ) ;

         string            getLocalAgentPort() ;

         INT32             refreshVersions() ;
         void              updateClusterVersion( string cluster ) ;
         void              removeClusterVersion( string cluster ) ;

         omTaskManager     *getTaskManager() ;

      public:
         virtual void   onRegistered( const MsgRouteID &nodeID ) ;
         virtual void   onPrimaryChange( BOOLEAN primary,
                                         SDB_EVENT_OCCUR_TYPE occurType ) ;

      protected:
         virtual void      onTimer ( UINT64 timerID, UINT32 interval ) ;

         MsgRouteID        _incNodeID() ;

         INT32             _initOmTables() ;

         INT32             _appendBusinessInfo( const string &businessName, 
                                                const string &businessType, 
                                                const string &clusterName,
                                                const string &deployMode ) ;

         INT32             _getBussinessInfo( const string &businessName, 
                                              string &businessType, 
                                              string &clusterName,
                                              string &deployMode ) ;

         INT32             _updateConfTable() ;
         INT32             _updateBusinessTable() ;
         INT32             _updateTable() ;

         INT32             _createJobs() ;

         INT32             _createCollectionIndex ( const CHAR *pCollection,
                                                    const CHAR *pIndex,
                                                    pmdEDUCB *cb ) ;

         INT32             _createCollection ( const CHAR *pCollection,
                                               pmdEDUCB *cb ) ;
         void              _readAgentPort() ;

         INT32             _onAgentQueryTaskReq( NET_HANDLE handle, 
                                                 MsgHeader *pMsg ) ;
         INT32             _onAgentUpdateTaskReq( NET_HANDLE handle, 
                                                  MsgHeader *pMsg ) ;
         BOOLEAN           _isCommand( const CHAR *pCheckName ) ;
         void              _sendResVector2Agent( NET_HANDLE handle, 
                                                 MsgHeader *pSrcMsg, 
                                                 INT32 flag, 
                                                 vector < BSONObj > &objs ) ;
         void              _sendRes2Agent( NET_HANDLE handle, 
                                           MsgHeader *pSrcMsg, 
                                           INT32 flag, BSONObj &obj ) ;
         void              _sendRes2Agent( NET_HANDLE handle, 
                                           MsgHeader *pSrcMsg, 
                                           INT32 flag, 
                                           rtnContextBuf &buffObj ) ;

         void              _checkSsqlTimeout() ;

         void              _checkTaskTimeout( const BSONObj &task ) ;

         void              _createVersionFile() ;


      // Msg functions
      protected:

      private:

         MAP_ID2HOSTPTR                         _mapID2Host ;
         MAP_HOST2ID                            _mapHost2ID ;
         MsgRouteID                             _hwRouteID ;

         ossSpinSLatch                          _omLatch ;
         ossEvent                               _attachEvent ;

         pmdRemoteSessionMgr                    _rsManager ;

         omMsgHandler                           _msgHandler ;
         omTimerHandler                         _timerHandler ;
         netRouteAgent                          _netAgent ;
         MsgRouteID                             _myNodeID ;

         pmdKRCB*                               _pKrcb ;
         SDB_DMSCB*                             _pDmsCB ;
         SDB_RTNCB*                             _pRtnCB ;

         string                                 _wwwRootPath ;

         string                                 _localAgentPort ;
         omHostVersion                          *_hostVersion ;

         omTaskManager                          *_taskManager ;

         UINT64                                 _ssqlCheckTimer ;

         BOOLEAN                                _isInitTable ;
   } ;

   typedef _omManager omManager ;
   /*
      get the global om manager object point
   */
   omManager *sdbGetOMManager() ;

}

#endif // OM_MANAGER_HPP__

