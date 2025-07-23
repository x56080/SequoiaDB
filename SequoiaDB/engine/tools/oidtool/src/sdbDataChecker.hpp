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

   Source File Name = sdbDataChecker.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who      Description
   ====== =========== ======== ==============================================
          2022/11/04  TZB      Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SDB_DATA_CHECKER_HPP_
#define SDB_DATA_CHECKER_HPP_

#include "core.hpp"
#include "../../../client/client.hpp"
#include "sdbOidToolUtil.hpp"
#include "sdbOidCatalog.hpp"
#include <vector>
#include <map>
#include <string>

using namespace std ;
using namespace sdbclient ;
using namespace bson ;

class NodeInfo
{
public:
   NodeInfo()
   {
      pConn = NULL ;
      isMaster = FALSE ;
   }
   ~NodeInfo() {}

public:
   sdb*     pConn ;
   string   hostName ;
   string   svcName ;
   BOOLEAN  isMaster ;
   string   groupName ;
} ;

class RecordSharding
{
public:
   RecordSharding( sdb* pConn ) ;
   ~RecordSharding() {} ;

   INT32 init() ;
   INT32 getGroupOfRecord( const CHAR *pBsonData, UINT32 &groupID ) ;
   INT32 getAllGroups( vector<UINT32> &list ) ;

private:
   InfoOptions*           _pOptions ;
   sdb*                   _pConn ;
   CatalogAgent           _cataAgent ;
   CataInfo               _cataInfo ;
} ;

class SdbOidChecker
{
public:
   SdbOidChecker() ;
   ~SdbOidChecker() ;

   INT32 init() ;
   INT32 run( BSONObj &result ) ;

private:
   void _fini() ;
   INT32 _initNodeInfo( UINT32 groupID, BOOLEAN isMaster, NodeInfo &node ) ;
   INT32 _connectToNode( NodeInfo &node ) ;
   INT32 _checkRecordOid( sdbCollection &cl, UINT32 groupID, BOOLEAN &hasError ) ;

private:
   InfoOptions*             _pOptions ;
   sdb*                     _pConn ;
   RecordSharding*          _pRecordSharding ;

   map<UINT32, NodeInfo>    _nodeMap ; // map of connection info of data node
} ;

#endif
