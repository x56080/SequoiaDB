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

   Source File Name = indexMergingRecord.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_INDEX_MERGING_RECORD_H_
#define VESSEL_INDEX_MERGING_RECORD_H_

#include "vessel/scanEntry.h"
#include "utilPooledObject.hpp"
#include "ossMemPool.hpp"
#include "../bson/bson.hpp"
#include "dpsDef.hpp"

#include <memory>

namespace engine
{
namespace vessel
{
   struct indexMergingRecord : public _utilPooledObject
   {
      indexMergingRecord() = default;
      ~indexMergingRecord() = default;
      indexMergingRecord(const indexMergingRecord &) = delete;
      indexMergingRecord &operator=(const indexMergingRecord &) = delete;

      recordID rid;
      DPS_TRANS_ID transID;
      DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET;
      ossPoolList<bson::BSONObj> inserting;
      ossPoolList<bson::BSONObj> discarded;
   };//struct indexMergingRecord

   using INDEX_MERGING_RECORD = std::unique_ptr<indexMergingRecord>;
   using INDEX_MERGING_LIST = ossPoolList<INDEX_MERGING_RECORD>;
} // namespace vessel

} // namespace engine


#endif//VESSEL_INDEX_MERGING_RECORD_H_

