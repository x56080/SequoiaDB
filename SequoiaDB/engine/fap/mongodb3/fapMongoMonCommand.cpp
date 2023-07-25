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

   Source File Name = fapMongoMonCommand.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== ==============================================
          06/27/2023  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/

#include "fapMongoMonCommand.hpp"
#include "fapMongoTrace.hpp"
#include "pdTrace.hpp"
#include "ossFile.hpp"
#include "ossCmdRunner.hpp"
#include "utilSystem.hpp"

namespace fap
{
INT32 fapMongoParseCLInfo( engine::rtnContextBuf &bodyBuf, INT32 &collectionCount,
                           INT64 &objects, INT64 &avgObjSize, INT64 &dataSize,
                           INT64 &totalDataSize, INT32 &indexCount, INT64 &indexSize )
{
   INT32 rc = SDB_OK ;

   collectionCount = 0 ;
   objects = 0 ;
   avgObjSize = 0 ;
   dataSize = 0 ;
   totalDataSize = 0 ;
   indexCount = 0 ;
   indexSize = 0 ;

   bodyBuf.resetItr() ;

   try
   {
      while ( !bodyBuf.eof() )
      {
         INT64 pageSize = 0 ;
         INT64 totalDataPages = 0 ;
         INT64 totalIndexPages = 0 ;
         INT64 totalDataFreeSpaces = 0 ;
         BSONObj obj ;

         rc = bodyBuf.nextObj( obj ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get next obj from reply msg buff", rc ) ;

         {
         BSONObjIterator itr( obj ) ;
         while( itr.more() )
         {
            BSONElement ele = itr.next() ;
            const CHAR* fieldName = ele.fieldName() ;

            if ( 0 == ossStrcmp( fieldName, FIELD_NAME_NAME ) &&
                 ossStrlen( ele.valuestrsafe() ) > 0 )
            {
               collectionCount++ ;
            }
            else if ( 0 == ossStrcmp( fieldName, FIELD_NAME_PAGE_SIZE ) )
            {
               pageSize = ele.numberLong() ;
            }
            else if ( 0 == ossStrcmp( fieldName, FIELD_NAME_INDEXES ) )
            {
               indexCount += ele.numberInt() ;
            }
            else if ( 0 == ossStrcmp( fieldName, FIELD_NAME_TOTAL_RECORDS ) )
            {
               objects += ele.numberLong() ;
            }
            else if ( 0 == ossStrcmp( fieldName, FIELD_NAME_TOTAL_DATA_PAGES ) )
            {
               totalDataPages = ele.numberLong() ;
            }
            else if ( 0 == ossStrcmp( fieldName, FIELD_NAME_TOTAL_INDEX_PAGES ) )
            {
               totalIndexPages = ele.numberLong() ;
            }
            else if ( 0 == ossStrcmp( fieldName, FIELD_NAME_TOTAL_DATA_FREESPACE ) )
            {
               totalDataFreeSpaces = ele.numberLong() ;
            }
         }
         }

         totalDataSize += ( pageSize * totalDataPages ) ;
         dataSize += ( pageSize * totalDataPages - totalDataFreeSpaces ) ;
         indexSize += ( pageSize * totalIndexPages ) ;
      }

      if ( dataSize > 0 && objects > 0 )
      {
         avgObjSize = dataSize / objects ;
      }
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when parsing cl info: "
              "%s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoConnectionStatusCommand)
//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_CONSTATUSBUILDMONREPL, "_mongoConnectionStatusCommand::buildMongoReply" )
INT32 _mongoConnectionStatusCommand::buildMongoReply( const MsgOpReply &sdbReply,
                                                      engine::rtnContextBuf &bodyBuf,
                                                      _mongoResponseBuffer &headerBuf )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_CONSTATUSBUILDMONREPL ) ;
   INT32 rc = SDB_OK ;
   const CHAR* username = engine::sdbGetThreadExecutor()->getSession()->getClient()->getUsername() ;

   try
   {
      /*
      {
         "authInfo":
         {
            // now cs name is empty
            "authenticatedUsers": [ { "user": username, "db": csName } ],
            "authenticatedUserRoles": [],
            "authenticatedUserPrivileges": []
         },
         "ok": 1
      }
      */
      BSONObjBuilder bob ;
      BSONObjBuilder authInfoBob( bob.subobjStart( FAP_MONGO_FIELD_NAME_AUTH_INFO ) ) ;

      BSONArrayBuilder authUsersbab( authInfoBob.subarrayStart( FAP_MONGO_FIELD_NAME_AUTH_USERS ) ) ;
      if ( NULL != username && ossStrlen( username ) > 0 )
      {
         BSONObjBuilder userBob( authUsersbab.subobjStart() ) ;
         userBob.append( FAP_MONGO_FIELD_NAME_USER, username ) ;
         userBob.append( FAP_MONGO_FIELD_NAME_DB, "" ) ;
         userBob.done() ;
      }
      authUsersbab.done() ;

      BSONArrayBuilder authRoles( authInfoBob.subarrayStart( FAP_MONGO_FIELD_NAME_AUTH_USER_ROLES ) ) ;
      authRoles.done() ;

      BSONArrayBuilder authPri( authInfoBob.subarrayStart( FAP_MONGO_FIELD_NAME_AUTH_USER_PRI ) ) ;
      authPri.done() ;
      authInfoBob.done() ;

      bob.append( FAP_MONGO_FIELD_NAME_OK, 1 ) ;
      bodyBuf = engine::rtnContextBuf( bob.obj() ) ;
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when building mongo connectionStatus "
              "reply: %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

   rc = _buildReplyCommon( sdbReply, bodyBuf, headerBuf ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to build common reply, rc: %d", rc ) ;
      goto error ;
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_CONSTATUSBUILDMONREPL, rc ) ;
   return rc ;
error:
   goto done ;
}

MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoHostInfoCommand)
//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_HOSTINFOGETCPUINFO, "_mongoHostInfoCommand::_getCpuInfo" )
INT32 _mongoHostInfoCommand::_getCpuInfo( mongoHostInfo &hostInfo )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_HOSTINFOGETCPUINFO ) ;
   INT32 rc = SDB_OK ;
   INT32 processorCount = 0 ;
   INT32 physicalCount = 0 ;
   set< string > coreIDSet ;
   map< string, vector<engine::cpuInfo> > cpuInfos ;

   rc = engine::utilSysGetCpuInfo( cpuInfos ) ;
   PD_RC_CHECK( rc, PDERROR, "Failed to get cpu info, rc: %d", rc ) ;

   for ( auto itr1 = cpuInfos.begin() ; itr1 != cpuInfos.end() ; itr1++ )
   {
      string physicalID = itr1->first ;
      vector<engine::cpuInfo> infos = itr1->second ;

      for ( auto itr2 = infos.begin() ; itr2 != infos.end() ; itr2++ )
      {
         engine::cpuInfo info = *itr2 ;
         hostInfo.cpuFeatures = info.flags ;
         hostInfo.cpuFrequencyMHz = info.freq ;
         coreIDSet.insert( info.coreID ) ;
         processorCount++ ;
      }
      physicalCount++ ;
   }

   hostInfo.cpuCoreNum = processorCount ;
   hostInfo.cpuPhyCoreNum = (INT32)coreIDSet.size() ;
   hostInfo.cpuSocketNum = physicalCount ;

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_HOSTINFOGETCPUINFO, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_HOSTINFOGENESYSINFO, "_mongoHostInfoCommand::_generateSystemInfo" )
INT32 _mongoHostInfoCommand::_generateSystemInfo( mongoHostInfo &hostInfo )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_HOSTINFOGENESYSINFO ) ;
   INT32 rc = SDB_OK ;
   ossTimestamp tm ;
   INT32 memLoadPercent = 0 ;
   INT64 memTotalPhys   = 0 ;
   INT64 memFreePhys    = 0 ;
   INT64 memAvailPhys   = 0 ;
   INT64 memTotalPF     = 0 ;
   INT64 memAvailPF     = 0 ;
   INT64 memTotalVirtual= 0 ;
   INT64 memAvailVirtual= 0 ;
   UINT32 numaNodes = 0 ;

   // system info includes: currentTime, hostname, cpuAddrSize, memSizeMB, memLimitMB, numCores,
   // numPhysicalCores, numCpuSockets, cpuArch, numaEnabled and numNumaNodes
   ossGetCurrentTime( tm ) ;
   hostInfo.currentTime = tm.time ;

   rc = ossGetHostName( hostInfo.hostName, OSS_MAX_HOSTNAME ) ;
   PD_RC_CHECK( rc, PDERROR, "Failed to get hostname, rc: %d", rc ) ;

   hostInfo.cpuAddrSize = _osInfo._bit ;

   rc = ossGetMemoryInfo( memLoadPercent, memTotalPhys, memFreePhys,
                           memAvailPhys, memTotalPF, memAvailPF,
                           memTotalVirtual, memAvailVirtual ) ;
   PD_RC_CHECK( rc, PDERROR, "Failed to get memory info, rc: %d", rc ) ;
   hostInfo.memSizeMB = memTotalPhys / 1024 / 1024 ;

   hostInfo.memLimitMB = engine::utilSysGetLimitMem() / 1024 / 1024 ;

   try
   {
      hostInfo.cpuArch = _osInfo._arch ;
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when getting cpu arch: "
              "%s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

   rc = engine::utilSysCountNumaNodes( numaNodes ) ;
   PD_RC_CHECK( rc, PDERROR, "Failed to count numa nodes, rc: %d", rc ) ;

   if ( numaNodes > 0 )
   {
      hostInfo.numaEnable = TRUE ;
      hostInfo.numaNodes = numaNodes ;
   }
   else
   {
      hostInfo.numaEnable = FALSE ;
      hostInfo.numaNodes = 1 ;
   }

   rc = _getCpuInfo( hostInfo ) ;
   PD_RC_CHECK( rc, PDERROR, "Failed to get cpu info, rc: %d", rc ) ;

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_HOSTINFOGENESYSINFO, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_HOSTINFOGENEOSINFO, "_mongoHostInfoCommand::_generateOsInfo" )
INT32 _mongoHostInfoCommand::_generateOsInfo( mongoHostInfo &hostInfo )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_HOSTINFOGENEOSINFO ) ;
   INT32 rc = SDB_OK ;
   string distributor ;
   string release ;
   string description ;

   // os info includes type, name and version
   ossMemcpy( hostInfo.osType, FAP_OS_TYPE, sizeof( hostInfo.osType ) ) ;

   rc = engine::utilSysGetOsReleaseInfoFromCmd( distributor, release, description ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to get os release info from cmd, rc: %d", rc ) ;
      rc = SDB_OK ;

      rc = engine::utilSysGetOsReleaseInfoFromFile( distributor, release, description ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to get os release info from file, rc: %d", rc ) ;
         rc = SDB_OK ;

         hostInfo.osName = _osInfo._distributor ;
         hostInfo.osVersion = _osInfo._release ;
         goto done ;
      }
      else
      {
         hostInfo.osName = distributor ;
         hostInfo.osVersion = release ;
      }
   }
   else
   {
      hostInfo.osName = distributor ;
      hostInfo.osVersion = release ;
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_HOSTINFOGENEOSINFO, rc ) ;
   return rc ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_HOSTINFOGENEEXTRAINFO, "_mongoHostInfoCommand::_generateExtraInfo" )
INT32 _mongoHostInfoCommand::_generateExtraInfo( mongoHostInfo &hostInfo )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_HOSTINFOGENEEXTRAINFO ) ;
   INT32 rc = SDB_OK ;

   // extra info includes versionString, libcVersion, versionSignature, kernelVersion,
   // cpuFrequencyMHz, cpuFeatures, pageSize, numPages and maxOpenFiles
   rc = engine::utilSysGetOSVersion( hostInfo.version ) ;
   PD_RC_CHECK( rc, PDERROR, "Failed to get os version, rc: %d", rc ) ;

   rc = engine::utilSysGetLibcVersion( hostInfo.libcVersion ) ;
   PD_RC_CHECK( rc, PDERROR, "Failed to get libc version, rc: %d", rc ) ;

   rc = engine::utilSysGetOSVersionSignature( hostInfo.versionSignature ) ;
   PD_RC_CHECK( rc, PDERROR, "Failed to get os version signature, rc: %d", rc ) ;

   try
   {
      hostInfo.kernelVersion = _osInfo._release ;
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when getting kernel version: "
              "%s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

   hostInfo.pageSize = ossGetPageSize() ;
   hostInfo.pageNum = ossGetPageNum() ;
   hostInfo.maxOpenFiles = ossGetMaxOpenFiles() ;

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_HOSTINFOGENEEXTRAINFO, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_HOSTINFOGENEHOSTINFO, "_mongoHostInfoCommand::_generateMongoHostInfo" )
INT32 _mongoHostInfoCommand::_generateMongoHostInfo( mongoHostInfo &hostInfo )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_HOSTINFOGENEHOSTINFO ) ;
   INT32 rc = SDB_OK ;

   rc = ossGetOSInfo( _osInfo ) ;
   if ( SDB_OK != rc )
   {
      PD_LOG( PDERROR, "Failed to get os info, rc: %d", rc ) ;
      goto error;
   }

   rc = _generateSystemInfo( hostInfo ) ;
   PD_RC_CHECK( rc, PDERROR, "Failed to generate system info, rc: %d", rc ) ;

   rc = _generateOsInfo( hostInfo ) ;
   PD_RC_CHECK( rc, PDERROR, "Failed to generate os info, rc: %d", rc ) ;

   rc = _generateExtraInfo( hostInfo ) ;
   PD_RC_CHECK( rc, PDERROR, "Failed to generate extra info, rc: %d", rc ) ;

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_HOSTINFOGENEHOSTINFO, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_HOSTINFOBUILDMONREPL, "_mongoHostInfoCommand::buildMongoReply" )
INT32 _mongoHostInfoCommand::buildMongoReply( const MsgOpReply &sdbReply,
                                              engine::rtnContextBuf &bodyBuf,
                                              _mongoResponseBuffer &headerBuf )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_HOSTINFOBUILDMONREPL ) ;
   INT32 rc = SDB_OK ;
   mongoHostInfo info ;

   rc = _generateMongoHostInfo( info ) ;
   PD_RC_CHECK( rc, PDERROR, "Failed to generate mongo host info, rc: %d", rc ) ;

   try
   {
      /*
      {
         "system":
         {
            "currentTime": xxx,
            "hostname": xxx,
            "cpuAddrSize": xxx,
            "memSizeMB": xxx,
            "memLimitMB": xxx,
            "numCores": xxx,
            "numPhysicalCores": xxx,
            "numCpuSockets": xxx,
            "cpuArch": xxx,
            "numaEnabled": xxx,
            "numNumaNodes": xxx
         },
         "os":
         {
            "type": xxx,
            "name": xxx,
            "version": xxx
         },
         "extra":
         {
            "versionString": xxx,
            "libcVersion": xxx,
            "versionSignature": xxx,
            "kernelVersion": xxx,
            "cpuFrequencyMHz": xxx,
            "cpuFeatures": xxx,
            "pageSize": xxx,
            "numPages": xxx,
            "maxOpenFiles": xxx
         }
      }
      */
      BSONObjBuilder bob ;
      BSONObjBuilder sysBob( bob.subobjStart( FAP_MONGO_FIELD_NAME_SYSTEM ) ) ;
      sysBob.appendTimeT( FAP_MONGO_FIELD_NAME_CURRENT_TIME, info.currentTime ) ;
      sysBob.append( FAP_MONGO_FIELD_NAME_HOSTNAME, info.hostName ) ;
      sysBob.append( FAP_MONGO_FIELD_NAME_CPUADDRSIZE, info.cpuAddrSize ) ;
      sysBob.append( FAP_MONGO_FIELD_NAME_MEMSIZEMB, info.memSizeMB ) ;
      sysBob.append( FAP_MONGO_FIELD_NAME_MEMLIMITMB, info.memLimitMB ) ;
      sysBob.append( FAP_MONGO_FIELD_NAME_NUMCORES, info.cpuCoreNum ) ;
#if !defined (_ARMLIN64)
      sysBob.append( FAP_MONGO_FIELD_NAME_CPU_PHT_CORES, info.cpuPhyCoreNum ) ;
#endif
      sysBob.append( FAP_MONGO_FIELD_NAME_CPU_SOCKET_NUM, info.cpuSocketNum ) ;
      sysBob.append( FAP_MONGO_FIELD_NAME_CPUARCH, info.cpuArch ) ;
      sysBob.appendBool( FAP_MONGO_FIELD_NAME_NUMA_ENABLE, info.numaEnable ) ;
      sysBob.append( FAP_MONGO_FIELD_NAME_NUMA_NODES, info.numaNodes ) ;
      sysBob.done() ;

      BSONObjBuilder osBob( bob.subobjStart( FAP_MONGO_FIELD_NAME_OS ) ) ;
      osBob.append( FAP_MONGO_FIELD_NAME_TYPE, info.osType ) ;
      osBob.append( FAP_MONGO_FIELD_NAME_NAME, info.osName ) ;
      osBob.append( FAP_MONGO_FIELD_NAME_VER, info.osVersion ) ;
      osBob.done() ;

      BSONObjBuilder extraBob( bob.subobjStart( FAP_MONGO_FIELD_NAME_EXTRA ) ) ;
      extraBob.append( FAP_MONGO_FIELD_NAME_VERSION_STR, info.version.c_str() ) ;
      extraBob.append( FAP_MONGO_FIELD_NAME_LIBC_VERSION, info.libcVersion.c_str() ) ;
#if !defined (_ARMLIN64)
      extraBob.append( FAP_MONGO_FIELD_NAME_VER_SIG, info.versionSignature.c_str() ) ;
      extraBob.append( FAP_MONGO_FIELD_NAME_CPUFREMHZ, info.cpuFrequencyMHz.c_str() ) ;
      extraBob.append( FAP_MONGO_FIELD_NAME_CPUFEATURES, info.cpuFeatures.c_str() ) ;
#endif
      extraBob.append( FAP_MONGO_FIELD_NAME_KERNEL_VERSION, info.kernelVersion ) ;
      extraBob.append( FAP_MONGO_FIELD_NAME_PAGESIZE, info.pageSize ) ;
      extraBob.append( FAP_MONGO_FIELD_NAME_NUMPAGES, info.pageNum ) ;
      extraBob.append( FAP_MONGO_FIELD_NAME_MAX_OPENFILES, info.maxOpenFiles ) ;
      extraBob.done() ;

      bob.append( FAP_MONGO_FIELD_NAME_OK, 1 ) ;
      bodyBuf = engine::rtnContextBuf( bob.obj() ) ;
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when building mongo hostInfo "
              "reply: %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

   rc = _buildReplyCommon( sdbReply, bodyBuf, headerBuf ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to build common reply, rc: %d", rc ) ;
      goto error ;
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_HOSTINFOBUILDMONREPL, rc ) ;
   return rc ;
error:
   goto done ;
}

MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoDatabaseStatsCommand)
//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_DBSTATSBUILDSDBREQ, "_mongoDatabaseStatsCommand::buildSdbRequest" )
INT32 _mongoDatabaseStatsCommand::buildSdbRequest( mongoMsgBuffer &sdbMsg,
                                                   mongoSessionCtx &ctx,
                                                   BOOLEAN &getMoreAll )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_DBSTATSBUILDSDBREQ ) ;
   SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;
   INT32 rc = SDB_OK ;
   MsgOpSql *pSql = NULL ;
   StringBuilder buf ;
   std::string sql ;

   buf << "select T2.Name, first(T2.PageSize) as PageSize, first(T2.Indexes) as Indexes, "
          "sum(T2.TotalRecords) as TotalRecords, sum(T2.TotalDataPages) as TotalDataPages, "
          "sum(T2.TotalIndexPages) as TotalIndexPages, "
          "sum(T2.TotalDataFreeSpace) as TotalDataFreeSpace from "
          "( select T1.Name, T1.Details.Indexes as Indexes, T1.Details.PageSize as PageSize, "
          "T1.Details.TotalRecords as TotalRecords, T1.Details.TotalDataPages as TotalDataPages, "
          "T1.Details.TotalIndexPages as TotalIndexPages, "
          "T1.Details.TotalDataFreeSpace as TotalDataFreeSpace, "
          "T1.Details.TotalIndexFreeSpace as TotalIndexFreeSpace from "
          "( select * from $SNAPSHOT_CL where nodeselect = \"primary\" and "
          "CollectionSpace =\"" << _csName.c_str() <<
          "\" split by Details ) as T1 ) as T2 group by T2.Name" ;
   sql = buf.str() ;

   /*
      output str, eg:

      {
         "Name": "cs.cl",
         "PageSize": 65536,
         "Indexes": 2,
         "TotalRecords": 6,
         "TotalDataPages": 1,
         "TotalIndexPages": 4,
         "TotalDataFreeSpace": 65024
      }
      {
         "Name": "cs.cl_shard",
         "PageSize": 65536,
         "Indexes": 2,
         "TotalRecords": 6,
         "TotalDataPages": 1,
         "TotalIndexPages": 8,
         "TotalDataFreeSpace": 65212
      }

   */

   rc = sdbMsg.reserve( sizeof( MsgOpSql ) ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = sdbMsg.advance( sizeof( MsgOpSql ) ) ;
   if ( rc )
   {
      goto error ;
   }

   pSql = ( MsgOpSql * )sdbMsg.data() ;
   mongoInitMsgHeader( &(pSql->header), MSG_BS_SQL_REQ, _requestID ) ;

   rc = sdbMsg.write( sql.c_str(), sql.length() + 1, TRUE ) ;
   if ( rc )
   {
      goto error ;
   }

   sdbMsg.doneLen() ;

   getMoreAll = TRUE ;

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_DBSTATSBUILDSDBREQ, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_DBSTATSBUILDMONGOREPLY, "_mongoDatabaseStatsCommand::buildMongoReply" )
INT32 _mongoDatabaseStatsCommand::buildMongoReply( const MsgOpReply &sdbReply,
                                                   engine::rtnContextBuf &bodyBuf,
                                                   _mongoResponseBuffer &headerBuf )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_DBSTATSBUILDMONGOREPLY ) ;
   INT32 rc = SDB_OK ;
   BSONObjBuilder bob ;

   try
   {
      /*

      {
         "db": xxx,
         "collections": xxx,
         "objects": xxx,
         "avgObjSize": xxx,
         "dataSize": xxx,
         "storageSize": xxx,
         "indexes": xxx,
         "indexSize": xxx,
         "totalSize": xxx,
         "ok": 1
      }

      */
      if ( SDB_OK == sdbReply.flags )
      {
         INT32 collectionCount = 0 ;
         INT64 objects = 0 ;
         INT64 avgObjSize = 0 ;
         INT64 dataSize = 0 ;
         INT64 totalDataSize = 0 ;
         INT32 indexCount = 0 ;
         INT64 indexSize = 0 ;

         rc = fapMongoParseCLInfo( bodyBuf,  collectionCount, objects, avgObjSize,
                                   dataSize, totalDataSize, indexCount, indexSize ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse cl info, rc: %d", rc ) ;

         bob.append( FAP_MONGO_FIELD_NAME_DB, _csName.c_str() ) ;
         bob.append( FAP_MONGO_FIELD_NAME_COLLECTIONS, collectionCount ) ;
         bob.append( FAP_MONGO_FIELD_NAME_OBJECTS, objects ) ;
         bob.append( FAP_MONGO_FIELD_NAME_AVG_OBJ_SIZE, avgObjSize ) ;
         bob.append( FAP_MONGO_FIELD_NAME_DATA_SIZE, dataSize ) ;
         bob.append( FAP_MONGO_FIELD_NAME_STOR_SIZE, totalDataSize ) ;
         bob.append( FAP_MONGO_FIELD_NAME_IDX_NUM, indexCount ) ;
         bob.append( FAP_MONGO_FIELD_NAME_IDX_SIZE, indexSize ) ;
         bob.append( FAP_MONGO_FIELD_NAME_TOTAL_SIZE, totalDataSize + indexSize ) ;
         bob.append( FAP_MONGO_FIELD_NAME_OK, 1 ) ;
         bodyBuf = engine::rtnContextBuf( bob.obj() ) ;
      }
      else if ( SDB_DMS_EOC == sdbReply.flags )
      {
         bodyBuf = engine::rtnContextBuf( BSON( FAP_MONGO_FIELD_NAME_OK << 1 ) ) ;
      }
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when building mongo dbStats reply: "
              "%s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

   rc = _buildReplyCommon( sdbReply, bodyBuf, headerBuf ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to build common reply, rc: %d", rc ) ;
      goto error ;
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_DBSTATSBUILDMONGOREPLY, rc ) ;
   return rc ;
error:
   goto done ;
}

MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoCurrentOpCommand)

}
