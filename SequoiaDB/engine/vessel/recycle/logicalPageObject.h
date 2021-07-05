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

   Source File Name = logicalPageObject.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_LOGICAL_PAGE_OBJECT_H_
#define VESSEL_LOGICAL_PAGE_OBJECT_H_

#include "vessel/pageIdentifier.h"
#include "vessel/vesselFileDef.h"
#include "vessel/lpidLockHelper.h"

namespace engine
{
namespace vessel
{
   class logicalPageObject : public SDBObject
   {
      public:
         logicalPageObject();
         ~logicalPageObject();
         logicalPageObject(const logicalPageObject &) = delete;
         logicalPageObject &operator=(const logicalPageObject &) = delete;

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return _pageId.isValid();
         }
         OSS_INLINE PAGE_SNAPSHOT_VERION getPsv()const
         {
            return _psv;
         }
         OSS_INLINE const mappedLogicalPageId &getPageId()const
         {
            return _pageId;
         }
         OSS_INLINE PAGE_ID getLpid()const
         {
            return _pageId.getLpid();
         }
         OSS_INLINE PAGE_ID getPid()const
         {
            return _pageId.getPid();
         }
         OSS_INLINE void *getMmapPtr()
         {
            return (void *)_mmapPtr;
         }
         OSS_INLINE const void *getMmapPtr()const
         {
            return (const void *)_mmapPtr;
         }

         void fini();

         INT32 init(PAGE_ID lpid,
                    PAGE_ID pid,
                    PAGE_SNAPSHOT_VERION psv,
                    ossValuePtr ptr);


      private:
         mappedLogicalPageId _pageId;
         PAGE_SNAPSHOT_VERION _psv = INVALID_PAGE_SNAPSHOT_VERSION;
         ossValuePtr _mmapPtr = 0;
   };//class logicalPageObject
}//namespace vessel
}//namespace engine

#endif//VESSEL_LOGICAL_PAGE_OBJECT_H_
