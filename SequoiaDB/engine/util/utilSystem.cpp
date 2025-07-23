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

   Source File Name = utilSystem.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== ==============================================
          07/05/2023  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#include "utilSystem.hpp"
#include "ossUtil.hpp"
#include "pd.hpp"
#include "ossFile.hpp"
#include "ossCmdRunner.hpp"
#include "utilStr.hpp"
#include <boost/algorithm/string.hpp>
#if defined(_LINUX)
#include <gnu/libc-version.h>
#include <sys/resource.h>
#endif

#if defined(_LINUX)

   #if defined(_ARMLIN64)
      #define UTIL_SYS_GET_CPU_INFO_CMD "cat /proc/cpuinfo | grep -E 'model name'"
   #elif defined (_PPCLIN64)
      #define UTIL_SYS_GET_CPU_INFO_CMD "cat /proc/cpuinfo | grep -E 'processor|cpu|clock|machine'"
   #else
      #define UTIL_SYS_GET_CPU_INFO_CMD "cat /proc/cpuinfo | grep -E 'processor|model name|cpu MHz|flags|core id|physical id|vmx flags'"
   #endif

#elif defined(_WINDOWS)
   #define UTIL_SYS_GET_CPU_INFO_CMD "wmic CPU GET CurrentClockSpeed,Name,NumberOfCores"
#endif

#define UTIL_SYS_GET_RELEASE_INFO_CMD     "lsb_release -a |grep -v \"LSB Version\""

#define UTIL_SYS_REDHAT_RELEASE_FILE      "/etc/redhat-release"
#define UTIL_SYS_SUSE_RELEASE_FILE        "/etc/SuSE-release"
#define UTIL_SYS_OS_RELEASE_FILE          "/etc/os-release"

#define UTIL_SYS_MULTI_NODES_FILE_NAME1   "/sys/devices/system/node/node1"
#define UTIL_SYS_MULTI_NODES_FILE_NAME    "/sys/devices/system/node/node"
#define UTIL_SYS_NUMA_MAP_FILE_NAME       "/proc/self/numa_maps"
#define UTIL_SYS_NUMA_INTERLEAVE          "interleave"

#define UTIL_SYS_PROC_VERSION_FILE_NAME   "/proc/version"
#define UTIL_SYS_PROC_VERSION_SIGNA       "/proc/version_signature"

namespace engine
{
#if defined (_LINUX)
   #if defined (_ARMLIN64)

   static INT32 _extractCpuInfo( const CHAR *buf,
                                 std::map< string, std::vector<cpuInfo> > &cpuInfos )
   {
      INT32 rc = SDB_OK ;
      std::string strModelName  = "model name" ;
      std::vector<string> splited ;
      std::vector<cpuInfo> tmp ;
      cpuInfo info ;

      try
      {
         boost::algorithm::split( splited, buf, boost::is_any_of( "\n" ) ) ;

         for ( auto itr = splited.begin();
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

         for ( auto itr = splited.begin(); itr != splited.end(); itr++ )
         {
            vector<string> columns ;
            boost::algorithm::split( columns, *itr, boost::is_any_of( "\t:" ),
                                     boost::token_compress_on ) ;

            for ( auto itr2 = columns.begin(); itr2 != columns.end(); itr2++ )
            {
               boost::algorithm::trim( *itr2 ) ;
            }

            if ( strModelName == columns.at( 0 ) )
            {
               info.modelName = columns.at( 1 ) ;
               tmp.push_back( info) ;
            }
            else
            {
               continue ;
            }
         }

         cpuInfos.insert( std::make_pair( "0", tmp ) ) ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "An exception occurred when extracting cpu info: "
                 "%s, rc: %d", e.what(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   #elif defined (_PPCLIN64)

   static INT32 _extractCpuInfo( const CHAR *buf,
                                 std::map< string, std::vector<cpuInfo> > &cpuInfos )
   {
      INT32 rc = SDB_OK ;
      // extract the follow 3 fields from the return content
      string strProcessor  = "processor" ;
      string strCpu        = "cpu" ;
      string strClock      = "clock" ;
      vector<string> splited ;
      // use to mark which field we had accessed
      INT32 flag = 0x00000000 ;
      BOOLEAN mustPush = FALSE ;
      cpuInfo info ;
      vector<cpuInfo> tmp ;

      try
      {
         boost::algorithm::split( splited, buf, boost::is_any_of( "\n" ) ) ;

         for ( auto itr = splited.begin();
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

         for ( auto itr = splited.begin(); itr != splited.end(); itr++ )
         {
            vector<string> columns ;
            boost::algorithm::split( columns, *itr, boost::is_any_of( "\t:" ),
                                     boost::token_compress_on ) ;

            for ( auto itr2 = columns.begin(); itr2 != columns.end(); itr2++ )
            {
               boost::algorithm::trim( *itr2 ) ;
            }

            mustPush = FALSE ;
            if ( strProcessor == columns.at(0) )
            {
               if ( ( flag ^ 0x00000001 ) > flag )
               {
                  info.processor = columns.at( 1 ) ;
                  flag ^= 0x00000001 ;
               }
               else
               {
                  mustPush = TRUE ;
               }
            }
            else if ( strCpu == columns.at(0) )
            {
               if ( ( flag ^ 0x00000010 ) > flag )
               {
                  info.modelName = columns.at( 1 ) ;
                  flag ^= 0x00000010 ;
               }
               else
               {
                  mustPush = TRUE ;
               }
            }
            else if ( strClock== columns.at(0) )
            {
               if ( ( flag ^ 0x00000100 ) > flag )
               {
                  info.clock = columns.at( 1 ) ;
                  flag ^= 0x00000100 ;
               }
               else
               {
                  mustPush = TRUE ;
               }
            }
            else
            {
               PD_LOG( PDERROR, "Unexpect field[%s]", columns.at(0).c_str() ) ;
               rc = SDB_SYS ;
               goto error ;
            }

            if ( TRUE == mustPush )
            {
               tmp.push_back( info ) ;
               flag = 0 ;
               info.reset() ;
               // need to decrease itr becase not use the value of itr
               itr-- ;
            }
         }

         if ( flag )
         {
            tmp.push_back( info ) ;
         }

         cpuInfos.insert( std::make_pair( "0", tmp ) ) ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "An exception occurred when extracting cpu info: "
                 "%s, rc: %d", e.what(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   #else

   static INT32 _extractCpuInfo( const CHAR *buf,
                                 std::map< string, std::vector<cpuInfo> > &cpuInfos )
   {
      INT32 rc = SDB_OK ;
      std::string strProcessor  = "processor" ;
      std::string strModelName  = "model name" ;
      std::string strFreq       = "cpu MHz" ;
      std::string strPhysicalID = "physical id" ;
      std::string strCoreID     = "core id" ;
      std::string strFlags      = "flags" ;
      std::string strVmxFlags   = "vmx flags" ;
      std::vector<string> splited ;
      // use to mark which field we had accessed
      INT32 flag = 0x00000000 ;
      BOOLEAN mustPush = FALSE ;
      cpuInfo info ;

      try
      {
         boost::algorithm::split( splited, buf, boost::is_any_of( "\n" ) ) ;

         for ( auto itr = splited.begin();
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

         for ( auto itr = splited.begin(); itr != splited.end(); itr++ )
         {
            vector<string> columns ;
            boost::algorithm::split( columns, *itr, boost::is_any_of( "\t:" ),
                                     boost::token_compress_on ) ;

            for ( auto itr2 = columns.begin(); itr2 != columns.end(); itr2++ )
            {
               boost::algorithm::trim( *itr2 ) ;
            }

            mustPush = FALSE ;
            if ( strProcessor == columns.at( 0 ) )
            {
               if ( ( flag ^ 0x00000001 ) > flag )
               {
                  info.processor = columns.at( 1 ) ;
                  flag ^= 0x00000001 ;
               }
               else
               {
                  mustPush = TRUE ;
               }
            }
            else if ( strModelName == columns.at( 0 ) )
            {
               if ( ( flag ^ 0x00000010 ) > flag )
               {
                  info.modelName = columns.at( 1 ) ;
                  flag ^= 0x00000010 ;
               }
               else
               {
                  mustPush = TRUE ;
               }
            }
            else if ( strFreq == columns.at( 0 ) )
            {
               if ( ( flag ^ 0x00000100 ) > flag )
               {
                  info.freq = columns.at( 1 ) ;
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
                  info.physicalID = columns.at(1) ;
                  flag ^= 0x00001000 ;
               }
               else
               {
                  mustPush = TRUE ;
               }
            }
            else if ( strCoreID == columns.at(0) )
            {
               if ( ( flag ^ 0x00010000 ) > flag )
               {
                  info.coreID = columns.at(1) ;
                  flag ^= 0x00010000 ;
               }
               else
               {
                  mustPush = TRUE ;
               }
            }
            else if ( strFlags == columns.at(0) )
            {
               if ( ( flag ^ 0x00100000 ) > flag )
               {
                  info.flags = columns.at(1) ;
                  flag ^= 0x00100000 ;
               }
               else
               {
                  mustPush = TRUE ;
               }
            }
            else if ( strVmxFlags == columns.at( 0 ) )
            {
               // do nothing
            }
            else
            {
               PD_LOG( PDERROR, "Unexpect field[%s]", columns.at(0).c_str() ) ;
               rc = SDB_SYS ;
               goto error ;
            }

            if ( TRUE == mustPush )
            {
               auto iter = cpuInfos.find( info.physicalID ) ;
               if ( iter != cpuInfos.end() )
               {
                  iter->second.push_back( info ) ;
               }
               else
               {
                  vector<cpuInfo> tmp ;
                  tmp.push_back( info ) ;
                  cpuInfos.insert( std::make_pair( info.physicalID, tmp ) ) ;
               }
               flag = 0 ;
               info.reset() ;
               // need to decrease itr becase not use the value of itr
               itr-- ;
            }
         }

         if ( flag )
         {
            auto iter = cpuInfos.find( info.physicalID ) ;
            if ( iter != cpuInfos.end() )
            {
               iter->second.push_back( info ) ;
            }
            else
            {
               vector<cpuInfo> tmp ;
               tmp.push_back( info ) ;
               cpuInfos.insert( std::make_pair( info.physicalID, tmp ) ) ;
            }
         }
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "An exception occurred when extracting cpu info: "
                 "%s, rc: %d", e.what(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   #endif
#else

   static INT32 _extractCpuInfo( const CHAR *buf,
                                 std::map< string, std::vector<cpuInfo> > &cpuInfos )
   {
      INT32 rc = SDB_OK ;
      vector<string> splited ;
      INT32 lineCount = 0 ;
      vector<cpuInfo> tmp ;
      cpuInfo info ;

      try
      {
         boost::algorithm::split( splited, buf, boost::is_any_of("\r\n") ) ;

         for ( vector<string>::iterator itr = splited.begin() ; itr != splited.end(); itr++ )
         {
            ++lineCount ;
            if ( 1 == lineCount || itr->empty() )
            {
               continue ;
            }
            vector<string> columns ;

            boost::algorithm::trim( *itr ) ;
            boost::algorithm::split( columns, *itr, boost::is_any_of("\t ") ) ;

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
            stringstream buf ;

            info.coreNum = boost::lexical_cast<UINT32>( columns.at( columns.size() - 1 ) ) ;

            for ( UINT32 i = 1; i < columns.size() - 1 ; i++ )
            {
               buf << columns.at( i ) << " " ;
            }
            info.sysInfo = buf.str() ;
            info.freq = columns[ 0 ] ;
            tmp.push_back( info ) ;
         }
         cpuInfos.insert( std::make_pair( "0", tmp ) ) ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "An exception occurred when extracting cpu info: "
                 "%s, rc: %d", e.what(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

#endif

   INT32 utilSysGetCpuInfo( std::map< string, std::vector<cpuInfo> > &cpuInfos )
   {
      INT32 rc = SDB_OK ;
      UINT32 exitCode = 0 ;
      ossCmdRunner runner ;
      string outStr ;

      rc = runner.exec( UTIL_SYS_GET_CPU_INFO_CMD, exitCode, FALSE,
                        -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc || SDB_OK != exitCode )
      {
         PD_LOG( PDERROR, "Failed to exec cmd[%s], rc: %d, exit: %d",
                 UTIL_SYS_GET_CPU_INFO_CMD, rc, exitCode ) ;
         rc = SDB_OK ;
      }
      else
      {
         rc = runner.read( outStr ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to read msg from cmd runner, rc: %d", rc ) ;
      }

      rc = _extractCpuInfo( outStr.c_str(), cpuInfos ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to extract cpu info, rc: %d", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   static INT32 _extractCmdReleaseInfo( const CHAR* buf,
                                        std::string &distributor,
                                        std::string &release,
                                        std::string &description )
   {
      INT32 rc = SDB_OK ;
      vector<string> splited ;

      try
      {
         // not performance sensitive.
         boost::algorithm::split( splited, buf, boost::is_any_of("\n:") ) ;

         for ( auto itr = splited.begin(); itr != splited.end(); itr++ )
         {
            boost::algorithm::trim( *itr ) ;
         }

         for ( auto itr = splited.begin(); itr != splited.end(); itr++ )
         {
            if ( itr->empty() )
            {
               continue ;
            }

            if ( "Distributor ID" == *itr && itr < splited.end() - 1 )
            {
               distributor = *( itr + 1 ) ;
            }
            else if ( "Release" == *itr && itr < splited.end() - 1 )
            {
               release = *( itr + 1 ) ;
            }
            else if ( "Description" == *itr && itr < splited.end() - 1 )
            {
               description = *( itr + 1 ) ;
            }
         }
         if ( distributor.empty() || release.empty() )
         {
            PD_LOG( PDERROR, "Failed to split release info: %s", buf )  ;
            rc = SDB_SYS ;
            goto error ;
         }
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "An exception occurred when extracting the release info of "
                 "lsb cmd: %s, rc: %d", e.what(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 utilSysGetOsReleaseInfoFromCmd( std::string &distributor,
                                         std::string &release,
                                         std::string &description )
   {
      INT32 rc = SDB_OK ;
      UINT32 exitCode = 0 ;
      ossCmdRunner runner ;
      string outStr ;

#if defined (_LINUX)
      rc = runner.exec( UTIL_SYS_GET_RELEASE_INFO_CMD, exitCode, FALSE,
                        -1, FALSE, NULL, TRUE ) ;
#elif defined (_WINDOWS)
      rc = SDB_SYS ;
#endif
      if ( SDB_OK != rc || SDB_OK != exitCode )
      {
         PD_LOG( PDERROR, "Failed to exec cmd[%s], rc: %d, exit: %d",
                  UTIL_SYS_GET_RELEASE_INFO_CMD, rc, exitCode ) ;
         rc = SDB_OK ;
      }
      else
      {
         rc = runner.read( outStr ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to read msg from cmd runner, rc: %d", rc ) ;
      }

      rc = _extractCmdReleaseInfo( outStr.c_str(), distributor, release, description ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to extract the release info of lsb cmd, rc: %d", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 utilSysGetOsReleaseInfoFromFile( std::string &distributor,
                                          std::string &release,
                                          std::string &description )
   {
      INT32 rc = SDB_OK ;
      INT64 fileSize = 0 ;
      CHAR *readBuffer = NULL ;
      INT64 readSize = 0 ;
      string releaseFilePath ;
      enum distroType { Redhat, Suse, OS } ;
      distroType type ;
      vector<string> splited ;
      ossFile file ;
      BOOLEAN isOpened = FALSE ;
      BOOLEAN ifExists ;

      try
      {
         if ( SDB_OK == ( rc = ossFile::exists( UTIL_SYS_REDHAT_RELEASE_FILE, ifExists ) ) &&
              true == ifExists )
         {
            releaseFilePath = UTIL_SYS_REDHAT_RELEASE_FILE ;
            type = Redhat ;
         }
         else if ( SDB_OK == ( rc = ossFile::exists( UTIL_SYS_SUSE_RELEASE_FILE, ifExists ) ) &&
                   true == ifExists )
         {
            releaseFilePath = UTIL_SYS_SUSE_RELEASE_FILE ;
            type = Suse ;
         }
         else if ( SDB_OK == ( rc = ossFile::exists( UTIL_SYS_OS_RELEASE_FILE, ifExists ) ) &&
                   true == ifExists )
         {
            releaseFilePath = UTIL_SYS_OS_RELEASE_FILE ;
            type = OS ;
         }
         else
         {
            if ( SDB_OK == rc && false == ifExists )
            {
               rc = SDB_FNE;
            }
            goto error ;
         }

         rc = file.open( releaseFilePath, OSS_READONLY|OSS_SHAREREAD, 0 ) ;
         if ( SDB_OK != rc )
         {
            goto error;
         }
         isOpened = TRUE ;

         rc = file.getFileSize( fileSize ) ;
         if ( SDB_OK != rc )
         {
            goto error;
         }

         readBuffer = ( CHAR* )SDB_OSS_MALLOC( fileSize + 1 ) ;
         if ( !readBuffer )
         {
            rc = SDB_OOM ;
            goto error ;
         }
         ossMemset( readBuffer, 0, fileSize + 1 ) ;

         rc = file.readN( readBuffer, fileSize, readSize ) ;
         if ( SDB_OK != rc )
         {
            goto error;
         }

         splited = utilStrSplit( readBuffer, "\n" ) ;
         if ( splited.empty() )
         {
            rc = SDB_SYS ;
            goto error ;
         }

         if ( Redhat == type )
         {
            description = splited[0] ;
            vector<string> words = utilStrSplit( splited[0] , " " ) ;
            for ( auto iter = words.begin(); iter != words.end(); iter++ )
            {
               if ( iter->empty() )
               {
                  continue ;
               }
               else if ( iter == words.begin() )
               {
                  if ( "CentOS" == *iter )
                  {
                     distributor = "CentOS" ;
                  }
                  else if ("Red" == *iter )
                  {
                     distributor = "RedHatEnterpriseServer" ;
                  }
               }
               else if ( "release" == *iter && iter != words.end() - 1 )
               {
                  release = *( iter + 1 ) ;
               }
            }
         }
         else if ( Suse == type )
         {
            distributor = "SUSE LINUX" ;
            for ( auto iter = splited.begin(); iter != splited.end(); iter++ )
            {
               if ( iter == splited.begin() )
               {
                  description = *iter ;
               }
               else
               {
                  vector<string> words = utilStrSplit( *iter , "=" ) ;
                  for ( auto iter = words.begin(); iter != words.end(); iter++ )
                  {
                     utilStrTrim( *iter ) ;
                     if ( "VERSION" == *iter && iter != words.end() - 1 )
                     {
                        utilStrTrim( *( iter + 1 )  ) ;
                        release = *( iter + 1 ) ;
                        break ;
                     }
                  }
               }
            }
         }
         else if ( OS == type )
         {
            for ( auto iter = splited.begin(); iter != splited.end(); iter++ )
            {
               vector<string> words = utilStrSplit( *iter , "=" ) ;
               for ( auto iter = words.begin(); iter != words.end(); iter++ )
               {
                  if ( iter->empty() )
                  {
                     continue ;
                  }
                  else if ( "NAME" == *iter && iter != words.end() -1 )
                  {
                     distributor = *( iter + 1 ) ;
                     boost::algorithm::erase_all( distributor, "\"" ) ;
                  }
                  else if ( "PRETTY_NAME" == *iter && iter != words.end() -1 )
                  {
                     description = *( iter + 1 ) ;
                     boost::algorithm::erase_all( description, "\"" ) ;
                  }
                  else if ( "VERSION_ID" == *iter && iter != words.end() -1 )
                  {
                     release = *( iter + 1 ) ;
                     boost::algorithm::erase_all( release, "\"" ) ;
                  }
               }
            }
         }

         if ( distributor.empty() || release.empty() || description.empty() )
         {
            rc = SDB_SYS ;
            goto error ;
         }
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "An exception occurred when getting os release info from file: "
                 "%s, rc: %d", e.what(), rc ) ;
         goto error ;
      }

   done:
      if ( isOpened )
      {
         file.close() ;
      }
      if ( readBuffer )
      {
         SDB_OSS_FREE( readBuffer ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 utilSysCountNumaNodes( UINT32 &numaNodes )
   {
      INT32 rc = SDB_OK ;
      OSSFILE file ;
      BOOLEAN isOpened = FALSE ;
      BOOLEAN hasMultiNodes = FALSE ;
      BOOLEAN hasNumaMaps = FALSE ;
      numaNodes = 0 ;

      rc = ossAccess( UTIL_SYS_MULTI_NODES_FILE_NAME1 ) ;
      if ( SDB_OK != rc && SDB_FNE != rc )
      {
         PD_LOG( PDERROR, "Failed to access file[%s], rc: %d", UTIL_SYS_MULTI_NODES_FILE_NAME1, rc ) ;
         goto error ;
      }
      else if ( SDB_OK == rc )
      {
         hasMultiNodes = TRUE ;
         numaNodes++ ;
      }

      rc = ossAccess( UTIL_SYS_NUMA_MAP_FILE_NAME ) ;
      if ( SDB_OK != rc && SDB_FNE != rc )
      {
         PD_LOG( PDERROR, "Failed to access file[%s], rc: %d", UTIL_SYS_NUMA_MAP_FILE_NAME, rc ) ;
         goto error ;
      }
      else if ( SDB_OK == rc )
      {
         hasNumaMaps = TRUE ;
      }
      rc = SDB_OK ;

      if ( hasMultiNodes && hasNumaMaps )
      {
         std::stringstream buf ;
         std::string line ;
         CHAR readChar = '\0' ;
         INT64 readSize = 0 ;
         size_t pos ;

         rc = ossOpen( UTIL_SYS_NUMA_MAP_FILE_NAME, OSS_READONLY|OSS_SHAREREAD, 0, file ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to open file[%s], rc: %d", UTIL_SYS_NUMA_MAP_FILE_NAME, rc ) ;
         isOpened = TRUE ;

         try
         {
            rc = ossReadN( &file, 1, &readChar, readSize ) ;
            while( SDB_OK == rc && readSize )
            {
               if( readChar == '\n' )
               {
                  break ;
               }
               buf << readChar ;
               rc = ossReadN( &file, 1, &readChar, readSize ) ;
            }
            line = buf.str() ;

            pos = line.find( ' ' ) ;
            if ( pos != std::string::npos &&
               line.substr( pos + 1, ossStrlen( UTIL_SYS_NUMA_INTERLEAVE ) ).find(
               UTIL_SYS_NUMA_INTERLEAVE ) == std::string::npos )
            {
               // interleave not found, count NUMA nodes by finding the highest numbered node file
               UINT32 i = 2 ;
               std::string fileName ;
               buf.clear() ;
               while( TRUE )
               {
                  buf << UTIL_SYS_MULTI_NODES_FILE_NAME << i ;
                  fileName = buf.str() ;

                  rc = ossAccess( fileName.c_str() ) ;
                  if ( SDB_OK != rc && SDB_FNE != rc )
                  {
                     PD_LOG( PDERROR, "Failed to access file[%s], rc: %d", fileName.c_str(), rc ) ;
                     goto error ;
                  }
                  else if ( SDB_OK == rc )
                  {
                     numaNodes = i ;
                     i++ ;
                  }
                  else
                  {
                     rc = SDB_OK ;
                     break ;
                  }
               }
            }
         }
         catch ( std::exception &e )
         {
            rc = ossException2RC( &e ) ;
            PD_LOG( PDERROR, "An exception occurred when counting numa nodes: "
                  "%s, rc: %d", e.what(), rc ) ;
            goto error ;
         }
      }

   done:
      if ( isOpened )
      {
         ossClose( file ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 utilSysGetOSVersion( std::string &version )
   {
      INT32 rc = SDB_OK ;
      OSSFILE file ;
      BOOLEAN isOpened = FALSE ;
      std::stringstream buf ;
      CHAR readChar = '\0' ;
      INT64 readSize = 0 ;

      rc = ossOpen( UTIL_SYS_PROC_VERSION_FILE_NAME, OSS_READONLY|OSS_SHAREREAD, 0, file ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open file[%s], rc: %d", UTIL_SYS_PROC_VERSION_FILE_NAME, rc ) ;
      isOpened = TRUE ;

      try
      {
         rc = ossReadN( &file, 1, &readChar, readSize ) ;
         while( SDB_OK == rc && readSize )
         {
            if( readChar == '\n' )
            {
               break ;
            }
            buf << readChar ;
            rc = ossReadN( &file, 1, &readChar, readSize ) ;
         }
         version = buf.str() ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "An exception occurred when getting os version: "
                  "%s, rc: %d", e.what(), rc ) ;
         goto error ;
      }

   done:
      if ( isOpened )
      {
         ossClose( file ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 utilSysGetOSVersionSignature( std::string &versionSignature )
   {
      INT32 rc = SDB_OK ;
      OSSFILE file ;
      BOOLEAN isOpened = FALSE ;
      std::stringstream buf ;
      CHAR readChar = '\0' ;
      INT64 readSize = 0 ;

      rc = ossOpen( UTIL_SYS_PROC_VERSION_SIGNA, OSS_READONLY|OSS_SHAREREAD, 0, file ) ;
      if ( SDB_FNE == rc )
      {
         rc = SDB_OK ;
         versionSignature = "" ;
         goto done ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to open file[%s], rc: %d", UTIL_SYS_PROC_VERSION_SIGNA, rc ) ;
      isOpened = TRUE ;

      try
      {
         rc = ossReadN( &file, 1, &readChar, readSize ) ;
         while( SDB_OK == rc && readSize )
         {
            if( readChar == '\n' )
            {
               break ;
            }
            buf << readChar ;
            rc = ossReadN( &file, 1, &readChar, readSize ) ;
         }
         versionSignature = buf.str() ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "An exception occurred when getting os version signature: "
                  "%s, rc: %d", e.what(), rc ) ;
         goto error ;
      }

   done:
      if ( isOpened )
      {
         ossClose( file ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 utilSysGetLibcVersion( std::string &version )
   {
      INT32 rc = SDB_OK ;

   #if defined(_LINUX)
      try
      {
         version = gnu_get_libc_version() ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "An exception occurred when getting libc version: "
                  "%s, rc: %d", e.what(), rc ) ;
         goto error ;
      }
   #endif

   done:
      return rc ;
   error:
      goto done ;
   }

   INT64 utilSysGetLimitMem()
   {
      INT64 limitMemBytes = 0 ;

   #if defined (_LINUX)
      rlimit rlim ;
      if ( 0 == getrlimit( RLIMIT_RSS, &rlim ) &&
         -1 != (INT64)rlim.rlim_cur )
      {
         limitMemBytes = (INT64)rlim.rlim_cur / 1024 / 1024 ;
      }
   #endif

      return limitMemBytes ;
   }

}