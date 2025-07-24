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

   Source File Name = sptUsrSystem.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "sptUsrSystem.hpp"
#include "ossCmdRunner.hpp"
#include "ossUtil.hpp"
#include "utilStr.hpp"
#include "ossSocket.hpp"
#include "ossIO.hpp"
#include "oss.h"
#include "ossPath.hpp"
#include "ossPrimitiveFileOp.hpp"
#include <set>
#include <vector>
#include <utility>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#if defined (_LINUX)
#include <net/if.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <pwd.h>
#else
#include <iphlpapi.h>
#pragma comment( lib, "IPHLPAPI.lib" )
#endif

using namespace bson ;
using std::pair ;

namespace engine
{
   JS_CONSTRUCT_FUNC_DEFINE( _sptUsrSystem, construct )
   JS_DESTRUCT_FUNC_DEFINE( _sptUsrSystem, destruct )
   JS_MEMBER_FUNC_DEFINE( _sptUsrSystem, getInfo )
   JS_MEMBER_FUNC_DEFINE( _sptUsrSystem, memberHelp )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, getObj )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, ping )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, type )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, getReleaseInfo )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, getHostsMap )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, getAHostMap )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, addAHostMap )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, delAHostMap )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, getCpuInfo )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, snapshotCpuInfo )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, getMemInfo )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, snapshotMemInfo )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, getDiskInfo )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, snapshotDiskInfo )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, getNetcardInfo )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, snapshotNetcardInfo )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, getIpTablesInfo )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, getHostName )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, sniffPort )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, getPID )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, getTID )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, getEWD )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, listProcess )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, killProcess )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, addUser )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, addGroup )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, setUserConfigs )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, delUser )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, delGroup )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, listLoginUsers )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, listAllUsers )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, listGroups )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, getCurrentUser )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, getSystemConfigs )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, getProcUlimitConfigs )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, setProcUlimitConfigs )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, runService )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, createSshKey )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, getHomePath )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, getUserEnv )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, staticHelp )

   JS_BEGIN_MAPPING( _sptUsrSystem, "System" )
      JS_ADD_CONSTRUCT_FUNC( construct )
      JS_ADD_DESTRUCT_FUNC( destruct )
      JS_ADD_MEMBER_FUNC_WITHATTR( "_getInfo", getInfo, 0 )
      JS_ADD_MEMBER_FUNC( "help", memberHelp )
      JS_ADD_STATIC_FUNC( "getObj", getObj )
      JS_ADD_STATIC_FUNC( "ping", ping )
      JS_ADD_STATIC_FUNC( "type", type )
      JS_ADD_STATIC_FUNC( "getReleaseInfo", getReleaseInfo )
      JS_ADD_STATIC_FUNC( "getHostsMap", getHostsMap )
      JS_ADD_STATIC_FUNC( "getAHostMap", getAHostMap )
      JS_ADD_STATIC_FUNC( "addAHostMap", addAHostMap )
      JS_ADD_STATIC_FUNC( "delAHostMap", delAHostMap )
      JS_ADD_STATIC_FUNC( "getCpuInfo", getCpuInfo )
      JS_ADD_STATIC_FUNC( "snapshotCpuInfo", snapshotCpuInfo )
      JS_ADD_STATIC_FUNC( "getMemInfo", getMemInfo )
      JS_ADD_STATIC_FUNC( "snapshotMemInfo", snapshotMemInfo )
      JS_ADD_STATIC_FUNC( "getDiskInfo", getDiskInfo )
      JS_ADD_STATIC_FUNC( "snapshotDiskInfo", snapshotDiskInfo )
      JS_ADD_STATIC_FUNC( "getNetcardInfo", getNetcardInfo )
      JS_ADD_STATIC_FUNC( "snapshotNetcardInfo", snapshotNetcardInfo )
      JS_ADD_STATIC_FUNC( "getIpTablesInfo", getIpTablesInfo )
      JS_ADD_STATIC_FUNC( "getHostName", getHostName )
      JS_ADD_STATIC_FUNC( "sniffPort", sniffPort )
      JS_ADD_STATIC_FUNC( "getPID", getPID )
      JS_ADD_STATIC_FUNC( "getTID", getTID )
      JS_ADD_STATIC_FUNC( "getEWD", getEWD )
      JS_ADD_STATIC_FUNC_WITHATTR( "_listProcess", listProcess, 0 )
      JS_ADD_STATIC_FUNC( "killProcess", killProcess )
      JS_ADD_STATIC_FUNC( "addUser", addUser )
      JS_ADD_STATIC_FUNC( "addGroup", addGroup )
      JS_ADD_STATIC_FUNC( "setUserConfigs", setUserConfigs )
      JS_ADD_STATIC_FUNC( "delUser", delUser )
      JS_ADD_STATIC_FUNC( "delGroup", delGroup )
      JS_ADD_STATIC_FUNC_WITHATTR( "_listLoginUsers", listLoginUsers, 0 )
      JS_ADD_STATIC_FUNC_WITHATTR( "_listAllUsers", listAllUsers, 0 )
      JS_ADD_STATIC_FUNC_WITHATTR( "_listGroups", listGroups, 0 )
      JS_ADD_STATIC_FUNC( "getCurrentUser", getCurrentUser )
      JS_ADD_STATIC_FUNC( "getSystemConfigs", getSystemConfigs )
      JS_ADD_STATIC_FUNC( "getProcUlimitConfigs", getProcUlimitConfigs )
      JS_ADD_STATIC_FUNC( "setProcUlimitConfigs", setProcUlimitConfigs )
      JS_ADD_STATIC_FUNC( "runService", runService )
      JS_ADD_STATIC_FUNC( "getUserEnv", getUserEnv )
      JS_ADD_STATIC_FUNC_WITHATTR( "_createSshKey", createSshKey, 0 )
      JS_ADD_STATIC_FUNC_WITHATTR( "_getHomePath", getHomePath, 0 )
      JS_ADD_STATIC_FUNC( "help", staticHelp )
   JS_MAPPING_END()

   _sptUsrSystem::_sptUsrSystem()
   {
   }

   _sptUsrSystem::~_sptUsrSystem()
   {
   }

   INT32 _sptUsrSystem::getObj( const _sptArguments &arg,
                                _sptReturnVal &rval,
                                BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      _sptUsrSystem * systemObj = _sptUsrSystem::crtInstance() ;
      if ( !systemObj )
      {
         rc = SDB_OOM ;
         detail = BSON( SPT_ERR << "Create object failed" ) ;
      }
      else
      {
         rval.setUsrObjectVal<_sptUsrSystem>( systemObj ) ;
      }
      return rc ;
   }

   INT32 _sptUsrSystem::construct( const _sptArguments & arg,
                                   _sptReturnVal & rval,
                                   BSONObj & detail )
   {
      detail = BSON( SPT_ERR << "Please get System Obj by calling Remote "
                                "member function: getSystem()" ) ;
      return SDB_SYS ;
   }

   INT32 _sptUsrSystem::destruct()
   {
      return SDB_OK ;
   }

   INT32 _sptUsrSystem::getInfo( const _sptArguments &arg,
                                 _sptReturnVal &rval,
                                 bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BSONObj remoteInfo ;
      BSONObjBuilder builder ;

      if ( 0 < arg.argc() )
      {
         rc = arg.getBsonobj( 0, remoteInfo ) ;
         if ( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "remoteInfo must be obj" ) ;
            goto error ;
         }
      }

      builder.append( "type", "System" ) ;
      builder.appendElements( remoteInfo ) ;
      rval.getReturnVal().setValue( builder.obj() ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrSystem::ping( const _sptArguments &arg,
                              _sptReturnVal &rval,
                              bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder builder ;
      string host ;
      stringstream cmd ;
      _ossCmdRunner runner ;
      UINT32 exitCode = 0 ;

      rc = arg.getString( 0, host ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "hostname must be config" ) ;
      }
      else if ( rc )
      {
         detail = BSON( SPT_ERR << "hostname must be string" ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get hostname, rc: %d", rc ) ;

#if defined (_LINUX)
      cmd << "ping " << " -q -c 1 "  << "\"" << host << "\"" ;
#elif defined (_WINDOWS)
      cmd << "ping -n 2 -w 1000 " << "\"" << host << "\"" ;
#endif

      rc = runner.exec( cmd.str().c_str(), exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to exec cmd, rc:%d, exit:%d",
                 rc, exitCode ) ;
         if ( SDB_OK == rc )
         {
            rc = SDB_SYS ;
         }
         stringstream ss ;
         ss << "failed to exec cmd \"ping\",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }

      builder.append( CMD_USR_SYSTEM_TARGET, host ) ;
      builder.appendBool( CMD_USR_SYSTEM_REACHABLE, SDB_OK == exitCode ) ;
      rval.getReturnVal().setValue( builder.obj() ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrSystem::type( const _sptArguments &arg,
                              _sptReturnVal &rval,
                              bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      if ( 0 < arg.argc() )
      {
         PD_LOG( PDERROR, "type() should have non arguments" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

#if defined (_LINUX)
      rval.getReturnVal().setValue( "LINUX") ;
#elif defined (_WINDOWS)
      rval.getReturnVal().setValue( "WINDOWS" ) ;
#endif
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrSystem::getReleaseInfo( const _sptArguments &arg,
                                        _sptReturnVal &rval,
                                        bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      UINT32 exitCode = 0 ;
      _ossCmdRunner runner ;
      string outStr ;
      BSONObjBuilder builder ;

      if ( 0 < arg.argc() )
      {
         PD_LOG( PDERROR, "getReleaseInfo() should have non arguments" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

#if defined (_LINUX)
      rc = runner.exec( "lsb_release -a |grep -v \"LSB Version\"", exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
#elif defined (_WINDOWS)
      rc = SDB_SYS ;
#endif
      if ( SDB_OK != rc || SDB_OK != exitCode )
      {
         rc = SDB_OK ;
         ossOSInfo info ;
         ossGetOSInfo( info ) ;

         builder.append( CMD_USR_SYSTEM_DISTRIBUTOR, info._distributor ) ;
         builder.append( CMD_USR_SYSTEM_RELASE, info._release ) ;
         builder.append( CMD_USR_SYSTEM_DESP, info._desp ) ;
         builder.append( CMD_USR_SYSTEM_BIT, info._bit ) ;

         rval.getReturnVal().setValue( builder.obj() ) ;
         goto done ;
      }

      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to read msg from cmd runner:%d", rc ) ;
         stringstream ss ;
         ss << "failed to read msg from cmd \"lsb_release -a\", rc:"
            << rc ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }

      rc = _extractReleaseInfo( outStr.c_str(), builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to read msg from cmd runner:%d", rc ) ;
         stringstream ss ;
         ss << "failed to extract info from release info:"
            << outStr ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }

      outStr = "" ;
#if defined (_LINUX)
      rc = runner.exec( "getconf LONG_BIT", exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
#elif defined (_WINDOWS)
      rc = SDB_SYS ;
#endif
      if ( SDB_OK != rc || SDB_OK != exitCode )
      {
         PD_LOG( PDERROR, "failed to exec cmd, rc:%d, exit:%d",
                 rc, exitCode ) ;
         if ( SDB_OK == rc )
         {
            rc = SDB_SYS ;
         }
         stringstream ss ;
         ss << "failed to exec cmd \"getconf LONG_BIT\", rc:"
            << rc
            << ",exit:"
            << exitCode ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }

      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to read msg from cmd runner:%d", rc ) ;
         stringstream ss ;
         ss << "failed to read msg from cmd \"getconf LONG_BIT\", rc:"
            << rc ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }

      if ( NULL != ossStrstr( outStr.c_str(), "64") )
      {
         builder.append( CMD_USR_SYSTEM_BIT, 64 ) ;
      }
      else
      {
         builder.append( CMD_USR_SYSTEM_BIT, 32 ) ;
      }
      rval.getReturnVal().setValue( builder.obj() ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrSystem::_extractReleaseInfo( const CHAR *buf,
                                             bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      vector<string> splited ;
      const string *distributor = NULL ;
      const string *release = NULL ;
      const string *desp = NULL ;

      /// not performance sensitive.
      try
      {
         boost::algorithm::split( splited, buf, boost::is_any_of("\n:") ) ;
      }
      catch( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                 rc, e.what() ) ;
         goto error ;
      }
      for ( vector<string>::iterator itr = splited.begin();
            itr != splited.end(); itr++ )
      {
         if ( itr->empty() )
         {
            continue ;
         }
         try
         {
            boost::algorithm::trim( *itr ) ;
         }
         catch( std::exception &e )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Failed to trim ,rc: %d, detail: %s",
                    rc, e.what() ) ;
            goto error ;
         }
         if ( "Distributor ID" == *itr &&
              itr < splited.end() - 1 )
         {
            distributor = &( *( itr + 1 ) ) ;
         }
         else if ( "Release" == *itr &&
                   itr < splited.end() - 1 )
         {
            release = &( *( itr + 1 ) ) ;
         }
         else if ( "Description" == *itr &&
                   itr < splited.end() - 1 )
         {
            desp = &( *( itr + 1 ) ) ;
         }
      }

      if ( NULL == distributor ||
           NULL == release )
      {
         PD_LOG( PDERROR, "failed to split release info:%s",
                 buf )  ;
         rc = SDB_SYS ;
         goto error ;
      }

      builder.append( CMD_USR_SYSTEM_DISTRIBUTOR, *distributor ) ;
      builder.append( CMD_USR_SYSTEM_RELASE, *release ) ;
      builder.append( CMD_USR_SYSTEM_DESP, *desp ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrSystem::getHostsMap( const _sptArguments &arg,
                                     _sptReturnVal &rval,
                                     bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder builder ;
      string err ;
      VEC_HOST_ITEM vecItems ;

      if ( 0 < arg.argc() )
      {
         err = "getHostsMap() should have non arguments" ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = _parseHostsFile( vecItems, err ) ;
      if ( rc )
      {
         goto error ;
      }

      _buildHostsResult( vecItems, builder ) ;
      rval.getReturnVal().setValue( builder.obj() ) ;

   done:
      return rc ;
   error:
      detail = BSON( SPT_ERR << err ) ;
      goto done ;
   }

   INT32 _sptUsrSystem::getAHostMap( const _sptArguments & arg,
                                     _sptReturnVal & rval,
                                     BSONObj & detail )
   {
      INT32 rc = SDB_OK ;
      string hostname ;
      string err ;
      VEC_HOST_ITEM vecItems ;

      rc = arg.getString( 0, hostname ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         err = "hostname must config" ;
         goto error ;
      }
      else if ( rc )
      {
         err = "hostname must be string" ;
         goto error ;
      }
      else if ( hostname.empty() )
      {
         rc = SDB_INVALIDARG ;
         err = "hostname can't be empty" ;
         goto error ;
      }

      rc = _parseHostsFile( vecItems, err ) ;
      if ( rc )
      {
         goto error ;
      }
      else
      {
         VEC_HOST_ITEM::iterator it = vecItems.begin() ;
         while ( it != vecItems.end() )
         {
            usrSystemHostItem &item = *it ;
            ++it ;
            if( LINE_HOST == item._lineType && hostname == item._host )
            {
               rval.getReturnVal().setValue( item._ip ) ;
               goto done ;
            }
         }
         err = "hostname not exist" ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      detail = BSON( SPT_ERR << err ) ;
      goto done ;
   }

   INT32 _sptUsrSystem::addAHostMap( const _sptArguments & arg,
                                     _sptReturnVal & rval,
                                     BSONObj & detail )
   {
      INT32 rc = SDB_OK ;
      string hostname ;
      string ip ;
      INT32  isReplace = 1 ;
      string err ;
      VEC_HOST_ITEM vecItems ;

      // hostname
      rc = arg.getString( 0, hostname ) ;
      if ( rc == SDB_OUT_OF_BOUND )
      {
         err = "hostname must be config" ;
         goto error ;
      }
      else if ( rc )
      {
         err = "hostname must be string" ;
         goto error ;
      }
      else if ( hostname.empty() )
      {
         rc = SDB_INVALIDARG ;
         err = "hostname can't be empty" ;
         goto error ;
      }

      // ip
      rc = arg.getString( 1, ip ) ;
      if ( rc == SDB_OUT_OF_BOUND )
      {
         err = "ip must be config" ;
         goto error ;
      }
      else if ( rc )
      {
         err = "ip must be string" ;
         goto error ;
      }
      else if ( ip.empty() )
      {
         rc = SDB_INVALIDARG ;
         err = "ip can't be empty" ;
         goto error ;
      }
      else if ( !isValidIPV4( ip.c_str() ) )
      {
         rc = SDB_INVALIDARG ;
         err = "ip is not ipv4" ;
         goto error ;
      }

      // isReplace
      if ( arg.argc() > 2 )
      {
         rc = arg.getNative( 2, (void*)&isReplace, SPT_NATIVE_INT32 ) ;
         if ( rc )
         {
            err = "isReplace must be BOOLEAN" ;
            goto error ;
        }
      }

      rc = _parseHostsFile( vecItems, err ) ;
      if ( rc )
      {
         goto error ;
      }
      else
      {
         VEC_HOST_ITEM::iterator it = vecItems.begin() ;
         BOOLEAN hasMod = FALSE ;
         while ( it != vecItems.end() )
         {
            usrSystemHostItem &item = *it ;
            ++it ;
            if( item._lineType == LINE_HOST && hostname == item._host )
            {
               if ( item._ip == ip )
               {
                  goto done ;
               }
               else if ( !isReplace )
               {
                  err = "hostname already exist" ;
                  rc = SDB_INVALIDARG ;
                  goto error ;
               }
               item._ip = ip ;
               hasMod = TRUE ;
            }
         }
         if ( !hasMod )
         {
            usrSystemHostItem info ;
            info._lineType = LINE_HOST ;
            info._host = hostname ;
            info._ip = ip ;
            vecItems.push_back( info ) ;
         }
         // write
         rc = _writeHostsFile( vecItems, err ) ;
         if ( rc )
         {
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      detail = BSON( SPT_ERR << err ) ;
      goto done ;
   }

   INT32 _sptUsrSystem::delAHostMap( const _sptArguments & arg,
                                     _sptReturnVal & rval,
                                     BSONObj & detail )
   {
      INT32 rc = SDB_OK ;
      string hostname ;
      string err ;
      VEC_HOST_ITEM vecItems ;

      // hostname
      rc = arg.getString( 0, hostname ) ;
      if ( rc == SDB_OUT_OF_BOUND )
      {
         err = "hostname must be config" ;
         goto error ;
      }
      else if ( rc )
      {
         err = "hostname must be string" ;
         goto error ;
      }
      else if ( hostname.empty() )
      {
         rc = SDB_INVALIDARG ;
         err = "hostname can't be empty" ;
         goto error ;
      }

      rc = _parseHostsFile( vecItems, err ) ;
      if ( rc )
      {
         goto error ;
      }
      else
      {
         VEC_HOST_ITEM::iterator it = vecItems.begin() ;
         BOOLEAN hasDel = FALSE ;
         while ( it != vecItems.end() )
         {
            usrSystemHostItem &item = *it ;
            if( item._lineType == LINE_HOST && hostname == item._host )
            {
               // del
               it = vecItems.erase( it ) ;
               hasDel = TRUE ;
               continue ;
            }
            ++it ;
         }
         // write
         if ( hasDel )
         {
            rc = _writeHostsFile( vecItems, err ) ;
            if ( rc )
            {
               goto error ;
            }
         }
      }
   done:
      return rc ;
   error:
      detail = BSON( SPT_ERR << err ) ;
      goto done ;
   }

#if defined( _LINUX )
   #define HOSTS_FILE      "/etc/hosts"
#else
   #define HOSTS_FILE      "C:\\Windows\\System32\\drivers\\etc\\hosts"
#endif // _LINUX

   INT32 _sptUsrSystem::_parseHostsFile( VEC_HOST_ITEM & vecItems,
                                         string &err )
   {
      INT32 rc = SDB_OK ;
      OSSFILE file ;
      stringstream ss ;
      BOOLEAN isOpen = FALSE ;
      INT64 fileSize = 0 ;
      CHAR *pBuff = NULL ;
      INT64 hasRead = 0 ;

      rc = ossGetFileSizeByName( HOSTS_FILE, &fileSize ) ;
      if ( rc )
      {
         ss << "get file[" << HOSTS_FILE << "] size failed: " << rc ;
         goto error ;
      }
      pBuff = ( CHAR* )SDB_OSS_MALLOC( fileSize + 1 ) ;
      if ( !pBuff )
      {
         ss << "alloc memory[" << fileSize << "] failed" ;
         rc = SDB_OOM ;
         goto error ;
      }
      ossMemset( pBuff, 0, fileSize + 1 ) ;

      rc = ossOpen( HOSTS_FILE, OSS_READONLY|OSS_SHAREREAD, 0,
                    file ) ;
      if ( rc )
      {
         ss << "open file[" << HOSTS_FILE << "] failed: " << rc ;
         goto error ;
      }
      isOpen = TRUE ;

      // read file
      rc = ossReadN( &file, fileSize, pBuff, hasRead ) ;
      if ( rc )
      {
         ss << "read file[" << HOSTS_FILE << "] failed: " << rc ;
         goto error ;
      }
      ossClose( file ) ;
      isOpen = FALSE ;

      rc = _extractHosts( pBuff, vecItems ) ;
      if ( rc )
      {
         ss << "extract hosts failed: " << rc ;
         goto error ;
      }

      // remove last empty
      if ( vecItems.size() > 0 )
      {
         VEC_HOST_ITEM::iterator itr = vecItems.end() - 1 ;
         usrSystemHostItem &info = *itr ;
         if ( info.toString().empty() )
         {
            vecItems.erase( itr ) ;
         }
      }

   done:
      if ( isOpen )
      {
         ossClose( file ) ;
      }
      if ( pBuff )
      {
         SDB_OSS_FREE( pBuff ) ;
      }
      return rc ;
   error:
      err = ss.str() ;
      goto done ;
   }

   INT32 _sptUsrSystem::_writeHostsFile( VEC_HOST_ITEM & vecItems,
                                         string & err )
   {
      INT32 rc = SDB_OK ;
      std::string tmpFile = HOSTS_FILE ;
      tmpFile += ".tmp" ;
      OSSFILE file ;
      BOOLEAN isOpen = FALSE ;
      BOOLEAN isBak = FALSE ;
      stringstream ss ;

      if ( SDB_OK == ossAccess( tmpFile.c_str() ) )
      {
         ossDelete( tmpFile.c_str() ) ;
      }

      // 1. first back up the file
      if ( SDB_OK == ossAccess( HOSTS_FILE ) )
      {
         if ( SDB_OK == ossRenamePath( HOSTS_FILE, tmpFile.c_str() ) )
         {
            isBak = TRUE ;
         }
      }

      // 2. Create the file
      rc = ossOpen ( HOSTS_FILE, OSS_READWRITE|OSS_SHAREWRITE|OSS_REPLACE,
                     OSS_RU|OSS_WU|OSS_RG|OSS_RO, file ) ;
      if ( rc )
      {
         ss << "open file[" <<  HOSTS_FILE << "] failed: " << rc ;
         goto error ;
      }
      isOpen = TRUE ;

      // 3. write data
      {
         VEC_HOST_ITEM::iterator it = vecItems.begin() ;
         UINT32 count = 0 ;
         while ( it != vecItems.end() )
         {
            ++count ;
            usrSystemHostItem &item = *it ;
            ++it ;
            string text = item.toString() ;
            if ( !text.empty() || count < vecItems.size() )
            {
               text += OSS_NEWLINE ;
            }
            rc = ossWriteN( &file, text.c_str(), text.length() ) ;
            if ( rc )
            {
               ss << "write context[" << text << "] to file[" << HOSTS_FILE
                  << "] failed: " << rc ;
               goto error ;
            }
         }
      }

      // 4. remove tmp
      if ( SDB_OK == ossAccess( tmpFile.c_str() ) )
      {
         ossDelete( tmpFile.c_str() ) ;
      }

   done:
      if ( isOpen )
      {
         ossClose( file ) ;
      }
      return rc ;
   error:
      if ( isBak )
      {
         if ( isOpen )
         {
            ossClose( file ) ;
            isOpen = FALSE ;
            ossDelete( HOSTS_FILE ) ;
         }
         ossRenamePath( tmpFile.c_str(), HOSTS_FILE ) ;
      }
      err = ss.str() ;
      goto done ;
   }

   INT32 _sptUsrSystem::_extractHosts( const CHAR *buf,
                                       VEC_HOST_ITEM &vecItems )
   {
      INT32 rc = SDB_OK ;
      vector<string> splited ;

      try
      {
         boost::algorithm::split( splited, buf, boost::is_any_of("\r\n") ) ;
      }
      catch( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                 rc, e.what() ) ;
         goto error ;
      }
      if ( splited.empty() )
      {
         goto done ;
      }

      for ( vector<string>::iterator itr = splited.begin() ;
            itr != splited.end() ;
            itr++ )
      {
         usrSystemHostItem item ;

         if ( itr->empty() )
         {
            vecItems.push_back( item ) ;
            continue ;
         }
         vector<string> columns ;

         try
         {
            boost::algorithm::trim( *itr ) ;
            boost::algorithm::split( columns, *itr, boost::is_any_of("\t ") ) ;
         }
         catch( std::exception &e )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                    rc, e.what() ) ;
            goto error ;
         }
         for ( vector<string>::iterator itr2 = columns.begin();
               itr2 != columns.end();
                /// do not ++
               )
         {
            if ( itr2->empty() )
            {
               itr2 = columns.erase( itr2 ) ;
            }
            else
            {
               ++itr2 ;
            }
         }

         /// xxx.xxx.xxx.xxx xxxx
         /// xxx.xxx.xxx.xxx xxxx.xxxx xxxx
         if ( 2 != columns.size() && 3 != columns.size() )
         {
            item._ip = *itr ;
            vecItems.push_back( item ) ;
            continue ;
         }

         if ( !isValidIPV4( columns.at( 0 ).c_str() ) )
         {
            item._ip = *itr ;
            vecItems.push_back( item ) ;
            continue ;
         }

         item._ip = columns[ 0 ] ;
         if ( columns.size() == 3 )
         {
            item._com = columns[ 1 ] ;
            item._host = columns[ 2 ] ;
         }
         else
         {
            item._host = columns[ 1 ] ;
         }
         item._lineType = LINE_HOST ;
         vecItems.push_back( item ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _sptUsrSystem::_buildHostsResult( VEC_HOST_ITEM & vecItems,
                                          BSONObjBuilder &builder )
   {
      BSONArrayBuilder arrBuilder ;
      VEC_HOST_ITEM::iterator it = vecItems.begin() ;
      while ( it != vecItems.end() )
      {
         usrSystemHostItem &item = *it ;
         ++it ;

         if ( LINE_HOST != item._lineType )
         {
            continue ;
         }
         arrBuilder << BSON( CMD_USR_SYSTEM_IP << item._ip <<
                             CMD_USR_SYSTEM_HOSTNAME << item._host ) ;
      }
      builder.append( CMD_USR_SYSTEM_HOSTS, arrBuilder.arr() ) ;
   }


#if defined (_LINUX)
   INT32 _sptUsrSystem::getCpuInfo( const _sptArguments &arg,
                                    _sptReturnVal &rval,
                                    bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      UINT32 exitCode = 0 ;
      _ossCmdRunner runner ;
      string outStr ;
      BSONObjBuilder builder ;
#if defined (_PPCLIN64)
   #define CPU_CMD "cat /proc/cpuinfo | grep -E 'processor|cpu|clock|machine'"
#else
   #define CPU_CMD "cat /proc/cpuinfo | grep -E 'model name|cpu MHz|cpu cores|physical id'"
#endif

      rc = runner.exec( CPU_CMD, exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc || SDB_OK != exitCode )
      {
         PD_LOG( PDERROR, "failed to exec cmd, rc:%d, exit:%d",
                 rc, exitCode ) ;
         if ( SDB_OK == rc )
         {
            rc = SDB_SYS ;
         }
         stringstream ss ;
         ss << "failed to exec cmd \" " << CPU_CMD << "\",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }

      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to read msg from cmd runner:%d", rc ) ;
         stringstream ss ;
         ss << "failed to read msg from cmd \"" << CPU_CMD << "\", rc:"
            << rc ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }

      rc = _extractCpuInfo( outStr.c_str(), builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to extract cpu info:%d", rc ) ;
         stringstream ss ;
         ss << "failed to read msg from buf:"
            << outStr ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }

      {
      SINT64 user = 0 ;
      SINT64 sys = 0 ;
      SINT64 idle = 0 ;
      SINT64 other = 0 ;
      rc = ossGetCPUInfo( user, sys, idle, other ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      builder.appendNumber( CMD_USR_SYSTEM_USER, user ) ;
      builder.appendNumber( CMD_USR_SYSTEM_SYS, sys ) ;
      builder.appendNumber( CMD_USR_SYSTEM_IDLE, idle ) ;
      builder.appendNumber( CMD_USR_SYSTEM_OTHER, other ) ;
      }
      rval.getReturnVal().setValue( builder.obj() ) ;

   done:
      return rc ;
   error:
      goto done ;
   }
#endif /// _LINUX

#if defined (_WINDOWS)
   INT32 _sptUsrSystem::getCpuInfo( const _sptArguments &arg,
                                    _sptReturnVal &rval,
                                    bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      UINT32 exitCode = 0 ;
      _ossCmdRunner runner ;
      string outStr ;
      BSONObjBuilder builder ;
      const CHAR *cmd = "wmic CPU GET CurrentClockSpeed,Name,NumberOfCores" ;

      rc = runner.exec( cmd, exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc || SDB_OK != exitCode )
      {
         PD_LOG( PDERROR, "failed to exec cmd, rc:%d, exit:%d",
                 rc, exitCode ) ;
         if ( SDB_OK == rc )
         {
            rc = SDB_SYS ;
         }
         stringstream ss ;
         ss << "failed to exec cmd \" " << cmd << "\",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }

      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to read msg from cmd runner:%d", rc ) ;
         stringstream ss ;
         ss << "failed to read msg from cmd \"" << cmd << "\", rc:"
            << rc ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }

      rc = _extractCpuInfo( outStr.c_str(), builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to extract cpu info:%d", rc ) ;
         stringstream ss ;
         ss << "failed to read msg from buf:"
            << outStr ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }

      {
         SINT64 user = 0 ;
         SINT64 sys = 0 ;
         SINT64 idle = 0 ;
         SINT64 other = 0 ;
         rc = ossGetCPUInfo( user, sys, idle, other ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }

         builder.appendNumber( CMD_USR_SYSTEM_USER, user ) ;
         builder.appendNumber( CMD_USR_SYSTEM_SYS, sys ) ;
         builder.appendNumber( CMD_USR_SYSTEM_IDLE, idle ) ;
         builder.appendNumber( CMD_USR_SYSTEM_OTHER, other ) ;
      }
      rval.getReturnVal().setValue( builder.obj() ) ;

   done:
      return rc ;
   error:
      goto done ;
   }
#endif /// _WINDOWS


   INT32 _sptUsrSystem::snapshotCpuInfo( const _sptArguments &arg,
                                         _sptReturnVal &rval,
                                         bson::BSONObj &detail )
   {
      INT32 rc     = SDB_OK ;
      SINT64 user  = 0 ;
      SINT64 sys   = 0 ;
      SINT64 idle  = 0 ;
      SINT64 other = 0 ;
      rc = ossGetCPUInfo( user, sys, idle, other ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to get cpuinfo:%d", rc ) ;
         stringstream ss ;
         ss << "failed to get cpuinfo:rc="
            << rc ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }

      {
         BSONObjBuilder builder ;
         builder.appendNumber( CMD_USR_SYSTEM_USER, user ) ;
         builder.appendNumber( CMD_USR_SYSTEM_SYS, sys ) ;
         builder.appendNumber( CMD_USR_SYSTEM_IDLE, idle ) ;
         builder.appendNumber( CMD_USR_SYSTEM_OTHER, other ) ;

         rval.getReturnVal().setValue( builder.obj() ) ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

#if defined (_LINUX)
   #if defined (_PPCLIN64)
   INT32 _sptUsrSystem::_extractCpuInfo( const CHAR *buf,
                                         bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      BSONArrayBuilder arrBuilder ;
      // extract the follow 3 fields from the return content
      string strProcessor  = "processor" ;
      string strCpu        = "cpu" ;
      string strClock      = "clock" ;
      string strMachine    = "machine" ;
      // use to record the frequency of those 3 fields
      INT32 processorCount = 0 ;
      INT32 cpuCount       = 0 ;
      INT32 clockCount     = 0 ;
      INT32 machineCount   = 0 ;
      string modelName     = "" ;
      string machine       = "" ;
      vector<string> splited ;
      vector<string> vecFreq ;

      try
      {
         boost::algorithm::split( splited, buf, boost::is_any_of( "\r\n" ) ) ;
      }
      catch( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                 rc, e.what() ) ;
         goto error ;
      }
      for ( vector<string>::iterator itr = splited.begin();
            itr != splited.end(); // don't itr++
          )
      {
         if( itr->empty() )
         {
            itr = splited.erase( itr ) ;
         }
         else
         {
            itr++ ;
         }
      }
      for ( vector<string>::iterator itr = splited.begin();
            itr != splited.end(); itr++ )
      {
         // *itr is in the format of "xxx : xx", so let's
         // split it with ":"
         vector<string> columns ;

         try
         {
            boost::algorithm::split( columns, *itr, boost::is_any_of(":") ) ;
         }
         catch( std::exception &e )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                    rc, e.what() ) ;
            goto error ;
         }
         for ( vector<string>::iterator itr2 = columns.begin();
               itr2 != columns.end(); itr2++ )
         {
            boost::algorithm::trim( *itr2 ) ;
         }
         if ( strProcessor == columns.at(0) )
         {
            processorCount++ ;
         }
         else if ( strCpu == columns.at(0) )
         {
            if ( modelName == "" )
            {
               modelName = columns.at(1) ;
            }
            cpuCount++ ;
         }
         else if ( strClock== columns.at(0) )
         {
            vecFreq.push_back( columns.at(1) ) ;
            clockCount++ ;
         }
         else if ( strMachine == columns.at(0) )
         {
            machine = columns.at(1) ;
            machineCount = 1 ;
         }
         else
         {
            PD_LOG( PDERROR, "unexpect field[%s]", columns.at(0).c_str() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         // check and keep the cpu info
         if ( 1 == machineCount )
         {
            if ( processorCount != cpuCount ||
                 cpuCount != clockCount ||
                 clockCount != processorCount )
            {
               PD_LOG( PDERROR, "unexpect cpu info[%s]", buf ) ;
               rc = SDB_SYS ;
               goto error ;
            }
            // merge cpu info
            {
               UINT32 coreNum    = processorCount ;
               string info       = modelName ;
               string strAvgFreq ;
               FLOAT32 totalFreq = 0.0 ;

               for ( vector<string>::iterator itr2 = vecFreq.begin();
                     itr2 != vecFreq.end(); itr2++ )
               {
                  string freq = *itr2 ;
                  try
                  {
                     boost::algorithm::replace_last( freq, "MHz", "" ) ;
                     FLOAT32 inc = boost::lexical_cast<FLOAT32>( freq ) ;
                     totalFreq += inc / 1000.0 ;
                  }
                  catch( std::exception &e )
                  {
                     PD_LOG( PDERROR, "unexpected err happened:%s, content:[%s]",
                             e.what(), freq.c_str() ) ;
                     rc = SDB_SYS ;
                     goto error ;
                  }
               }
               try
               {
                  strAvgFreq = boost::lexical_cast<string>( totalFreq / coreNum ) ;
               }
               catch( std::exception &e )
               {
                  PD_LOG( PDERROR, "unexpected err happened:%s, content:[%f]",
                          e.what(), totalFreq / coreNum ) ;
                  rc = SDB_SYS ;
                  goto error ;
               }
               arrBuilder << BSON( CMD_USR_SYSTEM_CORE << coreNum
                                   << CMD_USR_SYSTEM_INFO << info
                                   << CMD_USR_SYSTEM_FREQ << strAvgFreq + "GHz" ) ;
            }
            // clean the counters
            processorCount = 0 ;
            cpuCount       = 0 ;
            clockCount     = 0 ;
            machineCount   = 0 ;
            modelName      = "" ;
            machine        = "" ;
            vecFreq.clear() ;
         }
      }
      builder.append( CMD_USR_SYSTEM_CPUS, arrBuilder.arr() ) ;
   done:
      return rc ;
   error:
      goto done ;
   }
   #else
   INT32 _sptUsrSystem::_extractCpuInfo( const CHAR *buf,
                                         bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      BSONArrayBuilder arrBuilder ;
      // extract the follow 4 fields from the return content
      string strModelName  = "model name" ;
      string strFreq       = "cpu MHz" ;
      string strCoreNum    = "cpu cores" ;
      string strPhysicalID = "physical id" ;
      // use to mark which field we had accessed
      INT32 flag           = 0x00000000 ;
      BOOLEAN mustPush ;
      vector<string> splited ;
      vector<cpuInfo> vecCpuInfo ;
      set<string> physicalIDSet ;
      cpuInfo info ;

      try
      {
         boost::algorithm::split( splited, buf, boost::is_any_of( "\n" ) ) ;
      }
      catch( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                 rc, e.what() ) ;
         goto error ;
      }
      for ( vector<string>::iterator itr = splited.begin();
            itr != splited.end(); // don't itr++
          )
      {
         if( itr->empty() )
         {
            itr = splited.erase( itr ) ;
         }
         else
         {
            itr++ ;
         }
      }

      // there is at lease one cpu
      physicalIDSet.insert( "0" ) ;
      info.reset() ;
      for ( vector<string>::iterator itr = splited.begin();
            itr != splited.end();
            itr++ )
      {
         // *itr is in the format of "xxx : xx", so let's
         // split it with ":"
         vector<string> columns ;
         try
         {
            boost::algorithm::split( columns, *itr, boost::is_any_of( "\t:" ),
                                     boost::token_compress_on ) ;
         }
         catch( std::exception &e )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                    rc, e.what() ) ;
            goto error ;
         }
         try
         {
            for ( vector<string>::iterator itr2 = columns.begin();
                  itr2 != columns.end(); itr2++ )
            {
               boost::algorithm::trim( *itr2 ) ;
            }
         }
         catch( std::exception &e )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Failed to trim, rc: %d, detail: %s",
                    rc, e.what() ) ;
            goto error ;
         }

         mustPush = FALSE ;
         if ( strModelName == columns.at( 0 ) )
         {
            if ( ( flag ^ 0x00000001 ) > flag )
            {
               info.modelName = columns.at( 1 ) ;
               flag ^= 0x00000001 ;
            }
            else
            {
               mustPush = TRUE ;
            }
         }
         else if ( strFreq == columns.at( 0 ) )
         {
            if ( ( flag ^ 0x00000010 ) > flag )
            {
               info.freq = columns.at( 1 ) ;
               flag ^= 0x00000010 ;
            }
            else
            {
               mustPush = TRUE ;
            }
         }
         else if ( strCoreNum == columns.at( 0 ) )
         {
            if ( ( flag ^ 0x00000100 ) > flag )
            {
               info.coreNum = columns.at( 1 ) ;
               flag ^= 0x00000100 ;
            }
            else
            {
               mustPush = TRUE ;
            }
         }
         else if ( strPhysicalID == columns.at(0) )
         {
            if ( ( flag ^ 0x00001000 ) > flag )
            {
               physicalIDSet.insert( columns.at(1) ) ;
               info.physicalID = columns.at(1) ;
               flag ^= 0x00001000 ;
            }
            else
            {
               mustPush = TRUE ;
            }
         }
         else
         {
            PD_LOG( PDERROR, "unexpect field[%s]", columns.at(0).c_str() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         // check and keep the cpu info
         if ( TRUE == mustPush )
         {
            vecCpuInfo.push_back( info ) ;
            info.reset() ;
            flag = 0 ;
            // need to decrease itr becase not use the value of itr
            itr-- ;
         }
      }

      // push last info obj
      if ( flag )
      {
         vecCpuInfo.push_back( info ) ;
      }

      // merge the cpu info
      for ( set<string>::iterator itr = physicalIDSet.begin();
            itr != physicalIDSet.end(); itr++ )
      {
         string physicalID = *itr ;
         UINT32 coreNum    = 0 ;
         string modelName  = "" ;
         string strAvgFreq ;
         FLOAT32 totalFreq = 0.0 ;
         for ( vector<cpuInfo>::iterator itr2 = vecCpuInfo.begin();
               itr2 != vecCpuInfo.end(); itr2++ )
         {
            if ( physicalID == itr2->physicalID )
            {
               // sum freq
               try
               {
                  FLOAT32 inc = boost::lexical_cast<FLOAT32>( itr2->freq ) ;
                  totalFreq += inc / 1000.0 ;
               }
               catch ( std::exception &e )
               {
                  PD_LOG( PDERROR, "unexpected err happened:%s, content:[%s]",
                          e.what(), (itr2->freq).c_str() ) ;
                  rc = SDB_SYS ;
                  goto error ;
               }
               // set modelName if it is uninitialized
               if ( modelName == "" )
               {
                  modelName = itr2->modelName ;
               }
               // add core num
               coreNum++ ;
            }
         }
         try
         {
            strAvgFreq = boost::lexical_cast<string>( totalFreq / coreNum ) ;
         }
         catch( std::exception &e )
         {
            PD_LOG( PDERROR, "unexpected err happened:%s, content:[%f]",
                    e.what(), totalFreq / coreNum ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         arrBuilder << BSON( CMD_USR_SYSTEM_CORE << coreNum
                             << CMD_USR_SYSTEM_INFO << modelName
                             << CMD_USR_SYSTEM_FREQ << strAvgFreq + "GHz" ) ;
      }
      builder.append( CMD_USR_SYSTEM_CPUS, arrBuilder.arr() ) ;
   done:
      return rc ;
   error:
      goto done ;
   }
   #endif /// _PPCLIN64
#endif // _LINUX

#if defined (_WINDOWS)
   INT32 _sptUsrSystem::_extractCpuInfo( const CHAR *buf,
                                         bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      BSONArrayBuilder arrBuilder ;
      vector<string> splited ;
      INT32 lineCount = 0 ;

      try
      {
         boost::algorithm::split( splited, buf, boost::is_any_of("\r\n") ) ;
      }
      catch( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                 rc, e.what() ) ;
         goto error ;
      }
      for ( vector<string>::iterator itr = splited.begin();
            itr != splited.end();
            itr++ )
      {
         ++lineCount ;
         if ( 1 == lineCount || itr->empty() )
         {
            continue ;
         }
         vector<string> columns ;

         try
         {
            boost::algorithm::trim( *itr ) ;
            boost::algorithm::split( columns, *itr, boost::is_any_of("\t ") ) ;
         }
         catch( std::exception &e )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                    rc, e.what() ) ;
            goto error ;
         }
         for ( vector<string>::iterator itr2 = columns.begin();
               itr2 != columns.end();
               /// do not ++
               )
         {
            if ( itr2->empty() )
            {
               itr2 = columns.erase( itr2 ) ;
            }
            else
            {
               ++itr2 ;
            }
         }

         /// eg: 3200 AMD Athlon(tm) II X2 B26 Processor 2
         if ( columns.size() < 3 )
         {
            rc = SDB_SYS ;
            goto error ;
         }
         UINT32 coreNum = 0 ;
         stringstream info ;

         try
         {
            coreNum = boost::lexical_cast<UINT32>(
               columns.at( columns.size() - 1 ) ) ;
         }
         catch ( std::exception &e )
         {
            PD_LOG( PDERROR, "unexpected err happened:%s", e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         for ( UINT32 i = 1; i < columns.size() - 1 ; i++ )
         {
            info << columns.at( i ) << " " ;
         }

         arrBuilder << BSON( CMD_USR_SYSTEM_CORE << coreNum
                             << CMD_USR_SYSTEM_INFO << info.str()
                             << CMD_USR_SYSTEM_FREQ << columns[ 0 ] ) ;
      }

      builder.append( CMD_USR_SYSTEM_CPUS, arrBuilder.arr() ) ;
   done:
      return rc ;
   error:
      goto done ;
   }
#endif //_LINUX

   INT32 _sptUsrSystem::getMemInfo( const _sptArguments &arg,
                                    _sptReturnVal &rval,
                                    bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      UINT32 exitCode = 0 ;
      _ossCmdRunner runner ;
      string outStr ;
      BSONObjBuilder builder ;

#if defined (_LINUX)
      rc = runner.exec( "free -m |grep Mem", exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
#elif defined (_WINDOWS)
      rc = SDB_SYS ;
#endif
      if ( SDB_OK != rc || SDB_OK != exitCode )
      {
         INT32 loadPercent = 0 ;
         INT64 totalPhys = 0 ;
         INT64 availPhys = 0 ;
         INT64 totalPF = 0 ;
         INT64 availPF = 0 ;
         INT64 totalVirtual = 0 ;
         INT64 availVirtual = 0 ;
         rc = ossGetMemoryInfo( loadPercent, totalPhys, availPhys,
                                totalPF, availPF, totalVirtual,
                                availVirtual ) ;
         if ( rc )
         {
            stringstream ss ;
            ss << "ossGetMemoryInfo failed, rc:" << rc ;
            detail = BSON( SPT_ERR << ss.str() ) ;
            goto error ;
         }

         builder.append( CMD_USR_SYSTEM_SIZE, (INT32)(totalPhys/CMD_MB_SIZE) ) ;
         builder.append( CMD_USR_SYSTEM_USED,
                         (INT32)((totalPhys-availPhys)/CMD_MB_SIZE) ) ;
         builder.append( CMD_USR_SYSTEM_FREE,(INT32)(availPhys/CMD_MB_SIZE) ) ;
         builder.append( CMD_USR_SYSTEM_UNIT, "M" ) ;
         rval.getReturnVal().setValue( builder.obj() ) ;
         goto done ;
      }

      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to read msg from cmd runner:%d", rc ) ;
         stringstream ss ;
         ss << "failed to read msg from cmd \"free\", rc:"
            << rc ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }

      rc = _extractMemInfo( outStr.c_str(), builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to extract mem info:%d", rc ) ;
         stringstream ss ;
         ss << "failed to read msg from buf:"
            << outStr ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }
      rval.getReturnVal().setValue( builder.obj() ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrSystem::snapshotMemInfo( const _sptArguments &arg,
                                         _sptReturnVal &rval,
                                         bson::BSONObj &detail )
   {
      return getMemInfo( arg, rval, detail ) ;
   }

   INT32 _sptUsrSystem::_extractMemInfo( const CHAR *buf,
                                         bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      vector<string> splited ;

      try
      {
         boost::algorithm::split( splited, buf, boost::is_any_of("\t ") ) ;
      }
      catch( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                 rc, e.what() ) ;
         goto error ;
      }
      for ( vector<string>::iterator itr = splited.begin();
            itr != splited.end();
            /// do not ++
          )
      {
         if ( itr->empty() )
         {
            itr = splited.erase( itr ) ;
         }
         else
         {
            ++itr ;
         }
      }
      /// Mem:       8194232    2373776    5820456          0     387924     992756
      /// choose total used free
      if ( splited.size() < 4 )
      {
         rc = SDB_SYS ;
         goto error ;
      }

      try
      {
         builder.append( CMD_USR_SYSTEM_SIZE,
                         boost::lexical_cast<UINT32>(splited.at( 1 ) ) ) ;
         builder.append( CMD_USR_SYSTEM_USED,
                         boost::lexical_cast<UINT32>(splited.at( 2 ) ) ) ;
         builder.append( CMD_USR_SYSTEM_FREE,
                         boost::lexical_cast<UINT32>(splited.at( 3) ) ) ;
         builder.append( CMD_USR_SYSTEM_UNIT, "M" ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected err happened:%s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrSystem::getDiskInfo( const _sptArguments &arg,
                                     _sptReturnVal &rval,
                                     bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
#if defined (_LINUX)
      rc = _getLinuxDiskInfo( arg, rval, detail ) ;
#else
      rc = _getWinDiskInfo( arg, rval, detail ) ;
#endif
      if ( SDB_OK != rc )
      {
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrSystem::_getLinuxDiskInfo( const _sptArguments &arg,
                                           _sptReturnVal &rval,
                                           bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      SINT64 read = 0 ;
      OSSFILE file ;
      stringstream ss ;
      stringstream filess ;
      const UINT32 bufSize = 256 ;
      CHAR buf[ bufSize + 1 ] = { 0 } ;

      rc = ossOpen( CMD_DISK_SRC_FILE,
                    OSS_READONLY | OSS_SHAREREAD,
                    OSS_DEFAULTFILE,
                    file ) ;
      if ( SDB_OK != rc )
      {
         ss << "failed to open file(/etc/mtab), rc:" << rc ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }

      do
      {
         read = 0 ;
         ossMemset( buf, '\0', bufSize ) ;
         rc = ossReadN( &file, bufSize, buf, read ) ;

         if ( SDB_EOF == rc && 0 < filess.tellp() )
         {
            rc = SDB_OK ;
            break ;
         }

         if ( SDB_OK != rc )
         {
            ss << "failed to read file(/etc/mtab), rc:" << rc ;
            detail = BSON( SPT_ERR << ss.str() ) ;
            goto error ;
         }

         filess << buf ;
         if ( read < bufSize )
         {
            break ;
         }
      } while ( TRUE ) ;

      rc = _extractLinuxDiskInfo( filess.str().c_str(), rval, detail ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
   done:
      if ( file.isOpened() )
      {
         ossClose( file ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrSystem::_extractLinuxDiskInfo( const CHAR *buf,
                                               _sptReturnVal &rval,
                                               bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder builder ;
      BSONArrayBuilder arrBuilder ;
      vector<string> splited ;

      try
      {
         boost::algorithm::split( splited, buf, boost::is_any_of("\r\n") ) ;
      }
      catch( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                 rc, e.what() ) ;
         goto error ;
      }
      for ( vector<string>::iterator itr = splited.begin();
            itr != splited.end();
            itr++ )
      {
         BSONObjBuilder diskBuilder ;
         INT64 totalBytes = 0 ;
         INT64 freeBytes = 0 ;
         const CHAR *fs = NULL ;
         const CHAR *fsType = NULL ;
         const CHAR *mount = NULL ;
         vector<string> columns ;

         try
         {
            boost::algorithm::split( columns, *itr, boost::is_any_of( "\t " ) ) ;
         }
         catch( std::exception &e )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                    rc, e.what() ) ;
            goto error ;
         }
         if ( 6 != columns.size() )
         {
            continue ;
         }

         fs = columns.at( 0 ).c_str() ;
         fsType = columns.at( 2 ).c_str() ;
         mount = columns.at( 1 ).c_str() ;
         if ( SDB_OK == ossGetDiskInfo( mount, totalBytes, freeBytes ) )
         {
            if ( ossStrcasecmp( CMD_DISK_IGNORE_TYPE_BINFMT_MISC, fsType ) == 0
                 || ossStrcasecmp( CMD_DISK_IGNORE_TYPE_SYSFS, fsType ) == 0
                 || ossStrcasecmp( CMD_DISK_IGNORE_TYPE_PROC, fsType ) == 0
                 || ossStrcasecmp( CMD_DISK_IGNORE_TYPE_DEVPTS, fsType ) == 0
                 || ossStrcasecmp( CMD_DISK_IGNORE_TYPE_FUSECTL, fsType ) == 0
                 || ossStrcasecmp( CMD_DISK_IGNORE_TYPE_GVFS, fsType ) == 0
                 || ossStrcasecmp( CMD_DISK_IGNORE_TYPE_SECURITYFS,
                                                                 fsType ) == 0 )
            {
               continue ;
            }

            diskBuilder.append( CMD_USR_SYSTEM_FILESYSTEM,
                                fs ) ;
            diskBuilder.append( CMD_USR_SYSTEM_FSTYPE, fsType ) ;
            diskBuilder.appendNumber( CMD_USR_SYSTEM_SIZE, totalBytes / ( 1024 * 1024 ) ) ;
            diskBuilder.appendNumber( CMD_USR_SYSTEM_USED, ( totalBytes - freeBytes ) / ( 1024 * 1024 ) ) ;
            diskBuilder.append( CMD_USR_SYSTEM_UNIT, "MB" ) ;
            diskBuilder.append( CMD_USR_SYSTEM_MOUNT, mount ) ;

            BOOLEAN isLocal = ( string::npos != columns.at( 0 ).find( "/dev/", 0, 5 ) ) ;
            BOOLEAN gotStat = FALSE ;
            diskBuilder.appendBool( CMD_USR_SYSTEM_ISLOCAL, isLocal ) ;

            if ( isLocal )
            {
               ossDiskIOStat ioStat ;
               CHAR pathBuffer[ 256 ] = {0} ;
               INT32 rctmp = SDB_OK ;

               string driverName = columns.at( 0 ) ;

               rctmp = ossReadlink( driverName.c_str(), pathBuffer, 256 ) ;
               if ( SDB_OK == rctmp )
               {
                  // only match the driver name after /dev/
                  driverName = pathBuffer ;
                  if ( string::npos != driverName.find( "../", 0, 3 ) )
                  {
                     driverName.replace(0, 3, "" ) ;
                  }
                  else
                  {
                     driverName = columns.at( 0 ) ;
                     driverName.replace(0, 5, "" ) ;
                  }
               }
               else
               {
                  driverName.replace(0, 5, "" ) ;
               }

               ossMemset( &ioStat, 0, sizeof( ossDiskIOStat ) ) ;

               rctmp = ossGetDiskIOStat( driverName.c_str(), ioStat ) ;
               if ( SDB_OK == rctmp )
               {
                  diskBuilder.appendNumber( CMD_USR_SYSTEM_IO_R_SEC,
                                            (INT64)ioStat.rdSectors ) ;
                  diskBuilder.appendNumber( CMD_USR_SYSTEM_IO_W_SEC,
                                            (INT64)ioStat.wrSectors ) ;
                  gotStat = TRUE ;
               }
            }
            if ( !gotStat )
            {
               diskBuilder.appendNumber( CMD_USR_SYSTEM_IO_R_SEC, (INT64)0 ) ;
               diskBuilder.appendNumber( CMD_USR_SYSTEM_IO_W_SEC, (INT64)0 ) ;
            }

            arrBuilder << diskBuilder.obj() ;
         }
      }

      builder.append( CMD_USR_SYSTEM_DISKS, arrBuilder.arr() ) ;
      rval.getReturnVal().setValue( builder.obj() ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrSystem::_getWinDiskInfo( const _sptArguments &arg,
                                         _sptReturnVal &rval,
                                         bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      UINT32 exitCode = 0 ;
      _ossCmdRunner runner ;
      string outStr ;
      BSONObjBuilder builder ;

#define DISK_CMD  "wmic VOLUME get Capacity,DriveLetter,Caption,"\
                  "DriveType,FreeSpace,SystemVolume"

      rc = runner.exec( DISK_CMD, exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc || SDB_OK != exitCode )
      {
         PD_LOG( PDERROR, "failed to exec cmd, rc:%d, exit:%d",
                 rc, exitCode ) ;
         if ( SDB_OK == rc )
         {
            rc = SDB_SYS ;
         }
         stringstream ss ;
         ss << "failed to exec cmd \"" << DISK_CMD << "\",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }

      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to read msg from cmd runner:%d", rc ) ;
         stringstream ss ;
         ss << "failed to read msg from cmd \"df\", rc:"
            << rc ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }

      rc = _extractWinDiskInfo( outStr.c_str(), builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to extract disk info:%d", rc ) ;
         stringstream ss ;
         ss << "failed to read msg from buf:"
            << outStr ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }
      rval.getReturnVal().setValue( builder.obj() ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrSystem::snapshotDiskInfo( const _sptArguments &arg,
                                          _sptReturnVal &rval,
                                          bson::BSONObj &detail )
   {
      return getDiskInfo( arg, rval, detail ) ;
   }

   INT32 _sptUsrSystem::_extractWinDiskInfo( const CHAR *buf,
                                             bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      BSONArrayBuilder arrBuilder ;
      string fileSystem ;
      string freeSpace ;
      string total ;
      string mount ;
      vector<string> splited ;
      INT32 lineCount = 0 ;

      try
      {
         boost::algorithm::split( splited, buf, boost::is_any_of( "\r\n" ) ) ;
      }
      catch( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                 rc, e.what() ) ;
         goto error ;
      }
      for ( vector<string>::iterator itr = splited.begin();
            itr != splited.end();
            itr++ )
      {
         ++lineCount ;
         if ( 1 == lineCount || itr->empty() )
         {
            continue ;
         }

         vector<string> columns ;

         try
         {
            boost::algorithm::split( columns, *itr, boost::is_any_of("\t ") ) ;
         }
         catch( std::exception &e )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                    rc, e.what() ) ;
            goto error ;
         }
         for ( vector<string>::iterator itr2 = columns.begin();
               itr2 != columns.end();
               /// do not ++
               )
         {
            if ( itr2->empty() )
            {
               itr2 = columns.erase( itr2 ) ;
            }
            else
            {
               ++itr2 ;
            }
         }

         if ( columns.size() < 6 || columns.at( 5 ) == "TRUE" ||
              columns.at( 3 ) != "3" )
         {
            continue ;
         }

         total = columns[ 0 ] ;
         fileSystem = columns[ 1 ] ;
         freeSpace = columns[ 4 ] ;
         mount = columns[ 2 ] ;

         // build
         SINT64 totalNum = 0 ;
         SINT64 usedNumber = 0 ;
         SINT64 avaNumber = 0 ;
         BSONObjBuilder lineBuilder ;
         try
         {
            avaNumber = boost::lexical_cast<SINT64>( freeSpace ) ;
            totalNum = boost::lexical_cast<SINT64>( total ) ;
            usedNumber = totalNum - avaNumber ;
            lineBuilder.append( CMD_USR_SYSTEM_FILESYSTEM,
                                fileSystem.c_str() ) ;
            lineBuilder.appendNumber( CMD_USR_SYSTEM_SIZE,
                                      (INT32)( totalNum / CMD_MB_SIZE ) ) ;
            lineBuilder.appendNumber( CMD_USR_SYSTEM_USED,
                                      (INT32)( usedNumber / CMD_MB_SIZE ) ) ;
            lineBuilder.append( CMD_USR_SYSTEM_UNIT, "M" ) ;
            lineBuilder.append( CMD_USR_SYSTEM_MOUNT, mount ) ;
            lineBuilder.appendBool( CMD_USR_SYSTEM_ISLOCAL, TRUE ) ;
            lineBuilder.appendNumber( CMD_USR_SYSTEM_IO_R_SEC, (INT64)0 ) ;
            lineBuilder.appendNumber( CMD_USR_SYSTEM_IO_W_SEC, (INT64)0 ) ;
            arrBuilder << lineBuilder.obj() ;
         }
         catch ( std::exception )
         {
            rc = SDB_SYS ;
            goto error ;
         }

         freeSpace.clear();
         total.clear() ;
         mount.clear() ;
         fileSystem.clear() ;
      } // end for

      builder.append( CMD_USR_SYSTEM_DISKS, arrBuilder.arr() ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrSystem::getNetcardInfo( const _sptArguments &arg,
                                        _sptReturnVal &rval,
                                        bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder builder ;
      if ( 0 < arg.argc() )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = _extractNetcards( builder ) ;
      if ( SDB_OK != rc )
      {
         stringstream ss ;
         ss << "failed to get netcard info:" << rc ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }
      rval.getReturnVal().setValue( builder.obj() ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrSystem::_extractNetcards( bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      CHAR *pBuff = NULL ;
      BSONArrayBuilder arrBuilder ;

#if defined (_WINDOWS)
      PIP_ADAPTER_INFO pAdapterInfo = NULL ;
      DWORD dwRetVal = 0 ;
      ULONG ulOutbufLen = sizeof( PIP_ADAPTER_INFO ) ;

      pBuff = (CHAR*)SDB_OSS_MALLOC( ulOutbufLen ) ;
      if ( !pBuff )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      pAdapterInfo = (PIP_ADAPTER_INFO)pBuff ;

      // first call GetAdapterInfo to get ulOutBufLen size
      dwRetVal = GetAdaptersInfo( pAdapterInfo, &ulOutbufLen ) ;
      if ( dwRetVal == ERROR_BUFFER_OVERFLOW )
      {
         SDB_OSS_FREE( pBuff ) ;
         pBuff = ( CHAR* )SDB_OSS_MALLOC( ulOutbufLen ) ;
         if ( !pBuff )
         {
            rc = SDB_OOM ;
            goto error ;
         }
         pAdapterInfo = (PIP_ADAPTER_INFO)pBuff ;
         dwRetVal = GetAdaptersInfo( pAdapterInfo, &ulOutbufLen ) ;
      }

      if ( dwRetVal != NO_ERROR )
      {
         rc = SDB_SYS ;
         goto error ;
      }
      else
      {
         PIP_ADAPTER_INFO pAdapter = pAdapterInfo ;
         while ( pAdapter )
         {
            stringstream ss ;
            ss << "eth" << pAdapter->Index ;
            arrBuilder << BSON( CMD_USR_SYSTEM_NAME << ss.str()
                                << CMD_USR_SYSTEM_IP <<
                                pAdapter->IpAddressList.IpAddress.String ) ;
            pAdapter = pAdapter->Next ;
         }
      }
#elif defined (_LINUX)
      struct ifconf ifc ;
      struct ifreq *ifreq = NULL ;
      INT32 sock = -1 ;

      pBuff = ( CHAR* )SDB_OSS_MALLOC( 1024 ) ;
      if ( !pBuff )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      ifc.ifc_len = 1024 ;
      ifc.ifc_buf = pBuff;

      if ( (sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
      {
         PD_LOG( PDERROR, "failed to init socket" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( SDB_OK != ioctl( sock, SIOCGIFCONF, &ifc ) )
      {
         close( sock ) ;
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "failed to call ioctl" ) ;
         goto error ;
      }

      ifreq = ( struct ifreq * )pBuff ;
      for ( INT32 i = ifc.ifc_len / sizeof(struct ifreq);
            i > 0;
            --i )
      {
         arrBuilder << BSON( CMD_USR_SYSTEM_NAME << ifreq->ifr_name
                             << CMD_USR_SYSTEM_IP <<
                             inet_ntoa(((struct sockaddr_in*)&
                                         (ifreq->ifr_addr))->sin_addr) ) ;
         ++ifreq ;
      }
      close( sock ) ;
#endif
      builder.append( CMD_USR_SYSTEM_NETCARDS, arrBuilder.arr() ) ;
   done:
      if ( pBuff )
      {
         SDB_OSS_FREE( pBuff ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrSystem::getIpTablesInfo( const _sptArguments &arg,
                                         _sptReturnVal &rval,
                                         bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      if ( 0 < arg.argc() )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      {
         BSONObj info = BSON( "FireWall" << "unknown" ) ;
         rval.getReturnVal().setValue( info ) ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   #if defined (_LINUX)
   INT32 _sptUsrSystem::_extractNetCardSnapInfo( const CHAR *buf,
                                                 bson::BSONObjBuilder &builder )
   {
      time_t myTime = time( NULL ) ;
      BSONArrayBuilder arrayBuilder ;
      INT32 rc = SDB_OK ;
      vector<string> vLines ;
      vector<string>::iterator iterLine ;

      try
      {
         boost::algorithm::split( vLines, buf, boost::is_any_of( "\r\n" ) ) ;
      }
      catch( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                 rc, e.what() ) ;
         goto error ;
      }
      iterLine = vLines.begin() ;

      while ( iterLine != vLines.end() )
      {
         if ( !iterLine->empty() )
         {
            const CHAR *oneLine = iterLine->c_str() ;
            vector<string> vColumns ;
            try
            {
               boost::algorithm::split( vColumns, oneLine,
                                        boost::is_any_of( "\t " ) ) ;
            }
            catch( std::exception &e )
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                       rc, e.what() ) ;
               goto error ;
            }
            vector<string>::iterator iterColumn = vColumns.begin() ;
            while ( iterColumn != vColumns.end() )
            {
               if ( iterColumn->empty() )
               {
                  vColumns.erase( iterColumn++ ) ;
               }
               else
               {
                  iterColumn++ ;
               }
            }

            // if not match format,discard the row
            if ( vColumns.size() < 9 )
            {
               continue ;
            }
      //card rx_byte   rx_packet rx_err rx_drop tx_byte tx_packet tx_err tx_drop
      //lo   14755559460 44957591  0      0       14755559460 44957591 0 0
      //eth1 4334054313  11529654  0      0       9691246348  3513633  0 0
            try
            {
               BSONObjBuilder innerBuilder ;
               innerBuilder.append( CMD_USR_SYSTEM_NAME,
                             boost::lexical_cast<string>( vColumns.at( 0 ) ) ) ;
               innerBuilder.append( CMD_USR_SYSTEM_RX_BYTES,
                            ( long long )boost::lexical_cast<UINT64>(
                                                         vColumns.at( 1 ) ) ) ;
               innerBuilder.append( CMD_USR_SYSTEM_RX_PACKETS,
                            ( long long )boost::lexical_cast<UINT64>(
                                                         vColumns.at( 2 ) ) ) ;
               innerBuilder.append( CMD_USR_SYSTEM_RX_ERRORS,
                            ( long long )boost::lexical_cast<UINT64>(
                                                         vColumns.at( 3 ) ) ) ;
               innerBuilder.append( CMD_USR_SYSTEM_RX_DROPS,
                            ( long long )boost::lexical_cast<UINT64>(
                                                         vColumns.at( 4 ) ) ) ;
               innerBuilder.append( CMD_USR_SYSTEM_TX_BYTES,
                            ( long long )boost::lexical_cast<UINT64>(
                                                         vColumns.at( 5 ) ) ) ;
               innerBuilder.append( CMD_USR_SYSTEM_TX_PACKETS,
                            ( long long )boost::lexical_cast<UINT64>(
                                                         vColumns.at( 6 ) ) ) ;
               innerBuilder.append( CMD_USR_SYSTEM_TX_ERRORS,
                            ( long long )boost::lexical_cast<UINT64>(
                                                         vColumns.at( 7 ) ) ) ;
               innerBuilder.append( CMD_USR_SYSTEM_TX_DROPS,
                            ( long long )boost::lexical_cast<UINT64>(
                                                         vColumns.at( 8 ) ) ) ;
               BSONObj obj = innerBuilder.obj() ;
               arrayBuilder.append( obj ) ;
            }
            catch ( std::exception &e )
            {
               PD_LOG( PDERROR, "unexpected err happened:%s", e.what() ) ;
               rc = SDB_SYS ;
               goto error ;
            }
         }

         iterLine++ ;
      }

      try
      {
         builder.append( CMD_USR_SYSTEM_CALENDAR_TIME, (long long)myTime ) ;
         builder.append( CMD_USR_SYSTEM_NETCARDS, arrayBuilder.arr() ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected err happened:%s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }
#else
   INT32 _sptUsrSystem::_extractNetCardSnapInfo( const CHAR *buf,
                                                 bson::BSONObjBuilder &builder )
   {
      return SDB_INVALIDARG ;
   }
#endif

#if defined (_LINUX)
   INT32 _sptUsrSystem::_snapshotNetcardInfo( bson::BSONObjBuilder &builder,
                                              bson::BSONObj &detail )
   {
      INT32 rc        = SDB_OK ;
      UINT32 exitCode = 0 ;
      _ossCmdRunner runner ;
      string outStr ;
      stringstream ss ;
      const CHAR *netFlowCMD = "cat /proc/net/dev | grep -v Receive |"
                               " grep -v bytes | sed 's/:/ /' |"
                               " awk '{print $1,$2,$3,$4,$5,$10,$11,$12,$13}'" ;

      rc = runner.exec( netFlowCMD, exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc || SDB_OK != exitCode )
      {
         PD_LOG( PDERROR, "failed to exec cmd, rc:%d, exit:%d",
                 rc, exitCode ) ;
         if ( SDB_OK == rc )
         {
            rc = SDB_SYS ;
         }
         ss << "failed to exec cmd \"" << netFlowCMD << "\",rc:"
            << rc << ",exit:" << exitCode ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }

      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to read msg from cmd runner:%d", rc ) ;
         stringstream ss ;
         ss << "failed to read msg from cmd \"df\", rc:" << rc ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }

      rc = _extractNetCardSnapInfo( outStr.c_str(), builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to extract netcard snapshotinfo:%d", rc ) ;
         ss << "failed to extract netcard snapshotinfo from buf:" << outStr ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }
#else
   INT32 _sptUsrSystem::_snapshotNetcardInfo( bson::BSONObjBuilder &builder,
                                              bson::BSONObj &detail )
   {
      INT32 rc              = SDB_OK ;
      UINT32 exitCode       = 0 ;
      PMIB_IFTABLE pTable   = NULL ;
      stringstream ss ;
      time_t myTime ;

      DWORD size = sizeof( MIB_IFTABLE ) ;
      pTable     = (PMIB_IFTABLE) SDB_OSS_MALLOC( size ) ;
      if ( NULL == pTable )
      {
         rc = SDB_OOM ;
         PD_LOG ( PDERROR, "new MIB_IFTABLE failed:rc=%d", rc ) ;
         ss << "new MIB_IFTABLE failed:rc=" << rc ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }

      ULONG uRetCode = GetIfTable( pTable, &size, TRUE ) ;
      if ( uRetCode == ERROR_NOT_SUPPORTED )
      {
         PD_LOG ( PDERROR, "GetIfTable failed:rc=%u", uRetCode ) ;
         rc = SDB_INVALIDARG ;
         ss << "GetIfTable failed:rc=" << uRetCode ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }

      if ( uRetCode == ERROR_INSUFFICIENT_BUFFER )
      {
         SDB_OSS_FREE( pTable ) ;
         pTable = (PMIB_IFTABLE) SDB_OSS_MALLOC( size ) ;
         if ( NULL == pTable )
         {
            rc = SDB_OOM ;
            PD_LOG ( PDERROR, "new MIB_IFTABLE failed:rc=%d", rc ) ;
            ss << "new MIB_IFTABLE failed:rc=" << rc ;
            detail = BSON( SPT_ERR << ss.str() ) ;
            goto error ;
         }
      }

      // get the seconds since 1970.1.1:0:0:0(Calendar Time)
      myTime = time( NULL ) ;
      uRetCode = GetIfTable( pTable, &size, TRUE ) ;
      if ( NO_ERROR != uRetCode )
      {
         PD_LOG ( PDERROR, "GetIfTable failed:rc=%u", uRetCode ) ;
         rc = SDB_INVALIDARG ;
         ss << "GetIfTable failed:rc=" << uRetCode ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }

      try
      {
         BSONArrayBuilder arrayBuilder ;
         for ( UINT i = 0 ; i < pTable->dwNumEntries ; i++ )
         {
            MIB_IFROW Row = pTable->table[ i ];
            if ( IF_TYPE_ETHERNET_CSMACD != Row.dwType )
            {
               continue ;
            }

            BSONObjBuilder innerBuilder ;
            stringstream ss ;
            ss << "eth" << Row.dwIndex ;
            innerBuilder.append( CMD_USR_SYSTEM_NAME, ss.str() ) ;
            innerBuilder.append( CMD_USR_SYSTEM_RX_BYTES,
                                 ( long long )Row.dwInOctets ) ;
            innerBuilder.append( CMD_USR_SYSTEM_RX_PACKETS,
                          ( long long )
                                 ( Row.dwInUcastPkts + Row.dwInNUcastPkts ) ) ;
            innerBuilder.append( CMD_USR_SYSTEM_RX_ERRORS,
                                 ( long long )Row.dwInErrors ) ;
            innerBuilder.append( CMD_USR_SYSTEM_RX_DROPS,
                                 ( long long )Row.dwInDiscards ) ;
            innerBuilder.append( CMD_USR_SYSTEM_TX_BYTES,
                                 ( long long )Row.dwOutOctets ) ;
            innerBuilder.append( CMD_USR_SYSTEM_TX_PACKETS,
                          ( long long )
                                ( Row.dwOutUcastPkts + Row.dwOutNUcastPkts ) ) ;
            innerBuilder.append( CMD_USR_SYSTEM_TX_ERRORS,
                                 ( long long )Row.dwOutErrors ) ;
            innerBuilder.append( CMD_USR_SYSTEM_TX_DROPS,
                                 ( long long )Row.dwOutDiscards ) ;
            BSONObj obj = innerBuilder.obj() ;
            arrayBuilder.append( obj ) ;
         }

         builder.append( CMD_USR_SYSTEM_CALENDAR_TIME, (long long)myTime ) ;
         builder.append( CMD_USR_SYSTEM_NETCARDS, arrayBuilder.arr() ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected err happened:%s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      if ( NULL != pTable )
      {
         SDB_OSS_FREE( pTable ) ;
      }
      return rc ;
   error:
      goto done ;
   }

#endif

   INT32 _sptUsrSystem::snapshotNetcardInfo( const _sptArguments &arg,
                                             _sptReturnVal &rval,
                                             bson::BSONObj &detail )
   {
      bson::BSONObjBuilder builder ;
      INT32 rc = SDB_OK ;
      stringstream ss ;

      if ( 0 < arg.argc() )
      {
         PD_LOG ( PDERROR, "paramenter can't be greater then 0" ) ;
         rc = SDB_INVALIDARG ;
         ss << "paramenter can't be greater then 0" ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }

      rc = _snapshotNetcardInfo( builder, detail ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "_snapshotNetcardInfo failed:rc=%d", rc ) ;
         goto error ;
      }

      rval.getReturnVal().setValue( builder.obj() ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrSystem::getHostName( const _sptArguments &arg,
                                     _sptReturnVal &rval,
                                     bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      CHAR hostName[ OSS_MAX_HOSTNAME + 1 ] = { 0 } ;
      if ( 0 < arg.argc() )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = ossGetHostName( hostName, OSS_MAX_HOSTNAME ) ;
      if ( rc )
      {
         detail = BSON( SPT_ERR << "get hostname failed" ) ;
         goto error ;
      }

      rval.getReturnVal().setValue( hostName ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrSystem::sniffPort ( const _sptArguments &arg,
                                    _sptReturnVal &rval,
                                    bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      UINT32 port = 0 ;
      BOOLEAN result = FALSE ;
      stringstream ss ;
      BSONObjBuilder builder ;

      if ( 0 == arg.argc() )
      {
         rc = SDB_INVALIDARG ;
         ss << "not specified the port to sniff" ;
         goto error ;
      }
      rc = arg.getNative( 0, &port, SPT_NATIVE_INT32 ) ;
      if ( rc )
      {
         string svcname ;
         UINT16 tempPort ;
         rc = arg.getString( 0, svcname ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG ( PDERROR, "failed to get port argument: %d", rc ) ;
            ss << "port must be number or string" ;
            goto error ;
         }

         rc = ossGetPort( svcname.c_str(), tempPort ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG ( PDERROR, "failed to get port by serviceName: %d", rc ) ;
            ss << "Invalid svcname: " << svcname ;
            goto error ;
         }
         port = tempPort ;
      }
      if ( 0 >= port || 65535 < port )
      {
         rc = SDB_INVALIDARG ;
         ss << "port must in range ( 0, 65536 )" ;
         goto error ;
      }

      {
      PD_LOG ( PDDEBUG, "sniff port is: %d", port ) ;
      _ossSocket sock( port, OSS_ONE_SEC ) ;
      rc = sock.initSocket() ;
      if ( rc )
      {
         PD_LOG ( PDWARNING, "failed to connect to port[%d], "
                  "rc: %d", port, rc ) ;
         ss << "failed to sniff port" ;
         goto error ;
      }
      rc = sock.bind_listen() ;
      if ( rc )
      {
         PD_LOG ( PDDEBUG, "port[%d] is busy, rc: %d", port, rc ) ;
         result = FALSE ;
         rc = SDB_OK ;
      }
      else
      {
         PD_LOG ( PDDEBUG, "port[%d] is usable", port ) ;
         result = TRUE ;
      }
      builder.appendBool( CMD_USR_SYSTEM_USABLE, result ) ;
      rval.getReturnVal().setValue( builder.obj() ) ;
      //close the socket
      sock.close() ;
      }

   done:
      return rc ;
   error:
      detail = BSON( SPT_ERR << ss.str() ) ;
      goto done ;
   }

   INT32 _sptUsrSystem::getPID ( const _sptArguments &arg,
                                 _sptReturnVal &rval,
                                 bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      UINT32 id = 0 ;
      stringstream ss ;
      BSONObjBuilder builder ;

      if ( 0 < arg.argc() )
      {
         rc = SDB_INVALIDARG ;
         ss << "No need arguments" ;
         goto error ;
      }
      id = ossGetCurrentProcessID() ;
      rval.getReturnVal().setValue( id ) ;

   done:
      return rc ;
   error:
      detail = BSON( SPT_ERR << ss.str() ) ;
      goto done ;
   }

   INT32 _sptUsrSystem::getTID ( const _sptArguments &arg,
                                 _sptReturnVal &rval,
                                 bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      UINT32 id = 0 ;
      stringstream ss ;
      BSONObjBuilder builder ;

      if ( 0 < arg.argc() )
      {
         rc = SDB_INVALIDARG ;
         ss << "No need arguments" ;
         goto error ;
      }

      id = (UINT32)ossGetCurrentThreadID() ;
      rval.getReturnVal().setValue( id ) ;

   done:
      return rc ;
   error:
      detail = BSON( SPT_ERR << ss.str() ) ;
      goto done ;
   }

   INT32 _sptUsrSystem::getEWD ( const _sptArguments &arg,
                                 _sptReturnVal &rval,
                                 bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      CHAR buf[ OSS_MAX_PATHSIZE + 1 ] = {0} ;
      stringstream ss ;
      BSONObjBuilder builder ;

      if ( 0 < arg.argc() )
      {
         rc = SDB_INVALIDARG ;
         ss << "No need arguments" ;
         goto error ;
      }

      rc = ossGetEWD( buf, OSS_MAX_PATHSIZE ) ;
      if ( rc )
      {
         ss << "Get current executable file's working directory failed" ;
         goto error ;
      }

      rval.getReturnVal().setValue( buf ) ;

   done:
      return rc ;
   error:
      detail = BSON( SPT_ERR << ss.str() ) ;
      goto done ;
   }

   INT32 _sptUsrSystem::listProcess( const _sptArguments & arg,
                                     _sptReturnVal & rval,
                                     BSONObj & detail )
   {
      INT32 rc         = SDB_OK ;
      UINT32 exitCode  = 0 ;
      BSONObjBuilder   builder ;
      BSONObj          optionObj ;
      string           outStr ;
      stringstream     cmd ;
      _ossCmdRunner    runner ;
      BOOLEAN          showDetail = FALSE ;

      // get optionObj
      if( arg.argc() > 0 )
      {
         rc = arg.getBsonobj( 0, optionObj ) ;
         if ( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "optionObj must be object" ) ;
            PD_LOG( PDERROR, "optionObj must be object, rc: %d", rc ) ;
            goto error ;
         }
         showDetail = optionObj.getBoolField( "detail" ) ;
      }

      // build cmd
#if defined ( _LINUX )
   cmd << "ps ax -o user -o pid -o stat -o command" ;
#elif defined (_WINDOWS)
   cmd << "tasklist /FO \"CSV\"" ;
#endif

      // run cmd
      rc = runner.exec( cmd.str().c_str(), exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to exec cmd, rc:%d, exit:%d",
                 rc, exitCode ) ;
         stringstream ss ;
         ss << "failed to exec cmd " << cmd.str() << ",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }

      // get result
      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to read msg from cmd runner:%d", rc ) ;
         stringstream ss ;
         ss << "failed to read msg from cmd \"" << cmd.str() << "\", rc:"
            << rc ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }

      // extract result
      rc = _extractProcessInfo( outStr.c_str(), builder, showDetail ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      rval.getReturnVal().setValue( builder.obj() ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

#if defined ( _LINUX )
   INT32 _sptUsrSystem::_extractProcessInfo( const CHAR *buf,
                                             BSONObjBuilder &builder,
                                             BOOLEAN showDetail )
   {
      INT32 rc          = SDB_OK ;
      vector<string>    splited ;
      vector<BSONObj>   procVec ;

      if ( NULL == buf )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "buf can't be null, rc: %d", rc ) ;
         goto error ;
      }

      /* format:
      USER       PID %CPU %MEM    VSZ   RSS TTY      STAT START   TIME COMMAND
      root         1  0.0  0.0  84096  1352 ?        Ss   Jun12   0:06 /sbin/init
      */
      try
      {
         boost::algorithm::split( splited, buf, boost::is_any_of( "\r\n" ) ) ;
      }
      catch( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                 rc, e.what() ) ;
         goto error ;
      }
      for ( vector<string>::iterator itr = splited.begin();
            itr != splited.end();  )
      {
         if ( itr->empty() )
         {
            itr = splited.erase( itr ) ;
         }
         else
         {
            itr++ ;
         }
      }

      // build obj vector
      if( TRUE == showDetail )
      {
         for ( vector<string>::iterator itrSplit = splited.begin() + 1;
               itrSplit != splited.end(); itrSplit++ )
         {
            vector<string> columns ;
            BSONObjBuilder proObjBuilder ;

            try
            {
               boost::algorithm::split( columns, *itrSplit,
                                        boost::is_any_of(" ") ) ;
            }
            catch( std::exception &e )
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                       rc, e.what() ) ;
               goto error ;
            }
            for ( vector<string>::iterator itrCol = columns.begin();
                  itrCol != columns.end();  )
            {
               if ( itrCol->empty() )
               {
                  itrCol = columns.erase( itrCol ) ;
               }
               else
               {
                  itrCol++ ;
               }
            }

            // result at least contain 4 col
            if ( 4 > columns.size() )
            {
               continue ;
            }

            // filename may contain ' ', need to merge
            for ( UINT32 index = 4; index < columns.size(); index++ )
            {
               columns[ 3 ] += " " + columns[ index ] ;
            }
            proObjBuilder.append( CMD_USR_SYSTEM_PROC_USER, columns[ 0 ] ) ;
            proObjBuilder.append( CMD_USR_SYSTEM_PROC_PID, columns[ 1 ] ) ;
            proObjBuilder.append( CMD_USR_SYSTEM_PROC_STATUS, columns[ 2 ] ) ;
            proObjBuilder.append( CMD_USR_SYSTEM_PROC_CMD, columns[ 3 ] ) ;
            procVec.push_back( proObjBuilder.obj() ) ;
         }
      }
      else
      {
         for ( vector<string>::iterator itrSplit = splited.begin() + 1;
            itrSplit != splited.end(); itrSplit++ )
         {
            vector<string> columns ;
            BSONObjBuilder proObjBuilder ;

            try
            {
               boost::algorithm::split( columns, *itrSplit,
                                        boost::is_any_of(" ") ) ;
            }
            catch( std::exception &e )
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                       rc, e.what() ) ;
               goto error ;
            }
            for ( vector<string>::iterator itrCol = columns.begin();
                  itrCol != columns.end();  )
            {
               if ( itrCol->empty() )
               {
                  itrCol = columns.erase( itrCol ) ;
               }
               else
               {
                  itrCol++ ;
               }
            }

            // result at least contain 4 col
            if ( 4 > columns.size() )
            {
               continue ;
            }

            // filename may contain ' ', need to merge
            for ( UINT32 index = 4; index < columns.size(); index++ )
            {
               columns[ 3 ] += " " + columns[ index ] ;
            }
            proObjBuilder.append( CMD_USR_SYSTEM_PROC_PID, columns[ 1 ] ) ;
            proObjBuilder.append( CMD_USR_SYSTEM_PROC_CMD, columns[ 3 ] ) ;
            procVec.push_back( proObjBuilder.obj() ) ;
         }
      }

      // merge vector< BSONObj > into BsonObj
      for( UINT32 index = 0; index < procVec.size(); index++ )
      {
         try
         {
            builder.append( boost::lexical_cast<string>( index ).c_str(),
                            procVec[ index ] ) ;
         }
         catch( std::exception &e )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Fail to build retObj, rc: %d, detail: %s",
                    rc, e.what() ) ;
            goto error ;
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }

#elif defined (_WINDOWS)
   INT32 _sptUsrSystem::_extractProcessInfo( const CHAR *buf,
                                             BSONObjBuilder &builder,
                                             BOOLEAN showDetail )
   {
      INT32 rc            = SDB_OK ;
      vector<string>      splited ;
      vector< BSONObj >   procVec ;

      if ( NULL == buf )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "buf can't be null, rc: %d", rc ) ;
         goto error ;
      }

      /* format:
      System Idle Process","0","Services","0","24 K"
      */
      try
      {
         boost::algorithm::split( splited, buf, boost::is_any_of("\r\n") ) ;
      }
      catch( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                 rc, e.what() ) ;
         goto error ;
      }
      for ( vector<string>::iterator itr = splited.begin();
            itr != splited.end();  )
      {
         if ( itr->empty() )
         {
            itr = splited.erase( itr ) ;
         }
         else
         {
            itr++ ;
         }
      }

      // build obj vector
      if( TRUE == showDetail )
      {
         for ( vector<string>::iterator itrSplit = splited.begin() + 1;
               itrSplit != splited.end(); itrSplit++ )
         {
            vector<string> columns ;
            BSONObjBuilder proObjBuilder ;

            try
            {
               boost::algorithm::split( columns, *itrSplit,
                                        boost::is_any_of( ",\"" ) ) ;
            }
            catch( std::exception &e )
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                       rc, e.what() ) ;
               goto error ;
            }
            for ( vector<string>::iterator itrCol = columns.begin();
                  itrCol != columns.end();  )
            {
               if ( itrCol->empty() )
               {
                  itrCol = columns.erase( itrCol ) ;
               }
               else
               {
                  itrCol++ ;
               }
            }

            // result must contain 5 col
            if ( 5 != columns.size() )
            {
               continue ;
            }
            proObjBuilder.append( CMD_USR_SYSTEM_PROC_USER, "" ) ;
            proObjBuilder.append( CMD_USR_SYSTEM_PROC_PID, columns[ 1 ] ) ;
            proObjBuilder.append( CMD_USR_SYSTEM_PROC_STATUS, "" ) ;
            proObjBuilder.append( CMD_USR_SYSTEM_PROC_CMD, columns[ 0 ] ) ;
            procVec.push_back( proObjBuilder.obj() ) ;
         }
      }
      else
      {
         for ( vector<string>::iterator itrSplit = splited.begin() + 1;
               itrSplit != splited.end(); itrSplit++ )
         {
            vector<string> columns ;
            BSONObjBuilder proObjBuilder ;

            try
            {
               boost::algorithm::split( columns, *itrSplit,
                                        boost::is_any_of( ",\"" ) ) ;
            }
            catch( std::exception &e )
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                       rc, e.what() ) ;
               goto error ;
            }
            for ( vector<string>::iterator itrCol = columns.begin();
                  itrCol != columns.end();  )
            {
               if ( itrCol->empty() )
               {
                  itrCol = columns.erase( itrCol ) ;
               }
               else
               {
                  itrCol++ ;
               }
            }

            // result must contain 5 col
            if ( 5 != columns.size() )
            {
               continue ;
            }
            proObjBuilder.append( CMD_USR_SYSTEM_PROC_PID, columns[ 1 ] ) ;
            proObjBuilder.append( CMD_USR_SYSTEM_PROC_CMD, columns[ 0 ] ) ;
            procVec.push_back( proObjBuilder.obj() ) ;
         }
      }

      // merge into BsonObj
      for( UINT32 index = 0; index < procVec.size(); index++ )
      {
         try
         {
            builder.append( boost::lexical_cast<string>( index ).c_str(),
                            procVec[ index ] ) ;
         }
         catch( std::exception &e )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Fail to build retObj, rc: %d, detail: %s",
                    rc, e.what() ) ;
            goto error ;
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }
#endif


#if defined (_LINUX)
   INT32 _sptUsrSystem::killProcess( const _sptArguments &arg,
                                     _sptReturnVal &rval,
                                     BSONObj &detail )
   {
      INT32 rc           = SDB_OK ;
      UINT32 exitCode    = 0 ;
      INT32 sigNum       = 15 ;
      BSONObj            optionObj ;
      stringstream       cmd ;
      _ossCmdRunner      runner ;
      string             outStr ;
      string             sig ;

      // check argument
      if ( 1 < arg.argc() )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "killProcess() only have an argument" ) ;
         goto error ;
      }

      rc = arg.getBsonobj( 0, optionObj ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "optionObj must be config" ) ;
      }
      else if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "optionObj must be BsonObj" ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get optionObj, rc: %d", rc ) ;

      if ( TRUE == optionObj.hasField( "sig" ) )
      {
         if ( String != optionObj.getField( "sig" ).type() )
         {
            rc = SDB_INVALIDARG ;
            detail = BSON( SPT_ERR << "sig must be string" ) ;
            goto error ;
         }
         sig = optionObj.getStringField( "sig" ) ;

         if ( "term" != sig && "kill" != sig )
         {
            rc = SDB_INVALIDARG ;
            detail = BSON( SPT_ERR << "sig must be \"term\" or \"kill\"" ) ;
            goto error ;
         }
         else if ( "kill" == sig )
         {
            sigNum = 9 ;
         }
      }

      cmd << "kill -" << sigNum ;

      if ( FALSE == optionObj.hasField( "pid" ) )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "pid must be config" ) ;
      }
      else if ( NumberInt != optionObj.getField( "pid" ).type() )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "pid must be int" ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get pid, rc: %d", rc ) ;
      cmd << " " << optionObj.getIntField( "pid" ) ;

      // run cmd
      rc = runner.exec( cmd.str().c_str(), exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to exec cmd, rc:%d, exit:%d",
                 rc, exitCode ) ;
         stringstream ss ;
         ss << "failed to exec cmd " << cmd.str() << ",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }

      // get result
      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to read result" ) ;
         goto error ;
      }
      else if ( SDB_OK != exitCode )
      {
         rc = exitCode ;
         detail = BSON( SPT_ERR << outStr ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

#elif defined (_WINDOWS)

   INT32 _sptUsrSystem::killProcess( const _sptArguments &arg,
                                     _sptReturnVal &rval,
                                     BSONObj &detail )
   {
      INT32 rc           = SDB_OK ;
      UINT32 exitCode    = 0 ;
      INT32 sigNum       = 15 ;
      BSONObj            optionObj ;
      stringstream       cmd ;
      _ossCmdRunner      runner ;
      string             outStr ;
      string             sig ;

      // check argument and build cmd
      if ( 1 < arg.argc() )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "killProcess() only have an argument" ) ;
         goto error ;
      }

      rc = arg.getBsonobj( 0, optionObj ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "optionObj must be config" ) ;
      }
      else if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "optionObj must be string" ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get optionObj, rc: %d", rc ) ;

      if ( TRUE == optionObj.hasField( "sig" ) )
      {
         if ( String != optionObj.getField( "sig" ).type() )
         {
            rc = SDB_INVALIDARG ;
            detail = BSON( SPT_ERR << "sig must be string" ) ;
            goto error ;
         }
         sig = optionObj.getStringField( "sig" ) ;
         if ( "term" != sig &&
              "kill" != sig )
         {
            rc = SDB_INVALIDARG ;
            detail = BSON( SPT_ERR << "sig must be \"term\" or \"kill\"" ) ;
            goto error ;
         }
         else if ( "kill" == sig )
         {
            sigNum = 9 ;
         }
      }

      cmd << "taskkill" ;
      if ( 9 == sigNum )
      {
         cmd << " /F" ;
      }

      if ( FALSE == optionObj.hasField( "pid" ) )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "pid must be config" ) ;
      }
      else if ( NumberInt != optionObj.getField( "pid" ).type() )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "pid must be int" ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get pid, rc: %d", rc ) ;
      cmd << " /PID " << optionObj.getIntField( "pid" ) ;

      // run cmd
      rc = runner.exec( cmd.str().c_str(), exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to exec cmd, rc:%d, exit:%d",
                 rc, exitCode ) ;
         stringstream ss ;
         ss << "failed to exec cmd " << cmd.str() << ",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }

      // raed result
      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to read result" ) ;
         goto error ;
      }
      if ( SDB_OK != exitCode )
      {
         rc = exitCode ;
         detail = BSON( SPT_ERR << outStr ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }
#endif

   INT32 _sptUsrSystem::addUser( const _sptArguments &arg,
                                 _sptReturnVal &rval,
                                 BSONObj &detail )
   {
#if defined (_LINUX)
      INT32 rc          = SDB_OK ;
      BSONObj           userObj ;
      string            outStr ;
      stringstream      cmd ;
      _ossCmdRunner     runner ;
      UINT32            exitCode ;

      // init cmd
      cmd << "useradd" ;

      // check argument and build cmd
      rc = arg.getBsonobj( 0, userObj ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "userObj must be config" ) ;
      }
      else if ( SDB_INVALIDARG == rc )
      {
         detail = BSON( SPT_ERR << "userObj must be BSONObj" ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get userObj, rc: %d", rc ) ;

      if ( userObj.hasField( "passwd" ) )
      {
         if ( String != userObj.getField( "passwd" ).type() )
         {
            rc = SDB_INVALIDARG ;
            detail = BSON( SPT_ERR << "passwd must be string" ) ;
            goto error ;
         }
         else
         {
            cmd << " -p " << userObj.getStringField( "passwd" ) ;
         }
      }

      if ( userObj.hasField( "group" ) )
      {
         if ( String != userObj.getField( "group" ).type() )
         {
            rc = SDB_INVALIDARG ;
            detail = BSON( SPT_ERR << "group must be string" ) ;
            goto error ;
         }
         else
         {
            cmd << " -g " << userObj.getStringField( "group" ) ;
         }
      }

      if ( userObj.hasField( "Group" ) )
      {
         if ( String != userObj.getField( "Group" ).type() )
         {
            rc = SDB_INVALIDARG ;
            detail = BSON( SPT_ERR << "Group must be string" ) ;
            goto error ;
         }
         else
         {
            cmd << " -G " << userObj.getStringField( "Group" ) ;
         }
      }

      if ( userObj.hasField( "dir" ) )
      {
         if ( String != userObj.getField( "dir" ).type() )
         {
            rc = SDB_INVALIDARG ;
            detail = BSON( SPT_ERR << "dir must be string" ) ;
            goto error ;
         }
         else
         {
            cmd << " -d " << userObj.getStringField( "dir" ) ;
         }
      }

      if ( TRUE == userObj.getBoolField( "createDir" ) )
      {
         cmd << " -m" ;
      }

      if( FALSE == userObj.hasField( "name" ) )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "name must be config" ) ;
      }
      if( String != userObj.getField( "name" ).type() )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "name must be string" ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get name, rc: %d", rc ) ;
      cmd << " " << userObj.getStringField( "name" ) ;

      // run cmd
      rc = runner.exec( cmd.str().c_str(), exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to exec cmd, rc:%d, exit:%d",
                 rc, exitCode ) ;
         stringstream ss ;
         ss << "failed to exec cmd " << cmd.str() << ",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }

      // get result
      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to read result" ) ;
         goto error ;
      }
      else if ( SDB_OK != exitCode )
      {
         rc = exitCode ;
         detail = BSON( SPT_ERR << outStr ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;

#elif defined (_WINDOWS)
      return SDB_OK ;
#endif
   }

   INT32 _sptUsrSystem::addGroup( const _sptArguments &arg,
                                  _sptReturnVal &rval,
                                  BSONObj &detail )
   {
#if defined (_LINUX)
      INT32 rc        = SDB_OK ;
      BSONObj         groupObj ;
      string          outStr ;
      stringstream    cmd ;
      _ossCmdRunner   runner ;
      UINT32          exitCode ;

      // check argument and build cmd
      rc = arg.getBsonobj( 0, groupObj ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "groupObj must be config" ) ;
      }
      else if ( SDB_INVALIDARG == rc )
      {
         detail = BSON( SPT_ERR << "groupObj must be BSONObj" ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get groupObj, rc: %d", rc ) ;

      cmd << "groupadd" ;
      if ( groupObj.hasField( "passwd" ) )
      {
         if ( String != groupObj.getField( "passwd" ).type() )
         {
            rc = SDB_INVALIDARG ;
            detail = BSON( SPT_ERR << "passwd must be string" ) ;
            goto error ;
         }
         else
         {
            cmd << " -p " << groupObj.getStringField( "passwd" ) ;
         }
      }

      if ( groupObj.hasField( "id" ) )
      {
         if ( String != groupObj.getField( "id" ).type() )
         {
            rc = SDB_INVALIDARG ;
            detail = BSON( SPT_ERR << "id must be string" ) ;
            goto error ;
         }
         else
         {
            cmd << " -g " << groupObj.getStringField( "id" ) ;
            if ( TRUE == groupObj.hasField( "isUnique" ) )
            {
               if ( Bool != groupObj.getField( "isUnique" ).type() )
               {
                  rc = SDB_INVALIDARG ;
                  detail = BSON( SPT_ERR << "isUnique must be bool" ) ;
                  goto error ;
               }
               if ( FALSE == groupObj.getBoolField( "isUnique" ) )
               {
                  cmd << " -o" ;
               }
            }
         }
      }

      if( FALSE == groupObj.hasField( "name" ) )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "name must be config" ) ;
      }
      if( String != groupObj.getField( "name" ).type() )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "name must be string" ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get name, rc: %d", rc ) ;
      cmd << " " << groupObj.getStringField( "name" ) ;

      // run cmd
      rc = runner.exec( cmd.str().c_str(), exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to exec cmd, rc:%d, exit:%d",
                 rc, exitCode ) ;
         stringstream ss ;
         ss << "failed to exec cmd " << cmd.str() << ",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }

      // read result
      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to read result" ) ;
         goto error ;
      }
      else if ( SDB_OK != exitCode )
      {
         rc = exitCode ;
         detail = BSON( SPT_ERR << outStr ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;

#elif defined (_WINDOWS)
      return SDB_OK ;
#endif
   }

   INT32 _sptUsrSystem::setUserConfigs( const _sptArguments &arg,
                                        _sptReturnVal &rval,
                                        BSONObj &detail )
   {
#if defined(_LINUX)
      INT32 rc          = SDB_OK ;
      BSONObj           optionObj ;
      string            outStr ;
      stringstream      cmd ;
      _ossCmdRunner     runner ;
      UINT32            exitCode ;

      // check argument and build cmd
      rc = arg.getBsonobj( 0, optionObj ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "optionObj must be config" ) ;
      }
      else if ( SDB_INVALIDARG == rc )
      {
         detail = BSON( SPT_ERR << "optionObj must be BSONObj" ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get optionObj, rc: %d", rc ) ;

      cmd << "usermod" ;
      if ( optionObj.hasField( "passwd" ) )
      {
         if ( String != optionObj.getField( "passwd" ).type() )
         {
            rc = SDB_INVALIDARG ;
            detail = BSON( SPT_ERR << "passwd must be string" ) ;
            goto error ;
         }
         else
         {
            cmd << " -p "
                << optionObj.getStringField( "passwd" ) ;
         }
      }

      if ( optionObj.hasField( "group" ) )
      {
         if ( String != optionObj.getField( "group" ).type() )
         {
            rc = SDB_INVALIDARG ;
            detail = BSON( SPT_ERR << "group must be string" ) ;
            goto error ;
         }
         else
         {
            cmd << " -g "
                << optionObj.getStringField( "group" ) ;
         }
      }

      if ( optionObj.hasField( "Group" ) )
      {
         if ( String != optionObj.getField( "Group" ).type() )
         {
            rc = SDB_INVALIDARG ;
            detail = BSON( SPT_ERR << "Group must be string" ) ;
            goto error ;
         }
         else
         {
            cmd << " -G "
                << optionObj.getStringField( "Group" ) ;


            if ( TRUE == optionObj.getBoolField( "isAppend" ) )
            {
               cmd << " -a" ;
            }
         }
      }

      if ( optionObj.hasField( "dir" ) )
      {
         if ( String != optionObj.getField( "dir" ).type() )
         {
            rc = SDB_INVALIDARG ;
            detail = BSON( SPT_ERR << "dir must be string" ) ;
            goto error ;
         }
         else
         {
            cmd << " -d "
                << optionObj.getStringField( "dir" ) ;

            if ( TRUE == optionObj.getBoolField( "isMove" ) )
            {
               cmd << " -m" ;
            }
         }
      }

      if( FALSE == optionObj.hasField( "name" ) )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "name must be config" ) ;
      }
      if( String != optionObj.getField( "name" ).type() )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "name must be string" ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get name, rc: %d", rc ) ;
      cmd << " " << optionObj.getStringField( "name" ) ;

      // run cmd
      rc = runner.exec( cmd.str().c_str(), exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to exec cmd, rc:%d, exit:%d",
                 rc, exitCode ) ;
         stringstream ss ;
         ss << "failed to exec cmd " << cmd.str() << ",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }

      // read result
      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to read result" ) ;
         goto error ;
      }
      else if ( SDB_OK != exitCode )
      {
         rc = exitCode ;
         detail = BSON( SPT_ERR << outStr ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;

#elif defined (_WINDOWS)
      return SDB_OK ;
#endif
   }

   INT32 _sptUsrSystem::delUser( const _sptArguments & arg,
                                 _sptReturnVal & rval,
                                 BSONObj & detail )
   {
#if defined (_LINUX)
      INT32 rc          = SDB_OK ;
      BSONObj           optionObj ;
      string            outStr ;
      stringstream      cmd ;
      _ossCmdRunner     runner ;
      UINT32            exitCode ;

      // check argument and build cmd
      rc = arg.getBsonobj( 0, optionObj ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "optionObj must be config" ) ;
      }
      else if ( SDB_INVALIDARG == rc )
      {
         detail = BSON( SPT_ERR << "optionObj must be BSONObj" ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get optionObj, rc: %d", rc ) ;

      cmd << "userdel" ;
      if ( optionObj.hasField( "isRemoveDir" ) )
      {
         if ( Bool != optionObj.getField( "isRemoveDir" ).type() )
         {
            rc = SDB_INVALIDARG ;
            detail = BSON( SPT_ERR << "isRemoveDir must be bool" ) ;
            goto error ;
         }
         else if ( optionObj.getBoolField( "isRemoveDir" ) )
         {
            cmd << " -r" ;
         }
      }

      if( FALSE == optionObj.hasField( "name" ) )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "name must be config" ) ;
      }
      if( String != optionObj.getField( "name" ).type() )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "name must be string" ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get name, rc: %d", rc ) ;
      cmd << " " << optionObj.getStringField( "name" ) ;

      // run cmd
      rc = runner.exec( cmd.str().c_str(), exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to exec cmd, rc:%d, exit: %d",
                 rc, exitCode ) ;
         stringstream ss ;
         ss << "failed to exec cmd " << cmd.str() << ",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }

      // read result
      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to read result" ) ;
         goto error ;
      }
      else if ( SDB_OK != exitCode )
      {
         rc = exitCode ;
         detail = BSON( SPT_ERR << outStr ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;

#elif defined (_WINDOWS)
      return SDB_OK ;
#endif
   }

   INT32 _sptUsrSystem::delGroup( const _sptArguments & arg,
                                  _sptReturnVal & rval,
                                  BSONObj & detail )
   {
#if defined (_LINUX)
      INT32 rc          = SDB_OK ;
      BSONObj           optionObj ;
      string            name ;
      string            outStr ;
      stringstream      cmd ;
      _ossCmdRunner     runner ;
      UINT32            exitCode ;

      // check argument and build cmd
      cmd << "groupdel" ;
      rc = arg.getString( 0, name ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "name must be config" ) ;
      }
      else if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "name must be string" ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get name, rc: %d", rc ) ;
      cmd << " " << name ;

      // run cmd
      rc = runner.exec( cmd.str().c_str(), exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to exec cmd, rc:%d, exit:%d",
                 rc, exitCode ) ;
         stringstream ss ;
         ss << "failed to exec cmd " << cmd.str() << ",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }

      // read result
      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to read result" ) ;
         goto error ;
      }
      else if ( SDB_OK != exitCode )
      {
         rc = exitCode ;
         detail = BSON( SPT_ERR << outStr ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;

#elif defined (_WINDOWS)
      return SDB_OK ;
#endif
   }

   INT32 _sptUsrSystem::listLoginUsers( const _sptArguments & arg,
                                        _sptReturnVal & rval,
                                        BSONObj & detail )
   {
#if defined (_LINUX)
      INT32 rc           = SDB_OK ;
      BSONObjBuilder     builder ;
      BOOLEAN showDetail = FALSE ;
      UINT32 exitCode    = 0 ;
      BSONObj            optionObj ;
      stringstream       cmd ;
      _ossCmdRunner      runner ;
      string             outStr ;

      // check argument and build cmd
      cmd << "who" ;
      if ( 1 < arg.argc() )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "too much arguments" ) ;
         goto error ;
      }
      if ( 1 == arg.argc() )
      {
         rc = arg.getBsonobj( 0, optionObj ) ;
         if ( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "optionObj must be config" ) ;
            goto error ;
         }
         showDetail = optionObj.getBoolField( "detail" ) ;
      }

      // run cmd
      rc = runner.exec( cmd.str().c_str(), exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to exec cmd, rc:%d, exit:%d",
                 rc, exitCode ) ;
         stringstream ss ;
         ss << "failed to exec cmd " << cmd.str() << ",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }

      // read result
      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to read msg from cmd runner:%d", rc ) ;
         stringstream ss ;
         ss << "failed to read msg from cmd \"" << cmd.str() << "\", rc:"
            << rc ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }

      // extract result
      rc = _extractLoginUsersInfo( outStr.c_str(), builder, showDetail ) ;
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to extract login user info" ) ;
         goto error ;
      }
      rval.getReturnVal().setValue( builder.obj() ) ;
   done:
      return rc ;
   error:
      goto done ;

#elif defined (_WINDOWS)
      rval.getReturnVal().setValue( BSONObj() ) ;
      return SDB_OK ;
#endif
   }

   INT32 _sptUsrSystem::_extractLoginUsersInfo( const CHAR *buf,
                                               BSONObjBuilder &builder,
                                               BOOLEAN showDetail )
   {
      INT32 rc            = SDB_OK ;
      vector<string>      splited ;
      vector< BSONObj >   userVec ;

      if ( NULL == buf )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "buf can't be null, rc: %d", rc ) ;
         goto error ;
      }

      /* format:
            xxxxxxxxx tty          2016-10-11 13:01
         or
            xxxxxxxxx pts/0        2016-10-11 13:01 (xxx.xxx.xxx.xxx)
         or
            xxxxxxxxx pts/1        Dec  2 11:44     (192.168.10.53)

      */
      try
      {
         boost::algorithm::split( splited, buf, boost::is_any_of( "\r\n" ) ) ;
      }
      catch( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                 rc, e.what() ) ;
         goto error ;
      }
      for ( vector<string>::iterator itr = splited.begin();
            itr != splited.end(); )
      {
         if ( itr->empty() )
         {
            itr = splited.erase( itr ) ;
         }
         else
         {
            itr++ ;
         }
      }

      // build obj vector
      if( TRUE == showDetail )
      {
         for ( vector<string>::iterator itrSplit = splited.begin();
               itrSplit != splited.end(); itrSplit++ )
         {
            vector<string> columns ;
            BSONObjBuilder userObjBuilder ;
            string loginIp ;
            string loginTime ;
            try
            {
               boost::algorithm::split( columns, *itrSplit,
                                        boost::is_any_of(" ") ) ;
            }
            catch( std::exception &e )
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                       rc, e.what() ) ;
               goto error ;
            }
            for ( vector<string>::iterator itrCol = columns.begin();
                  itrCol != columns.end();  )
            {
               if ( itrCol->empty() )
               {
                  itrCol = columns.erase( itrCol ) ;
               }
               else
               {
                  itrCol++ ;
               }
            }

            // at least contain 4 col
            if ( 4 > columns.size() )
            {
               continue ;
            }
            else
            {
               string &ipStr = columns.back() ;
               if ( ipStr[ ipStr.size() - 1 ] == ')' )
               {
                  loginIp = ipStr.substr( 1, ipStr.size() - 2 );
                  loginTime = columns[ 2 ] ;
                  for ( UINT32 index = 3; index < columns.size() - 1; index++ )
                  {
                     loginTime += " " + columns[ index ] ;
                  }
               }
               else
               {
                  loginIp = "" ;
                  loginTime = columns[ 2 ] ;
                  for( UINT32 index = 3; index < columns.size(); index++ )
                  {
                     loginTime += " " + columns[ index ] ;
                  }
               }
            }
            userObjBuilder.append( CMD_USR_SYSTEM_LOGINUSER_USER, columns[ 0 ] ) ;
            userObjBuilder.append( CMD_USR_SYSTEM_LOGINUSER_TIME, loginTime ) ;
            userObjBuilder.append( CMD_USR_SYSTEM_LOGINUSER_FROM, loginIp ) ;
            userObjBuilder.append( CMD_USR_SYSTEM_LOGINUSER_TTY, columns[ 1 ] ) ;
            userVec.push_back( userObjBuilder.obj() ) ;
         }
      }
      else
      {
         for ( vector<string>::iterator itrSplit = splited.begin();
            itrSplit != splited.end(); itrSplit++ )
         {
            vector<string> columns ;
            BSONObjBuilder userObjBuilder ;

            try
            {
               boost::algorithm::split( columns, *itrSplit,
                                        boost::is_any_of(" ") ) ;
            }
            catch( std::exception &e )
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                       rc, e.what() ) ;
               goto error ;
            }
            for ( vector<string>::iterator itrCol = columns.begin();
                  itrCol != columns.end();  )
            {
               if ( itrCol->empty() )
               {
                  itrCol = columns.erase( itrCol ) ;
               }
               else
               {
                  itrCol++ ;
               }
            }

            // at least contain 4 col
            if ( 4 > columns.size() )
            {
               continue ;
            }
            userObjBuilder.append( CMD_USR_SYSTEM_LOGINUSER_USER,
                                  columns[ 0 ] ) ;
            userVec.push_back( userObjBuilder.obj() ) ;
         }
      }

      // merge vector< BSONObj > into BsonObj
      for( UINT32 index = 0; index < userVec.size(); index++ )
      {
         try
         {
            builder.append( boost::lexical_cast<string>( index ).c_str(),
                            userVec[ index ] ) ;
         }
         catch( std::exception &e )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Fail to build retObj, rc: %d, detail: %s",
                    rc, e.what() ) ;
            goto error ;
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrSystem::listAllUsers( const _sptArguments & arg,
                                      _sptReturnVal & rval,
                                      BSONObj & detail )
   {
#if defined (_LINUX)
      INT32 rc           = SDB_OK ;
      BSONObjBuilder     builder ;
      BOOLEAN showDetail = FALSE ;
      UINT32 exitCode    = 0 ;
      string             outStr ;
      BSONObj            optionObj ;
      stringstream       cmd ;
      _ossCmdRunner      runner ;

      // check argument and build cmd
      cmd << "cat /etc/passwd" ;
      if ( 1 < arg.argc() )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "too much arguments" ) ;
         goto error ;
      }
      if ( 1 == arg.argc() )
      {
         rc = arg.getBsonobj( 0, optionObj ) ;
         if ( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "optionObj must be config" ) ;
            goto error ;
         }
         showDetail = optionObj.getBoolField( "detail" ) ;
      }

      // run cmd
      rc = runner.exec( cmd.str().c_str(), exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to exec cmd, rc:%d, exit:%d",
                 rc, exitCode ) ;
         stringstream ss ;
         ss << "failed to exec cmd " << cmd.str() << ",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }

      // get result
      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to read msg from cmd runner:%d", rc ) ;
         stringstream ss ;
         ss << "failed to read msg from cmd \"" << cmd.str() << "\", rc:"
            << rc ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }

      // extract result
      rc = _extractAllUsersInfo( outStr.c_str(), builder, showDetail ) ;
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR <<"Failed to extract all users info" ) ;
         goto error ;
      }
      rval.getReturnVal().setValue( builder.obj() ) ;
   done:
      return rc ;
   error:
      goto done ;

#elif defined (_WINDOWS)
      rval.getReturnVal().setValue( BSONObj() ) ;
      return SDB_OK ;
#endif
   }

   INT32 _sptUsrSystem::_extractAllUsersInfo( const CHAR *buf,
                                              BSONObjBuilder &builder,
                                              BOOLEAN showDetail )
   {
      INT32 rc           = SDB_OK ;
      vector<string>     splited ;
      vector< BSONObj >  userVec ;

      if ( NULL == buf )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "buf can't be null, rc: %d", rc ) ;
         goto error ;
      }

      /* format:
         root:x:0:0:root:/root:/bin/bash
      */
      try
      {
         boost::algorithm::split( splited, buf, boost::is_any_of("\r\n") ) ;
      }
      catch( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                 rc, e.what() ) ;
         goto error ;
      }
      for ( vector<string>::iterator itr = splited.begin();
            itr != splited.end();  )
      {
         if ( itr->empty() )
         {
            itr = splited.erase( itr ) ;
         }
         else
         {
            itr++ ;
         }
      }

      // build obj vector
      if( TRUE == showDetail )
      {
         for ( vector<string>::iterator itrSplit = splited.begin();
               itrSplit != splited.end(); itrSplit++ )
         {
            vector<string> columns ;
            BSONObjBuilder userObjBuilder ;

            try
            {
               boost::algorithm::split( columns, *itrSplit,
                                        boost::is_any_of( ":" ) ) ;
            }
            catch( std::exception &e )
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                       rc, e.what() ) ;
               goto error ;
            }

            // if no match format
            if ( columns.size() != 7 )
            {
               continue ;
            }
            userObjBuilder.append( CMD_USR_SYSTEM_ALLUSER_USER, columns[ 0 ] ) ;
            userObjBuilder.append( CMD_USR_SYSTEM_ALLUSER_GID, columns[ 3 ] ) ;
            userObjBuilder.append( CMD_USR_SYSTEM_ALLUSER_DIR, columns[ 5 ] ) ;
            userVec.push_back( userObjBuilder.obj() ) ;
         }
      }
      else
      {
         for ( vector<string>::iterator itrSplit = splited.begin();
               itrSplit != splited.end(); itrSplit++ )
         {
            vector<string> columns ;
            BSONObjBuilder userObjBuilder ;

            try
            {
               boost::algorithm::split( columns, *itrSplit,
                                        boost::is_any_of( ":" ) ) ;
            }
            catch( std::exception &e )
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                       rc, e.what() ) ;
               goto error ;
            }

            // if no match format
            if ( columns.size() != 7 )
            {
               continue ;
            }
            userObjBuilder.append( CMD_USR_SYSTEM_ALLUSER_USER, columns[ 0 ] ) ;
            userVec.push_back( userObjBuilder.obj() ) ;
         }
      }

      // merge vector< BSONObj > into BsonObj
      for( UINT32 index = 0; index < userVec.size(); index++ )
      {
         try
         {
            builder.append( boost::lexical_cast<string>( index ).c_str(),
                            userVec[ index ] ) ;
         }
         catch( std::exception &e )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Fail to build retObj, rc: %d, detail: %s",
                    rc, e.what() ) ;
            goto error ;
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrSystem::listGroups( const _sptArguments & arg,
                                    _sptReturnVal & rval,
                                    BSONObj & detail )
   {
#if defined (_LINUX)
      INT32 rc           = SDB_OK ;
      BSONObjBuilder     builder ;
      BOOLEAN showDetail = FALSE ;
      UINT32 exitCode    = 0 ;
      BSONObj            optionObj ;
      stringstream       cmd ;
      _ossCmdRunner      runner ;
      string             outStr ;

      // check argument and build cmd
      cmd << "cat /etc/group" ;
      if ( 1 < arg.argc() )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "too much arguments" ) ;
         goto error ;
      }
      if ( 1 == arg.argc() )
      {
         rc = arg.getBsonobj( 0, optionObj ) ;
         if ( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "optionObj must be config" ) ;
            goto error ;
         }
         showDetail = optionObj.getBoolField( "detail" ) ;
      }

      // run cmd
      rc = runner.exec( cmd.str().c_str(), exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to exec cmd, rc:%d, exit:%d",
                 rc, exitCode ) ;
         stringstream ss ;
         ss << "failed to exec cmd " << cmd.str() << ",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }

      // get result
      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to read msg from cmd runner:%d", rc ) ;
         stringstream ss ;
         ss << "failed to read msg from cmd \"" << cmd.str() << "\", rc:"
            << rc ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }

      // extract result
      rc = _extractGroupsInfo( outStr.c_str(), builder, showDetail ) ;
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to extract group info" ) ;
         goto error ;
      }
      rval.getReturnVal().setValue( builder.obj() ) ;
   done:
      return rc ;
   error:
      goto done ;

#elif defined (_WINDOWS)
      rval.getReturnVal().setValue( BSONObj() ) ;
      return SDB_OK ;
#endif
   }

   INT32 _sptUsrSystem::_extractGroupsInfo( const CHAR *buf,
                                            BSONObjBuilder &builder,
                                            BOOLEAN showDetail )
   {
      INT32 rc           = SDB_OK ;
      vector<string>     splited ;
      vector< BSONObj >  groupVec ;

      if ( NULL == buf )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "buf can't be null, rc: %d", rc ) ;
         goto error ;
      }

      /* format:
         cdrom:x:24:sequoiadb
      */
      try
      {
         boost::algorithm::split( splited, buf, boost::is_any_of("\r\n") ) ;
      }
      catch( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                 rc, e.what() ) ;
         goto error ;
      }
      for ( vector<string>::iterator itr = splited.begin();
            itr != splited.end();  )
      {
         if ( itr->empty() )
         {
            itr = splited.erase( itr ) ;
         }
         else
         {
            itr++ ;
         }
      }

      // build obj vector
      if( TRUE == showDetail )
      {
         for ( vector<string>::iterator itrSplit = splited.begin();
               itrSplit != splited.end(); itrSplit++ )
         {
            vector<string> columns ;
            BSONObjBuilder groupObjBuilder ;
            string groupMem ;

            try
            {
               boost::algorithm::split( columns, *itrSplit,
                                        boost::is_any_of(":") ) ;
            }
            catch( std::exception &e )
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                       rc, e.what() ) ;
               goto error ;
            }
            for ( vector<string>::iterator itrCol = columns.begin();
                  itrCol != columns.end();  )
            {
               if ( itrCol->empty() )
               {
                  itrCol = columns.erase( itrCol ) ;
               }
               else
               {
                  itrCol++ ;
               }
            }

            // if no match format
            if ( columns.size() < 3 )
            {
               continue ;
            }

            // if group has list of user
            if ( columns.size() == 4 )
            {
               groupMem = columns[ 3 ] ;
            }
            else
            {
               groupMem = "" ;
            }
            groupObjBuilder.append( CMD_USR_SYSTEM_GROUP_NAME, columns[ 0 ] ) ;
            groupObjBuilder.append( CMD_USR_SYSTEM_GROUP_GID, columns[ 2 ] ) ;
            groupObjBuilder.append( CMD_USR_SYSTEM_GROUP_MEMBERS, groupMem ) ;
            groupVec.push_back( groupObjBuilder.obj() ) ;
         }
      }
      else
      {
         for ( vector<string>::iterator itrSplit = splited.begin() ;
               itrSplit != splited.end(); itrSplit++ )
         {
            vector<string> columns ;
            BSONObjBuilder groupObjBuilder ;

            try
            {
               boost::algorithm::split( columns, *itrSplit,
                                        boost::is_any_of( ":" ) ) ;
            }
            catch( std::exception &e )
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                       rc, e.what() ) ;
               goto error ;
            }
            for ( vector<string>::iterator itrCol = columns.begin();
                  itrCol != columns.end();  )
            {
               if ( itrCol->empty() )
               {
                  itrCol = columns.erase( itrCol ) ;
               }
               else
               {
                  itrCol++ ;
               }
            }

            // if no match format
            if ( columns.size() < 3 )
            {
               continue ;
            }
            groupObjBuilder.append( CMD_USR_SYSTEM_GROUP_NAME, columns[ 0 ] ) ;
            groupVec.push_back( groupObjBuilder.obj() ) ;
         }
      }

      // merge into BsonObj
      for( UINT32 index = 0; index < groupVec.size(); index++ )
      {
         try
         {
            builder.append( boost::lexical_cast<string>( index ).c_str(),
                            groupVec[ index ] ) ;
         }
         catch( std::exception &e )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Fail to build retObj, rc: %d, detail: %s",
                    rc, e.what() ) ;
            goto error ;
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrSystem::getCurrentUser( const _sptArguments & arg,
                                        _sptReturnVal & rval,
                                        BSONObj & detail )
   {
#if defined (_LINUX)
      INT32 rc           = SDB_OK ;
      BSONObjBuilder     builder ;
      UINT32 exitCode    = 0 ;
      stringstream       cmd ;
      stringstream       gidStr ;
      _ossCmdRunner      runner ;
      string             username ;
      string             homeDir ;
      OSSUID             uid ;
      OSSGID             gid ;

      cmd << "whoami 2>/dev/null" ;
      // run command
      rc = runner.exec( cmd.str().c_str(), exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to exec cmd, rc:%d, exit:%d",
                 rc, exitCode ) ;
         stringstream ss ;
         ss << "failed to exec cmd " << cmd.str() << ",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }

      // read result
      rc = runner.read( username ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to read msg from cmd runner:%d", rc ) ;
         stringstream ss ;
         ss << "failed to read msg from cmd \"" << cmd.str() << "\", rc:"
            << rc ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }
      if( username[ username.size() - 1 ] == '\n' )
      {
         username.erase( username.size()-1, 1 ) ;
      }

      // get user info
      rc = ossGetUserInfo( username.c_str(), uid, gid ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get gid" ) ;
      }
      else
      {
         gidStr << gid ;
      }

      // get home dir
      rc = _getHomePath( homeDir ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get home path" ) ;
         homeDir = "" ;
      }

      builder.append( "user", username ) ;
      builder.append( "gid", gidStr.str() ) ;
      builder.append( "dir", homeDir ) ;
      rval.getReturnVal().setValue( builder.obj() ) ;
   done:
      return rc ;
   error:
      goto done ;

#elif defined (_WINDOWS)
      rval.getReturnVal().setValue( BSONObj() ) ;
      return SDB_OK ;
#endif
   }

   INT32 _sptUsrSystem::getSystemConfigs( const _sptArguments &arg,
                                          _sptReturnVal &rval,
                                          BSONObj &detail )
   {
#if defined (_LINUX)
      INT32 rc           = SDB_OK ;
      BSONObjBuilder     builder ;
      string             type ;
      vector<string>     typeSplit ;
      string configsType[] = { "kernel", "vm", "fs", "debug", "dev", "abi",
                               "net" } ;

      if ( 0 < arg.argc() )
      {
         rc = arg.getString( 0, type) ;
         if ( SDB_OUT_OF_BOUND == rc )
         {
            detail = BSON( SPT_ERR << "type must be config" ) ;
         }
         else if ( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "type must be string" ) ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to get type, rc: %d", rc ) ;

         // input format: "xxx|xxxx|xxx"
         try
         {
            boost::algorithm::split( typeSplit, type, boost::is_any_of( " |" ) ) ;
         }
         catch( std::exception )
         {
            rc = SDB_SYS ;
            detail = BSON( SPT_ERR << "Failed to split result" ) ;
            PD_LOG( PDERROR, "Failed to split result" ) ;
            goto error ;
         }

         // if specify all type, remove other type
         if( typeSplit.end() != find( typeSplit.begin(), typeSplit.end(),
                                      "all" ) )
         {
            typeSplit.clear() ;
            typeSplit.push_back( "" ) ;
         }
         else
         {
            for( vector< string >::iterator itr = typeSplit.begin();
                 itr != typeSplit.end(); )
            {
               // if not required type ,erase it
               if( configsType + 7 == find( configsType,
                                            configsType + 7,
                                            *itr ) )
               {
                  itr = typeSplit.erase( itr ) ;
               }
               else
               {
                  itr++ ;
               }
            }
         }
      }
      else
      {
         typeSplit.push_back( "" ) ;
      }

      rc = _getSystemInfo( typeSplit, builder ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to get system info" ) ;
         goto error ;
      }

      rval.getReturnVal().setValue( builder.obj() ) ;
   done:
      return rc ;
   error:
      goto done ;

#elif defined (_WINDOWS)
      rval.getReturnVal().setValue( BSONObj() ) ;
      return SDB_OK ;
#endif
   }

   INT32 _sptUsrSystem::_getSystemInfo( vector< string > typeSplit,
                                        BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      INT32 readLen = 1024 ;
      INT32 bufLen = readLen + 1 ;
      multimap< string, string > fileMap ;
      vector< string > keySplit ;
      vector< string > valueSplit ;
      CHAR *buf = NULL ;

      buf = (CHAR*) SDB_OSS_MALLOC( bufLen ) ;
      if( NULL == buf )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "buf malloc failed" ) ;
         goto error ;
      }

      for( vector< string >::iterator itr = typeSplit.begin();
           itr != typeSplit.end(); itr++ )
      {
         string searchDir = "/proc/sys/" + (*itr) ;
         rc = ossAccess( searchDir.c_str() ) ;
         if( SDB_OK != rc )
         {
            rc = SDB_OK ;
            continue ;
         }

         rc = ossEnumFiles( searchDir, fileMap, "", 10 ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }

      for( multimap< string, string >::iterator itr = fileMap.begin();
           itr != fileMap.end();
           itr++ )
      {
         ossPrimitiveFileOp op ;
         string key ;
         string value ;

         // open file
         rc = op.Open( itr->second.c_str(), OSS_PRIMITIVE_FILE_OP_READ_ONLY ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDWARNING, "Can't open file: %s", itr->second.c_str() ) ;
            continue ;
         }

         // read file content
         {
            INT32 readByte = 0 ;
            INT32 hasRead = 0 ;
            INT32 increaseLen = 1024 ;
            CHAR *curPos = buf ;
            BOOLEAN finishRead = FALSE ;
            BOOLEAN isReadSuccess = TRUE ;

            while( !finishRead )
            {
               rc = op.Read( readLen , curPos , &readByte ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "Failed to read file: %s",
                          itr->second.c_str() ) ;
                  isReadSuccess = FALSE ;
                  break ;
               }
               hasRead += readByte ;

               // mem not enough, need to realloc, newBuffSize = 2*oldBuffSize
               if ( readByte == readLen )
               {
                  bufLen += increaseLen ;
                  buf = (CHAR*) SDB_OSS_REALLOC( buf, bufLen ) ;
                  if ( NULL == buf )
                  {
                     rc = SDB_OOM ;
                     PD_LOG( PDERROR, "Failed to realloc buff" ) ;
                     goto error ;
                  }
                  curPos = buf + hasRead ;
                  readLen = increaseLen ;
                  increaseLen *= 2 ;
               }
               else
               {
                  finishRead = TRUE ;
               }
            }
            if( FALSE == isReadSuccess )
            {
               continue ;
            }
            buf[ hasRead ] = '\0' ;
         }

         // split key
         try
         {
            boost::algorithm::split( keySplit, itr->second,
                                     boost::is_any_of( "/" ) ) ;
         }
         catch( std::exception &e )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                    rc, e.what() ) ;
            goto error ;
         }

         for( std::vector< string >::iterator vecItr = keySplit.begin();
              vecItr != keySplit.end(); )
         {
            if ( vecItr->empty() )
            {
               vecItr = keySplit.erase( vecItr ) ;
            }
            else
            {
               vecItr++ ;
            }
         }

         if ( keySplit.size() < 3 )
         {
            continue ;
         }

         key = *( keySplit.begin()+2 ) ;
         for( std::vector< string >::iterator vecItr = keySplit.begin()+3;
              vecItr != keySplit.end(); vecItr++ )
         {
            key += "." + ( *vecItr ) ;
         }

         // split value
         try
         {
            boost::algorithm::split( valueSplit, buf,
                                     boost::is_any_of( "\r\n" ) ) ;
         }
         catch( std::exception &e )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                    rc, e.what() ) ;
            goto error ;
         }

         for( std::vector< string >::iterator vecItr = valueSplit.begin();
              vecItr != valueSplit.end(); )
         {
            if ( vecItr->empty() )
            {
               vecItr = valueSplit.erase( vecItr ) ;
            }
            else
            {
               boost::replace_all( *vecItr, "\t", "    " ) ;
               vecItr++ ;
            }
         }

         if ( 0 == valueSplit.size() )
         {
            continue ;
         }
         value = *( valueSplit.begin() ) ;
         for( std::vector< string >::iterator vecItr = valueSplit.begin()+1;
              vecItr != valueSplit.end(); vecItr++ )
         {
            value += ";" + ( *vecItr ) ;
         }
         builder.append( key, value ) ;
      }
   done:
      SDB_OSS_FREE( buf ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrSystem::getProcUlimitConfigs( const _sptArguments &arg,
                                              _sptReturnVal &rval,
                                              BSONObj &detail )
   {
#if defined (_LINUX)
      INT32 rc               = SDB_OK ;
      BSONObjBuilder         builder ;
      INT32 resourceType[] = { RLIMIT_CORE, RLIMIT_DATA, RLIMIT_NICE,
                               RLIMIT_FSIZE, RLIMIT_SIGPENDING, RLIMIT_MEMLOCK,
                               RLIMIT_RSS, RLIMIT_NOFILE, RLIMIT_MSGQUEUE,
                               RLIMIT_RTPRIO, RLIMIT_STACK, RLIMIT_CPU,
                               RLIMIT_NPROC, RLIMIT_AS, RLIMIT_LOCKS } ;
      char *resourceName[] = { "core_file_size", "data_seg_size",
                               "scheduling_priority", "file_size",
                               "pending_signals", "max_locked_memory",
                               "max_memory_size", "open_files",
                               "POSIX_message_queues", "realtime_priority",
                               "stack_size", "cpu_time", "max_user_processes",
                               "virtual_memory", "file_locks" } ;
      stringstream           cmd ;
      _ossCmdRunner          runner ;
      string                 outStr ;

      // check argument
      if ( 1 <= arg.argc() )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "getUlimitConfigs() should have non arguments" ) ;
         goto error ;
      }

      // get ulimit
      for ( UINT32 index = 0; index < CMD_RESOURCE_NUM; index++ )
      {
         rlimit rlim ;
         if ( 0 != getrlimit( resourceType[ index ], &rlim ) )
         {
            rc = SDB_SYS ;
            detail = BSON( SPT_ERR << "Failed to get user limit info" ) ;
            goto error ;
         }

         // -1 mean unlimited
         // if val > max exact int or val == ulimited, append as string
         if ( OSS_SINT64_JS_MAX < (UINT64)rlim.rlim_cur &&
              (UINT64)-1 != rlim.rlim_cur )
         {
            builder.append( resourceName[ index ],
                            boost::lexical_cast<string>( rlim.rlim_cur ) ) ;
         }
         else
         {
            builder.append( resourceName[ index ], (INT64)rlim.rlim_cur ) ;
         }
      }
      rval.getReturnVal().setValue( builder.obj() ) ;
   done:
      return rc ;
   error:
      goto done ;

#elif defined (_WINDOWS)
      rval.getReturnVal().setValue( BSONObj() ) ;
      return SDB_OK ;
#endif
   }

   INT32 _sptUsrSystem::setProcUlimitConfigs( const _sptArguments &arg,
                                              _sptReturnVal &rval,
                                              BSONObj &detail )
   {
#if defined (_LINUX)
      INT32 rc           = SDB_OK ;
      BSONObj            configsObj ;
      INT32 resourceType[] = { RLIMIT_CORE, RLIMIT_DATA, RLIMIT_NICE,
                               RLIMIT_FSIZE, RLIMIT_SIGPENDING, RLIMIT_MEMLOCK,
                               RLIMIT_RSS, RLIMIT_NOFILE, RLIMIT_MSGQUEUE,
                               RLIMIT_RTPRIO, RLIMIT_STACK, RLIMIT_CPU,
                               RLIMIT_NPROC, RLIMIT_AS, RLIMIT_LOCKS } ;
      char *resourceName[] = { "core_file_size", "data_seg_size",
                               "scheduling_priority", "file_size",
                               "pending_signals", "max_locked_memory",
                               "max_memory_size", "open_files",
                               "POSIX_message_queues", "realtime_priority",
                               "stack_size", "cpu_time", "max_user_processes",
                               "virtual_memory", "file_locks" } ;
      // get argument
      rc = arg.getBsonobj( 0, configsObj ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "configsObj must be config" ) ;
         goto error ;
      }
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "configsObj must be obj" ) ;
         goto error ;
      }

      // set ulimit
      for ( UINT32 index = 0; index < CMD_RESOURCE_NUM; index++ )
      {
         char *limitItem = resourceName[ index ] ;
         if ( configsObj[ limitItem ].ok() )
         {
            // support string type or number type
            if( FALSE == configsObj.getField( limitItem ).isNumber() &&
                String != configsObj.getField( limitItem ).type() )
            {
               rc = SDB_INVALIDARG ;
               detail = BSON( SPT_ERR << "value must be number or string" ) ;
               goto error ;
            }

            rlimit rlim ;
            if ( 0 != getrlimit( resourceType[ index ], &rlim ) )
            {
               rc = SDB_SYS ;
               detail = BSON( SPT_ERR << "Failed to get user limit info" ) ;
               goto error ;
            }

            if ( configsObj.getField( limitItem ).isNumber() )
            {
               rlim.rlim_cur = (UINT64)configsObj.getField( limitItem ).numberLong() ;
            }
            else
            {
               try
               {
                  string valStr = configsObj.getStringField( limitItem ) ;
                  rlim.rlim_cur = boost::lexical_cast<UINT64>( valStr ) ;
               }
               catch( std::exception &e )
               {
                  rc = SDB_INVALIDARG ;
                  stringstream ss ;
                  ss << configsObj.getStringField( limitItem )
                     << " could not be interpreted as number" ;
                  detail = BSON( SPT_ERR << ss.str().c_str() ) ;
                  goto error ;
               }
            }

            if ( 0 != setrlimit( resourceType[ index ], &rlim ) )
            {
               if ( EINVAL == errno )
               {
                  rc = SDB_INVALIDARG ;
                  detail = BSON( SPT_ERR << "Invalid argument: " +
                                 string( limitItem ) ) ;
                  PD_LOG( PDERROR, "Invalid argument, argument: %s",
                          limitItem ) ;
                  goto error ;
               }
               else if ( EPERM == errno )
               {
                  rc = SDB_PERM ;
                  detail = BSON( SPT_ERR << "Permission error" ) ;
                  PD_LOG( PDERROR, "Permission error" ) ;
                  goto error ;
               }
               else
               {
                  rc = SDB_SYS ;
                  detail = BSON( SPT_ERR << "Failed to set ulimit configs" ) ;
                  PD_LOG( PDERROR, "Failed to set ulimit configs" ) ;
                  goto error ;
               }
            }
         }
      }
   done:
      return rc ;
   error:
      goto done ;

#elif defined (_WINDOWS)
      return SDB_OK ;
#endif
   }

   INT32 _sptUsrSystem::runService( const _sptArguments &arg,
                                    _sptReturnVal &rval,
                                    BSONObj &detail )
   {
      INT32 rc           = SDB_OK ;
      UINT32 exitCode    = 0 ;
      stringstream       cmd ;
      _ossCmdRunner      runner ;
      string             serviceName ;
      string             command ;
      string             options ;
      string             outStr ;

      // check argument
      rc = arg.getString( 0, serviceName ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "serviceName must be config" ) ;
      }
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "serviceName must be string" ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get serviceName, rc: %d", rc ) ;

      rc = arg.getString( 1, command ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "command must be config" ) ;
      }
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "command must be string" ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get command, rc: %d", rc ) ;

#if defined (_LINUX)
      cmd << "service " << serviceName << " " << command ;
#elif defined (_WINDOWS)
      cmd << "sc " << command << " " << serviceName ;
#endif

      if ( 2 < arg.argc() )
      {
         rc = arg.getString( 2, options ) ;
         if ( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "options must be string" ) ;
            goto error ;
         }
         cmd << " " << options ;
      }

      // run cmd
      rc = runner.exec( cmd.str().c_str(), exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to exec cmd, rc:%d, exit:%d",
                 rc, exitCode ) ;
         stringstream ss ;
         ss << "failed to exec cmd " << cmd.str() << ",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }

      // get result
      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to read msg from cmd runner:%d", rc ) ;
         stringstream ss ;
         ss << "failed to read msg from cmd \"" << cmd.str() << "\", rc:"
            << rc ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }
      else if ( SDB_OK != exitCode )
      {
         rc = exitCode ;
         detail = BSON( SPT_ERR << outStr ) ;
         goto error ;
      }
      if( '\n' == outStr[ outStr.size() - 1 ]  )
      {
         outStr.erase( outStr.size()-1, 1 ) ;
      }

      rval.getReturnVal().setValue( outStr ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrSystem::createSshKey( const _sptArguments &arg,
                                      _sptReturnVal &rval,
                                      BSONObj &detail )
   {
#if defined (_LINUX)
      INT32 rc           = SDB_OK ;
      UINT32 exitCode    = 0 ;
      _ossCmdRunner      runner ;
      string             outStr ;
      string             homePath ;
      string             cmd ;
      // get home path
      rc = _getHomePath( homePath ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get home path, rc: %d", rc ) ;
         detail = BSON( SPT_ERR << "Failed to get home path" ) ;
         goto error ;
      }

      cmd = "echo -e \"n\" | ssh-keygen -t rsa -f " + homePath +
            "/.ssh/id_rsa -N \"\" " ;
      // create Ssh key
      rc = runner.exec( cmd.c_str(),
                        exitCode, FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to exec cmd, rc:%d, exit:%d",
                 rc, exitCode ) ;
         stringstream ss ;
         ss << "failed to exec cmd " << cmd
            << ",rc: "
            << rc
            << ",exit: "
            << exitCode ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;

#elif defined (_WINDOWS)
      return SDB_OK ;
#endif
   }

   INT32 _sptUsrSystem::getHomePath( const _sptArguments & arg,
                                     _sptReturnVal & rval,
                                     BSONObj & detail )
   {
      INT32              rc = SDB_OK ;
      string             homeDir ;

      rc = _getHomePath( homeDir ) ;
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to get home path" ) ;
         goto error ;
      }
      rval.getReturnVal().setValue( homeDir ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrSystem::getUserEnv( const _sptArguments & arg,
                                _sptReturnVal & rval,
                                BSONObj & detail )
   {
      INT32 rc            = SDB_OK ;
      BSONObjBuilder      builder ;
      UINT32 exitCode     = 0 ;
      string              cmd ;
      _ossCmdRunner       runner ;
      string              outStr ;

// build cmd
#if defined (_LINUX)
      cmd = "env" ;
#elif defined (_WINDOWS)
      cmd = "cmd /C set" ;
#endif

      // run cmd
      rc = runner.exec( cmd.c_str(), exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to exec cmd, rc:%d, exit:%d",
                 rc, exitCode ) ;
         stringstream ss ;
         ss << "failed to exec cmd " << cmd << ",rc: "
            << rc
            << ",exit: "
            << exitCode ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }

      // get result
      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to read msg from cmd runner:%d", rc ) ;
         stringstream ss ;
         ss << "failed to read msg from cmd \"" << cmd << "\", rc:"
            << rc ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }

      // extract result
      rc = _extractEnvInfo( outStr.c_str(), builder ) ;
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to extract env info" ) ;
         goto error ;
      }
      rval.getReturnVal().setValue( builder.obj() ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrSystem::_extractEnvInfo( const CHAR *buf,
                                         BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      vector< string > splited ;
      vector< pair< string, string > > envVec ;
      if ( NULL == buf )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "buf can't be null, rc: %d", rc ) ;
         goto error ;
      }

      /* format:
         PWD=/home/users/wujiaming
         LANG=en_US.UTF-8
         SHLVL=1
         HOME=/root
         LANGUAGE=en_US:en
         LOGNAME=root
      */
      try
      {
         boost::algorithm::split( splited, buf, boost::is_any_of( "\r\n" ) ) ;
      }
      catch( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                 rc, e.what() ) ;
         goto error ;
      }
      for ( vector<string>::iterator itr = splited.begin();
            itr != splited.end();  )
      {
         if ( itr->empty() )
         {
            itr = splited.erase( itr ) ;
         }
         else
         {
            itr++ ;
         }
      }

      for ( vector<string>::iterator itrSplit = splited.begin();
            itrSplit != splited.end(); itrSplit++ )
      {
         vector<string> columns ;
         string value ;

         // if no contain '=', it is the part of previous row
         if ( std::string::npos == (*itrSplit).find( "=" ) )
         {
            if ( envVec.size() )
            {
               envVec.back().second += *itrSplit ;
            }
            continue ;
         }

         try
         {
            boost::algorithm::split( columns, *itrSplit,
                                     boost::is_any_of("=") ) ;
         }
         catch( std::exception &e )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                    rc, e.what() ) ;
            goto error ;
         }
         for ( vector<string>::iterator itrCol = columns.begin() ;
               itrCol != columns.end(); )
         {
            if ( itrCol->empty() )
            {
               itrCol = columns.erase( itrCol ) ;
            }
            else
            {
               itrCol++ ;
            }
         }

         // at least conatain 1 cols
         if ( columns.size() < 1 )
         {
            continue ;
         }
         else if ( columns.size() == 1 )
         {
            value = "" ;
         }
         else
         {
            value = *( columns.begin() + 1 ) ;
            /*
               may contain result like "LS_COLORS=rs=0:di=01;34:ln=01"
               need to merge into a string
            */
            for ( vector<string>::iterator itrCol = columns.begin() + 2 ;
                  itrCol != columns.end(); itrCol++ )
            {
               value += "=" + *itrCol ;
            }
         }
         envVec.push_back( pair< string, string >( *columns.begin(), value ) ) ;
      }

      for ( vector< pair< string, string > >::iterator itr = envVec.begin() ;
            itr != envVec.end();
            itr++ )
      {
         builder.append( itr->first, itr->second ) ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrSystem::_getHomePath( string &homePath )
   {
      INT32              rc = SDB_OK ;
      string             cmd ;
      _ossCmdRunner      runner ;
      UINT32             exitCode = 0 ;
      string             outStr ;

#if defined (_LINUX)
      cmd = "whoami" ;
#elif defined (_WINDOWS)
      cmd = "cmd /C set HOMEPATH" ;
#endif
      // run cmd
      rc = runner.exec( cmd.c_str(), exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to exec cmd, rc:%d, exit:%d",
                 rc, exitCode ) ;
         goto error ;
      }

      // get result
      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to read msg from cmd runner:%d", rc ) ;
         goto error ;
      }
      if( !outStr.empty() && outStr[ outStr.size() - 1 ] == '\n' )
      {
#if defined (_LINUX)
         // erase /n
         outStr.erase( outStr.size()-1, 1 ) ;
#elif defined (_WINDOWS)
         // erase /r/n
         outStr.erase( outStr.size()-2, 2 ) ;
#endif
      }

#if defined (_LINUX)
      {
         OSSUID uid = 0 ;
         OSSGID gid = 0 ;
         ossGetUserInfo( outStr.c_str(), uid, gid ) ;
         struct passwd *pw = getpwuid( uid ) ;
         homePath = pw->pw_dir ;
      }
#elif defined (_WINDOWS)
      {
         vector< string > splited ;
         try
         {
            boost::algorithm::split( splited, outStr,
                                     boost::is_any_of( "=" ) ) ;
         }
         catch( std::exception &e )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                    rc, e.what() ) ;
            goto error ;
         }
         homePath = splited[ 1 ] ;
         for( UINT32 index = 2; index < splited.size(); index++ )
         {
            homePath += splited[ index ] ;
         }
      }
#endif
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrSystem::staticHelp( const _sptArguments & arg,
                                    _sptReturnVal & rval,
                                    BSONObj & detail )
   {
      stringstream ss ;
      ss << "Local static functions:" << endl
         << "var system = remoteObj.getSystem()" << endl
         << " System.ping( hostname )" << endl
         << " System.type()" << endl
         << " System.getReleaseInfo()" << endl
         << " System.getHostsMap()" << endl
         << " System.getAHostMap( hostname )" << endl
         << " System.addAHostMap( hostname, ip, [isReplace] )" << endl
         << " System.delAHostMap( hostname )" << endl
         << " System.getCpuInfo()" << endl
         << " System.snapshotCpuInfo()" << endl
         << " System.getMemInfo()" << endl
         << " System.snapshotMemInfo()" << endl
         << " System.getDiskInfo()" << endl
         << " System.snapshotDiskInfo()" << endl
         << " System.getNetcardInfo()" << endl
         << " System.snapshotNetcardInfo()" << endl
         << " System.getIpTablesInfo()" << endl
         << " System.getHostName()" << endl
         << " System.sniffPort( port )" << endl
         << " System.getPID()" << endl
         << " System.getTID()" << endl
         << " System.getEWD()" << endl
         << " System.listProcess( [optionObj], [filterObj] )" << endl
         << " System.isProcExist( optionObj )" << endl
         << " System.killProcess( optionObj )" << endl
         << " System.getUserEnv()" << endl
#if defined (_LINUX)
         << " System.addUser( userObj )" << endl
         << " System.addGroup( groupObj )" << endl
         << " System.setUserConfigs( optionObj )" << endl
         << " System.delUser( optionObj )" << endl
         << " System.delGroup( name )" << endl
         << " System.listLoginUsers( [optionObj], [filterObj] )" << endl
         << " System.listAllUsers( [optionObj], [filterObj] )" << endl
         << " System.listGroups( [optionObj], [filterObj] )" << endl
         << " System.getCurrentUser()" << endl
         << " System.isUserExist( username )" << endl
         << " System.isGroupExist( groupname )" << endl
         << " System.getProcUlimitConfigs()" << endl
         << " System.setProcUlimitConfigs( configsObj )" << endl
         << " System.getSystemConfigs( [type] )" << endl
#endif
         << " System.runService( servicename, command, [option] )" << endl ;
      rval.getReturnVal().setValue( ss.str() ) ;
      return SDB_OK ;
   }

   INT32 _sptUsrSystem::memberHelp( const _sptArguments & arg,
                                    _sptReturnVal & rval,
                                    BSONObj & detail )
   {
      stringstream ss ;
      ss << "Remote System member functions:" << endl
         << "   ping( hostname )" << endl
         << "   type()" << endl
         << "   getReleaseInfo()" << endl
         << "   getHostsMap()" << endl
         << "   getAHostMap( hostname )" << endl
         << "   addAHostMap( hostname, ip, [isReplace] )" << endl
         << "   delAHostMap( hostname )" << endl
         << "   getCpuInfo()" << endl
         << "   snapshotCpuInfo()" << endl
         << "   getMemInfo()" << endl
         << "   snapshotMemInfo()" << endl
         << "   getDiskInfo()" << endl
         << "   snapshotDiskInfo()" << endl
         << "   getNetcardInfo()" << endl
         << "   snapshotNetcardInfo()" << endl
         << "   getIpTablesInfo()" << endl
         << "   getHostName()" << endl
         << "   sniffPort( port )" << endl
         << "   getPID()" << endl
         << "   getTID()" << endl
         << "   getEWD()" << endl
         << "   listProcess( [optionObj], [filterObj] )" << endl
         << "   isProcExist( optionObj )" << endl
         << "   killProcess( optionObj )" << endl
         << "   getUserEnv()" << endl
         << "   addUser( userObj )" << endl
         << "   addGroup( groupObj )" << endl
         << "   setUserConfigs( optionObj )" << endl
         << "   delUser( optionObj )" << endl
         << "   delGroup( name )" << endl
         << "   listLoginUsers( [optionObj], [filterObj] )" << endl
         << "   listAllUsers( [optionObj], [filterObj] )" << endl
         << "   listGroups( [optionObj], [filterObj] )" << endl
         << "   getCurrentUser()" << endl
         << "   isUserExist( username )" << endl
         << "   isGroupExist( groupname )" << endl
         << "   getProcUlimitConfigs()" << endl
         << "   setProcUlimitConfigs( configsObj )" << endl
         << "   getSystemConfigs( [type] )" << endl
         << "   buildTrusty()" << endl
         << "   removeTrusty()" << endl
         << "   runService( servicename, command, [option] )" << endl ;
      rval.getReturnVal().setValue( ss.str() ) ;
      return SDB_OK ;
   }
}

