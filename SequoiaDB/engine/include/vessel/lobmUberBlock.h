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

   Source File Name = lobmUberBlock.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_LOBM_UBER_BLOCK_H_
#define VESSEL_LOBM_UBER_BLOCK_H_

#include "vessel/pageIdentifier.h"

namespace engine
{
namespace vessel
{
   constexpr UINT32 LOBM_UBER_BLOCK_VERSION = 1;


   struct lobmUberBlock
   {
      lobmUberBlock &operator=(const lobmUberBlock &o)
      {
         version = o.version;
         totalLobdSegments = o.totalLobdSegments;
         lobdSmeEntryPid = o.lobdSmeEntryPid;
         bucketEntryPid = o.bucketEntryPid;
         return *this;
      }

      OSS_INLINE BOOLEAN isValid()const
      {
         return LOBM_UBER_BLOCK_VERSION == version &&
                INVALID_PAGE_ID != lobdSmeEntryPid &&
                INVALID_PAGE_ID != bucketEntryPid;
      }

      void reset()
      {
         version = 0;
         totalLobdSegments = 0;
         lobdSmeEntryPid = 0;
         bucketEntryPid = 0;
      }

      UINT32 version = 0;
      UINT32 totalLobdSegments = 0;
      UINT32 lobdSmeEntryPid = 0;
      UINT32 bucketEntryPid = 0;
   };//struct lobmUberBlock
} // namespace vessel

} // namespace engine


#endif//VESSEL_LOBM_UBER_BLOCK_H_
