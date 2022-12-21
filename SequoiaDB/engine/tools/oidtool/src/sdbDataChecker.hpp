/*******************************************************************************

   Copyright (C) 2011-2022 SequoiaDB Ltd.

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
