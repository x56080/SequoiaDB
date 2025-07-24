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

   Source File Name = omagentRemoteBase.cpp

   Dependencies: N/A


   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/03/2016  WJM  Initial Draft

   Last Changed =

*******************************************************************************/
#include "omagentRemoteBase.hpp"
#include "cmdUsrOmaUtil.hpp"
#include "pmdOptions.h"
#include "msgDef.h"
#include "pmd.hpp"
#include "ossCmdRunner.hpp"
#include "ossSocket.hpp"
#include "ossIO.hpp"
#include "oss.h"
#include "ossProc.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#if defined (_LINUX)
#include <net/if.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <pwd.h>
#else
#include <iphlpapi.h>
#pragma comment( lib, "IPHLPAPI.lib" )
#endif

using namespace bson ;

namespace engine
{
   /*
      _remoteExec implement
   */

   _remoteExec::_remoteExec()
   {
   }

   _remoteExec::~_remoteExec()
   {
   }

   INT32 _remoteExec::init( const CHAR * pInfomation )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONObj Obj( pInfomation ) ;

         _optionObj = Obj.getObjectField( "$optionObj" ) ;
         _matchObj  = Obj.getObjectField( "$matchObj" ) ;
         _valueObj  = Obj.getObjectField( "$valueObj" ) ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteHost implement
   */

   _remoteHost::_remoteHost()
   {
   }

   _remoteHost::~_remoteHost()
   {
   }

   INT32 _remoteHost::_parseHostsFile( VEC_HOST_ITEM &vecItems, string &err )
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

   INT32 _remoteHost::_writeHostsFile( VEC_HOST_ITEM & vecItems,
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

   INT32 _remoteHost::_extractHosts( const CHAR *buf,
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
         vector<string> columns ;
         if ( itr->empty() )
         {
            vecItems.push_back( item ) ;
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

   void _remoteHost::_buildHostsResult( VEC_HOST_ITEM & vecItems,
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

   /*
      _remoteOmaConfigs implement
   */

   _remoteOmaConfigs::_remoteOmaConfigs()
   {
   }

   _remoteOmaConfigs::~_remoteOmaConfigs()
   {
   }

   INT32 _remoteOmaConfigs::_getOmaConfFile( string &confFile )
   {
      INT32 rc = SDB_OK ;
      utilInstallInfo info ;
      CHAR confPath[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;

      if ( SDB_OK == utilGetInstallInfo( info ) )
      {
         // info._path + conf/sdbcm.conf
         if ( SDB_OK == utilBuildFullPath( info._path.c_str(),
                                           SPT_OMA_REL_PATH_FILE,
                                           OSS_MAX_PATHSIZE,
                                           confPath ) &&
              SDB_OK == ossAccess( confPath ) )
         {
            goto done ;
         }
      }

      // exePath + ../conf/sdbcm.conf
      rc = ossGetEWD( confPath, OSS_MAX_PATHSIZE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get EWD, rc: %d", rc ) ;
         goto error ;
      }
      rc = utilCatPath( confPath, OSS_MAX_PATHSIZE, SDBCM_CONF_PATH_FILE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to cat path, rc: %d", rc ) ;
         goto error ;
      }
      rc = ossAccess( confPath ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to access config file: %d, rc: %d",
                 confPath, rc ) ;
         goto error ;
      }
   done:
      if ( SDB_OK == rc )
      {
         confFile = confPath ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _remoteOmaConfigs::_getOmaConfInfo( const string & confFile,
                                             BSONObj &conf,
                                             string &errMsg,
                                             BOOLEAN allowNotExist )
   {
      INT32 rc = SDB_OK ;
      po::options_description desc ;
      po::variables_map vm ;

      MAP_CONFIG_DESC( desc ) ;

      rc = ossAccess( confFile.c_str() ) ;
      if ( rc )
      {
         if ( allowNotExist )
         {
            rc = SDB_OK ;
            goto done ;
         }
         stringstream ss ;
         ss << "conf file[" << confFile << "] is not exist" ;
         errMsg = ss.str() ;
         goto error ;
      }

      rc = utilReadConfigureFile( confFile.c_str(), desc, vm ) ;
      if ( SDB_FNE == rc )
      {
         stringstream ss ;
         ss << "conf file[" << confFile << "] is not exist" ;
         errMsg = ss.str() ;
         goto error ;
      }
      else if ( SDB_PERM == rc )
      {
         stringstream ss ;
         ss << "conf file[" << confFile << "] permission error" ;
         errMsg = ss.str() ;
         goto error ;
      }
      else if ( rc )
      {
         stringstream ss ;
         ss << "read conf file[" << confFile << "] error" ;
         errMsg = ss.str() ;
         goto error ;
      }
      else
      {
         BSONObjBuilder builder ;
         po ::variables_map::iterator it = vm.begin() ;
         while ( it != vm.end() )
         {
            if ( SDBCM_RESTART_COUNT == it->first ||
                 SDBCM_RESTART_INTERVAL == it->first ||
                 SDBCM_DIALOG_LEVEL == it->first )
            {
               try
               {
                  builder.append( it->first, it->second.as<INT32>() ) ;
               }
               catch( std::exception &e )
               {
                  PD_LOG_MSG( PDERROR, "Failed to append config item: %s(%s)",
                              it->first.c_str(), e.what() ) ;
               }
            }
            else if ( SDBCM_AUTO_START == it->first )
            {
               BOOLEAN autoStart = TRUE ;
               try
               {
                  ossStrToBoolean( it->second.as<string>().c_str(), &autoStart ) ;
               }
               catch( std::exception &e )
               {
                  PD_LOG_MSG( PDERROR, "Failed to turn convert Boolean item: "
                              "%s(%s) to string", it->first.c_str(), e.what() ) ;
               }
               builder.appendBool( it->first, autoStart ) ;
            }
            else
            {
               try
               {
                  builder.append( it->first, it->second.as<string>() ) ;
               }
               catch( std::exception &e )
               {
                  PD_LOG_MSG( PDERROR, "Failed to append config item: %s(%s)",
                              it->first.c_str(), e.what() ) ;
               }
            }
            ++it ;
         }
         conf = builder.obj() ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32  _remoteOmaConfigs::_confObj2Str( const bson::BSONObj &conf, string &str,
                                           string &errMsg,
                                           const CHAR* pExcept )
   {
      INT32 rc = SDB_OK ;
      stringstream ss ;
      BSONObjIterator it ( conf ) ;
      while ( it.more() )
      {
         BSONElement e = it.next() ;

         if ( pExcept && 0 != pExcept[0] &&
              0 == ossStrcmp( pExcept, e.fieldName() ) )
         {
            continue ;
         }

         ss << e.fieldName() << "=" ;
         if ( e.type() == String )
         {
            ss << e.valuestr() ;
         }
         else if ( e.type() == NumberInt )
         {
            ss << e.numberInt() ;
         }
         else if ( e.type() == NumberLong )
         {
            ss << e.numberLong() ;
         }
         else if ( e.type() == NumberDouble )
         {
            ss << e.numberDouble() ;
         }
         else if ( e.type() == Bool )
         {
            ss << ( e.boolean() ? "TRUE" : "FALSE" ) ;
         }
         else
         {
            rc = SDB_INVALIDARG ;
            stringstream errss ;
            errss << e.toString() << " is invalid config" ;
            errMsg = errss.str() ;
            goto error ;
         }
         ss << endl ;
      }
      str = ss.str() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _remoteOmaConfigs::_getNodeConfigFile( string svcname,
                                                string &filePath )
   {
      INT32 rc = SDB_OK ;
      utilInstallInfo info ;
      CHAR confFile[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      // ../conf/local/SVCNAME/sdb.conf
      string ewdConfPath = SDBCM_LOCAL_PATH OSS_FILE_SEP
                           + svcname + OSS_FILE_SEP PMD_DFT_CONF ;

      // get ewd
      rc = ossGetEWD( confFile, OSS_MAX_PATHSIZE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get ewd, rc: %s", rc ) ;
         goto error ;
      }

      // get full path: ewd + ../conf/local/SVCNAME/sdb.conf
      rc = utilCatPath( confFile, OSS_MAX_PATHSIZE, ewdConfPath.c_str() ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to cat path: %s", rc ) ;
         goto error ;
      }

      rc = ossAccess( confFile ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to access config file: %d, rc: %d",
                 confFile, rc ) ;
         goto error ;
      }
   done:
      if ( SDB_OK == rc )
      {
         filePath = confFile ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _remoteOmaConfigs::_getNodeConfInfo( const string & confFile,
                                              BSONObj &conf,
                                              string &errMsg,
                                              BOOLEAN allowNotExist )
   {
      INT32 rc = SDB_OK ;
      po::options_description desc ;
      po ::variables_map vm ;

      MAP_CONFIG_DESC( desc ) ;

      rc = ossAccess( confFile.c_str() ) ;
      if ( rc )
      {
         if ( allowNotExist )
         {
            rc = SDB_OK ;
            goto done ;
         }
         stringstream ss ;
         ss << "conf file[" << confFile << "] is not exist" ;
         errMsg = ss.str() ;
         goto error ;
      }

      rc = utilReadConfigureFile( confFile.c_str(), desc, vm ) ;
      if ( SDB_FNE == rc )
      {
         stringstream ss ;
         ss << "conf file[" << confFile << "] is not exist" ;
         errMsg = ss.str() ;
         goto error ;
      }
      else if ( SDB_PERM == rc )
      {
         stringstream ss ;
         ss << "conf file[" << confFile << "] permission error" ;
         errMsg = ss.str() ;
         goto error ;
      }
      else if ( rc )
      {
         stringstream ss ;
         ss << "read conf file[" << confFile << "] error" ;
         errMsg = ss.str() ;
         goto error ;
      }
      else
      {
         BSONObjBuilder builder ;
         po ::variables_map::iterator it = vm.begin() ;
         while ( it != vm.end() )
         {
            try
            {
               builder.append( it->first, it->second.as<string>() ) ;
            }
            catch( std::exception &e )
            {
               PD_LOG_MSG( PDERROR, "Failed to append config item: %s(%s)",
                           it->first.c_str(), e.what() ) ;
            }
            ++it ;
         }
         conf = builder.obj() ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteOmaGetHomePath implement
   */

   _remoteOmaGetHomePath::_remoteOmaGetHomePath()
   {
   }

   _remoteOmaGetHomePath::~_remoteOmaGetHomePath()
   {
   }

   INT32 _remoteOmaGetHomePath::_getHomePath( string &homePath )
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

}
