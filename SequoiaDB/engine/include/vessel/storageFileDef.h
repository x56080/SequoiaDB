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

   Source File Name = storageFileDef.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_STORAGE_FILE_DEF_H_
#define VESSEL_STORAGE_FILE_DEF_H_

#include "vessel/vesselFileDef.h"
#include "vessel/vesselIdDef.h"
#include "ossUtil.hpp"
#include "dms.hpp"
#include "vessel/strSlice.h"

namespace engine
{
namespace vessel
{
   static const UINT32 STORAGE_FILE_COMMON_HEAD_SIZE = 65536;
   static const UINT32 STORAGE_FILE_USER_DEFINED_HEAD_SIZE = 65536;
   static const UINT32 SOTRAGE_FILE_TOTAL_HEAD_SIZE = STORAGE_FILE_COMMON_HEAD_SIZE +
                                                      STORAGE_FILE_USER_DEFINED_HEAD_SIZE;

   static const UINT32 STORAGE_FILE_HEAD_VERSION = 1;

   static const UINT32 INVALID_FILE_HEAD_VERSION = 0;

   static const UINT64 STORAGE_FILE_SIZE = (UINT64(4) << 30); /// 4GB

   static const UINT32 STORAGE_FILE_SEGMENT_SIZE_2MB = ((UINT32)2 << 20);
   static const UINT32 STORAGE_FILE_SEGMENT_SIZE_32MB = ((UINT32)32 << 20);
   static const UINT32 STORAGE_FILE_SEGMENT_SIZE_128MB = ((UINT32)128 << 20);
   static const UINT32 STORAGE_FILE_SEGMENT_SIZE_256MB = ((UINT32)256 << 20);

   static const UINT64 STORAGE_FILE_INVALID_SEQUENCE = OSS_UINT64_MAX;

   BOOLEAN isValidSegmentSize(UINT32 size);

#pragma pack(4)

   struct storageCoreArgs
   {
      OSS_INLINE storageCoreArgs():
      pageSize(0),
      maxPageCountPerSeg(0),
      maxSegmentCountPerFile(0){}

      OSS_INLINE ~storageCoreArgs(){}

      OSS_INLINE storageCoreArgs(const storageCoreArgs &o):
      pageSize(o.pageSize),
      maxPageCountPerSeg(o.maxPageCountPerSeg),
      maxSegmentCountPerFile(o.maxSegmentCountPerFile)
      {}

      OSS_INLINE storageCoreArgs(UINT32 pageSize,
                                 UINT32 pgeCountPerSeg,
                                 UINT32 maxSegCount):
      pageSize(pageSize),
      maxPageCountPerSeg(pgeCountPerSeg),
      maxSegmentCountPerFile(maxSegCount)
      {
         
      }

      OSS_INLINE void reset()
      {
         pageSize = 0;
         maxPageCountPerSeg = 0;
         maxSegmentCountPerFile = 0;
      }

      BOOLEAN isValid()const;

      OSS_INLINE BOOLEAN operator==(const storageCoreArgs &o)const
      {
         return o.pageSize == pageSize &&
                o.maxPageCountPerSeg == maxPageCountPerSeg &&
                o.maxSegmentCountPerFile == maxSegmentCountPerFile;
      }

      OSS_INLINE BOOLEAN operator!=(const storageCoreArgs &o)const
      {
         return !(*this == o);
      }

      OSS_INLINE storageCoreArgs &operator=(const storageCoreArgs &o)
      {
         pageSize = o.pageSize;
         maxPageCountPerSeg = o.maxPageCountPerSeg;
         maxSegmentCountPerFile = o.maxSegmentCountPerFile;
         return *this;
      }

      OSS_INLINE UINT64 getMaxFileBodySize()const
      {
         return (UINT64)pageSize * getMaxPageCountInFile();
      }

      OSS_INLINE UINT32 getMaxPageCountInFile()const
      {
         return maxPageCountPerSeg * maxSegmentCountPerFile;
      }
      OSS_INLINE UINT64 getSegmentSize()const
      {
         return (UINT64)pageSize * maxPageCountPerSeg;
      }

      UINT32 pageSize;
      UINT32 maxPageCountPerSeg; /// define max mmap size
      UINT32 maxSegmentCountPerFile; /// define max file size
   };//struct storageCoreArgs

   struct createStorageFileOptions
   {
      OSS_INLINE createStorageFileOptions(){}
      OSS_INLINE ~createStorageFileOptions(){}

      createStorageFileOptions &operator=(const createStorageFileOptions &o)
      {
         dir = o.dir;
         secretValue = o.secretValue;
         args = o.args;
         replaceWhenCreate = o.replaceWhenCreate;
         createAsTmpFile = o.createAsTmpFile;
         return *this;
      }

      public:
      strSlice dir;
      UINT32 secretValue = 0;
      storageCoreArgs args;
      BOOLEAN replaceWhenCreate = FALSE;

      ///If "createAsTmpFile" is true, 
      ///file name will include tmp suffix.
      /// User should flush file before rename it to formal file name.
      /// WARNING: file with tmp suffix will be removed automaticly
      /// when startup.
      BOOLEAN createAsTmpFile = FALSE;
   }; // struct createStorageFileOptions


   /// common head
   struct storageFileHead
   {
      storageFileHead(){}
      ~storageFileHead(){}

      storageFileHead &operator=(const storageFileHead &o)
      {
         ossMemcpy(magicChars, o.magicChars, sizeof(storageFileHead));
         return *this;
      }

      void reset()
      {
         ossMemset(this, 0, sizeof(storageFileHead));
      }

      ///WARNING: If someone modified page head, remember to 
      /// update storageFile::createChecksum either.
      CHAR magicChars[4] = {0};
      UINT32 headChecksum = 0;
      UINT32 version = 0;
      CHAR name[MAX_FILE_NAME_LEN+1] = {0};
      UINT64 createTime = 0;
      UINT32 fingerprint = 0;
      UINT32 secretValue = 0;
      UINT64 flags = 0;
      UINT32 spaceID = 0;
      UINT32 spaceType = 0;
      UINT32 fileType = 0;
      //UINT32 logicalID = 0;
      UINT64 sequence = 0;
      UINT32 pageSize = 0;
      UINT32 maxPageCountPerSeg = 0;
      UINT32 maxSegmentCountPerFile = 0;
   }; // struct storageFileHead
   static const UINT32 STORAGE_FILE_HEAD_REAL_SIZE = sizeof(storageFileHead);

#pragma pack()
} /// end of namespace vessel
} /// end of namespace engine

#endif // VESSEL_STORAGE_FILE_DEF_H_
