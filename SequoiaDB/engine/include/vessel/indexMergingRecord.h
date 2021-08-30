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

namespace engine
{
namespace vessel
{
   struct indexMergingRecord : public _utilPooledObject
   {
      indexMergingRecord(){}
      ~indexMergingRecord(){}
      indexMergingRecord(const indexMergingRecord &) = delete;
      indexMergingRecord &operator=(const indexMergingRecord &) = delete;

      scanEntry entry;
      PAGE_ID lpid = INVALID_PAGE_ID;
      DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET;
      DPS_TRANS_ID transID;
      ossPoolList<bson::BSONObj> inserting;
      ossPoolList<bson::BSONObj> discarded;
   };//struct indexMergingRecord

   struct indexMergingRecordList : public SDBObject
   {
      indexMergingRecordList(){}
      ~indexMergingRecordList()
      {
         clear();
      }

      indexMergingRecordList(const indexMergingRecordList &) = delete;
      indexMergingRecordList &operator=(const indexMergingRecordList &) = delete;

      typedef ossPoolList<indexMergingRecord *> IDX_MERGING_RECORD_LIST;

      void clear()
      {
         IDX_MERGING_RECORD_LIST::const_iterator itr = rl.begin();
         for (; itr != rl.end(); ++itr)
         {
            if (NULL != *itr)
            {
               SDB_OSS_DEL *itr;
            }
         }
         rl.clear();
      }

      
      IDX_MERGING_RECORD_LIST rl;

   };//struct indexMergingRecordList
} // namespace vessel

} // namespace engine


#endif//VESSEL_INDEX_MERGING_RECORD_H_

