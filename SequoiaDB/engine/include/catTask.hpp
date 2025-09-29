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

   Source File Name = catTask.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          19/07/2013  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef CAT_TASK_HPP__
#define CAT_TASK_HPP__

#include "core.hpp"
#include "pd.hpp"
#include "oss.hpp"
#include "ossErr.h"
#include "../bson/bson.h"
#include "catDef.hpp"
#include "pmd.hpp"
#include "clsTask.hpp"
#include "catLevelLock.hpp"

using namespace bson ;

namespace engine
{

   INT32 catSplitPrepare ( const BSONObj &splitInfo, pmdEDUCB *cb,
                           UINT32 &returnGroupID, INT32 &returnVersion ) ;

   INT32 catSplitReady ( const BSONObj &splitInfo, UINT64 taskID,
                         BOOLEAN needLock, pmdEDUCB *cb, INT16 w,
                         UINT32 &returnGroupID, INT32 &returnVersion ) ;

   INT32 catSplitChgMeta ( const BSONObj &splitInfo, UINT64 taskID,
                           pmdEDUCB * cb, INT16 w ) ;

   INT32 catSplitCleanup ( UINT64 taskID, pmdEDUCB *cb, INT16 w ) ;

   INT32 catSplitFinish ( UINT64 taskID, pmdEDUCB *cb, INT16 w ) ;

   INT32 catTaskStart ( const BSONObj &boQuery, pmdEDUCB *cb, INT16 w ) ;

   INT32 catTaskCancel ( const BSONObj &boQuery, pmdEDUCB *cb,
                         INT16 w, UINT32 &returnGroupID ) ;
}


#endif // CAT_TASK_HPP__

