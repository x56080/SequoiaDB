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

   Source File Name = rdpAccessor.h

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_RDP_ACCESSOR_H_
#define VESSEL_RDP_ACCESSOR_H_

#include "vessel/pageAccessor.h"
#include "vessel/recordData.h"
#include "vessel/strSlice.h"

namespace engine
{
namespace vessel
{
   class rdpAccessor : public pageAccessor
   {
      public:
         rdpAccessor();
         virtual ~rdpAccessor();
      public:
         INT32 initRdp(PAGE_ID lpid,
                       UINT32 logicalID,
                       UINT32 sequence);

         INT32 setCLInfo(UINT32 logicalID,
                         utilCLUniqueID uniqueID,
                         const CHAR *csName,
                         const CHAR *clName);

         INT32 insert(const recordData &record,
                      UTIL_COMPRESSOR_TYPE compressionType,
                      const DPS_TRANS_ID &transID,
                      STRIPING_ID striping,
                      const insertOptions &options);

      private:
         BOOLEAN hasEnoughFreeSpace(const recordDataPageHead *head,
                                    UINT32 recordSize,
                                    BOOLEAN &needReorg);
         UINT32 getMiniFreeSizeOfNormalRecord(UINT32 recordSize);

         BOOLEAN findFreeSlot(const recordDataPageHead *head,
                              UINT16 &slot);
      private:
         UINT32 _clLogicalID;
         utilCLUniqueID _uniqueID;
         strSlice _csName;
         strSlice _clName;
   };//class rdpAccessor
}//namespace vessel
}//namespace engine

#endif//VESSEL_RDP_ACCESSOR_H_