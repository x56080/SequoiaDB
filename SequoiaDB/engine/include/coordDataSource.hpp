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

   Source File Name = coordDataSource.hpp

   Descriptive Name = Coordinator data source header.

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains Runtime code for insert
   request.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/08/2020  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef COORD_DATASOURCE_HPP__
#define COORD_DATASOURCE_HPP__

#include "IDataSource.hpp"
#include "pmdRemoteSession.hpp"
#include "ossRWMutex.hpp"
#include "../bson/bson.hpp"

using namespace bson ;

namespace engine
{

   class _pmdEDUCB ;
   class _coordDataSourceMsgHandler ;
   class _coordDSMsgConvertor ;

   /**
    * Data source manager on coordinator. Actually it's a data source cache,
    * which contains data source meta data. When operation on coordinator wants
    * to access data source information, it try to get it from the cache. If
    * it's not in the cache, it will be fetched from the catalogue and the cache
    * will be updated.
    * The data source manager uses a isolated net route agent to communicate
    * with data source nodes(Coordinators in the data source).
    */
   class _coordDataSourceMgr : public _IRemoteMgrControl
   {
      struct str_compare
      {
         bool operator() ( const CHAR *l, const CHAR *r ) const
         {
            return ossStrcmp( l, r ) < 0 ;
         }
      } ;

      typedef ossPoolMap< const CHAR*, CoordDataSourcePtr, str_compare > DATASOURCE_MAP ;
      typedef DATASOURCE_MAP::const_iterator    DATASOURCE_MAP_CITR ;

      typedef ossPoolMap< UTIL_DS_UID, CoordDataSourcePtr > DATASOURCE_ID_MAP ;
      typedef DATASOURCE_ID_MAP::iterator                   DATASOURCE_ID_MAP_ITR ;
      typedef DATASOURCE_ID_MAP::const_iterator             DATASOURCE_ID_MAP_CITR ;

   public:
      _coordDataSourceMgr() ;
      ~_coordDataSourceMgr() ;

      INT32       init( _pmdRemoteSessionMgr *pRemoteMgr ) ;
      void        fini() ;
      INT32       active() ;
      INT32       deactive() ;
      void        onConfigChange() ;

      void        clear() ;
      void        invalidate() ;

      _netRouteAgent*   getRouteAgent() { return _pDSAgent ; }

      void        removeDataSource( const CHAR *name ) ;
      void        removeDataSource( UTIL_DS_UID id ) ;

      INT32       getDataSource( const CHAR *name,
                                 CoordDataSourcePtr &dsPtr ) ;
      INT32       getDataSource( UTIL_DS_UID dataSourceID,
                                 CoordDataSourcePtr &dsPtr ) ;

      INT32       updateDataSource( const CHAR *name,
                                    CoordDataSourcePtr &dsPtr,
                                    _pmdEDUCB *cb ) ;
      INT32       updateDataSource( UTIL_DS_UID id,
                                    CoordDataSourcePtr &dsPtr,
                                    _pmdEDUCB *cb ) ;

      INT32       getOrUpdateDataSource( const CHAR *name,
                                         CoordDataSourcePtr &dsPtr,
                                         _pmdEDUCB *cb ) ;
      INT32       getOrUpdateDataSource( UTIL_DS_UID id,
                                         CoordDataSourcePtr &dsPtr,
                                         _pmdEDUCB *cb ) ;

      BOOLEAN     hasDataSource( UTIL_DS_UID dataSourceID ) ;

   public:
      virtual void   checkSubSession( const MsgRouteID &routeID,
                                      netRouteAgent **ppAgent,
                                      IRemoteMsgConvertor **ppMsgConvertor,
                                      BOOLEAN &isExternConn ) ;

      virtual BOOLEAN canErrFilterOut( _pmdSubSession *pSub,
                                       const MsgHeader *pOrgMsg,
                                       INT32 err ) ;

      virtual BOOLEAN canSwitchOtherNode( INT32 err ) ;

   private:
      INT32 _createDataSource( const CHAR *type, IDataSource **dataSource ) ;
      INT32 _updateDataSource( const BSONObj &object,
                               CoordDataSourcePtr &dsPtr ) ;
      INT32 _updateDataSourceInfo( const BSONObj &matcher,
                                   _pmdEDUCB *cb,
                                   CoordDataSourcePtr &dsPtr ) ;
      INT32 _updateRouteInfo( const CoordGroupInfoPtr &groupPtr ) ;
      INT32 _isErrFilterable( INT32 err ) ;

   private:
      ossRWMutex              _metaLatch ;
      DATASOURCE_MAP          _dataSources ;
      DATASOURCE_ID_MAP       _dataSourceIDMap ;

      _netRouteAgent                *_pDSAgent ;
      _coordDataSourceMsgHandler    *_pDSMsgHandler ;
      _coordDSMsgConvertor          *_pDSMsgConvertor ;

   } ;
   typedef _coordDataSourceMgr coordDataSourceMgr ;
}

#endif /* COORD_DATASOURCE_HPP__*/
