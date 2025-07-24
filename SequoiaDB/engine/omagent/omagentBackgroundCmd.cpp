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

   Source File Name = omagentBackgroundCmd.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/06/2014  TZB Initial Draft

   Last Changed =

*******************************************************************************/
#include "omagentUtil.hpp"
#include "omagentBackgroundCmd.hpp"
#include "utilStr.hpp"
#include "omagentMgr.hpp"

using namespace bson ;

namespace engine
{

   /*
      _omaAddHost
   */
   _omaAddHost::_omaAddHost ( AddHostInfo &info )
   {
      _addHostInfo = info ;
   }

   _omaAddHost::~_omaAddHost ()
   {
   }

   INT32 _omaAddHost::init( const CHAR *pInstallInfo )
   {
      INT32 rc = SDB_OK ;
      try
      {
         BSONObj bus ;
         BSONObj sys ;
         stringstream ss ;
         rc = _getAddHostInfo( bus, sys ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to get add host info for js file, "
                     "rc = %d", rc ) ;
            goto error ;
         }

         // build js file arguments
         ss << "var " << JS_ARG_BUS << " = " 
            << bus.toString(FALSE, TRUE).c_str() << " ; "
            << "var " << JS_ARG_SYS << " = "
            << sys.toString(FALSE, TRUE).c_str() << " ; " ;
         _jsFileArgs = ss.str() ;
         PD_LOG ( PDDEBUG, "Add host passes argument: %s", _jsFileArgs.c_str() ) ;
         // add js file
         rc = addJsFile( FILE_ADD_HOST, _jsFileArgs.c_str() ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                     FILE_ADD_HOST, rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Failed to build bson, exception is: %s",
                  e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error :
      goto done ;
   }

   INT32 _omaAddHost::_getAddHostInfo( BSONObj &retObj1, BSONObj &retObj2 )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder builder ;
      BSONObjBuilder bob ;
      BSONObj subObj ;

      // the output bson format
      // {
      //  "SdbUser":"sdbadmin",
      //  "SdbPasswd":"sdbadmin",
      //  "SdbUserGroup":"sdbadmin_group",
      //  "InstallPacket":"/home/users/tanzhaobo/sequoiadb/bin/../packet/sequoiadb-1.8-linux_x86_64-installer.run",
      //  "HostInfo":{"IP":"192.168.20.42","HostName":"susetzb","User":"root","Passwd":"sequoiadb","SshPort":"22","AgentServic":"11790","InstallPath":"/opt/sequoiadb"}
      // }

      try
      {
         //build subObj
         bob.append( OMA_FIELD_IP, _addHostInfo._item._ip.c_str() ) ;
         bob.append( OMA_FIELD_HOSTNAME, _addHostInfo._item._hostName.c_str() ) ;
         bob.append( OMA_FIELD_USER, _addHostInfo._item._user.c_str() ) ;
         bob.append( OMA_FIELD_PASSWD, _addHostInfo._item._passwd.c_str() ) ;
         bob.append( OMA_FIELD_SSHPORT, _addHostInfo._item._sshPort.c_str() ) ;
         bob.append( OMA_FIELD_AGENTSERVICE, _addHostInfo._item._agentService.c_str() ) ;
         bob.append( OMA_FIELD_INSTALLPATH, _addHostInfo._item._installPath.c_str() ) ;
         subObj = bob.obj() ;

         // build retObj
         builder.append( OMA_FIELD_SDBUSER,
                         _addHostInfo._common._sdbUser.c_str() ) ;
         builder.append( OMA_FIELD_SDBPASSWD,
                         _addHostInfo._common._sdbPasswd.c_str() ) ;
         builder.append( OMA_FIELD_SDBUSERGROUP,
                         _addHostInfo._common._userGroup.c_str() ) ;
         builder.append( OMA_FIELD_INSTALLPACKET,
                         _addHostInfo._common._installPacket.c_str() ) ;
         builder.append( OMA_FIELD_HOSTINFO, subObj ) ;
         retObj1 = builder.obj() ;
         retObj2 = BSON( OMA_FIELD_TASKID << _addHostInfo._taskID ) ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG ( PDERROR, "Failed to build bson for add host, "
                      "exception is: %s", e.what() ) ;
         goto error ;
      }
      
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _omaCheckAddHostInfo
   */
   _omaCheckAddHostInfo::_omaCheckAddHostInfo()
   {
   }

   _omaCheckAddHostInfo::~_omaCheckAddHostInfo()
   {
   }

   INT32 _omaCheckAddHostInfo::init ( const CHAR *pInstallInfo )
   {
      INT32 rc = SDB_OK ;
      try
      {
         BSONObj bus( pInstallInfo ) ;
         stringstream ss ;

         // build js file arguments
         ss << "var " << JS_ARG_BUS << " = " 
            << bus.toString(FALSE, TRUE).c_str() << " ; " ;
         _jsFileArgs = ss.str() ;
         PD_LOG ( PDDEBUG, "Check add host information passes argument: %s",
                  _jsFileArgs.c_str() ) ;
         // add js file
         rc = addJsFile( FIEL_ADD_HOST_CHECK_INFO, _jsFileArgs.c_str() ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                     FIEL_ADD_HOST_CHECK_INFO, rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Failed to build bson, exception is: %s",
                  e.what() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
     goto done ;
   }

   /*
      _omaRemoveHost
   */
   _omaRemoveHost::_omaRemoveHost ( RemoveHostInfo &info )
   {
      _removeHostInfo = info ;
   }

   _omaRemoveHost::~_omaRemoveHost ()
   {
   }

   INT32 _omaRemoveHost::init( const CHAR *pInstallInfo )
   {
      INT32 rc = SDB_OK ;
      try
      {
         BSONObj bus ;
         BSONObj sys ;
         stringstream ss ;
         rc = _getRemoveHostInfo( bus, sys ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to get remove host info for js file, "
                     "rc = %d", rc ) ;
            goto error ;
         }

         // build js file arguments
         ss << "var " << JS_ARG_BUS << " = " 
            << bus.toString(FALSE, TRUE).c_str() << " ; "
            << "var " << JS_ARG_SYS << " = "
            << sys.toString(FALSE, TRUE).c_str() << " ; " ;
         _jsFileArgs = ss.str() ;
         PD_LOG ( PDDEBUG, "Remove host passes argument: %s",
                  _jsFileArgs.c_str() ) ;
         // add js file
         rc = addJsFile( FILE_REMOVE_HOST, _jsFileArgs.c_str() ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                     FILE_REMOVE_HOST, rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Failed to build bson, exception is: %s",
                  e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error :
      goto done ;
   }

   INT32 _omaRemoveHost::_getRemoveHostInfo( BSONObj &retObj1,
                                             BSONObj &retObj2 )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder bob ;

      try
      {
         //build subObj
         bob.append( OMA_FIELD_IP, _removeHostInfo._item._ip.c_str() ) ;
         bob.append( OMA_FIELD_HOSTNAME, _removeHostInfo._item._hostName.c_str() ) ;
         bob.append( OMA_FIELD_USER, _removeHostInfo._item._user.c_str() ) ;
         bob.append( OMA_FIELD_PASSWD, _removeHostInfo._item._passwd.c_str() ) ;
         bob.append( OMA_FIELD_SSHPORT, _removeHostInfo._item._sshPort.c_str() ) ;
         bob.append( OMA_FIELD_CLUSTERNAME, _removeHostInfo._item._clusterName.c_str() ) ;
         bob.append( OMA_FIELD_INSTALLPATH, _removeHostInfo._item._installPath.c_str() ) ;
         retObj1 = bob.obj() ;
         retObj2 = BSON( OMA_FIELD_TASKID << _removeHostInfo._taskID ) ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG ( PDERROR, "Failed to build bson for add host, "
                      "exception is: %s", e.what() ) ;
         goto error ;
      }
      
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _omaCreateTmpCoord
   */
   _omaCreateTmpCoord::_omaCreateTmpCoord( INT64 taskID )
   {
      _taskID = taskID ;
   }

   _omaCreateTmpCoord::~_omaCreateTmpCoord()
   {
   }

   INT32 _omaCreateTmpCoord::init ( const CHAR *pInstallInfo )
   {
      INT32 rc = SDB_OK ;
      try
      {
         stringstream ss ;
         BSONObj bus = BSONObj(pInstallInfo).copy() ;
         BSONObj sys = BSON( OMA_FIELD_TASKID << _taskID ) ;
         // build js file arguments
         ss << "var " << JS_ARG_BUS << " = " 
            << bus.toString(FALSE, TRUE).c_str() << " ; "
            << "var " << JS_ARG_SYS << " = "
            << sys.toString(FALSE, TRUE).c_str() << " ; " ;
         _jsFileArgs = ss.str() ;
         PD_LOG ( PDDEBUG, "Install temporary coord passes argument: %s",
                  _jsFileArgs.c_str() ) ;
         // add js file
         rc = addJsFile( FILE_INSTALL_TMP_COORD, _jsFileArgs.c_str() ) ;
         if ( rc )
         {
            PD_LOG_MSG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                         FILE_INSTALL_TMP_COORD, rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG ( PDERROR, "Failed to build bson, exception is: %s",
                      e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
     goto done ;
   }

   INT32 _omaCreateTmpCoord::createTmpCoord( BSONObj &cfgObj, BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      rc = init( cfgObj.objdata() ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to init to create "
                  "temporary coord, rc = %d", rc ) ;
         goto error ;
      }
      rc = doit( retObj ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to create temporary coord, rc = %d", rc ) ;
         goto error ;
      }     
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _omaRemoveTmpCoord
   */
   _omaRemoveTmpCoord::_omaRemoveTmpCoord( INT64 taskID,
                                           string &tmpCoordSvcName )
   {
      _taskID          = taskID ;
      _tmpCoordSvcName = tmpCoordSvcName ;
   }

   _omaRemoveTmpCoord::~_omaRemoveTmpCoord ()
   {
   }

   INT32 _omaRemoveTmpCoord::init ( const CHAR *pInstallInfo )
   {
      INT32 rc = SDB_OK ;
      try
      {
         stringstream ss ;
         BSONObj bus = BSON( OMA_FIELD_TMPCOORDSVCNAME << _tmpCoordSvcName ) ;
         BSONObj sys = BSON( OMA_FIELD_TASKID << _taskID ) ;

         // build js file arguments
         ss << "var " << JS_ARG_BUS << " = " 
            << bus.toString(FALSE, TRUE).c_str() << " ; "
            << "var " << JS_ARG_SYS << " = "
            << sys.toString(FALSE, TRUE).c_str() << " ; " ;
         _jsFileArgs = ss.str() ;
         PD_LOG ( PDDEBUG, "Remove temporary coord passes argument: %s",
                  _jsFileArgs.c_str() ) ;
         // add js file
         rc = addJsFile( FILE_REMOVE_TMP_COORD, _jsFileArgs.c_str() ) ;
         if ( rc )
         {
            PD_LOG_MSG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                         FILE_REMOVE_TMP_COORD, rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG ( PDERROR, "Failed to build bson, exception is: %s",
                      e.what() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
     goto done ;
   }

   INT32 _omaRemoveTmpCoord::removeTmpCoord( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      rc = init( NULL ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to init to remove temporary coord, "
                  "rc = %d", rc ) ;
         goto error ;
      }
      rc = doit( retObj ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to remove temporary coord, rc = %d", rc ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _omaInstallStandalone
   */
   _omaInstallStandalone::_omaInstallStandalone( INT64 taskID,
                                                 InstDBInfo &info )
   {
      _taskID              = taskID ;
      _info._hostName      = info._hostName;
      _info._svcName       = info._svcName ;
      _info._dbPath        = info._dbPath ;
      _info._confPath      = info._confPath ;
      _info._dataGroupName = info._dataGroupName ;
      _info._sdbUser       = info._sdbUser ;
      _info._sdbPasswd     = info._sdbPasswd ;
      _info._sdbUserGroup  = info._sdbUserGroup ;
      _info._user          = info._user ;
      _info._passwd        = info._passwd ;
      _info._sshPort       = info._sshPort ;
      _info._conf          = info._conf.copy() ;
   }

   _omaInstallStandalone::~_omaInstallStandalone()
   {
   }

   INT32 _omaInstallStandalone::init ( const CHAR *pInstallInfo )
   {
      INT32 rc = SDB_OK ;
      try
      {
         stringstream ss ;
         BSONObj bus = BSON (
                 OMA_FIELD_SDBUSER         << _info._sdbUser.c_str() <<
                 OMA_FIELD_SDBPASSWD       << _info._sdbPasswd.c_str() <<
                 OMA_FIELD_SDBUSERGROUP    << _info._sdbUserGroup.c_str() <<
                 OMA_FIELD_USER            << _info._user.c_str() <<
                 OMA_FIELD_PASSWD          << _info._passwd.c_str() <<
                 OMA_FIELD_SSHPORT         << _info._sshPort.c_str() <<
                 OMA_FIELD_INSTALLHOSTNAME << _info._hostName.c_str() <<
                 OMA_FIELD_INSTALLSVCNAME  << _info._svcName.c_str() <<
                 OMA_FIELD_INSTALLPATH2    << _info._dbPath.c_str() <<
                 OMA_FIELD_INSTALLCONFIG   << _info._conf ) ;
         BSONObj sys = BSON ( OMA_FIELD_TASKID << _taskID ) ;
         // build js file arguments
         ss << "var " << JS_ARG_BUS << " = " 
            << bus.toString(FALSE, TRUE).c_str() << " ; "
            << "var " << JS_ARG_SYS << " = "
            << sys.toString(FALSE, TRUE).c_str() << " ; " ;
         _jsFileArgs = ss.str() ;
         PD_LOG ( PDDEBUG, "Install standalone passes argument: %s",
                  _jsFileArgs.c_str() ) ;
         // add js file
         rc = addJsFile( FILE_INSTALL_STANDALONE, _jsFileArgs.c_str() ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                     FILE_INSTALL_STANDALONE, rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Failed to build bson, exception is: %s",
                  e.what() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
     goto done ;
   }

   /*
      _omaInstallCatalog
   */
   _omaInstallCatalog::_omaInstallCatalog( INT64 taskID,
                                           string &tmpCoordSvcName,
                                           InstDBInfo &info )
   {
      _taskID              = taskID ;
      _tmpCoordSvcName     = tmpCoordSvcName ;
      _info._hostName      = info._hostName;
      _info._svcName       = info._svcName ;
      _info._dbPath        = info._dbPath ;
      _info._confPath      = info._confPath ;
      _info._dataGroupName = info._dataGroupName ;
      _info._sdbUser       = info._sdbUser ;
      _info._sdbPasswd     = info._sdbPasswd ;
      _info._sdbUserGroup  = info._sdbUserGroup ;
      _info._user          = info._user ;
      _info._passwd        = info._passwd ;
      _info._sshPort       = info._sshPort ;
      _info._conf          = info._conf.copy() ;
   }

   _omaInstallCatalog::~_omaInstallCatalog()
   {
   }

   INT32 _omaInstallCatalog::init ( const CHAR *pInstallInfo )
   {
      INT32 rc = SDB_OK ;
      try
      {
         stringstream ss ;
         BSONObj bus = BSON (
                 OMA_FIELD_SDBUSER         << _info._sdbUser.c_str() <<
                 OMA_FIELD_SDBPASSWD       << _info._sdbPasswd.c_str() <<
                 OMA_FIELD_SDBUSERGROUP    << _info._sdbUserGroup.c_str() <<
                 OMA_FIELD_USER            << _info._user.c_str() <<
                 OMA_FIELD_PASSWD          << _info._passwd.c_str() <<
                 OMA_FIELD_SSHPORT         << _info._sshPort.c_str() <<
                 OMA_FIELD_INSTALLHOSTNAME << _info._hostName.c_str() <<
                 OMA_FIELD_INSTALLSVCNAME  << _info._svcName.c_str() <<
                 OMA_FIELD_INSTALLPATH2    << _info._dbPath.c_str() <<
                 OMA_FIELD_INSTALLCONFIG   << _info._conf ) ;
         BSONObj sys = BSON (
                 OMA_FIELD_TASKID << _taskID <<
                 OMA_FIELD_TMPCOORDSVCNAME << _tmpCoordSvcName.c_str() ) ;
         // build js file arguments
         ss << "var " << JS_ARG_BUS << " = " 
            << bus.toString(FALSE, TRUE).c_str() << " ; "
            << "var " << JS_ARG_SYS << " = "
            << sys.toString(FALSE, TRUE).c_str() << " ; " ;
         _jsFileArgs = ss.str() ;
         PD_LOG ( PDDEBUG, "Install catalog passes argument: %s",
                  _jsFileArgs.c_str() ) ;
         // add js file
         rc = addJsFile( FILE_INSTALL_CATALOG, _jsFileArgs.c_str() ) ;
         if ( rc )
         {
            PD_LOG_MSG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                         FILE_INSTALL_CATALOG, rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG ( PDERROR, "Failed to build bson, exception is: %s",
                  e.what() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
     goto done ;
   }

   /*
      _omaInstallCoord
   */
   _omaInstallCoord::_omaInstallCoord( INT64 taskID,
                                       string &tmpCoordSvcName,
                                       InstDBInfo &info )
   {
      _taskID              = taskID ;
      _tmpCoordSvcName     = tmpCoordSvcName ;
      _info._hostName      = info._hostName;
      _info._svcName       = info._svcName ;
      _info._dbPath        = info._dbPath ;
      _info._confPath      = info._confPath ;
      _info._dataGroupName = info._dataGroupName ;
      _info._sdbUser       = info._sdbUser ;
      _info._sdbPasswd     = info._sdbPasswd ;
      _info._sdbUserGroup  = info._sdbUserGroup ;
      _info._user          = info._user ;
      _info._passwd        = info._passwd ;
      _info._sshPort       = info._sshPort ;
      _info._conf          = info._conf.copy() ;
   }

   _omaInstallCoord::~_omaInstallCoord()
   {
   }

   INT32 _omaInstallCoord::init ( const CHAR *pInstallInfo )
   {
      INT32 rc = SDB_OK ;
      try
      {
         stringstream ss ;
         BSONObj bus = BSON (
                 OMA_FIELD_SDBUSER         << _info._sdbUser.c_str() <<
                 OMA_FIELD_SDBPASSWD       << _info._sdbPasswd.c_str() <<
                 OMA_FIELD_SDBUSERGROUP    << _info._sdbUserGroup.c_str() <<
                 OMA_FIELD_USER            << _info._user.c_str() <<
                 OMA_FIELD_PASSWD          << _info._passwd.c_str() <<
                 OMA_FIELD_SSHPORT         << _info._sshPort.c_str() <<
                 OMA_FIELD_INSTALLHOSTNAME << _info._hostName.c_str() <<
                 OMA_FIELD_INSTALLSVCNAME  << _info._svcName.c_str() <<
                 OMA_FIELD_INSTALLPATH2    << _info._dbPath.c_str() <<
                 OMA_FIELD_INSTALLCONFIG   << _info._conf ) ;
         BSONObj sys = BSON (
                 OMA_FIELD_TASKID << _taskID <<
                 OMA_FIELD_TMPCOORDSVCNAME << _tmpCoordSvcName.c_str() ) ;
         // build js file arguments
         ss << "var " << JS_ARG_BUS << " = " 
            << bus.toString(FALSE, TRUE).c_str() << " ; "
            << "var " << JS_ARG_SYS << " = "
            << sys.toString(FALSE, TRUE).c_str() << " ; " ;
         _jsFileArgs = ss.str() ;
         PD_LOG ( PDDEBUG, "Install coord passes argument: %s",
                  _jsFileArgs.c_str() ) ;
         // add js file
         rc = addJsFile( FILE_INSTALL_COORD, _jsFileArgs.c_str() ) ;
         if ( rc )
         {
            PD_LOG_MSG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                         FILE_INSTALL_COORD, rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG ( PDERROR, "Failed to build bson, exception is: %s",
                      e.what() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
     goto done ;
   }

   /*
      _omaInstallDataNode
   */
   _omaInstallDataNode::_omaInstallDataNode( INT64 taskID,
                                             string tmpCoordSvcName,
                                             InstDBInfo &info )
   {
      _taskID              = taskID ;
      _tmpCoordSvcName     = tmpCoordSvcName ;
      _info._hostName      = info._hostName;
      _info._svcName       = info._svcName ;
      _info._dbPath        = info._dbPath ;
      _info._confPath      = info._confPath ;
      _info._dataGroupName = info._dataGroupName ;
      _info._sdbUser       = info._sdbUser ;
      _info._sdbPasswd     = info._sdbPasswd ;
      _info._sdbUserGroup  = info._sdbUserGroup ;
      _info._user          = info._user ;
      _info._passwd        = info._passwd ;
      _info._sshPort       = info._sshPort ;
      _info._conf          = info._conf.copy() ;
   }

   _omaInstallDataNode::~_omaInstallDataNode()
   {
   }

   INT32 _omaInstallDataNode::init ( const CHAR *pInstallInfo )
   {
      INT32 rc = SDB_OK ;
      try
      {
         stringstream ss ;
         BSONObj bus = BSON (
                 OMA_FIELD_SDBUSER          << _info._sdbUser.c_str() <<
                 OMA_FIELD_SDBPASSWD        << _info._sdbPasswd.c_str() << 
                 OMA_FIELD_SDBUSERGROUP     << _info._sdbUserGroup.c_str() <<
                 OMA_FIELD_USER             << _info._user.c_str() <<
                 OMA_FIELD_PASSWD           << _info._passwd.c_str() <<
                 OMA_FIELD_SSHPORT          << _info._sshPort.c_str() <<
                 OMA_FIELD_INSTALLGROUPNAME << _info._dataGroupName.c_str() <<
                 OMA_FIELD_INSTALLHOSTNAME  << _info._hostName.c_str() <<
                 OMA_FIELD_INSTALLSVCNAME   << _info._svcName.c_str() <<
                 OMA_FIELD_INSTALLPATH2     << _info._dbPath.c_str() <<
                 OMA_FIELD_INSTALLCONFIG    << _info._conf ) ;
         BSONObj sys = BSON (
                 OMA_FIELD_TASKID << _taskID <<
                 OMA_FIELD_TMPCOORDSVCNAME << _tmpCoordSvcName.c_str() ) ;
         // build js file arguments
         ss << "var " << JS_ARG_BUS << " = " 
            << bus.toString(FALSE, TRUE).c_str() << " ; "
            << "var " << JS_ARG_SYS << " = "
            << sys.toString(FALSE, TRUE).c_str() << " ; " ;
         _jsFileArgs = ss.str() ;
         PD_LOG ( PDDEBUG, "Install data node passes "
                  "argument: %s", _jsFileArgs.c_str() ) ;
         rc = addJsFile( FILE_INSTALL_DATANODE, _jsFileArgs.c_str() ) ;
         if ( rc )
         {
            PD_LOG_MSG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                         FILE_INSTALL_DATANODE, rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG ( PDERROR, "Failed to build bson, exception is: %s",
                      e.what() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
     goto done ;
   }

   /*
      rollback standalone
   */
   _omaRollbackStandalone::_omaRollbackStandalone ( BSONObj &bus, BSONObj &sys )
   {
      _bus    = bus.copy() ;
      _sys    = sys.copy() ;
   }

   _omaRollbackStandalone::~_omaRollbackStandalone ()
   {
   }
   
   INT32 _omaRollbackStandalone::init ( const CHAR *pInstallInfo )
   {
      INT32 rc = SDB_OK ;
      stringstream ss ;
      
      // build js file arguments
      ss << "var " << JS_ARG_BUS << " = " 
         << _bus.toString(FALSE, TRUE).c_str() << " ; "
         << "var " << JS_ARG_SYS << " = "
         << _sys.toString(FALSE, TRUE).c_str() << " ; " ;
      _jsFileArgs = ss.str() ;
      PD_LOG ( PDDEBUG, "Rollback standalone passes "
               "argument: %s", _jsFileArgs.c_str() ) ;
      // add js file
      rc = addJsFile( FILE_ROLLBACK_STANDALONE, _jsFileArgs.c_str() ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                  FILE_ROLLBACK_STANDALONE, rc ) ;
         goto error ;
      }
      
   done:
      return rc ;
   error:
     goto done ;
   }

   /*
      rollback catalog
   */
   _omaRollbackCatalog::_omaRollbackCatalog (
                                   INT64 taskID,
                                   string &tmpCoordSvcName )
   {
      _taskID          = taskID ;
      _tmpCoordSvcName = tmpCoordSvcName ;
   }

   _omaRollbackCatalog::~_omaRollbackCatalog ()
   {
   }
   
   INT32 _omaRollbackCatalog::init ( const CHAR *pInstallInfo )
   {
      INT32 rc = SDB_OK ;
      try
      {
         stringstream ss ;
         BSONObj sys = BSON (
                 OMA_FIELD_TASKID << _taskID <<
                 OMA_FIELD_TMPCOORDSVCNAME << _tmpCoordSvcName.c_str() ) ;

         // build js file arguments
         ss << "var " << JS_ARG_SYS << " = "
            << sys.toString(FALSE, TRUE).c_str() << " ; " ;
         _jsFileArgs = ss.str() ;
         PD_LOG ( PDDEBUG, "Rollback catalog passes "
                  "argument: %s", _jsFileArgs.c_str() ) ;
         rc = addJsFile( FILE_ROLLBACK_CATALOG, _jsFileArgs.c_str() ) ;
         if ( rc )
         {
            PD_LOG_MSG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                         FILE_ROLLBACK_CATALOG, rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG ( PDERROR, "Failed to build bson, exception is: %s",
                      e.what() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
     goto done ;
   }
   
   /*
      rollback coord
   */

   _omaRollbackCoord::_omaRollbackCoord ( INT64 taskID,
                                          string &tmpCoordSvcName )
   {
      _taskID          = taskID ;
      _tmpCoordSvcName = tmpCoordSvcName ;
   }

   _omaRollbackCoord::~_omaRollbackCoord ()
   {
   }
   
   INT32 _omaRollbackCoord::init ( const CHAR *pInstallInfo )
   {
      INT32 rc = SDB_OK ;
      try
      {
         stringstream ss ;
         BSONObj sys = BSON (
                 OMA_FIELD_TASKID << _taskID <<
                 OMA_FIELD_TMPCOORDSVCNAME << _tmpCoordSvcName.c_str() ) ;
         // build js file arguments
         ss << "var " << JS_ARG_SYS << " = "
            << sys.toString(FALSE, TRUE).c_str() << " ; " ;
         _jsFileArgs = ss.str() ;
         PD_LOG ( PDDEBUG, "Rollback coord passes "
                  "argument: %s", _jsFileArgs.c_str() ) ;
         // add js file
         rc = addJsFile( FILE_ROLLBACK_COORD, _jsFileArgs.c_str() ) ;
         if ( rc )
         {
            PD_LOG_MSG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                         FILE_ROLLBACK_COORD, rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG ( PDERROR, "Failed to build bson, exception is: %s",
                      e.what() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
     goto done ;
   }


   /*
      rollback data groups
   */

   _omaRollbackDataRG::_omaRollbackDataRG ( INT64 taskID,
                                            string &tmpCoordSvcName,
                                            set<string> &info )
   : _info( info )
   {
      _taskID          = taskID ;
      _tmpCoordSvcName = tmpCoordSvcName ;
   }

   _omaRollbackDataRG::~_omaRollbackDataRG ()
   {
   }
   
   INT32 _omaRollbackDataRG::init ( const CHAR *pInstallInfo )
   {
      INT32 rc = SDB_OK ;
      try
      {
         stringstream ss ;
         BSONObj bus ;
         BSONObj sys ;
         // get installed data nodes info
         _getInstalledDataGroupInfo( bus ) ;
         sys = BSON( OMA_FIELD_TASKID << _taskID <<
                     OMA_FIELD_TMPCOORDSVCNAME << _tmpCoordSvcName.c_str() ) ;
         // build js file arguments
         ss << "var " << JS_ARG_BUS << " = " 
            << bus.toString(FALSE, TRUE).c_str() << " ; "
            << "var " << JS_ARG_SYS << " = "
            << sys.toString(FALSE, TRUE).c_str() << " ; " ;
         _jsFileArgs = ss.str() ;
         PD_LOG ( PDDEBUG, "Rollback data groups passes "
                  "argument: %s", _jsFileArgs.c_str() ) ;
         rc = addJsFile( FILE_ROLLBACK_DATA_RG, _jsFileArgs.c_str() ) ;
         if ( rc )
         {
            PD_LOG_MSG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                         FILE_ROLLBACK_DATA_RG, rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG ( PDERROR, "Failed to build bson, exception is: %s",
                      e.what() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
     goto done ;
   }

   void _omaRollbackDataRG::_getInstalledDataGroupInfo( BSONObj &obj )
   {
      BSONObjBuilder bob ;
      BSONArrayBuilder bab ;
      set<string>::iterator it = _info.begin() ;

      for( ; it != _info.end(); it++ )
      {
         string groupname = *it ;
         bab.append( groupname.c_str() ) ;
      }
      bob.appendArray( OMA_FIELD_UNINSTALLGROUPNAMES, bab.arr() ) ;
      obj = bob.obj() ;
   }

   /*
      remove standalone
   */
   _omaRmStandalone::_omaRmStandalone( BSONObj &bus, BSONObj &sys )
   {
      _bus = bus.copy() ;
      _sys = sys.copy() ;
   }

   _omaRmStandalone::~_omaRmStandalone()
   {
   }

   INT32 _omaRmStandalone::init ( const CHAR *pInfo )
   {
      INT32 rc = SDB_OK ;
      stringstream ss ;
      
      // build js file arguments
         ss << "var " << JS_ARG_BUS << " = " 
            << _bus.toString(FALSE, TRUE).c_str() << " ; "
            << "var " << JS_ARG_SYS << " = "
            << _sys.toString(FALSE, TRUE).c_str() << " ; " ;
         _jsFileArgs = ss.str() ;
      PD_LOG ( PDDEBUG, "Remove standalone passes argument: %s",
               _jsFileArgs.c_str() ) ;
      // add js file
      rc = addJsFile( FILE_REMOVE_STANDALONE, _jsFileArgs.c_str() ) ;
      if ( rc )
      {
         PD_LOG_MSG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                      FILE_REMOVE_STANDALONE, rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
     goto done ;
   }

   /*
      remove catalog group
   */
   _omaRmCataRG::_omaRmCataRG ( INT64 taskID, string &tmpCoordSvcName,
                                BSONObj &info )
   {
      _taskID = taskID ;
      _tmpCoordSvcName = tmpCoordSvcName ;
      _info = info.copy() ;
   }

   _omaRmCataRG::~_omaRmCataRG ()
   {
   }
   
   INT32 _omaRmCataRG::init ( const CHAR *pInfo )
   {
      INT32 rc = SDB_OK ;
      stringstream ss ;

      BSONObj bus = _info.copy() ;
      BSONObj sys = BSON( OMA_FIELD_TASKID << _taskID <<
                          OMA_FIELD_TMPCOORDSVCNAME << _tmpCoordSvcName.c_str() ) ;
      // build js file arguments
      ss << "var " << JS_ARG_BUS << " = " 
         << bus.toString(FALSE, TRUE).c_str() << " ; "
         << "var " << JS_ARG_SYS << " = "
         << sys.toString(FALSE, TRUE).c_str() << " ; " ;
      _jsFileArgs = ss.str() ;
      PD_LOG ( PDDEBUG, "Remove catalog group passes "
               "argument: %s", _jsFileArgs.c_str() ) ;
      // add js file
      rc = addJsFile( FILE_REMOVE_CATALOG_RG, _jsFileArgs.c_str() ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                  FILE_REMOVE_CATALOG_RG, rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
     goto done ;
   }

   /*
      remove coord group
   */

   _omaRmCoordRG::_omaRmCoordRG ( INT64 taskID, string &tmpCoordSvcName,
                                  BSONObj &info )
   {
      _taskID          = taskID ;
      _tmpCoordSvcName = tmpCoordSvcName ;
      _info            = info.copy() ;
   }

   _omaRmCoordRG::~_omaRmCoordRG ()
   {
   }
   
   INT32 _omaRmCoordRG::init ( const CHAR *pInfo )
   {
      INT32 rc = SDB_OK ;
      stringstream ss ;
      BSONObj bus = _info.copy() ;
      BSONObj sys = BSON( OMA_FIELD_TASKID << _taskID <<
                          OMA_FIELD_TMPCOORDSVCNAME << _tmpCoordSvcName.c_str() ) ;

      // build js file arguments
      ss << "var " << JS_ARG_BUS << " = " 
         << bus.toString(FALSE, TRUE).c_str() << " ; "
         << "var " << JS_ARG_SYS << " = "
         << sys.toString(FALSE, TRUE).c_str() << " ; " ;
      _jsFileArgs = ss.str() ;
      PD_LOG ( PDDEBUG, "Remove coord group passes "
               "argument: %s", _jsFileArgs.c_str() ) ;
      // add js file
      rc = addJsFile( FILE_REMOVE_COORD_RG, _jsFileArgs.c_str() ) ;
      if ( rc )
      {
         PD_LOG_MSG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                      FILE_REMOVE_COORD_RG, rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
     goto done ;
   }

   /*
      remove data rg
   */
   _omaRmDataRG::_omaRmDataRG ( INT64 taskID, string &tmpCoordSvcName,
                                BSONObj &info )
   {
      _taskID = taskID ;
      _tmpCoordSvcName = tmpCoordSvcName ;
      _info = info.copy() ;
   }

   _omaRmDataRG::~_omaRmDataRG ()
   {
   }
   
   INT32 _omaRmDataRG::init ( const CHAR *pInfo )
   {
      INT32 rc = SDB_OK ;
      stringstream ss ;
      BSONObj bus = _info.copy() ;
      BSONObj sys = BSON( OMA_FIELD_TASKID << _taskID <<
                          OMA_FIELD_TMPCOORDSVCNAME << _tmpCoordSvcName.c_str() ) ;
      // build js file arguments
      ss << "var " << JS_ARG_BUS << " = " 
         << bus.toString(FALSE, TRUE).c_str() << " ; "
         << "var " << JS_ARG_SYS << " = "
         << sys.toString(FALSE, TRUE).c_str() << " ; " ;
      _jsFileArgs = ss.str() ;
      PD_LOG ( PDDEBUG, "Remove data group passes "
               "argument: %s", _jsFileArgs.c_str() ) ;
      // add js file
      rc = addJsFile( FILE_REMOVE_DATA_RG, _jsFileArgs.c_str() ) ;
      if ( rc )
      {
         PD_LOG_MSG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                      FILE_REMOVE_DATA_RG, rc ) ;
         goto error ;
      }
         
   done:
      return rc ;
   error:
     goto done ;
   }

   /*
      init for executing js
   */
   _omaInitEnv::_omaInitEnv ( INT64 taskID, BSONObj &info )
   {
      _taskID = taskID ;
      _info = info.copy() ;
   }

   _omaInitEnv::~_omaInitEnv ()
   {
   }
   
   INT32 _omaInitEnv::init ( const CHAR *pInfo )
   {
      INT32 rc = SDB_OK ;
      stringstream ss ;
      BSONObj bus = _info.copy() ;
      BSONObj sys = BSON( OMA_FIELD_TASKID << _taskID ) ;
      // build js file arguments
      ss << "var " << JS_ARG_BUS << " = " 
         << bus.toString(FALSE, TRUE).c_str() << " ; "
         << "var " << JS_ARG_SYS << " = "
         << sys.toString(FALSE, TRUE).c_str() << " ; " ;
      _jsFileArgs = ss.str() ;
      PD_LOG ( PDDEBUG, "Init for executing js passes "
               "argument: %s", _jsFileArgs.c_str() ) ;
      // add js file
      rc = addJsFile( FILE_INIT_ENV, _jsFileArgs.c_str() ) ;
      if ( rc )
      {
         PD_LOG_MSG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                      FILE_INIT_ENV, rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
     goto done ;
   }

   /*
      _omaAddZNode
   */
   _omaAddZNode::_omaAddZNode ( AddZNInfo &info )
   {
      _addZNInfo = info ;
   }

   _omaAddZNode::~_omaAddZNode ()
   {
   }

   INT32 _omaAddZNode::init( const CHAR *pInstallInfo )
   {
      INT32 rc = SDB_OK ;
      try
      {
         BSONObj bus ;
         BSONObj sys ;
         stringstream ss ;
         rc = _getAddZNInfo( bus, sys ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to get installing znode's info "
                     "for js file, rc = %d", rc ) ;
            goto error ;
         }

         // build js file arguments
         ss << "var " << JS_ARG_BUS << " = " 
            << bus.toString(FALSE, TRUE).c_str() << " ; "
            << "var " << JS_ARG_SYS << " = "
            << sys.toString(FALSE, TRUE).c_str() << " ; " ;
         _jsFileArgs = ss.str() ;
         PD_LOG ( PDDEBUG, "Installing znode passes argument: %s",
                  _jsFileArgs.c_str() ) ;
         // add js file
         rc = addJsFile( FILE_INSTALL_ZOOKEEPER, _jsFileArgs.c_str() ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                     FILE_INSTALL_ZOOKEEPER, rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Failed to build bson, exception is: %s",
                  e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error :
      goto done ;
   }

   INT32 _omaAddZNode::_getAddZNInfo( BSONObj &retObj1, BSONObj &retObj2 )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder bob ;
      BSONArrayBuilder bab ;
      vector<string>::iterator it = _addZNInfo._common._serverInfo.begin() ;

      // the output bson format for standalone is:
      // { 
      //  "DeployMod":"standalone",
      //  "PacketPath":"/opt/sequoiadb/packet/zookeeper-3.4.6.tar.gz",
      //  "HostName":"rhel64-test8",
      //  "User": "root", "Passwd": "sequoiadb",
      //  "SdbUser": "sdbadmin", "SdbPasswd": "sdbadmin", "SdbUserGroup": "sdbadmin_group",
      //  "SshPort": "22",
      //  "zooid": 1,
      //  "installpath":"/opt/zookeeper",
      //  "datapath":"/opt/zookeeper/data",
      //  "clientport":"2181",
      //  "ticktime":"2000"
      // } 

      // the output bson format for cluster is:
      // { 
      //  "DeployMod":"distribution",
      //  "PacketPath":"/opt/sequoiadb/packet/zookeeper-3.4.6.tar.gz",
      //  "HostName":"susetzb",
      //  "User": "root", "Passwd": "sequoiadb",
      //  "SdbUser": "sdbadmin", "SdbPasswd": "sdbadmin", "SdbUserGroup": "sdbadmin_group",
      //  "SshPort": "22",
      //  "zooid": 1,
      //  "installpath":"/opt/zookeeper",
      //  "datapath":"/opt/zookeeper/data",
      //  "dataport":"2888",
      //  "electport":"3888",
      //  "clientport":"2181",
      //  "synclimit":"5",
      //  "initlimit":"10",
      //  "ticktime":"2000",
      //  "ServerInfo":["server.1=susetzb:2888:3888", "server.2=rhel64-test8:2888:3888", "server.3=rhel64-test9:2888:3888"]
      // }


      try
      {
         bob.append( OMA_FIELD_DEPLOYMOD, _addZNInfo._common._deployMod.c_str() ) ;
         bob.append( OMA_FIELD_PACKET_PATH, _addZNInfo._common._installPacket.c_str() ) ;
         bob.append( OMA_FIELD_HOSTNAME, _addZNInfo._item._hostName.c_str() ) ;
         bob.append( OMA_FIELD_USER, _addZNInfo._item._user.c_str() ) ;
         bob.append( OMA_FIELD_PASSWD, _addZNInfo._item._passwd.c_str() ) ;
         bob.append( OMA_FIELD_SDBUSER, _addZNInfo._common._sdbUser.c_str() ) ;
         bob.append( OMA_FIELD_SDBPASSWD, _addZNInfo._common._sdbPasswd.c_str() ) ;
         bob.append( OMA_FIELD_SDBUSERGROUP, _addZNInfo._common._userGroup.c_str() ) ;
         bob.append( OMA_FIELD_SSHPORT, _addZNInfo._item._sshPort.c_str() ) ;
         bob.append( OMA_FIELD_ZOOID3, _addZNInfo._item._zooid.c_str() ) ;
         bob.append( OMA_FIELD_INSTALLPATH3, _addZNInfo._item._installPath.c_str() ) ;
         bob.append( OMA_FIELD_DATAPATH3, _addZNInfo._item._dataPath.c_str() ) ;
         bob.append( OMA_FIELD_DATAPORT3, _addZNInfo._item._dataPort.c_str() ) ;
         bob.append( OMA_FIELD_ELECTPORT3, _addZNInfo._item._electPort.c_str() ) ;
         bob.append( OMA_FIELD_CLIENTPORT3, _addZNInfo._item._clientPort.c_str() ) ;
         bob.append( OMA_FIELD_SYNCLIMIT3, _addZNInfo._item._syncLimit.c_str() ) ;
         bob.append( OMA_FIELD_INITLIMIT3, _addZNInfo._item._initLimit.c_str() ) ;
         bob.append( OMA_FIELD_TICKTIME3, _addZNInfo._item._tickTime.c_str() ) ;
         bob.append( OMA_FIELD_CLUSTERNAME3, _addZNInfo._common._clusterName.c_str() ) ;
         bob.append( OMA_FIELD_BUSINESSNAME3, _addZNInfo._common._businessName.c_str() ) ;
         for ( ; it != _addZNInfo._common._serverInfo.end(); it++ )
         {
            bab.append( *it ) ;
         }
         bob.appendArray( OMA_FIELD_SERVERINFO, bab.arr() ) ;

         // build retObj
         retObj1 = bob.obj() ;
         retObj2 = BSON( OMA_FIELD_TASKID << _addZNInfo._taskID ) ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG ( PDERROR, "Failed to build bson for add host, "
                      "exception is: %s", e.what() ) ;
         goto error ;
      }
      
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _omaRemoveZNode
   */
   _omaRemoveZNode::_omaRemoveZNode ( RemoveZNInfo &info )
   {
      _removeZNInfo = info ;
   }

   _omaRemoveZNode::~_omaRemoveZNode ()
   {
   }

   INT32 _omaRemoveZNode::init( const CHAR *pInstallInfo )
   {
      INT32 rc = SDB_OK ;
      try
      {
         BSONObj bus ;
         BSONObj sys ;
         stringstream ss ;
         rc = _getRemoveZNInfo( bus, sys ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to get removing znode's info "
                     "for js file, rc = %d", rc ) ;
            goto error ;
         }

         // build js file arguments
         ss << "var " << JS_ARG_BUS << " = " 
            << bus.toString(FALSE, TRUE).c_str() << " ; "
            << "var " << JS_ARG_SYS << " = "
            << sys.toString(FALSE, TRUE).c_str() << " ; " ;
         _jsFileArgs = ss.str() ;
         PD_LOG ( PDDEBUG, "Removing znode passes argument: %s",
                  _jsFileArgs.c_str() ) ;
         // add js file
         rc = addJsFile( FILE_REMOVE_ZOOKEEPER, _jsFileArgs.c_str() ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                     FILE_REMOVE_ZOOKEEPER, rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Failed to build bson, exception is: %s",
                  e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error :
      goto done ;
   }

   INT32 _omaRemoveZNode::_getRemoveZNInfo( BSONObj &retObj1, BSONObj &retObj2 )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder bob ;
      BSONArrayBuilder bab ;
      vector<string>::iterator it ;

      // the output bson format for standalone is:
      // { 
      //  "DeployMod":"standalone",
      //  "HostName":"rhel64-test8",
      //  "User": "root",
      //  "Passwd": "sequoiadb",
      //  "SdbUser": "sdbadmin", "SdbPasswd": "sdbadmin", "SdbUserGroup": "sdbadmin_group",
      //  "SshPort": "22",
      //  "zooid": 1,
      //  "installPath":"/opt/zookeeper",
      //  "datapath":"/opt/zookeeper/data",
      //  "clientport":"2181",
      //  "ticktime":"2000"
      // }

      // the output bson format for cluster is:
      // {
      //  "DeployMod":"distribution",
      //  "HostName":"susetzb",
      //  "User": "root", "Passwd": "sequoiadb",
      //  "SdbUser": "sdbadmin", "SdbPasswd": "sdbadmin", "SdbUserGroup": "sdbadmin_group",
      //  "SshPort": "22",
      //  "zooid": 1,
      //  "installpath":"/opt/zookeeper",
      //  "datapath":"/opt/zookeeper/data",
      //  "dataport":"2888",
      //  "electport":"3888",
      //  "clientport":"2181",
      //  "synclimit":"5",
      //  "initLimit":"10",
      //  "ticktime":"2000"
      // }

      try
      {
         bob.append( OMA_FIELD_DEPLOYMOD, _removeZNInfo._common._deployMod.c_str() ) ;
         bob.append( OMA_FIELD_HOSTNAME, _removeZNInfo._item._hostName.c_str() ) ;
         bob.append( OMA_FIELD_USER, _removeZNInfo._item._user.c_str() ) ;
         bob.append( OMA_FIELD_PASSWD, _removeZNInfo._item._passwd.c_str() ) ;
         bob.append( OMA_FIELD_SDBUSER, _removeZNInfo._common._sdbUser.c_str() ) ;
         bob.append( OMA_FIELD_SDBPASSWD, _removeZNInfo._common._sdbPasswd.c_str() ) ;
         bob.append( OMA_FIELD_SDBUSERGROUP, _removeZNInfo._common._userGroup.c_str() ) ;
         bob.append( OMA_FIELD_SSHPORT, _removeZNInfo._item._sshPort.c_str() ) ;
         bob.append( OMA_FIELD_ZOOID3, _removeZNInfo._item._zooid.c_str() ) ;
         bob.append( OMA_FIELD_INSTALLPATH3, _removeZNInfo._item._installPath.c_str() ) ;
         bob.append( OMA_FIELD_DATAPATH3, _removeZNInfo._item._dataPath.c_str() ) ;
         bob.append( OMA_FIELD_DATAPORT3, _removeZNInfo._item._dataPort.c_str() ) ;
         bob.append( OMA_FIELD_ELECTPORT3, _removeZNInfo._item._electPort.c_str() ) ;
         bob.append( OMA_FIELD_CLIENTPORT3, _removeZNInfo._item._clientPort.c_str() ) ;
         bob.append( OMA_FIELD_SYNCLIMIT3, _removeZNInfo._item._syncLimit.c_str() ) ;
         bob.append( OMA_FIELD_INITLIMIT3, _removeZNInfo._item._initLimit.c_str() ) ;
         bob.append( OMA_FIELD_TICKTIME3, _removeZNInfo._item._tickTime.c_str() ) ;

         // build retObj
         retObj1 = bob.obj() ;
         retObj2 = BSON( OMA_FIELD_TASKID << _removeZNInfo._taskID ) ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG ( PDERROR, "Failed to build bson for add host, "
                      "exception is: %s", e.what() ) ;
         goto error ;
      }
      
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _omaCheckZNodes
   */
   _omaCheckZNodes::_omaCheckZNodes ( vector<CheckZNInfo> &info )
   {
      _checkZNInfos = info ;
   }

   _omaCheckZNodes::~_omaCheckZNodes ()
   {
   }

   INT32 _omaCheckZNodes::init( const CHAR *pInstallInfo )
   {
      INT32 rc = SDB_OK ;
      try
      {
         BSONObj bus ;
         BSONObj sys ;
         stringstream ss ;
         rc = _getCheckZNInfos( bus, sys ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to get checking znodes' info "
                     "for js file, rc = %d", rc ) ;
            goto error ;
         }

         // build js file arguments
         ss << "var " << JS_ARG_BUS << " = " 
            << bus.toString(FALSE, TRUE).c_str() << " ; "
            << "var " << JS_ARG_SYS << " = "
            << sys.toString(FALSE, TRUE).c_str() << " ; " ;
         _jsFileArgs = ss.str() ;
         PD_LOG ( PDDEBUG, "Checking znodes pass argument: %s",
                  _jsFileArgs.c_str() ) ;
         // add js file
         rc = addJsFile( FILE_CHECK_ZOOKEEPER, _jsFileArgs.c_str() ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                     FILE_CHECK_ZOOKEEPER, rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Failed to build bson, exception is: %s",
                  e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error :
      goto done ;
   }

   INT32 _omaCheckZNodes::_getCheckZNInfos( BSONObj &retObj1, BSONObj &retObj2 )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder bob ;
      BSONArrayBuilder bab ;
      const CHAR *pStr = NULL ;
      INT64 taskID     = 0 ;
      vector<CheckZNInfo>::iterator it = _checkZNInfos.begin() ;

      // the output bson format for cluster is:
      // { 
      //  "DeployMod":"distribution",
      //  "ServerInfo":
      //   [ 
      //     {
      //      "HostName":"susetzb",
      //      "User": "root",
      //      "Passwd": "sequoiadb",
      //      "SdbUser": "sdbadmin",
      //      "SdbPasswd": "sdbadmin",
      //      "SdbUserGroup": "sdbadmin_group",
      //      "SshPort": "22",
      //      "zooid": "1",
      //      "installPath":"/opt/zookeeper",
      //      "datapath":"/opt/zookeeper/data",
      //      "dataport":"2888",
      //      "electport":"3888",
      //      "clientport":"2181",
      //      "synclimit":"5",
      //      "initLimit":"10",
      //      "ticktime":"2000",
      //      "clustername":"cl",
      //      "businessname":"bus"
      //     },
      //     ...
      //   ]
      // }

      if ( it == _checkZNInfos.end() )
      {
         rc = SDB_SYS;
         PD_LOG_MSG ( PDERROR, "No znodes' info to check" ) ;
         goto error ;
      }

      pStr = it->_common._deployMod.c_str() ;
      taskID = it->_taskID ;

      try
      {
         for( ; it != _checkZNInfos.end(); it++ )
         {
            BSONObjBuilder builder ;
            BSONObj obj ;
            
            builder.append( OMA_FIELD_HOSTNAME, it->_item._hostName.c_str() ) ;
            builder.append( OMA_FIELD_USER, it->_item._user.c_str() ) ;
            builder.append( OMA_FIELD_PASSWD, it->_item._passwd.c_str() ) ;
            builder.append( OMA_FIELD_SDBUSER, it->_common._sdbUser.c_str() ) ;
            builder.append( OMA_FIELD_SDBPASSWD, it->_common._sdbPasswd.c_str() ) ;
            builder.append( OMA_FIELD_SDBUSERGROUP, it->_common._userGroup.c_str() ) ;
            builder.append( OMA_FIELD_SSHPORT, it->_item._sshPort.c_str() ) ;
            builder.append( OMA_FIELD_ZOOID3, it->_item._zooid.c_str() ) ;
            builder.append( OMA_FIELD_INSTALLPATH3, it->_item._installPath.c_str() ) ;
            builder.append( OMA_FIELD_DATAPATH3, it->_item._dataPath.c_str() ) ;
            builder.append( OMA_FIELD_DATAPORT3, it->_item._dataPort.c_str() ) ;
            builder.append( OMA_FIELD_ELECTPORT3, it->_item._electPort.c_str() ) ;
            builder.append( OMA_FIELD_CLIENTPORT3, it->_item._clientPort.c_str() ) ;
            builder.append( OMA_FIELD_SYNCLIMIT3, it->_item._syncLimit.c_str() ) ;
            builder.append( OMA_FIELD_INITLIMIT3, it->_item._initLimit.c_str() ) ;
            builder.append( OMA_FIELD_TICKTIME3, it->_item._tickTime.c_str() ) ;
            builder.append( OMA_FIELD_CLUSTERNAME3, it->_common._clusterName.c_str() ) ;
            builder.append( OMA_FIELD_BUSINESSNAME3, it->_common._businessName.c_str() ) ;

            obj = builder.obj() ;
            bab.append( obj ) ;
         }

         bob.append( OMA_FIELD_DEPLOYMOD, pStr ) ;
         bob.append( OMA_FIELD_SERVERINFO, bab.arr() ) ;

         // build retObj
         retObj1 = bob.obj() ;
         retObj2 = BSON( OMA_FIELD_TASKID << taskID ) ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG ( PDERROR, "Failed to build bson for add host, "
                      "exception is: %s", e.what() ) ;
         goto error ;
      }
      
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _omaCheckZNEnv
   */
   _omaCheckZNEnv::_omaCheckZNEnv ( vector<CheckZNInfo> &info )
   :_omaCheckZNodes( info )
   {
   }

   _omaCheckZNEnv::~_omaCheckZNEnv ()
   {
   }

   INT32 _omaCheckZNEnv::init( const CHAR *pInstallInfo )
   {
      INT32 rc = SDB_OK ;
      try
      {
         BSONObj bus ;
         BSONObj sys ;
         stringstream ss ;
         rc = _getCheckZNInfos( bus, sys ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to get info for"
                     "js file, rc = %d", rc ) ;
            goto error ;
         }

         // build js file arguments
         ss << "var " << JS_ARG_BUS << " = " 
            << bus.toString(FALSE, TRUE).c_str() << " ; "
            << "var " << JS_ARG_SYS << " = "
            << sys.toString(FALSE, TRUE).c_str() << " ; " ;
         _jsFileArgs = ss.str() ;
         PD_LOG ( PDDEBUG, "Checking znodes' environment pass argument: %s",
                  _jsFileArgs.c_str() ) ;
         // add js file
         rc = addJsFile( FILE_CHECK_ZOOKEEPER_ENV, _jsFileArgs.c_str() ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                     FILE_CHECK_ZOOKEEPER_ENV, rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Failed to build bson, exception is: %s",
                  e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error :
      goto done ;
   }

   _omaCheckSsqlOlap::_omaCheckSsqlOlap( const BSONObj& config, const BSONObj& sysInfo )
   {
      _config = config ;
      _sysInfo = sysInfo ;
   }

   _omaCheckSsqlOlap::~_omaCheckSsqlOlap()
   {
   }

   INT32 _omaCheckSsqlOlap::init( const CHAR *pInstallInfo )
   {
      INT32 rc = SDB_OK ;
      try
      {
         stringstream ss ;

         // build js file arguments
         ss << "var " << JS_ARG_BUS << " = " 
            << _config.toString(FALSE, TRUE).c_str() << " ; "
            << "var " << JS_ARG_SYS << " = "
            << _sysInfo.toString(FALSE, TRUE).c_str() << " ; " ;
         _jsFileArgs = ss.str() ;
         PD_LOG ( PDDEBUG, "Checking sequoiasql olap passes argument: %s",
                  _jsFileArgs.c_str() ) ;

         // add common file
         rc = addJsFile( FILE_SEQUOIASQL_OLAP_COMMON ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                     FILE_SEQUOIASQL_OLAP_COMMON, rc ) ;
            goto error ;
         }

         // add check file
         rc = addJsFile( FILE_SEQUOIASQL_OLAP_CHECK, _jsFileArgs.c_str() ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                     FILE_SEQUOIASQL_OLAP_CHECK, rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Failed to build bson, exception is: %s",
                  e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error :
      goto done ;
   }

   _omaInstallSsqlOlap::_omaInstallSsqlOlap( const BSONObj& config,
                                             const BSONObj& sysInfo )
   {
      _config = config ;
      _sysInfo = sysInfo ;
   }

   _omaInstallSsqlOlap::~_omaInstallSsqlOlap()
   {
   }

   INT32 _omaInstallSsqlOlap::init( const CHAR *pInstallInfo )
   {
      INT32 rc = SDB_OK ;
      try
      {
         stringstream ss ;

         // build js file arguments
         ss << "var " << JS_ARG_BUS << " = " 
            << _config.toString(FALSE, TRUE).c_str() << " ; "
            << "var " << JS_ARG_SYS << " = "
            << _sysInfo.toString(FALSE, TRUE).c_str() << " ; " ;
         _jsFileArgs = ss.str() ;
         PD_LOG ( PDDEBUG, "Installing sequoiasql olap passes argument: %s",
                  _jsFileArgs.c_str() ) ;

         // add common file
         rc = addJsFile( FILE_SEQUOIASQL_OLAP_COMMON ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                     FILE_SEQUOIASQL_OLAP_COMMON, rc ) ;
            goto error ;
         }

         // add config file
         rc = addJsFile( FILE_SEQUOIASQL_OLAP_CONFIG ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                     FILE_SEQUOIASQL_OLAP_CONFIG, rc ) ;
            goto error ;
         }

         // add install file
         rc = addJsFile( FILE_SEQUOIASQL_OLAP_INSTALL, _jsFileArgs.c_str() ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                     FILE_SEQUOIASQL_OLAP_INSTALL, rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Failed to build bson, exception is: %s",
                  e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error :
      goto done ;
   }

   _omaTrustSsqlOlap::_omaTrustSsqlOlap( const BSONObj& config,
                                       const BSONObj& sysInfo )
   {
      _config = config ;
      _sysInfo = sysInfo ;
   }

   _omaTrustSsqlOlap::~_omaTrustSsqlOlap()
   {
   }

   INT32 _omaTrustSsqlOlap::init( const CHAR *pInstallInfo )
   {
      INT32 rc = SDB_OK ;
      try
      {
         stringstream ss ;

         // build js file arguments
         ss << "var " << JS_ARG_BUS << " = " 
            << _config.toString(FALSE, TRUE).c_str() << " ; "
            << "var " << JS_ARG_SYS << " = "
            << _sysInfo.toString(FALSE, TRUE).c_str() << " ; " ;
         _jsFileArgs = ss.str() ;
         PD_LOG ( PDDEBUG, "Trusting sequoiasql olap passes argument: %s",
                  _jsFileArgs.c_str() ) ;

         // add common file
         rc = addJsFile( FILE_SEQUOIASQL_OLAP_COMMON ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                     FILE_SEQUOIASQL_OLAP_COMMON, rc ) ;
            goto error ;
         }

         // add check file
         rc = addJsFile( FILE_SEQUOIASQL_OLAP_TRUST, _jsFileArgs.c_str() ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                     FILE_SEQUOIASQL_OLAP_TRUST, rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Failed to build bson, exception is: %s",
                  e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error :
      goto done ;
   }

   _omaCheckHdfsSsqlOlap::_omaCheckHdfsSsqlOlap( const BSONObj& config,
                                                 const BSONObj& sysInfo )
   {
      _config = config ;
      _sysInfo = sysInfo ;
   }

   _omaCheckHdfsSsqlOlap::~_omaCheckHdfsSsqlOlap()
   {
   }

   INT32 _omaCheckHdfsSsqlOlap::init( const CHAR *pInstallInfo )
   {
      INT32 rc = SDB_OK ;
      try
      {
         stringstream ss ;

         // build js file arguments
         ss << "var " << JS_ARG_BUS << " = " 
            << _config.toString(FALSE, TRUE).c_str() << " ; "
            << "var " << JS_ARG_SYS << " = "
            << _sysInfo.toString(FALSE, TRUE).c_str() << " ; " ;
         _jsFileArgs = ss.str() ;
         PD_LOG ( PDDEBUG, "Checking HDFS for sequoiasql olap passes argument: %s",
                  _jsFileArgs.c_str() ) ;

         // add common file
         rc = addJsFile( FILE_SEQUOIASQL_OLAP_COMMON ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                     FILE_SEQUOIASQL_OLAP_COMMON, rc ) ;
            goto error ;
         }

         // add check file
         rc = addJsFile( FILE_SEQUOIASQL_OLAP_CHECK_HDFS, _jsFileArgs.c_str() ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                     FILE_SEQUOIASQL_OLAP_CHECK_HDFS, rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Failed to build bson, exception is: %s",
                  e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error :
      goto done ;
   }

   _omaInitClusterSsqlOlap::_omaInitClusterSsqlOlap( const BSONObj& config,
                                                     const BSONObj& sysInfo )
   {
      _config = config ;
      _sysInfo = sysInfo ;
   }

   _omaInitClusterSsqlOlap::~_omaInitClusterSsqlOlap()
   {
   }

   INT32 _omaInitClusterSsqlOlap::init( const CHAR *pInstallInfo )
   {
      INT32 rc = SDB_OK ;
      try
      {
         stringstream ss ;

         // build js file arguments
         ss << "var " << JS_ARG_BUS << " = " 
            << _config.toString(FALSE, TRUE).c_str() << " ; "
            << "var " << JS_ARG_SYS << " = "
            << _sysInfo.toString(FALSE, TRUE).c_str() << " ; " ;
         _jsFileArgs = ss.str() ;
         PD_LOG ( PDDEBUG, "Init cluster for sequoiasql olap passes argument: %s",
                  _jsFileArgs.c_str() ) ;

         // add common file
         rc = addJsFile( FILE_SEQUOIASQL_OLAP_COMMON ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                     FILE_SEQUOIASQL_OLAP_COMMON, rc ) ;
            goto error ;
         }

         // add init file
         rc = addJsFile( FILE_SEQUOIASQL_OLAP_INIT, _jsFileArgs.c_str() ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                     FILE_SEQUOIASQL_OLAP_INIT, rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Failed to build bson, exception is: %s",
                  e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error :
      goto done ;
   }

   _omaRemoveSsqlOlap::_omaRemoveSsqlOlap( const BSONObj& config, 
                                           const BSONObj& sysInfo )
   {
      _config = config ;
      _sysInfo = sysInfo ;
   }

   _omaRemoveSsqlOlap::~_omaRemoveSsqlOlap()
   {
   }

   INT32 _omaRemoveSsqlOlap::init( const CHAR *pInstallInfo )
   {
      INT32 rc = SDB_OK ;
      try
      {
         stringstream ss ;

         // build js file arguments
         ss << "var " << JS_ARG_BUS << " = " 
            << _config.toString(FALSE, TRUE).c_str() << " ; "
            << "var " << JS_ARG_SYS << " = "
            << _sysInfo.toString(FALSE, TRUE).c_str() << " ; " ;
         _jsFileArgs = ss.str() ;
         PD_LOG ( PDDEBUG, "Removing sequoiasql olap passes argument: %s",
                  _jsFileArgs.c_str() ) ;

         // add common file
         rc = addJsFile( FILE_SEQUOIASQL_OLAP_COMMON ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                     FILE_SEQUOIASQL_OLAP_COMMON, rc ) ;
            goto error ;
         }

         // add remove file
         rc = addJsFile( FILE_SEQUOIASQL_OLAP_REMOVE, _jsFileArgs.c_str() ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                     FILE_SEQUOIASQL_OLAP_REMOVE, rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Failed to build bson, exception is: %s",
                  e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error :
      goto done ;
   }

   /*
      _omaRunPsqlCmd
   */
   _omaRunPsqlCmd::_omaRunPsqlCmd( SsqlExecInfo &ssqlInfo )
                  :_ssqlInfo( ssqlInfo )
   {
   }
   
   _omaRunPsqlCmd::~_omaRunPsqlCmd()
   {
   }
   
   INT32 _omaRunPsqlCmd::init( const CHAR *nullInfo )
   {
      INT32 rc = SDB_OK ;
      stringstream ss ;
      BSONObj bus ;
      BSONObj sys = BSON( OMA_FIELD_TASKID << _ssqlInfo._taskID ) ;

      bus = BSON( OMA_FIELD_HOSTNAME << _ssqlInfo._hostName << 
                  FIELD_NAME_SERVICE_NAME << _ssqlInfo._serviceName <<
                  OMA_FIELD_USER << _ssqlInfo._sshUser <<
                  OMA_FIELD_PASSWD << _ssqlInfo._sshPasswd <<
                  OMA_FIELD_INSTALLPATH << _ssqlInfo._installPath <<
                  OMA_FIELD_DBNAME << _ssqlInfo._dbName <<
                  OMA_FIELD_DBUSER << _ssqlInfo._dbUser <<
                  OMA_FIELD_DBPASSWD << _ssqlInfo._dbPasswd <<
                  OMA_FIELD_SQL << _ssqlInfo._sql << 
                  OMA_FIELD_RESULTFORMAT << _ssqlInfo._resultFormat ) ;
      // build js file arguments
      ss << "var " << JS_ARG_BUS << " = " 
         << bus.toString(FALSE, TRUE).c_str() << " ; "
         << "var " << JS_ARG_SYS << " = "
         << sys.toString(FALSE, TRUE).c_str() << " ; " ;
      _jsFileArgs = ss.str() ;

      PD_LOG ( PDDEBUG, "ssql execute argument: %s", _jsFileArgs.c_str() ) ;
      // add js file
      rc = addJsFile( FILE_RUN_PSQL, _jsFileArgs.c_str() ) ;
      if ( rc )
      {
         PD_LOG_MSG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                      FILE_RUN_PSQL, rc ) ;
         goto error ;
      }
         
   done:
      return rc ;
   error:
     goto done ;
   }

   /*
      _omaCleanSsqlExecCmd
   */
   _omaCleanSsqlExecCmd::_omaCleanSsqlExecCmd( SsqlExecInfo &ssqlInfo )
                        :_ssqlInfo( ssqlInfo )
   {
   }
   
   _omaCleanSsqlExecCmd::~_omaCleanSsqlExecCmd()
   {
   }
   
   INT32 _omaCleanSsqlExecCmd::init( const CHAR *nullInfo )
   {
      INT32 rc = SDB_OK ;
      stringstream ss ;
      BSONObj bus ;
      BSONObj sys = BSON( OMA_FIELD_TASKID << _ssqlInfo._taskID ) ;

      bus = BSON( OMA_FIELD_HOSTNAME << _ssqlInfo._hostName << 
                  FIELD_NAME_SERVICE_NAME << _ssqlInfo._serviceName <<
                  OMA_FIELD_USER << _ssqlInfo._sshUser <<
                  OMA_FIELD_PASSWD << _ssqlInfo._sshPasswd <<
                  OMA_FIELD_INSTALLPATH << _ssqlInfo._installPath <<
                  OMA_FIELD_DBNAME << _ssqlInfo._dbName <<
                  OMA_FIELD_DBUSER << _ssqlInfo._dbUser <<
                  OMA_FIELD_DBPASSWD << _ssqlInfo._dbPasswd <<
                  OMA_FIELD_SQL << _ssqlInfo._sql << 
                  OMA_FIELD_RESULTFORMAT << _ssqlInfo._resultFormat ) ;
      // build js file arguments
      ss << "var " << JS_ARG_BUS << " = " 
         << bus.toString(FALSE, TRUE).c_str() << " ; "
         << "var " << JS_ARG_SYS << " = "
         << sys.toString(FALSE, TRUE).c_str() << " ; " ;
      _jsFileArgs = ss.str() ;

      PD_LOG ( PDDEBUG, "ssql execute argument: %s", _jsFileArgs.c_str() ) ;
      // add js file
      rc = addJsFile( FILE_CLEAN_SSQL_EXEC, _jsFileArgs.c_str() ) ;
      if ( rc )
      {
         PD_LOG_MSG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                      FILE_CLEAN_SSQL_EXEC, rc ) ;
         goto error ;
      }
         
   done:
      return rc ;
   error:
     goto done ;
   }

   /*
      _omaGetPsqlCmd
   */
   _omaGetPsqlCmd::_omaGetPsqlCmd( SsqlExecInfo &ssqlInfo )
                  :_ssqlInfo( ssqlInfo )
   {
   }
   
   _omaGetPsqlCmd::~_omaGetPsqlCmd()
   {
   }
   
   INT32 _omaGetPsqlCmd::init( const CHAR *nullInfo )
   {
      INT32 rc = SDB_OK ;
      stringstream ss ;
      BSONObj bus ;
      BSONObj sys = BSON( OMA_FIELD_TASKID << _ssqlInfo._taskID ) ;

      bus = BSON( OMA_FIELD_HOSTNAME << _ssqlInfo._hostName << 
                  FIELD_NAME_SERVICE_NAME << _ssqlInfo._serviceName <<
                  OMA_FIELD_USER << _ssqlInfo._sshUser <<
                  OMA_FIELD_PASSWD << _ssqlInfo._sshPasswd <<
                  OMA_FIELD_INSTALLPATH << _ssqlInfo._installPath <<
                  OMA_FIELD_DBNAME << _ssqlInfo._dbName <<
                  OMA_FIELD_DBUSER << _ssqlInfo._dbUser <<
                  OMA_FIELD_DBPASSWD << _ssqlInfo._dbPasswd <<
                  OMA_FIELD_SQL << _ssqlInfo._sql << 
                  OMA_FIELD_RESULTFORMAT << _ssqlInfo._resultFormat ) ;
      // build js file arguments
      ss << "var " << JS_ARG_BUS << " = " 
         << bus.toString(FALSE, TRUE).c_str() << " ; "
         << "var " << JS_ARG_SYS << " = "
         << sys.toString(FALSE, TRUE).c_str() << " ; " ;
      _jsFileArgs = ss.str() ;

      PD_LOG ( PDDEBUG, "ssql execute argument: %s", _jsFileArgs.c_str() ) ;
      // add js file
      rc = addJsFile( FILE_GET_PSQL, _jsFileArgs.c_str() ) ;
      if ( rc )
      {
         PD_LOG_MSG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                      FILE_GET_PSQL, rc ) ;
         goto error ;
      }
         
   done:
      return rc ;
   error:
     goto done ;
   }

}

