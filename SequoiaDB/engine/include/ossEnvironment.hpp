/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = ossEnvironment.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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