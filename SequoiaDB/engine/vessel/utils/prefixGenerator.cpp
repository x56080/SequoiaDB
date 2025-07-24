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
#include <iterator>
namespace engine
{
namespace vessel
{
   ossPoolVector<slice> prefixGenerator::_preprocess(
       const ossPoolVector<slice>::const_iterator &begin,
       const ossPoolVector<slice>::const_iterator &end,
       UINT32 &totalSize) const
   {
      totalSize = 0;
      UINT32 inputSize = std::distance(begin, end);
      ossPoolVector<slice> out;
      out.reserve(inputSize);
      SDB_ASSERT(inputSize > 1, "Unexpected size");
      ossPoolVector<slice>::const_iterator left = begin;
      ossPoolVector<slice>::const_iterator cur = begin + 1;
      ossPoolVector<slice>::const_iterator right = begin + 2;

      slice prefix = left->commonPrefix(*cur);
      UINT32 prefixLen = prefix.size();
      out.emplace_back(prefixLen, prefix.data());
      totalSize += left->size();

      while (right != end)
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
      SDB_ASSERT(out.size() == 0 ||
                     out.back().high ==
                         static_cast<INT32>(pos + _options.itemBoundaryOffset),
                 "invalid bound");
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
         out.emplace_back(curElem,
                          0 + _options.itemBoundaryOffset,
                          1 + _options.itemBoundaryOffset);
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
               UINT32 tempReachedPos1 = reachedPos;
               saved1 += _dfs(v, pos + 1, depth - 1, tempReachedPos1, tempOut);
               // 2. Combined case
               UINT32 oldSaved = out.back().savedBytesWithExtra(extraSize);
               out.back().prefix.reset(prefixLen, out.back().prefix.data());
               out.back().high += 1;
               INT64 saved2 = saved;
               saved2 = saved2 - oldSaved +
                        out.back().savedBytesWithExtra(extraSize);
               UINT32 tempReachedPos2 = reachedPos;
               saved2 += _dfs(v, pos + 1, depth - 1, tempReachedPos2, out);

               // Compare with two cases. The combined case will add weight.
               if (saved1 > saved2 + _options.combinedWeightFactor *
                                         out.back().getRefCount())
               {
                  out.erase(out.begin() + posToAssign, out.end());
                  out.insert(out.begin() + posToAssign,
                             tempOut.begin() + posToAssign,
                             tempOut.end());
                  reachedPos = tempReachedPos1;
                  saved = saved1;
               }
               else
               {
                  reachedPos = tempReachedPos2;
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
      return generate(v.cbegin(), v.cend());
   }

   prefixGenerator::result prefixGenerator::generate(
       const ossPoolVector<slice>::const_iterator &begin,
       const ossPoolVector<slice>::const_iterator &end) const
   {
      ossPoolVector<prefixItem> out;
      out.clear();
      UINT32 inputSize = std::distance(begin, end);
      if(inputSize < 2)
      {
         UINT32 totalSize = 0;
         if (inputSize == 1) 
         {
            out.emplace_back(slice(),
                             _options.itemBoundaryOffset,
                             1 + _options.itemBoundaryOffset);
            totalSize = begin->size();
         }
         result r(_options.prefixExtraCost, totalSize, 0, std::move(out));
         return std::move(r);
      }
      UINT32 totalSize = 0;
      ossPoolVector<slice> prefixItems = _preprocess(begin, end, totalSize);
      UINT32 reachedIndex = 0;
      INT64 saved = 0;
      
      // Repeatedly until all elements are traversed.
      while (reachedIndex < prefixItems.size())
      {
         saved += _dfs(prefixItems,
                       reachedIndex,
                       _options.maxTreeDepth,
                       reachedIndex,
                       out);
      }
      // Remove prefixes that do not save bytes.
      if (_options.filterUselessPrefixItems)
      {
         INT64 totalSavedSize = 0;
         out.erase(std::remove_if(
                       out.begin(),
                       out.end(),
                       [&](const prefixItem &item) -> BOOLEAN {
                          if (item.savedBytes() <= _options.prefixExtraCost)
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
      }

      
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