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

   Descriptive Name = Runtime Coord Common

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
#include "aggrBuilder.hpp"

using namespace bson ;

namespace engine
{
   //default command-processer
   #define COORD_CMD_DEFAULT                  "COORD_CMD_DEFAULT"
   #define COORD_CMD_BACKUP_OFFLINE           CMD_ADMIN_PREFIX CMD_NAME_BACKUP_OFFLINE
   #define COORD_CMD_LIST_BACKUPS             CMD_ADMIN_PREFIX CMD_NAME_LIST_BACKUPS
   #define COORD_CMD_REMOVE_BACKUP            CMD_ADMIN_PREFIX CMD_NAME_REMOVE_BACKUP
   #define COORD_CMD_LISTGROUPS               CMD_ADMIN_PREFIX CMD_NAME_LIST_GROUPS
   #define COORD_CMD_LISTCOLLECTIONSPACES     CMD_ADMIN_PREFIX CMD_NAME_LIST_COLLECTIONSPACES
   #define COORD_CMD_LISTCOLLECTIONS          CMD_ADMIN_PREFIX CMD_NAME_LIST_COLLECTIONS
   #define COORD_CMD_LISTUSERS                CMD_ADMIN_PREFIX CMD_NAME_LIST_USERS
   #define COORD_CMD_CREATECOLLECTIONSPACE    CMD_ADMIN_PREFIX CMD_NAME_CREATE_COLLECTIONSPACE
   #define COORD_CMD_CREATECOLLECTION         CMD_ADMIN_PREFIX CMD_NAME_CREATE_COLLECTION
   #define COORD_CMD_ALTERCOLLECTION          CMD_ADMIN_PREFIX CMD_NAME_ALTER_COLLECTION
   #define COORD_CMD_DROPCOLLECTION           CMD_ADMIN_PREFIX CMD_NAME_DROP_COLLECTION
   #define COORD_CMD_DROPCOLLECTIONSPACE      CMD_ADMIN_PREFIX CMD_NAME_DROP_COLLECTIONSPACE
   #define COORD_CMD_SNAPSHOTCONTEXTS         CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_CONTEXTS
   #define COORD_CMD_SNAPSHOTCONTEXTSCUR      CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_CONTEXTS_CURRENT
   #define COORD_CMD_SNAPSHOTSESSIONS         CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_SESSIONS
   #define COORD_CMD_SNAPSHOTSESSIONSCUR      CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_SESSIONS_CURRENT
   #define COORD_CMD_SNAPSHOTDATABASE         CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_DATABASE
   #define COORD_CMD_SNAPSHOTSYSTEM           CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_SYSTEM
   #define COORD_CMD_SNAPSHOTRESET            CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_RESET
   #define COORD_CMD_SNAPSHOTCOLLECTIONS      CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_COLLECTIONS
   #define COORD_CMD_SNAPSHOTCOLLECTIONSPACES CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_COLLECTIONSPACES
   #define COORD_CMD_SNAPSHOTCATALOG          CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_CATA
   #define COORD_CMD_SNAPSHOTTRANSCUR         CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_TRANSACTIONS_CUR
   #define COORD_CMD_SNAPSHOTTRANS            CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_TRANSACTIONS
   #define COORD_CMD_TESTCOLLECTIONSPACE      CMD_ADMIN_PREFIX CMD_NAME_TEST_COLLECTIONSPACE
   #define COORD_CMD_TESTCOLLECTION           CMD_ADMIN_PREFIX CMD_NAME_TEST_COLLECTION
   #define COORD_CMD_CREATEGROUP              CMD_ADMIN_PREFIX CMD_NAME_CREATE_GROUP
   #define COORD_CMD_REMOVEGROUP              CMD_ADMIN_PREFIX CMD_NAME_REMOVE_GROUP
   #define COORD_CMD_CREATENODE               CMD_ADMIN_PREFIX CMD_NAME_CREATE_NODE
   #define COORD_CMD_REMOVENODE               CMD_ADMIN_PREFIX CMD_NAME_REMOVE_NODE
   #define COORD_CMD_UPDATENODE               CMD_ADMIN_PREFIX CMD_NAME_UPDATE_NODE
   #define COORD_CMD_ACTIVEGROUP              CMD_ADMIN_PREFIX CMD_NAME_ACTIVE_GROUP
   #define COORD_CMD_CREATEINDEX              CMD_ADMIN_PREFIX CMD_NAME_CREATE_INDEX
   #define COORD_CMD_DROPINDEX                CMD_ADMIN_PREFIX CMD_NAME_DROP_INDEX
   #define COORD_CMD_STARTUPNODE              CMD_ADMIN_PREFIX CMD_NAME_STARTUP_NODE
   #define COORD_CMD_SHUTDOWNNODE             CMD_ADMIN_PREFIX CMD_NAME_SHUTDOWN_NODE
   #define COORD_CMD_SHUTDOWNGROUP            CMD_ADMIN_PREFIX CMD_NAME_SHUTDOWN_GROUP
   #define COORD_CMD_SPLIT                    CMD_ADMIN_PREFIX CMD_NAME_SPLIT
   #define COORD_CMD_WAITTASK                 CMD_ADMIN_PREFIX CMD_NAME_WAITTASK
   #define COORD_CMD_GETCOUNT                 CMD_ADMIN_PREFIX CMD_NAME_GET_COUNT
   #define COORD_CMD_GETINDEXES               CMD_ADMIN_PREFIX CMD_NAME_GET_INDEXES
   #define COORD_CMD_GETDATABLOCKS            CMD_ADMIN_PREFIX CMD_NAME_GET_DATABLOCKS
   #define COORD_CMD_GETQUERYMETA             CMD_ADMIN_PREFIX CMD_NAME_GET_QUERYMETA
   #define COORD_CMD_GETDCINFO                CMD_ADMIN_PREFIX CMD_NAME_GET_DCINFO
   #define COORD_CMD_CREATECATAGROUP          CMD_ADMIN_PREFIX CMD_NAME_CREATE_CATA_GROUP
   #define COORD_CMD_TRACESTART               CMD_ADMIN_PREFIX CMD_NAME_TRACE_START
   #define COORD_CMD_TRACERESUME              CMD_ADMIN_PREFIX CMD_NAME_TRACE_RESUME
   #define COORD_CMD_TRACESTOP                CMD_ADMIN_PREFIX CMD_NAME_TRACE_STOP
   #define COORD_CMD_TRACESTATUS              CMD_ADMIN_PREFIX CMD_NAME_TRACE_STATUS
   #define COORD_CMD_EXPCONFIG                CMD_ADMIN_PREFIX CMD_NAME_EXPORT_CONFIG
   #define COORD_CMD_SNAPSHOTDBINTR           CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_DATABASE_INTR
   #define COORD_CMD_SNAPSHOTSYSINTR          CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_SYSTEM_INTR
   #define COORD_CMD_SNAPSHOTCLINTR           CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_COLLECTION_INTR
   #define COORD_CMD_SNAPSHOTCSINTR           CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_SPACE_INTR
   #define COORD_CMD_SNAPSHOTCTXINTR          CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_CONTEX_INTR
   #define COORD_CMD_SNAPSHOTCTXCURINTR       CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_CONTEXCUR_INTR
   #define COORD_CMD_SNAPSHOTSESSINTR         CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_SESSION_INTR
   #define COORD_CMD_SNAPSHOTSESSCURINTR      CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_SESSIONCUR_INTR
   #define COORD_CMD_SNAPSHOTCATAINTR         CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_CATA_INTR
   #define COORD_CMD_CRT_PROCEDURE            CMD_ADMIN_PREFIX CMD_NAME_CRT_PROCEDURE
   #define COORD_CMD_EVAL                     CMD_ADMIN_PREFIX CMD_NAME_EVAL
   #define COORD_CMD_RM_PROCEDURE             CMD_ADMIN_PREFIX CMD_NAME_RM_PROCEDURE
   #define COORD_CMD_LIST_PROCEDURES          CMD_ADMIN_PREFIX CMD_NAME_LIST_PROCEDURES
   #define COORD_CMD_LINK                     CMD_ADMIN_PREFIX CMD_NAME_LINK_CL
   #define COORD_CMD_UNLINK                   CMD_ADMIN_PREFIX CMD_NAME_UNLINK_CL
   #define COORD_CMD_LIST_TASKS               CMD_ADMIN_PREFIX CMD_NAME_LIST_TASKS
   #define COORD_CMD_CANCEL_TASK              CMD_ADMIN_PREFIX CMD_NAME_CANCEL_TASK
   #define COORD_CMD_SET_SESS_ATTR            CMD_ADMIN_PREFIX CMD_NAME_SETSESS_ATTR
   #define COORD_CMD_CREATE_DOMAIN            CMD_ADMIN_PREFIX CMD_NAME_CREATE_DOMAIN
   #define COORD_CMD_DROP_DOMAIN              CMD_ADMIN_PREFIX CMD_NAME_DROP_DOMAIN
   #define COORD_CMD_ALTER_DOMAIN             CMD_ADMIN_PREFIX CMD_NAME_ALTER_DOMAIN
   #define COORD_CMD_ADD_DOMAIN_GROUP         CMD_ADMIN_PREFIX CMD_NAME_ADD_DOMAIN_GROUP
   #define COORD_CMD_REMOVE_DOMAIN_GROUP      CMD_ADMIN_PREFIX CMD_NAME_REMOVE_DOMAIN_GROUP
   #define COORD_CMD_LIST_DOMAINS             CMD_ADMIN_PREFIX CMD_NAME_LIST_DOMAINS
   #define COORD_CMD_LIST_CS_IN_DOMAIN        CMD_ADMIN_PREFIX CMD_NAME_LIST_CS_IN_DOMAIN
   #define COORD_CMD_LIST_CL_IN_DOMAIN        CMD_ADMIN_PREFIX CMD_NAME_LIST_CL_IN_DOMAIN
   #define COORD_CMD_INVALIDATE_CACHE         CMD_ADMIN_PREFIX CMD_NAME_INVALIDATE_CACHE
   #define COORD_CMD_LIST_LOBS                CMD_ADMIN_PREFIX CMD_NAME_LIST_LOBS
   #define COORD_CMD_ALTER_DC                 CMD_ADMIN_PREFIX CMD_NAME_ALTER_DC
   #define COORD_CMD_REELECT                  CMD_ADMIN_PREFIX CMD_NAME_REELECT
   #define COORD_CMD_TRUNCATE                 CMD_ADMIN_PREFIX CMD_NAME_TRUNCATE
   #define COORD_CMD_SYNC_DB                  CMD_ADMIN_PREFIX CMD_NAME_SYNC_DB

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
                                 rtnContextCoord **ppContext = NULL ) ;

      INT32         executeOnDataGroup ( MsgHeader *pMsg,
                                         pmdEDUCB *cb,
                                         const CoordGroupList &groupLst,
                                         BOOLEAN onPrimary = TRUE,
                                         SET_RC *pIgnoreRC = NULL,
                                         CoordGroupList *pSucGrpLst = NULL,
                                         rtnContextCoord **ppContext = NULL ) ;

      INT32         executeOnCataGroup ( MsgHeader *pMsg,
                                         pmdEDUCB *cb,
                                         BOOLEAN onPrimary = TRUE,
                                         SET_RC *pIgnoreRC = NULL,
                                         rtnContextCoord **ppContext = NULL ) ;

      INT32         executeOnCataGroup ( MsgHeader *pMsg,
                                         pmdEDUCB *cb,
                                         CoordGroupList *pGroupList,
                                         vector<BSONObj> *pReplyObjs = NULL,
                                         BOOLEAN onPrimary = TRUE,
                                         SET_RC *pIgnoreRC = NULL ) ;

      INT32         executeOnCataCL( MsgOpQuery *pMsg,
                                     pmdEDUCB *cb,
                                     const CHAR *pCLName,
                                     BOOLEAN onPrimary = TRUE,
                                     SET_RC *pIgnoreRC = NULL,
                                     rtnContextCoord **ppContext = NULL ) ;

      INT32         queryOnCatalog( MsgHeader *pMsg,
                                    INT32 requestType,
                                    pmdEDUCB *cb,
                                    INT64 &contextID,
                                    rtnContextBuf *buf ) ;

      INT32         queryOnCatalog( const rtnQueryOptions &options,
                                    pmdEDUCB *cb,
                                    SINT64 &contextID ) ;

      INT32         queryOnCataAndPushToVec( const rtnQueryOptions &options,
                                             pmdEDUCB *cb,
                                             vector<BSONObj> &objs ) ;

      INT32         executeOnNodes( MsgHeader *pMsg,
                                    pmdEDUCB *cb,
                                    ROUTE_SET &nodes,
                                    ROUTE_RC_MAP &faileds,
                                    rtnCoordCtrlParam &ctrlParam,
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

   private:
      // don't define any members that will change while execute
      // because this obj will be shared for different threads

      INT32 _processCatReply( const BSONObj &obj,
                              CoordGroupList &groupLst ) ;

      INT32 _processSucReply( ROUTE_REPLY_MAP &okReply,
                              rtnContextCoord *pContext ) ;

      INT32 _processNodesReply( REPLY_QUE &replyQue,
                                ROUTE_RC_MAP &faileds,
                                ROUTE_SET &retriedNodes,
                                ROUTE_SET &needRetryNodes,
                                rtnCoordCtrlParam &ctrlParam,
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
                               rtnContextCoord **ppContext = NULL ) ;

      BOOLEAN _getRetryNodes( ROUTE_SET &retriedNodes,
                              ROUTE_SET &needRetryNodes,
                              rtnCoordCtrlParam &ctrlParam,
                              MsgOpReply *pReply ) ;

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
      virtual BOOLEAN         _allowFailed () = 0 ;
      virtual BOOLEAN         _useContext () = 0 ;
      virtual UINT32          _getMask() const = 0 ;

   } ;

   class rtnCoordListBackup : public rtnCoordBackupBase
   {
   protected:
      virtual FILTER_BSON_ID  _getGroupMatherIndex () ;
      virtual NODE_SEL_STY    _nodeSelWhenNoFilter () ;
      virtual BOOLEAN         _allowFailed () ;
      virtual BOOLEAN         _useContext () ;
      virtual UINT32          _getMask() const ;
   } ;

   class rtnCoordRemoveBackup : public rtnCoordBackupBase
   {
   protected:
      virtual FILTER_BSON_ID  _getGroupMatherIndex () ;
      virtual NODE_SEL_STY    _nodeSelWhenNoFilter () ;
      virtual BOOLEAN         _allowFailed () ;
      virtual BOOLEAN         _useContext () ;
      virtual UINT32          _getMask() const ;
   } ;

   class rtnCoordBackupOffline : public rtnCoordBackupBase
   {
   protected:
      virtual FILTER_BSON_ID  _getGroupMatherIndex () ;
      virtual NODE_SEL_STY    _nodeSelWhenNoFilter () ;
      virtual BOOLEAN         _allowFailed () ;
      virtual BOOLEAN         _useContext () ;
      virtual UINT32          _getMask() const ;
   } ;

   class rtnCoordCMDListGroups : public rtnCoordCommand
   {
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;
   };

   class rtnCoordCMDSnapshotIntrBase : public rtnCoordCommand
   {
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;

   private:
      virtual BOOLEAN _useContext() { return TRUE ; }

   };

   class rtnCoordCmdSnapshotReset : public rtnCoordCMDSnapshotIntrBase
   {
   private:
      virtual BOOLEAN _useContext() { return FALSE ; }
   } ;

   class rtnCoordAggrCmdBase : public _aggrCmdBase
   {
   public:
      INT32 appendObjs( const CHAR *pInputBuffer,
                        CHAR *&pOutputBuffer,
                        INT32 &bufferSize,
                        INT32 &bufUsed,
                        INT32 &buffObjNum ) ;
   } ;

   class rtnCoordCMDSnapShotBase : public rtnCoordCommand, public rtnCoordAggrCmdBase
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

   class rtnCoordCMDSnapshotDataBase: public rtnCoordCMDSnapShotBase
   {
   private:
      virtual const CHAR *getIntrCMDName() ;
      virtual const CHAR *getInnerAggrContent() ;
   };

   class rtnCoordCMDSnapshotSystem: public rtnCoordCMDSnapShotBase
   {
   private:
      virtual const CHAR *getIntrCMDName() ;
      virtual const CHAR *getInnerAggrContent() ;
   };

   class rtnCoordCMDSnapshotCollections: public rtnCoordCMDSnapShotBase
   {
   private:
      virtual const CHAR *getIntrCMDName() ;
      virtual const CHAR *getInnerAggrContent() ;
   };

   class rtnCoordCMDSnapshotSpaces: public rtnCoordCMDSnapShotBase
   {
   private:
      virtual const CHAR *getIntrCMDName() ;
      virtual const CHAR *getInnerAggrContent() ;
   };

   class rtnCoordCMDSnapshotContexts: public rtnCoordCMDSnapShotBase
   {
   private:
      virtual const CHAR *getIntrCMDName() ;
      virtual const CHAR *getInnerAggrContent() ;
   };

   class rtnCoordCMDSnapshotContextsCur: public rtnCoordCMDSnapShotBase
   {
   private:
      virtual const CHAR *getIntrCMDName() ;
      virtual const CHAR *getInnerAggrContent() ;
   };

   class rtnCoordCMDSnapshotSessions: public rtnCoordCMDSnapShotBase
   {
   private:
      virtual const CHAR *getIntrCMDName() ;
      virtual const CHAR *getInnerAggrContent() ;
   };

   class rtnCoordCMDSnapshotSessionsCur: public rtnCoordCMDSnapShotBase
   {
   private:
      virtual const CHAR *getIntrCMDName() ;
      virtual const CHAR *getInnerAggrContent() ;
   };

   class rtnCoordCMDSnapshotCollectionsTmp : public rtnCoordCommand
   {
   public :
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;
   };

   class rtnCoordCMDSnapshotCollectionSpacesTmp : public rtnCoordCommand
   {
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;
   };

   class rtnCoordCMDCreateCollectionSpace : public rtnCoordCommand
   {
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;
   } ;

   class rtnCoordCMDCreateCollection : public rtnCoordCommand
   {
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;

   private:
      INT32 _notifyDataGroupsToStartTask( const CHAR *pCLName,
                                          const BSONElement &task,
                                          pmdEDUCB *cb ) ;
   };

   class _rtnAlterJob ;
   class rtnCoordCMDAlterCollection : public rtnCoordCommand
   {
   public :
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;
   private:
      INT32 _executeOld( MsgHeader *pMsg,
                         pmdEDUCB *cb,
                         INT64 &contextID,
                         rtnContextBuf *buf,
                         string &clName ) ;

      INT32 _execute( MsgHeader *pMsg,
                      pmdEDUCB *cb,
                      INT64 &contextID,
                      rtnContextBuf *buf,
                      string &clName ) ;

      INT32 _testCollection( const CHAR *fullName, pmdEDUCB *cb ) ;
   } ;

   class rtnCoordCMD2PhaseCommit : public rtnCoordCommand
   {
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;
   protected:
      virtual void  _getIgnoreRCList( SET_RC &ignoreRCList ) ;
      virtual INT32 _getGroupList( CHAR *pReceiveBuffer,
                                   pmdEDUCB *cb,
                                   CoordGroupList &groupLst,
                                   string &strName ) = 0 ;

   private:
      virtual INT32 doP1OnDataGroup( CHAR *pReceiveBuffer,
                                     pmdEDUCB *cb,
                                     SET_RC &ignoreRCList,
                                     SINT64 &contextID,
                                     string &strName ) ;

      virtual INT32 doP2OnDataGroup( CHAR *pReceiveBuffer,
                                     pmdEDUCB * cb,
                                     const string &strName,
                                     SINT64 &contextID ) ;

      virtual INT32 doOnCataGroup( CHAR *pReceiveBuffer,
                                   pmdEDUCB * cb,
                                   const string &strName ) = 0;

      virtual INT32 complete( CHAR *pReceiveBuffer,
                              pmdEDUCB * cb,
                              const string &strName );
   };

   class rtnCoordCMDDropCollection : public rtnCoordCMD2PhaseCommit
   {
   protected:
      virtual INT32 _getGroupList( CHAR *pReceiveBuffer,
                                   pmdEDUCB *cb,
                                   CoordGroupList &groupLst,
                                   string &strName ) ;

      virtual void  _getIgnoreRCList( SET_RC &ignoreRCList ) ;

   private:
      virtual INT32 doOnCataGroup( CHAR *pReceiveBuffer,
                                   pmdEDUCB * cb,
                                   const string &strName );

      virtual INT32 complete( CHAR *pReceiveBuffer,
                              pmdEDUCB * cb,
                              const string &strName );

      INT32 _getCLName( CHAR *pReceiveBuffer,
                        string &strCLName ) ;
   };

   class rtnCoordCMDDropCollectionSpace : public rtnCoordCMD2PhaseCommit
   {
   protected:
      virtual INT32 _getGroupList( CHAR *pReceiveBuffer,
                                   pmdEDUCB *cb,
                                   CoordGroupList &groupLst,
                                   string &strName ) ;

      virtual void  _getIgnoreRCList( SET_RC &ignoreRCList ) ;

   private:
      virtual INT32 doOnCataGroup( CHAR *pReceiveBuffer,
                                   pmdEDUCB * cb,
                                   const string &strName ) ;

      virtual INT32 complete( CHAR *pReceiveBuffer,
                              pmdEDUCB * cb,
                              const string &strName ) ;

   };

   class rtnCoordCMDQueryBase : public rtnCoordCommand
   {
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;

   protected:
      virtual INT32 _preProcess( rtnQueryOptions &queryOpt,
                                 string &clName ) = 0 ;
   };

   class rtnCoordCMDSnapshotCata : public rtnCoordCMDQueryBase
   {
   protected:
      virtual INT32 _preProcess( rtnQueryOptions &queryOpt,
                                 string &clName ) ;
   };

   class rtnCoordCMDListCollectionSpace : public rtnCoordCMDQueryBase
   {
   protected:
      virtual INT32 _preProcess( rtnQueryOptions &queryOpt,
                                 string &clName ) ;
   };

   class rtnCoordCMDListCollection : public rtnCoordCMDQueryBase
   {
   protected:
      virtual INT32 _preProcess( rtnQueryOptions &queryOpt,
                                 string &clName ) ;
   };

   class rtnCoordCMDListUser : public rtnCoordCMDQueryBase
   {
   protected:
      virtual INT32 _preProcess( rtnQueryOptions &queryOpt,
                                 string &clName ) ;
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

   class rtnCoordCMDCreateGroup : public rtnCoordCommand
   {
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;
   };

   class rtnCoordCMDRemoveGroup : public rtnCoordCommand
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

   class rtnCoordCMDCreateNode : public rtnCoordCommand,
                                 public rtnCoordCMDConfigNode
   {
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;
   private:
      INT32 _createNode( MsgHeader *pMsg,
                         pmdEDUCB *cb,
                         INT64 &contextID,
                         rtnContextBuf *buf ) ;

      INT32 _attachNode( const BSONObj &obj,
                         pmdEDUCB *cb ) ;

      INT32 _buildAttachMsg( const BSONObj &conf,
                             const CHAR *groupName,
                             const CHAR *host,
                             CHAR *&buf,
                             MsgOpQuery *&header ) ;
   };

   class rtnCoordCMDRemoveNode : public rtnCoordCommand,
                                 public rtnCoordCMDConfigNode
   {
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;

   private:
      INT32 _validateOnDataNode( const BSONElement &groupName,
                                 const BSONElement &host,
                                 const BSONElement &srv,
                                 pmdEDUCB *cb ) ;
   } ;

   class rtnCoordCMDUpdateNode : public rtnCoordCommand,
                                 public rtnCoordCMDConfigNode
   {
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;
   };


   class rtnCoordCMDActiveGroup : public rtnCoordCommand
   {
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;
   private:
      INT32 startNodes( BSONObj &boGroupInfo,
                        vector<BSONObj> &objList ) ;

      INT32 startNodes( clsGroupItem *pItem,
                        vector<BSONObj> &objList ) ;

   };

   class rtnCoordCMDCreateIndex : public rtnCoordCommand
   {
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;

   protected:
      INT32         checkIndexKey( const CoordCataInfoPtr &cataInfo,
                                   const BSONObj &indexObj,
                                   set< UINT32 > &haveSet,
                                   pmdEDUCB *cb ) ;

   };

   class rtnCoordCMDDropIndex : public rtnCoordCommand
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

   class rtnCoordCMDOperateOnGroup : public rtnCoordCommand
   {
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;

      INT32 opOnGroup( BSONObj &boGroupInfo ) ;

      virtual SINT32 getOpType() = 0 ;
   };

   class rtnCoordCMDShutdownGroup : public rtnCoordCMDOperateOnGroup
   {
   public:
      virtual SINT32 getOpType();
   };

   class rtnCoordCMDSplit : public rtnCoordCommand
   {
   public :
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;

      INT32         getCLCount( const CHAR *clFullName,
                                CoordGroupList &groupList,
                                pmdEDUCB *cb, UINT64 &count ) ;

   protected:
      INT32 _getBoundByPercent( const CHAR *cl,
                                FLOAT64 percent,
                                CoordCataInfoPtr &cataInfo,
                                CoordGroupList &groupList,
                                pmdEDUCB *cb,
                                BSONObj &lowBound,
                                BSONObj &upBound ) ;

      INT32 _getBoundByCondition( const CHAR *cl,
                                  const BSONObj &begin,
                                  const BSONObj &end,
                                  CoordGroupList &groupList,
                                  pmdEDUCB *cb,
                                  CoordCataInfoPtr &cataInfo,
                                  BSONObj &lowBound,
                                  BSONObj &upBound ) ;

   private:
      INT32 _getBoundRecordOnData( const CHAR *cl,
                                   const BSONObj &condition,
                                   const BSONObj &hint,
                                   const BSONObj &sort,
                                   INT32 flag,
                                   INT64 skip,
                                   CoordGroupList &groupList,
                                   pmdEDUCB *cb,
                                   BSONObj &shardingKey,
                                   BSONObj &record ) ;

   } ;

   class rtnCoordCmdWaitTask : public rtnCoordCommand
   {
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;
   } ;

   class rtnCoordCmdListTask : public rtnCoordCMDQueryBase
   {
   protected:
      virtual INT32 _preProcess( rtnQueryOptions &queryOpt,
                                 string &clName ) ;
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
      typedef std::map< std::string, bson::BSONObj > CoordIndexMap;
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

   class rtnCoordCMDCreateCataGroup : public rtnCoordCommand
   {
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;
   private:
      INT32 getNodeConf( CHAR *pQuery, bson::BSONObj &boNodeConfig );
      INT32 getNodeInfo( CHAR *pQuery, bson::BSONObj &boNodeInfo );
   };

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

   class rtnCoordCMDExpConfig : public rtnCoordCommand
   {
   public :
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;
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

   class rtnCoordCMDListProcedures : public rtnCoordCMDQueryBase
   {
   protected:
      virtual INT32 _preProcess( rtnQueryOptions &queryOpt,
                                 string &clName ) ;
   } ;

   class rtnCoordCMDLinkCollection : public rtnCoordCommand
   {
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;
   };

   class rtnCoordCMDUnlinkCollection : public rtnCoordCommand
   {
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;
   };

   class rtnCoordCMDSetSessionAttr : public rtnCoordCommand
   {
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;
   };

   class rtnCoordCMDCreateDomain : public rtnCoordCommand
   {
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;
   } ;

   class rtnCoordCMDDropDomain : public rtnCoordCommand
   {
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;
   } ;

   class rtnCoordCMDAlterDomain : public rtnCoordCommand
   {
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;
   } ;

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

   class rtnCoordCMDListDomains : public rtnCoordCMDQueryBase
   {
   protected:
      virtual INT32 _preProcess( rtnQueryOptions &queryOpt,
                                 string &clName ) ;
   } ;

   class rtnCoordCMDListCSInDomain : public rtnCoordCMDQueryBase
   {
   protected:
      virtual INT32 _preProcess( rtnQueryOptions &queryOpt,
                                 string &clName ) ;
   } ;


   class rtnCoordCMDListCLInDomain : public rtnCoordCommand
   {
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;

   private:
      INT32 _rebuildListResult( const std::vector<BSONObj> &infoFromCata,
                                pmdEDUCB *cb,
                                SINT64 &contextID ) ;
   } ;

   class rtnCoordCMDInvalidateCache : public rtnCoordCommand
   {
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;
   } ;

   class rtnCoordCMDListLobs : public rtnCoordCommand
   {
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;
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

   class rtnCoordCMDSyncDB : public rtnCoordCommand
   {
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;
   private:
      INT32 _syncDB( MsgHeader *pMsg, pmdEDUCB *cb, SINT64 &contextID ) ;
   } ;

   class rtnCoordCMDQueryOnMain : public rtnCoordCMDSnapshotIntrBase
   {
   public :
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf );

      virtual INT32 getGroups( pmdEDUCB *cb, CoordGroupList &groupList ) = 0 ;
   } ;

   class rtnCoordSnapshotTransCur : public rtnCoordCMDQueryOnMain
   {
   public:
      virtual INT32 getGroups( pmdEDUCB *cb, CoordGroupList &groupList ) ;
   } ;

   class rtnCoordSnapshotTrans : public rtnCoordCMDQueryOnMain
   {
   public:
      virtual INT32 getGroups( pmdEDUCB *cb, CoordGroupList &groupList ) ;
   } ;

}
#endif
