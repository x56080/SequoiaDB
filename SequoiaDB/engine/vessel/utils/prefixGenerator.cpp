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
namespace engine
{
namespace vessel
{
   ossPoolVector<slice> prefixGenerator::_extractTwo(
       const ossPoolVector<slice> &v, UINT32 &totalSize) const
   {
      ossPoolVector<slice> out;
      SDB_ASSERT(v.size() > 1, "Unexpected size");
      auto left = v.cbegin();
      auto cur = v.cbegin() + 1;
      auto right = v.cbegin() + 2;

      slice prefix = left->commonPrefix(*cur);
      UINT32 prefixLen = prefix.size();
      out.emplace_back(prefixLen, prefix.data());
      totalSize += left->size();

      while (right != v.cend())
      {
         UINT32 prefixLen = std::max(cur->commonPrefix(*left).size(),
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

      totalSize += v.size() * _options.itemExtraCost;
      return out;
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
      const auto &curElem = v[pos];
      if (reachedPos < pos)
      {
         reachedPos = pos;
      }
      if (pos == v.size())
      {
         return 0;
      }
      if (out.empty())
      {
         out.emplace_back(curElem, 0, 1);
         saved += _dfs(v, pos + 1, depth, reachedPos, out);
      }
      else if (0 == curElem.size())
      {
         INT32 nextLow = out.back().high;
         if (0 == out.back().prefix.size())
         {
            out.back().high += 1;
         }
         else
         {
            out.emplace_back(curElem, nextLow, nextLow + 1);
         }
         saved += _dfs(v, pos + 1, depth, reachedPos, out);
      }
      else
      {
         const auto &lastString = out.back().prefix;
         if (lastString.equal(curElem))
         {
            UINT32 oldSaved = out.back().savedBytesWithExtra(extraSize);
            out.back().high += 1;
            saved =
                saved - oldSaved + out.back().savedBytesWithExtra(extraSize);
            // else pass
            saved += _dfs(v, pos + 1, depth, reachedPos, out);
         }
         else
         {
            slice prefix = lastString.commonPrefix(curElem);
            UINT32 prefixLen = prefix.getSize();
            if (0 == prefixLen)
            {
               INT32 nextLow = out.back().high;
               out.emplace_back(curElem, nextLow, nextLow + 1);
               saved += _dfs(v, pos + 1, depth, reachedPos, out);
            }
            if (0 != prefixLen && 0 != depth)
            {
               ossPoolVector<prefixGenerator::prefixItem> tempOut(out);
               INT32 nextLow = out.back().high;
               UINT32 posToAssign = out.size() - 1;
               tempOut.emplace_back(curElem, nextLow, nextLow + 1);
               INT64 saved1 = saved;
               saved1 += _dfs(v, pos + 1, depth - 1, reachedPos, tempOut);

               UINT32 oldSaved = out.back().savedBytesWithExtra(extraSize);
               out.back().prefix.reset(prefixLen, out.back().prefix.data());
               out.back().high += 1;
               INT64 saved2 = saved;
               saved2 = saved2 - oldSaved +
                        out.back().savedBytesWithExtra(extraSize);
               saved2 += _dfs(v, pos + 1, depth - 1, reachedPos, out);

               if (saved1 > saved2 + _options.combinedWeightFactor *
                                         out.back().getRefCount())
               {
                  out.erase(out.begin() + posToAssign, out.end());
                  out.insert(out.begin() + posToAssign,
                             tempOut.begin() + posToAssign,
                             tempOut.end());
                  // out.assign(tempOut.begin(), tempOut.end());
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

   prefixGenerator::resultStat prefixGenerator::generate(
       const ossPoolVector<slice> &v, ossPoolVector<prefixItem> &out) const
   {
      out.clear();
      UINT32 totalSize = 0;
      ossPoolVector<slice> prefixItems = _extractTwo(v, totalSize);
      UINT32 reachedIndex = 0;
      INT64 saved = 0;
      UINT32 validPrefixItemNum = 0;
      INT64 totalSavedSize = 0;
      while (reachedIndex < v.size())
      {
         saved += _dfs(prefixItems,
                       reachedIndex,
                       _options.maxExponent,
                       reachedIndex,
                       out);
      }
      for (auto item : out)
      {
         item.hasPrefix(_options.prefixExtraCost);
         validPrefixItemNum += 1;
         totalSavedSize += item.savedBytesWithExtra(_options.prefixExtraCost);
      }
      SDB_ASSERT(totalSavedSize == saved, "should be equal");
      return {static_cast<UINT32>(saved),
              totalSize,
              FLOAT64(saved) / FLOAT64(totalSize),
              validPrefixItemNum};
   }
} // namespace vessel
} // namespace engine