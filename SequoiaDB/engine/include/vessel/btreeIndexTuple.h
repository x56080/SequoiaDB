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

   Source File Name = btreeIndexTuple.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_BTREE_INDEX_TUPLE_H_
#define VESSEL_BTREE_INDEX_TUPLE_H_

#include "vessel/btreeNodePage.h"

namespace engine
{
namespace vessel
{
   class btreeIndexTuple : public SDBObject
   {
      public:
         btreeIndexTuple(){}
         btreeIndexTuple(const btreeNodeSlot *slot,
                         const CHAR *data,
                         BOOLEAN isLeaf);
         ~btreeIndexTuple(){}

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return NULL != _slot;
         }

         void fini();

         BOOLEAN init(const btreeNodeSlot *slot,
                      const CHAR *data,
                      BOOLEAN isLeaf);

         PAGE_ID getLeftNode()const;

         OSS_INLINE const btreeNodeSlot *getSlot()const
         {
            return _slot;
         }
         OSS_INLINE UINT32 getKeyDataSize()const
         {
            return _keyDataSize;
         }
         OSS_INLINE BOOLEAN isLeaf()const
         {
            return _isLeaf;
         }

      private:
         const btreeNodeSlot *_slot = NULL;
         const CHAR *_data = NULL;
         UINT32 _keyDataSize = 0;
         BOOLEAN _isLeaf = FALSE;
   };//class btreeIndexTuple
} // namespace vessel

} // namespace engine

#endif//VESSEL_BTREE_INDEX_TUPLE_H_