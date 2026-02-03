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

   Source File Name = catDCManager.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =     XJH Opt

*******************************************************************************/
#ifndef CAT_DCMANAGER_HPP__
#define CAT_DCMANAGER_HPP__

#include "pmd.hpp"
#include "netDef.hpp"
#include "catEventHandler.hpp"
#include "rtnContextBuff.hpp"
#include "dpsLogDef.hpp"

using namespace bson ;

namespace engine
{
   class sdbCatalogueCB ;
   class _SDB_RTNCB ;
   class _dpsLogWrapper ;
   class _SDB_DMSCB ;
   class _clsDCMgr ;
   class _clsDCBaseInfo ;

   /*
      _catDCManager define
   */
   class _catDCManager : public SDBObject, public _catEventHandler
   {
   public:
      _catDCManager() ;
      ~_catDCManager() ;

      INT32 init() ;
      INT32 fini() ;

      void  attachCB( _pmdEDUCB *cb ) ;
      void  detachCB( _pmdEDUCB *cb ) ;

      INT32 processMsg( const NET_HANDLE &handle, MsgHeader *pMsg ) ;

      INT32 active() ;
      INT32 deactive() ;

      INT32 updateGlobalAddr() ;

      _clsDCBaseInfo *getDCInfo() { return _pDCBaseInfo ; }

      BOOLEAN isDCActivated() const ;
      BOOLEAN isDCReadonly() const ;

      void    setWritedCommand( BOOLEAN writed ) { _isWritedCmd = writed ; }
      BOOLEAN isWritedCommand() const { return _isWritedCmd ; }

      INT32 updateDCCache()
      {
         return _mapData2DCMgr( _pDCMgr ) ;
      }

      INT32 getCATVersion( UINT32 &version ) ;
      INT32 setCATVersion( UINT32 version ) ;

   public :
      // functions of _catEventHandler
      virtual const CHAR *getHandlerName () { return "catDCManager" ; }
      virtual INT32 onBeginCommand ( MsgHeader *pReqMsg ) ;

   // message process functions
   protected:
      INT32 processCommandMsg( const NET_HANDLE &handle, MsgHeader *pMsg,
                               BOOLEAN writable ) ;
      INT32 processCmdAlterImage( const NET_HANDLE &handle,
                                  const CHAR *pQuery,
                                  rtnContextBuf &ctxBuff ) ;

   // inner process
   protected:
      INT32 processCmdActivate( const NET_HANDLE &handle,
                                _clsDCMgr *pDCMgr,
                                const BSONObj &objQuery,
                                BSONObjBuilder &retObjBuilder ) ;
      INT32 processCmdDeactivate( const NET_HANDLE &handle,
                                  _clsDCMgr *pDCMgr,
                                  const BSONObj &objQuery,
                                  BSONObjBuilder &retObjBuilder ) ;
      INT32 processCmdEnableReadonly( const NET_HANDLE &handle,
                                      _clsDCMgr *pDCMgr,
                                      const BSONObj &objQuery,
                                      BSONObjBuilder &retObjBuilder ) ;
      INT32 processCmdDisableReadonly( const NET_HANDLE &handle,
                                       _clsDCMgr *pDCMgr,
                                       const BSONObj &objQuery,
                                       BSONObjBuilder &retObjBuilder ) ;
      INT32 processCmdSetActiveLocation( const NET_HANDLE &handle,
                                         _clsDCMgr *pDCMgr,
                                         const BSONObj &objQuery,
                                         BSONObjBuilder &retObjBuilder ) ;
      INT32 processCmdSetLocation( const NET_HANDLE &handle,
                                   _clsDCMgr *pDCMgr,
                                   const BSONObj &objQuery,
                                   BSONObjBuilder &retObjBuilder ) ;
      INT32 processCmdAlterMaintenanceMode( const NET_HANDLE &handle,
                                            _clsDCMgr *pDCMgr,
                                            const BSONObj &objQuery,
                                            BSONObjBuilder &retObjBuilder,
                                            const BOOLEAN &isStartMode ) ;

   protected:
      INT32 _mapData2DCMgr( _clsDCMgr *pDCMgr ) ;

   // tool fuctions
   private:
      INT16 _majoritySize() ;
      INT32 _updateGlobalInfo() ;

      INT32 _checkMaintenanceMode( const BSONObj &option,
                                   const BSONObj &groupObj,
                                   const UINT32 &groupID,
                                   const BOOLEAN &isStartMode,
                                   clsGroupMode &groupMode,
                                   ossPoolString *pHostName ) ;

      INT32 _buildGroupModeInfo( const BSONObj &groupObj,
                                 const ossPoolString &hostName,
                                 clsGroupMode &groupMode ) ;

   private:
      _SDB_DMSCB                 *_pDmsCB;
      _dpsLogWrapper             *_pDpsCB;
      _SDB_RTNCB                 *_pRtnCB;
      sdbCatalogueCB             *_pCatCB;
      pmdEDUCB                   *_pEduCB;

      _clsDCMgr                  *_pDCMgr ;
      _clsDCBaseInfo             *_pDCBaseInfo ;

      // for commands
      BOOLEAN                    _isWritedCmd ;
      BOOLEAN                    _isActived ;

   } ;
   typedef _catDCManager catDCManager ;

}

#endif // CAT_DCMANAGER_HPP__

