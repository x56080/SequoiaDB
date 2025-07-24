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

   Source File Name = coordCB.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/28/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef COORDCB_HPP__
#define COORDCB_HPP__

#include "core.hpp"
#include "oss.hpp"
#include <map>
#include <vector>
#include <string>
#include <boost/shared_ptr.hpp>
#include "netRouteAgent.hpp"
#include "netMultiRouteAgent.hpp"
#include "rtnCoord.hpp"
#include "ossUtil.h"
#include "rtnPredicate.hpp"
#include "msgCatalog.hpp"
#include "clsCatalogAgent.hpp"
#include "coordDef.hpp"
#include "sdbInterface.hpp"

using namespace std ;

namespace engine
{
   /*
      _CoordCB define
   */
   class _CoordCB : public _IControlBlock
   {
      typedef std::map<std::string, UINT32>     GROUP_NAME_MAP ;
      typedef GROUP_NAME_MAP::iterator          GROUP_NAME_MAP_IT ;

      typedef std::map< UINT32, CoordGroupInfoPtr >  MAP_GROUP_INFO ;

   public:
      _CoordCB() ;
      virtual ~_CoordCB() ;

      virtual SDB_CB_TYPE cbType() const { return SDB_CB_COORD ; }
      virtual const CHAR* cbName() const { return "COORDCB" ; }

      virtual INT32  init () ;
      virtual INT32  active () ;
      virtual INT32  deactive () ;
      virtual INT32  fini () ;
      virtual void   onConfigChange() ;

      _netRouteAgent* netWork() { return _pNetWork ; }
      netMultiRouteAgent* getRouteAgent() { return &_multiRouteAgent ; }

      rtnCoordProcesserFactory *getProcesserFactory()
      {
         return &_processerFactory ;
      }

      UINT64 getGrpIdentify() const { return _upGrpIndentify ; }

      void getLock( OSS_LATCH_MODE mode )
      {
         if ( SHARED == mode )
         {
            _mutex.get_shared() ;
         }
         else
         {
            _mutex.get() ;
         }
      }
      void releaseLock( OSS_LATCH_MODE mode )
      {
         if ( SHARED == mode )
         {
            _mutex.release_shared() ;
         }
         else
         {
            _mutex.release() ;
         }
      }

      INT32 addCatNodeAddr( const _MsgRouteID &id,
                            const CHAR *pHost,
                            const CHAR *pService );

      void getCatNodeAddrList ( CoordVecNodeInfo &catNodeLst );

      void clearCatNodeAddrList();

      void updateCatGroupInfo( CoordGroupInfoPtr &groupInfo,
                               BOOLEAN inheritStat = FALSE ) ;

      CoordGroupInfoPtr getCatGroupInfo()
      {
         CoordGroupInfoPtr catGroupInfoTmp;
         {
            ossScopedLock _lock(&_mutex, SHARED) ;
            catGroupInfoTmp = _catGroupInfo ;
         }
         return catGroupInfoTmp;
      }

      MsgRouteID getPrimaryCat()
      {
         ossScopedLock _lock(&_mutex, SHARED) ;
         return _catGroupInfo->primary();
      }

      void setSlaveCat( MsgRouteID slave )
      {
         ossScopedLock _lock(&_mutex, EXCLUSIVE) ;
         _catGroupInfo->updatePrimary( slave, FALSE ) ;
      }

      void setPrimaryCat( MsgRouteID primary )
      {
         ossScopedLock _lock(&_mutex, EXCLUSIVE) ;
         _catGroupInfo->updatePrimary( primary, TRUE ) ;
      }

      INT32 groupID2Name ( UINT32 id, std::string &name ) ;

      INT32 groupName2ID ( const CHAR* name, UINT32 &id ) ;

      UINT32   getLocalGroupList( GROUP_VEC &groupLst,
                                  BOOLEAN exceptCata = FALSE,
                                  BOOLEAN exceptCoord = TRUE ) ;

      UINT32   getLocalGroupList( CoordGroupList &groupLst,
                                  BOOLEAN exceptCata = FALSE,
                                  BOOLEAN exceptCoord = TRUE ) ;

      void  addGroupInfo ( CoordGroupInfoPtr &groupInfo,
                           BOOLEAN inheritStat = FALSE ) ;

      void  removeGroupInfo( UINT32 groupID ) ;

      INT32 getGroupInfo ( UINT32 groupID, CoordGroupInfoPtr &groupInfo ) ;

      INT32 getGroupInfo ( const CHAR *groupName,
                           CoordGroupInfoPtr &groupInfo ) ;

      void  updateCataInfo ( const string &collectionName,
                             CoordCataInfoPtr &cataInfo ) ;

      INT32 getCataInfo ( const string &strCollectionName,
                          CoordCataInfoPtr &cataInfo ) ;

      void  delCataInfo ( const string &collectionName ) ;
      void  delCataInfoByCS( const CHAR *csName,
                             vector< string > *pRelatedCLs = NULL ) ;

      BOOLEAN isSubCollection( const CHAR *pCLName ) ;

      void  invalidateCataInfo() ;
      void  invalidateGroupInfo( UINT64 identify = 0 ) ;

   protected:
      INT32         _addGroupName ( const std::string& name, UINT32 id ) ;
      INT32         _clearGroupName ( UINT32 id ) ;

   private:
      _netRouteAgent                      *_pNetWork;
      ossSpinSLatch                       _mutex;
      netMultiRouteAgent                  _multiRouteAgent;
      CoordGroupInfoPtr                   _catGroupInfo;
      rtnCoordProcesserFactory            _processerFactory;

      ossSpinSLatch                       _nodeGroupMutex;
      MAP_GROUP_INFO                      _nodeGroupInfo;
      GROUP_NAME_MAP                      _groupNameMap ;

      ossSpinSLatch                       _cataInfoMutex;
      CoordCataMap                        _cataInfoMap;

      CoordVecNodeInfo                    _cataNodeAddrList;
      UINT64                              _upGrpIndentify ;

   };
   typedef _CoordCB CoordCB ;

   /*
      get global coord cb
   */
   CoordCB* sdbGetCoordCB() ;

}

#endif // COORDCB_HPP__

