/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

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
#include "catDCLogMgr.hpp"
#include "catEventHandler.hpp"
#include "rtnContextBuff.hpp"
#include "dpsLogDef.hpp"
#include "dpsMessageBlock.hpp"

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

      catDCLogMgr* getLogMgr() { return _pLogMgr ; }

      INT32 init() ;
      INT32 fini() ;

      void  attachCB( _pmdEDUCB *cb ) ;
      void  detachCB( _pmdEDUCB *cb ) ;

      INT32 processMsg( const NET_HANDLE &handle, MsgHeader *pMsg ) ;

      INT32 active() ;
      INT32 deactive() ;

      INT32 updateGlobalAddr() ;

      BOOLEAN isDCActivated() const ;
      BOOLEAN isDCReadonly() const ;
      BOOLEAN isImageEnabled() const ;
      BOOLEAN groupInImage( UINT32 groupID ) ;
      BOOLEAN groupInImage( const string &groupName ) ;

      void    setWritedCommand( BOOLEAN writed ) { _isWritedCmd = writed ; }
      BOOLEAN isWritedCommand() const { return _isWritedCmd ; }

   public :
      // functions of _catEventHandler
      virtual const CHAR *getHandlerName () { return "catDCManager" ; }
      virtual INT32 onBeginCommand ( MsgHeader *pReqMsg ) ;
      virtual INT32 onEndCommand ( MsgHeader *pReqMsg, INT32 result ) ;
      virtual INT32 onSendReply ( MsgOpReply *pReply, INT32 result ) ;

   // message process functions
   protected:
      INT32 processCommandMsg( const NET_HANDLE &handle, MsgHeader *pMsg,
                               BOOLEAN writable ) ;
      INT32 processCmdAlterImage( const NET_HANDLE &handle,
                                  const CHAR *pQuery,
                                  rtnContextBuf &ctxBuff ) ;

   // inner process
   protected:
      INT32 processCmdCreateImage( const NET_HANDLE &handle,
                                   _clsDCMgr *pDCMgr,
                                   const BSONObj &objQuery,
                                   BSONObjBuilder &retObjBuilder ) ;
      INT32 processCmdRemoveImage( const NET_HANDLE &handle,
                                   _clsDCMgr *pDCMgr,
                                   const BSONObj &objQuery,
                                   BSONObjBuilder &retObjBuilder ) ;
      INT32 processCmdAttachImage( const NET_HANDLE &handle,
                                   _clsDCMgr *pDCMgr,
                                   const BSONObj &objQuery,
                                   BSONObjBuilder &retObjBuilder ) ;
      INT32 processCmdEnableImage( const NET_HANDLE &handle,
                                   _clsDCMgr *pDCMgr,
                                   const BSONObj &objQuery,
                                   BSONObjBuilder &retObjBuilder ) ;
      INT32 processCmdDetachImage( const NET_HANDLE &handle,
                                   _clsDCMgr *pDCMgr,
                                   const BSONObj &objQuery,
                                   BSONObjBuilder &retObjBuilder ) ;
      INT32 processCmdDisableImage( const NET_HANDLE &handle,
                                    _clsDCMgr *pDCMgr,
                                    const BSONObj &objQuery,
                                    BSONObjBuilder &retObjBuilder) ;
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

   protected:
      void  _fillRspHeader( MsgHeader *rspMsg, const MsgHeader *reqMsg ) ;

      INT32 _mapData2DCMgr( _clsDCMgr *pDCMgr ) ;

      void  _dcBaseInfoGroups2Obj( _clsDCBaseInfo *pInfo,
                                   BSONObjBuilder &builder,
                                   const CHAR *pFieldName ) ;

      BOOLEAN _isAddrConflict( const string &addr,
                               const vector< pmdAddrPair > &dstAddr ) ;

      INT32   _checkGroupsValid( map< string, string > &mapGroups,
                                 nodeMgrAgent *pNodeAgent ) ;

   // tool fuctions
   private:
      INT16 _majoritySize() ;
      INT32 _updateGlobalInfo() ;
      INT32 _updateImageInfo() ;

   private:
      _SDB_DMSCB                 *_pDmsCB;
      _dpsLogWrapper             *_pDpsCB;
      _SDB_RTNCB                 *_pRtnCB;
      sdbCatalogueCB             *_pCatCB;
      pmdEDUCB                   *_pEduCB;

      _clsDCMgr                  *_pDCMgr ;
      _clsDCBaseInfo             *_pDCBaseInfo ;
      _catDCLogMgr               *_pLogMgr ;

      // for commands
      BOOLEAN                    _isWritedCmd ;
      DPS_LSN                    _lsn ;
      _dpsMessageBlock           _mb ;
      BOOLEAN                    _isActived ;

   } ;
   typedef _catDCManager catDCManager ;

}

#endif // CAT_DCMANAGER_HPP__

