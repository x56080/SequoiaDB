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

   Source File Name = prefixGenerator.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/17/2022  ZHY Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_PREFIX_GENERATOR_H_
#define VESSEL_PREFIX_GENERATOR_H_

#include "oss.hpp"
#include "ossMemPool.hpp"
#include "ossTypes.h"
#include "vessel/slice.h"
namespace engine
{
namespace vessel
{
   class prefixGenerator : public SDBObject
   {
   public:
      struct prefixItem
      {
         slice prefix;
         INT32 low = -1;
         INT32 high = -1;
         prefixItem(const slice &prefix, INT32 low, INT32 high)
             : prefix(prefix), low(low), high(high)
         {
         }
         UINT32 getRefCount() const
         {
            return high - low;
         }
         UINT32 savedBytes() const
         {
            if (high < 0 || low < 0)
            {
               return 0;
            }
            else if (high == low)
            {
               return 0;
            }
            return prefix.size() * (high - low - 1);
         }

         UINT32 savedBytesWithExtra(UINT32 extraCost) const
         {
            UINT32 s = savedBytes();
            return s > extraCost ? s - extraCost : 0;
         }

         BOOLEAN worthToUse(UINT32 extraCost) const
         {
            return savedBytes() > extraCost;
         }
      };

      struct options
      {
         static constexpr UINT32 DEFAULT_PREFIX_EXTRA_COST = 0;
         static constexpr UINT32 DEFAULT_MAX_TREE_DEPTH = 4;
         static constexpr UINT32 DEFAULT_COMBINED_WEIGHT_FACTOR = 1;
         static constexpr UINT32 DEFAULT_ITEM_BOUNDARY_OFFSET = 0;
         // The extra cost of generating a referenced prefix.
         UINT32 prefixExtraCost = DEFAULT_PREFIX_EXTRA_COST;
         // The maxinum depth of the solution space tree. It should not be too
         // large, otherwise it will significantly increase the time complexity.
         // The larger the value, the more accurate the solution can be achieved
         // partly.
         UINT32 maxTreeDepth = DEFAULT_MAX_TREE_DEPTH;
         // The combined case will add this factor relative to the uncombined
         // case. The larger the factor, the easier the combined case is to be
         // selected.
         UINT32 combinedWeightFactor = DEFAULT_COMBINED_WEIGHT_FACTOR;
         // Whether remove the prefixes which do not save bytes.
         BOOLEAN filterUselessPrefixItems = TRUE;
         // All low/high boundaries of generated prefixes will add the offset.
         INT32 itemBoundaryOffset = DEFAULT_ITEM_BOUNDARY_OFFSET;

         options(UINT32 prefixExtraCost = DEFAULT_PREFIX_EXTRA_COST,
                 UINT32 maxTreeDepth = DEFAULT_MAX_TREE_DEPTH,
                 UINT32 combinedWeightFactor = DEFAULT_COMBINED_WEIGHT_FACTOR,
                 BOOLEAN filterUselessPrefixItems = TRUE,
                 INT32 itemBoundaryOffset = DEFAULT_ITEM_BOUNDARY_OFFSET)
             : prefixExtraCost(prefixExtraCost),
               maxTreeDepth(maxTreeDepth),
               combinedWeightFactor(combinedWeightFactor),
               filterUselessPrefixItems(filterUselessPrefixItems),
               itemBoundaryOffset(itemBoundaryOffset)
         {
         }
      };

   public:
      prefixGenerator() = default;

   public:
      void setOptions(const options &o);
      
      class result
      {
      public:
         result(UINT32 prefixExtraCost,
                UINT32 originalSize,
                UINT32 totalSavedSize,
                ossPoolVector<prefixItem> &&prefixes);
         result(result &&r);
      
      public:
         // Return the number of bytes saved with considering the extra cost.
         UINT32 getTotalSavedSize() const;
         // Return the number of bytes saved without considering the extra cost.
         UINT32 getTotalSavedSizeWithoutExtraCost() const;
         // Return the ratio of total saved size and original size.
         FLOAT64 getCompressionRatio() const;

      public:
         // The extra cost of generating a referenced prefix.
         const UINT32 prefixExtraCost = options::DEFAULT_PREFIX_EXTRA_COST;
         // The total size of the entered keys.
         const UINT32 originalSize = 0;
         // The number of bytes saved with considering the extra cost.
         const UINT32 totalSavedSize = 0;
         // The output of generator, which only contains referenced prefixes.
         const ossPoolVector<prefixItem> prefixes;
      }; // class result

   public:
      result generate(const ossPoolVector<slice> &v) const;
      result generate(const ossPoolVector<slice>::const_iterator &begin,
                      const ossPoolVector<slice>::const_iterator &end) const;

   private:
      // Recursive function to compute partly optimal solution.
      INT64 _dfs(
          // Input to generate prefixes
          const ossPoolVector<slice> &v,
          // Current element position
          UINT32 pos,
          // Remaining depth
          UINT32 depth,
          // The farthest position of this recursion
          UINT32 &reachedPos,
          // The output of generator
          ossPoolVector<prefixItem> &out) const;
      
      // Preprocess the input. For every element, get the better one of prefixes
      // generated by the previous and next element.
      ossPoolVector<slice> _preprocess(
          const ossPoolVector<slice>::const_iterator &begin,
          const ossPoolVector<slice>::const_iterator &end,
          UINT32 &totalSize) const;

   private:
      options _options;
   };
} // namespace vessel
} // namespace engine

#endif