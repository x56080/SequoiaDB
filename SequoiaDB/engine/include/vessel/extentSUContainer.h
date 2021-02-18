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

   Source File Name = extentSUContainer.h

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

#ifndef VESSEL_EXTENT_STORAGE_UNIT_CONTAINER_H_
#define VESSEL_EXTENT_STORAGE_UNIT_CONTAINER_H_

#include "vessel/phyExtentID.h"
#include "vessel/extentStorageUnit.h"
#include "vessel/vesselOptions.h"
#include "ossLatch.hpp"
#include <string>
#include <map>
#include <set>

namespace engine
{
namespace vessel
{
   class requestContext;
   
   class extentSUContainer
   {
      public:
         extentSUContainer();
         ~extentSUContainer();

      private:
         extentSUContainer(const extentSUContainer &o);
         extentSUContainer *operator=(const extentSUContainer &o);

      public:
         INT32 open(requestContext *context, BOOLEAN crashRecovery);
         INT32 close(requestContext *context);

         OSS_INLINE BOOLEAN isOpen()const
         {
            return _isOpen;
         }

         INT32 allocateSpaceID(SPACE_ID &sid);
         INT32 releaseSpaceID(SPACE_ID sid);

         INT32 createSU(requestContext *context,
                        const createSUOptions &options,
                        extentStorageUnit **su);

         INT32 dropSU(requestContext *context, BOOLEAN releaseSpaceID);

         /// space id must be locked in context
         INT32 getSUByContext(requestContext *context,
                              extentStorageUnit **su);

         /// will not hold any lock. user must guarantee su obj be safe.
         INT32 getSUBySpaceID(SPACE_ID sid, extentStorageUnit **su);

         /// WARNING: can not be used unless instance startup.
         /// sid it will be modified as next space id or invalid space id.
         /// simply you can quit loop by return pointer.
         SPACE_ID getFirstSpaceIDWhenStartup();
         extentStorageUnit *getNextSUWhenStartup(SPACE_ID &sid);

      private:
         INT32 createSUAtSpace(const CHAR *name, UINT32 logicalID,
                               SPACE_ID space, const createCSOptions &o);
         
         INT32 loadStorageUnits(requestContext *context);

         INT32 initFreeListAfterLoading(SPACE_ID maxID);

         INT32 initBitMapsOfSU(requestContext *context, BOOLEAN rebuild);
      private:

         class _spaceSlot : public SDBObject
         {
            public:
            OSS_INLINE _spaceSlot():
            su(NULL)
            {}
            OSS_INLINE ~_spaceSlot()
            {
               su = NULL;
            }

            OSS_INLINE BOOLEAN free()const
            {
               return NULL == su;
            }

            extentStorageUnit *su;
         };//_spaceSlot _spaceSlot

      private:
         BOOLEAN _isOpen;
         ossSpinSLatch _mutex;
         std::set<SPACE_ID> _free;
         SPACE_ID _minIDInPool;
         _spaceSlot *_suVec;
   };// class extentSUContainer
} /// end of namespace vessel
} /// end of namespace engine
#endif //VESSEL_EXTENT_STORAGE_UNIT_CONTAINER_H_
