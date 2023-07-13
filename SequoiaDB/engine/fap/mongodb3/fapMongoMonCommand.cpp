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

MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoCurrentOpCommand)

}
