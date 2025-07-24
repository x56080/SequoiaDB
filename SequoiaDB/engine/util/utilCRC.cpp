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

   Source File Name = utilCRC.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/08/2021  LYC  Initial Draft

   Last Changed =

*******************************************************************************/
#include "utilCRC.hpp"
#include "pd.hpp"
#include <boost/crc.hpp>

namespace engine
{
   UINT32 utilCRC32(const void *buf, UINT32 len)
   {
      SDB_ASSERT(NULL != buf, "can not be null");
      SDB_ASSERT(0 != len, "can not be zero");

      boost::crc_32_type v;
      if (NULL == buf || 0 == len)
      {
         return 0;
      }
      v.process_bytes(buf, len);
      return v.checksum();
   }
}