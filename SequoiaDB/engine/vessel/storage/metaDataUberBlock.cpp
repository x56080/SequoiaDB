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

   Source File Name = metaDataUberBlock.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/metaDataUberBlock.h"
#include "pdTrace.hpp"
#include <boost/crc.hpp>

namespace engine
{
namespace vessel
{
   UINT32 lpmUberBlock::generateChecksum()const
   {
      boost::crc_32_type crc;
      crc.process_bytes(&smeEntryPid, sizeof(smeEntryPid));
      crc.process_bytes(mappingEntries, sizeof(mappingEntries));
      return crc.checksum();
   }

   void lpmUberBlock::refillChecksum()
   {
      SDB_ASSERT(isVaild(), "can not be invalid");
      checksum = generateChecksum();
      return;
   }

   BOOLEAN inspectUberBlockChecksum(const lpmUberBlock &ub)
   {
      return ub.isVaild() && (ub.checksum == ub.generateChecksum());
   }
} // namespace vessel

} // namespace engine
