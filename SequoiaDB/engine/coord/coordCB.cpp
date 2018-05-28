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

   Source File Name = coordCB.cpp

   Descriptive Name =

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

#include "coordCB.hpp"
#include "pmd.hpp"
#include "ossTypes.h"
#include "pdTrace.hpp"
#include "coordTrace.hpp"
#include "coordDef.hpp"
#include "pmdController.hpp"
#include "pmdStartup.hpp"

using namespace bson;
namespace engine
{

   /*
   note: _CoordCB implement
   */
   _CoordCB::_CoordCB()
   {
      _pNetWork = NULL ;
      _upGrpIndentify = 1 ;
   }

   _CoordCB::~_CoordCB()
   {
   }

   INT32 _CoordCB::init ()
   {
      INT32 rc = SDB_OK ;
      CoordGroupInfo *pGroupInfo = NULL ;
      UINT32 catGID = CATALOG_GROUPID ;
      UINT16 catNID = SYS_NODE_ID_BEGIN + CLS_REPLSET_MAX_NODE_SIZE ;
      MsgRouteID id ;
      pmdOptionsCB *optCB = pmdGetOptionCB() ;
      vector< _pmdAddrPair > catAddrs = optCB->catAddrs() ;

      // 1. create objs
      _pNetWork = SDB_OSS_NEW _netRouteAgent( &_multiRouteAgent ) ;
      if ( !_pNetWork )
      {
         PD_LOG( PDERROR, "Failed to alloc memory for net agent" ) ;
         rc = SDB_OOM ;
         goto error ;
      }
      _pNetWork->getFrame()->setBeatInfo( pmdGetOptionCB()->getOprTimeout() ) ;
      _multiRouteAgent.setNetWork( _pNetWork ) ;

      pGroupInfo = SDB_OSS_NEW CoordGroupInfo( CAT_CATALOG_GROUPID ) ;
      if ( !pGroupInfo )
      {
         PD_LOG( PDERROR, "Failed to alloc memory for group info" ) ;
         rc = SDB_OOM ;
         goto error ;
      }
      _catGroupInfo = CoordGroupInfoPtr( pGroupInfo ) ;

      // 2. init param
      for ( UINT32 i = 0 ; i < catAddrs.size() ; ++i )
      {
         if ( 0 == catAddrs[i]._host[ 0 ] )
         {
            break ;
         }
         id.columns.groupID = catGID ;
         id.columns.nodeID = catNID++ ;
         id.columns.serviceID = MSG_ROUTE_CAT_SERVICE ;
         addCatNodeAddr( id, catAddrs[i]._host, catAddrs[i]._service ) ;
      }

      // 3. set startup ok
      pmdGetStartup().ok( TRUE ) ;
      // set nodeid and group name
      pmdGetKRCB()->setGroupName( COORD_GROUPNAME ) ;
      {
         MsgRouteID id ;
         id.value = MSG_INVALID_ROUTEID ;
         id.columns.groupID = COORD_GROUPID ;
         pmdSetNodeID( id ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _CoordCB::active ()
   {
      INT32 rc = SDB_OK ;
      pmdEDUMgr* pEDUMgr = pmdGetKRCB()->getEDUMgr() ;
      EDUID eduID = PMD_INVALID_EDUID ;

      // set to primary
      pmdSetPrimary( TRUE ) ;
      sdbGetPMDController()->registerNet( _pNetWork->getFrame() ) ;

      // 1. start coord net work
      rc = pEDUMgr->startEDU ( EDU_TYPE_COORDNETWORK, (void*)netWork(),
                               &eduID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to start coord network edu, rc: %d",
                   rc ) ;
      pEDUMgr->regSystemEDU ( EDU_TYPE_COORDNETWORK, eduID ) ;
      rc = pEDUMgr->waitUntil( eduID , PMD_EDU_RUNNING ) ;
      PD_RC_CHECK( rc, PDERROR, "Wait CoordNet active failed, rc: %d", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _CoordCB::deactive ()
   {
      // 1. stop io
      if ( _pNetWork )
      {
         sdbGetPMDController()->unregNet( _pNetWork->getFrame() ) ;
         _pNetWork->stop() ;
      }
      return SDB_OK ;
   }

   INT32 _CoordCB::fini ()
   {
      if ( _pNetWork )
      {
         SDB_OSS_DEL _pNetWork ;
         _pNetWork = NULL ;
      }
      /// need to clear the catalog map, because the catalogset will
      /// use the global var, so need clear before main eixt
      _cataInfoMap.clear() ;
      _nodeGroupInfo.clear() ;
      return SDB_OK ;
   }

   void _CoordCB::onConfigChange ()
   {
      if ( _pNetWork )
      {
         UINT32 oprtimeout = pmdGetOptionCB()->getOprTimeout() ;
         _pNetWork->getFrame()->setBeatInfo( oprtimeout ) ;
      }
   }

   void _CoordCB::updateCatGroupInfo( CoordGroupInfoPtr &groupInfo )
   {
      ossScopedLock _lock(&_mutex, EXCLUSIVE) ;
      if ( _catGroupInfo->groupVersion() != groupInfo->groupVersion() )
      {
         // need to update local config
         string oldCfg, newCfg ;
         pmdOptionsCB *optCB = pmdGetOptionCB() ;
         optCB->toString( oldCfg ) ;
         optCB->clearCatAddr() ;
         clearCatNodeAddrList() ;

         UINT32 pos = 0 ;
         MsgRouteID id ;
         string hostName ;
         string svcName ;
         while ( SDB_OK == groupInfo->getNodeInfo( pos, id,
                           hostName, svcName, MSG_ROUTE_CAT_SERVICE ) )
         {
            addCatNodeAddr( id, hostName.c_str(), svcName.c_str() ) ;
            optCB->setCatAddr( hostName.c_str(), svcName.c_str() ) ;
            ++pos ;
         }
         optCB->toString( newCfg ) ;
         if ( oldCfg != newCfg )
         {
            optCB->reflush2File() ;
         }
      }
      _catGroupInfo = groupInfo ;
   }

   void _CoordCB::getCatNodeAddrList ( CoordVecNodeInfo &catNodeLst )
   {
      catNodeLst = _cataNodeAddrList;
   }

   void _CoordCB::clearCatNodeAddrList()
   {
      _cataNodeAddrList.clear();
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_COORDCB_ADDCATNDADDR, "CoordCB::addCatNodeAddr" )
   INT32 _CoordCB::addCatNodeAddr( const _MsgRouteID &id,
                                   const CHAR *pHost,
                                   const CHAR *pService )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_COORDCB_ADDCATNDADDR ) ;
      CoordNodeInfo nodeInfo ;
      nodeInfo._id = id ;
      nodeInfo._id.columns.groupID = 0 ;
      ossStrncpy( nodeInfo._host, pHost, OSS_MAX_HOSTNAME );
      nodeInfo._host[OSS_MAX_HOSTNAME] = 0 ;
      nodeInfo._service[MSG_ROUTE_CAT_SERVICE] = pService ;

      _cataNodeAddrList.push_back( nodeInfo ) ;
      _multiRouteAgent.updateRoute( nodeInfo._id,
                                    nodeInfo._host,
                                    nodeInfo._service[MSG_ROUTE_CAT_SERVICE].c_str() );
      PD_TRACE_EXIT ( SDB_COORDCB_ADDCATNDADDR );
      return rc ;
   }

   INT32 _CoordCB::_addGroupName ( const std::string& name, UINT32 id )
   {
      INT32 rc = SDB_OK ;
      GROUP_NAME_MAP_IT it = _groupNameMap.find ( name ) ;
      if ( it != _groupNameMap.end() )
      {
         // the same
         if ( it->second == id )
         {
            rc = SDB_OK ;
            goto done ;
         }
      }
      _groupNameMap[name] = id ;

   done :
      return rc ;
   }

   INT32 _CoordCB::_clearGroupName( UINT32 id )
   {
      GROUP_NAME_MAP_IT it = _groupNameMap.begin() ;
      while ( it != _groupNameMap.end() )
      {
         if ( it->second == id )
         {
            _groupNameMap.erase( it ) ;
            break ;
         }
         ++it ;
      }
      return SDB_OK ;
   }

   INT32 _CoordCB::groupID2Name ( UINT32 id, std::string &name )
   {
      ossScopedLock _lock( &_nodeGroupMutex, SHARED ) ;

      CoordGroupMap::iterator it = _nodeGroupInfo.find( id ) ;
      if ( it == _nodeGroupInfo.end() )
      {
         return SDB_COOR_NO_NODEGROUP_INFO ;
      }
      name = it->second->groupName() ;

      return SDB_OK ;
   }

   INT32 _CoordCB::groupName2ID ( const CHAR* name, UINT32 &id )
   {
      ossScopedLock _lock( &_nodeGroupMutex, SHARED ) ;

      GROUP_NAME_MAP::iterator it = _groupNameMap.find( name ) ;
      if ( it == _groupNameMap.end() )
      {
         return SDB_COOR_NO_NODEGROUP_INFO ;
      }
      id = it->second ;

      return SDB_OK ;
   }

   UINT32 _CoordCB::getLocalGroupList( GROUP_VEC &groupLst,
                                       BOOLEAN exceptCata,
                                       BOOLEAN exceptCoord )
   {
      UINT32 count = 0 ;
      ossScopedLock _lock( &_nodeGroupMutex, SHARED ) ;
      CoordGroupMap::iterator it = _nodeGroupInfo.begin() ;
      while( it != _nodeGroupInfo.end() )
      {
         if ( CATALOG_GROUPID == it->first && exceptCata )
         {
            ++it ;
            continue ;
         }
         else if ( COORD_GROUPID == it->first && exceptCoord )
         {
            ++it ;
            continue ;
         }
         groupLst.push_back( it->second ) ;
         ++count ;
         ++it ;
      }
      return count ;
   }

   UINT32 _CoordCB::getLocalGroupList( CoordGroupList &groupLst,
                                       BOOLEAN exceptCata,
                                       BOOLEAN exceptCoord )
   {
      UINT32 count = 0 ;
      ossScopedLock _lock( &_nodeGroupMutex, SHARED ) ;
      CoordGroupMap::iterator it = _nodeGroupInfo.begin() ;
      while( it != _nodeGroupInfo.end() )
      {
         if ( CATALOG_GROUPID == it->first && exceptCata )
         {
            ++it ;
            continue ;
         }
         else if ( COORD_GROUPID == it->first && exceptCoord )
         {
            ++it ;
            continue ;
         }
         groupLst[ it->first ] = it->first ;
         ++count ;
         ++it ;
      }
      return count ;
   }

   void _CoordCB::addGroupInfo ( CoordGroupInfoPtr &groupInfo )
   {
      ossScopedLock _lock( &_nodeGroupMutex, EXCLUSIVE ) ;

      groupInfo->setIdentify( ++_upGrpIndentify ) ;
      _nodeGroupInfo[groupInfo->groupID()] = groupInfo ;

      // clear group name map
      _clearGroupName( groupInfo->groupID() ) ;

      // add to group name map
      _addGroupName( groupInfo->groupName(), groupInfo->groupID() ) ;
   }

   void _CoordCB::removeGroupInfo( UINT32 groupID )
   {
      ossScopedLock _lock(&_nodeGroupMutex, EXCLUSIVE) ;
      _nodeGroupInfo.erase( groupID ) ;

      // clear group name map
      _clearGroupName( groupID ) ;
   }

   INT32 _CoordCB::getGroupInfo ( UINT32 groupID,
                                  CoordGroupInfoPtr &groupInfo )
   {
      INT32 rc = SDB_OK;
      ossScopedLock _lock( &_nodeGroupMutex, SHARED ) ;
      CoordGroupMap::iterator iter = _nodeGroupInfo.find ( groupID );
      if ( _nodeGroupInfo.end() == iter )
      {
         rc = SDB_COOR_NO_NODEGROUP_INFO;
      }
      else
      {
         groupInfo = iter->second;
      }
      return rc;
   }

   INT32 _CoordCB::getGroupInfo ( const CHAR *groupName,
                                  CoordGroupInfoPtr &groupInfo )
   {
      UINT32 groupID = 0 ;
      INT32 rc = groupName2ID( groupName, groupID ) ;
      if ( SDB_OK == rc )
      {
         rc = getGroupInfo( groupID, groupInfo ) ;
      }
      return rc ;
   }

   void _CoordCB::updateCataInfo ( const string &collectionName,
                                   CoordCataInfoPtr &cataInfo )
   {
      ossScopedLock _lock( &_cataInfoMutex, EXCLUSIVE );
      _cataInfoMap[collectionName] = cataInfo ;
   }

   INT32 _CoordCB::getCataInfo ( const string &strCollectionName,
                                 CoordCataInfoPtr &cataInfo )
   {
      INT32 rc = SDB_CAT_NO_MATCH_CATALOG;
      ossScopedLock _lock( &_cataInfoMutex, SHARED );
      CoordCataMap::iterator iter
                           = _cataInfoMap.find( strCollectionName );
      if ( iter != _cataInfoMap.end() )
      {
         rc = SDB_OK;
         cataInfo = iter->second;
      }
      return rc;
   }

   BOOLEAN _CoordCB::isSubCollection( const CHAR *pCLName )
   {
      string strSubCLName = pCLName ;
      CoordCataMap::iterator it ;
      clsCatalogSet *pCatSet = NULL ;
      ossScopedLock _lock( &_cataInfoMutex, SHARED ) ;

      it = _cataInfoMap.begin() ;
      while( it != _cataInfoMap.end() )
      {
         pCatSet = it->second->getCatalogSet() ;
         if ( !pCatSet || !pCatSet->isMainCL() )
         {
            /// do nothing
         }
         else if ( pCatSet->isContainSubCL( strSubCLName ) )
         {
            return TRUE ;
         }
         ++it ;
      }
      return FALSE ;
   }

   void _CoordCB::delMainCLCataInfo( const CHAR *pSubCLName )
   {
      string strSubCLName = pSubCLName ;
      CoordCataMap::iterator it ;
      clsCatalogSet *pCatSet = NULL ;
      ossScopedLock _lock( &_cataInfoMutex, EXCLUSIVE ) ;

      it = _cataInfoMap.begin() ;
      while( it != _cataInfoMap.end() )
      {
         pCatSet = it->second->getCatalogSet() ;
         if ( !pCatSet || !pCatSet->isMainCL() )
         {
            /// do nothing
         }
         else if ( pCatSet->isContainSubCL( strSubCLName ) )
         {
            _cataInfoMap.erase( it++ ) ;
            continue ;
         }
         ++it ;
      }
   }

   void _CoordCB::delCataInfo ( const string &collectionName )
   {
      ossScopedLock _lock( &_cataInfoMutex, EXCLUSIVE );
      _cataInfoMap.erase( collectionName );
   }

   void _CoordCB::delCataInfoByCS( const CHAR *csName,
                                   vector< string > *pRelatedCLs )
   {
      UINT32 len = ossStrlen( csName ) ;
      clsCatalogSet *pCatSet = NULL ;
      const CHAR *pCLName = NULL ;
      CoordCataMap::iterator it ;

      ossScopedLock _lock( &_cataInfoMutex, EXCLUSIVE ) ;
      it = _cataInfoMap.begin() ;

      if ( 0 == len )
      {
         return ;
      }

      while( it != _cataInfoMap.end() )
      {
         pCatSet = it->second->getCatalogSet() ;
         pCLName = it->first.c_str() ;

         if ( pRelatedCLs && len > 0 && pCatSet )
         {
            string strMainCL = pCatSet->getMainCLName() ;

            if ( 0 == ossStrncmp( strMainCL.c_str(), csName, len ) &&
                 '.' == strMainCL.at( len ) )
            {
               pRelatedCLs->push_back( it->first ) ;
            }
         }

         if ( 0 == ossStrncmp( pCLName, csName, len ) &&
              '.' == pCLName[ len ] )
         {
            _cataInfoMap.erase( it++ ) ;
         }
         else
         {
            ++it ;
         }
      }
   }

   void _CoordCB::invalidateCataInfo()
   {
      ossScopedLock _lock( &_cataInfoMutex, EXCLUSIVE );
      _cataInfoMap.clear() ;
   }

   void _CoordCB::invalidateGroupInfo( UINT64 identify )
   {
      ossScopedLock _lock(&_nodeGroupMutex, EXCLUSIVE) ;

      if ( 0 == identify )
      {
         _nodeGroupInfo.clear() ;
         _groupNameMap.clear() ;
      }
      else
      {
         CoordGroupMap::iterator it = _nodeGroupInfo.begin() ;
         while( it != _nodeGroupInfo.end() )
         {
            if ( it->second->getIdentify() <= identify )
            {
               /// first erase the name map
               GROUP_NAME_MAP::iterator itName =
                  _groupNameMap.find( it->second->groupName() ) ;
               if ( itName != _groupNameMap.end() )
               {
                  _groupNameMap.erase( itName ) ;
               }
               /// erase
               it = _nodeGroupInfo.erase( it ) ;
            }
            else
            {
               ++it ;
            }
         }
      }
   }

   /*
      get global coord cb
   */
   CoordCB* sdbGetCoordCB ()
   {
      static CoordCB s_coordCB ;
      return &s_coordCB ;
   }

}

