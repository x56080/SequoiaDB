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

   Source File Name = btreeAccessor.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_BTREE_ACCESSOR_H_
#define VESSEL_BTREE_ACCESSOR_H_

#include "vessel/btreeNode.h"
#include "vessel/logicalPageBuffer.h"
#include "vessel/btreeAccessContext.h"

namespace engine
{
namespace vessel
{
   class indexContext;
   class requestContext;
   class indexSpace;

   class btreeAccessor : public SDBObject
   {
      public:
         btreeAccessor();
         virtual ~btreeAccessor();
         btreeAccessor(const btreeAccessor &) = delete;
         btreeAccessor &operator=(const btreeAccessor &) = delete;

      public:
         OSS_INLINE BOOLEAN isInitialized()const
         {
            return NULL != _context;
         }

         INT32 init(requestContext *context,
                     indexContext *ic);
         void fini();

         INT32 insert(const bson::BSONObj &key,
                      const recordID &rid,
                      const DPS_TRANS_ID &transID);

      protected:
         OSS_INLINE requestContext *getContext()
         {
            return _context;
         }
         OSS_INLINE indexSpace *getIndexSpace()
         {
            return _is;
         }
         OSS_INLINE indexContext *getIndexContext()
         {
            return _ic;
         }

      private:
         INT32 pushNoneRootNodeIntoPath(PAGE_ID lpid,
                                        const ossSharedLatchMode &mode,
                                        btreeAccessContext &bac);
         INT32 pushRootIntoPath(ossSharedLatchMode mode, btreeAccessContext &bac);

         INT32 traverseDownToInsert(btreeAccessContext &bac,
                                    BOOLEAN &obstructed);

      private:
         INT32 createRootIfNotExists();

         INT32 insertIntoLeafNode(btreeNode &node,
                                  const ixmKey &key,
                                  const recordID &rid,
                                  const DPS_TRANS_ID &transID,
                                  BOOLEAN &obstructed);

         INT32 splitLeafNodeAndInsert(btreeAccessContext &bac,
                                      BOOLEAN &obstructed);
      private:
         INT32 tryToSplitEndNode(btreeAccessContext &bac, BOOLEAN &obstructed);
         INT32 tryToSplitRootNode(btreeNode &root, BOOLEAN &obstructed);
          

      private:
         INT32 validateBtreePage(const logicalPageBuffer &buffer)const;
         ossSharedLatchMode estimateRootModeWhenWriting(UINT32 updatedTimes)const;
         ossSharedLatchMode estimateChildModeWhenInserting(btreeAccessContext &bac)const;

      private:
         requestContext *_context = NULL;
         indexSpace *_is = NULL;
         indexContext *_ic = NULL;
   };//class btreeAccessor
} // namespace vessel

} // namespace engine


#endif//VESSEL_BTREE_ACCESSOR_H_
