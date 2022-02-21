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

   Source File Name = dataPageCluster.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_DATA_PAGE_CLUSTER_H_
#define VESSEL_DATA_PAGE_CLUSTER_H_

#include "vessel/pageDef.h"
#include "vessel/vesselFileDef.h"
#include "vessel/storageFileDef.h"
#include "vessel/mmapPagePointer.h"
#include "vessel/storageFileLoader.h"

namespace engine
{
namespace vessel
{
   class dataPageCluster : public SDBObject
   {
      public:
         dataPageCluster(){}
         virtual ~dataPageCluster(){}
         dataPageCluster(const dataPageCluster &) = delete;
         dataPageCluster &operator=(const dataPageCluster &) = delete;

      public:
         class options : public SDBObject
         {
            public:
               options(){}
               ~options(){}
               options(const options &o):
               segmentReusedMinFreePercent(o.segmentReusedMinFreePercent),
               segmentCountAutoExtending(o.segmentCountAutoExtending){}
               options &operator=(const options &o)
               {
                  segmentReusedMinFreePercent = o.segmentReusedMinFreePercent;
                  segmentCountAutoExtending = o.segmentCountAutoExtending;
                  return *this;
               }

            public:
               /// the segment will be reused only when hit min free percent
               FLOAT32 segmentReusedMinFreePercent = 0.10f;

               /// extend segments when failed to allocate free pages.
               INT32 segmentCountAutoExtending = 1;
         };//class options

      public:
         OSS_INLINE const storageCoreArgs &getCoreArgs()const
         {
            return _args;
         }
         OSS_INLINE BOOLEAN isOpen()const
         {
            return INVALID_SPACE_ID != _sid;
         }
         OSS_INLINE SPACE_ID getSpaceID()const
         {
            return _sid;
         }
         OSS_INLINE SPACE_TYPE getSpaceType()const
         {
            return _type;
         }
         OSS_INLINE UINT32 getSecretValue()const
         {
            return _secretValue;
         }

      public:
         virtual INT32 open(SPACE_ID sid,
                            SPACE_TYPE type,
                            UINT32 secretValue,
                            const storageFileLoader *loader, 
                            const storageCoreArgs &args,
                            const options &o) = 0;

         virtual void close() = 0;

         virtual void destroy() = 0;

         virtual INT32 allocatePages(UINT32 count,
                                     PAGE_ID *pids) = 0;

         virtual INT32 occupyPages(UINT32 count,
                                   const PAGE_ID *pids) = 0;

         virtual void releasePages(UINT32 count,
                                   const PAGE_ID *pids) = 0;

         virtual INT32 ensureSegmentCount(UINT32 totalSegmentCount) = 0;

         virtual INT32 ensurePidSpace(PAGE_ID pid) = 0;

      public:
         INT32 allocatePage(PAGE_ID &pid);
         INT32 occupyPage(PAGE_ID pid);
         void releasePage(PAGE_ID pid);

      public:
         virtual INT32 fsyncSegment(UINT32 globalSegmentId)const = 0;

         virtual INT32 fysncPage(PAGE_ID pid)const = 0;

         virtual INT32 getPagePtr(FILE_TYPE type,
                                  PAGE_ID pid,
                                  mmapPagePointer &ptr)const;

         virtual INT32 getDataPagePtr(PAGE_ID pid, mmapPagePointer &ptr)const = 0;

         virtual FILE_TYPE getDataFileType()const = 0;

         virtual UINT32 getTotalSegmentCount() = 0;
      
      protected:
         void _reset();

      protected:
         SPACE_ID _sid = INVALID_SPACE_ID;
         SPACE_TYPE _type = INVALID_SPACE_TYPE;
         UINT32 _secretValue = 0;
         storageCoreArgs _args;
         options _o;
   };//class dataPageCluster
}//namespace vessel
}//namespace engine

#endif//VESSEL_DATA_PAGE_CLUSTER_H_