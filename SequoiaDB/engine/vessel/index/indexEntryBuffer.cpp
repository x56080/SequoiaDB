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

   Source File Name = indexEntryBuffer.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/indexEntryBuffer.h"
#include "vessel/lsm/lsmIdxKey.hpp"
#include "ossLikely.hpp"
#include "pdTrace.hpp"
#include "vessel/btreeIndexDef.h"

namespace engine
{
namespace vessel
{
   indexEntryBuffer::~indexEntryBuffer()
   {
      _mb.release();
   }

   slice indexEntryBuffer::getEntry()const
   {
      return slice(_mb.getSize(), _mb.getBuffer());
   }

   INT32 indexEntryBuffer::save(INDEX_TYPE type,
                                const slice &entry)
   {
      INT32 rc = SDB_OK;
      UINT32 minEntrySize = 0;

      if (INDEX_TYPE_LSM == type)
      {
         minEntrySize = LSM_MIN_FULL_KEY_SIZE;
      }
      else if (INDEX_TYPE_BTREE == type)
      {
         minEntrySize = BTREE_MIN_ENTRY_SIZE;
      }
      else
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (entry.len() < minEntrySize)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _mb.copy(entry.len(), entry.data());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to save index entry:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   void indexEntryBuffer::reset()
   {
      _mb.resize(0);
   }
} // namespace vessel

} // namespace engine
