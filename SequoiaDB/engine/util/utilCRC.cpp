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

   Source File Name = utilCRC.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/08/2021  LYC  Initial Draft

   Last Changed =

******************************************************************************/
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