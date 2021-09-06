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

   Source File Name = btreeIndexWriter.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_BTREE_INDEX_WRITER_H_
#define VESSEL_BTREE_INDEX_WRITER_H_

#include "vessel/btreeIndexAccessor.h"
#include "../bson/bson.hpp"
#include "vessel/recordID.h"
#include "dpsTransID.hpp"

namespace engine
{
namespace vessel
{
   class btreeIndexWriter : public btreeIndexAccessor
   {
      public:
         btreeIndexWriter();
         virtual ~btreeIndexWriter();

      public:
         INT32 init(requestContext *context,
                    indexContext *ic);

         void fini();

         INT32 insert(const bson::BSONObj &key,
                      const recordID &rid,
                      DPS_LSN_OFFSET lsn,
                      const DPS_TRANS_ID &transID);

   };//class btreeIndexWriter
} // namespace vessel

} // namespace engine

#endif//VESSEL_BTREE_INDEX_WRITER_H_