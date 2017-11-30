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

   Source File Name = rtnCoordNodeCommands.hpp

   Descriptive Name = Runtime Coord Commands for Node Management

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

#ifndef RTNCOORDNODECOMMANDS_HPP__
#define RTNCOORDNODECOMMANDS_HPP__

#include "rtnCoordOperator.hpp"
#include "rtnCoordQuery.hpp"
#include "msgDef.hpp"
#include "rtnQueryOptions.hpp"
#include "rtnCoordCommands.hpp"

using namespace bson ;

namespace engine
{
   /*
    * rtnCoordNodeCMD2Phase define
    */
   class rtnCoordNodeCMD2Phase : public rtnCoordCMD2Phase
   {
   protected :
      virtual INT32 _generateDataMsg ( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       _rtnCMDArguments *pArgs,
                                       const vector<BSONObj> &cataObjs,
                                       CHAR **ppMsgBuf,
                                       MsgHeader **ppDataMsg ) ;

      virtual INT32 _generateRollbackDataMsg ( MsgHeader *pMsg,
                                               _rtnCMDArguments *pArgs,
                                               CHAR **ppMsgBuf,
                                               MsgHeader **ppRollbackMsg ) ;

      virtual INT32 _doAudit ( _rtnCMDArguments *pArgs, INT32 rc ) ;
   } ;

   /*
    * rtnCoordNodeCMD3Phase define
    */
   class rtnCoordNodeCMD3Phase : public rtnCoordNodeCMD2Phase
   {
   protected :
      virtual INT32 _doOnCataGroupP2 ( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       rtnContextCoord **ppContext,
                                       _rtnCMDArguments *pArgs,
                                       const CoordGroupList &pGroupLst ) ;
   } ;

   /*
    * rtnCoordCMDOpOnNodes define
    */
   class rtnCoordCMDOpOnNodes
   {
   protected :
      rtnCoordCMDOpOnNodes () {}

      virtual ~rtnCoordCMDOpOnNodes () {}

      INT32 _opOnOneNode ( const vector<INT32> &opList,
                           string hostName,
                           string svcName,
                           vector<BSONObj> &dataObjs ) ;

      INT32 _opOnNodes ( const vector<INT32> &opList,
                         const BSONObj &boGroupInfo,
                         vector<BSONObj> &dataObjs ) ;

      INT32 _opOnCataNodes ( const vector<INT32> &opList,
                             clsGroupItem *pItem,
                             vector<BSONObj> &dataObjs ) ;

      void _logErrorForNodes ( const string &commandName,
                               const string &targetName,
                               const vector<BSONObj> &dataObjs ) ;

      virtual BOOLEAN _flagIsStartNodes () { return FALSE ; }
   } ;

   /*
      rtnCoordCMDCreateCataGroup define
   */
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

   /*
    * rtnCoordCMDCreateGroup define
    */
   class rtnCoordCMDCreateGroup : public rtnCoordCommand
   {
   public :
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;
   } ;

   /*
    * rtnCoordCMDActiveGroup define
    */
   class rtnCoordCMDActiveGroup : public rtnCoordNodeCMD2Phase,
                                  public rtnCoordCMDOpOnNodes
   {
   protected :
      class _rtnCMDActiveGroupArgs : public _rtnCMDArguments
      {
      public :
         _rtnCMDActiveGroupArgs () {}

         virtual ~_rtnCMDActiveGroupArgs () {}

         /* cata group info cache */
         CoordGroupInfoPtr _catGroupInfo ;
      } ;

   protected :
      virtual _rtnCMDArguments *_generateArguments () ;

      virtual INT32 _parseMsg ( MsgHeader *pMsg,
                                _rtnCMDArguments *pArgs ) ;

      virtual INT32 _generateCataMsg ( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       _rtnCMDArguments *pArgs,
                                       CHAR **ppMsgBuf,
                                       MsgHeader **ppCataMsg ) ;

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
                                     CoordGroupList &sucGroupLst ) ;

      virtual const CHAR *_getCommandName () const
      { return CMD_NAME_ACTIVE_GROUP ; }

      virtual BOOLEAN _flagIsStartNodes () { return TRUE ; }
   } ;

   /*
    * rtnCoordCMDShutdownGroup define
    */
   class rtnCoordCMDShutdownGroup : public rtnCoordNodeCMD2Phase,
                                    public rtnCoordCMDOpOnNodes
   {
   protected :
      virtual INT32 _parseMsg ( MsgHeader *pMsg,
                                _rtnCMDArguments *pArgs ) ;

      virtual INT32 _generateCataMsg ( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       _rtnCMDArguments *pArgs,
                                       CHAR **ppMsgBuf,
                                       MsgHeader **ppCataMsg ) ;

      virtual INT32 _doOnDataGroup ( MsgHeader *pMsg,
                                     pmdEDUCB *cb,
                                     rtnContextCoord **ppContext,
                                     _rtnCMDArguments *pArgs,
                                     const CoordGroupList &groupLst,
                                     const vector<BSONObj> &cataObjs,
                                     CoordGroupList &sucGroupLst ) ;

      virtual INT32 _doCommit ( MsgHeader *pMsg,
                                pmdEDUCB * cb,
                                rtnContextCoord **ppContext,
                                _rtnCMDArguments *pArgs ) ;

      virtual const CHAR *_getCommandName () const
      { return CMD_NAME_SHUTDOWN_GROUP ; }

      virtual INT32 _doAudit ( _rtnCMDArguments *pArgs, INT32 rc ) ;
   } ;

   /*
    * rtnCoordCMDRemoveGroup define
    */
   class rtnCoordCMDRemoveGroup : public rtnCoordNodeCMD3Phase,
                                  public rtnCoordCMDOpOnNodes
   {
   protected :
      virtual INT32 _parseMsg ( MsgHeader *pMsg,
                                _rtnCMDArguments *pArgs ) ;

      virtual INT32 _generateCataMsg ( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       _rtnCMDArguments *pArgs,
                                       CHAR **ppMsgBuf,
                                       MsgHeader **ppCataMsg ) ;

      virtual INT32 _doOnDataGroup ( MsgHeader *pMsg,
                                     pmdEDUCB *cb,
                                     rtnContextCoord **ppContext,
                                     _rtnCMDArguments *pArgs,
                                     const CoordGroupList &groupLst,
                                     const vector<BSONObj> &cataObjs,
                                     CoordGroupList &sucGroupLst ) ;

      virtual INT32 _doOnDataGroupP2 ( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       rtnContextCoord **ppContext,
                                       _rtnCMDArguments *pArgs,
                                       const CoordGroupList &groupLst,
                                       const vector<BSONObj> &cataObjs ) ;

      virtual INT32 _doCommit ( MsgHeader *pMsg,
                                pmdEDUCB * cb,
                                rtnContextCoord **ppContext,
                                _rtnCMDArguments *pArgs ) ;

      virtual const CHAR *_getCommandName () const
      { return CMD_NAME_REMOVE_GROUP ; }
   } ;

   /*
    * rtnCoordCMDCreateNode define
    */
   class rtnCoordCMDCreateNode : public rtnCoordNodeCMD3Phase
   {
   protected :
      class _rtnCMDCreateNodeArgs : public _rtnCMDArguments
      {
      public :
         _rtnCMDCreateNodeArgs ()
         : _rtnCMDArguments()
         {
            _onlyAttach = FALSE ;
            _keepData = FALSE ;
         }

         virtual ~_rtnCMDCreateNodeArgs () {}

         /* Hostname of the target node */
         string _hostName ;

         /* Whether to attach node only */
         BOOLEAN _onlyAttach ;

         /* Whether to keep data after remove node */
         BOOLEAN _keepData ;

         /* cata group info cache */
         CoordGroupInfoPtr _catGroupInfo ;
      } ;

   protected :
      virtual _rtnCMDArguments *_generateArguments () ;

      virtual INT32 _parseMsg ( MsgHeader *pMsg,
                                _rtnCMDArguments *pArgs ) ;

      virtual INT32 _generateCataMsg ( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       _rtnCMDArguments *pArgs,
                                       CHAR **ppMsgBuf,
                                       MsgHeader **ppCataMsg ) ;

      virtual INT32 _doOnDataGroup ( MsgHeader *pMsg,
                                     pmdEDUCB *cb,
                                     rtnContextCoord **ppContext,
                                     _rtnCMDArguments *pArgs,
                                     const CoordGroupList &groupLst,
                                     const vector<BSONObj> &cataObjs,
                                     CoordGroupList &sucGroupLst ) ;

      virtual INT32 _doOnDataGroupP2 ( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       rtnContextCoord **ppContext,
                                       _rtnCMDArguments *pArgs,
                                       const CoordGroupList &groupLst,
                                       const vector<BSONObj> &cataObjs ) ;

      virtual INT32 _doComplete ( MsgHeader *pMsg,
                                  pmdEDUCB * cb,
                                  _rtnCMDArguments *pArgs ) ;

      virtual const CHAR *_getCommandName () const
      { return CMD_NAME_CREATE_NODE ; }

      INT32 _getNodeConf ( const BSONObj &boQuery,
                                  BSONObj &nodeConf,
                                  const CoordGroupInfoPtr &catGroupInfo ) ;

      INT32 _checkNodeInfo ( const BSONObj &boQuery ) ;
   } ;

   /*
    * rtnCoordCMDRemoveNode define
    */
   class rtnCoordCMDRemoveNode : public rtnCoordNodeCMD3Phase
   {
   protected :
      class _rtnCMDRemoveNodeArgs : public _rtnCMDArguments
      {
      public :
         _rtnCMDRemoveNodeArgs ()
         : _rtnCMDArguments()
         {
            _onlyDetach = FALSE ;
            _keepData = FALSE ;
            _force = FALSE ;
         }

         virtual ~_rtnCMDRemoveNodeArgs () {}

         /* Hostname of the target node */
         string _hostName ;

         /* Name of service of the target node */
         string _serviceName ;

         /* Whether to detach node only */
         BOOLEAN _onlyDetach ;

         /* Whether to keep data after remove node */
         BOOLEAN _keepData ;

         /* Whether to force remove node and ignore cm */
         BOOLEAN _force ;
      } ;

   protected :
      virtual _rtnCMDArguments *_generateArguments () ;

      virtual INT32 _parseMsg ( MsgHeader *pMsg,
                                _rtnCMDArguments *pArgs ) ;

      virtual INT32 _generateCataMsg ( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       _rtnCMDArguments *pArgs,
                                       CHAR **ppMsgBuf,
                                       MsgHeader **ppCataMsg ) ;

      virtual INT32 _doOnDataGroup ( MsgHeader *pMsg,
                                     pmdEDUCB *cb,
                                     rtnContextCoord **ppContext,
                                     _rtnCMDArguments *pArgs,
                                     const CoordGroupList &groupLst,
                                     const vector<BSONObj> &cataObjs,
                                     CoordGroupList &sucGroupLst ) ;

      virtual INT32 _doOnDataGroupP2 ( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       rtnContextCoord **ppContext,
                                       _rtnCMDArguments *pArgs,
                                       const CoordGroupList &groupLst,
                                       const vector<BSONObj> &cataObjs ) ;

      virtual INT32 _doComplete ( MsgHeader *pMsg,
                                  pmdEDUCB * cb,
                                  _rtnCMDArguments *pArgs ) ;

      virtual const CHAR *_getCommandName () const
      { return CMD_NAME_REMOVE_NODE ; }
   } ;
}

#endif
