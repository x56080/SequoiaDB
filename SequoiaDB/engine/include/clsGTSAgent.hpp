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

   Source File Name = clsGTSAgent.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of Replication component. This file contains structure for
   replication control block.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/05/2012  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef CLS_GTS_AGENT_HPP__
#define CLS_GTS_AGENT_HPP__

#include "oss.hpp"
#include "dpsDef.hpp"
#include "ossMemPool.hpp"
#include "dpsTransDef.hpp"
#include "sdbInterface.hpp"

namespace engine
{
   class _clsShardMgr ;
   /*
      _clsGTSAgent define
   */
   class _clsGTSAgent : public SDBObject, public _dpsTransEvent
   {
      public:
         _clsGTSAgent( _clsShardMgr *pShardMgr ) ;
         ~_clsGTSAgent() ;

      public:
         virtual INT32  onRollbackAll() ;

      public:

         INT32       checkTransStatus( DPS_TRANS_ID transID,
                                       UINT32 nodeNum,
                                       const UINT64 *pNodes,
                                       IExecutor *cb,
                                       DPS_TRANS_STATUS &status ) ;

      protected:

         INT32       _checkTransStatus( DPS_TRANS_ID transID,
                                        UINT32 group,
                                        IExecutor *cb,
                                        DPS_TRANS_STATUS &status ) ;

         INT32       _syncCheckTransStatus( DPS_TRANS_ID transID,
                                            DPS_LSN_OFFSET curLsn,
                                            DPS_TRANS_STATUS &status ) ;

         INT32       _commitTrans( DPS_TRANS_ID transID,
                                   DPS_LSN_OFFSET lastLsn,
                                   DPS_LSN_OFFSET &curLsn ) ;

      private:
         _clsShardMgr         *_pShardMgr ;

   } ;
   typedef _clsGTSAgent clsGTSAgent ;

}


#endif // CLS_GTS_AGENT_HPP__

