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

   Source File Name = utilCRC.hpp

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

#ifndef UTIL_CRC_HPP_
#define UTIL_CRC_HPP_

#include "core.hpp"
#include "oss.hpp"
#include <boost/crc.hpp>

namespace engine
{
   INT32 utilCRC32(const CHAR *buf, UINT32 len, UINT32 &result)
   {
      INT32 rc = SDB_OK;
      boost::crc_32_type v;
      if (NULL == buf || 0 == len)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      v.process_bytes(buf, len);
      result = v.checksum();
   done:
      return rc;
   error:
      goto done;
   }
}


#endif//UTIL_CRC_HPP_