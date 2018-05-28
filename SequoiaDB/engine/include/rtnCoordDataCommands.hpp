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

   Source File Name = rtnCoordDataCommands.hpp

   Descriptive Name = Runtime Coord Commands for Data Manager

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

#ifndef RTNCOORDDATACOMMANDS_HPP__
#define RTNCOORDDATACOMMANDS_HPP__

#include "rtnCoordOperator.hpp"
#include "rtnCoordQuery.hpp"
#include "msgDef.hpp"
#include "rtnQueryOptions.hpp"
#include "aggrBuilder.hpp"
#include "rtnCoordCommands.hpp"

using namespace bson ;

namespace engine
{
   /*
    * rtnCoordDataCMD2Phase define
    */
   class rtnCoordDataCMD2Phase : public rtnCoordCMD2Phase
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

      virtual INT32 _doAudit ( _rtnCMDArguments *pArgs, INT32 rc ) ;

   protected :
      /* Get list of Data Groups from Catalog with the P1 catalog command */
      virtual BOOLEAN _flagGetGrpLstFromCata () { return TRUE ; }

      /* command on collection */
      virtual BOOLEAN _flagDoOnCollection () { return TRUE ; }

      /* update catalog info before send command to Catalog */
      virtual BOOLEAN _flagUpdateBeforeCata () { return FALSE ; }

      /* update catalog info before send command to Data Groups */
      virtual BOOLEAN _flagUpdateBeforeData () { return FALSE ; }

      /* whether to use group in Coord cache */
      virtual BOOLEAN _flagUseGrpLstInCoord () { return FALSE ; }
   } ;

   /*
    * rtnCoordDataCMD3Phase define
    */
   class rtnCoordDataCMD3Phase : public rtnCoordDataCMD2Phase
   {
   protected :
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
   } ;

   /*
    * rtnCoordCMDCreateDomain define
    */
   class rtnCoordCMDCreateDomain : public rtnCoordCommand
   {
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;
   } ;

   /*
    * rtnCoordCMDDropDomain define
    */
   class rtnCoordCMDDropDomain : public rtnCoordCommand
   {
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;
   } ;

   /*
    * rntCoordCMDAlterDomain define
    */
   class rtnCoordCMDAlterDomain : public rtnCoordCommand
   {
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;
   } ;

   /*
    * rtnCoordCMDCreateCollectionSpace define
    */
   class rtnCoordCMDCreateCollectionSpace : public rtnCoordCommand
   {
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;
   } ;

   /*
    * rtnCoordCMDDropCollectionSpace define
    */
   class rtnCoordCMDDropCollectionSpace : public rtnCoordDataCMD3Phase
   {
   protected :
      virtual INT32 _parseMsg ( MsgHeader *pMsg,
                                _rtnCMDArguments *pArgs ) ;

      virtual INT32 _generateCataMsg ( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       _rtnCMDArguments *pArgs,
                                       CHAR **ppMsgBuf,
                                       MsgHeader **ppCataMsg ) ;

      virtual const CHAR *_getCommandName () const
      { return CMD_NAME_DROP_COLLECTIONSPACE ; }

      virtual INT32 _doComplete ( MsgHeader *pMsg,
                                  pmdEDUCB * cb,
                                  _rtnCMDArguments *pArgs ) ;

   protected :
      /* Send command to Data Groups with the TID of client */
      virtual BOOLEAN _flagReserveClientTID () { return TRUE ; }

      /* command on collection */
      virtual BOOLEAN _flagDoOnCollection () { return FALSE ; }
   } ;

   /*
    * rtnCoordCMDCreateCollection define
    */
   class rtnCoordCMDCreateCollection : public rtnCoordDataCMD2Phase
   {
   protected :
      virtual INT32 _parseMsg ( MsgHeader *pMsg,
                                _rtnCMDArguments *pArgs ) ;

      virtual INT32 _generateCataMsg ( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       _rtnCMDArguments *pArgs,
                                       CHAR **ppMsgBuf,
                                       MsgHeader **ppCataMsg ) ;

      virtual INT32 _generateRollbackDataMsg ( MsgHeader *pMsg,
                                               _rtnCMDArguments *pArgs,
                                               CHAR **ppMsgBuf,
                                               MsgHeader **ppRollbackMsg ) ;

      virtual INT32 _rollbackOnDataGroup ( MsgHeader *pMsg,
                                           pmdEDUCB *cb,
                                           _rtnCMDArguments *pArgs,
                                           const CoordGroupList &groupLst ) ;

      virtual const CHAR *_getCommandName () const
      { return CMD_NAME_CREATE_COLLECTION ; }

   protected :
      /* Commit on Catalog when rollback on Data groups failed */
      virtual BOOLEAN _flagCommitOnRollbackFailed () { return TRUE ; }
   };

   /*
    * rtnCoordCMDDropCollection define
    */
   class rtnCoordCMDDropCollection : public rtnCoordDataCMD3Phase
   {
   protected :
      virtual INT32 _parseMsg ( MsgHeader *pMsg,
                                _rtnCMDArguments *pArgs ) ;

      virtual INT32 _generateCataMsg ( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       _rtnCMDArguments *pArgs,
                                       CHAR **ppMsgBuf,
                                       MsgHeader **ppCataMsg ) ;

      virtual INT32 _generateDataMsg ( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       _rtnCMDArguments *pArgs,
                                       const vector<BSONObj> &cataObjs,
                                       CHAR **ppMsgBuf,
                                       MsgHeader **ppDataMsg ) ;

      virtual const CHAR *_getCommandName () const
      { return CMD_NAME_DROP_COLLECTION ; }

      virtual INT32 _doComplete ( MsgHeader *pMsg,
                                  pmdEDUCB * cb,
                                  _rtnCMDArguments *pArgs ) ;

      /* Send command to Data Groups with the TID of client */
      virtual BOOLEAN _flagReserveClientTID () { return TRUE ; }

      /* use group in Coord cache, since we only have short-term locks in Catalog */
      virtual BOOLEAN _flagUseGrpLstInCoord () { return TRUE ; }
   } ;

   /*
    * rtnCoordCMDAlterCollection define
    */
   class rtnCoordCMDAlterCollection : public rtnCoordDataCMD2Phase
   {
   protected :
      virtual INT32 _parseMsg ( MsgHeader *pMsg,
                                _rtnCMDArguments *pArgs ) ;

      virtual INT32 _generateCataMsg ( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       _rtnCMDArguments *pArgs,
                                       CHAR **ppMsgBuf,
                                       MsgHeader **ppCataMsg ) ;

      virtual const CHAR *_getCommandName () const
      { return CMD_NAME_ALTER_COLLECTION ; }

   protected :
      /* update catalog info before send command to Data Groups */
      virtual BOOLEAN _flagUpdateBeforeData () { return TRUE ; }

      /* use group in Coord cache, since we only have short-term locks in Catalog */
      virtual BOOLEAN _flagUseGrpLstInCoord () { return TRUE ; }
   } ;

   /*
    * rtnCoordCMDLinkCollection define
    */
   class rtnCoordCMDLinkCollection : public rtnCoordDataCMD2Phase
   {
   protected :
      class _rtnCMDLinkCLArgs : public _rtnCMDArguments
      {
      public :
         _rtnCMDLinkCLArgs () {}

         virtual ~_rtnCMDLinkCLArgs () {}

         /* Name of the sub-collection */
         string _subCLName ;
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

      virtual INT32 _generateRollbackDataMsg ( MsgHeader *pMsg,
                                               _rtnCMDArguments *pArgs,
                                               CHAR **ppMsgBuf,
                                               MsgHeader **ppRollbackMsg ) ;

      virtual INT32 _rollbackOnDataGroup ( MsgHeader *pMsg,
                                           pmdEDUCB *cb,
                                           _rtnCMDArguments *pArgs,
                                           const CoordGroupList &groupLst ) ;

      virtual const CHAR *_getCommandName () const
      { return CMD_NAME_LINK_CL ; }

   protected :
      /* update catalog info before send command to Data Groups */
      virtual BOOLEAN _flagUpdateBeforeData () { return TRUE ; }

      /* Rollback on Catalog before rollback on Data groups */
      virtual BOOLEAN _flagRollbackCataBeforeData () { return TRUE ; }
   } ;

   /*
    * _rtnCoordCMDUnlinkCollection define
    */
   class rtnCoordCMDUnlinkCollection : public rtnCoordDataCMD2Phase
   {
   protected :
      class _rtnCMDUnlinkCLArgs : public _rtnCMDArguments
      {
      public :
         _rtnCMDUnlinkCLArgs () {}

         virtual ~_rtnCMDUnlinkCLArgs () {}

         /* Name of the sub-collection */
         string _subCLName ;
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

      virtual INT32 _generateRollbackDataMsg ( MsgHeader *pMsg,
                                               _rtnCMDArguments *pArgs,
                                               CHAR **ppMsgBuf,
                                               MsgHeader **ppRollbackMsg ) ;

      virtual INT32 _rollbackOnDataGroup ( MsgHeader *pMsg,
                                           pmdEDUCB *cb,
                                           _rtnCMDArguments *pArgs,
                                           const CoordGroupList &groupLst ) ;

      virtual const CHAR *_getCommandName () const
      { return CMD_NAME_UNLINK_CL ; }

   protected :
      /* update catalog info before send command to Data Groups */
      virtual BOOLEAN _flagUpdateBeforeData () { return TRUE ; }

      /* Rollback on Catalog before rollback on Data groups */
      virtual BOOLEAN _flagRollbackCataBeforeData () { return TRUE ; }
   } ;

   /*
    * rtnCoordCMDSplit define
    */
   class rtnCoordCMDSplit : public rtnCoordCommand
   {
   public :
      virtual INT32 execute ( MsgHeader *pMsg,
                              pmdEDUCB *cb,
                              INT64 &contextID,
                              rtnContextBuf *buf ) ;

   protected :
      INT32 _getCLCount ( const CHAR *clFullName,
                          CoordGroupList &groupList,
                          pmdEDUCB *cb, UINT64 &count,
                          rtnContextBuf *buf ) ;

      INT32 _getBoundByPercent ( const CHAR *cl,
                                 FLOAT64 percent,
                                 CoordCataInfoPtr &cataInfo,
                                 CoordGroupList &groupList,
                                 pmdEDUCB *cb,
                                 BSONObj &lowBound,
                                 BSONObj &upBound,
                                 rtnContextBuf *buf ) ;

      INT32 _getBoundByCondition ( const CHAR *cl,
                                   const BSONObj &begin,
                                   const BSONObj &end,
                                   CoordGroupList &groupList,
                                   pmdEDUCB *cb,
                                   CoordCataInfoPtr &cataInfo,
                                   BSONObj &lowBound,
                                   BSONObj &upBound,
                                   rtnContextBuf *buf ) ;

      INT32 _getBoundRecordOnData ( const CHAR *cl,
                                    const BSONObj &condition,
                                    const BSONObj &hint,
                                    const BSONObj &sort,
                                    INT32 flag,
                                    INT64 skip,
                                    CoordGroupList &groupList,
                                    pmdEDUCB *cb,
                                    BSONObj &shardingKey,
                                    BSONObj &record,
                                    rtnContextBuf *buf ) ;

   } ;

   /*
    * rtnCoordCMDCreateIndex define
    */
   class rtnCoordCMDCreateIndex : public rtnCoordDataCMD2Phase
   {
   protected :
      class _rtnCMDCreateIndexArgs : public _rtnCMDArguments
      {
      public :
         _rtnCMDCreateIndexArgs () {}

         virtual ~_rtnCMDCreateIndexArgs () {}

         /* Name of the index, used by createIdx */
         string _indexName ;
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

      virtual INT32 _generateRollbackDataMsg ( MsgHeader *pMsg,
                                               _rtnCMDArguments *pArgs,
                                               CHAR **ppMsgBuf,
                                               MsgHeader **ppRollbackMsg ) ;

      virtual INT32 _rollbackOnDataGroup ( MsgHeader *pMsg,
                                           pmdEDUCB *cb,
                                           _rtnCMDArguments *pArgs,
                                           const CoordGroupList &groupLst ) ;

      virtual const CHAR *_getCommandName () const
      { return CMD_NAME_CREATE_INDEX ; }

   protected :
      /* update catalog info before send command to Data Groups */
      virtual BOOLEAN _flagUpdateBeforeData () { return TRUE ; }

      /* use group in Coord cache, since we only have short-term locks in Catalog */
      virtual BOOLEAN _flagUseGrpLstInCoord () { return TRUE ; }
   } ;

   /*
    * rtnCoordCMDDropIndex define
    */
   class rtnCoordCMDDropIndex : public rtnCoordDataCMD2Phase
   {
   protected :
      virtual INT32 _parseMsg ( MsgHeader *pMsg,
                                _rtnCMDArguments *pArgs ) ;

      virtual INT32 _generateCataMsg ( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       _rtnCMDArguments *pArgs,
                                       CHAR **ppMsgBuf,
                                       MsgHeader **ppCataMsg ) ;

      virtual const CHAR *_getCommandName () const
      { return CMD_NAME_DROP_INDEX ; }

   protected :
      /* update catalog info before send command to Data Groups */
      virtual BOOLEAN _flagUpdateBeforeData () { return TRUE ; }

      /* use group in Coord cache, since we only have short-term locks in Catalog */
      virtual BOOLEAN _flagUseGrpLstInCoord () { return TRUE ; }
   } ;
}

#endif
