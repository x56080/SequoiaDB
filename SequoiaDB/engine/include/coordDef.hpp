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

   Source File Name = coordDef.hpp

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
#ifndef COORDDEF_HPP__
#define COORDDEF_HPP__

#include "clsCatalogAgent.hpp"
#include "utilMap.hpp"
#include "utilList.hpp"
#include "../bson/bson.h"
#include <vector>
#include <queue>
#include <string>
#include <set>

using namespace bson ;

namespace engine
{
   struct coordErrorInfo
   {
      INT32       _rc ;
      BSONObj     _obj ;

      coordErrorInfo( INT32 rc = SDB_OK )
      {
         _rc = rc ;
      }
      coordErrorInfo( INT32 rc, const BSONObj &obj )
      {
         _rc = rc ;
         _obj = obj.getOwned() ;
      }
      coordErrorInfo( const MsgOpReply *reply )
      {
         INT32 length = reply->header.messageLength -
                        (INT32)sizeof( MsgOpReply ) ;
         _rc = reply->flags ;
         if ( reply->flags && length > 0 )
         {
            try
            {
               _obj = BSONObj( (const CHAR*)reply + sizeof( MsgOpReply ) ).getOwned() ;
            }
            catch( std::exception & )
            {
               /// do nothing
            }
         }
      }
   } ;
   typedef std::queue<CHAR *>                         REPLY_QUE ;
   typedef _utilMap< UINT64, coordErrorInfo, 20 >     ROUTE_RC_MAP ;
   typedef _utilMap< UINT64, MsgHeader*, 20 >         ROUTE_REPLY_MAP ;
   typedef _utilMap< UINT32, netIOVec, 20 >           GROUP_2_IOVEC ;
   typedef std::set< INT32 >                          SET_RC ;

   typedef _utilMap< UINT32, UINT32, 20 >             CoordGroupList ;
   typedef clsNodeItem                                CoordNodeInfo ;
   typedef VEC_NODE_INFO                              CoordVecNodeInfo ;

   typedef clsGroupItem                               CoordGroupInfo ;

   typedef boost::shared_ptr<CoordGroupInfo>          CoordGroupInfoPtr;
   typedef _utilMap< UINT32, CoordGroupInfoPtr, 20 >  CoordGroupMap;
   typedef std::vector< CoordGroupInfoPtr >           GROUP_VEC ;
   typedef std::vector<std::string>                   CoordSubCLlist;
   typedef _utilMap< UINT32, CoordSubCLlist, 20 >     CoordGroupSubCLMap;

   class _CoordCataInfo : public SDBObject
   {
   public:
      // if the catalogue-info is update,
      // build a new one, don't modify the old
      _CoordCataInfo( INT32 version, const char *pCollectionName )
      :_catlogSet ( pCollectionName, FALSE )
      {
         _catlogSet.setSKSite( clsGetShardingKeySite() ) ;
      }

      ~_CoordCataInfo()
      {}

      void getGroupLst( CoordGroupList &groupLst )
      {
         groupLst = _groupLst ;
      }

      INT32 getGroupNum()
      {
         return _groupLst.size();
      }

      BOOLEAN isMainCL()
      {
         return _catlogSet.isMainCL();
      }

      INT32 getSubCLList( CoordSubCLlist &subCLLst )
      {
         return _catlogSet.getSubCLList( subCLLst );
      }

      BOOLEAN isContainSubCL( const std::string &subCLName )
      {
         return _catlogSet.isContainSubCL( subCLName );
      }

      INT32 getSubCLCount ()
      {
         return _catlogSet.getSubCLCount() ;
      }

      INT32 getGroupByMatcher( const bson::BSONObj & matcher,
                               CoordGroupList &groupLst )
      {
         INT32 rc = SDB_OK;

         if ( matcher.isEmpty() )
         {
            getGroupLst( groupLst ) ;
         }
         else
         {
            UINT32 i = 0;
            VEC_GROUP_ID vecGroup;
            rc = _catlogSet.findGroupIDS( matcher, vecGroup );
            PD_RC_CHECK( rc, PDERROR,
                        "failed to find the match groups(rc=%d)",
                        rc );
            for ( ; i < vecGroup.size(); i++ )
            {
               groupLst[vecGroup[i]] = vecGroup[i];
            }
         }
      done:
         return rc;
      error:
         goto done;
      }

      INT32 getGroupByRecord( const bson::BSONObj &recordObj,
                              UINT32 &groupID )
      {
         return _catlogSet.findGroupID ( recordObj, groupID ) ;
      }

      INT32 getSubCLNameByRecord( const bson::BSONObj &recordObj,
                                  std::string &subCLName )
      {
         return _catlogSet.findSubCLName( recordObj, subCLName );
      }

      INT32 getMatchSubCLs( const bson::BSONObj &matcher,
                            CoordSubCLlist &subCLList )
      {
         if ( matcher.isEmpty() )
         {
            return _catlogSet.getSubCLList( subCLList );
         }
         else
         {
            return _catlogSet.findSubCLNames( matcher, subCLList );
         }
      }

      INT32 getVersion()
      {
         return _catlogSet.getVersion() ;
      }
      INT32 fromBSONObj ( const bson::BSONObj &boRecord )
      {
         INT32 rc = _catlogSet.updateCatSet ( boRecord, 0 ) ;
         if ( SDB_OK == rc )
         {
            UINT32 groupID = 0 ;
            VEC_GROUP_ID *vecGroup = _catlogSet.getAllGroupID() ;
            for ( UINT32 index = 0 ; index < vecGroup->size() ;++index )
            {
               groupID = (*vecGroup)[index] ;
               _groupLst[groupID] = groupID ;
            }
         }
         return rc ;
      }

      void getShardingKey ( bson::BSONObj &shardingKey )
      {
         shardingKey = _catlogSet.getShardingKey() ;
      }

      BOOLEAN isIncludeShardingKey( const bson::BSONObj &record )
      {
         return _catlogSet.isIncludeShardingKey( record );
      }

      BOOLEAN isSharded ()
      {
         return _catlogSet.isSharding() ;
      }

      BOOLEAN isRangeSharded() const
      {
         return _catlogSet.isRangeSharding() ;
      }

      INT32 getGroupLowBound( UINT32 groupID, BSONObj &lowBound )
      {
         return _catlogSet.getGroupLowBound( groupID, lowBound ) ;
      }

      clsCatalogSet* getCatalogSet()
      {
         return &_catlogSet ;
      }

      const CHAR* getName()
      {
         return _catlogSet.name() ;
      }

      UINT32 getShardingKeySiteID() const
      {
         return _catlogSet.getShardingKeySiteID() ;
      }

      INT32 getLobGroupID( const bson::OID &oid,
                           UINT32 sequence,
                           UINT32 &groupID )
      {
         return _catlogSet.findGroupID( oid, sequence, groupID ) ;
      }

   private:
      // if the catalogue-info is update, build a new one, don't modify the old
      _CoordCataInfo()
      :_catlogSet( NULL, FALSE ) 
      {}  

   private:
      clsCatalogSet        _catlogSet ;
      CoordGroupList       _groupLst ;

   };
   typedef _CoordCataInfo CoordCataInfo ;

   typedef boost::shared_ptr< CoordCataInfo >         CoordCataInfoPtr ;
   typedef std::map< std::string, CoordCataInfoPtr >  CoordCataMap ;

}

#endif // COORDDEF_HPP__
