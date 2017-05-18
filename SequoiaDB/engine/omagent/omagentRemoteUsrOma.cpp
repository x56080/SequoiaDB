/*******************************************************************************


   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = omagentRemoteUsrOma.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/03/2016  WJM Initial Draft

   Last Changed =

*******************************************************************************/
#include "omagentRemoteUsrOma.hpp"
#include "omagentDef.hpp"
#include "omagentMgr.hpp"
#include "cmdUsrOmaUtil.hpp"
#include "utilParam.hpp"
#include "pmdDef.hpp"
#include "ossProc.hpp"
#include "../bson/bsonobjbuilder.h"
using namespace bson ;

namespace engine
{

   #define SDBCMTOP_TIMEOUT            ( 30000 )

   /*
      _remoteOmaGetOmaInstallFile implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteOmaGetOmaInstallFile )

   _remoteOmaGetOmaInstallFile::_remoteOmaGetOmaInstallFile()
   {
   }

   _remoteOmaGetOmaInstallFile::~_remoteOmaGetOmaInstallFile()
   {
   }

   const CHAR* _remoteOmaGetOmaInstallFile::name()
   {
      return OMA_REMOTE_OMA_GET_OMA_INSTALL_FILE ;
   }

   INT32 _remoteOmaGetOmaInstallFile::doit( BSONObj &retObj )
   {
      retObj = BSON( "installFile" << SDB_INSTALL_FILE_NAME ) ;
      return SDB_OK ;
   }

   /*
      _remoteOmaGetOmaInstallInfo implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteOmaGetOmaInstallInfo )

   _remoteOmaGetOmaInstallInfo::_remoteOmaGetOmaInstallInfo()
   {
   }

   _remoteOmaGetOmaInstallInfo::~_remoteOmaGetOmaInstallInfo()
   {
   }

   const CHAR* _remoteOmaGetOmaInstallInfo::name()
   {
      return OMA_REMOTE_OMA_GET_OMA_INSTALL_INFO ;
   }

   INT32 _remoteOmaGetOmaInstallInfo::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      utilInstallInfo info ;

      // get install info
      rc = utilGetInstallInfo( info ) ;
      if ( rc )
      {
         PD_LOG_MSG( PDERROR, "Install file is not exist" ) ;
         goto error ;
      }
      else
      {
         BSONObjBuilder builder ;
         builder.append( SDB_INSTALL_RUN_FILED, info._run ) ;
         builder.append( SDB_INSTALL_USER_FIELD, info._user ) ;
         builder.append( SDB_INSTALL_PATH_FIELD, info._path ) ;
         builder.append( SDB_INSTALL_MD5_FIELD, info._md5 ) ;
         retObj = builder.obj() ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteOmaGetOmaConfigFile implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteOmaGetOmaConfigFile )

   _remoteOmaGetOmaConfigFile::_remoteOmaGetOmaConfigFile()
   {
   }

   _remoteOmaGetOmaConfigFile::~_remoteOmaGetOmaConfigFile()
   {
   }

   const CHAR* _remoteOmaGetOmaConfigFile::name()
   {
      return OMA_REMOTE_OMA_GET_OMA_CONFIG_FILE ;
   }

   INT32 _remoteOmaGetOmaConfigFile::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string confFile ;

      rc = _getOmaConfFile( confFile ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "Failed to get config file" ) ;
         goto error ;
      }

      retObj = BSON( "confFile" << confFile.c_str() ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteOmaGetOmaConfigs implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteOmaGetOmaConfigs )

   _remoteOmaGetOmaConfigs::_remoteOmaGetOmaConfigs()
   {
   }

   _remoteOmaGetOmaConfigs::~_remoteOmaGetOmaConfigs()
   {
   }

   const CHAR* _remoteOmaGetOmaConfigs::name()
   {
      return OMA_REMOTE_OMA_GET_OMA_CONFIGS ;
   }

   INT32 _remoteOmaGetOmaConfigs::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string confFile ;
      BSONObj conf ;
      string errMsg ;

      // get confFile
      if ( FALSE == _matchObj.isEmpty() )
      {
         if ( FALSE == _matchObj.hasField( "confFile" ) ||
              String != _matchObj.getField( "confFile" ).type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "confFile must be string" ) ;
            goto error ;
         }
         confFile = _matchObj.getStringField( "confFile" ) ;
      }
      else
      {
         rc = _getOmaConfFile( confFile ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG( PDERROR, "Failed to get config file" ) ;
            goto error ;
         }
      }

      // get config info
      rc = _getOmaConfInfo( confFile, conf, errMsg ) ;
      if ( rc )
      {
         PD_LOG_MSG( PDERROR, errMsg.c_str() ) ;
         goto error ;
      }

      retObj = conf ;
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteOmaSetOmaConfigs implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteOmaSetOmaConfigs )

   _remoteOmaSetOmaConfigs::_remoteOmaSetOmaConfigs()
   {
   }

   _remoteOmaSetOmaConfigs::~_remoteOmaSetOmaConfigs()
   {
   }

   const CHAR* _remoteOmaSetOmaConfigs::name()
   {
      return OMA_REMOTE_OMA_SET_OMA_CONFIGS ;
   }

   INT32 _remoteOmaSetOmaConfigs::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string confFile ;
      BSONObj conf ;
      string str ;
      string errMsg ;

      // get configsObj
      if ( FALSE == _valueObj.hasField( "configsObj" ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "obj must be config" ) ;
         goto error ;
      }
      else if ( Object != _valueObj.getField( "configsObj" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "obj must be object" ) ;
         goto error ;
      }
      conf = _valueObj.getObjectField( "configsObj" );

      // get confFile
      if ( _matchObj.hasField( "confFile" ) )
      {
         if ( String != _matchObj.getField( "confFile" ).type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "confFile must be string" ) ;
            goto error ;
         }
         confFile = _matchObj.getStringField( "confFile" ) ;
      }
      else
      {
         rc = _getOmaConfFile( confFile ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG( PDERROR, "Failed to get config file" ) ;
            goto error ;
         }
      }

      rc = _confObj2Str( conf, str, errMsg ) ;
      if ( rc )
      {
         PD_LOG_MSG( PDERROR, errMsg.c_str() ) ;
         goto error ;
      }

      // write confFile
      rc = utilWriteConfigFile( confFile.c_str(), str.c_str(), FALSE ) ;
      if ( rc )
      {
         stringstream ss ;
         ss << "write conf file[" << confFile << "] failed" ;
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteOmaGetAOmaSvcName implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteOmaGetAOmaSvcName )

   _remoteOmaGetAOmaSvcName::_remoteOmaGetAOmaSvcName()
   {
   }

   _remoteOmaGetAOmaSvcName::~_remoteOmaGetAOmaSvcName()
   {
   }

   const CHAR* _remoteOmaGetAOmaSvcName::name()
   {
      return OMA_REMOTE_OMA_GET_A_OMA_SVC_NAME ;
   }

   INT32 _remoteOmaGetAOmaSvcName::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string hostname ;
      string confFile ;
      BSONObj confObj ;
      string errMsg ;

      // get hostname
      if ( FALSE == _matchObj.hasField( "hostname" ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "hostname must be config" ) ;
         goto error ;
      }
      else if ( String != _matchObj.getField( "hostname" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "hostname must be string" ) ;
         goto error ;
      }
      hostname = _matchObj.getStringField( "hostname" ) ;

      // get config File
      if ( _matchObj.hasField( "confFile" ) )
      {
         if ( String != _matchObj.getField( "confFile" ).type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "confFile must be string" ) ;
            goto error ;
         }
         confFile = _matchObj.getStringField( "confFile" ) ;
      }
      else
      {
         rc = _getOmaConfFile( confFile ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG( PDERROR, "Failed to get config file" ) ;
            goto error ;
         }
      }

      // get config obj and match
      rc = _getOmaConfInfo( confFile, confObj, errMsg ) ;
      if ( rc )
      {
         PD_LOG_MSG( PDERROR, errMsg.c_str() ) ;
         goto error ;
      }
      else
      {
         const CHAR *p = ossStrstr( hostname.c_str(), SDBCM_CONF_PORT ) ;
         if ( !p || ossStrlen( p ) != ossStrlen( SDBCM_CONF_PORT ) )
         {
            hostname += SDBCM_CONF_PORT ;
         }
         BSONElement e = confObj.getField( hostname ) ;
         if ( e.eoo() )
         {
            e = confObj.getField( SDBCM_CONF_DFTPORT ) ;
         }

         if ( e.type() == String )
         {
            retObj = BSON( "svcName" << e.valuestr() ) ;
         }
         else
         {
            stringstream ss ;
            ss << e.toString() << " is invalid" ;
            PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteOmaAddAOmaSvcName implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteOmaAddAOmaSvcName )

   _remoteOmaAddAOmaSvcName::_remoteOmaAddAOmaSvcName()
   {
   }

   _remoteOmaAddAOmaSvcName::~_remoteOmaAddAOmaSvcName()
   {
   }

   const CHAR* _remoteOmaAddAOmaSvcName::name()
   {
      return OMA_REMOTE_OMA_ADD_A_OMA_SVC_NAME ;
   }

   INT32 _remoteOmaAddAOmaSvcName::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      INT32 isReplace = TRUE ;
      string hostname ;
      string svcname ;
      string errMsg ;
      string confFile ;
      BSONObj confObj ;
      string str ;

      // get hostname
      if ( FALSE == _valueObj.hasField( "hostname" ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "hostname must be config" ) ;
         goto error ;
      }
      else if ( String != _valueObj.getField( "hostname" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "hostname must be string" ) ;
         goto error ;
      }
      hostname = _valueObj.getStringField( "hostname" ) ;
      if ( hostname.empty() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "hostname can't be empty" ) ;
         goto error ;
      }

      // get svcname
      if ( FALSE == _valueObj.hasField( "svcname" ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "svcname must be config" ) ;
         goto error ;
      }
      else if ( String != _valueObj.getField( "svcname" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "svcname must be string" ) ;
         goto error ;
      }
      svcname = _valueObj.getStringField( "svcname" ) ;
      if ( svcname.empty() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "svcname can't be empty" ) ;
         goto error ;
      }

      // get isReplace
      if ( _optionObj.hasField( "isReplace" ) )
      {
         if ( Bool != _optionObj.getField( "isReplace" ).type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "isReplace must be BOOLEAN" ) ;
            goto error ;
         }
         isReplace = _optionObj.getBoolField( "isReplace" ) ;
      }

      // get confFile
      if ( _matchObj.hasField( "confFile" ) )
      {
         if ( String != _matchObj.getField( "confFile" ).type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "confFile must be string" ) ;
            goto error ;
         }
         confFile = _matchObj.getStringField( "confFile" ) ;
      }
      else
      {
         rc = _getOmaConfFile( confFile ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG( PDERROR, "Failed to get config file" ) ;
            goto error ;
         }
      }

      rc = _getOmaConfInfo( confFile, confObj, errMsg, TRUE ) ;
      if ( rc )
      {
         PD_LOG_MSG( PDERROR, errMsg.c_str() ) ;
         goto error ;
      }
      else
      {
         const CHAR *p = ossStrstr( hostname.c_str(), SDBCM_CONF_PORT ) ;
         if ( !p || ossStrlen( p ) != ossStrlen( SDBCM_CONF_PORT ) )
         {
            hostname += SDBCM_CONF_PORT ;
         }
         BSONElement e = confObj.getField( hostname ) ;
         BSONElement e1 = confObj.getField( SDBCM_CONF_DFTPORT ) ;

         if ( e.type() == String )
         {
            if ( 0 == ossStrcmp( e.valuestr(), svcname.c_str() ) )
            {
               // same with hostname, not change
               goto done ;
            }
            else if ( !isReplace )
            {
               stringstream ss ;
               ss << hostname << " already exist" ;
               PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
         }
         else if ( e1.type() == String &&
                   0 == ossStrcmp( e1.valuestr(), svcname.c_str() ) )
         {
            // same with default, not change
            goto done ;
         }
      }

      rc = _confObj2Str( confObj, str, errMsg, hostname.c_str() ) ;
      if ( rc )
      {
         PD_LOG_MSG( PDERROR, errMsg.c_str() ) ;
         goto error ;
      }
      str += hostname ;
      str += "=" ;
      str += svcname ;
      str += OSS_NEWLINE ;

      // write conf file
      rc = utilWriteConfigFile( confFile.c_str(), str.c_str(), FALSE ) ;
      if ( rc )
      {
         stringstream ss ;
         ss << "write conf file[" << confFile << "] failed" ;
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteOmaDelAOmaSvcName implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteOmaDelAOmaSvcName )

   _remoteOmaDelAOmaSvcName::_remoteOmaDelAOmaSvcName()
   {
   }

   _remoteOmaDelAOmaSvcName::~_remoteOmaDelAOmaSvcName()
   {
   }

   const CHAR* _remoteOmaDelAOmaSvcName::name()
   {
      return OMA_REMOTE_OMA_DEL_A_OMA_SVC_NAME ;
   }

   INT32 _remoteOmaDelAOmaSvcName::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string hostname ;
      string confFile ;
      BSONObj confObj ;
      string str ;
      string errMsg ;

      // get hostname
      if ( FALSE == _matchObj.hasField( "hostname" ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "hostname must be config" ) ;
         goto error ;
      }
      else if ( String != _matchObj.getField( "hostname" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "hostname must be string" ) ;
         goto error ;
      }
      hostname = _matchObj.getStringField( "hostname" ) ;

      // get confFile
      if ( _matchObj.hasField( "confFile" ) )
      {
         if ( String != _matchObj.getField( "confFile" ).type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "confFile must be string" ) ;
            goto error ;
         }
         confFile = _matchObj.getStringField( "confFile" ) ;
      }
      else
      {
         rc = _getOmaConfFile( confFile ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG( PDERROR, "Failed to get config file" ) ;
            goto error ;
         }
      }

      rc = _getOmaConfInfo( confFile, confObj, errMsg, TRUE ) ;
      if ( rc )
      {
         PD_LOG_MSG( PDERROR, errMsg.c_str() ) ;
         goto error ;
      }
      else
      {
         const CHAR *p = ossStrstr( hostname.c_str(), SDBCM_CONF_PORT ) ;
         if ( !p || ossStrlen( p ) != ossStrlen( SDBCM_CONF_PORT ) )
         {
            hostname += SDBCM_CONF_PORT ;
         }
         BSONElement e = confObj.getField( hostname ) ;
         if ( e.eoo() )
         {
            // not exist, don't delete
            goto done ;
         }
      }

      // turn into str
      rc = _confObj2Str( confObj, str, errMsg, hostname.c_str() ) ;
      if ( rc )
      {
         PD_LOG_MSG( PDERROR, errMsg.c_str() ) ;
         goto error ;
      }

      // write confFile
      rc = utilWriteConfigFile( confFile.c_str(), str.c_str(), FALSE ) ;
      if ( rc )
      {
         stringstream ss ;
         ss << "write conf file[" << confFile << "] failed" ;
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteOmaListNodes implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteOmaListNodes )

   _remoteOmaListNodes::_remoteOmaListNodes()
   {
   }

   _remoteOmaListNodes::~_remoteOmaListNodes()
   {
   }

   const CHAR* _remoteOmaListNodes::name()
   {
      return OMA_REMOTE_OMA_LIST_NODES ;
   }

   INT32 _remoteOmaListNodes::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      _sdbToolListParam optionParam ;
      UTIL_VEC_NODES nodes ;
      CHAR rootPath[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      CHAR localPath[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      vector< BSONObj > vecObj ;
      string errMsg ;
      BSONObjBuilder builder ;

      rc = ossGetEWD( rootPath, OSS_MAX_PATHSIZE ) ;
      if ( rc )
      {
         PD_LOG_MSG( PDERROR, "get current path failed" ) ;
         goto error ;
      }

      rc = utilBuildFullPath( rootPath, SDBCM_LOCAL_PATH,
                              OSS_MAX_PATHSIZE, localPath ) ;
      if ( rc )
      {
         PD_LOG_MSG( PDERROR, "build local conf path failed" ) ;
         goto error ;
      }

      // get param
      rc = _parseListParam( _optionObj, optionParam, errMsg ) ;
      if ( rc )
      {
         PD_LOG_MSG( PDERROR, errMsg.c_str() ) ;
         goto error ;
      }

      // get nodes list
      utilListNodes( nodes, optionParam._typeFilter, NULL,
                     OSS_INVALID_PID, optionParam._roleFilter,
                     optionParam._showAlone ) ;

      if ( RUN_MODE_RUN == optionParam._modeFilter )
      {
         BOOLEAN bFind = FALSE ;
         UTIL_VEC_NODES::iterator it = nodes.begin() ;
         while( it != nodes.end() && optionParam._svcnames.size() > 0 )
         {
            bFind = FALSE ;
            utilNodeInfo &info = *it ;

            // match specified svcname
            for ( UINT32 j = 0 ; j < optionParam._svcnames.size() ; ++j )
            {
               if ( info._svcname == optionParam._svcnames[ j ] )
               {
                  bFind = TRUE ;
                  break ;
               }
            }
            if ( !bFind )
            {
               it = nodes.erase( it ) ;
               continue ;
            }
            ++it ;
         }
      }
      else
      {
         BOOLEAN bFind = FALSE ;
         UTIL_VEC_NODES tmpNodes = nodes ;
         nodes.clear() ;
         utilEnumNodes( localPath, nodes, optionParam._typeFilter, NULL,
                        optionParam._roleFilter ) ;
         if ( ( -1 == optionParam._typeFilter ||
                SDB_TYPE_OMA == optionParam._typeFilter ) &&
              ( -1 == optionParam._roleFilter ||
                SDB_ROLE_OMA == optionParam._roleFilter ) )
         {
            CHAR hostName[ OSS_MAX_HOSTNAME + 1 ] = { 0 } ;
            CHAR confFile[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
            utilNodeInfo node ;
            node._orgname = "" ;
            node._pid = OSS_INVALID_PID ;
            node._role = SDB_ROLE_OMA ;
            node._type = SDB_TYPE_OMA ;
            ossGetHostName( hostName, OSS_MAX_HOSTNAME ) ;

            utilBuildFullPath( rootPath, SDBCM_CONF_PATH_FILE,
                               OSS_MAX_PATHSIZE, confFile ) ;
            // file exist
            if ( 0 == ossAccess( confFile ) )
            {
               utilGetCMService( rootPath, hostName, node._svcname, TRUE ) ;
               nodes.push_back( node ) ;
            }
         }

         UTIL_VEC_NODES::iterator it = nodes.begin() ;
         while ( it != nodes.end() && optionParam._svcnames.size() > 0 )
         {
            utilNodeInfo &info = *it ;
            bFind = FALSE ;

            // match specified svcname
            for ( UINT32 j = 0 ; j < optionParam._svcnames.size() ; ++j )
            {
               if ( info._svcname == optionParam._svcnames[ j ] )
               {
                  bFind = TRUE ;
                  break ;
               }
            }
            if ( !bFind )
            {
               it = nodes.erase( it ) ;
               continue ;
            }
            ++it ;
         }

         for ( UINT32 i = 0 ; i < nodes.size() ; ++i )
         {
            for ( UINT32 k = 0 ; k < tmpNodes.size() ; ++k )
            {
               if ( nodes[ i ]._svcname == tmpNodes[ k ]._svcname )
               {
                  nodes[ i ] = tmpNodes[ k ] ;
                  break ;
               }
            }
         }
      }

      // build BSONObj vector
      for ( UINT32 k = 0 ; k < nodes.size() ; ++k )
      {
         BSONObj obj = _nodeInfo2Bson( nodes[ k ],
                                       optionParam._expand ?
                                       _getConfObj( rootPath, localPath,
                                                    nodes[ k ]._svcname.c_str(),
                                                    nodes[ k ]._type ) :
                                       BSONObj() ) ;
         vecObj.push_back( obj ) ;
      }

      // if no svcname, and list all/list cm, need to show sdbcmd
      if ( optionParam._svcnames.size() == 0 &&
           ( SDB_TYPE_OMA == optionParam._typeFilter ||
             -1 == optionParam._typeFilter ) &&
           ( optionParam._roleFilter == -1 ||
             SDB_ROLE_OMA == optionParam._roleFilter ) )
      {
         vector < ossProcInfo > procs ;
         ossEnumProcesses( procs, PMDDMN_EXE_NAME, TRUE, FALSE ) ;

         for ( UINT32 i = 0 ; i < procs.size() ; ++i )
         {
            BSONObj obj = BSON( "type" << PMDDMN_SVCNAME_DEFAULT <<
                                "pid" << (UINT32)procs[ i ]._pid ) ;
            vecObj.push_back( obj ) ;
         }
      }

      // build retObj
      for( UINT32 index = 0; index < vecObj.size(); index++ )
      {
         try
         {
            builder.append( boost::lexical_cast<string>( index ).c_str(),
                            vecObj[index] ) ;
         }
         catch( std::exception )
         {
            rc = SDB_SYS ;
            PD_LOG_MSG( PDERROR, "Fail to build retObj" ) ;
            goto error ;
         }
      }

      // set result
      retObj = builder.obj() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _remoteOmaListNodes::_parseListParam( const BSONObj &option,
                                               _sdbToolListParam &param,
                                               string &errMsg  )
   {
      INT32 rc = SDB_OK ;

      BSONObjIterator it ( option ) ;
      while ( it.more() )
      {
         BSONElement e = it.next() ;

         // type
         if ( 0 == ossStrcasecmp( e.fieldName(), PMD_OPTION_TYPE ) )
         {
            if ( String != e.type() )
            {
               errMsg = "type must be string(db/om/cm/all)" ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            if ( 0 == ossStrcasecmp( e.valuestr(), SDBLIST_TYPE_DB_STR ) )
            {
               param._typeFilter = SDB_TYPE_DB ;
            }
            else if ( 0 == ossStrcasecmp( e.valuestr(), SDBLIST_TYPE_OM_STR ) )
            {
               param._typeFilter = SDB_TYPE_OM ;
            }
            else if ( 0 == ossStrcasecmp( e.valuestr(), SDBLIST_TYPE_OMA_STR ) )
            {
               param._typeFilter = SDB_TYPE_OMA ;
            }
            else if ( 0 == ossStrcasecmp( e.valuestr(), SDBLIST_TYPE_ALL_STR ) )
            {
               param._typeFilter = -1 ;
            }
            else
            {
               errMsg = "type must be db/om/cm/all" ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
         }

         // mode
         else if ( 0 == ossStrcasecmp( e.fieldName(), PMD_OPTION_MODE ) )
         {
            if ( String != e.type() )
            {
               errMsg = "mode must be string[run/local]" ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            if ( 0 == ossStrcasecmp( e.valuestr(),
                                     SDB_RUN_MODE_TYPE_LOCAL_STR ) )
            {
               param._modeFilter = RUN_MODE_LOCAL ;
            }
            else if ( 0 == ossStrcasecmp( e.valuestr(),
                                          SDB_RUN_MODE_TYPE_RUN_STR ) )
            {
               param._modeFilter = RUN_MODE_RUN ;
            }
            else
            {
               errMsg = "mode must be run/local" ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
         }

         // role
         else if ( 0 == ossStrcasecmp( e.fieldName(), PMD_OPTION_ROLE ) )
         {
            if ( String != e.type() )
            {
               errMsg = "role must be string[data/coord/"
                        "catalog/standalone/om/cm]" ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            param._roleFilter = utilGetRoleEnum( e.valuestr() ) ;
            if ( SDB_ROLE_MAX == param._roleFilter )
            {
               errMsg = "role must be data/coord/catalog/"
                        "standalone/om/cm" ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
         }

         // svcname
         else if ( 0 == ossStrcasecmp( e.fieldName(), PMD_OPTION_SVCNAME ) )
         {
            if ( String != e.type() )
            {
               errMsg = "svcname must be string" ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            rc = utilSplitStr( e.valuestr(), param._svcnames, ", \t" ) ;
            if ( rc )
            {
               errMsg = "svcname must be string, use comma(,) to sperate" ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
         }

         // showalone
         else if ( 0 == ossStrcasecmp( e.fieldName(), "showalone" ) )
         {
            param._showAlone = e.booleanSafe() ? TRUE : FALSE ;
         }

         // expand
         else if ( 0 == ossStrcasecmp( e.fieldName(), PMD_OPTION_EXPAND ) )
         {
            param._expand = e.booleanSafe() ? TRUE : FALSE ;
         }
      }

      if ( param._roleFilter != -1 )
      {
         param._typeFilter = -1 ;
      }
      if ( param._svcnames.size() > 0 )
      {
         param._roleFilter = -1 ;
         param._typeFilter = -1 ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   BSONObj _remoteOmaListNodes::_nodeInfo2Bson( const utilNodeInfo &info,
                                                const BSONObj &conf )
   {
      BSONObjBuilder builder ;
      struct tm otm ;
      time_t tt = info._startTime ;
      CHAR tmpTime[ 21 ] = { 0 } ;
#if defined (_WINDOWS)
      localtime_s( &otm, &tt ) ;
#else
      localtime_r( &tt, &otm ) ;
#endif
      ossSnprintf( tmpTime, sizeof( tmpTime ) - 1,
                   "%04d-%02d-%02d-%02d.%02d.%02d",
                   otm.tm_year+1900,
                   otm.tm_mon+1,
                   otm.tm_mday,
                   otm.tm_hour,
                   otm.tm_min,
                   otm.tm_sec ) ;

      builder.append( "svcname", info._svcname ) ;
      builder.append( "type", utilDBTypeStr( (SDB_TYPE)info._type ) ) ;
      builder.append( "role", utilDBRoleStr( (SDB_ROLE)info._role ) ) ;
      builder.append( "pid", (UINT32)info._pid ) ;
      builder.append( "groupid", info._groupID ) ;
      builder.append( "nodeid", info._nodeID ) ;
      builder.append( "primary", info._primary ) ;
      builder.append( "isalone", info._isAlone ) ;
      builder.append( "groupname", info._groupName ) ;
      builder.append( "starttime", tmpTime ) ;
      if ( !info._dbPath.empty() )
      {
         builder.append( "dbpath", info._dbPath ) ;
      }
      BSONObj infoObj = builder.obj() ;

      BSONObjBuilder retObjBuilder ;
      retObjBuilder.appendElements( infoObj ) ;
      BSONObjIterator it( conf ) ;
      while ( it.more() )
      {
         BSONElement e = it.next() ;
         if ( EOO == infoObj.getField( e.fieldName() ).type() )
         {
            retObjBuilder.append( e ) ;
         }
      }
      return retObjBuilder.obj() ;
   }

   BSONObj _remoteOmaListNodes::_getConfObj( const CHAR *rootPath,
                                             const CHAR *localPath,
                                             const CHAR *svcname,
                                             INT32 type )
   {
      BSONObj obj ;
      CHAR confFile[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;

      // not cm
      if ( type != SDB_TYPE_OMA )
      {
         pmdOptionsCB conf ;
         utilBuildFullPath( localPath, svcname, OSS_MAX_PATHSIZE, confFile ) ;
         if ( 0 != ossAccess( confFile, 0 ) )
         {
            goto done ;
         }
         utilCatPath( confFile, OSS_MAX_PATHSIZE, PMD_DFT_CONF ) ;
         if ( SDB_OK == conf.initFromFile( confFile, FALSE ) )
         {
            conf.toBSON( obj ) ;
         }
      }
      else
      {
         po::options_description desc ;
         po::variables_map vm ;
         BSONObjBuilder builder ;
         desc.add_options()
            ( "*", po::value<string>(), "" ) ;

         utilBuildFullPath( rootPath, SDBCM_CONF_PATH_FILE,
                            OSS_MAX_PATHSIZE, confFile ) ;
         if ( 0 != ossAccess( confFile, 0 ) )
         {
            goto done ;
         }
         utilReadConfigureFile( confFile, desc, vm ) ;
         po::variables_map::iterator it = vm.begin() ;
         while( it != vm.end() )
         {
            builder.append( it->first.data(), it->second.as<string>() ) ;
            ++it ;
         }
         obj = builder.obj() ;
      }

   done:
      return obj ;
   }

   /*
      _remoteOmaGetNodeConfigs implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteOmaGetNodeConfigs )

   _remoteOmaGetNodeConfigs::_remoteOmaGetNodeConfigs()
   {
   }

   _remoteOmaGetNodeConfigs::~_remoteOmaGetNodeConfigs()
   {
   }

   const CHAR* _remoteOmaGetNodeConfigs::name()
   {
      return OMA_REMOTE_OMA_GET_NODE_CONFIGS ;
   }

   INT32 _remoteOmaGetNodeConfigs::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string svcname ;
      BSONType bsonType ;
      string confPath ;
      string errMsg ;
      BSONObj conf ;

      // get svcname
      if ( FALSE == _matchObj.hasField( "svcname" ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "svcname must be config" ) ;
         goto error ;
      }

      bsonType = _matchObj.getField( "svcname" ).type() ;
      if ( NumberInt == bsonType  )
      {
         stringstream ss ;
         INT32 port ;
         port = _matchObj.getIntField( "svcname" ) ;
         if ( 0 >= port || 65535 < port )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "svcname must in range ( 0, 65536 )" ) ;
            goto error ;
         }
         ss << port ;
         svcname = ss.str() ;
      }
      else if ( String == bsonType )
      {
         svcname = _matchObj.getStringField( "svcname" ) ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "svcname must be string or number" ) ;
         goto error ;
      }

      // build conf path
      rc = _getNodeConfigFile( svcname, confPath ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "Failed to get conf file path" ) ;
         goto error ;
      }
      // get confObj
      rc = _getNodeConfInfo( confPath, conf, errMsg ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, errMsg.c_str() ) ;
         goto error ;
      }

      retObj = conf ;
   done:
     return rc ;
   error:
      goto done ;
   }

   /*
      _remoteOmaSetNodeConfigs implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteOmaSetNodeConfigs )

   _remoteOmaSetNodeConfigs::_remoteOmaSetNodeConfigs()
   {
   }

   _remoteOmaSetNodeConfigs::~_remoteOmaSetNodeConfigs()
   {
   }

   const CHAR* _remoteOmaSetNodeConfigs::name()
   {
      return OMA_REMOTE_OMA_SET_NODE_CONFIGS ;
   }

   INT32 _remoteOmaSetNodeConfigs::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string confFile ;
      BSONObj conf ;
      string str ;
      string errMsg ;
      string svcname ;
      BSONType bsonType ;

      // get svcname
      if ( FALSE == _matchObj.hasField( "svcname" ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "svcname must be config" ) ;
         goto error ;
      }

      bsonType = _matchObj.getField( "svcname" ).type() ;
      if ( NumberInt == bsonType  )
      {
         stringstream ss ;
         INT32 port ;
         port = _matchObj.getIntField( "svcname" ) ;
         if ( 0 >= port || 65535 < port )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "svcname must in range ( 0, 65536 )" ) ;
            goto error ;
         }
         ss << port ;
         svcname = ss.str() ;
      }
      else if ( String == bsonType )
      {
         svcname = _matchObj.getStringField( "svcname" ) ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "svcname must be string or number" ) ;
         goto error ;
      }

      // get configsObj
      if ( FALSE == _valueObj.hasField( "configsObj" ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "obj must be config" ) ;
         goto error ;
      }
      else if ( Object != _valueObj.getField( "configsObj" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "obj must be object" ) ;
         goto error ;
      }
      conf = _valueObj.getObjectField( "configsObj" );

      rc = _confObj2Str( conf, str, errMsg ) ;
      if ( rc )
      {
         PD_LOG_MSG( PDERROR, errMsg.c_str() ) ;
         goto error ;
      }

      // build conf path
      rc = _getNodeConfigFile( svcname, confFile ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "Failed to get conf file path" ) ;
         goto error ;
      }

      // write confFile
      rc = utilWriteConfigFile( confFile.c_str(), str.c_str(), FALSE ) ;
      if ( rc )
      {
         stringstream ss ;
         ss << "write conf file[" << confFile << "] failed" ;
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteOmaUpdateNodeConfigs implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteOmaUpdateNodeConfigs )

   _remoteOmaUpdateNodeConfigs::_remoteOmaUpdateNodeConfigs()
   {
   }

   _remoteOmaUpdateNodeConfigs::~_remoteOmaUpdateNodeConfigs()
   {
   }

   const CHAR* _remoteOmaUpdateNodeConfigs::name()
   {
      return OMA_REMOTE_OMA_UPDATE_NODE_CONFIGS ;
   }

   INT32 _remoteOmaUpdateNodeConfigs::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string confFile ;
      BSONObj newConf ;
      BSONObj oldConf ;
      string str ;
      string errMsg ;
      string svcname ;
      BSONType bsonType ;
      BSONObjBuilder builder ;

      // get svcname
      if ( FALSE == _matchObj.hasField( "svcname" ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "svcname must be config" ) ;
         goto error ;
      }

      bsonType = _matchObj.getField( "svcname" ).type() ;
      if ( NumberInt == bsonType  )
      {
         stringstream ss ;
         INT32 port ;
         port = _matchObj.getIntField( "svcname" ) ;
         if ( 0 >= port || 65535 < port )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "svcname must in range ( 0, 65536 )" ) ;
            goto error ;
         }
         ss << port ;
         svcname = ss.str() ;
      }
      else if ( String == bsonType )
      {
         svcname = _matchObj.getStringField( "svcname" ) ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "svcname must be string or number" ) ;
         goto error ;
      }

      // get configsObj
      if ( FALSE == _valueObj.hasField( "configsObj" ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "obj must be config" ) ;
         goto error ;
      }
      else if ( Object != _valueObj.getField( "configsObj" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "obj must be object" ) ;
         goto error ;
      }
      newConf = _valueObj.getObjectField( "configsObj" );

      // build conf path
      rc = _getNodeConfigFile( svcname, confFile ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "Failed to get conf file path" ) ;
         goto error ;
      }

      // get exist configs
      rc = _getNodeConfInfo( confFile, oldConf, errMsg ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, errMsg.c_str() ) ;
         goto error ;
      }

      // merge configs, new conf should be append before append olf conf
      builder.appendElementsUnique( newConf ) ;
      builder.appendElementsUnique( oldConf ) ;

      rc = _confObj2Str( builder.obj(), str, errMsg ) ;
      if ( rc )
      {
         PD_LOG_MSG( PDERROR, errMsg.c_str() ) ;
         goto error ;
      }

      // write configs file
      rc = utilWriteConfigFile( confFile.c_str(), str.c_str(), FALSE ) ;
      if ( rc )
      {
         stringstream ss ;
         ss << "write conf file[" << confFile << "] failed" ;
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteOmaReloadConfigs implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteOmaReloadConfigs )

   _remoteOmaReloadConfigs::_remoteOmaReloadConfigs()
   {
   }

   _remoteOmaReloadConfigs::~_remoteOmaReloadConfigs()
   {
   }

   const CHAR* _remoteOmaReloadConfigs::name()
   {
      return OMA_REMOTE_OMA_RELOAD_CONFIGS ;
   }

   INT32 _remoteOmaReloadConfigs::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      BSONObj optionObj ;
      CHAR currentPath[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      omAgentOptions option( PMD_CFG_MASK_SHOWALL ) ;

      // Get current path
      ossGetEWD( currentPath, OSS_MAX_PATHSIZE ) ;
      option.init( currentPath ) ;
      option.toBSON( optionObj ) ;

      rc = sdbGetOMAgentOptions()->change( optionObj ) ;
      if ( rc )
      {
         PD_LOG_MSG( PDERROR, "Failed to change option" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteOmaTest implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteOmaTest )

   _remoteOmaTest::_remoteOmaTest()
   {
   }

   _remoteOmaTest::~_remoteOmaTest()
   {
   }

   const CHAR * _remoteOmaTest::name()
   {
      return OMA_REMOTE_OMA_TEST ;
   }

   INT32 _remoteOmaTest::doit( BSONObj &retObj )
   {
      return SDB_OK ;
   }
}
