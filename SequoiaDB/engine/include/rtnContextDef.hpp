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

   Source File Name = rtnContextDef.hpp

   Descriptive Name = RunTime Context Define Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for Runtime
   Context.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/11/2021  HGM Initial draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_CONTEXT_DEF_HPP_
#define RTN_CONTEXT_DEF_HPP_

#include "ossTypes.hpp"

namespace engine
{

   #define RTN_CONTEXT_GETNUM_ONCE     (1000)

   // max context number in a node
   #define RTN_MAX_CTX_NUM_MIN         ( 1000 )
   #define RTN_MAX_CTX_NUM_MAX         ( OSS_SINT32_MAX )
   #define RTN_MAX_CTX_NUM_DFT         ( 100000 )

   // max context number in a session per node
   #define RTN_MAX_SESS_CTX_NUM_MIN    ( 10 )
   #define RTN_MAX_SESS_CTX_NUM_MAX    ( OSS_SINT32_MAX )
   #define RTN_MAX_SESS_CTX_NUM_DFT    ( 100 )

   // context idle timeout in minutes
   #define RTN_CTX_TIMEOUT_MIN         ( 0 )
   #define RTN_CTX_TIMEOUT_MAX         ( OSS_SINT32_MAX )
   // default is 30 minutes
   #define RTN_CTX_TIMEOUT_DFT         ( 30 )

   // 30 seconds ( in ms )
   #define RTN_CTX_CHECK_INTERVAL      ( 30000 )

}

#endif // RTN_CONTEXT_DEF_HPP_
