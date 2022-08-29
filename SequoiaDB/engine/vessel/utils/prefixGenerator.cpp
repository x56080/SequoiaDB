/*******************************************************************************


   Copyright (C) 2011-2019 SequoiaDB Ltd.

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

   Source File Name = prefixGenerator.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains declares for all memory
   allocation/free operations.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/17/2022  ZHY Initial Draft

   Last Changed =

*******************************************************************************/

#include "vessel/prefixGenerator.h"
#include "ossTypes.h"
#include "pd.hpp"
#include <algorithm>
namespace engine
{
namespace vessel
{
   ossPoolVector<slice> prefixGenerator::_preprocess(
       const ossPoolVector<slice> &v, UINT32 &totalSize) const
   {
      ossPoolVector<slice> out;
      out.reserve(v.size());
      SDB_ASSERT(v.size() > 1, "Unexpected size");
      ossPoolVector<slice>::const_iterator left = v.cbegin();
      ossPoolVector<slice>::const_iterator cur = v.cbegin() + 1;
      ossPoolVector<slice>::const_iterator right = v.cbegin() + 2;

      slice prefix = left->commonPrefix(*cur);
      UINT32 prefixLen = prefix.size();
      out.emplace_back(prefixLen, prefix.data());
      totalSize += left->size();

      while (right != v.cend())
      {
         prefixLen = std::max(cur->commonPrefix(*left).size(),
                              cur->commonPrefix(*right).size());
         out.emplace_back(prefixLen, cur->data());
         totalSize += cur->size();
         left++;
         cur++;
         right++;
      }
      totalSize += cur->size();
      prefixLen = left->commonPrefix(*cur).size();
      out.emplace_back(prefixLen, cur->data());
      return std::move(out);
   }

   INT64 prefixGenerator::_dfs(
       const ossPoolVector<slice> &v,
       UINT32 pos,
       UINT32 depth,
       UINT32 &reachedPos,
       ossPoolVector<prefixGenerator::prefixItem> &out) const
   {
      INT64 saved = 0;
      UINT32 extraSize = _options.prefixExtraCost;
      const slice &curElem = v[pos];
      // When the position is greater than the farthest position, update it.
      if (reachedPos < pos)
      {
         reachedPos = pos;
      }
      // Recursion ends
      if (pos == v.size())
      {
         return 0;
      }
      // If there is no prefix in out, add a new prefix.
      if (out.empty())
      {
         out.emplace_back(curElem, 0, 1);
         saved += _dfs(v, pos + 1, depth, reachedPos, out);
      }
      // If current element is an empty string.
      else if (0 == curElem.size())
      {
         // And if the last prefix is also empty string, use last prefix. Update
         // the high bound of last prefix.
         if (0 == out.back().prefix.size())
         {
            out.back().high += 1;
         }
         // Else it is impossible to extract common prefix between last prefix
         // and current element. Add a new prefix, which is empty string.
         else
         {  
            INT32 nextLow = out.back().high;
            out.emplace_back(curElem, nextLow, nextLow + 1);
         }
         saved += _dfs(v, pos + 1, depth, reachedPos, out);
      }
      // Current element is not an empty string.
      else
      {
         const slice &lastString = out.back().prefix;
         // If current element is same as last prefix, use last prefix. Update
         // the high bound of last prefix.
         if (lastString.equal(curElem))
         {
            UINT32 oldSaved = out.back().savedBytesWithExtra(extraSize);
            out.back().high += 1;
            // Calculate the size difference.
            saved =
                saved - oldSaved + out.back().savedBytesWithExtra(extraSize);
            saved += _dfs(v, pos + 1, depth, reachedPos, out);
         }
         // If current element is not same as last prefix, try to extract common
         // prefix between them.
         else
         {
            slice prefix = lastString.commonPrefix(curElem);
            UINT32 prefixLen = prefix.getSize();
            // Do not have common prefix, add a new prefix.
            if (0 == prefixLen)
            {
               INT32 nextLow = out.back().high;
               out.emplace_back(curElem, nextLow, nextLow + 1);
               saved += _dfs(v, pos + 1, depth, reachedPos, out);
            }
            // Do have common prefix, we have two choices:
            // 1. Uncombined case: add a new prefix.
            // 2. Combined case: use last prefix.
            // Calculate the saved bytes of two choices.
            // In order to reduce the number of prefixes with bad benefits, the
            // saved bytes of combined case will be added a weight, and compare
            // it with the saved bytes of uncombined case. Finally choose the
            // greater one. After make a choice, the depth minus 1.
            // The combined weight = (the reference count of combined prefix) *
            // (weight factor). The factor is configurable.
            if (0 != prefixLen && 0 != depth)
            {  
               // 1. Uncombined case
               ossPoolVector<prefixGenerator::prefixItem> tempOut(out);
               INT32 nextLow = out.back().high;
               UINT32 posToAssign = out.size() - 1;
               tempOut.emplace_back(curElem, nextLow, nextLow + 1);
               INT64 saved1 = saved;
               saved1 += _dfs(v, pos + 1, depth - 1, reachedPos, tempOut);
               // 2. Combined case
               UINT32 oldSaved = out.back().savedBytesWithExtra(extraSize);
               out.back().prefix.reset(prefixLen, out.back().prefix.data());
               out.back().high += 1;
               INT64 saved2 = saved;
               saved2 = saved2 - oldSaved +
                        out.back().savedBytesWithExtra(extraSize);
               saved2 += _dfs(v, pos + 1, depth - 1, reachedPos, out);

               // Compare with two cases. The combined case will add weight.
               if (saved1 > saved2 + _options.combinedWeightFactor *
                                         out.back().getRefCount())
               {
                  out.erase(out.begin() + posToAssign, out.end());
                  out.insert(out.begin() + posToAssign,
                             tempOut.begin() + posToAssign,
                             tempOut.end());
                  saved = saved1;
               }
               else
               {
                  saved = saved2;
               }
            }
         }
      }
      return saved;
   }

   void prefixGenerator::setOptions(const options &o)
   {
      _options = o;
   }

   prefixGenerator::result prefixGenerator::generate(
       const ossPoolVector<slice> &v) const
   {  
      ossPoolVector<prefixItem> out;
      out.clear();
      if(v.size() < 2)
      {
         UINT32 totalSize = v.size() == 0 ? 0: v[0].size();
         result r(_options.prefixExtraCost, totalSize, 0, std::move(out));
      }
      UINT32 totalSize = 0;
      ossPoolVector<slice> prefixItems = _preprocess(v, totalSize);
      UINT32 reachedIndex = 0;
      INT64 saved = 0;
      INT64 totalSavedSize = 0;
      // Repeatedly until all elements are traversed.
      while (reachedIndex < v.size())
      {
         saved += _dfs(prefixItems,
                       reachedIndex,
                       _options.maxTreeDepth,
                       reachedIndex,
                       out);
      }
      // Remove prefixes that do not save bytes.
      out.erase(std::remove_if(out.begin(),
                               out.end(),
                               [&](const prefixItem &item) -> BOOLEAN {
                                  if (item.savedBytes() <=
                                      _options.prefixExtraCost)
                                  {
                                     return TRUE;
                                  }
                                  else
                                  {
                                     totalSavedSize += item.savedBytesWithExtra(
                                         _options.prefixExtraCost);
                                     return FALSE;
                                  }
                               }),
                out.end());

      SDB_ASSERT(totalSavedSize == saved, "should be equal");
      result r(_options.prefixExtraCost, totalSize, saved, std::move(out));
      return std::move(r);
   }

   prefixGenerator::result::result(UINT32 prefixExtraCost,
                                   UINT32 originalSize,
                                   UINT32 totalSavedSize,
                                   ossPoolVector<prefixItem> &&prefixes)
       : prefixExtraCost(prefixExtraCost), originalSize(originalSize),
         totalSavedSize(totalSavedSize), prefixes(std::move(prefixes))
   {
   }

   prefixGenerator::result::result(prefixGenerator::result &&r)
       : prefixExtraCost(r.prefixExtraCost), originalSize(r.originalSize),
         totalSavedSize(r.totalSavedSize), prefixes(std::move(r.prefixes))
   {
   }

   UINT32 prefixGenerator::result::getTotalSavedSize() const
   {
      return totalSavedSize;
   }

   UINT32 prefixGenerator::result::getTotalSavedSizeWithoutExtraCost() const
   {
      return totalSavedSize + prefixes.size() * prefixExtraCost;
   }

   FLOAT64 prefixGenerator::result::getCompressionRatio() const
   {
      return FLOAT64(totalSavedSize) / FLOAT64(originalSize);
   }
} // namespace vessel
} // namespace engine