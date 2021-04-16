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

   Source File Name = extentStorageFile.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_EXTENT_STORAGE_FILE_H_
#define VESSEL_EXTENT_STORAGE_FILE_H_

#include "ossMmap.hpp"
#include "vessel/storageFileDef.h"
#include "vessel/pageDef.h"
#include "vessel/vesselIdDef.h"
#include "vessel/vesselFileName.h"

namespace engine
{
namespace vessel
{
   class extentStorageFile : public ossMmapFile
   {
      public:
         extentStorageFile();
         virtual ~extentStorageFile();

         extentStorageFile(const extentStorageFile &o) = delete;
         extentStorageFile &operator=(const extentStorageFile &o) = delete;
      public:
         INT32 create(const storageFileOptions &options,
                      const void *userDefinedOptions = NULL);

         INT32 open(const CHAR *fullPath, const vesselFileName &fn);

         INT32 destroy();
         INT32 close();
         BOOLEAN isOpen() const;

         INT32 allocateNewSegment(BOOLEAN sparse=FALSE);

         INT32 ensureSegmentCount(UINT32 count, BOOLEAN sparse=FALSE);

         INT32 getSegmentPtr(SEGMENT_ID seg, ossValuePtr &ptr);

         INT32 getExtentPtr(PAGE_ID page, ossValuePtr &ptr);

         INT32 getPagePtr(PAGE_ID page, ossValuePtr &ptr);

         INT32 fsync(PAGE_ID pid, UINT32 count, BOOLEAN sync=TRUE);

         OSS_INLINE const storageFileHead &getCommonHeadInMem()const
         {
            return _headInMem;
         }
         OSS_INLINE UINT32 getSegmentCount()const
         {
            return _dataSegmentCount;
         }

         OSS_INLINE UINT32 getMaxPageCountPerSeg()const
         {
            return _headInMem.maxPageCountPerSeg;
         }
         OSS_INLINE UINT32 getPageSize()const
         {
            return _headInMem.pageSize;
         }

      protected:
         INT32 getFileHeadPtr(ossValuePtr &ptr);

         INT32 getUserDefinedHeadPtr(ossValuePtr &ptr);

         INT32 createChecksum(const storageFileHead &head, UINT32 &checksum);
      
         INT32 createChecksum(const void *buf, UINT32 len, UINT32 &checksum);
      private:
         virtual FILE_TYPE getFileType()const = 0;
         virtual const CHAR *getMagicChars()const = 0;
         virtual BOOLEAN hasUserDefinedHead()const
         {
            return FALSE;
         }
         virtual INT32 initUserDefinedHead(const void *userDefinedOptions, CHAR *headBuf)
         {
            return SDB_VESSEL_INTERNAL_ERR;
         }
         virtual INT32 validateUserDefinedHead(const void *head)
         {
            return SDB_VESSEL_INTERNAL_ERR;
         }

      private:
         INT32 createFileAndInitHead(const storageFileOptions &options,
                                     const void *userDefinedOptions);

         INT32 openFileHead(const vesselFileName &fn);

         INT32 openFileSegments();

         BOOLEAN validateOptions(const storageFileOptions &options);
         INT32 initFileHead(const storageFileOptions &options,
                            CHAR *headBuf,
                            BOOLEAN hasUserDefinedHead);
         INT32 extendFileAndMMap(BOOLEAN sparse, UINT32 len, ossValuePtr *ptr);
         //INT32 initNewSegment(SEGMENT_ID sid, ossValuePtr ptr);
         INT32 validateHead(const void *head, const vesselFileName &fn);

      private:
         OSS_INLINE UINT32 getMMapSegmentID(SEGMENT_ID sid)const
         {
            return sid + getHeadMmapSegCount();
         }

         OSS_INLINE UINT32 getHeadMmapSegCount()const
         {
            return 0 == _headInMem.userDefinedHeadLen ? 1 : 2;
         }

         OSS_INLINE SEGMENT_ID getSegmentIDFromPageID(PAGE_ID page)
         {
            if (INVALID_PAGE_ID != page && 0 != _headInMem.maxPageCountPerSeg)
            {
               return page / _headInMem.maxPageCountPerSeg;
            }
            return INVALID_SEG_ID;
         }

      private:
         storageFileHead _headInMem;
         UINT32 _dataSegmentCount;
   }; // class extentStorageFile
} // namespace vessel
} // namespace engine

#endif // VESSEL_EXTENT_STORAGE_FILE_H_