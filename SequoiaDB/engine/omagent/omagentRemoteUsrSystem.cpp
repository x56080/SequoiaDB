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

   Source File Name = omagentRemoteUsrSystem.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/03/2016  WJM Initial Draft

   Last Changed =

*******************************************************************************/
#include "omagentRemoteUsrSystem.hpp"
#include "cmdUsrSystemUtil.hpp"
#include "pmdOptions.h"
#include "utilCommon.hpp"
#include "omagentDef.hpp"
#include "pmd.hpp"
#include "ossCmdRunner.hpp"
#include "ossUtil.hpp"
#include "utilStr.hpp"
#include "ossIO.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <vector>
#include <set>
#include <utility>
#include "ossPath.hpp"
#include "ossPrimitiveFileOp.hpp"
#if defined (_LINUX)
#include <net/if.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <sys/resource.h>
#else
#include <iphlpapi.h>
#pragma comment( lib, "IPHLPAPI.lib" )
#endif

using namespace bson ;
using std::pair ;

namespace engine
{
   /*
      _remoteSystemPing implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemPing )

   _remoteSystemPing::_remoteSystemPing()
   {
   }

   _remoteSystemPing::~_remoteSystemPing()
   {
   }

   const CHAR* _remoteSystemPing::name()
   {
      return OMA_REMOTE_SYS_PING ;
   }

   INT32 _remoteSystemPing::doit( BSONObj &retObj )
   {

      INT32 rc = SDB_OK ;
      BSONObjBuilder objBuilder ;
      string host ;
      stringstream cmd ;
      _ossCmdRunner runner ;
      UINT32 exitCode = 0 ;

      // get argument
      if ( _matchObj.isEmpty() )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "hostname must be config") ;
         goto error ;
      }
      if ( String != _matchObj.getField( "hostname" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "hostname must be string") ;
         goto error ;
      }
      host = _matchObj.getStringField( "hostname" ) ;
      if ( "" == host )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "hostname can't be empty") ;
         goto error ;
      }

      // build cmd
#if defined (_LINUX)
      cmd << "ping " << " -q -c 1 "  << "\"" << host << "\"" ;
#elif defined (_WINDOWS)
      cmd << "ping -n 2 -w 1000 " << "\"" << host << "\"" ;
#endif

      // run cmd
      rc = runner.exec( cmd.str().c_str(), exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         stringstream ss ;
         ss << "failed to exec cmd \"ping\",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
         goto error ;
      }

      objBuilder.append( CMD_USR_SYSTEM_TARGET, host ) ;
      objBuilder.appendBool( CMD_USR_SYSTEM_REACHABLE, SDB_OK == exitCode ) ;
      retObj = objBuilder.obj() ;
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteSystemType implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemType )

    _remoteSystemType::_remoteSystemType()
   {
   }

   _remoteSystemType::~_remoteSystemType()
   {
   }

   const CHAR* _remoteSystemType::name()
   {
      return OMA_REMOTE_SYS_TYPE ;
   }

   INT32 _remoteSystemType::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      BSONObj bsonObj ;

      // check argument
      if ( FALSE == _optionObj.isEmpty( ) ||
           FALSE == _matchObj.isEmpty( ) ||
           FALSE == _valueObj.isEmpty( ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "type() should have non arguments" ) ;
         goto error ;
      }

#if defined (_LINUX)
      bsonObj = BSON( "type" << "LINUX" ) ;
#elif defined (_WINDOWS)
      bsonObj = BSON( "type" << "WINDOWS" ) ;
#endif

      retObj = bsonObj ;
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteSystemGetReleaseInfo implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemGetReleaseInfo )

     _remoteSystemGetReleaseInfo::_remoteSystemGetReleaseInfo()
   {
   }

   _remoteSystemGetReleaseInfo::~_remoteSystemGetReleaseInfo()
   {
   }

    const CHAR* _remoteSystemGetReleaseInfo::name()
   {
      return OMA_REMOTE_SYS_GET_RELEASE_INFO ;
   }

   INT32 _remoteSystemGetReleaseInfo::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      UINT32 exitCode = 0 ;
      _ossCmdRunner runner ;
      string outStr ;
      BSONObjBuilder objBuilder ;

      // check argument
      if ( FALSE == _optionObj.isEmpty( ) ||
           FALSE == _matchObj.isEmpty( ) ||
           FALSE == _valueObj.isEmpty( ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "getReleaseInfo() should have non arguments" ) ;
         goto error ;
      }

#if defined (_LINUX)
      rc = runner.exec( "lsb_release -a |grep -v \"LSB Version\"", exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
#elif defined (_WINDOWS)
      rc = SDB_SYS ;
#endif

      // failed to run cmd ->get info by oss function
      if ( SDB_OK != rc || SDB_OK != exitCode )
      {
         rc = SDB_OK ;
         ossOSInfo info ;
         ossGetOSInfo( info ) ;

         objBuilder.append( CMD_USR_SYSTEM_DISTRIBUTOR, info._distributor ) ;
         objBuilder.append( CMD_USR_SYSTEM_RELASE, info._release ) ;
         objBuilder.append( CMD_USR_SYSTEM_DESP, info._desp ) ;
         objBuilder.append( CMD_USR_SYSTEM_BIT, info._bit ) ;

         retObj = objBuilder.obj() ;
         goto done ;
      }

      // succeed to run cmd ->get result
      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         stringstream ss ;
         ss << "failed to read msg from cmd \"lsb_release -a\", rc:"
            << rc ;
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
         goto error ;
      }

      // extract result
      rc = _extractReleaseInfo( outStr.c_str(), objBuilder ) ;
      if ( SDB_OK != rc )
      {
         stringstream ss ;
         ss << "failed to extract info from release info:"
            << outStr ;
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
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
         if ( SDB_OK == rc )
         {
            rc = SDB_SYS ;
         }
         stringstream ss ;
         ss << "failed to exec cmd \"getconf LONG_BIT\", rc:"
            << rc
            << ",exit:"
            << exitCode ;
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
         goto error ;
      }

      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         stringstream ss ;
         ss << "failed to read msg from cmd \"getconf LONG_BIT\", rc:"
            << rc ;
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
         goto error ;
      }

      if ( NULL != ossStrstr( outStr.c_str(), "64") )
      {
         objBuilder.append( CMD_USR_SYSTEM_BIT, 64 ) ;
      }
      else
      {
         objBuilder.append( CMD_USR_SYSTEM_BIT, 32 ) ;
      }

      retObj = objBuilder.obj() ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _remoteSystemGetReleaseInfo::_extractReleaseInfo( const CHAR *buf,
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
            PD_LOG( PDERROR, "Failed to trim, rc: %d, detail: %s",
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

   /*
      _remoteSystemGetHostsMap implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemGetHostsMap )

   _remoteSystemGetHostsMap::_remoteSystemGetHostsMap()
   {
   }

   _remoteSystemGetHostsMap::~_remoteSystemGetHostsMap()
   {
   }

   const CHAR* _remoteSystemGetHostsMap::name()
   {
      return OMA_REMOTE_SYS_GET_HOSTS_MAP ;
   }

   INT32 _remoteSystemGetHostsMap::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder objBuilder ;
      string err ;
      VEC_HOST_ITEM vecItems ;

      if ( FALSE == _optionObj.isEmpty( ) ||
           FALSE == _matchObj.isEmpty( ) ||
           FALSE == _valueObj.isEmpty( ) )
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

      _buildHostsResult( vecItems, objBuilder ) ;
      retObj = objBuilder.obj() ;
   done:
      return rc ;
   error:
      PD_LOG_MSG( PDERROR, err.c_str() ) ;
      goto done ;
   }

   /*
      _remoteSystemGetAHostMap
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemGetAHostMap )

   _remoteSystemGetAHostMap::_remoteSystemGetAHostMap()
   {
   }

   _remoteSystemGetAHostMap::~_remoteSystemGetAHostMap()
   {
   }

   INT32 _remoteSystemGetAHostMap::init( const CHAR *pInfomation  )
   {
      int rc = SDB_OK ;

      rc = _remoteExec::init( pInfomation ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get argument, rc: %d", rc ) ;

      // check argument
      if ( FALSE == _matchObj.hasField( "hostname" ) ||
           jstNULL == _valueObj.getField( "hostname" ).type() )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "hostname must be config" ) ;
         goto error ;
      }

      if ( String != _matchObj.getField( "hostname" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "hostname must be string" ) ;
         goto error ;
      }

      _hostname = _matchObj.getStringField( "hostname" ) ;
      if ( "" == _hostname )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "hostname can't be empty" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   const CHAR* _remoteSystemGetAHostMap::name()
   {
      return OMA_REMOTE_SYS_GET_AHOST_MAP ;
   }

   INT32 _remoteSystemGetAHostMap::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string err ;
      VEC_HOST_ITEM vecItems ;

      // get host map
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
            if( LINE_HOST == item._lineType && _hostname == item._host )
            {

               retObj = BSON( "ip" << item._ip.c_str() ) ;
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
      PD_LOG_MSG( PDERROR, err.c_str() ) ;
      goto done ;
   }

   /*
      _remoteSystemAddAHostMap
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemAddAHostMap )

   _remoteSystemAddAHostMap::_remoteSystemAddAHostMap()
   {
   }

   _remoteSystemAddAHostMap::~_remoteSystemAddAHostMap()
   {
   }

   INT32 _remoteSystemAddAHostMap::init( const CHAR *pInfomation  )
   {
      int rc = SDB_OK ;

      rc = _remoteExec::init( pInfomation ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get argument, rc: %d", rc ) ;

      // check argument
      if ( FALSE == _valueObj.hasField( "hostname" ) ||
           jstNULL == _valueObj.getField( "hostname" ).type() )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "hostname must be config" ) ;
         goto error ;
      }
      if ( String != _valueObj.getField( "hostname" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "hostname must be string" ) ;
         goto error ;
      }
      _hostname = _valueObj.getStringField( "hostname" ) ;
      if ( "" == _hostname )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "hostname can't be empty" ) ;
         goto error ;
      }

      if ( FALSE == _valueObj.hasField( "ip" ) ||
           jstNULL == _valueObj.getField( "ip" ).type() )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "ip must be config" ) ;
         goto error ;
      }
      if ( String != _valueObj.getField( "ip" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "ip must be string" ) ;
         goto error ;
      }
      _ip = _valueObj.getStringField( "ip" ) ;
      if ( "" == _ip )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "ip can't be empty" ) ;
         goto error ;
      }

      if ( !isValidIPV4( _ip.c_str() ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "ip is not ipv4" ) ;
         goto error ;
      }

      if ( FALSE == _valueObj.hasField( "isReplace" ) ||
           jstNULL == _valueObj.getField( "isReplace" ).type() ||
           Undefined == _valueObj.getField( "isReplace" ).type() )
      {
         _isReplace = TRUE ;
         goto done ;
      }

      if ( Bool != _valueObj.getField( "isReplace" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "isReplace must be BOOLEAN" ) ;
         goto error ;
      }
      _isReplace = _valueObj.getBoolField( "isReplace" ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   const CHAR* _remoteSystemAddAHostMap::name()
   {
      return OMA_REMOTE_SYS_ADD_AHOST_MAP ;
   }

   INT32 _remoteSystemAddAHostMap::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string err ;
      VEC_HOST_ITEM vecItems ;

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
            if( item._lineType == LINE_HOST && _hostname == item._host )
            {
               if ( item._ip == _ip )
               {
                  goto done ;
               }
               else if ( !_isReplace )
               {
                  err = "hostname already exist" ;
                  rc = SDB_INVALIDARG ;
                  goto error ;
               }
               item._ip = _ip ;
               hasMod = TRUE ;
            }
         }
         if ( !hasMod )
         {
            usrSystemHostItem info ;
            info._lineType = LINE_HOST ;
            info._host = _hostname ;
            info._ip = _ip ;
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
      PD_LOG_MSG( PDERROR, err.c_str() ) ;
      goto done ;
   }

   /*
      _remoteSystemDelAHostMap
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemDelAHostMap )

   _remoteSystemDelAHostMap::_remoteSystemDelAHostMap()
   {
   }

   _remoteSystemDelAHostMap::~_remoteSystemDelAHostMap()
   {
   }

   INT32 _remoteSystemDelAHostMap::init( const CHAR *pInfomation  )
   {
      int rc = SDB_OK ;

      rc = _remoteExec::init( pInfomation ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get argument, rc: %d", rc ) ;

      if ( FALSE == _matchObj.hasField( "hostname" ) ||
           jstNULL == _valueObj.getField( "hostname" ).type() )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "hostname must be config" ) ;
         goto error ;
      }
      if ( String != _matchObj.getField( "hostname" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "hostname must be string" ) ;
         goto error ;
      }
      _hostname = _matchObj.getStringField( "hostname" ) ;
      if ( "" == _hostname )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "hostname can't be empty" ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   const CHAR* _remoteSystemDelAHostMap::name()
   {
      return OMA_REMOTE_SYS_DEL_AHOST_MAP ;
   }

   INT32 _remoteSystemDelAHostMap::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string err ;
      VEC_HOST_ITEM vecItems ;

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
            if( item._lineType == LINE_HOST && _hostname == item._host )
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
      PD_LOG_MSG( PDERROR, err.c_str() ) ;
      goto done ;
   }

   /*
      _remoteSystemGetCpuInfo implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemGetCpuInfo )

   _remoteSystemGetCpuInfo::_remoteSystemGetCpuInfo()
   {
   }

   _remoteSystemGetCpuInfo::~_remoteSystemGetCpuInfo()
   {
   }

    const CHAR* _remoteSystemGetCpuInfo::name()
   {
      return OMA_REMOTE_SYS_GET_CPU_INFO ;
   }

   INT32 _remoteSystemGetCpuInfo::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      rc = _getCpuInfo( retObj ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get cpu info, rc: %d", rc ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

#if defined (_LINUX)
   INT32 _remoteSystemGetCpuInfo::_getCpuInfo( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      UINT32 exitCode = 0 ;
      _ossCmdRunner runner ;
      string outStr ;
      BSONObjBuilder builder ;
#if defined (_PPCLIN64)
   #define CPU_CMD "cat /proc/cpuinfo | grep -E 'processor|cpu|clock|machine'"
#else
   #define CPU_CMD "cat /proc/cpuinfo | " \
                   "grep -E 'model name|cpu MHz|cpu cores|physical id'"
#endif

      // run cmd
      rc = runner.exec( CPU_CMD, exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc || SDB_OK != exitCode )
      {
         if ( SDB_OK == rc )
         {
            rc = SDB_SYS ;
         }
         stringstream ss ;
         ss << "failed to exec cmd \" " << CPU_CMD << "\",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
         goto error ;
      }

      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         stringstream ss ;
         ss << "failed to read msg from cmd \"" << CPU_CMD << "\", rc:"
            << rc ;
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
         goto error ;
      }

      rc = _extractCpuInfo( outStr.c_str(), builder ) ;
      if ( SDB_OK != rc )
      {
         stringstream ss ;
         ss << "failed to read msg from buf:"
            << outStr ;
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
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
      retObj = builder.obj() ;

   done:
      return rc ;
   error:
      goto done ;
   }
#endif /// _LINUX

#if defined (_WINDOWS)
   INT32 _remoteSystemGetCpuInfo::_getCpuInfo( BSONObj &retObj )
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
         if ( SDB_OK == rc )
         {
            rc = SDB_SYS ;
         }
         stringstream ss ;
         ss << "failed to exec cmd \" " << cmd << "\",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
         goto error ;
      }

      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         stringstream ss ;
         ss << "failed to read msg from cmd \"" << cmd << "\", rc:"
            << rc ;
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
         goto error ;
      }

      rc = _extractCpuInfo( outStr.c_str(), builder ) ;
      if ( SDB_OK != rc )
      {
         stringstream ss ;
         ss << "failed to read msg from buf:"
            << outStr ;
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
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
      retObj = builder.obj() ;

   done:
      return rc ;
   error:
      goto done ;
   }
#endif /// _WINDOWS

#if defined (_LINUX)
  #if defined (_PPCLIN64)
   INT32 _remoteSystemGetCpuInfo::_extractCpuInfo( const CHAR *buf,
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
                                   << CMD_USR_SYSTEM_FREQ
                                   << strAvgFreq + "GHz" ) ;
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
   INT32 _remoteSystemGetCpuInfo::_extractCpuInfo( const CHAR *buf,
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
   INT32 _remoteSystemGetCpuInfo::_extractCpuInfo( const CHAR *buf,
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
         vector<string> columns ;
         ++lineCount ;
         if ( 1 == lineCount || itr->empty() )
         {
            continue ;
         }
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
#endif //_WINDOWS

   /*
      _remoteSystemSnapshotCpuInfo implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemSnapshotCpuInfo )

   _remoteSystemSnapshotCpuInfo::_remoteSystemSnapshotCpuInfo()
   {
   }

   _remoteSystemSnapshotCpuInfo::~_remoteSystemSnapshotCpuInfo()
   {
   }

    const CHAR* _remoteSystemSnapshotCpuInfo::name()
   {
      return OMA_REMOTE_SYS_SNAPSHOT_CPU_INFO ;
   }

   INT32 _remoteSystemSnapshotCpuInfo::doit( BSONObj &retObj )
   {
      INT32 rc     = SDB_OK ;
      SINT64 user  = 0 ;
      SINT64 sys   = 0 ;
      SINT64 idle  = 0 ;
      SINT64 other = 0 ;
      rc = ossGetCPUInfo( user, sys, idle, other ) ;
      if ( SDB_OK != rc )
      {
         stringstream ss ;
         ss << "failed to get cpuinfo:rc="
            << rc ;
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
         goto error ;
      }

      {
         BSONObjBuilder objBuilder ;
         objBuilder.appendNumber( CMD_USR_SYSTEM_USER, user ) ;
         objBuilder.appendNumber( CMD_USR_SYSTEM_SYS, sys ) ;
         objBuilder.appendNumber( CMD_USR_SYSTEM_IDLE, idle ) ;
         objBuilder.appendNumber( CMD_USR_SYSTEM_OTHER, other ) ;

         retObj = objBuilder.obj() ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteSystemGetMemInfo implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemGetMemInfo )

   _remoteSystemGetMemInfo::_remoteSystemGetMemInfo()
   {
   }

   _remoteSystemGetMemInfo::~_remoteSystemGetMemInfo()
   {
   }

   const CHAR* _remoteSystemGetMemInfo::name()
   {
      return OMA_REMOTE_SYS_GET_MEM_INFO ;
   }

   INT32 _remoteSystemGetMemInfo::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      UINT32 exitCode = 0 ;
      _ossCmdRunner runner ;
      string outStr ;
      BSONObjBuilder objBuilder ;

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
            PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
            goto error ;
         }

         objBuilder.append( CMD_USR_SYSTEM_SIZE, (INT32)(totalPhys/CMD_MB_SIZE) ) ;
         objBuilder.append( CMD_USR_SYSTEM_USED,
                         (INT32)((totalPhys-availPhys)/CMD_MB_SIZE) ) ;
         objBuilder.append( CMD_USR_SYSTEM_FREE,(INT32)(availPhys/CMD_MB_SIZE) ) ;
         objBuilder.append( CMD_USR_SYSTEM_UNIT, "M" ) ;

         retObj = objBuilder.obj() ;
         goto done ;
      }

      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         stringstream ss ;
         ss << "failed to read msg from cmd \"free\", rc:"
            << rc ;
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
         goto error ;
      }

      rc = _extractMemInfo( outStr.c_str(), objBuilder ) ;
      if ( SDB_OK != rc )
      {
         stringstream ss ;
         ss << "failed to read msg from buf:"
            << outStr ;
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
         goto error ;
      }

      retObj = objBuilder.obj() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _remoteSystemGetMemInfo::_extractMemInfo( const CHAR *buf,
                                                   BSONObjBuilder &builder )
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
      /// Mem:   8194232    2373776    5820456          0     387924     992756
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

   /*
      _remoteSystemGetDiskInfo implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemGetDiskInfo )

   _remoteSystemGetDiskInfo::_remoteSystemGetDiskInfo()
   {
   }

   _remoteSystemGetDiskInfo::~_remoteSystemGetDiskInfo()
   {
   }

   const CHAR* _remoteSystemGetDiskInfo::name()
   {
      return OMA_REMOTE_SYS_GET_DISK_INFO ;
   }

   INT32 _remoteSystemGetDiskInfo::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;

      rc = _getDiskInfo( retObj ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get disk info, rc: %d", rc ) ;
   done:
      return rc ;
   error:
      goto done ;
   }



#if defined (_LINUX)
   INT32 _remoteSystemGetDiskInfo::_getDiskInfo( BSONObj &retObj )
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
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
         goto error ;
      }

      do
      {
         read = 0 ;
         ossMemset( buf, '\0', bufSize ) ;
         rc = ossReadN( &file, bufSize, buf, read ) ;
         if ( SDB_OK != rc )
         {
            ss << "failed to read file(/etc/mtab), rc:" << rc ;
            PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
            goto error ;
         }

         filess << buf ;
         if ( read < bufSize )
         {
            break ;
         }
      } while ( TRUE ) ;

      rc = _extractDiskInfo( filess.str().c_str(), retObj ) ;
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

   INT32 _remoteSystemGetDiskInfo::_extractDiskInfo( const CHAR * buf,
                                                     BSONObj & retObj )
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
            boost::algorithm::split( columns, *itr, boost::is_any_of("\t ") ) ;
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
                  diskBuilder.appendNumber( CMD_USR_SYSTEM_IO_R_SEC, (INT64)ioStat.rdSectors ) ;
                  diskBuilder.appendNumber( CMD_USR_SYSTEM_IO_W_SEC, (INT64)ioStat.wrSectors ) ;
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
      retObj = builder.obj() ;
   done:
      return rc ;
   error:
      goto done ;
   }
#endif

#if defined (_WINDOWS)
   INT32 _remoteSystemGetDiskInfo::_getDiskInfo( BSONObj &retObj )
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
         if ( SDB_OK == rc )
         {
            rc = SDB_SYS ;
         }
         stringstream ss ;
         ss << "failed to exec cmd \"" << DISK_CMD << "\",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
         goto error ;
      }

      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         stringstream ss ;
         ss << "failed to read msg from cmd \"df\", rc:"
            << rc ;
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
         goto error ;
      }

      rc = _extractDiskInfo( outStr.c_str(), builder ) ;
      if ( SDB_OK != rc )
      {
         stringstream ss ;
         ss << "failed to read msg from buf:"
            << outStr ;
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
         goto error ;
      }
      retObj = builder.obj() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _remoteSystemGetDiskInfo::_extractDiskInfo( const CHAR * buf,
                                                     BSONObjBuilder &builder )
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
#endif

   /*
      _remoteSystemGetNetcardInfo implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemGetNetcardInfo )

   _remoteSystemGetNetcardInfo::_remoteSystemGetNetcardInfo()
   {
   }

   _remoteSystemGetNetcardInfo::~_remoteSystemGetNetcardInfo()
   {
   }

   const CHAR* _remoteSystemGetNetcardInfo::name()
   {
      return OMA_REMOTE_SYS_GET_NETCARD_INFO ;
   }

   INT32 _remoteSystemGetNetcardInfo::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder objBuilder ;

      rc = _extractNetcards( objBuilder ) ;
      if ( SDB_OK != rc )
      {
         stringstream ss ;
         ss << "failed to get netcard info:" << rc ;
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
         goto error ;
      }
      retObj = objBuilder.obj() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _remoteSystemGetNetcardInfo::_extractNetcards( BSONObjBuilder &builder )
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

   /*
      _remoteSystemSnapshotNetcardInfo implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemSnapshotNetcardInfo )

   _remoteSystemSnapshotNetcardInfo::_remoteSystemSnapshotNetcardInfo()
   {
   }

   _remoteSystemSnapshotNetcardInfo::~_remoteSystemSnapshotNetcardInfo()
   {
   }

   const CHAR* _remoteSystemSnapshotNetcardInfo::name()
   {
      return OMA_REMOTE_SYS_SNAPSHOT_NETCARD_INFO ;
   }

   INT32 _remoteSystemSnapshotNetcardInfo::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder objBuilder ;
      stringstream ss ;

      rc = _snapshotNetcardInfo( objBuilder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "_snapshotNetcardInfo failed:rc=%d", rc ) ;
         goto error ;
      }
      retObj = objBuilder.obj() ;

   done:
      return rc ;
   error:
      goto done ;

   }

#if defined (_LINUX)
   INT32 _remoteSystemSnapshotNetcardInfo::_snapshotNetcardInfo( BSONObjBuilder &builder )
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
         if ( SDB_OK == rc )
         {
            rc = SDB_SYS ;
         }
         ss << "failed to exec cmd \"" << netFlowCMD << "\",rc:"
            << rc << ",exit:" << exitCode ;
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
         goto error ;
      }

      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         stringstream ss ;
         ss << "failed to read msg from cmd \"df\", rc:" << rc ;
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
         goto error ;
      }

      rc = _extractNetCardSnapInfo( outStr.c_str(), builder ) ;
      if ( SDB_OK != rc )
      {
         ss << "failed to extract netcard snapshotinfo from buf:" << outStr ;
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _remoteSystemSnapshotNetcardInfo::_extractNetCardSnapInfo( const CHAR *buf,
                                                                    BSONObjBuilder &builder )
   {
      time_t myTime = time( NULL ) ;
      BSONArrayBuilder arrayBuilder ;
      INT32 rc = SDB_OK ;
      vector<string> vLines ;
      vector<string>::iterator iterLine ;

      try
      {
         boost::algorithm::split( vLines, buf, boost::is_any_of("\r\n") ) ;
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
            vector<string>::iterator iterColumn ;
            try
            {
               boost::algorithm::split( vColumns, oneLine,
                                        boost::is_any_of("\t ") ) ;
            }
            catch( std::exception &e )
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                       rc, e.what() ) ;
               goto error ;
            }
            iterColumn = vColumns.begin() ;
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
   INT32 _remoteSystemSnapshotNetcardInfo::_snapshotNetcardInfo( BSONObjBuilder &builder )
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
         ss << "new MIB_IFTABLE failed:rc=" << rc ;
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
         goto error ;
      }

      ULONG uRetCode = GetIfTable( pTable, &size, TRUE ) ;
      if ( uRetCode == ERROR_NOT_SUPPORTED )
      {
         rc = SDB_INVALIDARG ;
         ss << "GetIfTable failed:rc=" << uRetCode ;
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
         goto error ;
      }

      if ( uRetCode == ERROR_INSUFFICIENT_BUFFER )
      {
         SDB_OSS_FREE( pTable ) ;
         pTable = (PMIB_IFTABLE) SDB_OSS_MALLOC( size ) ;
         if ( NULL == pTable )
         {
            rc = SDB_OOM ;
            ss << "new MIB_IFTABLE failed:rc=" << rc ;
            PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
            goto error ;
         }
      }

      // get the seconds since 1970.1.1:0:0:0(Calendar Time)
      myTime = time( NULL ) ;
      uRetCode = GetIfTable( pTable, &size, TRUE ) ;
      if ( NO_ERROR != uRetCode )
      {
         rc = SDB_INVALIDARG ;
         ss << "GetIfTable failed:rc=" << uRetCode ;
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
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

   INT32 _remoteSystemSnapshotNetcardInfo::_extractNetCardSnapInfo( const CHAR *buf,
                                                                    BSONObjBuilder &builder )
   {
      return SDB_INVALIDARG ;
   }
#endif

   /*
      _remoteSystemGetIpTablesInfo implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemGetIpTablesInfo )

   _remoteSystemGetIpTablesInfo::_remoteSystemGetIpTablesInfo()
   {
   }

   _remoteSystemGetIpTablesInfo::~_remoteSystemGetIpTablesInfo()
   {
   }

   const CHAR* _remoteSystemGetIpTablesInfo::name()
   {
      return OMA_REMOTE_SYS_GET_IPTABLES_INFO ;
   }

   INT32 _remoteSystemGetIpTablesInfo::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      retObj = BSON( "FireWall" << "unknown" ) ;
      return rc ;
   }

   /*
      _remoteSystemGetHostName implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemGetHostName )

   _remoteSystemGetHostName::_remoteSystemGetHostName()
   {
   }

   _remoteSystemGetHostName::~_remoteSystemGetHostName()
   {
   }

   const CHAR* _remoteSystemGetHostName::name()
   {
      return OMA_REMOTE_SYS_GET_HOSTNAME ;
   }

   INT32 _remoteSystemGetHostName::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      CHAR hostName[ OSS_MAX_HOSTNAME + 1 ] = { 0 } ;

      rc = ossGetHostName( hostName, OSS_MAX_HOSTNAME ) ;
      if ( rc )
      {
         PD_LOG_MSG( PDERROR, "get hostname failed" ) ;
         goto error ;
      }

      retObj = BSON( "hostname" << hostName ) ;

   done:
      return rc ;
   error:
      goto done ;

   }

   /*
      _remoteSystemSniffPort implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemSniffPort )

   _remoteSystemSniffPort::_remoteSystemSniffPort()
   {
   }

   _remoteSystemSniffPort::~_remoteSystemSniffPort()
   {
   }

   const CHAR* _remoteSystemSniffPort::name()
   {
      return OMA_REMOTE_SYS_SNIFF_PORT ;
   }

   INT32 _remoteSystemSniffPort::init( const CHAR * pInfomation )
   {
      INT32 rc = SDB_OK ;

      rc = _remoteExec::init( pInfomation ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "Failed to get argument" ) ;
         goto error ;
      }

      if ( FALSE == _matchObj.hasField( "port" ) ||
           jstNULL == _matchObj.getField( "port" ).type())
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "not specified the port to sniff" ) ;
         goto error ;
      }

      if ( NumberInt == _matchObj.getField( "port" ).type() )
      {
         _port = _matchObj.getIntField( "port" ) ;
      }
      else if ( String == _matchObj.getField( "port" ).type() )
      {
         UINT16 tempPort ;
         string svcname = _matchObj.getStringField( "port" ) ;
         rc = ossGetPort( svcname.c_str(), tempPort ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG ( PDERROR, "Invalid svcname: %s", svcname.c_str() ) ;
            goto error ;
         }
         _port = tempPort ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "port must be number or string" ) ;
         goto error ;
      }

      if ( 0 >= _port || 65535 < _port )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "port must in range ( 0, 65536 )" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _remoteSystemSniffPort::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN result = FALSE ;
      stringstream ss ;
      BSONObjBuilder objBuilder ;

      PD_LOG ( PDDEBUG, "sniff port is: %d", _port ) ;
      _ossSocket sock( _port, OSS_ONE_SEC ) ;
      rc = sock.initSocket() ;
      if ( rc )
      {
         PD_LOG ( PDWARNING, "Failed to connect to port[%d], "
                  "rc: %d", _port, rc ) ;
         ss << "failed to sniff port" ;
         goto error ;
      }
      rc = sock.bind_listen() ;
      if ( rc )
      {
         PD_LOG ( PDDEBUG, "port[%d] is busy, rc: %d", _port, rc ) ;
         result = FALSE ;
         rc = SDB_OK ;
      }
      else
      {
         PD_LOG ( PDDEBUG, "port[%d] is usable", _port ) ;
         result = TRUE ;
      }
      objBuilder.appendBool( CMD_USR_SYSTEM_USABLE, result ) ;

      retObj = objBuilder.obj() ;
      //close the socket
      sock.close() ;
   done:
      return rc ;
   error:
      PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
      goto done ;
   }

   /*
      _remoteSystemListProcess implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemListProcess )

   _remoteSystemListProcess::_remoteSystemListProcess()
   {
   }

   _remoteSystemListProcess::~_remoteSystemListProcess()
   {
   }

   const CHAR* _remoteSystemListProcess::name()
   {
      return OMA_REMOTE_SYS_LIST_PROC ;
   }

   INT32 _remoteSystemListProcess::init( const CHAR * pInfomation )
   {
      INT32 rc = SDB_OK ;

      rc = _remoteExec::init( pInfomation ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "Failed to get argument" ) ;
         goto error ;
      }
      _showDetail = _optionObj.getBoolField( "detail" ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _remoteSystemListProcess::doit( BSONObj &retObj )
   {
      INT32 rc         = SDB_OK ;
      UINT32 exitCode  = 0 ;
      BSONObjBuilder builder ;
      BSONObj optionObj ;
      string outStr ;
      stringstream     cmd ;
      _ossCmdRunner runner ;


#if defined ( _LINUX )
   cmd << "ps ax -o user -o pid -o stat -o command" ;
#elif defined (_WINDOWS)
   cmd << "tasklist /FO \"CSV\"" ;
#endif

      rc = runner.exec( cmd.str().c_str(), exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "failed to exec cmd, rc:%d, exit:%d",
                 rc, exitCode ) ;
         goto error ;
      }

      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         stringstream ss ;
         ss << "failed to exec cmd " << cmd.str() << ",rc:"
            << rc ;
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
         goto error ;
      }

      _extractProcessInfo( outStr.c_str(), builder, _showDetail ) ;
      retObj = builder.obj() ;

   done:
      return rc ;
   error:
      goto done ;
   }

#if defined ( _LINUX )
   INT32 _remoteSystemListProcess::_extractProcessInfo( const CHAR *buf,
                                                        BSONObjBuilder &builder,
                                                        BOOLEAN showDetail )
   {
      INT32 rc = SDB_OK ;
      INT32 i = 0 ;
      vector<string> splited ;
      vector< BSONObj > procVec ;

      if ( NULL == buf )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "buf can't be null, rc: %d", rc ) ;
         goto error ;
      }

      /* format:
      USER       PID STAT COMMAND
      root         1  Ss  /sbin/init
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
            itrSplit != splited.end(); itrSplit++,i++  )
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

            // result at least contain 4 cols
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
            itrSplit != splited.end(); itrSplit++,i++ )
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

            // result at least contain 4 cols
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
   INT32 _remoteSystemListProcess::_extractProcessInfo( const CHAR *buf,
                                                        BSONObjBuilder &builder,
                                                        BOOLEAN showDetail )
   {
      INT32 rc = SDB_OK ;
      INT32 i = 0 ;
      vector<string> splited ;
      vector< BSONObj > procVec ;

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
               itrSplit != splited.end(); itrSplit++,i++  )
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
            } ;

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
            itrSplit != splited.end(); itrSplit++,i++ )
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
      for( INT32 index = 0; index < procVec.size(); index++ )
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

   /*
      _remoteSystemAddUser implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemAddUser )

   _remoteSystemAddUser::_remoteSystemAddUser()
   {
   }

   _remoteSystemAddUser::~_remoteSystemAddUser()
   {
   }

   const CHAR * _remoteSystemAddUser::name()
   {
      return OMA_REMOTE_SYS_ADD_USER ;
   }

   INT32 _remoteSystemAddUser::doit( BSONObj &retObj )
   {
#if defined (_LINUX)
      INT32 rc = SDB_OK ;
      BSONObj userObj ;
      string outStr ;
      stringstream cmd ;
      _ossCmdRunner runner ;
      UINT32 exitCode ;

      // init cmd
      cmd << "useradd" ;

      // check argument and build cmd
      if ( _valueObj.hasField( "passwd" ) )
      {
         if ( String !=  _valueObj.getField( "passwd" ).type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "passwd must be string" ) ;
            goto error ;
         }
         else
         {
            cmd << " -p "
                <<  _valueObj.getStringField( "passwd" ) ;
         }
      }

      if (  _valueObj.hasField( "group" ) )
      {
         if ( String !=  _valueObj.getField( "group" ).type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "group must be string" ) ;
            goto error ;
         }
         else
         {
            cmd << " -g "
                <<  _valueObj.getStringField( "group" ) ;
         }
      }

      if (  _valueObj.hasField( "Group" ) )
      {
         if ( String !=  _valueObj.getField( "Group" ).type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "Group must be string" ) ;
            goto error ;
         }
         else
         {
            cmd << " -G "
                <<  _valueObj.getStringField( "Group" ) ;
         }
      }

      if (  _valueObj.hasField( "dir" ) )
      {
         if ( String !=  _valueObj.getField( "dir" ).type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "dir must be string" ) ;
            goto error ;
         }
         else
         {
            cmd << " -d "
                <<  _valueObj.getStringField( "dir" ) ;
         }
      }

      if ( TRUE ==  _valueObj.getBoolField( "createDir" ) )
      {
         cmd << " -m" ;
      }

      if( FALSE == _valueObj.hasField( "name" ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "name must be config" ) ;
      }
      if( String != _valueObj.getField( "name" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "name must be string" ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get name, rc: %d", rc ) ;
      cmd << " " <<  _valueObj.getStringField( "name" ) ;

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
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
         goto error ;
      }

      // get result
      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "Failed to read result" ) ;
         goto error ;
      }
      else if ( SDB_OK != exitCode )
      {
         rc = exitCode ;
         PD_LOG_MSG( PDERROR, outStr.c_str() ) ;
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

   /*
      _remoteSystemSetUserConfigs implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemSetUserConfigs )

   _remoteSystemSetUserConfigs::_remoteSystemSetUserConfigs()
   {
   }

   _remoteSystemSetUserConfigs::~_remoteSystemSetUserConfigs()
   {
   }

   const CHAR * _remoteSystemSetUserConfigs::name()
   {
      return OMA_REMOTE_SYS_SET_USER_CONFIGS ;
   }

   INT32 _remoteSystemSetUserConfigs::doit( BSONObj &retObj )
   {
#if defined(_LINUX)
      INT32 rc = SDB_OK ;
      string outStr ;
      stringstream cmd ;
      _ossCmdRunner runner ;
      UINT32 exitCode ;

      // check argument and build cmd
      cmd << "usermod" ;
      if ( _valueObj.hasField( "passwd" ) )
      {
         if ( String !=  _valueObj.getField( "passwd" ).type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "passwd must be string" ) ;
            goto error ;
         }
         else
         {
            cmd << " -p "
                <<  _valueObj.getStringField( "passwd" ) ;
         }
      }

      if ( _valueObj.hasField( "group" ) )
      {
         if ( String !=  _valueObj.getField( "group" ).type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "group must be string" ) ;
            goto error ;
         }
         else
         {
            cmd << " -g "
                <<  _valueObj.getStringField( "group" ) ;
         }
      }

      if (  _valueObj.hasField( "Group" ) )
      {
         if ( String !=  _valueObj.getField( "Group" ).type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "Group must be string" ) ;
            goto error ;
         }
         else
         {
            cmd << " -G "
                <<  _valueObj.getStringField( "Group" ) ;
            if ( TRUE ==  _valueObj.getBoolField( "isAppend" ) )
            {
               cmd << " -a" ;
            }
         }
      }

      if (  _valueObj.hasField( "dir" ) )
      {
         if ( String !=  _valueObj.getField( "dir" ).type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "dir must be string" ) ;
            goto error ;
         }
         else
         {
            cmd << " -d "
                <<  _valueObj.getStringField( "dir" ) ;
            if ( TRUE ==  _valueObj.getBoolField( "isMove" ) )
            {
               cmd << " -m" ;
            }
         }
      }

      if( FALSE ==  _valueObj.hasField( "name" ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "name must be config" ) ;
      }
      if( String !=  _valueObj.getField( "name" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "name must be string" ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get name, rc: %d", rc ) ;
      cmd << " " <<  _valueObj.getStringField( "name" ) ;

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
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
         goto error ;
      }

      // get result
      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "Failed to read result" ) ;
         goto error ;
      }
      else if ( SDB_OK != exitCode )
      {
         rc = exitCode ;
         PD_LOG_MSG( PDERROR, outStr.c_str() ) ;
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

   /*
      _remoteSystemDelUser implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemDelUser )

   _remoteSystemDelUser::_remoteSystemDelUser()
   {
   }

   _remoteSystemDelUser::~_remoteSystemDelUser()
   {
   }

   const CHAR * _remoteSystemDelUser::name()
   {
      return OMA_REMOTE_SYS_DEL_USER ;
   }

   INT32 _remoteSystemDelUser::doit( BSONObj &retObj )
   {
#if defined (_LINUX)
      INT32 rc = SDB_OK ;
      string outStr ;
      stringstream cmd ;
      _ossCmdRunner runner ;
      UINT32 exitCode ;

      // check argument and build cmd
      cmd << "userdel" ;
      if ( _matchObj.hasField( "isRemoveDir" ) )
      {
         if ( Bool != _matchObj.getField( "isRemoveDir" ).type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "isRemoveDir must be bool" ) ;
            goto error ;
         }
         else if ( _matchObj.getBoolField( "isRemoveDir" ) )
         {
            cmd << " -r" ;
         }
      }

      if( FALSE == _matchObj.hasField( "name" ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "name must be config" ) ;
      }
      if( String != _matchObj.getField( "name" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "name must be string" ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get name, rc: %d", rc ) ;

      cmd << " " << _matchObj.getStringField( "name" ) ;

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
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
         goto error ;
      }

      // read result
      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "Failed to read result" ) ;
         goto error ;
      }
      else if ( SDB_OK != exitCode )
      {
         rc = exitCode ;
         PD_LOG_MSG( PDERROR, outStr.c_str() ) ;
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

   /*
      _remoteSystemAddGroup implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemAddGroup )

   _remoteSystemAddGroup::_remoteSystemAddGroup()
   {
   }

   _remoteSystemAddGroup::~_remoteSystemAddGroup()
   {
   }

   const CHAR * _remoteSystemAddGroup::name()
   {
      return OMA_REMOTE_SYS_ADD_GROUP ;
   }

   INT32 _remoteSystemAddGroup::doit( BSONObj &retObj )
   {
#if defined (_LINUX)
      INT32 rc = SDB_OK ;
      string outStr ;
      stringstream cmd ;
      _ossCmdRunner runner ;
      UINT32 exitCode ;

      // check argument and build cmd
      cmd << "groupadd" ;
      if ( _valueObj.hasField( "passwd" ) )
      {
         if ( String != _valueObj.getField( "passwd" ).type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "passwd must be string" ) ;
            goto error ;
         }
         else
         {
            cmd << " -p "
                << _valueObj.getStringField( "passwd" ) ;
         }
      }

      if ( _valueObj.hasField( "id" ) )
      {
         if ( String != _valueObj.getField( "id" ).type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "id must be string" ) ;
            goto error ;
         }
         else
         {
            cmd << " -g " << _valueObj.getStringField( "id" ) ;
            if ( TRUE == _valueObj.hasField( "isUnique" ) )
            {
               if ( Bool != _valueObj.getField( "isUnique" ).type() )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG_MSG( PDERROR, "isUnique must be bool" ) ;
                  goto error ;
               }
               if ( FALSE == _valueObj.getBoolField( "isUnique" ) )
               {
                  cmd << " -o" ;
               }
            }
         }
      }

      if( FALSE == _valueObj.hasField( "name" ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "name must be config" ) ;
      }
      if( String != _valueObj.getField( "name" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "name must be string" ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get name, rc: %d", rc ) ;
      cmd << " " << _valueObj.getStringField( "name" ) ;

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
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
         goto error ;
      }

      // read result
      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "Failed to read result" ) ;
         goto error ;
      }
      else if ( SDB_OK != exitCode )
      {
         rc = exitCode ;
         PD_LOG_MSG( PDERROR, outStr.c_str() ) ;
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

   /*
      _remoteSystemDelGroup implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemDelGroup )

   _remoteSystemDelGroup::_remoteSystemDelGroup()
   {
   }

   _remoteSystemDelGroup::~_remoteSystemDelGroup()
   {
   }

   const CHAR * _remoteSystemDelGroup::name()
   {
      return OMA_REMOTE_SYS_DEL_GROUP ;
   }

   INT32 _remoteSystemDelGroup::doit( BSONObj &retObj )
   {
#if defined(_LINUX)
      INT32 rc = SDB_OK ;
      string outStr ;
      stringstream cmd ;
      _ossCmdRunner runner ;
      UINT32 exitCode ;

      // check argument and build cmd
      cmd << "groupdel" ;
      if( FALSE == _matchObj.hasField( "name" ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "name must be config" ) ;
      }
      if( String != _matchObj.getField( "name" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "name must be string" ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get name, rc: %d", rc ) ;
      cmd << " " << _matchObj.getStringField( "name" ) ;

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
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
         goto error ;
      }

      // read result
      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "Failed to read result" ) ;
         goto error ;
      }
      if ( SDB_OK != exitCode )
      {
         rc = exitCode ;
         PD_LOG_MSG( PDERROR, outStr.c_str() ) ;
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

   /*
      _remoteSystemListLoginUsers implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemListLoginUsers )

   _remoteSystemListLoginUsers::_remoteSystemListLoginUsers()
   {
   }

   _remoteSystemListLoginUsers::~_remoteSystemListLoginUsers()
   {
   }

   const CHAR * _remoteSystemListLoginUsers::name()
   {
      return OMA_REMOTE_SYS_LIST_LOGIN_USERS ;
   }

   INT32 _remoteSystemListLoginUsers::doit( BSONObj &retObj )
   {
#if defined (_LINUX)
      INT32 rc           = SDB_OK ;
      BSONObjBuilder     builder ;
      BOOLEAN showDetail = FALSE ;
      UINT32 exitCode    = 0 ;
      stringstream       cmd ;
      _ossCmdRunner      runner ;
      string             outStr ;

      // check argument and build cmd
      cmd << "who" ;
      showDetail = _optionObj.getBoolField( "detail" ) ;

      // run cmd
      rc = runner.exec( cmd.str().c_str(), exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         stringstream ss ;
         ss << "failed to exec cmd " << cmd.str() << ",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         PD_LOG_MSG( PDERROR,ss.str().c_str() ) ;
         goto error ;
      }

      // get result
      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         stringstream ss ;
         ss << "failed to read msg from cmd \"" << cmd.str() << "\", rc:"
            << rc ;
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
         goto error ;
      }

      // extract result
      rc = _extractLoginUsersInfo( outStr.c_str(), builder, showDetail ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      retObj = builder.obj() ;
   done:
      return rc ;
   error:
      goto done ;

#elif defined (_WINDOWS)
      return SDB_OK ;
#endif
   }

   INT32 _remoteSystemListLoginUsers::_extractLoginUsersInfo( const CHAR *buf,
                                                              BSONObjBuilder &builder,
                                                              BOOLEAN showDetail )
   {
      INT32 rc = SDB_OK ;
      vector<string> splited ;
      vector< BSONObj > userVec ;

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
            userObjBuilder.append( CMD_USR_SYSTEM_LOGINUSER_USER, columns[ 0 ] ) ;
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

   /*
      _remoteSystemListAllUsers implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemListAllUsers )

   _remoteSystemListAllUsers::_remoteSystemListAllUsers()
   {
   }

   _remoteSystemListAllUsers::~_remoteSystemListAllUsers()
   {
   }

   const CHAR * _remoteSystemListAllUsers::name()
   {
      return OMA_REMOTE_SYS_LIST_ALL_USERS ;
   }

   INT32 _remoteSystemListAllUsers::doit( BSONObj &retObj )
   {
#if defined (_LINUX)
      INT32 rc           = SDB_OK ;
      BSONObjBuilder     builder ;
      BOOLEAN showDetail = FALSE ;
      UINT32 exitCode    = 0 ;
      stringstream       cmd ;
      _ossCmdRunner      runner ;
      string             outStr ;

      // check argument and build cmd
      cmd << "cat /etc/passwd" ;
      showDetail = _optionObj.getBoolField( "detail" ) ;

      // run cmd
      rc = runner.exec( cmd.str().c_str(), exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         stringstream ss ;
         ss << "failed to exec cmd " << cmd.str() << ",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
         goto error ;
      }

      // get result
      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         stringstream ss ;
         ss << "failed to read msg from cmd \"" << cmd.str() << "\", rc:"
            << rc ;
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
         goto error ;
      }

      // extract result
      rc = _extractAllUsersInfo( outStr.c_str(), builder, showDetail ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      retObj = builder.obj() ;
   done:
      return rc ;
   error:
      goto done ;

#elif defined (_WINDOWS)
      return SDB_OK ;
#endif
   }

   INT32 _remoteSystemListAllUsers::_extractAllUsersInfo( const CHAR *buf,
                                                          BSONObjBuilder &builder,
                                                          BOOLEAN showDetail )
   {
      INT32 rc = SDB_OK ;
      vector<string> splited ;
      vector< BSONObj > userVec ;

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
         boost::algorithm::split( splited, buf, boost::is_any_of("\n") ) ;
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
         for ( vector<string>::iterator itrSplit = splited.begin() ;
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
         for ( vector<string>::iterator itrSplit = splited.begin() ;
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

   /*
      _remoteSystemListGroups implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemListGroups )

   _remoteSystemListGroups::_remoteSystemListGroups()
   {
   }

   _remoteSystemListGroups::~_remoteSystemListGroups()
   {
   }

   const CHAR * _remoteSystemListGroups::name()
   {
      return OMA_REMOTE_SYS_LIST_GROUPS ;
   }

   INT32 _remoteSystemListGroups::doit( BSONObj &retObj )
   {
#if defined (_LINUX)
      INT32 rc           = SDB_OK ;
      BSONObjBuilder     builder ;
      BOOLEAN showDetail = FALSE ;
      UINT32 exitCode    = 0 ;
      stringstream       cmd ;
      _ossCmdRunner      runner ;
      string             outStr ;

      // check argument and build cmd
      cmd << "cat /etc/group" ;
      showDetail = _optionObj.getBoolField( "detail" ) ;

      // run cmd
      rc = runner.exec( cmd.str().c_str(), exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         stringstream ss ;
         ss << "failed to exec cmd " << cmd.str() << ",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
         goto error ;
      }

      // get result
      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         stringstream ss ;
         ss << "failed to read msg from cmd \"" << cmd.str() << "\", rc:"
            << rc ;
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
         goto error ;
      }

      // extract result
      rc = _extractGroupsInfo( outStr.c_str(), builder, showDetail ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      retObj = builder.obj() ;
   done:
      return rc ;
   error:
      goto done ;

#elif defined (_WINDOWS)
      return SDB_OK ;
#endif
   }

   INT32 _remoteSystemListGroups::_extractGroupsInfo( const CHAR *buf,
                                                      BSONObjBuilder &builder,
                                                      BOOLEAN showDetail )
   {
      INT32 rc = SDB_OK ;
      vector<string> splited ;
      vector< BSONObj > groupVec ;

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
         for ( vector<string>::iterator itrSplit = splited.begin() ;
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
            groupObjBuilder.append( CMD_USR_SYSTEM_GROUP_MEMBERS, columns[ 3 ] ) ;
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

   /*
      _remoteSystemGetCurrentUser implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemGetCurrentUser )

   _remoteSystemGetCurrentUser::_remoteSystemGetCurrentUser()
   {
   }

   _remoteSystemGetCurrentUser::~_remoteSystemGetCurrentUser()
   {
   }

   const CHAR * _remoteSystemGetCurrentUser::name()
   {
      return OMA_REMOTE_SYS_GET_CURRENT_USER ;
   }

   INT32  _remoteSystemGetCurrentUser::doit( BSONObj &retObj )
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
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
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
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
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
      retObj = builder.obj() ;
   done:
      return rc ;
   error:
      goto done ;

#elif defined (_WINDOWS)
      return SDB_OK ;
#endif
   }

   /*
      _remoteSystemKillProcess implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemKillProcess )

   _remoteSystemKillProcess::_remoteSystemKillProcess ()
   {
   }

   _remoteSystemKillProcess::~_remoteSystemKillProcess ()
   {
   }

   const CHAR* _remoteSystemKillProcess::name()
   {
      return OMA_REMOTE_SYS_KILL_PROCESS ;
   }

#if defined (_LINUX)
   INT32 _remoteSystemKillProcess::doit( BSONObj &retObj )
   {
      INT32 rc           = SDB_OK ;
      UINT32 exitCode    = 0 ;
      INT32 sigNum       = 15 ;
      stringstream       cmd ;
      _ossCmdRunner      runner ;
      string             outStr ;
      string             sig ;

      // check argument
      if ( TRUE == _optionObj.hasField( "sig" ) )
      {
         if ( String != _optionObj.getField( "sig" ).type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "sig must be string" ) ;
            goto error ;
         }

         sig = _optionObj.getStringField( "sig" ) ;
         if ( "term" != sig &&
              "kill" != sig )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "sig must be \"term\" or \"kill\"" ) ;
            goto error ;
         }
         else if ( "kill" == sig )
         {
            sigNum = 9 ;
         }
      }

      cmd << "kill -" << sigNum ;

      if ( FALSE == _matchObj.hasField( "pid" ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "pid must be config" ) ;
      }
      if ( NumberInt != _matchObj.getField( "pid" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "pid must be int" ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get pid, rc: %d", rc ) ;
      cmd << " " << _matchObj.getIntField( "pid" ) ;

      // run cmd
      rc = runner.exec( cmd.str().c_str(), exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         stringstream ss ;
         ss << "failed to exec cmd " << cmd.str() << ",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
         goto error ;
      }

      // get result
      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "Failed to read result" ) ;
         goto error ;
      }
      else if ( SDB_OK != exitCode )
      {
         rc = exitCode ;
         PD_LOG_MSG( PDERROR, outStr.c_str() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;

   }
#elif defined (_WINDOWS)
   INT32 _remoteSystemKillProcess::doit( BSONObj &retObj )
   {
      INT32 rc           = SDB_OK ;
      UINT32 exitCode    = 0 ;
      INT32 sigNum       = 15 ;
      stringstream       cmd ;
      _ossCmdRunner      runner ;
      string             outStr ;
      string             sig ;

      // check argument and build cmd
      if ( TRUE == _optionObj.hasField( "sig" ) )
      {
         if ( String != _optionObj.getField( "sig" ).type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "sig must be string" ) ;
            goto error ;
         }

         sig = _optionObj.getStringField( "sig" ) ;
         if ( "term" != sig &&
              "kill" != sig )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "sig must be \"term\" or \"kill\"" ) ;
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

      if ( FALSE == _matchObj.hasField( "pid" ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "pid must be config" ) ;
      }
      if ( NumberInt != _matchObj.getField( "pid" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "pid must be int" ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get pid, rc: %d", rc ) ;
      cmd << " /PID " << _matchObj.getIntField( "pid" ) ;

      // run cmd
      rc = runner.exec( cmd.str().c_str(), exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         stringstream ss ;
         ss << "failed to exec cmd " << cmd.str() << ",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
         goto error ;
      }

      // raed result
      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "Failed to read result" ) ;
         goto error ;
      }
      if ( SDB_OK != exitCode )
      {
         rc = exitCode ;
         PD_LOG_MSG( PDERROR, outStr.c_str() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;

   }
#endif

   /*
      _remoteSystemGetProcUlimitConfigs implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemGetProcUlimitConfigs )

   _remoteSystemGetProcUlimitConfigs::_remoteSystemGetProcUlimitConfigs ()
   {
   }

   _remoteSystemGetProcUlimitConfigs::~_remoteSystemGetProcUlimitConfigs ()
   {
   }

   const CHAR* _remoteSystemGetProcUlimitConfigs::name()
   {
      return OMA_REMOTE_SYS_GET_PROC_ULIMIT_CONFIGS ;
   }

   INT32 _remoteSystemGetProcUlimitConfigs::doit( BSONObj &retObj )
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
      if ( FALSE == _optionObj.isEmpty() ||
           FALSE == _matchObj.isEmpty() ||
           FALSE == _valueObj.isEmpty() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "getUlimitConfigs() should have non arguments" ) ;
         goto error ;
      }

      // check argument
      for ( UINT32 index = 0; index < CMD_RESOURCE_NUM; index++ )
      {
         rlimit rlim ;
         if ( 0 != getrlimit( resourceType[ index ], &rlim ) )
         {
            rc = SDB_SYS ;
            PD_LOG_MSG( PDERROR, "Failed to get user limit info" ) ;
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

      retObj = builder.obj() ;
   done:
      return rc ;
   error:
      goto done ;

#elif defined (_WINDOWS)
      return SDB_OK ;
#endif
   }

   /*
      _remoteSystemSetProcUlimitConfigs implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemSetProcUlimitConfigs )

   _remoteSystemSetProcUlimitConfigs::_remoteSystemSetProcUlimitConfigs ()
   {
   }

   _remoteSystemSetProcUlimitConfigs::~_remoteSystemSetProcUlimitConfigs ()
   {
   }

   const CHAR* _remoteSystemSetProcUlimitConfigs::name()
   {
      return OMA_REMOTE_SYS_SET_PROC_ULIMIT_CONFIGS ;
   }

   INT32 _remoteSystemSetProcUlimitConfigs::doit( BSONObj &retObj )
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
      if ( FALSE == _valueObj.hasField( "configs") )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "configsObj must be config" ) ;
         goto error ;
      }
      if ( Object != _valueObj.getField( "configs" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "configsObj must be obj" ) ;
         goto error ;
      }
      configsObj = _valueObj.getObjectField( "configs" ) ;

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
               PD_LOG_MSG( PDERROR, "value must be number or string" ) ;
               goto error ;
            }

            rlimit rlim ;
            if ( 0 != getrlimit( resourceType[ index ], &rlim ) )
            {
               rc = SDB_SYS ;
               PD_LOG_MSG( PDERROR, "Failed to get user limit info" ) ;
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
                  PD_LOG_MSG( PDERROR, "%s could not be interpreted as number",
                              configsObj.getStringField( limitItem ) ) ;
                  goto error ;
               }
            }
            if ( 0 != setrlimit( resourceType[ index ], &rlim ) )
            {
               if ( EINVAL == errno )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG_MSG( PDERROR, "invalid argument: %s",
                              limitItem ) ;
                  goto error ;
               }
               else if ( EPERM == errno )
               {
                  rc = SDB_PERM ;
                  PD_LOG_MSG( PDERROR, "Permission error" ) ;
                  goto error ;
               }
               else
               {
                  rc = SDB_SYS ;
                  PD_LOG_MSG( PDERROR, "Failed to set ulimit configs" ) ;
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

   /*
      _remoteSystemGetSystemConfigs implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemGetSystemConfigs )

   _remoteSystemGetSystemConfigs::_remoteSystemGetSystemConfigs ()
   {
   }

   _remoteSystemGetSystemConfigs::~_remoteSystemGetSystemConfigs ()
   {
   }

   const CHAR* _remoteSystemGetSystemConfigs::name()
   {
      return OMA_REMOTE_SYS_GET_SYSTEM_CONFIGS ;
   }

   INT32 _remoteSystemGetSystemConfigs::doit( BSONObj &retObj )
   {
#if defined (_LINUX)
      INT32 rc           = SDB_OK ;
      BSONObjBuilder     builder ;
      string             type ;
      vector<string>     typeSplit ;
      string configsType[] = { "kernel", "vm", "fs", "debug", "dev", "abi",
                               "net" } ;

      if ( TRUE == _optionObj.hasField( "type" ) )
      {
         if( String != _optionObj.getField( "type" ).type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "type must be string" ) ;
            goto error ;
         }

         type =  _optionObj.getStringField( "type" ) ;
         // input format: "xxx|xxxx|xxx"
         try
         {
            boost::algorithm::split( typeSplit, type, boost::is_any_of( " |" ) ) ;
         }
         catch( std::exception )
         {
            rc = SDB_SYS ;
            PD_LOG_MSG( PDERROR, "Failed to split result" ) ;
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
         PD_LOG_MSG( PDERROR, "Failed to get system info" ) ;
         goto error ;
      }

      retObj = builder.obj() ;
   done:
      return rc ;
   error:
      goto done ;

#elif defined (_WINDOWS)
      return SDB_OK ;
#endif
   }

   INT32 _remoteSystemGetSystemConfigs::_getSystemInfo( std::vector< std::string > typeSplit,
                                                        bson::BSONObjBuilder &builder )
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

         key = *( keySplit.begin()+2 );
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

   /*
      _remoteSystemRunService implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemRunService )

   _remoteSystemRunService::_remoteSystemRunService ()
   {
   }

   _remoteSystemRunService::~_remoteSystemRunService ()
   {
   }

   const CHAR* _remoteSystemRunService::name()
   {
      return OMA_REMOTE_SYS_RUN_SERVICE ;
   }

   INT32 _remoteSystemRunService::doit( BSONObj &retObj )
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
      if ( FALSE == _matchObj.hasField( "serviceName" ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "serviceName must be config" ) ;
         goto error ;
      }
      if ( String != _matchObj.getField( "serviceName" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "serviceName must be string" ) ;
         goto error ;
      }
      serviceName = _matchObj.getStringField( "serviceName" ) ;

      if ( FALSE == _optionObj.hasField( "command" ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "command must be config" ) ;
         goto error ;
      }
      if ( String != _optionObj.getField( "command" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "command must be string" ) ;
         goto error ;
      }
      command = _optionObj.getStringField( "command" ) ;

#if defined (_LINUX)
      cmd << "service " << serviceName << " " << command ;
#elif defined (_WINDOWS)
      cmd << "sc " << command << " " << serviceName ;
#endif

      if ( TRUE == _optionObj.hasField( "options" ) )
      {
         if ( String != _optionObj.getField( "options" ).type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "options must be string" ) ;
            goto error ;
         }
         options = _optionObj.getStringField( "options" ) ;

         cmd << " " << options ;
      }

      // run cmd
      rc = runner.exec( cmd.str().c_str(), exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         stringstream ss ;
         ss << "failed to exec cmd " << cmd.str() << ",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
         goto error ;
      }

      // get result
      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         stringstream ss ;
         ss << "failed to read msg from cmd \"" << cmd.str() << "\", rc:"
            << rc ;
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
         goto error ;
      }
      else if ( SDB_OK != exitCode )
      {
         rc = exitCode ;
         PD_LOG_MSG( PDERROR, outStr.c_str() ) ;
         goto error ;
      }
      if( '\n' == outStr[ outStr.size() - 1 ]  )
      {
         outStr.erase( outStr.size()-1, 1 ) ;
      }

      retObj = BSON( "outStr" << outStr ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteSystemBuildTrusty implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemBuildTrusty )

   _remoteSystemBuildTrusty::_remoteSystemBuildTrusty ()
   {
   }

   _remoteSystemBuildTrusty::~_remoteSystemBuildTrusty ()
   {
   }

   const CHAR* _remoteSystemBuildTrusty::name()
   {
      return OMA_REMOTE_SYS_BUILD_TRUSTY ;
   }

   INT32 _remoteSystemBuildTrusty::doit( BSONObj &retObj )
   {
#if defined(_LINUX)
      INT32           rc = SDB_OK ;
      UINT32          exitCode    = 0 ;
      stringstream    cmd ;
      stringstream    grepCmd ;
      stringstream    echoCmd ;
      _ossCmdRunner   runner ;
      string          outStr ;
      string          pubKey ;
      string          homePath ;
      string          sshPath ;
      string          keyPath ;

      // get pub key
      if ( FALSE == _valueObj.hasField( "key" ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "key must be config" ) ;
         goto error ;
      }
      if ( String != _valueObj.getField( "key" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "key must be string" ) ;
         goto error ;
      }
      pubKey = _valueObj.getStringField( "key" ) ;

      rc = _getHomePath( homePath ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "Failed to get home path" ) ;
         goto error ;
      }

      sshPath = homePath + "/.ssh/" ;
      rc = ossAccess( sshPath.c_str() ) ;
      if ( SDB_OK != rc )
      {
         if ( SDB_FNE == rc )
         {
            rc = ossMkdir( sshPath.c_str(), OSS_RWXU ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG_MSG( PDERROR, "Failed to mkdir, rc:%d", rc ) ;
               goto error ;
            }
         }
         else
         {
            PD_LOG( PDERROR, "Failed to access dir: %s", sshPath.c_str() ) ;
            goto error ;
         }
      }

      // check whether it had build Trusty
      grepCmd << "touch " << homePath << "/.ssh/authorized_keys; grep -x \""
              << pubKey << "\" "<< homePath << "/.ssh/authorized_keys" ;
      rc = runner.exec( grepCmd.str().c_str(), exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         stringstream ss ;
         ss << "failed to exec cmd " << grepCmd.str() << ",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
         goto error ;
      }

      rc = runner.read( outStr ) ;
      if( '\n' == outStr[ outStr.size()-1 ] )
      {
         outStr.erase( outStr.size()-1, 1 ) ;
      }

      // if not, build Trusty
      if ( outStr.empty() )
      {
         // echo pub key to authorized_keys
         cmd << "echo -n \"" << pubKey << "\" >> " << homePath
             << "/.ssh/authorized_keys" ;
         rc = runner.exec( cmd.str().c_str(), exitCode,
                           FALSE, -1, FALSE, NULL, TRUE ) ;
         if ( SDB_OK != rc )
         {
            stringstream ss ;
            ss << "failed to exec cmd " << cmd.str() << ",rc:"
               << rc
               << ",exit:"
               << exitCode ;
            PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
            goto error ;
         }

         // set authorizedkeys mode
         echoCmd << "chmod 755 " << homePath << ";"
                 << "chmod 700 " << homePath << "/.ssh;"
                 << "chmod 644 " << homePath << "/.ssh/authorized_keys" ;
         rc = runner.exec( echoCmd.str().c_str(), exitCode, FALSE,
                           -1, FALSE, NULL, TRUE ) ;
         if ( SDB_OK != rc )
         {
            stringstream ss ;
            ss << "failed to exec cmd " << "chmod 600 authorized_keys" << ",rc:"
               << rc
               << ",exit:"
               << exitCode ;
            PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
            goto error ;
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

   /*
      _remoteSystemRemoveTrusty implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemRemoveTrusty )

   _remoteSystemRemoveTrusty::_remoteSystemRemoveTrusty ()
   {
   }

   _remoteSystemRemoveTrusty::~_remoteSystemRemoveTrusty ()
   {
   }

   const CHAR* _remoteSystemRemoveTrusty::name()
   {
      return OMA_REMOTE_SYS_REMOVE_TRUSTY ;
   }

   INT32 _remoteSystemRemoveTrusty::doit( BSONObj &retObj )
   {
#if defined(_LINUX)
      INT32  rc          = SDB_OK ;
      UINT32 exitCode    = 0 ;
      string             matchStr ;
      stringstream       cmd ;
      _ossCmdRunner      runner ;
      string             outStr ;
      string             homePath ;

      if ( FALSE == _valueObj.hasField( "matchStr" ) )
      {
         rc = SDB_OUT_OF_BOUND;
         PD_LOG_MSG( PDERROR, "matchStr must be config" ) ;
         goto error ;
      }

      if ( String != _valueObj.getField( "matchStr" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "matchStr must be string" ) ;
         goto error ;
      }
      matchStr = _valueObj.getStringField( "matchStr" ) ;

      rc = _getHomePath( homePath ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get root dir" ) ;
         goto error ;
      }

      // get authorized_keys content
      cmd << "cat " << homePath << "/.ssh/authorized_keys";

      rc = runner.exec( cmd.str().c_str(), exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         stringstream ss ;
         ss << "failed to exec cmd " << cmd.str() << ",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
         goto error ;
      }

      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         stringstream ss ;
         ss << "failed to read msg from cmd \"" << cmd.str() << "\", rc:"
            << rc ;
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
         goto error ;
      }

      rc = _removeKey( outStr.c_str(), matchStr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "Failed to remove key" ) ;
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

   INT32 _remoteSystemRemoveTrusty::_removeKey( const CHAR* buf,
                                                string matchStr )
   {
#if defined (_LINUX)
      INT32 rc = SDB_OK ;
      OSSFILE file ;
      string fileDir ;
      vector<string> splited ;

      if ( NULL == buf )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "buf can't be null, rc: %d", rc ) ;
         goto error ;
      }

      if ( '\n' == matchStr[ matchStr.size()-1] )
      {
         matchStr.erase( matchStr.size()-1, 1 ) ;
      }

      rc = _getHomePath( fileDir ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get root dir" ) ;
         goto error ;
      }
      fileDir += "/.ssh/authorized_keys" ;

      try
      {
         boost::algorithm::split( splited, buf, boost::is_any_of("\n") ) ;
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

      rc = ossOpen( fileDir.c_str(),
                    OSS_READWRITE | OSS_REPLACE,
                    OSS_RWXU, file ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to open file" ) ;
         goto error ;
      }

      for ( vector<string>::iterator itr = splited.begin();
            itr != splited.end(); itr++ )
      {
         // check if pubkey exist
         if ( matchStr != *itr )
         {
            rc = ossWriteN( &file, ( *itr + "\n" ).c_str() , itr->size() + 1 ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to write file" ) ;
               goto error ;
            }
         }
      }

      if ( file.isOpened() )
      {
         rc  = ossClose( file ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to close file" ) ;
            goto error ;
         }
      }
   done:
      return rc ;
   error:
      if ( file.isOpened() )
      {
         ossClose( file ) ;
      }
      goto done ;

#elif defined (_WINDOWS)
      return SDB_OK ;
#endif
   }

   /*
      _remoteSystemGetUserEnv implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemGetUserEnv )

   _remoteSystemGetUserEnv::_remoteSystemGetUserEnv ()
   {
   }

   _remoteSystemGetUserEnv::~_remoteSystemGetUserEnv ()
   {
   }

   const CHAR* _remoteSystemGetUserEnv::name()
   {
      return OMA_REMOTE_SYS_GET_USER_ENV ;
   }

   INT32 _remoteSystemGetUserEnv::doit( BSONObj &retObj )
   {
      INT32 rc            = SDB_OK ;
      BSONObjBuilder      builder ;
      UINT32 exitCode     = 0 ;
      string              cmd ;
      _ossCmdRunner       runner ;
      string              outStr ;

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
         stringstream ss ;
         ss << "failed to exec cmd " << cmd << ",rc: "
            << rc
            << ",exit: "
            << exitCode ;
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
         goto error ;
      }

      // get result
      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         stringstream ss ;
         ss << "failed to read msg from cmd \"" << cmd << "\", rc:"
            << rc ;
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
         goto error ;
      }

      // extract result
      rc = _extractEnvInfo( outStr.c_str(), builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "Failed to extract env info" ) ;
         goto error ;
      }
      retObj = builder.obj() ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _remoteSystemGetUserEnv::_extractEnvInfo( const CHAR *buf,
                                                   BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      vector<string> splited ;
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
         boost::algorithm::split( splited, buf, boost::is_any_of("\n") ) ;
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
                                     boost::is_any_of( "=" ) ) ;
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

   /*
      _remoteSystemGetPID implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemGetPID )

   _remoteSystemGetPID::_remoteSystemGetPID ()
   {
   }

   _remoteSystemGetPID::~_remoteSystemGetPID ()
   {
   }

   const CHAR* _remoteSystemGetPID::name()
   {
      return OMA_REMOTE_SYS_GET_PID ;
   }

   INT32 _remoteSystemGetPID::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      UINT32 id = 0 ;
      stringstream ss ;
      BSONObjBuilder builder ;

      if ( FALSE == _optionObj.isEmpty() ||
           FALSE == _matchObj.isEmpty() ||
           FALSE == _valueObj.isEmpty() )
      {
         rc = SDB_INVALIDARG ;
         ss << "No need arguments" ;
         goto error ;
      }

      id = ossGetCurrentProcessID() ;
      builder.append( "PID", id ) ;
      retObj = builder.obj() ;
   done:
      return rc ;
   error:
      PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
      goto done ;
   }

   /*
      _remoteSystemGetTID implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemGetTID )

   _remoteSystemGetTID::_remoteSystemGetTID ()
   {
   }

   _remoteSystemGetTID::~_remoteSystemGetTID ()
   {
   }

   const CHAR* _remoteSystemGetTID::name()
   {
      return OMA_REMOTE_SYS_GET_TID ;
   }

   INT32 _remoteSystemGetTID::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      UINT32 id = 0 ;
      stringstream ss ;
      BSONObjBuilder builder ;

      if ( FALSE == _optionObj.isEmpty() ||
           FALSE == _matchObj.isEmpty() ||
           FALSE == _valueObj.isEmpty() )
      {
         rc = SDB_INVALIDARG ;
         ss << "No need arguments" ;
         goto error ;
      }

      id = (UINT32)ossGetCurrentThreadID() ;
      builder.append( "TID", id ) ;
      retObj = builder.obj() ;
   done:
      return rc ;
   error:
      PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
      goto done ;
   }

   /*
      _remoteSystemGetEWD implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemGetEWD )

   _remoteSystemGetEWD::_remoteSystemGetEWD ()
   {
   }

   _remoteSystemGetEWD::~_remoteSystemGetEWD ()
   {
   }

   const CHAR* _remoteSystemGetEWD::name()
   {
      return OMA_REMOTE_SYS_GET_EWD ;
   }

   INT32 _remoteSystemGetEWD::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      CHAR buf[ OSS_MAX_PATHSIZE + 1 ] = {0} ;
      stringstream ss ;
      BSONObjBuilder builder ;

      if ( FALSE == _optionObj.isEmpty() ||
           FALSE == _matchObj.isEmpty() ||
           FALSE == _valueObj.isEmpty() )
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
      builder.append( "EWD", buf ) ;
      retObj = builder.obj() ;
   done:
      return rc ;
   error:
      PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
      goto done ;
   }
}
