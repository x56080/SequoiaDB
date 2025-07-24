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

   Source File Name = vesselFileDef.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_VESSEL_FILE_DEF_H_
#define VESSEL_VESSEL_FILE_DEF_H_

#include "ossTypes.h"
#include "ossUtil.hpp"
#include "vessel/strSlice.h"


namespace engine
{
namespace vessel
{
   constexpr UINT32 MAX_FILE_NAME_LEN = 63;
   constexpr UINT32 MAX_SPACE_DIR_LEN = 15;

   constexpr CHAR * const FILE_MAGICAL_CHARS = "SDBV";
   constexpr UINT32 FILE_MAGICSAL_CHARS_LEN = ossStrlen(FILE_MAGICAL_CHARS);

   constexpr CHAR * const DIR_NAME_PREFIX = "_cs_";
   constexpr UINT32 DIR_NAME_PREFIX_LEN = 4;

   constexpr CHAR * const CSNAME_FILE_NAME = "CSNAME";
   constexpr CHAR * const MANIFEST_FILE_NAME = "MANIFEST";

   constexpr UINT16 INVALID_FILE_SHADOW_SUFFIX = 0xFFFF;
   constexpr UINT16 FILE_SHADOW_SUFFIX_TMP = 0;
   constexpr UINT16 FILE_SHADOW_SUFFIX_READY = 1;


   class fileTypeDescriptor
   {
      public:
         OSS_INLINE fileTypeDescriptor(){}
         OSS_INLINE fileTypeDescriptor(const CHAR *s):
         _fileTypeName(s)
         {}

         OSS_INLINE fileTypeDescriptor(const fileTypeDescriptor &o):
         _fileTypeName(o._fileTypeName)
         {}

         fileTypeDescriptor &operator=(const fileTypeDescriptor &o)
         {
            _fileTypeName = o._fileTypeName;
            return *this;
         }

         ~fileTypeDescriptor(){}

      public:
         OSS_INLINE const CHAR *getTypeName()const
         {
            return _fileTypeName;
         }
         OSS_INLINE BOOLEAN isValid()const
         {
            return nullptr != _fileTypeName;
         }

      private:
         const CHAR * _fileTypeName = nullptr;
   };//class fileDescriptor

   class spaceTypeDescriptor
   {
      public:
         OSS_INLINE spaceTypeDescriptor(){}
         OSS_INLINE ~spaceTypeDescriptor(){}
         OSS_INLINE spaceTypeDescriptor(const CHAR *s):
         _spaceTypeName(s)
         {}

         OSS_INLINE spaceTypeDescriptor(const spaceTypeDescriptor &o):
         _spaceTypeName(o._spaceTypeName)
         {}

         OSS_INLINE spaceTypeDescriptor &operator=(const spaceTypeDescriptor &o)
         {
            _spaceTypeName = o._spaceTypeName;
            return *this;
         }

         OSS_INLINE const CHAR *getTypeName()const
         {
            return _spaceTypeName;
         }

         OSS_INLINE BOOLEAN isValid()const
         {
            return nullptr != _spaceTypeName;
         }

      private:
         const CHAR *_spaceTypeName = nullptr;
   };//class spaceTypeDescriptor

   typedef UINT8 SPACE_TYPE;
   constexpr SPACE_TYPE INVALID_SPACE_TYPE = 255;
   constexpr SPACE_TYPE SPACE_TYPE_MAIN_DATA = 0;
   constexpr SPACE_TYPE SPACE_TYPE_IDX = 1;
   constexpr SPACE_TYPE SPACE_TYPE_LOB = 2;
   constexpr SPACE_TYPE MAX_SPACE_TYPE = SPACE_TYPE_LOB;

   typedef UINT8 FILE_TYPE;
   constexpr FILE_TYPE INVALID_FILE_TYPE = 255;
   constexpr FILE_TYPE FILE_TYPE_LPM = 0;
   constexpr FILE_TYPE FILE_TYPE_DATA_STORAGE = 1;
   constexpr FILE_TYPE FILE_TYPE_FSM = 2;
   constexpr FILE_TYPE FILE_TYPE_LOBM = 3;

   BOOLEAN parseFileType(const CHAR *typeSuffix,
                         FILE_TYPE &type,
                         fileTypeDescriptor *descriptor);

   BOOLEAN parseSpaceType(const CHAR *suffix,
                          SPACE_TYPE &type,
                          spaceTypeDescriptor *descriptor);

   BOOLEAN getFileTypeDescriptor(FILE_TYPE type,
                                 fileTypeDescriptor &descriptor);

   BOOLEAN getSpaceTypeDescriptor(SPACE_TYPE type,
                                  spaceTypeDescriptor &descriptor);

   UINT32 getShadowSuffixType(const CHAR *shadowSuffix);

   BOOLEAN getShadowSuffix(UINT16 t, strSlice &suffix);
}
}

#endif//VESSEL_VESSEL_FILE_DEF_H_