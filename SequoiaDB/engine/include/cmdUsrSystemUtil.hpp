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

   Source File Name = cmdUsrSystemUtil.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/12/2016  WJM  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef CMD_USRSYSTEM_UTIL_HPP__
#define CMD_USRSYSTEM_UTIL_HPP__

namespace engine
{

#if defined (_LINUX)
   struct _cpuInfo
   {
      string modelName ;
      string coreNum ;
      string freq ;
      string physicalID ;
      void reset()
      {
         modelName  = "" ;
         coreNum    = "1" ;
         freq       = "" ;
         physicalID = "0" ;
      }
   } ;
   typedef struct _cpuInfo cpuInfo ;

   #define HOSTS_FILE      "/etc/hosts"
#else
   #define HOSTS_FILE      "C:\\Windows\\System32\\drivers\\etc\\hosts"
#endif // _LINUX

   #define CMD_USR_SYSTEM_DISTRIBUTOR        "Distributor"
   #define CMD_USR_SYSTEM_RELASE             "Release"
   #define CMD_USR_SYSTEM_DESP               "Description"
   #define CMD_USR_SYSTEM_BIT                "Bit"
   #define CMD_USR_SYSTEM_IP                 "Ip"
   #define CMD_USR_SYSTEM_HOSTS              "Hosts"
   #define CMD_USR_SYSTEM_HOSTNAME           "HostName"
   #define CMD_USR_SYSTEM_FILESYSTEM         "Filesystem"
   #define CMD_USR_SYSTEM_CORE               "Core"
   #define CMD_USR_SYSTEM_INFO               "Info"
   #define CMD_USR_SYSTEM_FREQ               "Freq"
   #define CMD_USR_SYSTEM_CPUS               "Cpus"
   #define CMD_USR_SYSTEM_USER               "User"
   #define CMD_USR_SYSTEM_SYS                "Sys"
   #define CMD_USR_SYSTEM_IDLE               "Idle"
   #define CMD_USR_SYSTEM_OTHER              "Other"
   #define CMD_USR_SYSTEM_SIZE               "Size"
   #define CMD_USR_SYSTEM_FREE               "Free"
   #define CMD_USR_SYSTEM_USED               "Used"
   #define CMD_USR_SYSTEM_ISLOCAL            "IsLocal"
   #define CMD_USR_SYSTEM_MOUNT              "Mount"
   #define CMD_USR_SYSTEM_DISKS              "Disks"
   #define CMD_USR_SYSTEM_NAME               "Name"
   #define CMD_USR_SYSTEM_NETCARDS           "Netcards"
   #define CMD_USR_SYSTEM_TARGET             "Target"
   #define CMD_USR_SYSTEM_REACHABLE          "Reachable"
   #define CMD_USR_SYSTEM_USABLE             "Usable"
   #define CMD_USR_SYSTEM_UNIT               "Unit"
   #define CMD_USR_SYSTEM_FSTYPE             "FsType"
   #define CMD_USR_SYSTEM_IO_R_SEC           "ReadSec"
   #define CMD_USR_SYSTEM_IO_W_SEC           "WriteSec"

   #define CMD_USR_SYSTEM_RX_PACKETS         "RXPackets"
   #define CMD_USR_SYSTEM_RX_BYTES           "RXBytes"
   #define CMD_USR_SYSTEM_RX_ERRORS          "RXErrors"
   #define CMD_USR_SYSTEM_RX_DROPS           "RXDrops"

   #define CMD_USR_SYSTEM_TX_PACKETS         "TXPackets"
   #define CMD_USR_SYSTEM_TX_BYTES           "TXBytes"
   #define CMD_USR_SYSTEM_TX_ERRORS          "TXErrors"
   #define CMD_USR_SYSTEM_TX_DROPS           "TXDrops"

   #define CMD_USR_SYSTEM_CALENDAR_TIME      "CalendarTime"



   #define CMD_RESOURCE_NUM                  15
   #define CMD_MB_SIZE                       ( 1024*1024 )
   #define CMD_DISK_SRC_FILE                 "/etc/mtab"

   #define CMD_DISK_IGNORE_TYPE_PROC         "proc"
   #define CMD_DISK_IGNORE_TYPE_SYSFS        "sysfs"
   #define CMD_DISK_IGNORE_TYPE_BINFMT_MISC  "binfmt_misc"
   #define CMD_DISK_IGNORE_TYPE_DEVPTS       "devpts"
   #define CMD_DISK_IGNORE_TYPE_FUSECTL      "fusectl"
   #define CMD_DISK_IGNORE_TYPE_SECURITYFS   "securityfs"
   #define CMD_DISK_IGNORE_TYPE_GVFS         "fuse.gvfs-fuse-daemon"

   #define CMD_USR_SYSTEM_IP                 "Ip"
   #define CMD_USR_SYSTEM_HOSTS              "Hosts"
   #define CMD_USR_SYSTEM_HOSTNAME           "HostName"

   #define CMD_USR_SYSTEM_PROC_USER          "user"
   #define CMD_USR_SYSTEM_PROC_PID           "pid"
   #define CMD_USR_SYSTEM_PROC_STATUS        "status"
   #define CMD_USR_SYSTEM_PROC_CMD           "cmd"
   #define CMD_USR_SYSTEM_LOGINUSER_USER     "user"
   #define CMD_USR_SYSTEM_LOGINUSER_FROM     "from"
   #define CMD_USR_SYSTEM_LOGINUSER_TTY      "tty"
   #define CMD_USR_SYSTEM_LOGINUSER_TIME     "time"
   #define CMD_USR_SYSTEM_ALLUSER_USER       "user"
   #define CMD_USR_SYSTEM_ALLUSER_GID        "gid"
   #define CMD_USR_SYSTEM_ALLUSER_DIR        "dir"
   #define CMD_USR_SYSTEM_GROUP_NAME         "name"
   #define CMD_USR_SYSTEM_GROUP_GID          "gid"
   #define CMD_USR_SYSTEM_GROUP_MEMBERS      "members"

   enum USRSYSTEM_HOST_LINE_TYPE
   {
      LINE_HOST         = 1,
      LINE_UNKNONW,
   } ;

   struct _usrSystemHostItem
   {
      INT32    _lineType ;
      string   _ip ;
      string   _com ;
      string   _host ;

      _usrSystemHostItem()
      {
         _lineType = LINE_UNKNONW ;
      }

      string toString() const
      {
         if ( LINE_UNKNONW == _lineType )
         {
            return _ip ;
         }
         string space = "    " ;
         if ( _com.empty() )
         {
            return _ip + space + _host ;
         }
         return _ip + space + _com + space + _host ;
      }
   } ;
   typedef _usrSystemHostItem usrSystemHostItem ;

   typedef vector< usrSystemHostItem >    VEC_HOST_ITEM ;

}
#endif
