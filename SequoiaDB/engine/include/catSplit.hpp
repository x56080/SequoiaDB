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

   Source File Name = catSplit.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          19/07/2013  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef CAT_SPLIT_HPP__
#define CAT_SPLIT_HPP__

#include "core.hpp"
#include "pd.hpp"
#include "oss.hpp"
#include "ossErr.h"
#include "../bson/bson.h"
#include "catDef.hpp"
#include "pmd.hpp"
#include "clsTask.hpp"

using namespace bson ;

namespace engine
{

   INT32 catSplitPrepare( const BSONObj &splitInfo, const CHAR *clFullName,
                          clsCatalogSet *cataSet, UINT32 &groupID,
                          pmdEDUCB *cb ) ;

   INT32 catSplitReady( const BSONObj &splitInfo, const CHAR *clFullName,
                        clsCatalogSet *cataSet, UINT32 &groupID,
                        clsTaskMgr &taskMgr, pmdEDUCB *cb, INT16 w,
                        UINT64 *pTaskID = NULL ) ;

   INT32 catSplitStart( const BSONObj &splitInfo, pmdEDUCB *cb, INT16 w ) ;

   INT32 catSplitChgMeta( const BSONObj &splitInfo, const CHAR *clFullName,
                          clsCatalogSet *cataSet, pmdEDUCB *cb, INT16 w ) ;

   INT32 catSplitCleanup( const BSONObj &splitInfo, pmdEDUCB *cb, INT16 w ) ;

   INT32 catSplitFinish( const BSONObj &splitInfo, pmdEDUCB *cb, INT16 w ) ;

   INT32 catSplitCancel( const BSONObj &splitInfo, pmdEDUCB *cb,
                         UINT32 &groupID, INT16 w ) ;

   INT32 catSplitCheckConflict( BSONObj &match, clsSplitTask &splitTask,
                                BOOLEAN &conflict, pmdEDUCB *cb ) ;

}


#endif //CAT_SPLIT_HPP__

