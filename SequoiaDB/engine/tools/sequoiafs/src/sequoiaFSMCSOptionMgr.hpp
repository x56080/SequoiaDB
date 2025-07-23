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

   Source File Name = sequoiaFSMCSOptionMgr.hpp

   Descriptive Name = MCS options manager.

   When/how to use:  This program is used on sequoiafs.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date      Who Description
   ====== =========== === ==============================================
        01/07/2021  YWX  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef _SEQUOIAFSMCS_OPTIONMGR_HPP_
#define _SEQUOIAFSMCS_OPTIONMGR_HPP_

#include "pmdOptionsMgr.hpp"
#include "utilStr.hpp"

#define SDB_SEQUOIAFS_MCS_EXE_FILE_NAME "sequoiamcs"
#define SDB_SEQUOIAFS_MCS_CFG_FILE_NAME SDB_SEQUOIAFS_MCS_EXE_FILE_NAME".conf"
#define SDB_SEQUOIAFS_MCS_LOG_FILE_NAME SDB_SEQUOIAFS_MCS_EXE_FILE_NAME".log"

#define SDB_SEQUOIAFS_MCS_HELP            "help"
#define SDB_SEQUOIAFS_MCS_VERSION         "version"
#define SDB_SEQUOIAFS_MCS_HOSTS           "hosts"
#define SDB_SEQUOIAFS_MCS_USERNAME        "username"
#define SDB_SEQUOIAFS_MCS_PASSWD          "passwd"
#define SDB_SEQUOIAFS_MCS_CONNECTION_NUM  "connectionnum"
#define SDB_SEQUOIAFS_MCS_CONF_PATH       "confpath"
#define SDB_SEQUOIAFS_MCS_DIAGLEVEL       "diaglevel"
#define SDB_SEQUOIAFS_MCS_DIAGPATH        "diagpath"
#define SDB_SEQUOIAFS_MCS_DIAGNUM         "diagnum"
#define SDB_SEQUOIAFS_MCS_PORT            "port"
#define SDB_SEQUOIAFS_MCS_START_FORCE     "force"

#define SDB_SEQUOIAFS_MCS_CONNECTION_DEFAULT_MAX_NUM 100
//#define SDB_SEQUOIAFS_CACHE_DEFAULT_SIZE 100000
#define SDB_SEQUOIAFS_MCS_HOSTS_DEFAULT_VALUE "localhost:11810"
#define SDB_SEQUOIAFS_MCS_USER_DEFAULT_NAME "sdbadmin"
#define SDB_SEQUOIAFS_MCS_USER_DEFAULT_PASSWD "sdbadmin"
#define SDB_SEQUOIAFS_MCS_DEFAULT_PORT "11742"

namespace sequoiafs
{
   class _sequoiafsMcsOptionMgr : public engine::_pmdCfgRecord
   {
      public:
         _sequoiafsMcsOptionMgr() ;
         virtual ~_sequoiafsMcsOptionMgr(){}

         INT32 init( INT32 argc, CHAR **argv) ;
         INT32 save() ;
         void setSvcName( const CHAR *svcName ) ;
         PDLEVEL getDiaglogLevel()const ;
         const CHAR *getCfgFileName()const{return _cfgFileName ;}
         const CHAR *getHosts()const{return _hosts ;}
         const CHAR *getUserName()const{return _userName ;}
         const CHAR *getPasswd()const{return _passwd ;}
         const INT32 getConnNum()const{return _connectionNum ;}
         const INT32 getDiagMaxNUm()const{return _diagnum ;}
         const CHAR *getCfgPath()const{return _cfgPath ;}
         CHAR *getDiaglogPath(){return _diagPath ;}
         CHAR *getPort(){return _port;}
         const BOOLEAN getForceStart()const{return _forcestart;}

      protected:
         virtual INT32 doDataExchange( engine::pmdCfgExchange *pEX ) ;

      private:
         CHAR _hosts[OSS_MAX_PATHSIZE + 1] ;
         CHAR _userName[OSS_MAX_PATHSIZE + 1] ;
         CHAR _passwd[OSS_MAX_PATHSIZE + 1] ;

         INT32 _connectionNum ;
         CHAR _cfgPath[OSS_MAX_PATHSIZE + 1] ;
         CHAR _cfgFileName[OSS_MAX_PATHSIZE + 1] ;
         CHAR _diagPath[OSS_MAX_PATHSIZE + 1] ;
         CHAR _port[OSS_MAX_SERVICENAME + 1] ;
         INT32 _diagnum ;
         UINT16 _diagLevel ;
         BOOLEAN _forcestart ;
   } ;
   typedef _sequoiafsMcsOptionMgr sequoiafsMcsOptionMgr ;
}

#endif