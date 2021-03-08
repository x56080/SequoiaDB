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

   Source File Name = collectionAllocator.h

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

#ifndef VESSEL_COLLECTION_ALLOCATOR_H_
#define VESSEL_COLLECTION_ALLOCATOR_H_

#include "ossLatch.hpp"
#include "vessel/collectionRecordPage.h"
#include "ossLatch.hpp"
#include "vessel/strSlice.h"
#include "vessel/vesselOptions.h"
#include "ossMemPool.hpp"
#include <map>


namespace engine
{
namespace vessel
{
   const UINT32 CL_MAP_TUPLE_COUNT_PER_PAGE = 64;
   class collection;

   class collectionAllocator : public SDBObject
   {
      public:
         collectionAllocator();
         ~collectionAllocator();

      public:
         class collectionHolder : public SDBObject
         {
            friend class collectionAllocator;
            public:
               collectionHolder()
               :_cl(NULL),
                _mbID(INVALID_CL_MB_ID),
                _flags(0)
               {}

               ~collectionHolder()
               {
                  releaseCLObj();
               }

            public:
               OSS_INLINE ossSpinSLatch *getMutex()
               {
                  return &_mutex;
               }
               OSS_INLINE collection *getCollection()
               {
                  return _cl;
               }
               OSS_INLINE CL_MB_ID getMBId()const
               {
                  return _mbID;
               }
               OSS_INLINE BOOLEAN isFree()const
               {
                  return NULL == _cl;
               }

            private:
               collection *allocateCLObj();
               void releaseCLObj();

            private:
               ossSpinSLatch _mutex;
               collection *_cl;
               CL_MB_ID _mbID;
               UINT16 _flags;
         };//struct collectionHolder

      private:
         struct _collectionCachePage : public SDBObject
         {
            _collectionCachePage():
            pageID(0),
            bitMap(0)
            {
            }

            UINT32 pageID;
            UINT64 bitMap;
            //UINT64 usageCount; for dynamic releasing page.
            collectionHolder slots[CL_MAP_TUPLE_COUNT_PER_PAGE];
         };//struct _collectionSlot

         //typedef std::map<UINT32, _collectionCachePage *> PAGE_MAP;
         typedef ossPoolMap<UINT32, _collectionCachePage *> PAGE_MAP;

      public:
         INT32 fini();
         INT32 allocateNewMB(CL_MB_ID &mbID, collectionHolder **holder);
         INT32 releaseMB(CL_MB_ID mbID);
         INT32 occupyMB(CL_MB_ID mbID, collectionHolder **holder);
         INT32 getHolder(CL_MB_ID mbID, collectionHolder *&holder);

      private:
         INT32 allocateNewPage();

         INT32 allocateMBFromNotFullPages(CL_MB_ID &mbID,
                                          collectionHolder **holder);

      private:
         ossSpinSLatch _mutex;
         UINT32 _pageCount;
         PAGE_MAP _pageMap;
         PAGE_MAP _notFullPages;

   };//namespace collectionAllocator
}//namespace vessel
}//namespace engine

#endif//VESSEL_COLLECTION_ALLOCATOR_H_