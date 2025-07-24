/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = storageFileDef.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_STORAGE_FILE_DEF_H_
#define VESSEL_STORAGE_FILE_DEF_H_

#include "vessel/vesselFileDef.h"
#include "vessel/vesselIdDef.h"
#include "ossUtil.hpp"
#include "dms.hpp"
#include "vessel/slice.h"

namespace engine
{
namespace vessel
{
   constexpr UINT32 STORAGE_FILE_COMMON_HEAD_SIZE = 32768;
   constexpr UINT32 STORAGE_FILE_USER_DEFINED_HEAD_SIZE = 32768;
   constexpr UINT32 SOTRAGE_FILE_TOTAL_HEAD_SIZE = STORAGE_FILE_COMMON_HEAD_SIZE +
                                                      STORAGE_FILE_USER_DEFINED_HEAD_SIZE;
   static_assert(65536 == SOTRAGE_FILE_TOTAL_HEAD_SIZE, "must be 64K");

   constexpr UINT32 STORAGE_FILE_HEAD_VERSION = 1;

   constexpr UINT32 INVALID_FILE_HEAD_VERSION = 0;

   constexpr UINT64 DATA_STORAGE_FILE_SIZE = (UINT64(4) << 30); /// 4GB
   
   constexpr UINT32 STORAGE_FILE_SEGMENT_SIZE_32MB = 32 << 20;

   /// max page count per segment
   constexpr UINT32 STORAGE_FILE_SEGMENT_MAX_PCNT = 32768;

   constexpr UINT32 DEFAULT_STORAGE_PAGE_SIZE = 65536;

#pragma pack(4)

   struct storageCoreArgs
   {
      storageCoreArgs() = default;
      explicit storageCoreArgs(UINT32 pageSize,
                               UINT32 pgeCountPerSeg,
                               UINT32 maxSegCount):
      pageSize(pageSize),
      maxPageCountPerSeg(pgeCountPerSeg),
      maxSegmentCountPerFile(maxSegCount)
      {
         
      }

      storageCoreArgs(const storageCoreArgs &) = default;
      storageCoreArgs &operator=(const storageCoreArgs &) = default;

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

      UINT32 pageSize = 0;
      UINT32 maxPageCountPerSeg = 0; /// define max mmap size
      UINT32 maxSegmentCountPerFile = 0; /// define max file size
   };//struct storageCoreArgs

   struct createStorageFileOptions
   {
      UINT32 secretValue = 0;
      storageCoreArgs args;
      BOOLEAN replaceWhenCreate = FALSE;

      ///If "createAsTmpFile" is true, 
      ///file name will include tmp suffix.
      /// User should flush file before rename it to formal file name.
      /// WARNING: file with tmp suffix will be removed automaticly
      /// when startup.
      BOOLEAN createAsTmpFile = FALSE;

      slice userDefinedHeader;

      UINT32 flags = 0;

   }; // struct createStorageFileOptions


   enum storageFileCtlFlag : INT32
   {
      MMAP_DATA_SEGMENT = 0x01,
   };//enum storageFileCtlFlag


   /// common head
   struct storageFileHead
   {
      void reset()
      {
         ossMemset(this, 0, sizeof(storageFileHead));
      }

      storageCoreArgs getCoreArgs()const
      {
         return storageCoreArgs(pageSize, maxPageCountPerSeg, maxSegmentCountPerFile);
      }

      UINT32 getSegmentSize()const
      {
         return pageSize * maxPageCountPerSeg;
      }
      UINT32 getMaxFileSize()const
      {
         return getSegmentSize() * maxSegmentCountPerFile;
      }

      ///WARNING: If someone modified page head, remember to 
      /// update storageFile::createChecksum either.
      CHAR magicChars[4] = {};
      UINT32 headChecksum = 0;
      UINT32 version = 0;
      CHAR name[MAX_FILE_NAME_LEN+1] = {};
      UINT64 createTime = 0;
      UINT32 fingerprint = 0;
      UINT32 secretValue = 0;
      UINT32 flags = 0;
      UINT32 pageSize = 0;
      UINT32 maxPageCountPerSeg = 0;
      UINT32 maxSegmentCountPerFile = 0;
      UINT32 reservedAreaSize = 0;/// the area between header and first data segment
   }; // struct storageFileHead
   constexpr UINT32 STORAGE_FILE_HEAD_REAL_SIZE = sizeof(storageFileHead);

#pragma pack()
} /// end of namespace vessel
} /// end of namespace engine

#endif // VESSEL_STORAGE_FILE_DEF_H_
