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

   Source File Name = lcChunkPageArray.cpp

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

#include "vessel/lcChunkPageArray.h"
#include "ossMem.hpp"
#include "ossLikely.hpp"
#include "pdTrace.hpp"

namespace engine
{
namespace vessel
{
   lcChunkPageArray::~lcChunkPageArray()
   {
      if (NULL != _pages && _pages != _static)
      {
         SDB_OSS_DEL []_pages;
      }
   }

   INT32 lcChunkPageArray::resize(UINT32 size)
   {
      INT32 rc = SDB_OK;
      lcChunkPage *tmp = NULL;
      if (size <= _size)
      {
         goto done;
      }

      tmp = SDB_OSS_NEW lcChunkPage[size];
      if (NULL == tmp)
      {
         PD_LOG(PDERROR, "failed to allcoate mem");
         rc = SDB_OOM;
         goto error;
      }

      for (UINT32 i = 0; i < _size; ++i)
      {
         tmp[i] = _pages[i];
      }

      if (_pages != _static)
      {
         SDB_OSS_DEL []_pages;
      }

      _pages = tmp;
      _size = size;
      tmp = NULL;
      
   done:
      if (NULL != tmp)
      {
         SDB_OSS_DEL []tmp;
      }
      return rc;
   error:
      goto done;
   }
} /// end of namespace vessel
} /// end of namespace engine