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

   Source File Name = slice.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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
