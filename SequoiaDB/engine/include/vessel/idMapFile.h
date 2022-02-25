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

   Source File Name = idMapFile.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_ID_MAP_FILE_H_
#define VESSEL_ID_MAP_FILE_H_

#include "vessel/storageFile.h"
#include "vessel/logicalPageSpaceCheckpoint.h"

namespace engine
{
namespace vessel
{
   constexpr UINT32 ID_MAP_FILE_HEAD_VERSION = 1;

#pragma pack(4)
   struct idMapFileHead
   {
      OSS_INLINE idMapFileHead(){}
      OSS_INLINE ~idMapFileHead(){}
      OSS_INLINE idMapFileHead(const idMapFileHead &o) = delete;

      OSS_INLINE idMapFileHead &operator=(const idMapFileHead &o)
      {
         version = o.version;
         flags = o.flags;
         dataPageSize = o.dataPageSize;
         dataPageCountInSeg = o.dataPageCountInSeg;
         dataSegCountInFile = o.dataSegCountInFile;
         totalPageCount = o.totalPageCount;
         checkpoint = o.checkpoint;
         return *this;
      }

      OSS_INLINE BOOLEAN isValid()const
      {
         return ID_MAP_FILE_HEAD_VERSION == version;
      }

      BOOLEAN hasSameArgs(const idMapFileHead &h)const
      {
         return flags == h.flags &&
                dataPageSize == h.dataPageSize &&
                dataPageCountInSeg == h.dataPageCountInSeg &&
                dataSegCountInFile == h.dataSegCountInFile;
      }

      UINT32 version = 0;
      UINT32 flags = 0;
      UINT32 dataPageSize = 0;
      UINT32 dataPageCountInSeg = 0;
      UINT32 dataSegCountInFile = 0;

      /// mutable fields
      UINT32 totalPageCount = 0;
      
      ///checkpoint
      LPS_CHECKPOINT checkpoint;

   };//struct idMapFileHead

#pragma pack()

   class idMapFile : public storageFile
   {
      public:
         idMapFile(){}
         virtual ~idMapFile(){}

      public:
         virtual BOOLEAN validateUserDefinedHead(const void *head)const override;
         virtual void cacheUserDefinedHead(const void *head) override;
         virtual void resetCachedUserDefinedHead() override;
         INT32 getIdMapFileHead(idMapFileHead &h)const;
         UINT32 getTotalPageCount()const {return _pageCount;}
      private:
         UINT32 _pageCount = 0;
   }; // class idMapFile
} // namespace vessel
} // namespace engine

#endif // VESSEL_ID_MAP_FILE_H_