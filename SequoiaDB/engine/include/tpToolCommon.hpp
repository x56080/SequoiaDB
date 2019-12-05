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

   Source File Name = tpToolCommon.hpp

   Descriptive Name = SequoiaDB Time Protocol Service

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for SequoiaDB
   Time Protocol Service.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/30/2019  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef TP_TOOL_COMMON_HPP__
#define TP_TOOL_COMMON_HPP__

#include "oss.hpp"
#include "pd.hpp"
#include "omagentDef.hpp"

namespace engine
{

   #define SDBTP_NAME                  "sdbtp"
   #define SDBTP_START_NAME            "sdbtpart"
   #define SDBTP_STOP_NAME             "sdbtptop"

#if defined (_LINUX)
   #define SDBTP_EXE_FILE_NAME         SDBTP_NAME
   #define SDBTPART_EXE_FILE_NAME      SDBTP_START_NAME
   #define SDBTPTOP_EXE_FILE_NAME      SDBTP_STOP_NAME
#elif defined (_WINDOWS)
   #define SDBTP_EXE_FILE_NAME         SDBTP_NAME".exe"
   #define SDBTPART_EXE_FILE_NAME      SDBTP_START_NAME".exe"
   #define SDBTPTOP_EXE_FILE_NAME      SDBTP_STOP_NAME".exe"
#endif
   #define SDBTP_CFG_FILE_NAME         SDBTP_NAME".conf"
   #define SDBTP_DIALOG_FILE_NAME      SDBTP_NAME".log"
   #define SDBTP_PID_FILE_NAME         SDBTP_NAME".pid"
   #define SDBTP_META_FILE_NAME        SDBTP_NAME".meta"

   #define SDB_TP_USER                 "TP_ADMIN"
   #define SDB_TP_USERPASSWD           "TP_ADMIN_PASSWD"

   // shared the same path as sdbcm
   #define SDBTP_ROOT_PATH             SDB_CM_ROOT_PATH
   #define SDBTP_LOG_PATH              SDBCM_LOG_PATH

   #define PMD_TP_OPTION_SERVERLIST    "serverlist"
   #define PMD_TP_OPTION_SYNCINTERVAL  "syncinterval"
   #define PMD_TP_OPTION_MAXTIMEERROR  "maxtimeerror"

   #define TP_DEF_PORT                 ( 9622 )
   #define TP_DEF_SVCNAME              "9622"

   /*
      TP_ROLE define
    */
   enum TP_ROLE
   {
      // standalone: only one tp node
      TP_ROLE_STANDALONE = -1,
      // client: run as synchronize client
      TP_ROLE_CLIENT = -2,
      // server: run as synchronize server which could vote primary
      // NOTE: only primary server is the synchronize source
      TP_ROLE_SERVER = -3,
   } ;

   #define TP_ROLE_MANE_UNKNOWN        "unknown"
   #define TP_ROLE_NAME_STANDALONE     "standalone"
   #define TP_ROLE_NAME_CLIENT         "client"
   #define TP_ROLE_NAME_SERVER         "server"

   OSS_INLINE const CHAR *tpGetRoleName( TP_ROLE role )
   {
      switch ( role )
      {
         case TP_ROLE_STANDALONE :
            return TP_ROLE_NAME_STANDALONE ;
         case TP_ROLE_SERVER :
            return TP_ROLE_NAME_SERVER ;
         case TP_ROLE_CLIENT :
            return TP_ROLE_NAME_CLIENT ;
         default :
            break ;
      }
      return TP_ROLE_MANE_UNKNOWN ;
   }

   /*
      TP_SYNC_STATUS
    */
   enum TP_SYNC_STATUS
   {
      // no synchronize source
      TP_SYNC_NOSOURCE = 0,
      // quick check time offset
      TP_SYNC_CHECKOFFSET,
      // check slew rate between machines
      TP_SYNC_CHECKSLEWRATE,
      // re-check time offset
      TP_SYNC_RECHECKOFFSET,
      // interval check ( specified by --syncinterval )
      TP_SYNC_INTERVALCHECK,
      // check error ( delay too long )
      TP_SYNC_CHECKERROR
   } ;

   #define TP_SYNC_STATUS_NAME_UNKNOWN          "Unknown"
   #define TP_SYNC_STATUS_NAME_NOSOURCE         "NoSource"
   #define TP_SYNC_STATUS_NAME_CHECKOFFSET      "CheckOffset"
   #define TP_SYNC_STATUS_NAME_CHECKSLEWRATE    "CheckSlewRate"
   #define TP_SYNC_STATUS_NAME_RECHECKOFFSET    "RecheckOffset"
   #define TP_SYNC_STATUS_NAME_INTERVALCHECK    "IntervalCheck"
   #define TP_SYNC_STATUS_NAME_CHECKERROR       "CheckError"

   OSS_INLINE const CHAR *tpGetSyncStatusName( TP_SYNC_STATUS status )
   {
      switch ( status )
      {
         case TP_SYNC_NOSOURCE :
            return TP_SYNC_STATUS_NAME_NOSOURCE ;
         case TP_SYNC_CHECKOFFSET :
            return TP_SYNC_STATUS_NAME_CHECKOFFSET ;
         case TP_SYNC_CHECKSLEWRATE :
            return TP_SYNC_STATUS_NAME_CHECKSLEWRATE ;
         case TP_SYNC_RECHECKOFFSET :
            return TP_SYNC_STATUS_NAME_RECHECKOFFSET ;
         case TP_SYNC_INTERVALCHECK :
            return TP_SYNC_STATUS_NAME_INTERVALCHECK ;
         case TP_SYNC_CHECKERROR :
            return TP_SYNC_STATUS_NAME_CHECKERROR ;
         default :
            break ;
      }
      return TP_SYNC_STATUS_NAME_UNKNOWN ;
   }

   /*
      TP_CATALOG_STATUS
    */
   enum TP_CATALOG_STATUS
   {
      // catalog information is normal
      TP_CATALOG_NORMAL = 0,
      // need to update catalog information
      TP_CATALOG_QUERY,
      // need to add self into servers
      TP_CATALOG_ADDSERVER,
      // need to remove self from servers
      TP_CATALOG_REMOVESERVER
   } ;

   #define TP_FIELD_NAME_VERSION             FIELD_NAME_VERSION
   #define TP_FIELD_NAME_ROLE                FIELD_NAME_ROLE
   #define TP_FIELD_NAME_PRIMARY             FIELD_NAME_PRIMARY
   #define TP_FIELD_NAME_IS_PRIMARY          FIELD_NAME_IS_PRIMARY
   #define TP_FIELD_NAME_GROUPID             FIELD_NAME_GROUPID
   #define TP_FIELD_NAME_NODEID              FIELD_NAME_NODEID
   #define TP_FIELD_NAME_HOST                FIELD_NAME_HOST
   #define TP_FIELD_NAME_SERVICE             FIELD_NAME_SERVICE
   #define TP_FIELD_NAME_GROUP               FIELD_NAME_GROUP
   #define TP_FIELD_NAME_SYNC_CLIENTS        "SyncClients"
   #define TP_FIELD_NAME_SYNC_SOURCES        "SyncSources"
   #define TP_FIELD_NAME_NODE_OID            "OID"
   #define TP_FIELD_NAME_SYNC_STATUS         "SyncStatus"
   #define TP_FIELD_NAME_SYNC_INTERVAL       "SyncInterval"
   #define TP_FIELD_NAME_TIME_ERROR          "TimeError"
   #define TP_FIELD_NAME_MAX_TIME_ERROR      "MaxTimeError"
   #define TP_FIELD_NAME_LAST_SYNC_PASSED    "LastSyncPassed"
   #define TP_FIELD_NAME_SYNC_COUNT          "SyncCount"
   #define TP_FIELD_NAME_VALID_COUNT         "ValidCount"
   #define TP_FIELD_NAME_MAX_DELAY           "MaxDelay"
   #define TP_FIELD_NAME_MIN_DELAY           "MinDelay"
   #define TP_FIELD_NAME_INIT_OFFSET         "InitOffset"
   #define TP_FIELD_NAME_TIMESTAMP           "TimeStamp"
   #define TP_FIELD_NAME_OFFSET              "Offset"
   #define TP_FIELD_NAME_SLEW_RATE           "SlewRate"
   #define TP_FIELD_NAME_BASE_HW_TIME        "BaseHWTime"
   #define TP_FIELD_NAME_BASE_REAL_TIME      "BaseRealTime"
   #define TP_FIELD_NAME_SYNC_TIME           "SyncTime"
   #define TP_FIELD_NAME_META_DATA           "MetaData"
   #define TP_FIELD_NAME_META_SHMKEY         "MetaSHMKey"
   #define TP_FIELD_NAME_META_LSN            "MetaLSN"
   #define TP_FIELD_NAME_META_OFFSET         FIELD_NAME_LSN_OFFSET
   #define TP_FIELD_NAME_META_VERSION        FIELD_NAME_LSN_VERSION
   #define TP_FIELD_NAME_SECOND              "Second"
   #define TP_FIELD_NAME_NANO_SECOND         "NanoSecond"

}

#endif // TP_TOOL_COMMON_HPP__
