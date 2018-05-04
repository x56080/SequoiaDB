/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = clsBase.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          29/11/2012  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CLS_BASE_HPP_
#define CLS_BASE_HPP_

#include "core.hpp"
#include "msg.hpp"
#include "oss.hpp"
#include <vector>
#include <string>

namespace engine
{

   enum CLS_MEMBER_TYPE
   {
      CLS_SHARD   = 1,           //shard
      CLS_REPL    = 2            //repl
   };

   enum CLS_INNER_SESSION_TID
   {
      CLS_TID_REPL_SYC     = 1,  //repl active sync
      CLS_TID_REPL_FS_SYC  = 2,  //repl active full sync

      CLS_TID_END          = 10  //end
   };

   #define CLS_INVALID_TIMERID         (0)

   enum CLS_SYNC_STRATEGY
   {
      CLS_SYNC_NONE        = 0,
      CLS_SYNC_KEEPNORMAL  = 1,
      CLS_SYNC_KEEPALL     = 2
   } ;
   #define CLS_SYNC_DTF_STRATEGY    CLS_SYNC_KEEPNORMAL

   typedef MsgRouteID   NodeID ;
   #define INVALID_NODE_ID       (MSG_INVALID_ROUTEID)

   #define SAFE_DELETE(p) \
      do { \
         if ( p ) \
         { \
            SDB_OSS_DEL p ; \
            p = NULL ; \
         } \
      } while (0)

   #define SAFE_NEW_GOTO_ERROR(p, className) \
      do { \
         p = SDB_OSS_NEW className() ; \
         if ( !p ) \
         { \
            PD_LOG ( PDERROR, "Failed to allocate memory to #className" ) ; \
            rc = SDB_OOM ; \
            goto error ; \
         } \
      } while (0)

   #define SAFE_NEW_GOTO_ERROR1(p, className, arg1) \
      do { \
         p = SDB_OSS_NEW className( arg1 ) ; \
         if ( !p ) \
         { \
            PD_LOG ( PDERROR, "Failed to allocate memory to #className" ) ; \
            rc = SDB_OOM ; \
            goto error ; \
         } \
      } while (0)

   #define SAFE_NEW_GOTO_ERROR2(p, className, arg1, arg2) \
      do { \
         p = SDB_OSS_NEW className( arg1, arg2 ) ; \
         if ( !p ) \
         { \
            PD_LOG ( PDERROR, "Failed to allocate memory to #className" ) ; \
            rc = SDB_OOM ; \
            goto error ; \
         } \
      } while (0)

   #define INIT_OBJ_GOTO_ERROR(p) \
      do { \
         if ( SDB_OK != ( rc = p->initialize () ) ) \
         { \
            PD_LOG ( PDERROR, "init failed" ) ; \
            goto error ; \
         } \
      } while (0)

}

#endif //CLS_BASE_HPP_



