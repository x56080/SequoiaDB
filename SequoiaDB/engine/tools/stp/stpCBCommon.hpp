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

   Source File Name = stpCBCommon.hpp

   Descriptive Name = Serial Time Protocol common defines for STPCB

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
#ifndef STP_CB_COMMON_HPP__
#define STP_CB_COMMON_HPP__

#include "oss.hpp"
#include "pd.hpp"
#include "stpCommon.hpp"
#include "stpToolCommon.hpp"

namespace engine
{

   // invalid timer ID ( for net agent timer )
   #define STP_INVALID_TIMERID ( 0 )

   // invalid synchronize port
   #define STP_INVALID_SYNCPORT ( 0 )

   // invalid server group version
   #define STP_GROUP_INVALID_VERSION      ( 0 )
   // initial version of server group
   #define STP_GROUP_INIT_VERSION         ( 1 )

   // role masks for STPCB modules
   // indicate that a module could be used in a role
   // empty mask
   #define STP_ROLE_MASK_UNKNOWN       0x00000000
   // mark module used in test mode
   #define STP_ROLE_MASK_TESTMODE      0x00000001
   // mark module used in client role
   #define STP_ROLE_MASK_CLIENT        0x00000002
   // mark module used in server role
   #define STP_ROLE_MASK_SERVER        0x00000004
   // mark module used in all roles
   #define STP_ROLE_MASK_ALL           0xFFFFFFFF

   // convert STP role to STPCB module mask
   OSS_INLINE UINT32 tpGetRoleMask( BOOLEAN testMode, STP_ROLE role )
   {
      // test mode has higher priority than role
      if ( testMode )
      {
         return STP_ROLE_MASK_TESTMODE ;
      }
      switch ( role )
      {
         case STP_ROLE_SERVER :
            return STP_ROLE_MASK_SERVER ;
         case STP_ROLE_CLIENT :
            return STP_ROLE_MASK_CLIENT ;
         default :
            break ;
      }
      return STP_ROLE_MASK_UNKNOWN ;
   }

   // flags to notify synchronize source
   // tell the source to perform an action on a client
   // empty flag
   #define STP_SYNC_TIME_FLAG_EMPTY             ( 0x0000 )
   // tell the source to increase the time error of a client
   // NOTE: increase time error should be excluded with decrease time error
   #define STP_SYNC_TIME_FLAG_INCTIMEERROR      ( 0x0001 )
   // tell the source to decrease the time error of a client
   // NOTE: decrease time error should be excluded with increase time error
   #define STP_SYNC_TIME_FLAG_DECTIMEERROR      ( 0x0002 )
   // tell the source to push time forward
   #define STP_SYNC_TIME_FLAG_PUSHTIME          ( 0x0004 )

   // interval ( in milliseconds ) to clear expired synchronize clients or
   // sources without any synchronize in 2 hours )
   #define STP_CLEAR_SYNCHRONIZE_INTERVAL       \
                                 ( STP_SEC_TO_MILLISEC( ( 2 * 3600 ) ) )

   // interval to clear expired clients ( without synchronize in 2 hours )
   #define STP_CLEAR_CLIENT_INTERVAL   ( STP_CLEAR_SYNCHRONIZE_INTERVAL )

   // pre-declaration of classes

   // main control block of STP ( STPCB )
   class _stpCB ;
   typedef class _stpCB STPCB ;

   class _stpNetMsgHandlerBase ;
   typedef class _stpNetMsgHandlerBase stpNetMsgHandlerBase ;

   // synchronize source message handler
   class _stpSyncSourceMsgHandler ;
   typedef class _stpSyncSourceMsgHandler stpSyncSourceMsgHandler ;

   // net message handler
   class _stpNetMsgHandler ;
   typedef class _stpNetMsgHandler stpNetMsgHandler ;

   // pipe message handler
   class _stpPipeMsgHandler ;
   typedef class _stpPipeMsgHandler stpPipeMsgHandler ;

   // service manager handles sessions from client ( sdbshell )
   class _stpServiceManager ;
   typedef class _stpServiceManager stpServiceManager ;

   // node manager handles node info of servers, primary node, etc.
   class _stpNodeManager ;
   typedef class _stpNodeManager stpNodeManager ;

   // meta manager handles meta data, shared memory and meta LSN
   class _stpMetaManager ;
   typedef class _stpMetaManager stpMetaManager ;

   // synchronize source handles time synchronize as source
   class _stpSyncSource ;
   typedef class _stpSyncSource stpSyncSource ;

   // synchronize source manager manages time synchronize sources
   class _stpSyncSourceManager ;
   typedef class _stpSyncSourceManager stpSyncSourceManager ;

   // synchronize client manager handles time synchronize as client
   class _stpSyncClientManager ;
   typedef class _stpSyncClientManager stpSyncClientManager ;

   // replica manager handles replica votes between servers
   class _stpReplManager ;
   typedef class _stpReplManager stpReplManager ;

   // names of STPCB modules
   #define STP_MODULE_NAME                "STP_MODULE"
   #define STP_NET_MSG_HANDLER_NAME       "STP_NET_MSG_HANDLER"
   #define STP_PIPE_MSG_HANDLER_NAME      "STP_PIPE_MSG_HANDLER"
   #define STP_SERVICE_MANAGER_NAME       "STP_SERVICE_MANAGER"
   #define STP_NODE_MANAGER_NAME          "STP_NODE_MANAGER"
   #define STP_META_MANAGER_NAME          "STP_META_MANAGER"
   #define STP_SYNC_SOURCE_MANAGER_NAME   "STP_SYNC_SOURCE_MANAGER"
   #define STP_SYNC_CLIENT_MANAGER_NAME   "STP_SYNC_CLIENT_MANAGER"
   #define STP_REPL_MANAGER_NAME          "STP_REPL_MANAGER"

}

#endif // STP_CB_COMMON_HPP__
