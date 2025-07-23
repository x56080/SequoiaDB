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

   Source File Name = stpToolCommon.hpp

   Descriptive Name = Serial Time Protocol common defines for tools or drivers

   When/how to use: this program may be used on binary and text-formatted
   versions of STP component. This file contains structure for Serial Time
   Protocol.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/30/2019  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef STP_TOOL_COMMON_HPP__
#define STP_TOOL_COMMON_HPP__

#include "oss.hpp"
#include "ossUtil.hpp"
#include "pd.hpp"
#include "omagentDef.hpp"

namespace engine
{

   // name of STP program
   #define STP_NAME                 "stp"
   // name of stpstart program
   #define STPSTART_NAME            "stpstart"
   // name of stpstop program
   #define STPSTOP_NAME             "stpstop"

#if defined (_LINUX)
   // name of executable file of STP
   #define STP_EXE_FILE_NAME        STP_NAME
   // name of executable file of stpstart
   #define STPSTART_EXE_FILE_NAME   STPSTART_NAME
   // name of executable file of stpstop
   #define STPSTOP_EXE_FILE_NAME    STPSTOP_NAME
#elif defined (_WINDOWS)
   // name of executable file of STP
   #define STP_EXE_FILE_NAME        STP_NAME".exe"
   // name of executable file of stpstart
   #define STPSTART_EXE_FILE_NAME   STPSTART_NAME".exe"
   // name of executable file of stpstop
   #define STPSTOP_EXE_FILE_NAME    STPSTOP_NAME".exe"
#endif

   // directory structure
   // |- conf
   //     |- stp
   //         |- stp.conf
   //         |- stp.log
   //         |- stp.pid
   //         |- stp.meta

   // name of stp directory
   #define STP_DIR_NAME             STP_NAME
   // name configure file of STP
   #define STP_CFG_FILE_NAME        STP_NAME".conf"
   // name of diagnostic log file of STP
   #define STP_DIAGLOG_FILE_NAME    STP_NAME".log"
   // name of PID file of STP
   #define STP_PID_FILE_NAME        STP_NAME".pid"
   // name of meta file of STP ( store meta LSN )
   #define STP_META_FILE_NAME       STP_NAME".meta"

   // user and password for STP service sessions
   #define STP_USER                 "STP_ADMIN"
   #define STP_USERPASSWD           "STP_ADMIN_PASSWD"

   // append "stp" to "conf" which is the conf path of sdbcm
   #define STP_ROOT_PATH            SDB_CM_ROOT_PATH STP_DIR_NAME OSS_FILE_SEP
   // log path is the same as root path
   #define STP_LOG_PATH             STP_ROOT_PATH

   // names of STP options
   // port of STP
   #define STP_OPTION_PORT             PMD_OPTION_PORT
   // role of STP
   #define STP_OPTION_ROLE             PMD_OPTION_ROLE
   // weight of STP server to vote for primary
   #define STP_OPTION_WEIGHT           PMD_OPTION_WEIGHT
   // diagnostic level of STP
   #define STP_OPTION_DIAGLEVEL        PMD_OPTION_DIAGLEVEL
   // sharing break timeout of STP servers
   // NOTE: timeout between servers to detect each other
   #define STP_OPTION_SHARINGBRK       PMD_OPTION_SHARINGBRK
   // start shift time of STP servers
   // NOTE: for new added server, it will not become primary in
   //       start shift time
   #define STP_OPTION_STARTSHIFTTIME   PMD_OPTION_START_SHIFT_TIME
   // configuration path of STP
   #define STP_OPTION_CONFPATH         PMD_OPTION_CONFPATH
   // get version of STP
   #define STP_OPTION_VERSION          PMD_OPTION_VERSION
   // get help of STP
   #define STP_OPTION_HELP             PMD_OPTION_HELP
   // get full help of STP
   #define STP_OPTION_HELPFULL         PMD_OPTION_HELPFULL
   #define STP_OPTION_CURUSER          PMD_OPTION_CURUSER

   // server list
   #define STP_OPTION_SERVERLIST       "serverlist"
   // synchronize interval
   #define STP_OPTION_SYNCINTERVAL     "syncinterval"
   // max time error
   #define STP_OPTION_MAXTIMEERROR     "maxtimeerror"
   // max synchronize history records
   #define STP_OPTION_MAXSYNCHIST      "maxsynchist"
   // maximum UDP ports to synchronize time
   #define STP_OPTION_MAXSYNCPORTS     "maxsyncports"
   // default synchronize clients per UDP port
   #define STP_OPTION_DEFCLIENTSPERPORT "defclientsperport"
   // force to start synchronize ports
   #define STP_OPTION_PREOPENPORTS     "preopenports"
   // allow synchronize with system port
   #define STP_OPTION_SYNCWITHSYSPORT  "allowsyncwithsysport"
   // daemon mode
   #define STP_OPTION_DAEMON           "daemon"
   // test mode
   #define STP_OPTION_TESTMODE         "testmode"

   // default STP port ( service name ) is 9622
   #define STP_DEF_PORT                ( 9622 )
   #define STP_DEF_SERVICE_NAME        "9622"

   // unknown STP host and service names
   #define STP_UNKNOWN_HOST_NAME       "Unknown"
   #define STP_UNKNOWN_SERVICE_NAME    "Unknown"

   /*
      STP_ROLE define
    */
   // roles of STP
   enum STP_ROLE
   {
      // server: run as synchronize server which could vote primary
      // NOTE: only primary server is the synchronize source
      STP_ROLE_SERVER = 0,
      // client: run as synchronize client
      STP_ROLE_CLIENT
   } ;

   // names of STP role
   #define STP_ROLE_MANE_UNKNOWN       "unknown"
   #define STP_ROLE_NAME_CLIENT        "client"
   #define STP_ROLE_NAME_SERVER        "server"

   // convert STP role to its name
   OSS_INLINE const CHAR *stpGetRoleName( STP_ROLE role )
   {
      switch ( role )
      {
         case STP_ROLE_CLIENT :
            return STP_ROLE_NAME_CLIENT ;
         case STP_ROLE_SERVER :
            return STP_ROLE_NAME_SERVER ;
         default :
            break ;
      }
      return STP_ROLE_MANE_UNKNOWN ;
   }

   // convert to STP role by name
   OSS_INLINE STP_ROLE stpGetRoleByName( const CHAR *roleName )
   {
      if ( NULL != roleName )
      {
         if ( 0 == ossStrcmp( roleName, STP_ROLE_NAME_CLIENT ) )
         {
            return STP_ROLE_CLIENT ;
         }
         else if ( 0 == ossStrcmp( roleName, STP_ROLE_NAME_SERVER ) )
         {
            return STP_ROLE_SERVER ;
         }
      }
      // default is server
      return STP_ROLE_SERVER ;
   }

   // check if role is valid
   OSS_INLINE BOOLEAN stpCheckRole( STP_ROLE role )
   {
      return ( STP_ROLE_CLIENT == role ||
               STP_ROLE_SERVER == role ) ;
   }

   /*
      STP_SYNC_STATUS
    */
   // synchronize status of STP
   enum STP_SYNC_STATUS
   {
      // no synchronize source
      STP_SYNC_NOSOURCE = 0,
      // quick check time offset
      // NOTE: send synchronize request at high frequency, to adjust the
      //       offset between source and client quickly
      STP_SYNC_CHECKOFFSET,
      // check slew rate between machines
      // NOTE: frequency of CPU ticks might be different between client
      //       and source, send few synchronize requests in equal periods to
      //       calculate the slew rate between CPU ticks of client and source
      STP_SYNC_CHECKSLEWRATE,
      // re-check time offset
      // NOTE: send synchronize request at high frequency, to adjust the
      //       offset between source and client quickly after slew rate is
      //       adjusted
      STP_SYNC_RECHECKOFFSET,
      // interval check ( specified by --syncinterval )
      // NOTE: send synchronize request at high frequency, to adjust the
      //       offset between source and client quickly after slew rate is
      //       adjusted
      STP_SYNC_INTERVALCHECK,
      // check error ( delay too long )
      // NOTE: send synchronize request at high frequency, to adjust the time
      //       error to tolerate the delay
      STP_SYNC_CHECKERROR
   } ;

   // names of STP synchronize status
   #define STP_SYNC_STATUS_NAME_UNKNOWN         "Unknown"
   #define STP_SYNC_STATUS_NAME_NOSOURCE        "NoSource"
   #define STP_SYNC_STATUS_NAME_CHECKOFFSET     "CheckOffset"
   #define STP_SYNC_STATUS_NAME_CHECKSLEWRATE   "CheckSlewRate"
   #define STP_SYNC_STATUS_NAME_RECHECKOFFSET   "RecheckOffset"
   #define STP_SYNC_STATUS_NAME_INTERVALCHECK   "IntervalCheck"
   #define STP_SYNC_STATUS_NAME_CHECKERROR      "CheckError"

   // convert STP synchronize status to its name
   OSS_INLINE const CHAR *stpGetSyncStatusName( STP_SYNC_STATUS status )
   {
      switch ( status )
      {
         case STP_SYNC_NOSOURCE :
            return STP_SYNC_STATUS_NAME_NOSOURCE ;
         case STP_SYNC_CHECKOFFSET :
            return STP_SYNC_STATUS_NAME_CHECKOFFSET ;
         case STP_SYNC_CHECKSLEWRATE :
            return STP_SYNC_STATUS_NAME_CHECKSLEWRATE ;
         case STP_SYNC_RECHECKOFFSET :
            return STP_SYNC_STATUS_NAME_RECHECKOFFSET ;
         case STP_SYNC_INTERVALCHECK :
            return STP_SYNC_STATUS_NAME_INTERVALCHECK ;
         case STP_SYNC_CHECKERROR :
            return STP_SYNC_STATUS_NAME_CHECKERROR ;
         default :
            break ;
      }
      return STP_SYNC_STATUS_NAME_UNKNOWN ;
   }

   // convert to STP synchronize status by name
   OSS_INLINE STP_SYNC_STATUS stpGetSyncStatusByName( const CHAR *statusName )
   {
      if ( NULL != statusName )
      {
         if ( 0 == ossStrcmp( statusName,
                              STP_SYNC_STATUS_NAME_NOSOURCE ) )
         {
            return STP_SYNC_NOSOURCE ;
         }
         else if ( 0 == ossStrcmp( statusName,
                                   STP_SYNC_STATUS_NAME_CHECKOFFSET ) )
         {
            return STP_SYNC_CHECKOFFSET ;
         }
         else if ( 0 == ossStrcmp( statusName,
                                   STP_SYNC_STATUS_NAME_CHECKSLEWRATE ) )
         {
            return STP_SYNC_CHECKSLEWRATE ;
         }
         else if ( 0 == ossStrcmp( statusName,
                                   STP_SYNC_STATUS_NAME_RECHECKOFFSET) )
         {
            return STP_SYNC_RECHECKOFFSET ;
         }
         else if ( 0 == ossStrcmp( statusName,
                                   STP_SYNC_STATUS_NAME_INTERVALCHECK ) )
         {
            return STP_SYNC_INTERVALCHECK ;
         }
         else if ( 0 == ossStrcmp( statusName,
                                   STP_SYNC_STATUS_NAME_CHECKERROR ) )
         {
            return STP_SYNC_CHECKERROR ;
         }
      }
      // default is no-source
      return STP_SYNC_NOSOURCE ;
   }

   // check if synchronize status is valid
   OSS_INLINE BOOLEAN stpCheckSyncStatus( STP_SYNC_STATUS status )
   {
      return ( STP_SYNC_NOSOURCE == status ||
               STP_SYNC_CHECKOFFSET == status ||
               STP_SYNC_CHECKSLEWRATE == status ||
               STP_SYNC_RECHECKOFFSET == status ||
               STP_SYNC_INTERVALCHECK == status ||
               STP_SYNC_CHECKERROR == status ) ;
   }

   /*
      STP_NODE_STATUS
    */
   // node status of STP
   enum STP_NODE_STATUS
   {
      // node information is normal
      STP_NODE_NORMAL = 0,
      // need to update server information
      STP_NODE_QUERYSERVERS,
      // need to add self into servers
      STP_NODE_ADDSERVER,
      // need to remove self from servers
      STP_NODE_REMOVESERVER
   } ;

   // names of STP node status
   #define STP_NODE_STATUS_NAME_UNKNOWN      "Unknown"
   #define STP_NODE_STATUS_NAME_NORMAL       "Normal"
   #define STP_NODE_STATUS_NAME_QUERYSERVERS "QueryServers"
   #define STP_NODE_STATUS_NAME_ADDSERVER    "AddServer"
   #define STP_NODE_STATUS_NAME_REMOVESERVER "RemoveServer"

   // convert STP node status to its name
   OSS_INLINE const CHAR *stpGetNodeStatusName( STP_NODE_STATUS status )
   {
      switch ( status )
      {
         case STP_NODE_NORMAL :
            return STP_NODE_STATUS_NAME_NORMAL ;
         case STP_NODE_QUERYSERVERS :
            return STP_NODE_STATUS_NAME_QUERYSERVERS ;
         case STP_NODE_ADDSERVER :
            return STP_NODE_STATUS_NAME_ADDSERVER ;
         case STP_NODE_REMOVESERVER :
            return STP_NODE_STATUS_NAME_REMOVESERVER ;
         default :
            break ;
      }
      return STP_NODE_STATUS_NAME_UNKNOWN ;
   }

   // names of fields for STP BSON output
   #define STP_FIELD_NAME_VERSION            FIELD_NAME_VERSION
   #define STP_FIELD_NAME_ROLE               FIELD_NAME_ROLE
   #define STP_FIELD_NAME_PRIMARY            FIELD_NAME_PRIMARY
   #define STP_FIELD_NAME_IS_PRIMARY         FIELD_NAME_IS_PRIMARY
   #define STP_FIELD_NAME_GROUPID            FIELD_NAME_GROUPID
   #define STP_FIELD_NAME_NODEID             FIELD_NAME_NODEID
   #define STP_FIELD_NAME_HOST               FIELD_NAME_HOST
   #define STP_FIELD_NAME_SERVICE            FIELD_NAME_SERVICE
   #define STP_FIELD_NAME_GROUP              FIELD_NAME_GROUP
   #define STP_FIELD_NAME_SYNC_CLIENTS       "SyncClients"
   #define STP_FIELD_NAME_SYNC_SOURCE        "SyncSource"
   #define STP_FIELD_NAME_SYNC_SOURCES       "SyncSources"
   #define STP_FIELD_NAME_NODE_OID           "OID"
   #define STP_FIELD_NAME_SYNC_STATUS        "SyncStatus"
   #define STP_FIELD_NAME_SYNC_INTERVAL      "SyncInterval"
   #define STP_FIELD_NAME_TIME_ERROR         "TimeError"
   #define STP_FIELD_NAME_MAX_TIME_ERROR     "MaxTimeError"
   #define STP_FIELD_NAME_SYNC_PASSED        "SyncPassed"
   #define STP_FIELD_NAME_UPDATE_PASSED      "LastPassed"
   #define STP_FIELD_NAME_SYNC_COUNT         "SyncCount"
   #define STP_FIELD_NAME_SYNC_PORT          "SyncPort"
   #define STP_FIELD_NAME_SYNC_HISTORY       "SyncHistory"
   #define STP_FIELD_NAME_VALID_COUNT        "ValidCount"
   #define STP_FIELD_NAME_MAX_DELAY          "MaxDelay"
   #define STP_FIELD_NAME_MIN_DELAY          "MinDelay"
   #define STP_FIELD_NAME_LAST_DELAY         "LastDelay"
   #define STP_FIELD_NAME_INIT_OFFSET        "InitOffset"
   #define STP_FIELD_NAME_POS_OFFSET         "PosOffset"
   #define STP_FIELD_NAME_NEG_OFFSET         "NegOffset"
   #define STP_FIELD_NAME_LAST_OFFSET        "LastOffset"
   #define STP_FIELD_NAME_OFFSET_COUNT       FIELD_NAME_COUNT
   #define STP_FIELD_NAME_OFFSET_MIN         FIELD_NAME_MIN
   #define STP_FIELD_NAME_OFFSET_MAX         FIELD_NAME_MAX
   #define STP_FIELD_NAME_TIMESTAMP          "TimeStamp"
   #define STP_FIELD_NAME_OFFSET             "Offset"
   #define STP_FIELD_NAME_SLEW_RATE          "SlewRate"
   #define STP_FIELD_NAME_BASE_HW_TIME       "BaseHWTime"
   #define STP_FIELD_NAME_BASE_REAL_TIME     "BaseRealTime"
   #define STP_FIELD_NAME_SYNC_HW_TIME       "SyncHWTime"
   #define STP_FIELD_NAME_META_DATA          "MetaData"
   #define STP_FIELD_NAME_META_SHMKEY        "MetaSHMKey"
   #define STP_FIELD_NAME_META_LSN           "MetaLSN"
   #define STP_FIELD_NAME_META_OFFSET        FIELD_NAME_LSN_OFFSET
   #define STP_FIELD_NAME_META_VERSION       FIELD_NAME_LSN_VERSION
   #define STP_FIELD_NAME_SECOND             "Second"
   #define STP_FIELD_NAME_NANO_SECOND        "NanoSecond"
   #define STP_FIELD_NAME_REQUEST_ID         "RequestID"
   #define STP_FIELD_NAME_DELAY              "Delay"

}

#endif // STP_TOOL_COMMON_HPP__
