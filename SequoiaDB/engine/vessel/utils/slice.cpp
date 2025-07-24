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

   Source File Name = slice.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/slice.h"

namespace engine
{
namespace vessel
{
   INT32 compareSlicePairs(const slice &lprefix, const slice &lsuffix,
                           const slice &rprefix, const slice rsuffix)
   {
      if (!lprefix.isValid() && !rprefix.isValid())
      {
         return lsuffix.compare(rsuffix);
      }
      else if (lprefix.isValid() && !rprefix.isValid())
      {
         return -rsuffix.compare(lprefix, lsuffix);
      }
      else if (!lprefix.isValid() && rprefix.isValid())
      {
         return lsuffix.compare(rprefix, rsuffix);
      }
      else if (lprefix.size() == rprefix.size())
      {
         INT32 res = lprefix.compare(rprefix);
         if (0 == res)
         {
            res = lsuffix.compare(rsuffix);
         }
         return res;
      }
      else
      {
         const slice *prefixA = nullptr;
         const slice *suffixA = nullptr;
         const slice *prefixB = nullptr;
         const slice *suffixB = nullptr;
         INT32 direction = 0;

         if (lprefix.size() < rprefix.size())
         {
            prefixA = &rprefix;
            suffixA = &rsuffix;
            prefixB = &lprefix;
            suffixB = &lsuffix;
            direction = -1;
         }
         else
         {
            prefixA = &lprefix;
            suffixA = &lsuffix;
            prefixB = &rprefix;
            suffixB = &rsuffix;
            direction = 1;
         }

         INT32 res = ossMemcmp(prefixA->data(), prefixB->data(), prefixB->size());
         if (0 != res)
         {
            return res * direction;
         }
         else
         {
            slice remainPrefixA(prefixA->size() - prefixB->size(),
                                prefixA->data() + prefixB->size());
            res -= suffixB->compare(remainPrefixA, *suffixA);
            return res * direction;
         }
      }
   }
} // namespace vessel

} // namespace engine
