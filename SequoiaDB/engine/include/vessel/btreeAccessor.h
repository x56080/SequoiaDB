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
         ~btreeAccessor();
         btreeAccessor(const btreeAccessor &) = delete;
         btreeAccessor &operator=(const btreeAccessor &) = delete;

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return NULL != _context;
         }

         INT32 init(requestContext *context,
                     indexContext *ic);
         void fini();

         INT32 insert(const ixmKey &key,
                      const recordID &rid);

         INT32 remove(const ixmKey &key,
                      const recordID &rid);

      private:/// writing

         INT32 traverseDownAndInsert(const ixmKey &key,
                                     const recordID &rid,
                                     BOOLEAN &obstructed);

         INT32 insertRaisedKeyRecursively(const btreeSplitRaisedKey &raisedKey);
         INT32 createRootIfNotExists();

         INT32 insertWhenPathEndIsLeaf(const ixmKey &key,
                                       const recordID &rid,
                                       BOOLEAN &obstructed);

         

         INT32 splitAndInsertWhenPathEndIsLeaf(const ixmKey &key,
                                               const recordID &rid,
                                               BOOLEAN &obstructed);

         /// insert key and rid when raised key is null
         INT32 splitAndInsertWhenPathEndIsRoot(const ixmKey &key,
                                               const recordID &rid,
                                               const btreeSplitRaisedKey *raisedKey=NULL);

         /// also can not be root
         INT32 splitNonLeafPathEnd(BOOLEAN &obstructed);

      private:
         INT32 removeWhenPathEndIsLeaf(const ixmKey &key,
                                       const recordID &rid,
                                       BOOLEAN &obstructed);
         INT32 traverseDownAndRemove(const ixmKey &key,
                                     const recordID &rid,
                                     BOOLEAN &obstructed);

      private:
         requestContext *_context = NULL;
         indexSpace *_is = NULL;
         indexContext *_ic = NULL;
         btreeAccessContext _bac;
   };//class btreeAccessor
} // namespace vessel

} // namespace engine


#endif//VESSEL_BTREE_ACCESSOR_H_
