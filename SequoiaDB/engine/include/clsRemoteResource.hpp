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

   Source File Name = clsRemoteResource.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/20/2022  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CLS_REMOTE_RESOURCE_HPP__
#define CLS_REMOTE_RESOURCE_HPP__

#include "coordDef.hpp"
#include "pmdOptionsMgr.hpp"
#include "coordOmCache.hpp"
#include "IDataSource.hpp"
#include "ossMemPool.hpp"
#include "pmdEDU.hpp"
#include "netRouteAgent.hpp"
#include "sdbIOmProxy.hpp"
#include "coordDataSource.hpp"
#include "coordSequenceAgent.hpp"
#include "../bson/bson.hpp"

namespace engine
{

   /*
      _clsRemoteResource define
    */
   class _clsRemoteResource
   {
   protected:
      struct cmp_str
      {
         bool operator() ( const char *a, const char *b )
         {
            return ossStrcmp( a, b ) < 0 ;
         }
      } ;

      typedef ossPoolMap< UINT32, CoordGroupInfoPtr > MAP_GROUP_INFO ;
      typedef MAP_GROUP_INFO::iterator                MAP_GROUP_INFO_IT ;

      typedef ossPoolMap<std::string, UINT32>         MAP_GROUP_NAME ;
      typedef MAP_GROUP_NAME::iterator                MAP_GROUP_NAME_IT ;

      typedef ossPoolMap<const CHAR*, CoordCataInfoPtr, cmp_str>  MAP_CATA_INFO ;
#if defined (_WINDOWS)
      typedef MAP_CATA_INFO::iterator                 MAP_CATA_INFO_IT ;
      typedef MAP_CATA_INFO::const_iterator           MAP_CATA_INFO_CIT ;
#else
      typedef ossPoolMap<const CHAR*, CoordCataInfoPtr>::iterator       MAP_CATA_INFO_IT ;
      typedef ossPoolMap<const CHAR*, CoordCataInfoPtr>::const_iterator MAP_CATA_INFO_CIT ;
#endif // _WINDOWS

   public:
      _clsRemoteResource() ;
      virtual ~_clsRemoteResource() ;

      INT32       init( _netRouteAgent *pAgent,
                        pmdOptionsCB *pOptionsCB,
                        _coordDataSourceMgr *pDSMgr = NULL ) ;
      INT32       init() ;
      void        fini() ;

      void        invalidateCataInfo( const CHAR *clFullName = NULL ) ;
      void        invalidateGroupInfo( UINT64 identify = 0 ) ;
      void        invalidateStrategy() ;
      void        invalidateDataSourceInfo( const CHAR *name = NULL ) ;

      netRouteAgent*   getRouteAgent() ;
      IOmProxy*        getOmProxy() ;
      coordOmStrategyAgent* getOmStrategyAgent() ;

      coordSequenceAgent* getSequenceAgent()
      {
         return _pSequenceAgent ;
      }

      coordDataSourceMgr* getDSManager()
      {
         return _pDataSourceMgr ;
      }

      void setNodeID( const MsgRouteID &nodeID )
      {
         _selfNodeID.value = nodeID.value ;
      }

      const MsgRouteID &getNodeID() const
      {
         return _selfNodeID ;
      }

   public:
      INT32       getGroupInfo( UINT32 groupID,
                                CoordGroupInfoPtr &groupPtr ) ;
      INT32       getGroupInfo( const CHAR *groupName,
                                CoordGroupInfoPtr &groupPtr ) ;

      UINT32      getGroupsInfo( GROUP_VEC &vecGroupPtr,
                                 BOOLEAN exceptCata,
                                 BOOLEAN exceptCoord ) ;
      UINT32      getGroupList( CoordGroupList &groupList,
                                BOOLEAN exceptCata,
                                BOOLEAN exceptCoord ) ;
      INT32       getGroupIDs( VEC_UINT32 &groupIDs,
                               BOOLEAN exceptCata,
                               BOOLEAN exceptCoord ) ;
      INT32       getGroupNames( VEC_POOLSTR &groupNames,
                                 BOOLEAN exceptCata,
                                 BOOLEAN exceptCoord ) ;
      INT32       getGroupNames( std::vector< std::string > &groupNames,
                                 BOOLEAN exceptCata,
                                 BOOLEAN exceptCoord ) ;
      INT32       addGroupInfo( const bson::BSONObj &groupObj ) ;
      INT32       addGroupInfo( const bson::BSONObj &groupObj,
                                CoordGroupInfoPtr &groupPtr ) ;
      INT32       addGroupInfo( UINT32 groupID,
                                NET_ROUTE_MAP &nodes,
                                UINT32 primaryNodeID ) ;
      INT32       addGroupInfo( UINT32 groupID,
                                NET_ROUTE_MAP &nodes,
                                UINT32 primaryNodeID,
                                CoordGroupInfoPtr &groupPtr ) ;
      INT32       updateGroupInfo( UINT32 groupID,
                                   CoordGroupInfoPtr &groupPtr,
                                   _pmdEDUCB *cb ) ;
      INT32       updateGroupInfo( const CHAR *groupName,
                                   CoordGroupInfoPtr &groupPtr,
                                   _pmdEDUCB *cb ) ;
      INT32       getOrUpdateGroupInfo( const CHAR *groupName,
                                        CoordGroupInfoPtr &groupPtr,
                                        _pmdEDUCB *cb ) ;

      INT32       getOrUpdateGroupInfo( UINT32 groupID,
                                        CoordGroupInfoPtr &groupPtr,
                                        _pmdEDUCB *cb ) ;

      INT32       updateGroupsInfo( GROUP_VEC &vecGroupPtr,
                                    _pmdEDUCB *cb,
                                    const bson::BSONObj *pCondObj = NULL,
                                    BOOLEAN exceptCata = FALSE,
                                    BOOLEAN exceptCoord = FALSE ) ;

      INT32       updateGroupList( CoordGroupList &groupList,
                                   _pmdEDUCB *cb,
                                   const bson::BSONObj *pCondObj = NULL,
                                   BOOLEAN exceptCata = FALSE,
                                   BOOLEAN exceptCoord = FALSE,
                                   BOOLEAN useLocalWhenFailed = TRUE ) ;

      void        removeGroupInfo( UINT32 groupID ) ;
      void        removeGroupInfo( const CHAR *groupName ) ;

      CoordGroupInfoPtr    getCataGroupInfo() ;
      INT32                updateCataGroupInfo( CoordGroupInfoPtr &groupPtr,
                                                _pmdEDUCB *cb ) ;

      INT32       groupID2Name ( UINT32 id, std::string &name ) ;
      INT32       groupName2ID ( const CHAR* name, UINT32 &id ) ;

      void        getCataNodeAddrList( CoordVecNodeInfo &vecCata ) ;
      INT32       syncAddress2Options( BOOLEAN flush = TRUE,
                                       BOOLEAN force = FALSE ) ;

      void        clearCataNodeAddrList() ;
      BOOLEAN     addCataNodeAddrWhenEmpty( const CHAR *pHostName,
                                            const CHAR *pSvcName ) ;

      CoordGroupInfoPtr    getOmGroupInfo() ;
      INT32                updateOmGroupInfo( CoordGroupInfoPtr &groupPtr,
                                              _pmdEDUCB *cb ) ;

   public:
      INT32       addCataInfo( const bson::BSONObj &cataObj ) ;
      INT32       addCataInfo( const bson::BSONObj &cataObj,
                               CoordCataInfoPtr &cataPtr ) ;

      INT32       getCataInfo( const CHAR *collectionName,
                               CoordCataInfoPtr &cataPtr ) ;

      void        removeCataInfo( const CHAR *collectionName ) ;
      void        removeCataInfo( const CHAR *collectionName,
                                  CoordCataInfoPtr &removedCataPtr ) ;
      void        removeCataInfoWithMain( const CHAR *collectionName ) ;
      void        removeCataInfoWithMain( const CHAR *collectionName,
                                          CoordCataInfoPtr &removedCataPtr,
                                          CoordCataInfoPtr &removedMainCataPtr ) ;

      void        removeCataInfoByCS( const CHAR *csName,
                                      BOOLEAN needRemoveRelated = FALSE ) ;
      void        removeCataInfoByCS( const CHAR *csName,
                                      ossPoolVector< ossPoolString > &subCLs,
                                      ossPoolSet< ossPoolString > &mainCLs ) ;

      INT32       updateCataInfo( const CHAR *collectionName,
                                  CoordCataInfoPtr &cataPtr,
                                  _pmdEDUCB *cb ) ;

      INT32       updateCataInfoByCLUID( utilCLUniqueID clUID,
                                         CoordCataInfoPtr &cataPtr,
                                         _pmdEDUCB *cb ) ; ;

      INT32       getOrUpdateCataInfo( const CHAR *collectionName,
                                       CoordCataInfoPtr &cataPtr,
                                       _pmdEDUCB *cb ) ;

      void        updateNodeStat( const MsgRouteID &nodeID, INT32 rc ) ;

   protected:
      void        setCataGroupInfo( CoordGroupInfoPtr &groupPtr,
                                    BOOLEAN inheritStat = FALSE ) ;
      void        setOmGroupInfo( CoordGroupInfoPtr &groupPtr ) ;
      void        addGroupInfo( CoordGroupInfoPtr &groupPtr,
                                BOOLEAN inheritStat = FALSE ) ;

      UINT32      checkAndRemoveCataInfoBySub( const CHAR *collectionName ) ;

   protected:
      INT32       _addCataInfo( const bson::BSONObj &cataObj,
                                CoordCataInfoPtr &cataPtr ) ;
      INT32       _addCataInfo( CoordCataInfoPtr &cataPtr ) ;
      INT32       _addGroupInfo( const bson::BSONObj &groupObj,
                                 CoordGroupInfoPtr &groupPtr ) ;
      INT32       _addGroupInfo( UINT32 groupID,
                                 NET_ROUTE_MAP &nodes,
                                 UINT32 primaryNodeID,
                                 CoordGroupInfoPtr &groupPtr ) ;

      void        _clearGroupName( UINT32 groupID ) ;
      void        _addGroupName( const std::string &name, UINT32 id ) ;

      INT32       _updateCataGroupInfoByAddr( _pmdEDUCB *cb,
                                              CoordGroupInfoPtr &groupPtr ) ;
      INT32       _updateCataGroupInfo( _pmdEDUCB *cb,
                                        const CoordGroupInfoPtr &cataGroupPtr,
                                        CoordGroupInfoPtr &groupPtr ) ;

      INT32       _processGroupReply( MsgHeader *pMsg,
                                      CoordGroupInfoPtr &groupPtr ) ;

      INT32       _processGroupContextReply( INT64 &contextID,
                                             GROUP_VEC &vecGroupPtr,
                                             _pmdEDUCB *cb ) ;

      INT32       _updateRouteInfo( const CoordGroupInfoPtr &groupPtr,
                                    MSG_ROUTE_SERVICE_TYPE type ) ;

      INT32       _updateGroupInfo( MsgHeader *pMsg,
                                    _pmdEDUCB *cb,
                                    MSG_ROUTE_SERVICE_TYPE type,
                                    CoordGroupInfoPtr &groupPtr ) ;

      void        _addAddrNode( const MsgRouteID &id,
                                const CHAR *pHostName,
                                const CHAR *pSvcName,
                                INT32 serviceType,
                                CoordVecNodeInfo &vecAddr ) ;

      void        _initAddressFromPair( const std::vector< pmdAddrPair > &vecAddrPair,
                                        INT32 serviceType,
                                        CoordVecNodeInfo &vecAddr ) ;

      INT32       _updateCataInfo( const bson::BSONObj &obj,
                                   const CHAR *collectionName,
                                   CoordCataInfoPtr &cataPtr,
                                   _pmdEDUCB *cb ) ;

      INT32       _processCatalogReply( MsgHeader *pMsg,
                                        const CHAR *collectionName,
                                        CoordCataInfoPtr &cataPtr ) ;

      bson::BSONObj _buildOmGroupInfo() ;

      INT32       _updateCataInfoByCLUID( utilCLUniqueID clUID,
                                          CoordCataInfoPtr &cataPtr,
                                          _pmdEDUCB *cb ) ;

      INT32       _processCatalogReplyByCLUID( MsgHeader *pMsg,
                                               CoordCataInfoPtr &cataPtr ) ;

      virtual UINT32 _getCataInfoParseGroupID() const
      {
         return INVALID_GROUPID ;
      }

   protected:
      MsgRouteID                       _selfNodeID ;
      MAP_GROUP_INFO                   _mapGroupInfo ;
      MAP_GROUP_NAME                   _mapGroupName ;
      ossSpinSLatch                    _nodeMutex ;

      CoordGroupInfoPtr                _cataGroupInfo ;
      CoordGroupInfoPtr                _omGroupInfo ;

      UINT64                           _upGrpIndentify ;
      CoordVecNodeInfo                 _cataNodeAddrList ;
      BOOLEAN                          _cataAddrChanged ;

      CoordVecNodeInfo                 _omNodeAddrList ;

      MAP_CATA_INFO                    _mapCataInfo ;
      ossSpinSLatch                    _cataMutex ;

      _netRouteAgent                   *_pAgent ;
      pmdOptionsCB                     *_pOptionsCB ;
      CoordGroupInfoPtr                _emptyGroupPtr ;

      _IOmProxy                        *_pOmProxy ;
      _coordOmStrategyAgent            *_pOmStrategyAgent ;

      _coordSequenceAgent              *_pSequenceAgent ;

      _coordDataSourceMgr              *_pDataSourceMgr ;
   } ;
   typedef class _clsRemoteResource clsRemoteResource ;

}

#endif // CLS_REMOTE_RESOURCE_HPP__
