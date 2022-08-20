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
         BOOLEAN isWorthToSave(UINT32 extraSize) const
         {
            return savedBytes() > extraSize;
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
      };

      struct options
      {
         UINT32 prefixExtraCost = 0;
         UINT32 itemExtraCost = 0;
         UINT32 maxExponent = 4;
         UINT32 combinedWeightFactor = 1;
         options() = default;
         options(UINT32 prefixExtraCost,
                 UINT32 itemExtraCost,
                 UINT32 maxExponent,
                 UINT32 combinedWeightFactor)
             : prefixExtraCost(prefixExtraCost), itemExtraCost(itemExtraCost),
               maxExponent(maxExponent),
               combinedWeightFactor(combinedWeightFactor)
         {
         }
      };
      struct resultStat
      {
         UINT32 totalSavedSize = 0;
         UINT32 originalSize = 0;
         FLOAT64 compressionRatio = 0.0;
         UINT32 validPrefixItemNum = 0;
         resultStat(UINT32 totalSavedSize,
                    UINT32 originalSize,
                    FLOAT64 compressionRatio,
                    UINT32 validPrefixItemNum)
             : totalSavedSize(totalSavedSize), originalSize(originalSize),
               compressionRatio(compressionRatio),
               validPrefixItemNum(validPrefixItemNum)
         {
         }
      };

   public:
      prefixGenerator() = default;

   public:
      void setOptions(const options &o);
      resultStat generate(const ossPoolVector<slice> &v,
                          ossPoolVector<prefixItem> &out) const;

   private:
      INT64 _dfs(const ossPoolVector<slice> &v,
                 UINT32 pos,
                 UINT32 depth,
                 UINT32 &reachedPos,
                 ossPoolVector<prefixItem> &out) const;
      ossPoolVector<slice> _extractTwo(const ossPoolVector<slice> &v,
                                       UINT32 &totalSize) const;

   private:
      options _options;
   };
} // namespace vessel
} // namespace engine

#endif