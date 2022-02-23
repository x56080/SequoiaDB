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

   Source File Name = vesselFileDef.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/vesselFileDef.h"
#include "ossUtil.hpp"
#include "pdTrace.hpp"
#include "ossLikely.hpp"
#include "vessel/strSlice.h"

namespace engine
{
namespace vessel
{ 
   UINT32 VESSEL_FILE_GLOBAL_OPTIONS::_flags = 0;
   
   static const fileTypeDescriptor VFD_ARRAY[] =
   {
      {"idmap"},
      {"ds"},
      {"fsm"},
      {"delta"},
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