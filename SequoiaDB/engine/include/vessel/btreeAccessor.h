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

#include "vessel/btreeAccessContext.h"

namespace engine
{
namespace vessel
{
   class indexObject;
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
            return nullptr != _is;
         }

         INT32 init(requestContext *context,
                    indexSpace *is,
                    indexObject *obj);
         void fini();

         INT32 insert(const ixmKey &key,
                      const recordID &rid);

         INT32 remove(const ixmKey &key,
                      const recordID &rid);

         INT32 truncate();

      private:/// writing

         INT32 traverseDownAndInsert(const ixmKey &key,
                                     const recordID &rid,
                                     BOOLEAN &obstructed);

         INT32 insertRaisedKeyRecursively(const btreeSplitRaisedKey &raisedKey);
         INT32 _initBtreeEntryAndRoot();

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

         INT32 insertWithRecreatingChild(const ixmKey &key,
                                         const recordID &rid,
                                         RECORD_SLOT_POS pos);

      private:
         INT32 traverseDownAndRemove(const ixmKey &key,
                                     const recordID &rid,
                                     BOOLEAN &obstructed);
 
         void tryToDestroyNodesIfNecessary(); 

         INT32 removeFromLeafPathEnd(const btreeItemLocation &location,
                                     BOOLEAN &obstructed);

         INT32 removeFromNonleafPathEnd(const btreeItemLocation &location,
                                        BOOLEAN &obstructed);

      private:
         INT32 removeBtreeRoot();

         INT32 releaseWholeTreeExceptRoot();

         INT32 releaseNonLeafNodeRecursively();

         INT32 seekChildToReleaseFirst(RECORD_SLOT_POS begin,
                                       RECORD_SLOT_POS &pos);

         INT32 atomicReleaseNonLeafPathEnd();

      private:
         indexSpace *_is = nullptr;
         indexObject *_obj = nullptr;
         btreeAccessContext _bac;
   };//class btreeAccessor
} // namespace vessel

} // namespace engine


#endif//VESSEL_BTREE_ACCESSOR_H_
