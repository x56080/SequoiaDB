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

   Source File Name = rtn.hpp

   Descriptive Name = RunTime Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains declare for runtime
   functions.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/08/2022  YSD  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_OPRHANDLER_HPP_
#define RTN_OPRHANDLER_HPP_

#include "dmsOprHandler.hpp"

namespace engine
{
   class _IRtnOprHandler : public IDmsOprHandler
   {
      public:
         _IRtnOprHandler() {}
         virtual ~_IRtnOprHandler() {}

      public:
         virtual INT32 getShardingKey( const CHAR* clName,
                                       BSONObj &shardingKey ) = 0 ;
   } ;
   typedef _IRtnOprHandler IRtnOprHandler ;
}

#endif /* RTN_OPRHANDLER_HPP_ */
