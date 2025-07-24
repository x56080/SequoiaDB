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

   Source File Name = unorderedRidSet.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_UNORDERED_RID_SET_H_
#define VESSEL_UNORDERED_RID_SET_H_

#include "vessel/recordID.h"
#include "ossMemPool.hpp"

namespace engine
{
namespace vessel
{
   struct ridHash
   {
      OSS_INLINE std::size_t operator()(const recordID &rid)const
      {
         return rid.hash();
      }
   };//struct ridHash
   typedef ossPoolUnorderedSet<recordID, ridHash> UNORDERED_RID_SET;
} // namespace vessel
} // namespace engin

#endif//VESSEL_UNORDERED_RID_MAP_H_