/*******************************************************************************

   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = rtnCoordCommands.hpp

   Descriptive Name = Runtime Coord Commands

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/

#ifndef RTNCOORDCOMMANDS_HPP__
#define RTNCOORDCOMMANDS_HPP__

#include "rtnCoordOperator.hpp"
#include "rtnCoordQuery.hpp"
#include "msgDef.hpp"
#include "rtnQueryOptions.hpp"
#include "rtnCoordCommandDef.hpp"
#include "aggrBuilder.hpp"
#include "utilMap.hpp"

using namespace bson ;

namespace engine
{

   class rtnCoordCommand : virtual public rtnCoordOperator
   {
   public:
      rtnCoordCommand(){};
      virtual ~rtnCoordCommand(){};

      virtual INT32        execute( MsgHeader *pMsg,
                                    pmdEDUCB *cb,
                                    INT64 &contextID,
                                    rtnContextBuf *buf ) { return SDB_SYS ; }

   public:
      INT32         executeOnCL( MsgHeader *pMsg,
                                 pmdEDUCB *cb,
                                 const CHAR *pCLName,
                                 BOOLEAN firstUpdateCata = FALSE,
                                 const CoordGroupList *pSpecGrpLst = NULL,
                                 SET_RC *pIgnoreRC = NULL,
                                 CoordGroupList *pSucGrpLst = NULL,
                                 rtnContextCoord **ppContext = NULL,
                                 rtnContextBuf *buf = NULL ) ;

      INT32         executeOnDataGroup ( MsgHeader *pMsg,
                                         pmdEDUCB *cb,
                                         const CoordGroupList &groupLst,
                                         BOOLEAN onPrimary = TRUE,
                                         SET_RC *pIgnoreRC = NULL,
                                         CoordGroupList *pSucGrpLst = NULL,
                                         rtnContextCoord **ppContext = NULL,
                                         rtnContextBuf *buf = NULL ) ;

      INT32         executeOnCataGroup ( MsgHeader *pMsg,
                                         pmdEDUCB *cb,
                                         BOOLEAN onPrimary = TRUE,
                                         SET_RC *pIgnoreRC = NULL,
                                         rtnContextCoord **ppContext = NULL,
                                         rtnContextBuf *buf = NULL ) ;

      INT32         executeOnCataGroup ( MsgHeader *pMsg,
                                         pmdEDUCB *cb,
                                         CoordGroupList *pGroupList,
                                         vector<BSONObj> *pReplyObjs = NULL,
                                         BOOLEAN onPrimary = TRUE,
                                         SET_RC *pIgnoreRC = NULL,
                                         rtnContextCoord **ppContext = NULL,
                                         rtnContextBuf *buf = NULL ) ;

      INT32         executeOnCataCL( MsgOpQuery *pMsg,
                                     pmdEDUCB *cb,
                                     const CHAR *pCLName,
                                     BOOLEAN onPrimary = TRUE,
                                     SET_RC *pIgnoreRC = NULL,
                                     rtnContextCoord **ppContext = NULL,
                                     rtnContextBuf *buf = NULL ) ;

      INT32         queryOnCatalog( MsgHeader *pMsg,
                                    INT32 requestType,
                                    pmdEDUCB *cb,
                                    INT64 &contextID,
                                    rtnContextBuf *buf ) ;

      INT32         queryOnCatalog( const rtnQueryOptions &options,
                                    pmdEDUCB *cb,
                                    SINT64 &contextID,
                                    rtnContextBuf *buf ) ;

      INT32         queryOnCataAndPushToVec( const rtnQueryOptions &options,
                                             pmdEDUCB *cb,
                                             vector<BSONObj> &objs,
                                             rtnContextBuf *buf ) ;

      INT32         executeOnNodes( MsgHeader *pMsg,
                                    pmdEDUCB *cb,
                                    ROUTE_SET &nodes,
                                    ROUTE_RC_MAP &faileds,
                                    ROUTE_SET *pSucNodes = NULL,
                                    SET_RC *pIgnoreRC = NULL,
                                    rtnContextCoord *pContext = NULL ) ;

      INT32         executeOnNodes( MsgHeader *pMsg,
                                    pmdEDUCB *cb,
                                    rtnCoordCtrlParam &ctrlParam,
                                    UINT32 mask,
                                    ROUTE_RC_MAP &faileds,
                                    rtnContextCoord **ppContext = NULL,
                                    BOOLEAN openEmptyContext = FALSE,
                                    SET_RC *pIgnoreRC = NULL,
                                    ROUTE_SET *pSucNodes = NULL ) ;

   protected:
      virtual void _printDebug ( CHAR *pReceiveBuffer, const CHAR *pFuncName ) ;

      /* Enable preRead in Coord context (send GetMore in advanced) */
      virtual BOOLEAN _flagCoordCtxPreRead () { return TRUE ; }

   private:
      // don't define any members that will change while execute
      // because this obj will be shared for different threads

      INT32 _processCatReply( const BSONObj &obj,
                              CoordGroupList &groupLst ) ;

      INT32 _processSucReply( ROUTE_REPLY_MAP &okReply,
                              rtnContextCoord *pContext ) ;

      INT32 _processNodesReply( REPLY_QUE &replyQue,
                                ROUTE_RC_MAP &faileds,
                                rtnContextCoord *pContext = NULL,
                                SET_RC *pIgnoreRC = NULL,
                                ROUTE_SET *pSucNodes = NULL ) ;

      INT32 _buildFailedNodeReply( ROUTE_RC_MAP &failedNodes,
                                   rtnContextCoord *pContext ) ;

      INT32 _executeOnGroups ( MsgHeader *pMsg,
                               pmdEDUCB *cb,
                               const CoordGroupList &groupLst,
                               MSG_ROUTE_SERVICE_TYPE type,
                               BOOLEAN onPrimary = TRUE,
                               SET_RC *pIgnoreRC = NULL,
                               CoordGroupList *pSucGrpLst = NULL,
                               rtnContextCoord **ppContext = NULL,
                               rtnContextBuf *buf = NULL ) ;

   };

   class rtnCoordDefaultCommand : public rtnCoordCommand
   {
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;
   };

   class rtnCoordBackupBase : public rtnCoordCommand
   {
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;

   protected:
      virtual FILTER_BSON_ID  _getGroupMatherIndex () = 0 ;
      virtual NODE_SEL_STY    _nodeSelWhenNoFilter () = 0 ;
      virtual BOOLEAN         _useContext () = 0 ;
      virtual UINT32          _getMask() const = 0 ;

   } ;

   class rtnCoordRemoveBackup : public rtnCoordBackupBase
   {
   protected:
      virtual FILTER_BSON_ID  _getGroupMatherIndex () ;
      virtual NODE_SEL_STY    _nodeSelWhenNoFilter () ;
      virtual BOOLEAN         _useContext () ;
      virtual UINT32          _getMask() const ;
   } ;

   class rtnCoordBackupOffline : public rtnCoordBackupBase
   {
   protected:
      virtual FILTER_BSON_ID  _getGroupMatherIndex () ;
      virtual NODE_SEL_STY    _nodeSelWhenNoFilter () ;
      virtual BOOLEAN         _useContext () ;
      virtual UINT32          _getMask() const ;
   } ;

   /*
      rtnCoordCmdWithLocation define
   */
   class rtnCoordCmdWithLocation : public rtnCoordCommand
   {
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;

   private:
      virtual BOOLEAN _useContext() = 0 ;
      virtual INT32   _onLocalMode( INT32 flag ) = 0 ;
      virtual void    _preSet( pmdEDUCB *cb, rtnCoordCtrlParam &ctrlParam ) = 0 ;
      virtual UINT32  _getControlMask() const = 0 ;

      virtual INT32   _preExcute( MsgHeader *pMsg,
                                  pmdEDUCB *cb,
                                  rtnCoordCtrlParam &ctrlParam ) ;
      virtual INT32   _posExcute( MsgHeader *pMsg,
                                  pmdEDUCB *cb,
                                  ROUTE_RC_MAP &faileds ) ;

   } ;

   /*
      rtnCoordCMDMonIntrBase define
   */
   class rtnCoordCMDMonIntrBase : public rtnCoordCmdWithLocation
   {
   private:
      virtual void    _preSet( pmdEDUCB *cb, rtnCoordCtrlParam &ctrlParam ) ;
      virtual INT32   _onLocalMode( INT32 flag ) ;

   } ;

   /*
      rtnCoordCMDMonCurIntrBase define
   */
   class rtnCoordCMDMonCurIntrBase : public rtnCoordCMDMonIntrBase
   {
   private:
      virtual void    _preSet( pmdEDUCB *cb, rtnCoordCtrlParam &ctrlParam ) ;
   } ;

   /*
      rtnCoordAggrCmdBase define
   */
   class rtnCoordAggrCmdBase : public _aggrCmdBase
   {
   public:
      INT32 appendObjs( const CHAR *pInputBuffer,
                        CHAR *&pOutputBuffer,
                        INT32 &bufferSize,
                        INT32 &bufUsed,
                        INT32 &buffObjNum ) ;
   } ;

   /*
      rtnCoordCMDMonBase define
   */
   class rtnCoordCMDMonBase : public rtnCoordCommand, public rtnCoordAggrCmdBase
   {
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;
   private:
      virtual const CHAR *getIntrCMDName() = 0 ;
      virtual const CHAR *getInnerAggrContent() = 0 ;
      virtual BOOLEAN    _useContext() { return TRUE ; }
   };

   /*
      rtnCoordCMDQueryBase define
   */
   class rtnCoordCMDQueryBase : public rtnCoordCommand
   {
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;

   protected:
      virtual INT32 _preProcess( rtnQueryOptions &queryOpt,
                                 string &clName,
                                 BSONObj &outSelector ) = 0 ;
   };

   class rtnCoordCMDTestCollectionSpace : public rtnCoordCommand
   {
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;
   };

   class rtnCoordCMDTestCollection : public rtnCoordCommand
   {
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;
   };

   class rtnCoordCMDConfigNode
   {
   public:
      INT32 getNodeInfo( char *pQuery, bson::BSONObj &NodeInfo );
      INT32 getNodeConf( char *pQuery, bson::BSONObj &nodeConf,
                         CoordGroupInfoPtr &catGroupInfo );
   };

   class rtnCoordCMDUpdateNode : public rtnCoordCommand,
                                 public rtnCoordCMDConfigNode
   {
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;
   };

   class rtnCoordCMDOperateOnNode : public rtnCoordCommand
   {
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;
      virtual SINT32 getOpType()=0;
   };

   class rtnCoordCMDStartupNode : public rtnCoordCMDOperateOnNode
   {
   public:
      virtual SINT32 getOpType();
   };

   class rtnCoordCMDShutdownNode : public rtnCoordCMDOperateOnNode
   {
   public:
      virtual SINT32 getOpType();
   };

   class rtnCoordCmdWaitTask : public rtnCoordCommand
   {
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;
   } ;

   class rtnCoordCmdCancelTask : public rtnCoordCommand
   {
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;
   } ;

   class rtnCoordCMDStatisticsBase : virtual public rtnCoordCommand
   {
   public :
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;
   private:
      virtual INT32 generateResult( rtnContext *pContext,
                                    netMultiRouteAgent *pRouteAgent,
                                    pmdEDUCB *cb ) = 0 ;

      virtual BOOLEAN openEmptyContext() const { return FALSE ; }
   } ;

   class rtnCoordCMDGetIndexes : public rtnCoordCMDStatisticsBase
   {
      typedef _utilMap< std::string, bson::BSONObj, 10 >    CoordIndexMap ;

   private :
      virtual INT32 generateResult( rtnContext *pContext,
                                    netMultiRouteAgent *pRouteAgent,
                                    pmdEDUCB *cb ) ;
   } ;
   class rtnCoordCMDGetCount : public rtnCoordCMDStatisticsBase
   {
   private :
      virtual INT32 generateResult( rtnContext *pContext,
                                    netMultiRouteAgent *pRouteAgent,
                                    pmdEDUCB *cb );
      virtual BOOLEAN openEmptyContext() const { return TRUE ; }
   };
   class rtnCoordCMDGetDatablocks : public rtnCoordCMDStatisticsBase
   {
   private :
      virtual INT32 generateResult( rtnContext *pContext,
                                    netMultiRouteAgent *pRouteAgent,
                                    pmdEDUCB *cb ) ;
   } ;

   class rtnCoordCMDGetQueryMeta : public rtnCoordCMDGetDatablocks
   {
   } ;

   class rtnCoordCMDTraceStart : public rtnCoordCommand
   {
   public :
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;
   } ;

   class rtnCoordCMDTraceResume : public rtnCoordCommand
   {
   public :
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;
   } ;

   class rtnCoordCMDTraceStop : public rtnCoordCommand
   {
   public :
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;
   } ;

   class rtnCoordCMDTraceStatus : public rtnCoordCommand
   {
   public :
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;
   } ;

   class rtnCoordCMDExpConfig : public rtnCoordCmdWithLocation
   {
   private:
      virtual BOOLEAN _useContext() { return FALSE ; }
      virtual INT32   _onLocalMode( INT32 flag ) ;
      virtual void    _preSet( pmdEDUCB *cb, rtnCoordCtrlParam &ctrlParam ) ;
      virtual UINT32  _getControlMask() const ;

      virtual INT32   _posExcute( MsgHeader *pMsg,
                                  pmdEDUCB *cb,
                                  ROUTE_RC_MAP &faileds ) ;
   } ;

   class rtnCoordCMDCrtProcedure : public rtnCoordCommand
   {
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;
   } ;

   class rtnCoordCMDEval : public rtnCoordCommand
   {
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;
   private:
      INT32 _buildContext( _spdSession *session,
                           pmdEDUCB *cb,
                           SINT64 &contextID ) ;
   } ;

   class rtnCoordCMDRmProcedure : public rtnCoordCommand
   {
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;
   } ;

   class rtnCoordCMDSetSessionAttr : public rtnCoordCommand
   {
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;
   };

   class rtnCoordCMDAddDomainGroup : public rtnCoordCommand
   {
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;
   } ;

   class rtnCoordCMDRemoveDomainGroup : public rtnCoordCommand
   {
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;
   } ;

   class rtnCoordCMDInvalidateCache : public rtnCoordCmdWithLocation
   {
   private:
      virtual BOOLEAN _useContext() { return FALSE ; }
      virtual INT32   _onLocalMode( INT32 flag ) { return SDB_OK ; }
      virtual void    _preSet( pmdEDUCB *cb, rtnCoordCtrlParam &ctrlParam ) ;
      virtual UINT32  _getControlMask() const ;

      virtual INT32   _preExcute( MsgHeader *pMsg,
                                  pmdEDUCB *cb,
                                  rtnCoordCtrlParam &ctrlParam ) ;
   } ;

   class rtnCoordCMDReelection : public rtnCoordCommand
   {
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;
   } ;

   class rtnCoordCMDTruncate : public rtnCoordCommand
   {
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;
   } ;

   class rtnCoordCMDSyncDB : public rtnCoordCmdWithLocation
   {
   private:
      virtual BOOLEAN _useContext() { return FALSE ; }
      virtual INT32   _onLocalMode( INT32 flag ) { return flag ; }
      virtual void    _preSet( pmdEDUCB *cb, rtnCoordCtrlParam &ctrlParam ) ;
      virtual UINT32  _getControlMask() const ;

      virtual INT32   _preExcute( MsgHeader *pMsg,
                                  pmdEDUCB *cb,
                                  rtnCoordCtrlParam &ctrlParam ) ;
   } ;

   class rtnCoordCmdLoadCS : public rtnCoordCmdWithLocation
   {
   private:
      virtual BOOLEAN _useContext() { return FALSE ; }
      virtual INT32   _onLocalMode( INT32 flag ) { return flag ; }
      virtual void    _preSet( pmdEDUCB *cb, rtnCoordCtrlParam &ctrlParam ) ;
      virtual UINT32  _getControlMask() const ;

      virtual INT32   _preExcute( MsgHeader *pMsg,
                                  pmdEDUCB *cb,
                                  rtnCoordCtrlParam &ctrlParam ) ;
   } ;

   typedef rtnCoordCmdLoadCS rtnCoordCmdUnloadCS ;

   class rtnCoordForceSession: public rtnCoordCmdWithLocation
   {
   private:
      virtual BOOLEAN _useContext() { return FALSE ; }
      virtual INT32   _onLocalMode( INT32 flag ) ;
      virtual void    _preSet( pmdEDUCB *cb, rtnCoordCtrlParam &ctrlParam ) ;
      virtual UINT32  _getControlMask() const ;
   } ;

   class rtnCoordSetPDLevel : public rtnCoordCmdWithLocation
   {
   private:
      virtual BOOLEAN _useContext() { return FALSE ; }
      virtual INT32   _onLocalMode( INT32 flag ) ;
      virtual void    _preSet( pmdEDUCB *cb, rtnCoordCtrlParam &ctrlParam ) ;
      virtual UINT32  _getControlMask() const ;
   } ;

   class rtnCoordReloadConf : public rtnCoordCmdWithLocation
   {
   private:
      virtual BOOLEAN _useContext() { return FALSE ; }
      virtual INT32   _onLocalMode( INT32 flag ) ;
      virtual void    _preSet( pmdEDUCB *cb, rtnCoordCtrlParam &ctrlParam ) ;
      virtual UINT32  _getControlMask() const ;
   } ;

   /*
    * rtnCoordCMD2Phase define
    */
   class rtnCoordCMD2Phase : public rtnCoordCommand
   {
   protected :
      /* Dynamic arguments for commands */
      class _rtnCMDArguments : public SDBObject
      {
      public :
         _rtnCMDArguments () { _pBuf = NULL ; }

         virtual ~_rtnCMDArguments () {}

         /* A copy of the query object */
         BSONObj _boQuery ;

         /* Name of the catalog target to be updated */
         string _targetName ;

         /* ignore error return codes */
         SET_RC _ignoreRCList ;

         /* the return context buf pointer */
         rtnContextBuf *_pBuf ;
      } ;

   public:
      virtual INT32 execute ( MsgHeader *pMsg,
                              pmdEDUCB *cb,
                              INT64 &contextID,
                              rtnContextBuf *buf ) ;
   protected :
      virtual _rtnCMDArguments *_generateArguments () ;

      virtual INT32 _extractMsg ( MsgHeader *pMsg, _rtnCMDArguments *pArgs ) ;

      virtual INT32 _parseMsg ( MsgHeader *pMsg,
                                _rtnCMDArguments *pArgs ) = 0 ;

      virtual INT32 _generateCataMsg ( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       _rtnCMDArguments *pArgs,
                                       CHAR **ppMsgBuf,
                                       MsgHeader **ppCataMsg ) = 0;

      virtual INT32 _generateDataMsg ( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       _rtnCMDArguments *pArgs,
                                       const vector<BSONObj> &cataObjs,
                                       CHAR **ppMsgBuf,
                                       MsgHeader **ppDataMsg ) = 0 ;

      virtual INT32 _generateRollbackDataMsg ( MsgHeader *pMsg,
                                               _rtnCMDArguments *pArgs,
                                               CHAR **ppMsgBuf,
                                               MsgHeader **ppRollbackMsg ) = 0 ;

      virtual const CHAR *_getCommandName () const = 0;

   protected :
      virtual INT32 _doOnCataGroup ( MsgHeader *pMsg,
                                     pmdEDUCB *cb,
                                     rtnContextCoord **ppContext,
                                     _rtnCMDArguments *pArgs,
                                     CoordGroupList *pGroupLst,
                                     vector<BSONObj> *pReplyObjs ) ;

      virtual INT32 _doOnDataGroup ( MsgHeader *pMsg,
                                     pmdEDUCB *cb,
                                     rtnContextCoord **ppContext,
                                     _rtnCMDArguments *pArgs,
                                     const CoordGroupList &groupLst,
                                     const vector<BSONObj> &cataObjs,
                                     CoordGroupList &sucGroupLst ) = 0;

      virtual INT32 _doOnCataGroupP2 ( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       rtnContextCoord **ppContext,
                                       _rtnCMDArguments *pArgs,
                                       const CoordGroupList &pGroupLst ) ;

      virtual INT32 _doOnDataGroupP2 ( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       rtnContextCoord **ppContext,
                                       _rtnCMDArguments *pArgs,
                                       const CoordGroupList &groupLst,
                                       const vector<BSONObj> &cataObjs ) ;

      virtual INT32 _rollbackOnDataGroup ( MsgHeader *pMsg,
                                           pmdEDUCB *cb,
                                           _rtnCMDArguments *pArgs,
                                           const CoordGroupList &groupLst ) ;

      virtual INT32 _doCommit ( MsgHeader *pMsg,
                                pmdEDUCB * cb,
                                rtnContextCoord **ppContext,
                                _rtnCMDArguments *pArgs ) ;

      virtual INT32 _doComplete ( MsgHeader *pMsg,
                                  pmdEDUCB * cb,
                                  _rtnCMDArguments *pArgs ) ;

      virtual INT32 _doAudit ( _rtnCMDArguments *pArgs, INT32 rc ) = 0 ;

      virtual INT32 _processContext ( pmdEDUCB *cb,
                                      rtnContextCoord **ppContext,
                                      SINT32 maxNumSteps ) ;

   protected :
      /* Disable preRead in Coord context (send GetMore in advanced) to control
       * the sub-context from Catalog or Data step by step */
      virtual BOOLEAN _flagCoordCtxPreRead () { return FALSE ; }

      /* Send command to Data Groups with the TID of client */
      virtual BOOLEAN _flagReserveClientTID () { return FALSE ; }

      /* Get list of Data Groups from Catalog with the P1 catalog command */
      virtual BOOLEAN _flagGetGrpLstFromCata () { return FALSE ; }

      /* Commit on Catalog when rollback on Data groups failed */
      virtual BOOLEAN _flagCommitOnRollbackFailed () { return FALSE ; }

      /* Rollback on Catalog before rollback on Data groups */
      virtual BOOLEAN _flagRollbackCataBeforeData () { return FALSE ; }
   } ;


}
#endif
