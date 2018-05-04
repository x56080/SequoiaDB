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
*******************************************************************************/

#include "ossTypes.h"
#include <gtest/gtest.h>

#include "pmdEDU.hpp"
#include "ossErr.h"
#include "pmdEDUMgr.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"

using namespace engine;

TEST(catalogMainControllerTest, init)
{
   pmdEDUMgr *pEduMgr = pmdGetKRCB()->getEDUMgr();
   EDUID agentEDU = PMD_INVALID_EDUID;
   INT32 rc = pEduMgr->startEDU(EDU_TYPE_CATMGR, NULL, &agentEDU);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_NE(PMD_INVALID_EDUID, agentEDU);
   boost::xtime sleepTime;
   boost::xtime_get(&sleepTime, boost::TIME_UTC_);
   sleepTime.sec += 1;
   boost::thread::sleep(sleepTime);
   pmdGetKRCB()->destroy();
}
