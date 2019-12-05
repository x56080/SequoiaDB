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

   Source File Name = tpCBCommon.hpp

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

#ifndef TP_CB_COMMON_HPP__
#define TP_CB_COMMON_HPP__

#include "oss.hpp"
#include "pd.hpp"
#include "tpCommon.hpp"
#include "tpToolCommon.hpp"

namespace engine
{

   #define TP_INVALID_TIMERID ( 0 )

   #define TP_GROUP_INVALID_VERSION      ( 0 )
   #define TP_GROUP_STANDALONE_VERSION   ( 1 )
   #define TP_GROUP_INIT_VERSION         ( 2 )

   #define TP_TIME_ERROR_ADJUST_STEP ( 0.1 )

   #define TP_ROLE_MASK_UNKNOWN       0x00000000
   #define TP_ROLE_MASK_STANDALONE    0x00000001
   #define TP_ROLE_MASK_CLIENT        0x00000002
   #define TP_ROLE_MASK_SERVER        0x00000004
   #define TP_ROLE_MASK_ALL           0xFFFFFFFF

   #define TP_SYNC_TIME_FLAG_EMPTY                    ( 0x0000 )
   #define TP_SYNC_TIME_FLAG_INCTIMEERROR             ( 0x0001 )
   #define TP_SYNC_TIME_FLAG_DECTIMEERROR             ( 0x0002 )
   #define TP_SYNC_TIME_FLAG_PUSHTIME                 ( 0x0004 )

   OSS_INLINE UINT32 tpGetRoleMask( TP_ROLE role )
   {
      switch ( role )
      {
         case TP_ROLE_STANDALONE :
            return TP_ROLE_MASK_STANDALONE ;
         case TP_ROLE_SERVER :
            return TP_ROLE_MASK_SERVER ;
         case TP_ROLE_CLIENT :
            return TP_ROLE_MASK_CLIENT ;
         default :
            break ;
      }
      return TP_ROLE_MASK_UNKNOWN ;
   }

   class _tpCB ;
   typedef class _tpCB SDB_TPCB ;

   class _tpNetMsgHandler ;
   typedef class _tpNetMsgHandler tpNetMsgHandler ;

   class _tpPipeMsgHandler ;
   typedef class _tpPipeMsgHandler tpPipeMsgHandler ;

   class _tpServiceManager ;
   typedef class _tpServiceManager tpServiceManager ;

   class _tpCatalogManager ;
   typedef class _tpCatalogManager tpCatalogManager ;

   class _tpMetaManager ;
   typedef class _tpMetaManager tpMetaManager ;

   class _tpSourceManager ;
   typedef class _tpSourceManager tpSourceManager ;

   class _tpSyncManager ;
   typedef class _tpSyncManager tpSyncManager ;

   class _tpReplManager ;
   typedef class _tpReplManager tpReplManager ;

   #define TP_MODULE_NAME             "TP_MODULE"
   #define TP_NET_MSG_HANDLER_NAME    "TP_NET_MSG_HANDLER"
   #define TP_PIPE_MSG_HANDLER_NAME   "TP_PIPE_MSG_HANDLER"
   #define TP_SERVICE_MANAGER_NAME    "TP_SERVICE_MANAGER"
   #define TP_CATALOG_MANAGER_NAME    "TP_CATALOG_MANAGER"
   #define TP_META_MANAGER_NAME       "TP_META_MANAGER"
   #define TP_SOURCE_MANAGER_NAME     "TP_SOURCE_MANAGER"
   #define TP_SYNC_MANAGER_NAME       "TP_SYNC_MANAGER"
   #define TP_REPL_MANAGER_NAME       "TP_REPL_MANAGER"

}

#endif // TP_CB_COMMON_HPP__
