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

   Source File Name = lsmCompactionFilter.hpp

   Descriptive Name = 

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/16/2021  LYC  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_LSM_COMPACTION_FILTER_H_
#define VESSEL_LSM_COMPACTION_FILTER_H_

#include "oss.hpp"
#include "rocksdb/compaction_filter.h"
#include "vessel/lsm/lsmIdxKey.hpp"
#include "vessel/lsm/lsmIndexValue.hpp"
#include "../bson/util/builder.h"

using namespace rocksdb;

namespace engine
{
namespace vessel
{
class lsmCompactionFilter : public CompactionFilter
{
   public:
      bool Filter(INT32 level,
                  const Slice& key,
                  const Slice& existing_value,
                  std::string* new_value,
                  bool* value_changed) const override;

      const CHAR* Name() const override{return "sdb.LsmCompactionFilter";}
   
   private:
      mutable bson::StackBufBuilder _cachedFullKeyBuilder;
      mutable lsmIndexValue _cachedValue;
      mutable UINT32 _invalidCount = 0;
};

class lsmCompactionFilterFactory : public CompactionFilterFactory
{
   public:
      unique_ptr<CompactionFilter> CreateCompactionFilter(
                     const CompactionFilter::Context& context) override
      {
         return unique_ptr<CompactionFilter>(new lsmCompactionFilter());
      }

      virtual const CHAR* Name() const override
      {
         return "sdb.LsmCompactionFilterFactory";
      }

};

extern shared_ptr<CompactionFilterFactory> createCompactionFilterFactory();

}
}
#endif