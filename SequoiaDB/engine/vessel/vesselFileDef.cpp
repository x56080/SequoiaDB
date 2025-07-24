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

   Source File Name = vesselFileDef.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/vesselFileDef.h"
#include "ossUtil.hpp"
#include "pdTrace.hpp"
#include "ossLikely.hpp"
#include "vessel/strSlice.h"

namespace engine
{
namespace vessel
{    
   static const fileTypeDescriptor VFD_ARRAY[] =
   {
      {"lpm"},
      {"ds"},
      {"fsm"},
      {"lobm"},
   };

   static const spaceTypeDescriptor VSTD_ARRAY [] = 
   {
      {"data"},
      {"idx"},
      {"lob"}
   };

   static const strSlice SHADOW_SUFFIX_ARRAY [] = 
   {
      strSlice("_tmp"),
      strSlice("_ready")
   };

   BOOLEAN parseFileType(const CHAR *suffix,
                         FILE_TYPE &type,
                         fileTypeDescriptor *descriptor)
   {
      BOOLEAN r = FALSE;
      constexpr UINT32 _ARRAY_SIZE = sizeof(VFD_ARRAY) / sizeof(fileTypeDescriptor);

      if (OSS_UNLIKELY(NULL == suffix))
      {
         SDB_ASSERT(FALSE, "can not be null");
         goto done;
      }

      for (UINT32 i = 0; i < _ARRAY_SIZE; ++i)
      {
         const fileTypeDescriptor &vfd = VFD_ARRAY[i];
         SDB_ASSERT(NULL != vfd.getTypeName(), "can not be null");
         if (0 == ossStrcmp(suffix, vfd.getTypeName()))
         {
            type = i;
            if (NULL != descriptor)
            {
               *descriptor = vfd;
            }
            r = TRUE;
            goto done;
         }
      }
   done:
      return r;
   }

   BOOLEAN parseSpaceType(const CHAR *suffix,
                          SPACE_TYPE &type,
                          spaceTypeDescriptor *descriptor)
   {
      BOOLEAN r = FALSE;
      constexpr UINT32 _ARRAY_SIZE = sizeof(VSTD_ARRAY) / sizeof(spaceTypeDescriptor);
      SDB_ASSERT(NULL != suffix, "can not be null");
      if (OSS_UNLIKELY(NULL == suffix))
      {
         goto done;
      }

      for (UINT32 i = 0; i < _ARRAY_SIZE; ++i)
      {
         const spaceTypeDescriptor &d = VSTD_ARRAY[i];
         SDB_ASSERT(d.isValid(), "must be valid");
         if (0 == ossStrcmp(suffix, d.getTypeName()))
         {
            if (NULL != descriptor)
            {
               *descriptor = d;
            }
            type = i;
            r = TRUE;
            goto done;
         }
      }

   done:
      return r;
   }

   BOOLEAN getFileTypeDescriptor(FILE_TYPE type,
                                 fileTypeDescriptor &descriptor)
   {
      BOOLEAN r = FALSE;
      constexpr UINT32 _ARRAY_SIZE = sizeof(VFD_ARRAY) / sizeof(fileTypeDescriptor);
      if (OSS_UNLIKELY(INVALID_FILE_TYPE == type ||
                       _ARRAY_SIZE <= (UINT32)type))
      {
         goto done;
      }
      descriptor = VFD_ARRAY[type];
      r = TRUE;
   done:
      return r;
   }

   BOOLEAN getSpaceTypeDescriptor(SPACE_TYPE type,
                                  spaceTypeDescriptor &descriptor)
   {
      BOOLEAN r = FALSE;
      constexpr UINT32 _ARRAY_SIZE = sizeof(VSTD_ARRAY) / sizeof(spaceTypeDescriptor);

      if (OSS_UNLIKELY(INVALID_SPACE_TYPE == type ||
                       _ARRAY_SIZE <= type))
      {
         SDB_ASSERT(FALSE, "impossible");
         goto done;
      }

      descriptor = VSTD_ARRAY[type];
      r = TRUE;
   done:
      return r;
   }

   UINT32 getShadowSuffixType(const CHAR *shadowSuffix)
   {
      UINT16 t = INVALID_FILE_SHADOW_SUFFIX;
      SDB_ASSERT(NULL != shadowSuffix, "can not be null");
      constexpr UINT16 _SIZE = sizeof(SHADOW_SUFFIX_ARRAY) / sizeof(strSlice);
      if (OSS_UNLIKELY(NULL == shadowSuffix))
      {
         goto done;
      }
      for (UINT16 i = 0; i < _SIZE; ++i)
      {
         const strSlice &s = SHADOW_SUFFIX_ARRAY[i];
         if (0 == ossStrcmp(s.str(), shadowSuffix))
         {
            t = i;
            break;
         }
      }
   done:
      return t;
   }

   BOOLEAN getShadowSuffix(UINT16 t, strSlice &suffix)
   {
      BOOLEAN r = FALSE;
      constexpr UINT16 _SIZE = sizeof(SHADOW_SUFFIX_ARRAY) / sizeof(strSlice);
      if (INVALID_FILE_SHADOW_SUFFIX == t ||
          _SIZE <= t)
      {
         goto done;
      }

      suffix = SHADOW_SUFFIX_ARRAY[t];
      r = TRUE;
   done:
      return r;
   }

}//namespace vessel
}//namespace engine