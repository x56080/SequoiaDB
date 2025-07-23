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

   Source File Name = sdbDataChecker.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who      Description
   ====== =========== ======== ==============================================
          2022/11/04  TZB      Initial Draft

   Last Changed =

*******************************************************************************/
#include "ossVer.h"
#include "msgDef.h"
#include "ossUtil.hpp"
#include "pd.hpp"
#include "sdbDataChecker.hpp"
#include <iostream>
#include <sstream>

using namespace engine ;
using namespace bson ;

namespace pt = boost::posix_time ;

/**
 * Implement of RecordSharding
 */
RecordSharding::RecordSharding( sdb* pConn )
{
   _pOptions = getInfoOptions() ;
   _pConn    = pConn ;
}

INT32 RecordSharding::init()
{
   INT32 rc = SDB_OK ;
   BSONObj cataObj ;
   sdbCursor cursor ;

   SDB_ASSERT( _pConn, "Conn can't be NULL" ) ;

   // init _cataAgent
   rc = _pConn->getSnapshot( cursor, SDB_SNAP_CATALOG ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to get snapshot[%d], rc = %d", SDB_SNAP_CATALOG, rc ) ;
      goto error ;
   }
   while( TRUE )
   {
      rc = cursor.next( cataObj ) ;
      if ( SDB_OK != rc )
      {
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_OK ;
            break ;
         }
         PD_LOG( PDERROR, "Failed to get record from cursor, rc = %d", rc ) ;
         goto error ;
      }

      rc = _cataAgent.updateCatalog( cataObj.objdata() ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to update catalog agent, rc=%d", rc ) ;
         goto error ;
      }
   }

   // init _cataInfo
   rc = _cataAgent.getCataInfo( _pOptions->srcCLFullName.c_str(), _cataInfo ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to get cataInfo for cl[%s], rc = %d",
              _pOptions->srcCLFullName.c_str(), rc ) ;
      goto error ;
   }

done:
   cursor.close() ;
   return rc ;
error:
   goto done ;
}

INT32 RecordSharding::getGroupOfRecord( const CHAR *pBsonData, UINT32 &groupID )
{
   INT32 rc = SDB_OK ;

   SDB_ASSERT( NULL != pBsonData, "Bson data can't be NULL" ) ;

   rc = _cataInfo.getGroupByRecord( pBsonData, groupID ) ;
   if ( SDB_OK != rc )
   {
      PD_LOG( PDERROR, "Failed to get group by record, rc=%d", rc ) ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 RecordSharding::getAllGroups( vector<UINT32> &grpIDList )
{
   return _cataInfo.getAllGroupID( grpIDList ) ;
}

/**
 * Implement of sdbOidChecker
 */
SdbOidChecker::SdbOidChecker()
{
   _pRecordSharding = NULL ;
   _pConn = NULL ;
   _pOptions = getInfoOptions() ;
}

SdbOidChecker::~SdbOidChecker()
{
   _fini() ;
}

INT32 SdbOidChecker::init()
{
   INT32 rc = SDB_OK ;
   vector<UINT32> grpIDList ;
   vector<UINT32>::iterator itr ;

   // create connection
   _pConn = new(nothrow) sdb() ;
   if ( NULL == _pConn )
   {
      rc = SDB_OOM ;
      PD_LOG( PDERROR, "Failed new connection, rc=%d", rc ) ;
      goto error;
   }
   rc = _pConn->connect( _pOptions->hostName.c_str(), _pOptions->svcName.c_str(),
                         _pOptions->user.c_str(), _pOptions->passwd.c_str() ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to connect to [%s:%s], rc=%d",
              _pOptions->hostName.c_str(), _pOptions->svcName.c_str(), rc ) ;
      goto error ;
   }

   // create RecordSharding
   _pRecordSharding = new(nothrow) RecordSharding( _pConn ) ;
   if ( NULL == _pRecordSharding )
   {
      rc = SDB_OOM ;
      PD_LOG( PDERROR, "Failed new collection assist, rc=%d", rc ) ;
      goto error;
   }
   rc = _pRecordSharding->init() ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to init assistant in cl[%s], rc=%d",
              _pOptions->srcCLFullName.c_str(), rc ) ;
      goto error ;
   }

   // get group id
   if ( _pOptions->checkAll )
   {
      rc = _pRecordSharding->getAllGroups( grpIDList ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to get all the groups of cl[%s], rc=%d",
                 _pOptions->srcCLFullName.c_str(), rc ) ;
         goto error ;
      }
   }
   else
   {
      UINT32 groupID = 0 ;
      BSONObj emptyObj ;
      rc = _pRecordSharding->getGroupOfRecord( emptyObj.objdata(), groupID ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to get group id of empty record in cl[%s], rc=%d",
                 _pOptions->srcCLFullName.c_str(), rc ) ;
         goto error ;
      }
      PD_LOG( PDEVENT, "Empty record in cl[%s] should be sharded to group[%d]",
              _pOptions->srcCLFullName.c_str(), groupID ) ;
      grpIDList.push_back( groupID ) ;
   }

   // init _nodeMap
   for ( itr = grpIDList.begin(); itr != grpIDList.end(); itr++ )
   {
      NodeInfo node ;
      UINT32 groupID = *itr ;
      rc = _initNodeInfo( groupID, TRUE, node ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to get conn by groupID[%d], rc=%d", groupID, rc ) ;
         goto error ;
      }
      rc = _connectToNode( node ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to get conn by groupID[%d], rc=%d", groupID, rc ) ;
         goto error ;
      }
      _nodeMap[groupID] = node ;
   }

done:
   return rc ;
error:
   _fini() ;
   goto done ;
}

INT32 SdbOidChecker::run( BSONObj &result )
{
   INT32 rc = SDB_OK ;
   BSONObjBuilder bob ;
   BOOLEAN hasError = FALSE ;
   const CHAR *pFullName = _pOptions->srcCLFullName.c_str() ;
   map<UINT32, NodeInfo>::iterator itr = _nodeMap.begin() ;

   for ( ; itr != _nodeMap.end(); itr++ )
   {
      sdbCollection cl ;
      sdb *pConn = itr->second.pConn ;
      rc = pConn->getCollection( pFullName, cl ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to get cl[%s], rc=%d", pFullName, rc ) ;
         goto  error ;
      }
      hasError = FALSE ;
      rc = _checkRecordOid( cl, itr->first, hasError ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to check record oid in cl [%s], rc=%d", pFullName, rc ) ;
         goto  error ;
      }
      if ( hasError )
      {
         PD_LOG( PDEVENT, "cl[%s] has abnormal record",
                 _pOptions->srcCLFullName.c_str() ) ;
         goto done ;
      }
   }

done:
   // build result obj
   bob.append( "ErrNo", rc ) ;
   bob.append( "Action", _pOptions->action.c_str() ) ;
   bob.append( "Name", pFullName ) ;
   bob.appendBool( "HasAbnormalRecord", hasError ) ;
   result = bob.obj() ;
   return rc ;
error:
   goto done ;
}

INT32 SdbOidChecker::_initNodeInfo( UINT32 groupID, BOOLEAN isMaster, NodeInfo &node )
{
   INT32 rc = SDB_OK ;
   sdbCursor cursor ;
   BSONObj groupInfo ;
   UINT32 retRgID = 0 ;

   // list group
   rc = _pConn->getList( cursor, SDB_LIST_GROUPS, BSON( "GroupID" << groupID ) ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to get node info of group[%d], rc=%d", groupID, rc ) ;
      goto error ;
   }
   rc = cursor.next( groupInfo ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to get record of group[%d], rc=%d", groupID, rc ) ;
      goto error ;
   }

/*
groupInfo is as below:
{
  "Group": [
    {
      "HostName": "u1604-tzb",
      "Status": 1,
      "dbpath": "/data/sequoiadb/database2/60000/",
      "Service": [
        {
          "Type": 0,
          "Name": "60000"
        },
        {
          "Type": 1,
          "Name": "60001"
        },
        {
          "Type": 2,
          "Name": "60002"
        }
      ],
      "NodeID": 1004
    }
  ],
  "GroupID": 1002,
  "GroupName": "db3",
  "PrimaryNode": 1004,
  "Role": 0,
  "SecretID": 1734643548,
  "Status": 1,
  "Version": 3,
  "_id": {
    "$oid": "635b8ace3efa680df9021826"
  }
}
*/
   // check the return record
   retRgID = (UINT32)groupInfo.getIntField( FIELD_NAME_GROUPID ) ;
   if ( groupID != retRgID )
   {
      rc = SDB_SYS ;
      PD_LOG( PDERROR, "The return group id[%d] is not the expected one[%d], rc=%d",
              retRgID, groupID, rc ) ;
      goto error ;
   }

   // get hostname and svcname
   try
   {
      const INT32 primaryNodeID = groupInfo.getIntField( FIELD_NAME_PRIMARY ) ;
      BSONElement grpEle = groupInfo.getField( FIELD_NAME_GROUP ) ;
      BSONObjIterator itr( grpEle.embeddedObject() ) ;
      while ( itr.more() )
      {
         BSONObj nodeObj = itr.next().embeddedObject() ;
         const CHAR *hostName = nodeObj.getStringField( FIELD_NAME_HOST ) ;
         const CHAR *svcName = NULL ;
         const INT32 nodeID = nodeObj.getIntField( FIELD_NAME_NODEID ) ;
         BSONElement svcEle = nodeObj.getField( FIELD_NAME_SERVICE ) ;
         BSONObjIterator subItr( svcEle.embeddedObject() ) ;
         while ( subItr.more() )
         {
            BSONObj svcObj = subItr.next().embeddedObject() ;
            if ( 0 == svcObj.getIntField( FIELD_NAME_SERVICE_TYPE ) )
            {
               svcName = svcObj.getStringField( FIELD_NAME_NAME ) ;
               break ;
            }
         }
         if ( isMaster && primaryNodeID != nodeID )
         {
            continue ;
         }
         else
         {
            node.hostName = hostName ;
            node.svcName = svcName ;
            node.isMaster = isMaster ;
            node.groupName = groupInfo.getField( FIELD_NAME_GROUPNAME ) ;
            break ;
         }
      }
   }
   catch ( std::exception &e )
   {
      rc = SDB_SYS ;
      PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
      goto error ;
   }

done:
   cursor.close() ;
   return rc ;
error:
   goto done ;
}

INT32 SdbOidChecker::_connectToNode( NodeInfo &node )
{
   INT32 rc = SDB_OK ;
   sdb *pConn = NULL ;

   if ( NULL != node.pConn )
   {
      delete node.pConn ;
      node.pConn = NULL ;
   }

   pConn = new sdb() ;
   if ( NULL == pConn )
   {
      rc = SDB_OOM ;
      goto error ;
   }

   rc = pConn->connect( node.hostName.c_str(), node.svcName.c_str(),
                        _pOptions->user.c_str(), _pOptions->passwd.c_str() ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to connect to node[%s:%s], rc = %d",
              node.hostName.c_str(), node.svcName.c_str(), rc ) ;
      goto error ;
   }

   node.pConn = pConn ;

done:
   return rc ;
error:
   if ( pConn )
   {
      delete pConn ;
   }
   goto done ;
}

INT32 SdbOidChecker::_checkRecordOid( sdbCollection &cl, UINT32 groupID, BOOLEAN &hasError )
{
   INT32 rc = SDB_OK ;
   sdbCursor cursor ;
   BSONObj record ;

   // get records
   rc = cl.query( cursor ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to query in cl[%s], rc=%d",
              _pOptions->srcCLFullName.c_str(), rc ) ;
      goto error ;
   }

   // check the records
   while( TRUE )
   {
      UINT32 grpID = 0 ;
      // get record
      rc = cursor.next( record ) ;
      if ( SDB_OK != rc )
      {
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_OK ;
            break ;
         }
         PD_LOG( PDERROR, "Failed to get record from cursor, rc = %d", rc ) ;
         goto error ;
      }
      // get the group the record should be in
      rc =  _pRecordSharding->getGroupOfRecord( record.objdata(), grpID ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to get group id of record in cl[%s], rc=%d",
                 _pOptions->srcCLFullName.c_str(), rc ) ;
         goto error ;
      }
      // checking
      if ( groupID != grpID )
      {
         PD_LOG( PDEVENT, "Record[%s] in cl[%s] is sharded to group[%d],"
                 " but it should be in group[%d]",
                 record.toString( false, true, false ).c_str(),
                 _pOptions->srcCLFullName.c_str(), groupID, grpID ) ;
         hasError = TRUE ;
         goto done ;
      }
   }
   hasError= FALSE ;

done:
   cursor.close() ;
   return rc ;
error:
   goto done ;
}

void SdbOidChecker::_fini()
{
   if ( _pConn )
   {
      _pConn->disconnect() ;
      delete _pConn ;
      _pConn = NULL ;
   }
   if ( _pRecordSharding )
   {
      delete _pRecordSharding ;
      _pRecordSharding = NULL ;
   }
   map<UINT32, NodeInfo>::iterator itr = _nodeMap.begin() ;
   for (; itr != _nodeMap.end(); itr++ )
   {
      NodeInfo &node = itr->second ;
      node.pConn->disconnect() ;
      delete node.pConn ;
   }
   _nodeMap.clear() ;
}
