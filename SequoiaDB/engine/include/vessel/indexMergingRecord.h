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

   Source File Name = indexMergingRecord.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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

