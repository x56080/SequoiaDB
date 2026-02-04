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

   Source File Name = catNodeManager.hpp

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
#ifndef CATNODEMANAGER_HPP__
#define CATNODEMANAGER_HPP__

#include "pmd.hpp"
#include "netDef.hpp"
#include "rtnContextBuff.hpp"

using namespace bson ;

namespace engine
{
   class sdbCatalogueCB ;
   class _SDB_RTNCB ;
   class _dpsLogWrapper ;
   class _SDB_DMSCB ;

   /*
      catNodeManager define
   */
   class catNodeManager : public SDBObject
   {
   public:
      catNodeManager() ;
      ~catNodeManager() ;

      INT32 init() ;
      INT32 fini() ;

      void  attachCB( _pmdEDUCB *cb ) ;
      void  detachCB( _pmdEDUCB *cb ) ;

      INT32 processMsg( const NET_HANDLE &handle, MsgHeader *pMsg ) ;

      INT32 active() ;
      INT32 deactive() ;

      INT32 loadNodeInfo() ;

   // message process functions
   protected:
      INT32 processCommandMsg( const NET_HANDLE &handle, MsgHeader *pMsg,
                               BOOLEAN writable ) ;

      INT32 processCmdCreateGrp( const CHAR *pQuery, rtnContextBuf &buf ) ;
      INT32 processCmdUpdateNode( const NET_HANDLE &handle,
                                  const CHAR *pQuery,
                                  const CHAR *pSelector ) ;

      INT32 processGrpReq( const NET_HANDLE &handle, MsgHeader *pMsg ) ;
      INT32 processRegReq( const NET_HANDLE &handle, MsgHeader *pMsg ) ;
      INT32 processPrimaryChange( const NET_HANDLE &handle, MsgHeader *pMsg ) ;

      INT32 readCataConf();
      INT32 parseCatalogConf( CHAR *pData, const SINT64 sDataSize,
                              SINT64 &sParseBytes );
      INT32 parseLine( const CHAR *pLine, BSONObj &obj );
      INT32 generateGroupInfo( BSONObj &boConf,
                               BSONObj &boGroupInfo );
      INT32 saveGroupInfo ( BSONObj &boGroupInfo, INT16 w );
      INT32 parseIDInfo( const BSONObj &obj );
      INT32 getNodeInfo( const BSONObj &boReq, BSONObj &boNodeInfo,
                         INT32 &role ) ;

      INT32 _checkAndUpdateNodeInfo( const BSONObj &reqObj,
                                     INT32 role,
                                     const BSONObj &nodeObj ) ;

      INT32 _loadGroupInfo() ;

   // tool fuctions
   private:
      INT32 _createGrp( const CHAR *groupName, rtnContextBuf &buf ) ;
      INT32 _updateNodeToGrp ( BSONObj &boGroupInfo,
                               const BSONObj &boNodeInfoNew,
                               UINT16 nodeID,
                               BOOLEAN isLoalConn,
                               BOOLEAN setStatus,
                               BOOLEAN keepInstanceID ) ;
      INT32 _getRemovedGroupsObj( const BSONObj &srcGroupsObj,
                                  UINT16 removeNode,
                                  BSONObj &removedObj,
                                  BSONArrayBuilder &newObjBuilder ) ;
      INT16 _majoritySize() ;

      INT32 _getNodeInfoByConf( BSONObj &boConf, BSONObjBuilder &bobNodeInfo ) ;

      INT32 _setReplicaPrimaryChange( MsgRouteID srcNode,
                                      MsgRouteID oldPrimary,
                                      MsgRouteID newPrimary ) ;

      INT32 _setLocationPrimaryChange( MsgRouteID srcNode,
                                       MsgRouteID oldPrimary,
                                       MsgRouteID newPrimary,
                                       UINT32 locationID ) ;

   public:
      INT32 setLocation( UINT16 nodeID,
                         UINT32 groupID,
                         const ossPoolString &newLoc ) ;

      INT32 removeLocation( UINT16 nodeID,
                            UINT32 groupID ) ;

      INT32 setActiveLocation( UINT32 groupID,
                               const ossPoolString &newActLoc ) ;

      INT32 removeActiveLocation( UINT32 groupID ) ;

      INT32 setGroupLocation( const BSONObj &groupInfo ,
                              UINT32 groupID,
                              const ossPoolString &newLoc,
                              const ossPoolString &hostName,
                              BOOLEAN *pHasChanged = NULL,
                              BOOLEAN *pHasMatched = NULL ) ;

      INT32 startGrpMode( const clsGroupMode &grpMode,
                          const string &groupName,
                          const BSONObj &groupObj ) ;

      INT32 stopGrpMode( const clsGroupMode &grpMode, BOOLEAN *pHasChanged = NULL ) ;

   private:
      _SDB_DMSCB                 *_pDmsCB ;
      _dpsLogWrapper             *_pDpsCB ;
      _SDB_RTNCB                 *_pRtnCB ;
      sdbCatalogueCB             *_pCatCB ;
      pmdEDUCB                   *_pEduCB ;

   } ;
}

#endif // CATNODEMANAGER_HPP__

