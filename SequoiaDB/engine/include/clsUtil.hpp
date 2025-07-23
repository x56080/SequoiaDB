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

   Source File Name = clsUitl.hpp

   Descriptive Name = Replication Control Block Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Replication component. This file contains structure for
   replication control block.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef CLSUTIL_HPP_
#define CLSUTIL_HPP_

#include <map>

#include "clsDef.hpp"
#include "netDef.hpp"
#include "clsBase.hpp"
#include "msgReplicator.hpp"

using namespace std ;

namespace engine
{

   CLS_SYNC_STATUS clsSyncWindow( const DPS_LSN &remoteLsn,
                                  const DPS_LSN &fileBeginLsn,
                                  const DPS_LSN &memBeginLSn,
                                  const DPS_LSN &endLsn ) ;

   INT32 clsString2Strategy( const CHAR *str, INT32 &sty ) ;

   INT32 clsStrategy2String( INT32 sty, CHAR *str, UINT32 len ) ;

   const CHAR*    clsNodeRunStat2String( INT32 stat ) ;
   const CHAR*    clsFSNotifyType2String( CLS_FS_NOTIFY_TYPE type ) ;

   void clsUpdateReplsize( UINT32 replsize ) ;
   UINT32 clsGetReplsize() ;

   void clsUpdateConsistencyStrategy( SDB_CONSISTENCY_STRATEGY consistencyStrategy ) ;
   SDB_CONSISTENCY_STRATEGY clsGetConsistencyStrategy() ;
}

#endif //CLSUTIL_HPP_

