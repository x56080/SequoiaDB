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

   Source File Name = rtnRemoteExec.hpp

   Descriptive Name = Remote Excuting Header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains declares for process op.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          2/27/2013  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTNREMOTEEXEC_HPP__
#define RTNREMOTEEXEC_HPP__

#include "core.hpp"
#include "../bson/bson.h"

using namespace bson ;

namespace engine
{

   INT32 rtnRemoteExec ( SINT32 remoCode,
                         const CHAR * hostname,
                         SINT32 *retCode,
                         const BSONObj *arg1 = NULL,
                         const BSONObj *arg2 = NULL,
                         const BSONObj *arg3 = NULL,
                         const BSONObj *arg4 = NULL,
                         std::vector<BSONObj> *retObjs = NULL ) ;

}

#endif /* RTNREMOTEEXEC_HPP__ */

