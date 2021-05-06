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

namespace engine
{
namespace vessel
{
   const UINT32 STORAGE_FILE_HEAD_SIZE = 65536;

   const UINT32 STORAGE_FILE_HEAD_VERSION = 1;

   const UINT32 INVALID_FILE_HEAD_VERSION = 0;

   static const UINT64 STORAGE_FILE_SIZE = (UINT64(4) << 30); /// 4GB

   static const UINT32 STORAGE_FILE_SEGMENT_SIZE_2MB = 2;
   static const UINT32 STORAGE_FILE_SEGMENT_SIZE_32MB = 32;
   static const UINT32 STORAGE_FILE_SEGMENT_SIZE_128MB = 128;
   static const UINT32 STORAGE_FILE_SEGMENT_SIZE_256MB = 256;

   static const UINT64 STORAGE_FILE_INVALID_SEQUENCE = OSS_UINT64_MAX;

   BOOLEAN isValidSegmentSize(UINT32 size);
   

#pragma pack(4)

   struct storageCoreArgs
   {
      OSS_INLINE storageCoreArgs():
      pageSize(0),
      maxPageCountPerSeg(0),
      maxSegmentCountPerFile(0){}

      OSS_INLINE storageCoreArgs(UINT32 pageSize,
                                 UINT32 pgeCountPerSeg,
                                 UINT32 maxSegCount):
      pageSize(pageSize),
      maxPageCountPerSeg(pgeCountPerSeg),
      maxSegmentCountPerFile(maxSegCount)
      {
         
      }

      BOOLEAN isValid()const;

      OSS_INLINE BOOLEAN operator==(const storageCoreArgs &o)const
      {
         return o.pageSize == pageSize &&
                o.maxPageCountPerSeg == maxPageCountPerSeg &&
                o.maxSegmentCountPerFile == maxSegmentCountPerFile;
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
         UINT64 size = pageSize;
         size *= maxPageCountPerSeg;
         size *= maxSegmentCountPerFile;
         return size;
      }

      OSS_INLINE UINT32 getMaxPageCountInFile()const
      {
         return maxPageCountPerSeg * maxSegmentCountPerFile;
      }

      UINT32 pageSize;
      UINT32 maxPageCountPerSeg; /// define max mmap size
      UINT32 maxSegmentCountPerFile; /// define max file size
   };//struct storageCoreArgs

   struct storageFileOptions
   {
      OSS_INLINE storageFileOptions(){}
      OSS_INLINE ~storageFileOptions(){}

      storageFileOptions &operator=(const storageFileOptions &o)
      {
         dir = o.dir;
         name = o.name;
         secretValue = o.secretValue;
         spaceID = o.spaceID;
         logicalID = o.logicalID;
         sequence = o.sequence;
         args = o.args;
         replaceWhenCreate = o.replaceWhenCreate;
         delayFlushHead = o.delayFlushHead;
         return *this;
      }

      const CHAR * dir = NULL;
      const CHAR *name = NULL;
      UINT32 secretValue = 0;
      UINT16 spaceID = INVALID_SPACE_ID;
      UINT64 sequence = 0;
      UINT32 logicalID = DMS_INVALID_LOGICCSID;
      const storageCoreArgs *args = NULL;
      BOOLEAN replaceWhenCreate = FALSE;

      ///if "delayFlushHead" is false, file head will be flushed when created.
      ///else, file's status will be "in creating" until user modify it.
      ///WARNING: once open a file with flag "in creating", the file will be handled
      /// as a crashed one.
      BOOLEAN delayFlushHead = FALSE;
   }; // struct storageFileOptions

   static const UINT64 STORAGE_FILE_HEAD_FLAG_IN_CREATING = 0x01;

   /// common head
   struct storageFileHead
   {
      storageFileHead():
      version(INVALID_FILE_HEAD_VERSION),
      headChecksum(0),
      createTime(0),
      secretValue(0),
      flags(0),
      spaceID(INVALID_SPACE_ID),
      logicalID(DMS_INVALID_LOGICCSID),
      fileType(INVALID_FILE_TYPE),
      sequence(STORAGE_FILE_INVALID_SEQUENCE),
      pageSize(0),
      maxPageCountPerSeg(0),
      maxSegmentCountPerFile(0),
      userDefinedHeadLen(0)
      {
         ossMemset(magicChars, 0, sizeof(magicChars));
         ossMemset(name, 0, sizeof(name));
      }

      storageFileHead &operator=(const storageFileHead &o)
      {
         ossMemcpy(magicChars, o.magicChars, sizeof(storageFileHead));
         return *this;
      }

      CHAR magicChars[8];
      UINT32 version;
      UINT32 headChecksum;
      CHAR name[MAX_FILE_NAME_LEN+1];
      UINT64 createTime;
      UINT32 secretValue;
      UINT64 flags;
      UINT32 spaceID;
      UINT32 logicalID;
      UINT32 fileType;
      UINT64 sequence;
      UINT32 pageSize;
      UINT32 maxPageCountPerSeg;
      UINT32 maxSegmentCountPerFile;
      UINT32 userDefinedHeadLen;
   }; // struct storageFileHead
   static const UINT32 STORAGE_FILE_HEAD_REAL_SIZE = sizeof(storageFileHead);

   const UINT32 META_FILE_USER_HEAD_VERSION = 1;
   struct dataIDMapFileHead
   {
      OSS_INLINE dataIDMapFileHead():
      version(INVALID_FILE_HEAD_VERSION),
      headChecksum(0){}

      OSS_INLINE ~dataIDMapFileHead(){}

      OSS_INLINE dataIDMapFileHead &operator=(const dataIDMapFileHead &o)
      {
         version = o.version;
         headChecksum = o.headChecksum;
         meta = o.meta;
         data = o.data;
         indexMeta = o.indexMeta;
         index = o.index;
         return *this;
      }
      
      UINT32 version;
      UINT32 headChecksum;
      storageCoreArgs meta;
      storageCoreArgs data;
      storageCoreArgs indexMeta;
      storageCoreArgs index;
      
   };// struct dataIDMapFileHead

#pragma pack()
} /// end of namespace vessel
} /// end of namespace engine

#endif // VESSEL_STORAGE_FILE_DEF_H_
