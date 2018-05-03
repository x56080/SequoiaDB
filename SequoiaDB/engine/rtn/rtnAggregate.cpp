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

   Source File Name = rtnAggregate.cpp

   Descriptive Name = Runtime Aggregate

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   aggregation on data node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/

#include "rtn.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"

using namespace bson;

namespace engine
{
   INT32 rtnAggregate( const CHAR *pCollectionName, BSONObj &objs,
                       INT32 objNum, SINT32 flags, pmdEDUCB *cb,
                       SDB_DMSCB *dmsCB, SINT64 &contextID )
   {
      INT32 rc = SDB_OK;
      rc = pmdGetKRCB()->getAggrCB()->build( objs, objNum, pCollectionName,
                                             cb, contextID );
      PD_RC_CHECK( rc, PDERROR,
                  "failed to execute aggregation operation(rc=%d)",
                  rc );

   done:
      return rc;
   error:
      goto done;
   }
}
