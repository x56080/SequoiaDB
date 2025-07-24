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

   Source File Name = ossEnvironment.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OSS_ENVIRONMENT_HPP__
#define OSS_ENVIRONMENT_HPP__

#include "ossIO.hpp"
#include "ossUtil.h"

namespace engine
{
   class _ossEnvironment : public SDBObject
   {
      public:
         INT32 extendFile(OSSFILE &file, UINT64 incrementSize);

      
      public:
         static BOOLEAN isFAllocBanned()
         {
            return OSS_BIT_TEST(_FILE_FLAGS, _FILE_FLAG_FALLOC_BANNED);
         }

      private:
         static constexpr UINT32 _FILE_FLAG_FALLOC_BANNED = 0x01;
         static UINT32 _FILE_FLAGS;
   };
   using ossEnv = class _ossEnvironment;
} // namespace engine


#endif//OSS_ENVIRONMENT_HPP__