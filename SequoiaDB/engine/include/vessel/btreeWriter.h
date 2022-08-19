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

   Source File Name = btreeWriter.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_BTREE_WRITER_H_
#define VESSEL_BTREE_WRITER_H_

#include "vessel/btreeAccessContext.h"
#include "vessel/btreeKeyStringEntry.h"
#include "dpsTransID.hpp"

namespace engine
{
namespace vessel
{
   class spacePteAccessCtx;

   class btreeWriter : public SDBObject
   {
      public:
         btreeWriter() = default;
         ~btreeWriter() = default;

      public:
         OSS_INLINE BOOLEAN isValid()const {return _bac.isValid();}
         
         INT32 init(requestContext *context,
                    indexSpace *is,
                    indexObject *obj,
                    spacePteAccessCtx *ac);
         void reset();

         INT32 insert(const btreeKeyStringEntry &entry,
                      DPS_LSN_OFFSET lsn,
                      const DPS_TRANS_ID &transID);

         INT32 remove(const btreeKeyStringEntry &entry,
                      DPS_LSN_OFFSET lsn,
                      const DPS_TRANS_ID &transID);

         INT32 truncate(BOOLEAN removeEntryPage);

         /// commit stats to btree entry page
         INT32 refreshEntryPage();

      private:/// writing
         INT32 _createBtreeRoot(BOOLEAN isLeaf);

         /// ensure other nodes in tree already been removed.
         INT32 _removeBtreeRoot();

         INT32 _insert(const btreeKeyStringEntry &entry);

         INT32 _insertRaisedKey(const btreeSplitRaisedKey &raisedEntry);

         INT32 _remove(const btreeKeyStringEntry &entry);

         INT32 _recreateChildAsLeaf(btreeNode &father,
                                    RECORD_SLOT_POS pos,
                                    PAGE_ID &child);

         INT32 _createNewRootToInsert(const btreeSplitRaisedKey &raisedEntry);

         INT32 _removeEntryFromPathEnd(RECORD_SLOT_POS pos);

         INT32 _destroyNodesIfNecessary();

         INT32 _destroyChildNodesRecursively();
     
      private:
         spacePteAccessCtx *_ac = nullptr;
         btreeAccessContext _bac;
   };//class btreeWriter
} // namespace vessel

} // namespace engine


#endif//VESSEL_BTREE_WRITER_H_
