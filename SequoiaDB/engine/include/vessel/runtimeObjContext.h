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

   Source File Name = runtimeObjContext.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_RUNTIME_OBJ_CONTEXT_H_
#define VESSEL_RUNTIME_OBJ_CONTEXT_H_

#include "vessel/objectIdentifier.h"
#include "vessel/objectHolder.hpp"
#include "ossSharedLatch.hpp"

namespace engine
{
namespace vessel
{
   class collectionSpace;
   class collectionObject;

   class collectionSpaceContext : public SDBObject
   {
      public:
         collectionSpaceContext(){}
         collectionSpaceContext(const collectionSpaceId &csid,
                                ossSharedLatchMode mode,
                                objectHolder<collectionSpace> *holder);
         ~collectionSpaceContext();
         collectionSpaceContext(const collectionSpaceContext &) = delete;
         collectionSpaceContext &operator=(const collectionSpaceContext &) = delete;
         collectionSpaceContext(collectionSpaceContext &&o);
         collectionSpaceContext &operator=(collectionSpaceContext &&o);

      public:
         void close();
         OSS_INLINE BOOLEAN isOpen()const
         {
            return _csid.isValid() &&
                   !_mode.isNone();
         }

         OSS_INLINE const collectionSpaceId &getCSId()const
         {
            return _csid;
         }
         OSS_INLINE collectionSpace *getCollectionSpace()
         {
            return _holder->get();
         }

         OSS_INLINE ossSharedLatchMode getMode()const
         {
            return _mode;
         }

      private:
         collectionSpaceId _csid;
         ossSharedLatchMode _mode;
         objectHolder<collectionSpace> *_holder = nullptr;
   };//class collectionSpaceContext

   class collectionContext : public SDBObject
   {
      public:
         collectionContext();
         collectionContext(const collectionId &clid,
                           ossSharedLatchMode mode,
                           objectHolder<collectionObject> *holder);
         ~collectionContext();
         collectionContext(const collectionContext &) = delete;
         collectionContext &operator=(const collectionContext &) = delete;
         collectionContext(collectionContext &&o);
         collectionContext &operator=(collectionContext &&o);

      public:
         void close();

         OSS_INLINE BOOLEAN isOpen()const
         {
            return _clid.isValid() && !_mode.isNone();
         }

         OSS_INLINE const collectionId &getCollectionId()const
         {
            return _clid;
         }

         OSS_INLINE collectionObject *getCollectionObj()
         {
            return _holder->get();
         }

         OSS_INLINE ossSharedLatchMode getMode()const
         {
            return _mode;
         }

      private:
         collectionId _clid;
         ossSharedLatchMode _mode;
         objectHolder<collectionObject> *_holder = nullptr;

   };//class collectionContext
} // namespace vessel

} // namespace engine


#endif//VESSEL_RUNTIME_OBJ_CONTEXT_H_