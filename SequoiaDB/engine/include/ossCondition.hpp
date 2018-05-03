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

   Source File Name = ossMmap.cpp

   Descriptive Name = Operating System Services Memory Map

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains functions for Memory Mapping
   Files.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef OSSCONDITION_H_
#define OSSCONDITION_H_

#include "ossTypes.hpp"
#include <boost/thread/condition.hpp>
#include <boost/thread/mutex.hpp>

namespace engine
{
   typedef boost::condition_variable _ossCondition;
   typedef boost::mutex _ossConditionMutex;
}

#define QNIQUE_LOCK boost::unique_lock<boost::mutex>

#endif
