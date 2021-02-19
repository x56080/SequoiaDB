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

   Source File Name = objectContainer.h

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_OBJECT_CONTAINER_H_
#define VESSEL_OBJECT_CONTAINER_H_

#include "vessel/vesselDef.h"
#include "ossLatch.hpp"
#include "vessel/vesselOptions.h"
#include "vessel/requestContext.h"

namespace engine
{
namespace vessel
{
   class collectionSpace;
   class spaceIDLocker;
   class extentStorageUnit;
   class objectContainer : public SDBObject
   {
      public:
         objectContainer();
         ~objectContainer();

      public:
         INT32 setup();
         INT32 teardown();
         BOOLEAN csExists(const CHAR *name, UINT32 logicalID);

         INT32 getCSByLogicalID(requestContext *context,
                                UINT32 logicalID,
                                OSS_LATCH_MODE mode,
                                collectionSpace **obj);

         /// returns the first cs whose id is considered to go after logicalID.
         /// when logicalID is invalid, return first cs in index.
         INT32 getCSByUpperBoundLogicalID(requestContext *context,
                                          UINT32 logicalID,
                                          OSS_LATCH_MODE mode,
                                          collectionSpace **obj);

         INT32 allocateCSObj(requestContext *context,
                             const CHAR *name,
                             UINT32 logicalID,
                             collectionSpace **obj);

         INT32 allocateCSObjWhenStartup(requestContext *context,
                                        extentStorageUnit *su,
                                        collectionSpace **obj);

         INT32 releaseCSObj(requestContext *context);

         

         UINT32 getNameCountInIndex();

      private:
          /// after we get a space id from index,
         /// cs may be dropped or recreated.
         /// user must check name or logical id again.
         INT32 getCSUnderIDLocked(requestContext *context,
                                  collectionSpace **obj);
         INT32 upperBoundLogicalID(UINT32 logicalID, UINT32 &next);
         INT32 getSpaceIDFromIndex(const CHAR *name, SPACE_ID &sid);
         INT32 getSpaceIDFromIndex(UINT32 logicalID, SPACE_ID &sid);
         INT32 addToIndex(const CHAR *name, UINT32 logicalID, SPACE_ID sid);
         void eraseFromIndex(const CHAR *name, UINT32 logicalID);

      private:
         /*struct comp
         {
            BOOLEAN operator()(const CHAR *l, const CHAR *r)const
            {
               return ossStrcmp(l, r) < 0;
            }
         };//struct comp
         */
         typedef std::map<std::string, SPACE_ID> NAME_INDEX;
         typedef std::map<UINT32, SPACE_ID> ID_INDEX;

         struct _csSlot : public SDBObject
         {
            _csSlot():cs(NULL){}
            collectionSpace *cs;
            OSS_INLINE BOOLEAN isFree()const
            {
               return NULL == cs;
            }
         };//struct _csSlot

      private:
         ossSpinSLatch _mutex;
         _csSlot *_csVec;
         NAME_INDEX _nameIndex;
         ID_INDEX _idIndex;

   };//class objectContainer
}//namespace vessel
}//namespace engine

#endif//VESSEL_OBJECT_CONTAINER_H_