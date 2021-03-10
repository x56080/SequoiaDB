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

#ifndef VESSEL_STORAGE_FILE_DEF_H_
#define VESSEL_STORAGE_FILE_DEF_H_

#include "ossTypes.h"
#include "ossUtil.hpp"
#include "dms.hpp"
#include "vessel/vesselDef.h"

namespace engine
{
namespace vessel
{
   const UINT32 STORAGE_FILE_HEAD_SIZE = 65536;
   const UINT32 INVALID_STORAGE_FILE_VERSION = 0;
   const UINT32 STORAGE_FILE_CURRENT_VERSION = 1;

   const UINT32 MAX_SU_DIR_LEN = 15;
   const UINT32 SU_FILE_NAME_LEN = 31;

   #define SU_FILE_NAME_PREFIX "$space_"
   const UINT32 SU_NAME_PREFIX_LEN = 7;
   #define SU_FILE_NAME_META_SUFFIX "meta"
   #define SU_FILE_NAME_IDX_SUFFIX "idx"
   #define SU_FILE_NAME_LOB_SUFFIX "lob"
   #define SU_FILE_NAME_CSNAME_SUFFIX "name"
   #define SU_FILE_NAME_INMEM_BITMAP_SUFFIX "imbm"
   #define SU_FILE_NAME_FSM_SG_SUFFIX "sg"
   #define SU_FILE_NAME_FSM_BITMAP_SUFFIX "fsm"

   const UINT32 INVALID_FILE_HEAD_VERSION = 0;

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
      storageFileOptions():
      dir(NULL),
      name(NULL),
      secretValue(0),
      spaceID(INVALID_SPACE_ID),
      sequence(0),
      args(NULL)
      {}

      const CHAR * dir;
      const CHAR *name;
      UINT32 secretValue;
      UINT16 spaceID;
      UINT32 sequence;
      const storageCoreArgs *args;
   }; // struct storageFileOptions

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
      uniqueID(UTIL_INVALID_CS_UNIQUE_ID),
      spaceType(INVALID_SPACE_TYPE),
      sequence(0),
      pageSize(0),
      maxPageCountPerSeg(0),
      maxSegmentCountPerFile(0),
      userDefinedHeadLen(0),
      lastSegmentSize(0)
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
      CHAR name[SU_FILE_NAME_LEN+1];
      UINT64 createTime;
      UINT32 secretValue;
      UINT32 flags;
      UINT32 spaceID;
      UINT32 uniqueID;
      UINT32 spaceType;
      UINT32 sequence;
      UINT32 pageSize;
      UINT32 maxPageCountPerSeg;
      UINT32 maxSegmentCountPerFile;
      UINT32 userDefinedHeadLen;
      UINT32 lastSegmentSize;
   }; // struct storageFileHead

   const UINT32 META_FILE_USER_HEAD_VERSION = 1;
   struct dataIDMapFileHead
   {
      UINT32 version;
      UINT32 headChecksum;
      storageCoreArgs meta;
      storageCoreArgs data;
      storageCoreArgs indexMeta;
      storageCoreArgs index;
      
   };// struct dataIDMapFileHead

} /// end of namespace vessel
} /// end of namespace engine

#endif // VESSEL_STORAGE_FILE_DEF_H_
