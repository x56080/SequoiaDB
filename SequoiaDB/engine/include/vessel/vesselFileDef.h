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

   Source File Name = vesselFileDef.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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
   constexpr UINT32 FILE_MAGICSAL_CHARS_LEN = 4;

   constexpr CHAR * const DIR_NAME_PREFIX = "_cs_";
   constexpr UINT32 DIR_NAME_PREFIX_LEN = 4;

   constexpr CHAR * const CSNAME_FILE_NAME = "CSNAME";

   constexpr UINT16 INVALID_FILE_SHADOW_SUFFIX = 0xFFFF;
   constexpr UINT16 FILE_SHADOW_SUFFIX_TMP = 0;
   constexpr UINT16 FILE_SHADOW_SUFFIX_READY = 1;
   

   class VESSEL_FILE_GLOBAL_OPTIONS : public SDBObject
   {
      public:
         VESSEL_FILE_GLOBAL_OPTIONS(){}
         ~VESSEL_FILE_GLOBAL_OPTIONS()=delete;
      
      public:
         static void setSparseExtending(BOOLEAN allowed)
         {
            if (allowed)
            {
               OSS_BIT_CLEAR(_flags, FLAG_NOT_SPARSE_EXTENDING);
            }
            else
            {
               OSS_BIT_SET(_flags, FLAG_NOT_SPARSE_EXTENDING);
            }
         }
         static BOOLEAN isSparseExtending()
         {
            return 0 == OSS_BIT_TEST(_flags, FLAG_NOT_SPARSE_EXTENDING);
         }

      private:
         static constexpr UINT32 FLAG_NOT_SPARSE_EXTENDING = 0x01;

         static UINT32 _flags;
   };//class VESSEL_FILE_GLOBAL_OPTIONS

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
            return NULL != _fileTypeName;
         }

      private:
         const CHAR * _fileTypeName = NULL;
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
            return NULL != _spaceTypeName;
         }

      private:
         const CHAR *_spaceTypeName = NULL;
   };//class spaceTypeDescriptor

   typedef UINT8 SPACE_TYPE;
   constexpr SPACE_TYPE INVALID_SPACE_TYPE = 255;
   constexpr SPACE_TYPE SPACE_TYPE_MAIN_DATA = 0;
   constexpr SPACE_TYPE SPACE_TYPE_IDX = 1;
   constexpr SPACE_TYPE SPACE_TYPE_LOB = 2;
   constexpr SPACE_TYPE MAX_SPACE_TYPE = SPACE_TYPE_LOB;

   typedef UINT8 FILE_TYPE;
   constexpr FILE_TYPE INVALID_FILE_TYPE = 255;
   constexpr FILE_TYPE FILE_TYPE_ID_MAP = 0;
   constexpr FILE_TYPE FILE_TYPE_DATA_STORAGE = 1;
   constexpr FILE_TYPE FILE_TYPE_FSM = 2;
   constexpr FILE_TYPE FILE_TYPE_DELTA_LOG = 3;

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