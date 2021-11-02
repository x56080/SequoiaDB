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

   Source File Name = indexScanEntry.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/indexScanEntry.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"
#include "vessel/btreeScanEntryParser.h"
#include "vessel/lsm/lsmScanEntryParser.h"

namespace engine
{
namespace vessel
{
   INT32 indexScanEntry::init(INDEX_TYPE type, const slice &entryData)
   {
      if (INDEX_TYPE_BTREE == type)
      {
         btreeScanEntryParser parser;
         return init(entryData, &parser);
      }
      else if (INDEX_TYPE_LSM == type)
      {
         lsmScanEntryParser parser;
         return init(entryData, &parser);
      }
      else
      {
         return SDB_INVALIDARG;
      }
   }

   INT32 indexScanEntry::init(const slice &data,
                              indexScanEntryParser *parser)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(NULL == parser))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = parser->parse(data);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to parse entry data:%d", rc);
         goto error;
      }

      _type = parser->getType();
      _rid = parser->getRid();
      _transID = parser->getTransID();
      _keySlice = parser->getKeySlice();
      _entryData = data;
   done:
      return rc;
   error:
      reset();
      goto done;
   }

   void indexScanEntry::reset()
   {
      _type = INVALID_INDEX_TYPE;
      _rid = recordID();
      _transID.reset();
      _keySlice.reset();
      _entryData.reset();
      return;
   }
} // namespace vessel

} // namespace engine

