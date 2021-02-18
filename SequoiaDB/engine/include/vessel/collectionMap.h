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

   Source File Name = collectionMap.h

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

#ifndef VESSEL_COLLECTION_MAP_H_
#define VESSEL_COLLECTION_MAP_H_

#include "ossLatch.hpp"
#include "vessel/collectionRecordPage.h"
#include "ossLatch.hpp"
#include "vessel/strSlice.h"
#include "vessel/vesselOptions.h"
#include <map>


namespace engine
{
namespace vessel
{
   const UINT32 CL_MAP_TUPLE_COUNT_PER_PAGE = 64;
   class collection;
   class collectionSpace;

   class collectionMap : public SDBObject
   {
      public:
         collectionMap();
         ~collectionMap();

      public:
         class collectionHolder : public SDBObject
         {
            friend class collectionMap;
            public:
               collectionHolder()
               :_cl(NULL)
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

               OSS_INLINE BOOLEAN free()const
               {
                  return NULL == _cl;
               }

            private:
               INT32 allocateCLObj();

               INT32 releaseCLObj();

            private:
               ossSpinSLatch _mutex;
               collection *_cl;
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

         struct comp
         {
            BOOLEAN operator()(const CHAR *l, const CHAR *r)const
            {
               return ossStrcmp(l, r) < 0;
            }
         };//struct comp
         typedef std::map<UINT32, _collectionCachePage *> PAGE_MAP;
         typedef std::map<const CHAR *, CL_MB_ID, comp> NAME_INDEX;
         typedef std::map<UINT32, CL_MB_ID> ID_INDEX;

      public:
         INT32 teardown();
         BOOLEAN clExists(const strSlice &clName, UINT32 logicalID);

         INT32 allocateMBID(CL_MB_ID &mbID, collectionHolder **holder);

         INT32 occupyMBID(CL_MB_ID mbID, collectionHolder **holder);

         INT32 releaseMBID(CL_MB_ID mbID);

         INT32 upperBound(UINT32 logicalID,
                          CL_MB_ID &mbID,
                          collectionHolder **holder);

         /// must get collection exclusive lock first
         INT32 createCLObject(const strSlice &clName,
                              UINT32 logicalID,
                              CL_MB_ID mbID,
                              const createCLOptions &options,
                              collectionSpace *cs,
                              collectionHolder &holder);

         INT32 destoryCLObject(collectionHolder &holder);

         INT32 initObjWhenStartup(collectionSpace *cs,
                                  const collectionRecord &record);
      private:
         INT32 allocateNewPage();

         INT32 allocateMBFromNotFullPages(CL_MB_ID &mbID, collectionHolder **holder);

         INT32 insertIntoIndex(collection *clObj);
         INT32 eraseFromIndex(collection *clObj);

         INT32 getHolder(CL_MB_ID mbID, collectionHolder *&holder);
      private:
         ossSpinSLatch _mutex;
         UINT32 _pageCount;
         PAGE_MAP _pageMap;
         PAGE_MAP _notFullPages;
         NAME_INDEX _nameIndex;
         ID_INDEX _idIndex;

   };//namespace collectionMap
}//namespace vessel
}//namespace engine

#endif//VESSEL_COLLECTION_MAP_H_