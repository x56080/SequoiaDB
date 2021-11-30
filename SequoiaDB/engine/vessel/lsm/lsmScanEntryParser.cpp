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

   Source File Name = lsmScanEntryParser.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/lsm/lsmScanEntryParser.h"

namespace engine
{
namespace vessel
{
   INT32 lsmScanEntryParser::parse(const slice &entryData)
   {
      INT32 rc = SDB_OK;
      rocksdb::Slice s(entryData.data(), entryData.getSize());
      rc = _lsmEntry.shallowCopy(s);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to parse full entry data:%d", rc);
         goto error;
      }

      if (!_lsmEntry.getRid().isValid() ||
           DPS_INVALID_LSN_OFFSET == _lsmEntry.getDataLsn() ||
           0 == _lsmEntry.getDataLsn())
      {
         PD_LOG(PDERROR, "invalid rid or lsn found in entry");
         rc = SDB_INVALIDARG;
         goto error;
      }

      _fullEntry = entryData;
   done:
      return rc;
   error:
      reset();
      goto done;
   }
} // namespace vessel

} // namespace engine
